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
            NODE_SET_PROTOTYPE_METHOD(tpl, "create", create);
            NODE_SET_PROTOTYPE_METHOD(tpl, "insert", insert);
            NODE_SET_PROTOTYPE_METHOD(tpl, "columns", columns);

            NODE_SET_METHOD(exports, "DoubleColumnInfo", doubleColumnInfo);
            NODE_SET_METHOD(exports, "BlobColumnInfo", blobColumnInfo);

            AddTsColumnType(exports, "TS_COLUMN_BLOB", qdb_ts_column_blob);
            AddTsColumnType(exports, "TS_COLUMN_DOUBLE", qdb_ts_column_double);
        });
    }

    static void blobColumnInfo(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        columnInfo(args, qdb_ts_column_blob);
    }

    static void doubleColumnInfo(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        columnInfo(args, qdb_ts_column_double);
    }

    static void create(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        createOrInsert(args, qdb_ts_create);
    }

    static void insert(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        createOrInsert(args, qdb_ts_insert_columns);
    }

    static void columns(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Entry<TimeSeries>::queue_work(args,
                                      [](qdb_request * qdb_req) {
                                          qdb_req->output.error = qdb_ts_list_columns(
                                              qdb_req->handle(), qdb_req->input.alias.c_str(),
                                              reinterpret_cast<qdb_ts_column_info_t **>(
                                                  const_cast<void **>(&(qdb_req->output.content.buffer.begin))),
                                              &(qdb_req->output.content.buffer.size));
                                      },
                                      TimeSeries::processArrayColumnsInfoResult, &ArgsEaterBinder::holder);
    }

private:
    static void New(const v8::FunctionCallbackInfo<v8::Value> & args);

    static void columnInfo(const v8::FunctionCallbackInfo<v8::Value> & args, qdb_ts_column_type_t type)
    {
        v8::Isolate * isolate = args.GetIsolate();
        v8::Local<v8::Object> info = v8::Object::New(isolate);

        if (args.Length() != 1)
        {
            const char * msg = "Expected one argument - column name";
            isolate->ThrowException(v8::Exception::SyntaxError(v8::String::NewFromUtf8(isolate, msg)));
            return;
        }

        info->Set(v8::String::NewFromUtf8(isolate, "name"), args[0]);
        info->Set(v8::String::NewFromUtf8(isolate, "type"), v8::Integer::New(isolate, type));

        args.GetReturnValue().Set(info);
    }

    template <typename Func>
    static void createOrInsert(const v8::FunctionCallbackInfo<v8::Value> & args, Func f)
    {
        Entry<TimeSeries>::queue_work(
            args,
            [f](qdb_request * qdb_req) {
                auto & info = qdb_req->input.content.columnsInfo;
                std::vector<qdb_ts_column_info_t> columns;
                columns.resize(info.size());

                std::transform(info.cbegin(), info.cend(), columns.begin(), [qdb_req](const column_info & ci) {
                    qdb_ts_column_info_t info;
                    info.name = ci.name.c_str();
                    info.type = ci.type;
                    return info;
                });

                qdb_req->output.error =
                    f(qdb_req->handle(), qdb_req->input.alias.c_str(), columns.data(), columns.size());
            },
            TimeSeries::processColumnsCreateResult, &ArgsEaterBinder::holder, &ArgsEaterBinder::columnsInfo);
    }

    static void processArrayColumnsInfoResult(uv_work_t * req, int status);
    static void processColumnsCreateResult(uv_work_t * req, int status);

    static void AddTsColumnType(v8::Local<v8::Object> exports, const char * name, qdb_ts_column_type_t type)
    {
        v8::Isolate * isolate = exports->GetIsolate();
        detail::AddConstantProperty(isolate, exports, name, v8::Int32::New(isolate, type));
    }

private:
    static v8::Persistent<v8::Function> constructor;
};
} // quasardb namespace
