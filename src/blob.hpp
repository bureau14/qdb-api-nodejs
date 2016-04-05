
#pragma once

#include <string>

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <uv.h>

#include <qdb/blob.h>

#include "expirable_entity.hpp"

namespace qdb
{

    class Cluster;

    class Blob : public ExpirableEntity<Blob>
    {

        friend class Entity<Blob>;
        friend class Cluster;

    private:
        Blob(cluster_data_ptr cd, const char * alias) : ExpirableEntity<Blob>(cd, alias) {}
        virtual ~Blob(void) {}

    public:
        static void Init(v8::Local<v8::Object> exports)
        {
            ExpirableEntity<Blob>::Init(exports, "Blob", [](v8::Local<v8::FunctionTemplate> tpl)
            {
                NODE_SET_PROTOTYPE_METHOD(tpl, "put", put);
                NODE_SET_PROTOTYPE_METHOD(tpl, "update", update);
                NODE_SET_PROTOTYPE_METHOD(tpl, "get", get);
            });
        }

    public:
        // put a new entry, with an optional expiry time
        static void put(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            ExpirableEntity<Blob>::queue_work(args,
                [](qdb_request * qdb_req)
                {
                    qdb_req->output.error = qdb_blob_put(qdb_req->handle(),
                        qdb_req->input.alias.c_str(),
                        qdb_req->input.content.buffer.begin,
                        qdb_req->input.content.buffer.size,
                        qdb_req->input.expiry);
                },
                ExpirableEntity<Blob>::processVoidResult,
                &ArgsEaterBinder::buffer,
                &ArgsEaterBinder::expiry);
        }

        // put a new entry, with an optional expiry time
        static void update(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            ExpirableEntity<Blob>::queue_work(args,
                [](qdb_request * qdb_req)
                {
                    qdb_req->output.error = qdb_blob_update(qdb_req->handle(),
                        qdb_req->input.alias.c_str(),
                        qdb_req->input.content.buffer.begin,
                        qdb_req->input.content.buffer.size,
                        qdb_req->input.expiry);
                },
                ExpirableEntity<Blob>::processVoidResult,
                &ArgsEaterBinder::buffer,
                &ArgsEaterBinder::expiry);
        }

        // return the alias content
        static void get(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            ExpirableEntity<Blob>::queue_work(args,
                [](qdb_request * qdb_req)
                {
                    qdb_req->output.error = qdb_blob_get(qdb_req->handle(),
                        qdb_req->input.alias.c_str(),
                        &(qdb_req->output.content.buffer.begin),
                        &(qdb_req->output.content.buffer.size));
                },
                ExpirableEntity<Blob>::processBufferResult,
                &ArgsEaterBinder::buffer);
        }

    private:
        static void New(const v8::FunctionCallbackInfo<v8::Value>& args);

    private:
        static v8::Persistent<v8::Function> constructor;
    };

}

