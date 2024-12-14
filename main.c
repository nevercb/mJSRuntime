#include <stdio.h>
#include <stdlib.h>
#include "quickjs.h"
#include "console.h"
#include "require.h"
#include "module_cache.h"

int main(int argc, char **argv) {
    // 创建 QuickJS 运行时和上下文
    JSRuntime *runtime = JS_NewRuntime();
    JSContext *ctx = JS_NewContext(runtime);

    // 注册 console 和 require
    register_console(ctx);
    register_require(ctx);

    // 检查是否提供了 JavaScript 文件
    if (argc < 2) {
        printf("Usage: %s <script.js>\n", argv[0]);
        return 1;
    }

    // 读取 JavaScript 文件
    const char *filename = argv[1];
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *script = malloc(file_size + 1);
    fread(script, 1, file_size, file);
    script[file_size] = '\0';
    fclose(file);

    // 执行 JavaScript 文件
    JSValue result = JS_Eval(ctx, script, file_size, filename, JS_EVAL_TYPE_GLOBAL);
    free(script);

    if (JS_IsException(result)) {
        JSValue exception = JS_GetException(ctx);
        const char *error = JS_ToCString(ctx, exception);
        printf("Error: %s\n", error);
        JS_FreeCString(ctx, error);
        JS_FreeValue(ctx, exception);
    }

    // 释放运行结果
    JS_FreeValue(ctx, result);

    // 清空模块缓存
    free_module_cache(ctx);

    JS_RunGC(runtime);
    // 释放 QuickJS 运行时和上下文
    JS_FreeContext(ctx);
    JS_FreeRuntime(runtime);

    return 0;
}