
#include <node.h>

#include <clocale>

#include "cluster.hpp"

void InitAll(v8::Handle<v8::Object> exports, v8::Handle<v8::Object> module) 
{
    std::setlocale(LC_ALL, "en_US.UTF-8");

    qdb::Cluster::Init(exports);
    qdb::Blob::Init(exports);
    qdb::Integer::Init(exports);
    qdb::Queue::Init(exports);
    qdb::Set::Init(exports);
    qdb::Tag::Init(exports);
}

NODE_MODULE(qdb, InitAll)
