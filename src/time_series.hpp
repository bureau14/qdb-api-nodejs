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
            NODE_SET_PROTOTYPE_METHOD(tpl, "create", ts_create);
            NODE_SET_PROTOTYPE_METHOD(tpl, "insert", ts_insert_columns);
            NODE_SET_PROTOTYPE_METHOD(tpl, "columns", columns);

            // Export to global namespace
            NODE_SET_METHOD(exports, "DoubleColumnInfo", columnInfoTpl<qdb_ts_column_double>);
            NODE_SET_METHOD(exports, "BlobColumnInfo", columnInfoTpl<qdb_ts_column_blob>);

            AddTsType(exports, "TS_COLUMN_BLOB", qdb_ts_column_blob);
            AddTsType(exports, "TS_COLUMN_DOUBLE", qdb_ts_column_double);

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

    static void ts_create(const v8::FunctionCallbackInfo<v8::Value> & args)
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
                qdb_req->output.error = qdb_ts_create(qdb_req->handle(), alias, qdb_d_default_shard_size, cols.data(), cols.size());
            },
            TimeSeries::processColumnsCreateResult, &ArgsEaterBinder::holder, &ArgsEaterBinder::columnsInfo);
    }

    static void ts_insert_columns(const v8::FunctionCallbackInfo<v8::Value> & args)
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
                qdb_req->output.error = qdb_ts_insert_columns(qdb_req->handle(), alias, cols.data(), cols.size());
            },
            TimeSeries::processColumnsCreateResult, &ArgsEaterBinder::holder, &ArgsEaterBinder::columnsInfo);
    }

    static void processArrayColumnsInfoResult(uv_work_t * req, int status);
    static void processColumnsCreateResult(uv_work_t * req, int status);

    static void AddTsType(v8::Local<v8::Object> exports, const char * name, qdb_ts_column_type_t type)
    {
        v8::Isolate * isolate = exports->GetIsolate();
        detail::AddConstantProperty(isolate, exports, name, v8::Int32::New(isolate, type));
    }

private:
    static v8::Persistent<v8::Function> constructor;
};
} // quasardb namespace
