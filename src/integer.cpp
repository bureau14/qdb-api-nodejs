
#include "integer.hpp"
#include "cluster.hpp"

namespace quasardb
{

v8::Persistent<v8::Function> Integer::constructor;

void Integer::New(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    Cluster::newObject<Integer>(args);
}
}