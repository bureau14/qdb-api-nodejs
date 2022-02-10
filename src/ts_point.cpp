#include "ts_point.hpp"

namespace quasardb
{

v8::Persistent<v8::Function> DoublePoint::constructor;
v8::Persistent<v8::Function> BlobPoint::constructor;
v8::Persistent<v8::Function> StringPoint::constructor;
v8::Persistent<v8::Function> Int64Point::constructor;
v8::Persistent<v8::Function> TimestampPoint::constructor;

} // namespace quasardb
