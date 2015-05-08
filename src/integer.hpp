
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

    class Integer : public Entity<Integer>
    {

        friend class Entity<Integer>;
        friend class Cluster;

    private:
        Integer(qdb_handle_t h, const char * alias) : Entity<Integer>(h, alias) {}
        virtual ~Integer(void) {}

    public:
        static void Init(v8::Handle<v8::Object> exports)
        {
            Entity<Integer>::Init(exports, "Integer", [](v8::Local<v8::FunctionTemplate> tpl)
            {
                NODE_SET_PROTOTYPE_METHOD(tpl, "put", Integer::put);
                NODE_SET_PROTOTYPE_METHOD(tpl, "update", Integer::update);
                NODE_SET_PROTOTYPE_METHOD(tpl, "get", Integer::get);
                NODE_SET_PROTOTYPE_METHOD(tpl, "remove", Integer::remove);
                NODE_SET_PROTOTYPE_METHOD(tpl, "add", Integer::add);
            });
        }
        
    public:
        template <typename F>
        static uv_work_t * spawnRequest(const MethodMan<Integer> & call, F f)
        {
            Integer * pthis = call.nativeHolder();
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
                
                // do we have value
                if (call.args()[1]->IsNumber())
                {
                    qdb_req->input.content.value = static_cast<qdb_int>(call.args()[1]->NumberValue());
                }
                else
                {
                    qdb_req->input.content.value = 0;
                }

                // sanitize output
                qdb_req->output.content.value = 0;
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
            auto result_data = (qdb_req->output.error == qdb_e_ok) ? v8::Number::New(isolate, static_cast<double>(qdb_req->output.content.value)) : v8::Number::New(isolate, 0.0);

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
                qdb_req->output.error = qdb_int_put(qdb_req->handle, qdb_req->input.alias.c_str(), qdb_req->input.content.value, qdb_req->input.expiry);
            });
        }

        // put a new entry, with an optional expiry time
        static void update(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity::queue_work(args, [](qdb_request * qdb_req)
            {
                qdb_req->output.error = qdb_int_update(qdb_req->handle, qdb_req->input.alias.c_str(), qdb_req->input.content.value, qdb_req->input.expiry);
            });
        }

        // return the alias content
        static void get(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity::queue_work(args, [](qdb_request * qdb_req)
            {
                qdb_req->output.error = qdb_int_get(qdb_req->handle, qdb_req->input.alias.c_str(), &(qdb_req->output.content.value));
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

        // add
        static void add(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity::queue_work(args, [](qdb_request * qdb_req)
            {
                qdb_req->output.error = qdb_int_add(qdb_req->handle, qdb_req->input.alias.c_str(), qdb_req->input.content.value, &(qdb_req->output.content.value));
            });
        }

    private:
        static void New(const v8::FunctionCallbackInfo<v8::Value>& args);

    private:
        static v8::Persistent<v8::Function> constructor;
    };

}

