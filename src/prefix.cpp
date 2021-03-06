#include "prefix.hpp"
#include "cluster.hpp"

namespace quasardb
{

v8::Persistent<v8::Function> Prefix::constructor;

void Prefix::New(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    Cluster::newObject<Prefix>(args);
}

} // namespace quasardb
