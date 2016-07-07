
#pragma once

#include <memory>
#include <string>

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <uv.h>

#include <qdb/integer.h>

#include "expirable_entry.hpp"

namespace quasardb
{

class Integer : public ExpirableEntry<Integer>
{

    friend class Entry<Integer>;
    friend class Cluster;

public:
    static const size_t ParameterCount = 1;

private:
    Integer(cluster_data_ptr cd, const char * alias) : ExpirableEntry<Integer>(cd, alias)
    {
    }
    virtual ~Integer(void)
    {
    }

public:
    static void Init(v8::Local<v8::Object> exports)
    {
        ExpirableEntry<Integer>::Init(exports, "Integer", [](v8::Local<v8::FunctionTemplate> tpl) {
            NODE_SET_PROTOTYPE_METHOD(tpl, "put", Integer::put);
            NODE_SET_PROTOTYPE_METHOD(tpl, "update", Integer::update);
            NODE_SET_PROTOTYPE_METHOD(tpl, "get", Integer::get);
            NODE_SET_PROTOTYPE_METHOD(tpl, "remove", Integer::remove);
            NODE_SET_PROTOTYPE_METHOD(tpl, "add", Integer::add);
        });
    }

public:
    // put a new entry, with an optional expiry time
    static void put(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        ExpirableEntry<Integer>::queue_work(
            args,
            [](qdb_request * qdb_req) {
                qdb_req->output.error = qdb_int_put(qdb_req->handle(), qdb_req->input.alias.c_str(),
                                                    qdb_req->input.content.value, qdb_req->input.expiry);
            },
            ExpirableEntry<Integer>::processVoidResult, &ArgsEaterBinder::integer, &ArgsEaterBinder::expiry);
    }

    // put a new entry, with an optional expiry time
    static void update(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        ExpirableEntry<Integer>::queue_work(
            args,
            [](qdb_request * qdb_req) {
                qdb_req->output.error = qdb_int_update(qdb_req->handle(), qdb_req->input.alias.c_str(),
                                                       qdb_req->input.content.value, qdb_req->input.expiry);
            },
            ExpirableEntry<Integer>::processVoidResult, &ArgsEaterBinder::integer, &ArgsEaterBinder::expiry);
    }

    // return the alias content
    static void get(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        ExpirableEntry<Integer>::queue_work(args,
                                            [](qdb_request * qdb_req) {
                                                qdb_req->output.error =
                                                    qdb_int_get(qdb_req->handle(), qdb_req->input.alias.c_str(),
                                                                &(qdb_req->output.content.value));
                                            },
                                            ExpirableEntry<Integer>::processIntegerResult);
    }

    // remove the alias
    static void remove(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        ExpirableEntry<Integer>::queue_work(args,
                                            [](qdb_request * qdb_req) {
                                                qdb_req->output.error =
                                                    qdb_remove(qdb_req->handle(), qdb_req->input.alias.c_str());
                                            },
                                            ExpirableEntry<Integer>::processVoidResult);
    }

    // add
    static void add(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        ExpirableEntry<Integer>::queue_work(args,
                                            [](qdb_request * qdb_req) {
                                                qdb_req->output.error = qdb_int_add(
                                                    qdb_req->handle(), qdb_req->input.alias.c_str(),
                                                    qdb_req->input.content.value, &(qdb_req->output.content.value));
                                            },
                                            ExpirableEntry<Integer>::processIntegerResult, &ArgsEaterBinder::integer);
    }

private:
    static void New(const v8::FunctionCallbackInfo<v8::Value> & args);

private:
    static v8::Persistent<v8::Function> constructor;
};
}
