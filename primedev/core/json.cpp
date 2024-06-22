#include <cstddef>
#include <yyjson.h>
#include "json.h"
#include "memalloc.h"

using namespace yyjson;

static void *yyjson_malloc(void *ctx, size_t size)
{
    return _malloc_base(size);
}

static void *yyjson_realloc(void *ctx, void *ptr, size_t old_size, size_t size)
{
    return _realloc_base(ptr, size);
}

static void yyjson_free(void *ctx, void *ptr)
{
    _free_base(ptr);
}

const yyjson_alc YYJSON_ALLOCATOR = {
    yyjson_malloc,
    yyjson_realloc,
    yyjson_free,
    NULL
};

Document::~Document()
{
    if (is_valid())
        yyjson_doc_free(doc);
}

void Document::read(const char* data, size_t length)
{
    yyjson_read_flag flg = YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS;

    // yyjson expects a char* only for the opts variant.
    // We can safely ignore that just like yyjson_read() does.
    doc = yyjson_read_opts(const_cast<char*>(data), length, flg, &YYJSON_ALLOCATOR, &err);
}

MutDocument::MutDocument()
{
    doc = yyjson_mut_doc_new(&YYJSON_ALLOCATOR);
}

MutDocument::MutDocument(Document& idoc)
{
    doc = yyjson_doc_mut_copy(idoc.get_doc(), &YYJSON_ALLOCATOR);
}


MutDocument::~MutDocument()
{
    if (is_valid())
        yyjson_mut_doc_free(doc);
}
