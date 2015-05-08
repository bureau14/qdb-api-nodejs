
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
        static uv_work_t * spawnRequest(const MethodMan<Blob> & call, F f)
        {
            Blob * pthis = call.nativeHolder();
            assert(pthis);

            qdb_request * qdb_req = nullptr;

            if (!call.checkArgc(1))
            {
                qdb_req = new qdb_request(qdb_e_invalid_argument);
            }
            else
            {
                // do we have expiry?
                qdb_time_t expiry = 0;

                if (call.args()[2]->IsNumber())
                {
                    expiry = static_cast<qdb_time_t>(call.args()[2]->NumberValue());
                }

                qdb_req = new qdb_request(pthis->qdb_handle(), f, call.argCallback(0), pthis->native_alias(), expiry);
                
                // do we have content
                if (call.args()[1]->IsObject())
                {
                    auto buf = call.args()[1]->ToObject();

                    qdb_req->input.content.buffer.begin = node::Buffer::Data(buf);
                    qdb_req->input.content.buffer.size = node::Buffer::Length(buf);
                }
                else
                {
                    qdb_req->input.content.buffer.begin = nullptr;
                    qdb_req->input.content.buffer.size = 0;
                }

                // sanitize output
                qdb_req->output.content.buffer.begin = nullptr;
                qdb_req->output.content.buffer.size = 0;
            }

            uv_work_t * req = new uv_work_t();
            req->data = qdb_req;

            return req;
        }

    public:
        static void processResult(uv_work_t * req, int status)
        {
            v8::Isolate * isolate = v8::Isolate::GetCurrent();

            v8::TryCatch try_catch;

            qdb_request * qdb_req = static_cast<qdb_request *>(req->data);
            assert(qdb_req);

            // get the callback and give the result (error code as integer + result as string if any)
            v8::Local<v8::Function> cb = qdb_req->callbackAsLocal();

            auto error_code = v8::Int32::New(isolate, static_cast<int32_t>(qdb_req->output.error));
            auto result_data = ((qdb_req->output.error == qdb_e_ok) && (qdb_req->output.content.buffer.size > 0u)) ?
                node::Buffer::New(isolate, qdb_req->output.content.buffer.begin, qdb_req->output.content.buffer.size) : node::Buffer::New(isolate, 0);

            // safe to call even on null/invalid buffers
            qdb_free_buffer(qdb_req->handle, qdb_req->output.content.buffer.begin);

            const unsigned argc = 2;
            v8::Local<v8::Value> argv[argc] = { error_code, result_data };

            cb->Call(isolate->GetCurrentContext()->Global(), argc, argv);                    
                    
            delete qdb_req;                     
                        
            if (try_catch.HasCaught() ) 
            {
                node::FatalException(try_catch);
            }              
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

