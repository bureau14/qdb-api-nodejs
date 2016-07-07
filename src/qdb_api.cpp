
#include <node.h>

#include <clocale>

#include "cluster.hpp"

void InitConstants(v8::Local<v8::Object> exports)
{
    v8::Isolate * isolate = exports->GetIsolate();

    exports->ForceSet(v8::String::NewFromUtf8(isolate, "NEVER_EXPIRES"), v8::Int32::New(isolate, qdb_never_expires),
                      v8::ReadOnly);
    exports->ForceSet(v8::String::NewFromUtf8(isolate, "PRESERVE_EXPIRATION"),
                      v8::Int32::New(isolate, qdb_preserve_expiration), v8::ReadOnly);

    exports->ForceSet(v8::String::NewFromUtf8(isolate, "COMPRESSION_NONE"), v8::Int32::New(isolate, qdb_comp_none),
                      v8::ReadOnly);
    exports->ForceSet(v8::String::NewFromUtf8(isolate, "COMPRESSION_FAST"), v8::Int32::New(isolate, qdb_comp_fast),
                      v8::ReadOnly);
    exports->ForceSet(v8::String::NewFromUtf8(isolate, "COMPRESSION_BEST"), v8::Int32::New(isolate, qdb_comp_best),
                      v8::ReadOnly);
}

void InitAll(v8::Local<v8::Object> exports, v8::Local<v8::Object> module)
{
    std::setlocale(LC_ALL, "en_US.UTF-8");

    quasardb::Cluster::Init(exports);

    quasardb::Error::Init(exports);

    quasardb::Blob::Init(exports);
    quasardb::Deque::Init(exports);
    quasardb::Integer::Init(exports);
    quasardb::Prefix::Init(exports);
    quasardb::Range::Init(exports);
    quasardb::Set::Init(exports);
    quasardb::Tag::Init(exports);

    InitConstants(exports);
}

NODE_MODULE(quasardb, InitAll)
