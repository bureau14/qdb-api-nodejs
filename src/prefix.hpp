#pragma once

#include "entry.hpp"

#include <qdb/client.h>
#include <qdb/prefix.h>

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <uv.h>

#include <memory>
#include <string>

namespace qdb
{

class Cluster;

class Prefix : public Entry<Prefix>
{
    friend class Entry<Prefix>;
    friend class Cluster;

private:
    Prefix(cluster_data_ptr cd, const char * prefix) : Entry<Prefix>(cd, prefix)
    {
    }

    virtual ~Prefix(void)
    {
    }

public:
    static void Init(v8::Local<v8::Object> exports)
    {
        v8::Isolate * isolate = exports->GetIsolate();

        // Prepare constructor template
        v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, Prefix::New);
        tpl->SetClassName(v8::String::NewFromUtf8(isolate, "Prefix"));
        tpl->InstanceTemplate()->SetInternalFieldCount(Entry<Prefix>::FieldsCount);

        NODE_SET_PROTOTYPE_METHOD(tpl, "getEntries", getEntries);
        NODE_SET_PROTOTYPE_METHOD(tpl, "prefix", Entry<Prefix>::alias);

        Prefix::constructor.Reset(isolate, tpl->GetFunction());

        exports->Set(v8::String::NewFromUtf8(isolate, "Prefix"), tpl->GetFunction());
    }

public:
    static void getEntries(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Entry<Prefix>::queue_work(
            args,
            [](qdb_request * qdb_req) {
                qdb_req->output.error = qdb_prefix_get(
                    qdb_req->handle(), qdb_req->input.alias.c_str(), /*max_count=*/qdb_req->input.content.value,
                    reinterpret_cast<const char ***>(const_cast<void **>(&(qdb_req->output.content.buffer.begin))),
                    &(qdb_req->output.content.buffer.size));
            },
            Entry<Prefix>::processArrayStringResult, &ArgsEaterBinder::integer);
    }

private:
    static void New(const v8::FunctionCallbackInfo<v8::Value> & args);

private:
    static v8::Persistent<v8::Function> constructor;
};

} // namespace qdb
