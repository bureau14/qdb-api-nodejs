#pragma once

#include <string>

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <uv.h>

#include <qdb/blob.h>

#include "expirable_entry.hpp"

namespace quasardb
{

class Cluster;

class Blob : public ExpirableEntry<Blob>
{
    friend class Entry<Blob>;
    friend class Cluster;

public:
    static const size_t ParameterCount = 1;

private:
    Blob(cluster_data_ptr cd, const char * alias)
        : ExpirableEntry<Blob>(cd, alias)
    {
    }
    virtual ~Blob(void)
    {
    }

public:
    static void Init(v8::Local<v8::Object> exports)
    {
        ExpirableEntry<Blob>::Init(exports, "Blob",
            [](v8::Local<v8::FunctionTemplate> tpl)
            {
                NODE_SET_PROTOTYPE_METHOD(tpl, "put", put);
                NODE_SET_PROTOTYPE_METHOD(tpl, "update", update);
                NODE_SET_PROTOTYPE_METHOD(tpl, "get", get);
            });
    }

public:
    // put a new entry, with an optional expiry time
    // :desc: Sets blob's content but fails if the blob already exists. See also update().
    // :args: content (Buffer) - A string representing the blob's content to be set.
    // expiry_time (Date) - An optional Date with the absolute time at which the entry should expire.
    // callback(err) (function) - A callback or anonymous function with error parameter.
    static void put(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        ExpirableEntry<Blob>::queue_work(
            args,
            [](qdb_request * qdb_req)
            {
                qdb_req->output.error = qdb_blob_put(qdb_req->handle(), qdb_req->input.alias.c_str(),
                    qdb_req->input.content.buffer.begin, qdb_req->input.content.buffer.size, qdb_req->input.expiry);
            },
            ExpirableEntry<Blob>::processVoidResult, &ArgsEaterBinder::buffer, &ArgsEaterBinder::expiry);
    }

    // put a new entry, with an optional expiry time
    // :desc: Updates the content of the blob.
    // :args: content (Buffer) - A Buffer representing the blob's content to be added.
    // expiry_time (Date) - An optional Date with the absolute time at which the entry should expire.
    // callback(err) (function) - A callback or anonymous function with error parameter.
    static void update(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        ExpirableEntry<Blob>::queue_work(
            args,
            [](qdb_request * qdb_req)
            {
                qdb_req->output.error = qdb_blob_update(qdb_req->handle(), qdb_req->input.alias.c_str(),
                    qdb_req->input.content.buffer.begin, qdb_req->input.content.buffer.size, qdb_req->input.expiry);
            },
            ExpirableEntry<Blob>::processVoidResult, &ArgsEaterBinder::buffer, &ArgsEaterBinder::expiry);
    }

    // return the alias content
    // :desc: Retrieves the blob's content, passes to callback as data.
    // :args: callback(err, data) (function) - A callback or anonymous function with error and data parameters.
    static void get(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        ExpirableEntry<Blob>::queue_work(
            args,
            [](qdb_request * qdb_req)
            {
                qdb_req->output.error = qdb_blob_get(qdb_req->handle(), qdb_req->input.alias.c_str(),
                    &(qdb_req->output.content.buffer.begin), &(qdb_req->output.content.buffer.size));
            },
            ExpirableEntry<Blob>::processBufferResult, &ArgsEaterBinder::buffer);
    }

private:
    static void New(const v8::FunctionCallbackInfo<v8::Value> & args);

private:
    static v8::Persistent<v8::Function> constructor;
};

} // namespace quasardb
