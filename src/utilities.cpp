#include <array>
#include <utility>

#include "utilities.hpp"

namespace quasardb
{

template <>
std::array<v8::Local<v8::Value>, 1> CopyArguments<1>(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    return {{args[0]}};
}

template <>
std::array<v8::Local<v8::Value>, 2> CopyArguments<2>(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    return {{args[0], args[1]}};
}

} // quasardb namespace
