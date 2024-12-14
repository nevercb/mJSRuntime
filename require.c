#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quickjs.h"
#include "require.h"
#include "module_cache.h"

// 包装模块代码为函数
static char *wrap_module_code(const char *script) {
    size_t script_len = strlen(script);
    char *wrapped_script = malloc(script_len + 64);
    sprintf(wrapped_script, "(function(module, exports) { %s\n})", script);
    return wrapped_script;
}

// require 函数实现
static JSValue js_require(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "require() expects a module filename");
    }

    const char *filename = JS_ToCString(ctx, argv[0]);
    if (!filename) {
        return JS_ThrowTypeError(ctx, "require() expects a string");
    }

    // 检查模块缓存
    ModuleCache *cached = find_cached_module(filename);
    if (cached) {
        JS_FreeCString(ctx, filename);
        return JS_DupValue(ctx, cached->exports); // 返回缓存的 exports
    }

    // 读取模块文件
    FILE *file = fopen(filename, "r");
    if (!file) {
        JS_FreeCString(ctx, filename);
        return JS_ThrowReferenceError(ctx, "Module not found: %s", filename);
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *script = malloc(file_size + 1);
    fread(script, 1, file_size, file);
    script[file_size] = '\0';
    fclose(file);

    // 为模块创建独立作用域
    JSValue module_obj = JS_NewObject(ctx);
    JSValue exports_obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, module_obj, "exports", JS_DupValue(ctx, exports_obj));

    printf("module_obj tag: %d\n", JS_VALUE_GET_TAG(module_obj)); 
    // 包装模块代码为函数
    char *wrapped_script = malloc(file_size + 64);
    sprintf(wrapped_script, "(function(module, exports) { %s\n})", script);
    free(script);

    JSValue func = JS_Eval(ctx, wrapped_script, strlen(wrapped_script), filename, JS_EVAL_TYPE_GLOBAL);
    free(wrapped_script);
    if (JS_IsException(func)) {
        JS_FreeCString(ctx, filename);
        JS_FreeValue(ctx, module_obj);
        JS_FreeValue(ctx, exports_obj);
        return func; // 返回异常
    }

    // 调用包装函数
    JSValue args[2] = {module_obj, exports_obj};
    JSValue result = JS_Call(ctx, func, JS_UNDEFINED, 2, args);
    JS_FreeValue(ctx, func);

    if (JS_IsException(result)) {
        JS_FreeCString(ctx, filename);
        JS_FreeValue(ctx, module_obj);
        JS_FreeValue(ctx, exports_obj);
        return result; // 返回异常
    }
    JS_FreeValue(ctx, result);

    // 从 module.exports 获取导出的值
    JSValue exports = JS_GetPropertyStr(ctx, module_obj, "exports");
    JS_FreeValue(ctx, module_obj);

    // 缓存模块
    add_module_to_cache(ctx, filename, exports); // 缓存模块的 exports
    JS_FreeCString(ctx, filename);

    return exports; // 返回模块的导出值
}

// 注册 require 函数
void register_require(JSContext *ctx) {
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global_obj, "require", JS_NewCFunction(ctx, js_require, "require", 1));
    JS_FreeValue(ctx, global_obj);
}