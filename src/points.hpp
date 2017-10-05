#pragma once

#include <cassert>

#include "utilities.hpp"
#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>

#include <qdb/client.h>

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
private:
public:
    static const size_t FieldsCount = 1;
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
            NODE_SET_PROTOTYPE_METHOD(tpl, "timestamp", Point<Derivate>::getTimestamp);

            init(tpl);
        });
    }

    template <typename F>
    static void InitConstructorOnly(v8::Local<v8::Object> exports, const char * className, F init)
    {
        v8::Isolate * isolate = exports->GetIsolate();

        // Prepare constructor template
        v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, Derivate::New);
        tpl->SetClassName(v8::String::NewFromUtf8(isolate, className));
        tpl->InstanceTemplate()->SetInternalFieldCount(Point<Derivate>::FieldsCount);

        init(tpl);

        Derivate::constructor.Reset(isolate, tpl->GetFunction());

        exports->Set(v8::String::NewFromUtf8(isolate, className), tpl->GetFunction());
    }

    static void NewInstance(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        static const size_t argc = Derivate::ParameterCount + 1;
        v8::Isolate * isolate = args.GetIsolate();

        auto argv = ArgumentsCopier<argc>::copy(args);
        v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, Derivate::constructor);
        assert(!cons.IsEmpty() && "Verify that Object::Init has been called in qdb_api.cpp:InitAll()");
        v8::Local<v8::Object> instance =
            cons->NewInstance(isolate->GetCurrentContext(), argc, argv.data()).ToLocalChecked();

        args.GetReturnValue().Set(instance);
    }

    static void getTimestamp(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        MethodMan call(args);

        Derivate * pthis = call.nativeHolder<Derivate>();

        assert(pthis);

        auto ms = qdb_timespec_to_ms(pthis->timestamp);
        auto date = v8::Date::New(args.GetIsolate(), ms);
        args.GetReturnValue().Set(date);
    }

private:
    qdb_timespec_t timestamp;

    static v8::Persistent<v8::Function> constructor;
};

class DoublePoint : public Point<DoublePoint>
{
    friend class Point<DoublePoint>;

public:
    static const size_t ParameterCount = 1;

    DoublePoint(qdb_timespec_t ts, double v) : Point<DoublePoint>(ts), value(v)
    {
    }
    ~DoublePoint()
    {
    }

    // static void Init(v8::Isolate * isolate)
    static void Init(v8::Local<v8::Object> exports)
    {
        Point<DoublePoint>::Init(exports, "DoublePoint", [](v8::Local<v8::FunctionTemplate> tpl) {
            NODE_SET_PROTOTYPE_METHOD(tpl, "value", DoublePoint::getValue);
        });
    }

    static void getValue(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        MethodMan call(args);

        DoublePoint * pthis = call.nativeHolder<DoublePoint>();

        assert(pthis);

        call.template setReturnValue<v8::Number>(pthis->value);
    }

private:
    static void New(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        if (args.IsConstructCall())
        {
            assert(args.Length() == 2);
            // Invoked as constructor: `new xPoint(...)`

            MethodMan call(args);

            if (!args[0]->IsDate())
            {
                call.throwException("Expected Date as first argument");
                return;
            }

            if (!args[1]->IsNumber())
            {
                call.throwException("Expected Number as second argument");
                return;
            }

            auto ms = args[0]->NumberValue();
            auto value = args[1]->NumberValue();
            auto obj = new DoublePoint(ms_to_qdb_timespec(ms), value);

            obj->Wrap(args.This());
            args.GetReturnValue().Set(args.This());
        }
        else
        {
            // Invoked as plain function `xPoint(...)`, turn into construct call.
            NewInstance(args);
        }
    }

private:
    double value;

    static v8::Persistent<v8::Function> constructor;
}; // class DoublePoint

class BlobPoint : public Point<BlobPoint>
{
    friend class Point<BlobPoint>;

public:
    static const size_t ParameterCount = 1;

    BlobPoint(qdb_timespec_t ts, v8::Isolate * isolate, v8::Local<v8::Object> obj)
        : Point<BlobPoint>(ts), buffer(isolate, obj)
    {
    }
    ~BlobPoint()
    {
        this->buffer.Reset();
    }

    static void Init(v8::Local<v8::Object> exports)
    {
        Point<BlobPoint>::Init(exports, "BlobPoint", [](v8::Local<v8::FunctionTemplate> tpl) {
            NODE_SET_PROTOTYPE_METHOD(tpl, "value", BlobPoint::getValue);
        });
    }

    static void getValue(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        MethodMan call(args);

        BlobPoint * pthis = call.nativeHolder<BlobPoint>();

        assert(pthis);

        auto local = v8::Local<v8::Object>::New(args.GetIsolate(), pthis->buffer);

        args.GetReturnValue().Set(local);
    }

private:
    static void New(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        if (args.IsConstructCall())
        {
            assert(args.Length() == 2);
            // Invoked as constructor: `new xPoint(...)`

            MethodMan call(args);

            if (!args[0]->IsDate())
            {
                call.throwException("Expected Date as first argument");
                return;
            }

            if (!args[1]->IsObject())
            {
                call.throwException("Expected Buffer as second argument");
                return;
            }

            auto ms = args[0]->NumberValue();
            auto obj = new BlobPoint(ms_to_qdb_timespec(ms), args.GetIsolate(), args[1]->ToObject());

            obj->Wrap(args.This());
            args.GetReturnValue().Set(args.This());
        }
        else
        {
            // Invoked as plain function `xPoint(...)`, turn into construct call.
            NewInstance(args);
        }
    }

private:
    v8::Persistent<v8::Object> buffer;

    static v8::Persistent<v8::Function> constructor;
}; // class BlobPoint

} // namespace quasardb
