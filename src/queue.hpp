
#pragma once

#include <memory>
#include <string>

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <uv.h>

#include <qdb/queue.h>

#include "entity.hpp"

namespace qdb
{

    class Queue : public Entity<Queue>
    {

        friend class Entity<Queue>;
        friend class Cluster;

    private:
        Queue(cluster_data_ptr cd, const char * alias) : Entity<Queue>(cd, alias) {}
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
                NODE_SET_PROTOTYPE_METHOD(tpl, "size", size);
                NODE_SET_PROTOTYPE_METHOD(tpl, "at", at);
            });
        }

    public:
        static void pushFront(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity<Queue>::queue_work(args,
                [](qdb_request * qdb_req)
                {
                    qdb_req->output.error = qdb_queue_push_front(qdb_req->handle(), qdb_req->input.alias.c_str(), qdb_req->input.content.buffer.begin, qdb_req->input.content.buffer.size);
                }, 
                Entity<Queue>::processVoidResult,
                &ArgsEaterBinder::buffer);
        }

        static void pushBack(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity<Queue>::queue_work(args, 
                [](qdb_request * qdb_req)
                {
                    qdb_req->output.error = qdb_queue_push_back(qdb_req->handle(), qdb_req->input.alias.c_str(), qdb_req->input.content.buffer.begin, qdb_req->input.content.buffer.size);
                }, 
                Entity<Queue>::processVoidResult,
                &ArgsEaterBinder::buffer);
        }

        static void popFront(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity<Queue>::queue_work(args, 
                [](qdb_request * qdb_req)
                {
                    qdb_req->output.error = qdb_queue_pop_front(qdb_req->handle(), qdb_req->input.alias.c_str(), &(qdb_req->output.content.buffer.begin), &(qdb_req->output.content.buffer.size));
                }, 
                Entity<Queue>::processBufferResult,
                &ArgsEaterBinder::buffer);
        }

        static void popBack(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity<Queue>::queue_work(args, 
                [](qdb_request * qdb_req)
                {
                    qdb_req->output.error = qdb_queue_pop_back(qdb_req->handle(), qdb_req->input.alias.c_str(), &(qdb_req->output.content.buffer.begin), &(qdb_req->output.content.buffer.size));
                }, 
                Entity<Queue>::processBufferResult,
                &ArgsEaterBinder::buffer);
        }

        static void front(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity<Queue>::queue_work(args, 
                [](qdb_request * qdb_req)
                {
                    qdb_req->output.error = qdb_queue_front(qdb_req->handle(), qdb_req->input.alias.c_str(), &(qdb_req->output.content.buffer.begin), &(qdb_req->output.content.buffer.size));
                }, 
                Entity<Queue>::processBufferResult,
                &ArgsEaterBinder::buffer);
        }

        static void back(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity<Queue>::queue_work(args, 
                [](qdb_request * qdb_req)
                {
                    qdb_req->output.error = qdb_queue_back(qdb_req->handle(), qdb_req->input.alias.c_str(), &(qdb_req->output.content.buffer.begin), &(qdb_req->output.content.buffer.size));
                }, 
                Entity<Queue>::processBufferResult,
                &ArgsEaterBinder::buffer);
        }

        static void size(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity<Queue>::queue_work(args, 
                [](qdb_request * qdb_req)
                {
                    qdb_size_t got_size = 0;
                    qdb_req->output.error = qdb_queue_size(qdb_req->handle(), qdb_req->input.alias.c_str(), &got_size);
                    qdb_req->output.content.value = static_cast<qdb_int_t>(got_size);
                }, 
                Entity<Queue>::processIntegerResult);
        }

        static void at(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity<Queue>::queue_work(args, 
                [](qdb_request * qdb_req)
                {
                    qdb_req->output.error = qdb_queue_get_at(qdb_req->handle(), 
                        qdb_req->input.alias.c_str(), 
                        qdb_req->input.content.value,
                        &(qdb_req->output.content.buffer.begin), 
                        &(qdb_req->output.content.buffer.size));
                }, 
                Entity<Queue>::processBufferResult,
                &ArgsEaterBinder::integer);
        }

    private:
        static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
    
    private:
        static v8::Persistent<v8::Function> constructor;

    };

}