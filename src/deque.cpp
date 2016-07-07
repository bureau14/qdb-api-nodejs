
#include "deque.hpp"
#include "cluster.hpp"

namespace quasardb
{

v8::Persistent<v8::Function> Deque::constructor;

void Deque::New(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    Cluster::newObject<Deque>(args);
}
}
