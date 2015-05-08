
#include <node.h>

#include "cluster.hpp"

void InitAll(v8::Handle<v8::Object> exports, v8::Handle<v8::Object> module) 
{
    qdb::Cluster::Init(exports);
    qdb::Blob::Init(exports);
    qdb::Integer::Init(exports);
    qdb::Queue::Init(exports);
    qdb::Set::Init(exports);
}

NODE_MODULE(qdb, InitAll)
