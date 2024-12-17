#include "event_loop.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/select.h>
#include <pthread.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <cerrno>

// 定义任务结构
typedef struct {
    JSValue func;                 // JavaScript 回调函数
    JSContext *ctx;               // JavaScript 上下文
    int delay;                    // 延迟时间（毫秒）
    struct timespec execute_time; // 任务的执行时间点
    int id;                       // 任务 ID
} Task;

// 定义异步任务结构
typedef struct {
    void (*callback)(JSContext *ctx, void *arg); // 回调函数
    void *arg;                                  // 回调参数
} AsyncTask;

// 定义 openFile 异步任务数据结构
typedef struct {
    int fd;                // 文件描述符
    char *buffer;          // 文件内容缓存
    size_t buffer_size;    // 缓存大小
    JSValue callback;      // JavaScript 回调函数
    JSContext *ctx;        // JavaScript 上下文
} OpenFileTask;

// 最大任务数
#define MAX_TASKS 256
#define MAX_ASYNC_TASKS 256

// 任务队列
static Task task_queue[MAX_TASKS];
static int task_count = 0;

// 异步任务队列
static AsyncTask async_task_queue[MAX_ASYNC_TASKS];
static int async_task_count = 0;

// 用于生成任务 ID
static int next_task_id = 1;

// 异步任务队列锁
static pthread_mutex_t async_task_mutex = PTHREAD_MUTEX_INITIALIZER;

// 添加任务到任务队列
int add_task(JSContext *ctx, JSValue func, int delay) {
    if (task_count >= MAX_TASKS) {
        printf("Task queue overflow!\n"); // 超出最大任务数
        return -1;
    }

    // 获取当前时间
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    // 计算任务的执行时间
    struct timespec execute_time = {
        .tv_sec = now.tv_sec + delay / 1000,
        .tv_nsec = now.tv_nsec + (delay % 1000) * 1000000,
    };
    if (execute_time.tv_nsec >= 1000000000) {
        execute_time.tv_sec += 1;
        execute_time.tv_nsec -= 1000000000;
    }

    // 创建任务
    task_queue[task_count].ctx = ctx;
    task_queue[task_count].func = JS_DupValue(ctx, func);
    task_queue[task_count].delay = delay;
    task_queue[task_count].execute_time = execute_time;
    task_queue[task_count].id = next_task_id++;

    return task_queue[task_count++].id; // 返回任务 ID
}

// 移除任务
void remove_task(int id) {
    for (int i = 0; i < task_count; i++) {
        if (task_queue[i].id == id) {
            // 释放任务的 JavaScript 函数引用
            JS_FreeValue(task_queue[i].ctx, task_queue[i].func);
            // 将后续任务向前移动，覆盖当前任务
            for (int j = i; j < task_count - 1; j++) {
                task_queue[j] = task_queue[j + 1];
            }
            task_count--;
            return;
        }
    }
}

// 添加异步任务到异步任务队列
void add_async_task(JSContext *ctx, void (*callback)(JSContext *ctx, void *arg), void *arg) {
    pthread_mutex_lock(&async_task_mutex);
    if (async_task_count >= MAX_ASYNC_TASKS) {
        printf("Async task queue overflow!\n");
        pthread_mutex_unlock(&async_task_mutex);
        return;
    }
    async_task_queue[async_task_count].callback = callback;
    async_task_queue[async_task_count].arg = arg;
    async_task_count++;
    pthread_mutex_unlock(&async_task_mutex);
}

// 执行异步任务
void execute_async_tasks(JSContext *ctx) {
    pthread_mutex_lock(&async_task_mutex);
    for (int i = 0; i < async_task_count; i++) {
        AsyncTask *task = &async_task_queue[i];
        task->callback(ctx, task->arg);
    }
    async_task_count = 0; // 清空异步任务队列
    pthread_mutex_unlock(&async_task_mutex);
}

// 执行到期任务
void execute_tasks() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    for (int i = 0; i < task_count; i++) {
        Task *task = &task_queue[i];
        // 检查任务是否到期
        if (task->execute_time.tv_sec < now.tv_sec ||
            (task->execute_time.tv_sec == now.tv_sec && task->execute_time.tv_nsec <= now.tv_nsec)) {

            // 调用任务的 JavaScript 函数
            JSValue result = JS_Call(task->ctx, task->func, JS_UNDEFINED, 0, NULL);
            if (JS_IsException(result)) {
                JSValue exception = JS_GetException(task->ctx);
                const char *error = JS_ToCString(task->ctx, exception);
                printf("Task execution failed: %s\n", error);
                JS_FreeCString(task->ctx, error);
                JS_FreeValue(task->ctx, exception);
            }
            JS_FreeValue(task->ctx, result);

            // 释放任务的 JavaScript 函数引用
            JS_FreeValue(task->ctx, task->func);
            // 将后续任务向前移动，覆盖当前任务
            for (int j = i; j < task_count - 1; j++) {
                task_queue[j] = task_queue[j + 1];
            }
            task_count--;
            i--; // 更新任务索引
        }
    }
}

// 执行所有挂起的 Promise 回调任务
void execute_pending_jobs(JSRuntime *rt) {
    JSContext *ctx;
    int ret;

    while (true) {
        ret = JS_ExecutePendingJob(rt, &ctx);
        if (ret <= 0) {
            break; // 没有挂起任务或出错
        }
    }
}

// openFile 的异步回调
void open_file_callback(JSContext *ctx, void *arg) {
    OpenFileTask *task = (OpenFileTask *)arg;

    // 调用 JavaScript 回调函数
    JSValue result = JS_NewString(ctx, task->buffer);
    JSValue callback_result = JS_Call(ctx, task->callback, JS_UNDEFINED, 1, &result);

    // 检查是否发生异常
    if (JS_IsException(callback_result)) {
        JSValue exception = JS_GetException(ctx);
        const char *error = JS_ToCString(ctx, exception);
        printf("openFile callback failed: %s\n", error);
        JS_FreeCString(ctx, error);
        JS_FreeValue(ctx, exception);
    }

    // 释放资源
    JS_FreeValue(ctx, callback_result);
    JS_FreeValue(ctx, task->callback);
    free(task->buffer);
    close(task->fd);
    free(task);
}

// JavaScript 的 openFile 实现
static JSValue js_open_file(JSContext *ctx, JSValueConst this_val,
                            int argc, JSValueConst *argv) {
    if (argc < 2 || !JS_IsFunction(ctx, argv[1])) {
        return JS_ThrowTypeError(ctx, "Invalid arguments: Expected file path and callback function");
    }

    const char *file_path = JS_ToCString(ctx, argv[0]);
    if (!file_path) {
        return JS_ThrowTypeError(ctx, "Invalid file path");
    }

    // 打开文件（非阻塞模式）
    int fd = open(file_path, O_RDONLY | O_NONBLOCK);
    JS_FreeCString(ctx, file_path);

    if (fd < 0) {
        return JS_ThrowInternalError(ctx, "Failed to open file: %s", strerror(errno));
    }

    // 创建异步任务
    OpenFileTask *task = malloc(sizeof(OpenFileTask));
    task->fd = fd;
    task->buffer = malloc(1024); // 假设最大文件大小为 1024 字节
    task->buffer_size = 1024;
    task->callback = JS_DupValue(ctx, argv[1]); // 保存 JavaScript 回调
    task->ctx = ctx;

    // 将文件描述符添加到异步任务队列
    add_async_task(ctx, open_file_callback, task);

    return JS_UNDEFINED;
}

// 事件循环，与文件描述符关联
void event_loop_with_io(JSRuntime *rt, int fd) {
    fd_set read_fds;
    struct timeval timeout;

    while (task_count > 0 || async_task_count > 0) {
        FD_ZERO(&read_fds);

        // 添加文件描述符到 select 监听
        if (fd >= 0) {
            FD_SET(fd, &read_fds);
        }

        timeout.tv_sec = 0;
        timeout.tv_usec = 1000; // 1 毫秒的超时时间

        // 检查文件描述符是否有可读数据
        int ret = select(fd + 1, &read_fds, NULL, NULL, &timeout);
        if (ret > 0 && fd >= 0 && FD_ISSET(fd, &read_fds)) {
            execute_async_tasks(rt);
        }

        // 执行到期任务
        execute_tasks();
        // 执行挂起的 Promise 回调任务
        execute_pending_jobs(rt);
        // 执行异步任务
        execute_async_tasks(rt);
    }
}

// 注册全局 JavaScript 函数
void register_global_functions(JSContext *ctx) {
    JSValue global_obj = JS_GetGlobalObject(ctx);

    // 注册 setTimeout 函数
    JS_SetPropertyStr(ctx, global_obj, "setTimeout",
                      JS_NewCFunction(ctx, js_set_timeout, "setTimeout", 2));
    // 注册 clearTimeout 函数
    JS_SetPropertyStr(ctx, global_obj, "clearTimeout",
                      JS_NewCFunction(ctx, js_clear_timeout, "clearTimeout", 1));
    // 注册 openFile 函数
    JS_SetPropertyStr(ctx, global_obj, "openFile",
                      JS_NewCFunction(ctx, js_open_file, "openFile", 2));

    JS_FreeValue(ctx, global_obj);
}