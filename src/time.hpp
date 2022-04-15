#pragma once

#include <array>

#include <qdb/client.h>
#include <node.h>
#include <node_object_wrap.h>
#include <chrono>

namespace quasardb
{

using nanoseconds = std::chrono::duration<double, std::nano>;
using milliseconds = std::chrono::duration<double, std::milli>;
using seconds = std::chrono::duration<double>;

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
        auto context = isolate->GetCurrentContext();
        auto tmpl = v8::FunctionTemplate::New(isolate, New);
        tmpl->SetClassName(v8::String::NewFromUtf8(isolate, "Timestamp", v8::NewStringType::kNormal).ToLocalChecked());
        tmpl->InstanceTemplate()->SetInternalFieldCount(1);
        tmpl->InstanceTemplate()->SetAccessor(
            v8::String::NewFromUtf8(isolate, "seconds", v8::NewStringType::kNormal).ToLocalChecked(), GetProperty);
        tmpl->InstanceTemplate()->SetAccessor(
            v8::String::NewFromUtf8(isolate, "nanoseconds", v8::NewStringType::kNormal).ToLocalChecked(), GetProperty);

        tmpl->Set(v8::String::NewFromUtf8(isolate, "fromDate", v8::NewStringType::kNormal).ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, FromDate));
        NODE_SET_PROTOTYPE_METHOD(tmpl, "toDate", ToDate);

        Timestamp::tmpl.Reset(isolate, tmpl);

        auto cons = tmpl->GetFunction(context).ToLocalChecked();

        Timestamp::constructor.Reset(isolate, cons);
        exports->Set(
            context, v8::String::NewFromUtf8(isolate, "Timestamp", v8::NewStringType::kNormal).ToLocalChecked(), cons);
    }

    static v8::Local<v8::Object> NewFromTimespec(v8::Isolate * isolate, qdb_timespec_t ts)
    {
        auto seconds = static_cast<double>(ts.tv_sec);
        auto nanoseconds = static_cast<double>(ts.tv_nsec);

        const int argc = 2;
        v8::Local<v8::Value> argv[argc] = {
            v8::Number::New(isolate, seconds),
            v8::Number::New(isolate, nanoseconds),
        };

        auto cons = v8::Local<v8::Function>::New(isolate, constructor);
        return (cons->NewInstance(isolate->GetCurrentContext(), argc, argv)).ToLocalChecked();
    }

    static v8::Local<v8::Object> NewFromDate(v8::Isolate * isolate, v8::Local<v8::Date> d)
    {
        milliseconds epoch_ms{d->ToNumber(isolate->GetCurrentContext()).ToLocalChecked()->Value()};
        seconds s = epoch_ms;
        nanoseconds ns = epoch_ms - s;

        const int argc = 2;
        v8::Local<v8::Value> argv[argc] = {
            v8::Number::New(isolate, static_cast<double>(s.count())),
            v8::Number::New(isolate, static_cast<double>(ns.count())),
        };

        auto cons = v8::Local<v8::Function>::New(isolate, constructor);
        return (cons->NewInstance(isolate->GetCurrentContext(), argc, argv)).ToLocalChecked();
    }

    static bool InstanceOf(v8::Isolate * isolate, v8::Local<v8::Value> val)
    {
        auto context = isolate->GetCurrentContext();
        auto maybe_obj = val->ToObject(context);
        if (maybe_obj.IsEmpty())
        {
            return false;
        }

        auto obj = maybe_obj.ToLocalChecked();
        auto ts_tmpl = Timestamp::tmpl.Get(isolate);
        return ts_tmpl->HasInstance(obj);
    }

    qdb_timespec_t getTimespec()
    {
        qdb_timespec_t ts;
        ts.tv_sec = static_cast<qdb_time_t>(seconds_.count());
        ts.tv_nsec = static_cast<qdb_time_t>(nanoseconds_.count());
        return ts;
    }

private:
    explicit Timestamp(double seconds, double nanoseconds)
        : seconds_(seconds)
        , nanoseconds_(nanoseconds)
    {
    }

    ~Timestamp()
    {
    }

    static void New(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        auto isolate = args.GetIsolate();
        auto context = isolate->GetCurrentContext();

        if (args.IsConstructCall())
        {
            auto seconds = 0.0;
            auto nanoseconds = 0.0;

            if (args[0]->IsNumber())
            {
                auto maybe_seconds = args[0]->NumberValue(context);
                if (maybe_seconds.IsJust())
                {
                    seconds = maybe_seconds.FromJust();
                }

                auto maybe_nanoseconds = args[1]->NumberValue(context);
                if (maybe_nanoseconds.IsJust())
                {
                    nanoseconds = maybe_nanoseconds.FromJust();
                }
            }
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

        v8::String::Utf8Value s(isolate, property);
        std::string str(*s, s.length());

        if (str == "seconds")
        {
            info.GetReturnValue().Set(v8::Number::New(isolate, obj->seconds_.count()));
        }
        else if (str == "nanoseconds")
        {
            info.GetReturnValue().Set(v8::Number::New(isolate, obj->nanoseconds_.count()));
        }
    }

    static void FromDate(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        auto isolate = args.GetIsolate();

        if (args.Length() != 1 || !args[0]->IsDate())
        {
            isolate->ThrowException(v8::Exception::TypeError(
                v8::String::NewFromUtf8(isolate, "Expected a Date", v8::NewStringType::kNormal).ToLocalChecked()));
        }

        milliseconds epoch_ms{args[0]->ToNumber(isolate->GetCurrentContext()).ToLocalChecked()->Value()};
        seconds s = epoch_ms;
        nanoseconds ns = epoch_ms - s;

        const int argc = 2;
        v8::Local<v8::Value> argv[argc] = {v8::Number::New(isolate, s.count()), v8::Number::New(isolate, ns.count())};
        auto cons = v8::Local<v8::Function>::New(isolate, constructor);
        args.GetReturnValue().Set(cons->NewInstance(isolate->GetCurrentContext(), argc, argv).ToLocalChecked());
    }

private:
    static void ToDate(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        auto isolate = args.GetIsolate();
        auto obj = ObjectWrap::Unwrap<Timestamp>(args.Holder());

        milliseconds epoch_ms = obj->seconds_ + obj->nanoseconds_;

        auto maybe_date = v8::Date::New(isolate->GetCurrentContext(), epoch_ms.count());
        args.GetReturnValue().Set(maybe_date.ToLocalChecked());
    }

    static v8::Persistent<v8::Function> constructor;
    static v8::Persistent<v8::FunctionTemplate> tmpl;

    seconds seconds_;
    nanoseconds nanoseconds_;
};

} // namespace quasardb
