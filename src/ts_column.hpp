#pragma once

#include "entry.hpp"
#include "time_series.hpp"
#include "utilities.hpp"
#include <qdb/ts.h>
#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <uv.h>
#include <memory>
#include <string>

namespace quasardb
{

// FIXME(marek): Deriving from Entry is conceptually wrong.
// A column of a timeseries is not a standalone entry for a database and operations such as getTags() or remove() are
// illegal.
template <typename Derivate>
class Column : public Entry<Derivate>
{
    friend class Entry<Derivate>;

    static const size_t ParametersCount = 2;

public:
    Column(cluster_data_ptr cd, const char * name, const char * ts, qdb_ts_column_type_t type)
        : Entry<Derivate>(cd, name), ts(ts), type(type)

    {
    }
    virtual ~Column(void)
    {
    }

public:
    template <typename F>
    static void Init(v8::Handle<v8::Object> exports, const char * className, F init)
    {
        Entry<Derivate>::Init(exports, className, [exports, init](v8::Local<v8::FunctionTemplate> tpl) {
            // call init function of derivate
            init(tpl);

            NODE_SET_PROTOTYPE_METHOD(tpl, "erase", Column<Derivate>::erase);

            v8::Isolate * isolate = exports->GetIsolate();

            auto s = v8::Signature::New(isolate, tpl);
            auto proto = tpl->PrototypeTemplate();

            proto->SetAccessorProperty(
                v8::String::NewFromUtf8(isolate, "timeseries"),
                v8::FunctionTemplate::New(isolate, Column<Derivate>::getTsAlias, v8::Local<v8::Value>(), s),
                v8::Local<v8::FunctionTemplate>(), v8::ReadOnly);

            proto->SetAccessorProperty(
                v8::String::NewFromUtf8(isolate, "type"),
                v8::FunctionTemplate::New(isolate, Column<Derivate>::getType, v8::Local<v8::Value>(), s),
                v8::Local<v8::FunctionTemplate>(), v8::ReadOnly);
        });
    }

    static v8::Local<v8::Object> MakeColumn(v8::Isolate * isolate, v8::Local<v8::Object> owner, const char * name)
    {
        static const size_t argc = Derivate::ParametersCount;

        v8::Local<v8::Value> argv[argc] = {
            owner,
            v8::String::NewFromUtf8(isolate, name),
        };
        v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, Derivate::constructor);
        assert(!cons.IsEmpty() && "Verify that Object::Init has been called in qdb_api.cpp:InitAll()");
        return (cons->NewInstance(isolate->GetCurrentContext(), argc, argv)).ToLocalChecked();
    }

    std::string timeSeries(void) const
    {
        return ts;
    }

private:
    static void erase(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        MethodMan call(args);

        Derivate * c = call.nativeHolder<Derivate>();
        assert(c);

        auto ts = c->timeSeries();
        Column<Derivate>::queue_work(
            args,
            [ts](qdb_request * qdb_req) {
                auto & ranges = qdb_req->input.content.ranges;
                auto erased = &qdb_req->output.content.uvalue;
                auto alias = qdb_req->input.alias.c_str();

                qdb_req->output.error =
                    qdb_ts_erase_ranges(qdb_req->handle(), ts.c_str(), alias, ranges.data(), ranges.size(), erased);
            },
            Column<Derivate>::processUintegerResult, &ArgsEaterBinder::ranges);
    }

    static void NewInstance(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        v8::Isolate * isolate = args.GetIsolate();

        static const int argc = ParametersCount;
        v8::Local<v8::Value> argv[argc] = {args[0], args[1]};
        v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, Derivate::constructor);
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
            v8::String::Utf8Value colname(args.GetIsolate(), str.first);

            auto obj = new Derivate(ts->cluster_data(), *colname, ts->native_alias().c_str());
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

    static void getTsAlias(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Column::getter(args, [](const v8::FunctionCallbackInfo<v8::Value> & args, const Column * c) {
            v8::Isolate * isolate = args.GetIsolate();
            args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, c->ts.c_str()));
        });
    }

    static void getType(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Column::getter(args, [](const v8::FunctionCallbackInfo<v8::Value> & args, const Column * c) {
            v8::Isolate * isolate = args.GetIsolate();
            args.GetReturnValue().Set(v8::Number::New(isolate, c->type));
        });
    }

private:
    std::string ts;
    qdb_ts_column_type_t type;
};

class DoubleColumn : public Column<DoubleColumn>
{
    friend class Column<DoubleColumn>;
    friend class Entry<DoubleColumn>;

    DoubleColumn(cluster_data_ptr cd, const char * name, const char * ts)
        : Column<DoubleColumn>(cd, name, ts, qdb_ts_column_double)

    {
    }
    virtual ~DoubleColumn(void)
    {
    }

public:
    static void Init(v8::Local<v8::Object> exports)
    {
        Column<DoubleColumn>::Init(exports, "DoubleColumn", [](v8::Local<v8::FunctionTemplate> tpl) {
            NODE_SET_PROTOTYPE_METHOD(tpl, "insert", DoubleColumn::insert);
            NODE_SET_PROTOTYPE_METHOD(tpl, "ranges", DoubleColumn::ranges);
            NODE_SET_PROTOTYPE_METHOD(tpl, "aggregate", DoubleColumn::aggregate);
        });
    }

private:
    static void insert(const v8::FunctionCallbackInfo<v8::Value> & args);
    static void ranges(const v8::FunctionCallbackInfo<v8::Value> & args);
    static void aggregate(const v8::FunctionCallbackInfo<v8::Value> & args);

    static void processDoublePointArrayResult(uv_work_t * req, int status);
    static void processDoubleAggregateResult(uv_work_t * req, int status);

    static v8::Persistent<v8::Function> constructor;
};

class BlobColumn : public Column<BlobColumn>
{
    friend class Column<BlobColumn>;
    friend class Entry<BlobColumn>;

    BlobColumn(cluster_data_ptr cd, const char * name, const char * ts)
        : Column<BlobColumn>(cd, name, ts, qdb_ts_column_blob)

    {
    }
    virtual ~BlobColumn(void)
    {
    }

public:
    static void Init(v8::Local<v8::Object> exports)
    {
        Column<BlobColumn>::Init(exports, "BlobColumn", [](v8::Local<v8::FunctionTemplate> tpl) {
            NODE_SET_PROTOTYPE_METHOD(tpl, "insert", BlobColumn::insert);
            NODE_SET_PROTOTYPE_METHOD(tpl, "ranges", BlobColumn::ranges);
            NODE_SET_PROTOTYPE_METHOD(tpl, "aggregate", BlobColumn::aggregate);
        });
    }

private:
    static void insert(const v8::FunctionCallbackInfo<v8::Value> & args);
    static void ranges(const v8::FunctionCallbackInfo<v8::Value> & args);
    static void aggregate(const v8::FunctionCallbackInfo<v8::Value> & args);

    static void processBlobPointArrayResult(uv_work_t * req, int status);
    static void processBlobAggregateResult(uv_work_t * req, int status);

    static v8::Persistent<v8::Function> constructor;
};

std::pair<v8::Local<v8::Object>, bool>
CreateColumn(v8::Isolate * isolate, v8::Local<v8::Object> owner, const char * name, qdb_ts_column_type_t type);

} // namespace quasardb
