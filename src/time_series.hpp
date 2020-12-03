#pragma once

#include "entry.hpp"
#include <qdb/ts.h>
#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <uv.h>
#include <memory>
#include <string>

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
            NODE_SET_METHOD(exports, "StringColumnInfo", columnInfoTpl<qdb_ts_column_string>);
            NODE_SET_METHOD(exports, "Int64ColumnInfo", columnInfoTpl<qdb_ts_column_int64>);
            NODE_SET_METHOD(exports, "TimestampColumnInfo", columnInfoTpl<qdb_ts_column_timestamp>);
            NODE_SET_METHOD(exports, "SymbolColumnInfo", columnInfoTpl<qdb_ts_column_symbol>);

            AddTsType(exports, "TS_COLUMN_DOUBLE", qdb_ts_column_double);
            AddTsType(exports, "TS_COLUMN_BLOB", qdb_ts_column_blob);
            AddTsType(exports, "TS_COLUMN_STRING", qdb_ts_column_string);
            AddTsType(exports, "TS_COLUMN_INT64", qdb_ts_column_int64);
            AddTsType(exports, "TS_COLUMN_TIMESTAMP", qdb_ts_column_timestamp);
            AddTsType(exports, "TS_COLUMN_SYMBOL", qdb_ts_column_symbol);
        });
    }

private:
    static void New(const v8::FunctionCallbackInfo<v8::Value> & args);

    // :desc: Retrieves the columns
    // :args: callback(err, columns) (function) - A call back function that has error and columns parameter. columns
    // will store the result.
    static void columns(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Entry<TimeSeries>::queue_work(args,
                                      [](qdb_request * qdb_req) {
                                          auto alias = qdb_req->input.alias.c_str();
                                          auto info = reinterpret_cast<qdb_ts_column_info_ex_t **>(
                                              const_cast<void **>(&(qdb_req->output.content.buffer.begin)));
                                          auto count = &(qdb_req->output.content.buffer.size);

                                          qdb_req->output.error =
                                              qdb_ts_list_columns_ex(qdb_req->handle(), alias, info, count);
                                      },
                                      TimeSeries::processArrayColumnsInfoResult, &ArgsEaterBinder::holder);
    }

    template <qdb_ts_column_type_t type>
    static void columnInfoTpl(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        const bool is_symbol = (type == qdb_ts_column_symbol);

        v8::Isolate * isolate = args.GetIsolate();
        v8::Local<v8::Object> info = v8::Object::New(isolate);

        MethodMan call(args);
        if (!is_symbol && args.Length() != 1)
        {
            call.throwException("Wrong number of arguments: expected 'name'");
            return;
        }
        else if (is_symbol && args.Length() != 2)
        {
            call.throwException("Wrong number of arguments: expected 'name', 'symtable'");
            return;
        }

        info->Set(isolate->GetCurrentContext(), v8::String::NewFromUtf8(isolate, "name", v8::NewStringType::kNormal).ToLocalChecked(), args[0]);
        info->Set(isolate->GetCurrentContext(), v8::String::NewFromUtf8(isolate, "type", v8::NewStringType::kNormal).ToLocalChecked(), v8::Integer::New(isolate, type));
        if (is_symbol)
        {
            info->Set(isolate->GetCurrentContext(), v8::String::NewFromUtf8(isolate, "symtable", v8::NewStringType::kNormal).ToLocalChecked(), args[1]);
        }
        args.GetReturnValue().Set(info);
    }

    // :sign: create([columnInfo], function(err, columns) {})
    // :desc: Creates a column in the timeseries object
    // :args: [columnInfo] (qdb.DoubleColumnInfo/qdb.BlobColumnInfo) - The column to create
    // callback(err,columns) (function) - A call back function that has error and columns parameter. columns store the
    // result.
    static void ts_create(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Entry<TimeSeries>::queue_work(
            args,
            [](qdb_request * qdb_req) {
                const auto & info = qdb_req->input.content.columns;
                std::vector<qdb_ts_column_info_ex_t> cols;
                cols.resize(info.size());

                std::transform(info.cbegin(), info.cend(), cols.begin(), [](const column_info & ci) {
                    qdb_ts_column_info_ex_t info;
                    info.name = ci.name.c_str();
                    info.type = ci.type;
                    info.symtable = ci.symtable.c_str();
                    return info;
                });

                auto alias = qdb_req->input.alias.c_str();

                qdb_req->output.error =
                    qdb_ts_create_ex(qdb_req->handle(), alias, qdb_d_default_shard_size, cols.data(), cols.size());
            },
            TimeSeries::processColumnsCreateResult, &ArgsEaterBinder::holder, &ArgsEaterBinder::columnsInfo);
    }

    // :sign: insert(columnInfo, function(err, columns))
    // :desc: Inserts one or more columns
    // :args: columnInfo (qdb.DoubleColumnInfo / qdb.BlobColumnInfo) - column data to insert
    //  callback(err,columns) (function) - A call back function that has error and columns parameter. columns store the
    //  result.
    static void ts_insert_columns(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Entry<TimeSeries>::queue_work(
            args,
            [](qdb_request * qdb_req) {
                const auto & info = qdb_req->input.content.columns;
                std::vector<qdb_ts_column_info_ex_t> cols;
                cols.resize(info.size());

                std::cout << "INSERT COLS BEGIN" << std::endl;
                std::transform(info.cbegin(), info.cend(), cols.begin(), [](const column_info & ci) {
                    qdb_ts_column_info_ex_t info;
                    info.name = ci.name.c_str();
                    info.type = ci.type;
                    info.symtable = ci.symtable.c_str();
                    std::cout << "  - " << info.name << " (" << info.type << "),s=" << info.symtable << std::endl;
                    return info;
                });
                std::cout << "INSERT COLS END" << std::endl;

                auto alias = qdb_req->input.alias.c_str();
                qdb_req->output.error = qdb_ts_insert_columns_ex(qdb_req->handle(), alias, cols.data(), cols.size());
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

} // namespace quasardb
