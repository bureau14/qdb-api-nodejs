
#pragma once

#include <memory>
#include <string>

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <uv.h>

#include <qdb/client.h>

#include "entity.hpp"

namespace qdb
{

    class Cluster;

    class Blob : public Entity<Blob>
    {

        friend class Entity<Blob>;
        friend class Cluster;

    private:
        Blob(qdb_handle_t h, const char * alias) : Entity<Blob>(h, alias) {}
        virtual ~Blob(void) {}

    public:
        static void Init(v8::Handle<v8::Object> exports)
        {
            Entity<Blob>::Init(exports, "Blob", [](v8::Local<v8::FunctionTemplate> tpl)
            {
                NODE_SET_PROTOTYPE_METHOD(tpl, "put", put);
                NODE_SET_PROTOTYPE_METHOD(tpl, "update", update);
                NODE_SET_PROTOTYPE_METHOD(tpl, "get", get);
                NODE_SET_PROTOTYPE_METHOD(tpl, "remove", remove);
            });
        }
        
    public:
        template <typename F>
        static uv_work_t * spawnRequest(const MethodMan & call, F f)
        {
            Blob * pthis = call.nativeHolder<Blob>();
            assert(pthis);

            // first position must be the object to process
            ArgsEater eater(call);

            auto buf = eater.eatObject();
            auto expiry = eater.eatInteger<qdb_time_t>();
            auto callback = eater.eatCallback();

            if (!callback.second)
            {
                call.throwException("callback expected");
                return nullptr;                  
            }

            return pthis->MakeWorkItem(callback.first, buf, expiry.first, f);
        }

    public:
        static void processResult(uv_work_t * req, int status)
        {
            Entity::processBufferResult(req, status);
        }

    public:
        // put a new entry, with an optional expiry time
        static void put(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity::queue_work(args, [](qdb_request * qdb_req)
            {
                qdb_req->output.error = qdb_put(qdb_req->handle, qdb_req->input.alias.c_str(), qdb_req->input.content.buffer.begin, qdb_req->input.content.buffer.size, qdb_req->input.expiry);
            });
        }

        // put a new entry, with an optional expiry time
        static void update(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity::queue_work(args, [](qdb_request * qdb_req)
            {
                qdb_req->output.error = qdb_update(qdb_req->handle, qdb_req->input.alias.c_str(), qdb_req->input.content.buffer.begin, qdb_req->input.content.buffer.size, qdb_req->input.expiry);
            });
        }

        // return the alias content
        static void get(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity::queue_work(args, [](qdb_request * qdb_req)
            {
                qdb_req->output.error = qdb_get(qdb_req->handle, qdb_req->input.alias.c_str(), &(qdb_req->output.content.buffer.begin), &(qdb_req->output.content.buffer.size));
            });
        }

        // remove the alias
        static void remove(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity::queue_work(args, [](qdb_request * qdb_req)
            {
                qdb_req->output.error = qdb_remove(qdb_req->handle, qdb_req->input.alias.c_str());
            }); 
        }

    private:
        static void New(const v8::FunctionCallbackInfo<v8::Value>& args);

    private:
        static v8::Persistent<v8::Function> constructor;
    };

}

