#include "quickjs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 初始化 QuickJS 运行时
int main(int argc, char **argv) {
    // 创建 JS 运行时和上下文
    JSRuntime *runtime = JS_NewRuntime();
    JSContext *ctx = JS_NewContext(runtime);

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
    if (JS_IsException(result)) {
        JSValue exception = JS_GetException(ctx);
        const char *error = JS_ToCString(ctx, exception);
        printf("Error: %s\n", error);
        JS_FreeCString(ctx, error);
        JS_FreeValue(ctx, exception);
    }

    JS_FreeValue(ctx, result);
    free(script);

    // 释放资源
    JS_FreeContext(ctx);
    JS_FreeRuntime(runtime);

    return 0;
}