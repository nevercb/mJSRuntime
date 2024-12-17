#include <stdio.h>
#include <stdlib.h>
#include "quickjs.h"
#include "event_loop.h"
#include "console.h"
#include "require.h"

// 主程序入口
int main(int argc, char **argv) {
    // 检查命令行参数是否提供了脚本文件
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <script.js>\n", argv[0]);
        return 1;
    }
    const char *script_file = argv[1];

    // 创建 QuickJS 运行时和上下文
    JSRuntime *runtime = JS_NewRuntime();
    if (!runtime) {
        fprintf(stderr, "Failed to create JS runtime\n");
        return 1;
    }

    JSContext *ctx = JS_NewContext(runtime);
    if (!ctx) {
        fprintf(stderr, "Failed to create JS context\n");
        JS_FreeRuntime(runtime);
        return 1;
    }

    // 注册 console 模块
    register_console(ctx);

    // 注册 require 函数
    register_require(ctx);

    // 注册 setTimeout 和 clearTimeout
    register_global_functions(ctx);

    // 读取脚本文件内容
    FILE *file = fopen(script_file, "r");
    if (!file) {
        perror("Failed to open script file");
        JS_FreeContext(ctx);
        JS_FreeRuntime(runtime);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *script = malloc(file_size + 1);
    if (!script) {
        perror("Failed to allocate memory for script");
        fclose(file);
        JS_FreeContext(ctx);
        JS_FreeRuntime(runtime);
        return 1;
    }

    fread(script, 1, file_size, file);
    script[file_size] = '\0';
    fclose(file);

    // 执行脚本
    JSValue result = JS_Eval(ctx, script, file_size, script_file, JS_EVAL_TYPE_GLOBAL);
    free(script);

    if (JS_IsException(result)) {
        // 打印异常信息
        JSValue exception = JS_GetException(ctx);
        const char *error = JS_ToCString(ctx, exception);
        fprintf(stderr, "Script exception: %s\n", error);
        JS_FreeCString(ctx, error);
        JS_FreeValue(ctx, exception);
        JS_FreeValue(ctx, result);
        JS_FreeContext(ctx);
        JS_FreeRuntime(runtime);
        return 1;
    }

    JS_FreeValue(ctx, result);

    // 执行事件循环
    printf("Starting event loop...\n");
    execute_pending_jobs(runtime); // 处理所有 Promise 回调
    event_loop_with_io(runtime, -1); // -1 表示没有额外的文件描述符需要监听

    // 释放 QuickJS 运行时和上下文
    JS_FreeContext(ctx);
    JS_FreeRuntime(runtime);

    return 0;
}