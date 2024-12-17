#ifndef MODULE_CACHE_H
#define MODULE_CACHE_H

#include "quickjs.h"

// 模块缓存结构
typedef struct ModuleCache {
    char *filename;           // 模块文件名
    JSValue exports;          // 模块的导出对象
    struct ModuleCache *next; // 链表用于处理冲突
} ModuleCache;

// 查找缓存中的模块
ModuleCache *find_cached_module(const char *filename);

// 将模块添加到缓存
void add_module_to_cache(JSContext *ctx, const char *filename, JSValue exports);

// 清空模块缓存
void free_module_cache(JSContext *ctx);

#endif