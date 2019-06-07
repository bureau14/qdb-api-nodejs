#pragma once

#include "utilities.hpp"
#include <qdb/client.h>
#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>

// The reason why this points classes is out of Cluster factory object
// is that we don't need cluster shared state inside these points.
// These points could be created on different scopes and could have nothing to do
// with lifetime of the cluster. They are fully passive and use to pass data
// to and from JS and qdb API.

namespace quasardb
{

template <typename Derivate>
class Point : public node::ObjectWrap
{
    static const size_t FieldsCount = 1;

public:
    Point(qdb_timespec_t ts) : timestamp(ts)
    {
    }

    virtual ~Point()
    {
    }

    template <typename F>
    static void Init(v8::Local<v8::Object> exports, const char * className, F init)
    {
        InitConstructorOnly(exports, className, [init](v8::Local<v8::FunctionTemplate> tpl) {
            auto isolate = v8::Isolate::GetCurrent();
            auto s = v8::Signature::New(isolate, tpl);
            auto proto = tpl->PrototypeTemplate();

            proto->SetAccessorProperty(v8::String::NewFromUtf8(isolate, "timestamp"),
                                       v8::FunctionTemplate::New(isolate, getTimestamp, v8::Local<v8::Value>(), s),
                                       v8::Local<v8::FunctionTemplate>(), v8::ReadOnly);

            proto->SetAccessorProperty(
                v8::String::NewFromUtf8(isolate, "value"),
                v8::FunctionTemplate::New(isolate, Derivate::getValue, v8::Local<v8::Value>(), s),
                v8::Local<v8::FunctionTemplate>(), v8::ReadOnly);

            init(tpl);
        });
    }

    static void NewInstance(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        static const size_t argc = Derivate::ParametersCount;
        v8::Isolate * isolate = args.GetIsolate();

        auto argv = ArgumentsCopier<argc>::copy(args);
        v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, Derivate::constructor);
        assert(!cons.IsEmpty() && "Verify that Object::Init has been called in qdb_api.cpp:InitAll()");
        v8::Local<v8::Object> instance =
            cons->NewInstance(isolate->GetCurrentContext(), argc, argv.data()).ToLocalChecked();

        args.GetReturnValue().Set(instance);
    }

private:
    template <typename F>
    static void InitConstructorOnly(v8::Local<v8::Object> exports, const char * className, F init)
    {
        v8::Isolate * isolate = exports->GetIsolate();

        // Prepare constructor template
        v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, Derivate::New);
        tpl->SetClassName(v8::String::NewFromUtf8(isolate, className));
        tpl->InstanceTemplate()->SetInternalFieldCount(Derivate::FieldsCount);

        init(tpl);

        auto maybe_function = tpl->GetFunction(isolate->GetCurrentContext());
        if (maybe_function.IsEmpty()) return;

        Derivate::constructor.Reset(isolate, maybe_function.ToLocalChecked());
        exports->Set(v8::String::NewFromUtf8(isolate, className), maybe_function.ToLocalChecked());
    }

    static void getTimestamp(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        MethodMan call(args);

        Derivate * pthis = call.nativeHolder<Derivate>();

        assert(pthis);

        auto ms = qdb_timespec_to_ms(pthis->timestamp);
        auto maybe_date = v8::Date::New(args.GetIsolate()->GetCurrentContext(), ms);
        if (maybe_date.IsEmpty())
        {
            assert("Cannot create new date");
            return;
        }

        args.GetReturnValue().Set(maybe_date.ToLocalChecked());
    }

private:
    qdb_timespec_t timestamp;

    static v8::Persistent<v8::Function> constructor;
};

class DoublePoint : public Point<DoublePoint>
{
    friend class Point<DoublePoint>;

    static const size_t ParametersCount = 2;

    DoublePoint(qdb_timespec_t ts, double v) : Point<DoublePoint>(ts), value(v)
    {
    }

    virtual ~DoublePoint()
    {
    }

public:
    static void Init(v8::Local<v8::Object> exports)
    {
        Point<DoublePoint>::Init(exports, "DoublePoint", [](v8::Local<v8::FunctionTemplate> tpl) {});
    }

    static v8::Local<v8::Object> MakePoint(v8::Isolate * isolate, qdb_timespec_t ts, double value)
    {
        static const size_t argc = ParametersCount;

        auto maybe_date = v8::Date::New(isolate->GetCurrentContext(), qdb_timespec_to_ms(ts));
        if (maybe_date.IsEmpty())
        {
            assert("Cannot create new date");
            return {};
        }

        v8::Local<v8::Value> argv[argc] = {
            maybe_date.ToLocalChecked(),
            v8::Number::New(isolate, value),
        };

        v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);
        assert(!cons.IsEmpty() && "Verify that Object::Init has been called in qdb_api.cpp:InitAll()");
        return (cons->NewInstance(isolate->GetCurrentContext(), argc, argv)).ToLocalChecked();
    }

private:
    static void New(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        if (args.IsConstructCall())
        {
            MethodMan call(args);
            if (args.Length() != ParametersCount)
            {
                call.throwException("Wrong number of arguments");
                return;
            }

            if (!args[0]->IsDate() || !args[1]->IsNumber())
            {
                call.throwException("Invalid parameter supplied to object");
                return;
            }

            auto isolate = args.GetIsolate();

            auto maybe_ms = args[0]->NumberValue(isolate->GetCurrentContext());
            if (maybe_ms.IsNothing())
            {
                call.throwException("Invalid 'ms' parameter supplied to object");
                return;
            }

            auto maybe_value = args[1]->NumberValue(isolate->GetCurrentContext());
            if (maybe_value.IsNothing())
            {
                call.throwException("Invalid 'value' parameter supplied to object");
                return;
            }

            auto ms = maybe_ms.FromJust();
            auto value = maybe_value.FromJust();
            auto obj = new DoublePoint(ms_to_qdb_timespec(ms), value);

            obj->Wrap(args.This());
            args.GetReturnValue().Set(args.This());
        }
        else
        {
            NewInstance(args);
        }
    }

    static void getValue(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        MethodMan call(args);

        DoublePoint * pthis = call.nativeHolder<DoublePoint>();

        assert(pthis);

        call.template setReturnValue<v8::Number>(pthis->value);
    }

private:
    double value;

    static v8::Persistent<v8::Function> constructor;
}; // class DoublePoint

class BlobPoint : public Point<BlobPoint>
{
    friend class Point<BlobPoint>;

    static const size_t ParametersCount = 2;

    BlobPoint(qdb_timespec_t ts, v8::Isolate * isolate, v8::Local<v8::Object> obj)
        : Point<BlobPoint>(ts), buffer(isolate, obj)
    {
    }

    virtual ~BlobPoint()
    {
        this->buffer.Reset();
    }

public:
    static void Init(v8::Local<v8::Object> exports)
    {
        Point<BlobPoint>::Init(exports, "BlobPoint", [](v8::Local<v8::FunctionTemplate> tpl) {});
    }

    static v8::Local<v8::Object>
    MakePointWithCopy(v8::Isolate * isolate, qdb_timespec_t ts, const void * content, size_t size)
    {
        static const size_t argc = ParametersCount;

        auto maybe_date = v8::Date::New(isolate->GetCurrentContext(), qdb_timespec_to_ms(ts));
        if (maybe_date.IsEmpty())
        {
            assert("Cannot create new date");
            return {};
        }

        auto bufp = static_cast<const char *>(content);
        v8::Local<v8::Value> argv[argc] = {
            maybe_date.ToLocalChecked(),
            node::Buffer::Copy(isolate, bufp, size).ToLocalChecked(),
        };

        v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);
        assert(!cons.IsEmpty() && "Verify that Object::Init has been called in qdb_api.cpp:InitAll()");
        return (cons->NewInstance(isolate->GetCurrentContext(), argc, argv)).ToLocalChecked();
    }

private:
    static void getValue(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        MethodMan call(args);

        BlobPoint * pthis = call.nativeHolder<BlobPoint>();

        assert(pthis);

        auto local = v8::Local<v8::Object>::New(args.GetIsolate(), pthis->buffer);

        args.GetReturnValue().Set(local);
    }

    static void New(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        if (args.IsConstructCall())
        {
            MethodMan call(args);
            if (args.Length() != ParametersCount)
            {
                call.throwException("Wrong number of arguments");
                return;
            }

            if (!args[0]->IsDate() || !args[1]->IsObject())
            {
                call.throwException("Invalid parameter supplied to object");
                return;
            }

            auto isolate = args.GetIsolate();

            auto maybe_ms = args[0]->NumberValue(isolate->GetCurrentContext());
            if (maybe_ms.IsNothing())
            {
                call.throwException("Invalid 'ms' parameter supplied to object");
                return;
            }

            auto ms = maybe_ms.FromJust();

            auto maybe_obj = args[1]->ToObject(isolate->GetCurrentContext());
            if (maybe_obj.IsEmpty())
            {
                return;
            }

            auto obj = new BlobPoint(ms_to_qdb_timespec(ms), args.GetIsolate(), maybe_obj.ToLocalChecked());

            obj->Wrap(args.This());
            args.GetReturnValue().Set(args.This());
        }
        else
        {
            NewInstance(args);
        }
    }

private:
    v8::Persistent<v8::Object> buffer;

    static v8::Persistent<v8::Function> constructor;
};

} // namespace quasardb
