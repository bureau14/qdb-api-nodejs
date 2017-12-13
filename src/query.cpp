
#include "query.hpp"
#include "cluster.hpp"

namespace quasardb
{

v8::Persistent<v8::Function> Query::constructor;

void Query::New(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    Cluster::newObject<Query>(args);
}

} // namespace quasardb
