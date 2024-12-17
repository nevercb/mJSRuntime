#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include "quickjs.h"

// 声明事件循环相关函数
void execute_pending_jobs(JSRuntime *rt);
void event_loop_with_io(JSRuntime *rt, int fd);
void register_global_functions(JSContext *ctx);

#endif // EVENT_LOOP_H