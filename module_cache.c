#include <stdlib.h>
#include "quickjs.h"
#include <string.h>
#include "module_cache.h"

// 全局模块缓存表
static ModuleCache *module_cache = NULL;

// 查找缓存中的模块
ModuleCache *find_cached_module(const char *filename) {
    ModuleCache *current = module_cache;
    while (current) {
        if (strcmp(current->filename, filename) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// 将模块添加到缓存
void add_module_to_cache(JSContext *ctx, const char *filename, JSValue exports) {
    ModuleCache *new_module = malloc(sizeof(ModuleCache));
    new_module->filename = strdup(filename);
    new_module->exports = JS_DupValue(ctx, exports); // 增加引用计数
    new_module->next = module_cache;
    module_cache = new_module;
}

// 清空模块缓存
void free_module_cache(JSContext *ctx) {
    ModuleCache *current = module_cache;
    while (current) {
        ModuleCache *next = current->next;
        // Debug: 打印模块文件名和引用计数
        printf("Freeing module: %s\n", current->filename);
        printf("exports ref count before free: %d\n", JS_VALUE_GET_TAG(current->exports));

        // 释放 filename
        free(current->filename);

        // 释放 exports
        JS_FreeValue(ctx, current->exports);

        // 释放当前模块
        free(current);

        current = next;
    }
    module_cache = NULL;
}