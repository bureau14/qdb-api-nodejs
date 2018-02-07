
#pragma once

#include <memory>
#include <string>

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <uv.h>

#include <qdb/hset.h>

#include "entry.hpp"

namespace quasardb
{

class Set : public Entry<Set>
{

    friend class Entry<Set>;
    friend class Cluster;

public:
    static const size_t ParameterCount = 1;

private:
    Set(cluster_data_ptr cd, const char * alias) : Entry<Set>(cd, alias)
    {
    }
    virtual ~Set(void)
    {
    }

public:
    static void Init(v8::Local<v8::Object> exports)
    {
        Entry<Set>::Init(exports, "Set", [](v8::Local<v8::FunctionTemplate> tpl) {
            NODE_SET_PROTOTYPE_METHOD(tpl, "insert", insert);
            NODE_SET_PROTOTYPE_METHOD(tpl, "erase", erase);
            NODE_SET_PROTOTYPE_METHOD(tpl, "contains", contains);
        });
    }

public:

    // :desc: Adds the specified value to the set.
    // :args: value (Buffer) - the value to add to the Set.
    // callback(err, data) (function) - A callback or anonymous function with error and data parameters.
    // :returns : true if the value was added, false if it was already present in the set.
    static void insert(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Entry<Set>::queue_work(args,
                               [](qdb_request * qdb_req) {
                                   qdb_req->output.error = qdb_hset_insert(
                                       qdb_req->handle(), qdb_req->input.alias.c_str(),
                                       qdb_req->input.content.buffer.begin, qdb_req->input.content.buffer.size);
                               },
                               Entry<Set>::processVoidResult, &ArgsEaterBinder::buffer);
    }

    //:desc: Removes the value from the set.
    //:args: value (Buffer) - the value to remove from the Set.
    //callback(err, data) (function) - A callback or anonymous function with error and data parameters.
    static void erase(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Entry<Set>::queue_work(args,
                               [](qdb_request * qdb_req) {
                                   qdb_req->output.error = qdb_hset_erase(
                                       qdb_req->handle(), qdb_req->input.alias.c_str(),
                                       qdb_req->input.content.buffer.begin, qdb_req->input.content.buffer.size);
                               },
                               Entry<Set>::processVoidResult, &ArgsEaterBinder::buffer);
    }

    //:desc: Determines if the value is present in the set.
    //:args: value (Buffer) - the value to look for in the Set.
    // callback(err, data) (function) - A callback or anonymous function with error and data parameters.
    static void contains(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Entry<Set>::queue_work(args,
                               [](qdb_request * qdb_req) {
                                   qdb_req->output.error = qdb_hset_contains(
                                       qdb_req->handle(), qdb_req->input.alias.c_str(),
                                       qdb_req->input.content.buffer.begin, qdb_req->input.content.buffer.size);
                               },
                               Entry<Set>::processVoidResult, &ArgsEaterBinder::buffer);
    }

private:
    static void New(const v8::FunctionCallbackInfo<v8::Value> & args);

private:
    static v8::Persistent<v8::Function> constructor;
};
}