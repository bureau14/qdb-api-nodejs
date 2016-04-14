#include "range.hpp"
#include "cluster.hpp"

namespace qdb
{

v8::Persistent<v8::Function> Range::constructor;

void Range::New(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    Cluster::newObject<Range>(args);
}

} // namespace qdb
