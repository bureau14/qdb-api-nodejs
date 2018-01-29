
#include "query_find.hpp"
#include "cluster.hpp"

namespace quasardb
{

v8::Persistent<v8::Function> QueryFind::constructor;

void QueryFind::New(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    Cluster::newObject<QueryFind>(args);
}

} // namespace quasardb
