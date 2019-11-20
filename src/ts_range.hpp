#pragma once

#include "time.hpp"
#include <qdb/client.h>
#include <qdb/ts.h>
#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>

namespace quasardb
{

class TsRange : public node::ObjectWrap
{
public:
    explicit TsRange(qdb_ts_range_t range)
    {
        this->range = range;
    }

    virtual ~TsRange()
    {
    }

    static void Init(v8::Local<v8::Object> exports)
    {
        v8::Isolate * isolate = exports->GetIsolate();

        // Prepare constructor template
        v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, New);
        tpl->SetClassName(v8::String::NewFromUtf8(isolate, "TsRange", v8::NewStringType::kNormal).ToLocalChecked());
        tpl->InstanceTemplate()->SetInternalFieldCount(1);

        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Signature> s = v8::Signature::New(isolate, tpl);

        auto proto = tpl->PrototypeTemplate();

        proto->SetAccessorProperty(v8::String::NewFromUtf8(isolate, "begin", v8::NewStringType::kNormal).ToLocalChecked(),
                                   v8::FunctionTemplate::New(isolate, TsRange::begin, v8::Local<v8::Value>(), s),
                                   v8::Local<v8::FunctionTemplate>(), v8::ReadOnly);
        proto->SetAccessorProperty(v8::String::NewFromUtf8(isolate, "end", v8::NewStringType::kNormal).ToLocalChecked(),
                                   v8::FunctionTemplate::New(isolate, TsRange::end, v8::Local<v8::Value>(), s),
                                   v8::Local<v8::FunctionTemplate>(), v8::ReadOnly);

        auto maybe_function = tpl->GetFunction(isolate->GetCurrentContext());
        if (maybe_function.IsEmpty()) return;

        constructor.Reset(isolate, maybe_function.ToLocalChecked());
        exports->Set(isolate->GetCurrentContext(), v8::String::NewFromUtf8(isolate, "TsRange", v8::NewStringType::kNormal).ToLocalChecked(), maybe_function.ToLocalChecked());
    }

    // Two arguments for regular range
    static void NewInstance(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        v8::Isolate * isolate = args.GetIsolate();

        assert(args.Length() == 2);
        static const int argcmax = 2;

        v8::Local<v8::Value> argv[argcmax] = {args[0], args[1]};

        v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);
        assert(!cons.IsEmpty() && "Verify that Object::Init has been called in qdb_api.cpp:InitAll()");
        v8::MaybeLocal<v8::Object> instance = cons->NewInstance(isolate->GetCurrentContext(), args.Length(), argv);
        args.GetReturnValue().Set(instance.ToLocalChecked());
    }

    qdb_ts_range_t nativeRange() const
    {
        return this->range;
    }

private:
    static void New(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        auto isolate = args.GetIsolate();
        auto context = isolate->GetCurrentContext();

        if (args.IsConstructCall())
        {
            // Can only be a range with 2 dates
            if (args.Length() != 2)
            {
                throwException(args, "Wrong number of arguments");
                return;
            }

            if (!Timestamp::InstanceOf(isolate, args[0]) || !Timestamp::InstanceOf(isolate, args[1]))
            {
                throwException(args, "Wrong type of arguments");
                return;
            }

            auto maybe_begin_timestamp = args[0]->ToObject(context);
            auto maybe_end_timestamp = args[1]->ToObject(context);
            if (maybe_begin_timestamp.IsEmpty() || maybe_end_timestamp.IsEmpty())
            {
                throwException(args, "Invalid timestamp object arguments");
                return;
            }

            auto begin_timestamp = node::ObjectWrap::Unwrap<Timestamp>(maybe_begin_timestamp.ToLocalChecked());
            auto end_timestamp = node::ObjectWrap::Unwrap<Timestamp>(maybe_end_timestamp.ToLocalChecked());

            qdb_ts_range_t range;
            range.begin = begin_timestamp->getTimespec();
            range.end = end_timestamp->getTimespec();

            TsRange * obj = new TsRange(range);

            obj->Wrap(args.This());
            args.GetReturnValue().Set(args.This());
        }
        else
        {
            NewInstance(args);
        }
    }

private:
    static void throwException(const v8::FunctionCallbackInfo<v8::Value> & args, const char * msg)
    {
        auto isolate = args.GetIsolate();
        isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, msg, v8::NewStringType::kNormal).ToLocalChecked()));
    }

    template <typename F>
    static void getter(const v8::FunctionCallbackInfo<v8::Value> & args, F f)
    {
        if (args.Length() != 0)
        {
            throwException(args, "Wrong number of arguments");
            return;
        }

        auto obj = node::ObjectWrap::Unwrap<TsRange>(args.Holder());
        assert(obj);

        f(args, obj);
    }

    static void begin(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        TsRange::getter(args, [](const v8::FunctionCallbackInfo<v8::Value> & args, TsRange * fr) {
            auto isolate = args.GetIsolate();
            auto timestamp = Timestamp::NewFromTimespec(isolate, fr->range.begin);
            args.GetReturnValue().Set(timestamp);
        });
    }

    static void end(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        TsRange::getter(args, [](const v8::FunctionCallbackInfo<v8::Value> & args, TsRange * fr) {
            auto isolate = args.GetIsolate();
            auto timestamp = Timestamp::NewFromTimespec(isolate, fr->range.end);
            args.GetReturnValue().Set(timestamp);
        });
    }

private:
    qdb_ts_range_t range;

    static v8::Persistent<v8::Function> constructor;
};

} // namespace quasardb
