#pragma once
#include <cstddef>
#include "yyjson.h"

namespace yyjson
{
    class Document
    {
    private:
        yyjson_doc* doc;
        yyjson_read_err err;

    public:
        Document() { doc = nullptr; };
        Document(yyjson_doc* idoc) { doc = idoc; }
        Document(std::string data) { read(data); }
        Document(const char* data) { read(data, strlen(data)); }
        Document(const char* data, size_t size) { read(data, size); }
        ~Document();

        void read(std::string data) { return read(const_cast<char*>(data.c_str()), data.length()); }
        void read(const char* data) { return read(data, strlen(data)); }
        void read(const char*, size_t);

        bool is_valid() { return doc != nullptr; }
        yyjson_doc* get_doc() { return doc; }
        yyjson_read_err& get_err() { return err; }
        yyjson_val* get_root() { return yyjson_doc_get_root(doc); }
    };

    class MutDocument
    {
    private:
        yyjson_mut_doc* doc;

    public:
        MutDocument();
        MutDocument(Document&);
        ~MutDocument();

        bool is_valid() { return doc != nullptr; }
        yyjson_mut_doc* get_doc() { return doc; }
        yyjson_mut_val* get_root() { return yyjson_mut_doc_get_root(doc); }
    };

}