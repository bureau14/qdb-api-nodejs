#pragma once

#include <array>

#include <qdb/client.h>
#include <node.h>
#include <node_object_wrap.h>

namespace quasardb
{

inline qdb_timespec_t ms_to_qdb_timespec(double ms)
{
    auto ns = ms * 1000000ull;

    qdb_timespec_t ts;
    ts.tv_sec = static_cast<qdb_time_t>(ns / 1000000000ull);
    ts.tv_nsec = static_cast<qdb_time_t>(ns - ts.tv_sec * 1000000000ull);

    return ts;
}

inline double qdb_timespec_to_ms(const qdb_timespec_t & ts)
{
    return static_cast<double>(ts.tv_sec) * 1000.0 + static_cast<double>(ts.tv_nsec / 1000000ull);
}

class Timestamp : public node::ObjectWrap
{
public:
    static void Init(v8::Local<v8::Object> exports)
    {
        auto isolate = exports->GetIsolate();
        auto tmpl = v8::FunctionTemplate::New(isolate, New);
        tmpl->SetClassName(v8::String::NewFromUtf8(isolate, "Timestamp"));
        tmpl->InstanceTemplate()->SetInternalFieldCount(1);
        tmpl->InstanceTemplate()->SetAccessor(v8::String::NewFromUtf8(isolate, "seconds"), GetProperty);
        tmpl->InstanceTemplate()->SetAccessor(v8::String::NewFromUtf8(isolate, "nanoseconds"), GetProperty);

        tmpl->Set(v8::String::NewFromUtf8(isolate, "fromDate"), v8::FunctionTemplate::New(isolate, FromDate));
        NODE_SET_PROTOTYPE_METHOD(tmpl, "toDate", ToDate);

        Timestamp::tmpl.Reset(isolate, tmpl);
        Timestamp::constructor.Reset(isolate, tmpl->GetFunction());

        exports->Set(v8::String::NewFromUtf8(isolate, "Timestamp"), tmpl->GetFunction());
    }

    static v8::Local<v8::Object> NewFromTimespec(v8::Isolate * isolate, qdb_timespec_t ts)
    {
        const int argc = 2;
        auto seconds = static_cast<double>(ts.tv_sec);
        auto nanoseconds = static_cast<double>(ts.tv_nsec);

        v8::Local<v8::Value> argv[argc] = {
            v8::Number::New(isolate, seconds),
            v8::Number::New(isolate, nanoseconds),
        };

        v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);

        return (cons->NewInstance(isolate->GetCurrentContext(), argc, argv)).ToLocalChecked();
    }

    static bool InstanceOf(v8::Isolate * isolate, v8::Local<v8::Object> object)
    {
        auto ts_tmpl = Timestamp::tmpl.Get(isolate);
        return ts_tmpl->HasInstance(object);
    }

    qdb_timespec_t getTimespec()
    {
        qdb_timespec_t ts;
        ts.tv_sec = static_cast<qdb_time_t>(seconds_);
        ts.tv_nsec = static_cast<qdb_time_t>(nanoseconds_);
        return ts;
    }

private:
    explicit Timestamp(double seconds, double nanoseconds) : seconds_(seconds), nanoseconds_(nanoseconds)
    {
    }

    ~Timestamp()
    {
    }

    static void New(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        auto isolate = args.GetIsolate();
        if (args.IsConstructCall())
        {
            auto seconds = args[0]->IsUndefined() ? 0 : args[0]->NumberValue();
            auto nanoseconds = args[1]->IsUndefined() ? 0 : args[1]->NumberValue();
            auto obj = new Timestamp(seconds, nanoseconds);
            obj->Wrap(args.This());
            args.GetReturnValue().Set(args.This());
        }
        else
        {
            const int argc = 2;
            v8::Local<v8::Value> argv[argc] = {args[0], args[1]};
            auto cons = v8::Local<v8::Function>::New(isolate, constructor);
            args.GetReturnValue().Set(cons->NewInstance(isolate->GetCurrentContext(), argc, argv).ToLocalChecked());
        }
    }

    static void GetProperty(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value> & info)
    {
        auto isolate = info.GetIsolate();
        auto obj = ObjectWrap::Unwrap<Timestamp>(info.This());

        v8::String::Utf8Value s(property);
        std::string str(*s, s.length());

        if (str == "seconds")
        {
            info.GetReturnValue().Set(v8::Number::New(isolate, obj->seconds_));
        }
        else if (str == "nanoseconds")
        {
            info.GetReturnValue().Set(v8::Number::New(isolate, static_cast<double>(obj->nanoseconds_)));
        }
    }

    static void FromDate(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        auto isolate = args.GetIsolate();

        if (args.Length() != 1 || !args[0]->IsDate())
        {
            isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Expected a Date")));
        }

        auto ms = args[0]->ToNumber(isolate)->Value();
        auto ns = ms * 1000000ull;

        auto seconds = static_cast<double>(ns / 1000000000ull);
        auto nanoseconds = static_cast<double>(ns - seconds * 1000000000ull);

        const int argc = 2;
        v8::Local<v8::Value> argv[argc] = {v8::Number::New(isolate, seconds), v8::Number::New(isolate, nanoseconds)};
        auto cons = v8::Local<v8::Function>::New(isolate, constructor);
        args.GetReturnValue().Set(cons->NewInstance(isolate->GetCurrentContext(), argc, argv).ToLocalChecked());
    }

    static void ToDate(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        auto isolate = args.GetIsolate();
        auto obj = ObjectWrap::Unwrap<Timestamp>(args.Holder());
        auto epoch_ms = obj->seconds_ * 1000.0 + static_cast<double>(obj->nanoseconds_ / 1000000ull);
        auto maybe_date = v8::Date::New(isolate->GetCurrentContext(), epoch_ms);
        args.GetReturnValue().Set(maybe_date.ToLocalChecked());
    }

    static v8::Persistent<v8::Function> constructor;
    static v8::Persistent<v8::FunctionTemplate> tmpl;
    double seconds_;
    double nanoseconds_;
};

} // namespace quasardb
