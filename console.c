#include <stdio.h>
#include "quickjs.h"

// 实现 console.log
static JSValue js_console_log(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    for (int i = 0; i < argc; i++) {
        const char *str = JS_ToCString(ctx, argv[i]);
        if (str) {
            if (i > 0) {
                printf(" "); // 在多个参数之间添加空格
            }
            printf("%s", str);
            JS_FreeCString(ctx, str);
        }
    }
    printf("\n"); // 换行
    return JS_UNDEFINED;
}

// 注册 console 到全局对象
void register_console(JSContext *ctx) {
    // 创建 console 对象
    JSValue console = JS_NewObject(ctx);

    // 添加 log 方法
    JS_SetPropertyStr(ctx, console, "log",
                      JS_NewCFunction(ctx, js_console_log, "log", 1));

    // 添加更多方法（例如 console.error）
    JS_SetPropertyStr(ctx, console, "error",
                      JS_NewCFunction(ctx, js_console_log, "error", 1)); // 这里简单实现与 log 相同

    // 将 console 对象挂载到全局对象上
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global_obj, "console", console);
    JS_FreeValue(ctx, global_obj);
}