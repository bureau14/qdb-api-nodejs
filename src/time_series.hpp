#pragma once

#include <memory>
#include <string>

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <uv.h>

#include <qdb/ts.h>

#include "entry.hpp"

namespace quasardb
{

class TimeSeries : public Entry<TimeSeries>
{

    friend class Entry<TimeSeries>;
    friend class Cluster;

public:
    static const size_t ParameterCount = 1;

private:
    TimeSeries(cluster_data_ptr cd, const char * alias) : Entry<TimeSeries>(cd, alias)
    {
    }
    virtual ~TimeSeries(void)
    {
    }

public:
    static void Init(v8::Local<v8::Object> exports)
    {
        Entry<TimeSeries>::Init(exports, "TimeSeries", [exports](v8::Local<v8::FunctionTemplate> tpl) {
            NODE_SET_PROTOTYPE_METHOD(tpl, "create", newColumnTpl<qdb_ts_create>);
            NODE_SET_PROTOTYPE_METHOD(tpl, "insert", newColumnTpl<qdb_ts_insert_columns>);
            NODE_SET_PROTOTYPE_METHOD(tpl, "columns", columns);

            NODE_SET_PROTOTYPE_METHOD(tpl, "Range", range);

            // Export to global namespace
            NODE_SET_METHOD(exports, "DoubleColumnInfo", columnInfoTpl<qdb_ts_column_double>);
            NODE_SET_METHOD(exports, "BlobColumnInfo", columnInfoTpl<qdb_ts_column_blob>);

            AddTsType<qdb_ts_column_type_t>(exports, "TS_COLUMN_BLOB", qdb_ts_column_blob);
            AddTsType<qdb_ts_column_type_t>(exports, "TS_COLUMN_DOUBLE", qdb_ts_column_double);

            AddTsType<qdb_ts_aggregation_type_t>(exports, "AggFirst", qdb_agg_first);
            AddTsType<qdb_ts_aggregation_type_t>(exports, "AggLast", qdb_agg_last);

            NODE_SET_METHOD(exports, "TsAggregation", aggregation);
        });
    }

private:
    static void New(const v8::FunctionCallbackInfo<v8::Value> & args);

    static void columns(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Entry<TimeSeries>::queue_work(args,
                                      [](qdb_request * qdb_req) {
                                          auto alias = qdb_req->input.alias.c_str();
                                          auto info = reinterpret_cast<qdb_ts_column_info_t **>(
                                              const_cast<void **>(&(qdb_req->output.content.buffer.begin)));
                                          auto count = &(qdb_req->output.content.buffer.size);

                                          qdb_req->output.error =
                                              qdb_ts_list_columns(qdb_req->handle(), alias, info, count);
                                      },
                                      TimeSeries::processArrayColumnsInfoResult, &ArgsEaterBinder::holder);
    }

    static void range(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        v8::Isolate * isolate = args.GetIsolate();
        v8::Local<v8::Object> obj = v8::Object::New(isolate);

        MethodMan call(args);
        if (args.Length() != 2)
        {
            call.throwException("Wrong number of arguments");
            return;
        }

        if (!args[0]->IsDate() || !args[1]->IsDate())
        {
            call.throwException("Wrong type of arguments");
            return;
        }

        obj->Set(v8::String::NewFromUtf8(isolate, "begin"), args[0]);
        obj->Set(v8::String::NewFromUtf8(isolate, "end"), args[1]);

        args.GetReturnValue().Set(obj);
    }

    static void aggregation(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        v8::Isolate * isolate = args.GetIsolate();
        v8::Local<v8::Object> obj = v8::Object::New(isolate);

        MethodMan call(args);
        if (args.Length() != 2 && args.Length() != 3)
        {
            call.throwException("Wrong number of arguments");
            return;
        }

        if (!args[0]->IsInt32() || !args[1]->IsObject())
        {
            call.throwException("Wrong type of arguments");
            return;
        }

        if (args.Length() == 3 && !args[2]->IsNumber())
        {
            call.throwException("Wrong type of arguments");
            return;
        }

        auto count = (args.Length() == 3) ? args[2]->Int32Value() : 0;

        obj->Set(v8::String::NewFromUtf8(isolate, "type"), args[0]);
        obj->Set(v8::String::NewFromUtf8(isolate, "range"), args[1]);
        obj->Set(v8::String::NewFromUtf8(isolate, "count"), v8::Number::New(isolate, count));

        args.GetReturnValue().Set(obj);
    }

    template <qdb_ts_column_type_t type>
    static void columnInfoTpl(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        v8::Isolate * isolate = args.GetIsolate();
        v8::Local<v8::Object> info = v8::Object::New(isolate);

        MethodMan call(args);
        if (args.Length() != 1)
        {
            call.throwException("Wrong number of arguments");
            return;
        }

        info->Set(v8::String::NewFromUtf8(isolate, "name"), args[0]);
        info->Set(v8::String::NewFromUtf8(isolate, "type"), v8::Integer::New(isolate, type));

        args.GetReturnValue().Set(info);
    }

    template <qdb_error_t func(qdb_handle_t, const char *, const qdb_ts_column_info_t *, qdb_size_t)>
    static void newColumnTpl(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Entry<TimeSeries>::queue_work(
            args,
            [](qdb_request * qdb_req) {
                auto & info = qdb_req->input.content.columns;
                std::vector<qdb_ts_column_info_t> cols;
                cols.resize(info.size());

                std::transform(info.cbegin(), info.cend(), cols.begin(), [qdb_req](const column_info & ci) {
                    qdb_ts_column_info_t info;
                    info.name = ci.name.c_str();
                    info.type = ci.type;
                    return info;
                });

                auto alias = qdb_req->input.alias.c_str();
                qdb_req->output.error = func(qdb_req->handle(), alias, cols.data(), cols.size());
            },
            TimeSeries::processColumnsCreateResult, &ArgsEaterBinder::holder, &ArgsEaterBinder::columnsInfo);
    }

    static void processArrayColumnsInfoResult(uv_work_t * req, int status);
    static void processColumnsCreateResult(uv_work_t * req, int status);

    template <typename Type>
    static void AddTsType(v8::Local<v8::Object> exports, const char * name, Type t)
    {
        v8::Isolate * isolate = exports->GetIsolate();
        detail::AddConstantProperty(isolate, exports, name, v8::Int32::New(isolate, t));
    }

private:
    static v8::Persistent<v8::Function> constructor;
};
} // quasardb namespace
