#pragma once

#include "entry.hpp"

#include <qdb/blob.h>
#include <qdb/client.h>

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <uv.h>

#include <memory>
#include <string>

namespace qdb
{

class Cluster;

class Range : public Entry<Range>
{
    friend class Entry<Range>;
    friend class Cluster;

public:
    static const size_t ParameterCount = 0;

private:
    Range(cluster_data_ptr cd) : Entry<Range>(cd, "")
    {
    }

    virtual ~Range(void)
    {
    }

public:
    static void Init(v8::Local<v8::Object> exports)
    {
        Entry<Range>::InitConstructorOnly(exports, "Range", [](v8::Local<v8::FunctionTemplate> tpl) {
            NODE_SET_PROTOTYPE_METHOD(tpl, "blobScan", blobScan);
        });
    }

public:
    static void blobScan(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Entry<Range>::queue_work(
            args,
            [](qdb_request * qdb_req) {
                qdb_req->output.error = qdb_blob_scan(
                    qdb_req->handle(), qdb_req->input.content.str.c_str(), qdb_req->input.content.str.size(),
                    /*max_count=*/qdb_req->input.content.value,
                    reinterpret_cast<const char ***>(const_cast<void **>(&(qdb_req->output.content.buffer.begin))),
                    &(qdb_req->output.content.buffer.size));
            },
            Entry<Range>::processArrayStringResult, &ArgsEaterBinder::string, &ArgsEaterBinder::integer);
    }

private:
    static void New(const v8::FunctionCallbackInfo<v8::Value> & args);

private:
    static v8::Persistent<v8::Function> constructor;
};

} // namespace qdb
