
#include "blob.hpp"
#include "cluster.hpp"

namespace quasardb
{

v8::Persistent<v8::Function> Blob::constructor;

void Blob::New(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    Cluster::newObject<Blob>(args);
}

} // namespace quasardb