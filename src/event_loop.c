#include "event_loop.h"
#include "quickjs.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <sys/select.h>

// 定义任务结构
typedef struct {
    JSValue func;
    JSContext *ctx;
    int delay;
    struct timespec execute_time;
    int id;
} Task;

// 最大任务数
#define MAX_TASKS 256
static Task task_queue[MAX_TASKS];
static int task_count = 0;
static int next_task_id = 1;

// 添加任务到队列
int add_task(JSContext *ctx, JSValue func, int delay) {
    if (task_count >= MAX_TASKS) {
        printf("Task queue overflow!\n");
        return -1;
    }

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    struct timespec execute_time = {
        .tv_sec = now.tv_sec + delay / 1000,
        .tv_nsec = now.tv_nsec + (delay % 1000) * 1000000,
    };
    if (execute_time.tv_nsec >= 1000000000) {
        execute_time.tv_sec += 1;
        execute_time.tv_nsec -= 1000000000;
    }

    task_queue[task_count].ctx = ctx;
    task_queue[task_count].func = JS_DupValue(ctx, func);
    task_queue[task_count].delay = delay;
    task_queue[task_count].execute_time = execute_time;
    task_queue[task_count].id = next_task_id++;

    return task_queue[task_count++].id;
}

// 移除任务
void remove_task(int id) {
    for (int i = 0; i < task_count; i++) {
        if (task_queue[i].id == id) {
            JS_FreeValue(task_queue[i].ctx, task_queue[i].func);
            for (int j = i; j < task_count - 1; j++) {
                task_queue[j] = task_queue[j + 1];
            }
            task_count--;
            return;
        }
    }
}

// 执行任务
void execute_tasks() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    for (int i = 0; i < task_count; i++) {
        Task *task = &task_queue[i];
        if (task->execute_time.tv_sec < now.tv_sec ||
            (task->execute_time.tv_sec == now.tv_sec && task->execute_time.tv_nsec <= now.tv_nsec)) {

            JSValue result = JS_Call(task->ctx, task->func, JS_UNDEFINED, 0, NULL);
            if (JS_IsException(result)) {
                JSValue exception = JS_GetException(task->ctx);
                const char *error = JS_ToCString(task->ctx, exception);
                printf("Task execution failed: %s\n", error);
                JS_FreeCString(task->ctx, error);
                JS_FreeValue(task->ctx, exception);
            }
            JS_FreeValue(task->ctx, result);

            JS_FreeValue(task->ctx, task->func);
            for (int j = i; j < task_count - 1; j++) {
                task_queue[j] = task_queue[j + 1];
            }
            task_count--;
            i--;
        }
    }
}

// 执行 Promise 回调
void execute_pending_jobs(JSRuntime *rt) {
    JSContext *ctx;
    int ret;

    while (true) {
        ret = JS_ExecutePendingJob(rt, &ctx);
        if (ret <= 0) {
            break;
        }
    }
}

// 事件循环
void event_loop_with_io(JSRuntime *rt, int fd) {
    fd_set read_fds;
    struct timeval timeout;

    while (task_count > 0) {
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);

        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;

        int ret = select(fd + 1, &read_fds, NULL, NULL, &timeout);
        if (ret > 0 && FD_ISSET(fd, &read_fds)) {
            char buffer[1024];
            int n = read(fd, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                buffer[n] = '\0';
                printf("Read from fd: %s\n", buffer);
            }
        }

        execute_tasks();
        execute_pending_jobs(rt);
    }
}

// 注册全局函数
static JSValue js_set_timeout(JSContext *ctx, JSValueConst this_val,
                              int argc, JSValueConst *argv) {
    if (argc < 2 || !JS_IsFunction(ctx, argv[0]) || !JS_IsNumber(argv[1])) {
        return JS_ThrowTypeError(ctx, "Invalid arguments");
    }

    JSValue func = argv[0];
    int delay;
    if (JS_ToInt32(ctx, &delay, argv[1])) {
        return JS_ThrowTypeError(ctx, "Invalid delay");
    }

    int task_id = add_task(ctx, func, delay);
    return JS_NewInt32(ctx, task_id);
}

static JSValue js_clear_timeout(JSContext *ctx, JSValueConst this_val,
                                int argc, JSValueConst *argv) {
    if (argc < 1 || !JS_IsNumber(argv[0])) {
        return JS_ThrowTypeError(ctx, "Invalid arguments");
    }

    int id;
    if (JS_ToInt32(ctx, &id, argv[0])) {
        return JS_ThrowTypeError(ctx, "Invalid timeout ID");
    }

    remove_task(id);
    return JS_UNDEFINED;
}

void register_global_functions(JSContext *ctx) {
    JSValue global_obj = JS_GetGlobalObject(ctx);

    JS_SetPropertyStr(ctx, global_obj, "setTimeout",
                      JS_NewCFunction(ctx, js_set_timeout, "setTimeout", 2));
    JS_SetPropertyStr(ctx, global_obj, "clearTimeout",
                      JS_NewCFunction(ctx, js_clear_timeout, "clearTimeout", 1));

    JS_FreeValue(ctx, global_obj);
}