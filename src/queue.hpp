
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

    class Queue : public Entity<Queue>
    {

        friend class Entity<Queue>;
        friend class Cluster;

    private:
        Queue(qdb_handle_t h, const char * alias) : Entity<Queue>(h, alias) {}
        virtual ~Queue(void) {}

    public:
        static void Init(v8::Handle<v8::Object> exports)
        {
            Entity<Queue>::Init(exports, "Queue", [](v8::Local<v8::FunctionTemplate> tpl)
            {
                NODE_SET_PROTOTYPE_METHOD(tpl, "pushFront", pushFront);
                NODE_SET_PROTOTYPE_METHOD(tpl, "pushBack", pushBack);
                NODE_SET_PROTOTYPE_METHOD(tpl, "popFront", popFront);
                NODE_SET_PROTOTYPE_METHOD(tpl, "popBack", popBack);
                NODE_SET_PROTOTYPE_METHOD(tpl, "front", front);
                NODE_SET_PROTOTYPE_METHOD(tpl, "back", back);
            });
        }

    public:
        template <typename F>
        static uv_work_t * spawnRequest(const MethodMan & call, F f)
        {
            Queue * pthis = call.nativeHolder<Queue>();
            assert(pthis);

            // first position must be the object to process
            ArgsEater eater(call);

            auto buf = eater.eatObject();
            auto callback = eater.eatCallback();

            if (!callback.second)
            {
                call.throwException("callback expected");
                return nullptr;
            }

            return pthis->MakeWorkItem(callback.first, buf, 0u, f);
        }

    public:
        static void processResult(uv_work_t * req, int status)
        {
            Entity::processBufferResult(req, status);
        }

    public:
        static void pushFront(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity::queue_work(args, [](qdb_request * qdb_req)
            {
                qdb_req->output.error = qdb_queue_push_front(qdb_req->handle, qdb_req->input.alias.c_str(), qdb_req->input.content.buffer.begin, qdb_req->input.content.buffer.size);
            });
        }

        static void pushBack(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity::queue_work(args, [](qdb_request * qdb_req)
            {
                qdb_req->output.error = qdb_queue_push_back(qdb_req->handle, qdb_req->input.alias.c_str(), qdb_req->input.content.buffer.begin, qdb_req->input.content.buffer.size);
            });
        }

        static void popFront(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity::queue_work(args, [](qdb_request * qdb_req)
            {
                qdb_req->output.error = qdb_queue_pop_front(qdb_req->handle, qdb_req->input.alias.c_str(), &(qdb_req->output.content.buffer.begin), &(qdb_req->output.content.buffer.size));
            });
        }

        static void popBack(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity::queue_work(args, [](qdb_request * qdb_req)
            {
                qdb_req->output.error = qdb_queue_pop_back(qdb_req->handle, qdb_req->input.alias.c_str(), &(qdb_req->output.content.buffer.begin), &(qdb_req->output.content.buffer.size));
            });
        }

        static void front(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity::queue_work(args, [](qdb_request * qdb_req)
            {
                qdb_req->output.error = qdb_queue_front(qdb_req->handle, qdb_req->input.alias.c_str(), &(qdb_req->output.content.buffer.begin), &(qdb_req->output.content.buffer.size));
            });
        }

        static void back(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity::queue_work(args, [](qdb_request * qdb_req)
            {
                qdb_req->output.error = qdb_queue_back(qdb_req->handle, qdb_req->input.alias.c_str(), &(qdb_req->output.content.buffer.begin), &(qdb_req->output.content.buffer.size));
            });
        }

    private:
        static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
    
    private:
        static v8::Persistent<v8::Function> constructor;

    };

}