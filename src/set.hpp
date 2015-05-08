
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

    class Set : public Entity<Set>
    {

        friend class Entity<Set>;
        friend class Cluster;

    private:
        Set(qdb_handle_t h, const char * alias) : Entity<Set>(h, alias) {}
        virtual ~Set(void) {}

    public:
        static void Init(v8::Handle<v8::Object> exports)
        {
            Entity<Set>::Init(exports, "Set", [](v8::Local<v8::FunctionTemplate> tpl)
            {
                NODE_SET_PROTOTYPE_METHOD(tpl, "insert", insert);
                NODE_SET_PROTOTYPE_METHOD(tpl, "erase", erase);
                NODE_SET_PROTOTYPE_METHOD(tpl, "contains", contains);
            });
        }

    public:
        template <typename F>
        static uv_work_t * spawnRequest(const MethodMan<Set> & call, F f)
        {
            Set * pthis = call.nativeHolder();
            assert(pthis);

            qdb_request * qdb_req = nullptr;

            if (!call.checkArgc(1))
            {
                qdb_req = new qdb_request(qdb_e_invalid_argument);
            }
            else
            {
                qdb_req = new qdb_request(pthis->qdb_handle(), f, call.argCallback(0), pthis->native_alias());
                
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

            const unsigned argc = 1;
            v8::Local<v8::Value> argv[argc] = { error_code, };

            cb->Call(isolate->GetCurrentContext()->Global(), argc, argv);                    
                    
            delete qdb_req;                     
                        
            if (try_catch.HasCaught() ) 
            {
                node::FatalException(try_catch);
            }              
        }

    public:
        static void insert(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity::queue_work(args, [](qdb_request * qdb_req)
            {
                qdb_req->output.error = qdb_set_insert(qdb_req->handle, qdb_req->input.alias.c_str(), qdb_req->input.content.buffer.begin, qdb_req->input.content.buffer.size);
            });
        }

        static void erase(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity::queue_work(args, [](qdb_request * qdb_req)
            {
                qdb_req->output.error = qdb_set_contains(qdb_req->handle, qdb_req->input.alias.c_str(), qdb_req->input.content.buffer.begin, qdb_req->input.content.buffer.size);
            });
        }

        static void contains(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity::queue_work(args, [](qdb_request * qdb_req)
            {
                qdb_req->output.error = qdb_set_erase(qdb_req->handle, qdb_req->input.alias.c_str(), qdb_req->input.content.buffer.begin, qdb_req->input.content.buffer.size);
            });
        }

    private:
        static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
    
    private:
        static v8::Persistent<v8::Function> constructor;

    };

}