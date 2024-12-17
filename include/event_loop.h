#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include "quickjs.h"
#include <time.h>

// 定义最大任务数
#define MAX_TASKS 256
#define MAX_ASYNC_TASKS 256

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

// 添加任务到任务队列
// 参数：ctx - JavaScript 上下文，func - JavaScript 回调函数，delay - 延迟时间（毫秒）
// 返回：任务 ID，如果失败则返回 -1
int add_task(JSContext *ctx, JSValue func, int delay);

// 根据任务 ID 移除任务
// 参数：id - 要移除的任务 ID
void remove_task(int id);

// 添加异步任务到异步任务队列
// 参数：ctx - JavaScript 上下文，callback - 回调函数，arg - 回调参数
void add_async_task(JSContext *ctx, void (*callback)(JSContext *ctx, void *arg), void *arg);

// 执行到期的定时器任务
void execute_tasks(void);

// 执行所有异步任务
void execute_async_tasks(JSContext *ctx);

// 执行所有挂起的 Promise 回调任务
// 参数：rt - QuickJS 运行时
void execute_pending_jobs(JSRuntime *rt);

// 事件循环，与文件描述符关联
// 参数：rt - QuickJS 运行时，fd - 文件描述符（如果没有文件描述符，则传入 -1）
void event_loop_with_io(JSRuntime *rt, int fd);

// 注册全局 JavaScript 函数（如 setTimeout 和 clearTimeout）
// 参数：ctx - JavaScript 上下文
void register_global_functions(JSContext *ctx);

#endif // EVENT_LOOP_H