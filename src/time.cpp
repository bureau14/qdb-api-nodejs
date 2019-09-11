#include "time.hpp"

namespace quasardb
{
v8::Persistent<v8::Function> Timestamp::constructor;
v8::Persistent<v8::FunctionTemplate> Timestamp::tmpl;

} // namespace quasardb
