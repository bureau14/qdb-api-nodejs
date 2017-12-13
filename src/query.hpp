
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

class Query : public Entry<Query>
{
    friend class Entry<Query>;
    friend class Cluster;

public:
    static const size_t ParameterCount = 1;

private:
    Query(cluster_data_ptr cd, const char * alias) : Entry<Query>(cd, alias)
    {
    }
    virtual ~Query(void)
    {
    }

public:
    static void Init(v8::Local<v8::Object> exports)
    {
        Entry<Query>::Init(exports, "Query",
                           [](v8::Local<v8::FunctionTemplate> tpl) { NODE_SET_PROTOTYPE_METHOD(tpl, "run", run); });
    }

public:
    static void run(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Entry<Query>::queue_work(args,
                                 [](qdb_request * qdb_req) {
                                     qdb_req->output.error =
                                         qdb_query(qdb_req->handle(), qdb_req->input.alias.c_str(),
                                                   reinterpret_cast<const char ***>(
                                                       const_cast<void **>(&(qdb_req->output.content.buffer.begin))),
                                                   &(qdb_req->output.content.buffer.size));
                                 },
                                 Entry<Query>::processArrayStringResult);
    }

private:
    static void New(const v8::FunctionCallbackInfo<v8::Value> & args);

private:
    static v8::Persistent<v8::Function> constructor;
};

} // namespace quasardb
