
#pragma once

#include <memory>
#include <string>

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <uv.h>

#include "entry.hpp"
#include <qdb/client.h>
#include <qdb/query.h>

namespace quasardb
{

class Cluster;

class QueryFind : public Entry<QueryFind>
{
    friend class Entry<QueryFind>;
    friend class Cluster;

public:
    static const size_t ParameterCount = 1;

private:
    QueryFind(cluster_data_ptr cd, const char * alias) : Entry<QueryFind>(cd, alias)
    {
    }
    virtual ~QueryFind(void)
    {
    }

public:
    static void Init(v8::Local<v8::Object> exports)
    {
        Entry<QueryFind>::Init(exports, "QueryFind",
                           [](v8::Local<v8::FunctionTemplate> tpl) { NODE_SET_PROTOTYPE_METHOD(tpl, "run", run); });
    }

public:
    static void run(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Entry<QueryFind>::queue_work(args,
                                 [](qdb_request * qdb_req) {
                                     qdb_req->output.error =
                                         qdb_query_find(qdb_req->handle(), qdb_req->input.alias.c_str(),
                                                   reinterpret_cast<const char ***>(
                                                       const_cast<void **>(&(qdb_req->output.content.buffer.begin))),
                                                   &(qdb_req->output.content.buffer.size));
                                 },
                                 Entry<QueryFind>::processArrayStringResult);
    }

private:
    static void New(const v8::FunctionCallbackInfo<v8::Value> & args);

private:
    static v8::Persistent<v8::Function> constructor;
};

} // namespace quasardb
