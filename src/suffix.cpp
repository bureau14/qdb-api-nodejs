#include "cluster.hpp"
#include "suffix.hpp"

namespace quasardb
{

v8::Persistent<v8::Function> Suffix::constructor;

void Suffix::New(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    Cluster::newObject<Suffix>(args);
}

} // namespace quasardb
