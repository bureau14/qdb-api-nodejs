
#include "queue.hpp"
#include "cluster.hpp"

namespace qdb
{

v8::Persistent<v8::Function> Queue::constructor;

void Queue::New(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    Cluster::newObject<Queue>(args);
}

}
