#pragma once

#include <memory>
#include <string>

#include "entry.hpp"
#include "time_series.hpp"
#include "utilities.hpp"

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <uv.h>

#include <qdb/ts.h>

namespace quasardb
{
class Column : public Entry<Column>
{
    friend class Entry<Column>;
    friend class Cluster;

    static const size_t ParametersCount = 3;

public:
    Column(cluster_data_ptr cd, const char * name, const char * ts, qdb_ts_column_type_t type)
        : Entry<Column>(cd, name), ts(ts), type(type)

    {
    }
    virtual ~Column(void)
    {
    }

public:
    static void Init(v8::Local<v8::Object> exports)
    {
        Entry<Column>::Init(exports, "Column", [exports](v8::Local<v8::FunctionTemplate> tpl) {
            v8::Isolate * isolate = exports->GetIsolate();

            auto s = v8::Signature::New(isolate, tpl);
            auto proto = tpl->PrototypeTemplate();

            proto->SetAccessorProperty(v8::String::NewFromUtf8(isolate, "name"),
                                       v8::FunctionTemplate::New(isolate, Column::getName, v8::Local<v8::Value>(), s),
                                       v8::Local<v8::FunctionTemplate>(), v8::ReadOnly);

            proto->SetAccessorProperty(v8::String::NewFromUtf8(isolate, "type"),
                                       v8::FunctionTemplate::New(isolate, Column::getType, v8::Local<v8::Value>(), s),
                                       v8::Local<v8::FunctionTemplate>(), v8::ReadOnly);

            NODE_SET_PROTOTYPE_METHOD(tpl, "insert", Column::insert);

            // constructor.Reset(isolate, tpl->GetFunction());
            // exports->Set(v8::String::NewFromUtf8(isolate, "Column"), tpl->GetFunction());
        });
    }

    static v8::Local<v8::Object>
    MakeColumn(v8::Isolate * isolate, v8::Local<v8::Object> owner, const char * name, qdb_ts_column_type_t type)
    {
        static const size_t argc = ParametersCount;

        v8::Local<v8::Value> argv[argc] = {owner, v8::String::NewFromUtf8(isolate, name),
                                           v8::Integer::New(isolate, type)};
        v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);
        assert(!cons.IsEmpty() && "Verify that Object::Init has been called in qdb_api.cpp:InitAll()");
        return (cons->NewInstance(isolate->GetCurrentContext(), argc, argv)).ToLocalChecked();
    }

private:
    static void NewInstance(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        v8::Isolate * isolate = args.GetIsolate();

        static const int argc = ParametersCount;
        v8::Local<v8::Value> argv[argc] = {args[0], args[1]};
        v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);
        assert(!cons.IsEmpty() && "Verify that Object::Init has been called in qdb_api.cpp:InitAll()");
        v8::MaybeLocal<v8::Object> instance = cons->NewInstance(isolate->GetCurrentContext(), argc, argv);
        args.GetReturnValue().Set(instance.ToLocalChecked());
    }

    static void New(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        if (args.IsConstructCall())
        {
            // Invoked as constructor: `new MyObject(...)`
            MethodMan call(args);
            if (args.Length() != ParametersCount)
            {
                call.throwException("Wrong number of arguments");
                return;
            }

            ArgsEater argsEater(call);
            auto owner = argsEater.eatObject();
            if (!owner.second)
            {
                call.throwException("Invalid parameter supplied to object");
                return;
            }

            TimeSeries * ts = node::ObjectWrap::Unwrap<TimeSeries>(owner.first);
            assert(ts);

            auto str = argsEater.eatString();
            if (!str.second)
            {
                call.throwException("Invalid parameter supplied to object");
                return;
            }
            v8::String::Utf8Value colname(str.first);

            auto type = argsEater.eatInteger<qdb_ts_column_type_t>();
            if (!type.second)
            {
                call.throwException("Invalid parameter supplied to object");
                return;
            }

            auto obj = new Column(ts->cluster_data(), *colname, ts->native_alias().c_str(), type.first);
            obj->Wrap(args.This());
            args.GetReturnValue().Set(args.This());
        }
        else
        {
            NewInstance(args);
        }
    }

    template <typename Func>
    static void getter(const v8::FunctionCallbackInfo<v8::Value> & args, Func f)
    {
        MethodMan call(args);

        Column * c = call.nativeHolder<Column>();
        assert(c);

        f(args, c);
    }

    static void getName(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Column::getter(args, [](const v8::FunctionCallbackInfo<v8::Value> & args, const Column * c) {
            v8::Isolate * isolate = args.GetIsolate();
            args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, c->native_alias().c_str()));
        });
    }

    static void getType(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Column::getter(args, [](const v8::FunctionCallbackInfo<v8::Value> & args, const Column * c) {
            v8::Isolate * isolate = args.GetIsolate();
            args.GetReturnValue().Set(v8::Number::New(isolate, c->type));
        });
    }

    static void insert(const v8::FunctionCallbackInfo<v8::Value> & args);
    static void insertBlobs(const std::string & ts, const v8::FunctionCallbackInfo<v8::Value> & args);
    static void insertDoubles(const std::string & ts, const v8::FunctionCallbackInfo<v8::Value> & args);

    std::string ts;
    qdb_ts_column_type_t type;

    static v8::Persistent<v8::Function> constructor;
};

} // quasardb namespace
