
#include "tag.hpp"
#include "cluster.hpp"

namespace qdb
{

v8::Persistent<v8::Function> Tag::constructor;

void Tag::New(const v8::FunctionCallbackInfo<v8::Value> & args) { Cluster::newObject<Tag>(args); }

} // namespace qdb
