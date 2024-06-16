#include <cstddef>
#include "json.h"
#include "memalloc.h"

static void *yyjson_malloc(void *ctx, size_t size) {
    return _malloc_base(size);
}

static void *yyjson_realloc(void *ctx, void *ptr, size_t old_size, size_t size) {
    return _realloc_base(ptr, size);
}

static void yyjson_free(void *ctx, void *ptr) {
    _free_base(ptr);
}

const yyjson_alc YYJSON_ALLOCATOR = {
    yyjson_malloc,
    yyjson_realloc,
    yyjson_free,
    NULL
};
