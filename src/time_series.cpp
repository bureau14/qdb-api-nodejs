
#include "time_series.hpp"
#include "cluster.hpp"

namespace quasardb
{

v8::Persistent<v8::Function> TimeSeries::constructor;

void TimeSeries::New(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    Cluster::newObject<TimeSeries>(args);
}
}
