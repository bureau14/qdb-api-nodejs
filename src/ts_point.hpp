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

            proto->SetAccessorProperty(
                v8::String::NewFromUtf8(isolate, "timestamp", v8::NewStringType::kNormal).ToLocalChecked(),
                v8::FunctionTemplate::New(isolate, getTimestamp, v8::Local<v8::Value>(), s),
                v8::Local<v8::FunctionTemplate>(), v8::ReadOnly);

            proto->SetAccessorProperty(
                v8::String::NewFromUtf8(isolate, "value", v8::NewStringType::kNormal).ToLocalChecked(),
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
        tpl->SetClassName(v8::String::NewFromUtf8(isolate, className, v8::NewStringType::kNormal).ToLocalChecked());
        tpl->InstanceTemplate()->SetInternalFieldCount(Derivate::FieldsCount);

        init(tpl);

        auto maybe_function = tpl->GetFunction(isolate->GetCurrentContext());
        if (maybe_function.IsEmpty()) return;

        Derivate::constructor.Reset(isolate, maybe_function.ToLocalChecked());
        exports->Set(isolate->GetCurrentContext(),
                     v8::String::NewFromUtf8(isolate, className, v8::NewStringType::kNormal).ToLocalChecked(),
                     maybe_function.ToLocalChecked());
    }

    static void getTimestamp(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        MethodMan call(args);

        Derivate * pthis = call.nativeHolder<Derivate>();

        assert(pthis);

        auto qdb_timestamp = Timestamp::NewFromTimespec(args.GetIsolate(), pthis->timestamp);

        args.GetReturnValue().Set(qdb_timestamp);
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

        v8::Local<v8::Value> argv[argc] = {
            Timestamp::NewFromTimespec(isolate, ts),
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
            auto isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();
            if (args.Length() != ParametersCount)
            {
                call.throwException("Wrong number of arguments");
                return;
            }

            if (!Timestamp::InstanceOf(isolate, args[0]) || !args[1]->IsNumber())
            {
                call.throwException("Invalid parameter supplied to object");
                return;
            }

            auto maybe_timestamp = args[0]->ToObject(context);
            if (maybe_timestamp.IsEmpty())
            {
                call.throwException("Invalid 'timestamp' parameter supplied to object");
                return;
            }

            auto timestamp = ObjectWrap::Unwrap<Timestamp>(maybe_timestamp.ToLocalChecked());

            auto maybe_value = args[1]->NumberValue(isolate->GetCurrentContext());
            if (maybe_value.IsNothing())
            {
                call.throwException("Invalid 'value' parameter supplied to object");
                return;
            }

            auto value = maybe_value.FromJust();
            auto obj = new DoublePoint(timestamp->getTimespec(), value);

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
};

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

        auto bufp = static_cast<const char *>(content);
        v8::Local<v8::Value> argv[argc] = {
            Timestamp::NewFromTimespec(isolate, ts),
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
            auto isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();
            if (args.Length() != ParametersCount)
            {
                call.throwException("Wrong number of arguments");
                return;
            }

            if (!Timestamp::InstanceOf(isolate, args[0]) || !args[1]->IsObject())
            {
                call.throwException("Invalid parameter supplied to object");
                return;
            }

            auto maybe_timestamp = args[0]->ToObject(context);
            if (maybe_timestamp.IsEmpty())
            {
                return;
            }

            auto timestamp = ObjectWrap::Unwrap<Timestamp>(maybe_timestamp.ToLocalChecked());

            auto maybe_obj = args[1]->ToObject(isolate->GetCurrentContext());
            if (maybe_obj.IsEmpty())
            {
                return;
            }

            auto obj = new BlobPoint(timestamp->getTimespec(), args.GetIsolate(), maybe_obj.ToLocalChecked());

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

class StringPoint : public Point<StringPoint>
{
    friend class Point<StringPoint>;

    static const size_t ParametersCount = 2;

    StringPoint(qdb_timespec_t ts, v8::Isolate * isolate, v8::Local<v8::Object> obj)
        : Point<StringPoint>(ts), buffer(isolate, obj)
    {
    }

    virtual ~StringPoint()
    {
        this->buffer.Reset();
    }

public:
    static void Init(v8::Local<v8::Object> exports)
    {
        Point<StringPoint>::Init(exports, "StringPoint", [](v8::Local<v8::FunctionTemplate> tpl) {});
    }

    static v8::Local<v8::Object>
    MakePointWithCopy(v8::Isolate * isolate, qdb_timespec_t ts, const void * content, size_t size)
    {
        static const size_t argc = ParametersCount;

        auto bufp = static_cast<const char *>(content);
        v8::Local<v8::Value> argv[argc] = {
            Timestamp::NewFromTimespec(isolate, ts),
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

        StringPoint * pthis = call.nativeHolder<StringPoint>();

        assert(pthis);

        auto local = v8::Local<v8::Object>::New(args.GetIsolate(), pthis->buffer);

        args.GetReturnValue().Set(local);
    }

    static void New(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        if (args.IsConstructCall())
        {
            MethodMan call(args);
            auto isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();
            if (args.Length() != ParametersCount)
            {
                call.throwException("Wrong number of arguments");
                return;
            }

            if (!Timestamp::InstanceOf(isolate, args[0]) || !args[1]->IsObject())
            {
                call.throwException("Invalid parameter supplied to object");
                return;
            }

            auto maybe_timestamp = args[0]->ToObject(context);
            if (maybe_timestamp.IsEmpty())
            {
                return;
            }

            auto timestamp = ObjectWrap::Unwrap<Timestamp>(maybe_timestamp.ToLocalChecked());

            auto maybe_obj = args[1]->ToObject(isolate->GetCurrentContext());
            if (maybe_obj.IsEmpty())
            {
                return;
            }

            auto obj = new StringPoint(timestamp->getTimespec(), args.GetIsolate(), maybe_obj.ToLocalChecked());

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

class SymbolPoint : public Point<SymbolPoint>
{
    friend class Point<SymbolPoint>;

    static const size_t ParametersCount = 2;

    SymbolPoint(qdb_timespec_t ts, v8::Isolate * isolate, v8::Local<v8::Object> obj)
        : Point<SymbolPoint>(ts), buffer(isolate, obj)
    {
    }

    virtual ~SymbolPoint()
    {
        this->buffer.Reset();
    }

public:
    static void Init(v8::Local<v8::Object> exports)
    {
        Point<SymbolPoint>::Init(exports, "SymbolPoint", [](v8::Local<v8::FunctionTemplate> tpl) {});
    }

    static v8::Local<v8::Object>
    MakePointWithCopy(v8::Isolate * isolate, qdb_timespec_t ts, const void * content, size_t size)
    {
        static const size_t argc = ParametersCount;

        auto bufp = static_cast<const char *>(content);
        v8::Local<v8::Value> argv[argc] = {
            Timestamp::NewFromTimespec(isolate, ts),
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

        SymbolPoint * pthis = call.nativeHolder<SymbolPoint>();

        assert(pthis);

        auto local = v8::Local<v8::Object>::New(args.GetIsolate(), pthis->buffer);

        args.GetReturnValue().Set(local);
    }

    static void New(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        if (args.IsConstructCall())
        {
            MethodMan call(args);
            auto isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();
            if (args.Length() != ParametersCount)
            {
                call.throwException("Wrong number of arguments");
                return;
            }

            if (!Timestamp::InstanceOf(isolate, args[0]) || !args[1]->IsObject())
            {
                call.throwException("Invalid parameter supplied to object");
                return;
            }

            auto maybe_timestamp = args[0]->ToObject(context);
            if (maybe_timestamp.IsEmpty())
            {
                return;
            }

            auto timestamp = ObjectWrap::Unwrap<Timestamp>(maybe_timestamp.ToLocalChecked());

            auto maybe_obj = args[1]->ToObject(isolate->GetCurrentContext());
            if (maybe_obj.IsEmpty())
            {
                return;
            }

            auto obj = new SymbolPoint(timestamp->getTimespec(), args.GetIsolate(), maybe_obj.ToLocalChecked());

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

class Int64Point : public Point<Int64Point>
{
    friend class Point<Int64Point>;

    static const size_t ParametersCount = 2;

    Int64Point(qdb_timespec_t ts, qdb_int_t v) : Point<Int64Point>(ts), value(v)
    {
    }

    virtual ~Int64Point()
    {
    }

public:
    static void Init(v8::Local<v8::Object> exports)
    {
        Point<Int64Point>::Init(exports, "Int64Point", [](v8::Local<v8::FunctionTemplate> tpl) {});
    }

    static v8::Local<v8::Object> MakePoint(v8::Isolate * isolate, qdb_timespec_t ts, qdb_int_t value)
    {
        static const size_t argc = ParametersCount;

        v8::Local<v8::Value> argv[argc] = {
            Timestamp::NewFromTimespec(isolate, ts),
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
            auto isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();
            if (args.Length() != ParametersCount)
            {
                call.throwException("Wrong number of arguments");
                return;
            }

            if (!Timestamp::InstanceOf(isolate, args[0]) || !args[1]->IsNumber())
            {
                call.throwException("Invalid parameter supplied to object");
                return;
            }

            auto maybe_timestamp = args[0]->ToObject(context);
            if (maybe_timestamp.IsEmpty())
            {
                call.throwException("Invalid 'timestamp' parameter supplied to object");
                return;
            }

            auto timestamp = ObjectWrap::Unwrap<Timestamp>(maybe_timestamp.ToLocalChecked());

            auto maybe_value = args[1]->NumberValue(isolate->GetCurrentContext());
            if (maybe_value.IsNothing())
            {
                call.throwException("Invalid 'value' parameter supplied to object");
                return;
            }

            auto value = maybe_value.FromJust();
            auto obj = new Int64Point(timestamp->getTimespec(), value);

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

        Int64Point * pthis = call.nativeHolder<Int64Point>();

        assert(pthis);

        call.template setReturnValue<v8::Number>(pthis->value);
    }

private:
    qdb_int_t value;

    static v8::Persistent<v8::Function> constructor;
};

class TimestampPoint : public Point<TimestampPoint>
{
    friend class Point<TimestampPoint>;

    static const size_t ParametersCount = 2;

    TimestampPoint(qdb_timespec_t ts, qdb_timespec_t v) : Point<TimestampPoint>(ts), value(v)
    {
    }

    virtual ~TimestampPoint()
    {
    }

public:
    static void Init(v8::Local<v8::Object> exports)
    {
        Point<TimestampPoint>::Init(exports, "TimestampPoint", [](v8::Local<v8::FunctionTemplate> tpl) {});
    }

    static v8::Local<v8::Object> MakePoint(v8::Isolate * isolate, qdb_timespec_t ts, qdb_timespec_t value)
    {
        static const size_t argc = ParametersCount;

        v8::Local<v8::Value> argv[argc] = {Timestamp::NewFromTimespec(isolate, ts),
                                           Timestamp::NewFromTimespec(isolate, value)};

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
            auto isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();

            if (args.Length() != ParametersCount)
            {
                call.throwException("Wrong number of arguments");
                return;
            }

            if (!Timestamp::InstanceOf(isolate, args[0]) || !Timestamp::InstanceOf(isolate, args[1]))
            {
                call.throwException("Invalid parameter supplied to object");
                return;
            }

            auto maybe_timestamp = args[0]->ToObject(context);
            auto maybe_timestamp_value = args[1]->ToObject(context);

            if (maybe_timestamp.IsEmpty() || maybe_timestamp_value.IsEmpty())
            {
                call.throwException("Invalid timestamp objects supplied as arguments");
                return;
            }

            auto timestamp = ObjectWrap::Unwrap<Timestamp>(maybe_timestamp.ToLocalChecked());
            auto timestamp_value = ObjectWrap::Unwrap<Timestamp>(maybe_timestamp_value.ToLocalChecked());

            auto obj = new TimestampPoint(timestamp->getTimespec(), timestamp_value->getTimespec());

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

        TimestampPoint * pthis = call.nativeHolder<TimestampPoint>();

        assert(pthis);

        call.template setReturnValue<Timestamp>(pthis->value);
    }

private:
    qdb_timespec_t value;

    static v8::Persistent<v8::Function> constructor;
};

} // namespace quasardb
