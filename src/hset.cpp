
#include "hset.hpp"
#include "cluster.hpp"

namespace quasardb
{

v8::Persistent<v8::Function> Set::constructor;

void Set::New(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    Cluster::newObject<Set>(args);
}
}
