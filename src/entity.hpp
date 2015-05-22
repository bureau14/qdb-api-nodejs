
#pragma once

#include <memory>
#include <string>

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <uv.h>

#include <qdb/client.h>

#include "utilities.hpp"

namespace qdb
{

    class Cluster;

    template <typename Derivate>
    class Entity : public node::ObjectWrap 
    {
    public:
        static const int FieldsCount = 2;

    public:
        Entity(qdb_handle_t h, const char * alias) : _handle(h), _alias(new std::string(alias)) {}
        virtual ~Entity(void) 
        {
            _handle = nullptr;
        }

    public:
        static void alias(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            MethodMan this_call(args);
            
            Derivate * pthis = this_call.nativeHolder<Derivate>();

            assert(pthis);

            this_call.template setReturnValue<v8::String>(pthis->native_alias().c_str());
        }

    public:
        qdb_handle_t qdb_handle(void) const
        {
            return _handle;
        }

        const std::string & native_alias(void) const
        {
            assert(_alias);
            return *_alias;
        }

    public:
        static uv_work_t * MakeBadArgumentWorkItem(void) 
        {
            uv_work_t * req = new uv_work_t();
            req->data = new qdb_request(qdb_e_invalid_argument);
            return req;
        }

        template <typename F>
        uv_work_t * MakeWorkItem(const v8::Local<v8::Function> & callback, qdb_int value, qdb_time_t expiry, F f)
        {
            uv_work_t * req = new uv_work_t();

            qdb_request * qdb_req = new qdb_request(qdb_handle(), f, callback, native_alias(), expiry);

            qdb_req->input.content.value = value;

            // sanitize output
            qdb_req->output.content.value = 0;

            req->data = qdb_req;

            return req;  
        }

        template <typename F>
        uv_work_t * MakeWorkItem(const v8::Local<v8::Function> & callback, 
            const std::pair<v8::Local<v8::Object>, bool> & buffer,
            qdb_time_t expiry, 
            F f)
        {
            uv_work_t * req = new uv_work_t();

            qdb_request * qdb_req = new qdb_request(qdb_handle(), f, callback, native_alias(), expiry);

            if (buffer.second)
            {
                qdb_req->input.content.buffer.begin = node::Buffer::Data(buffer.first);
                qdb_req->input.content.buffer.size = node::Buffer::Length(buffer.first);   
            }
            else
            {
                qdb_req->input.content.buffer.begin = nullptr;
                qdb_req->input.content.buffer.size = 0;               
            }

            // sanitize output
            qdb_req->output.content.buffer.begin = nullptr;
            qdb_req->output.content.buffer.size = 0;    

            req->data = qdb_req;

            return req; 
        }

    public:
        template <typename F>
        static void Init(v8::Handle<v8::Object> exports, const char * className, F init)
        {
            v8::Isolate* isolate = v8::Isolate::GetCurrent();

            // Prepare constructor template
            v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, Derivate::New);
            tpl->SetClassName(v8::String::NewFromUtf8(isolate, className));
            tpl->InstanceTemplate()->SetInternalFieldCount(Entity<Derivate>::FieldsCount);
            
            // Prototype
            NODE_SET_PROTOTYPE_METHOD(tpl, "alias", Entity<Derivate>::alias);

            init(tpl);

            Derivate::constructor.Reset(isolate, tpl->GetFunction());
            exports->Set(v8::String::NewFromUtf8(isolate, className), tpl->GetFunction());
        }

        static void NewInstance(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            v8::Isolate* isolate = v8::Isolate::GetCurrent();
            v8::HandleScope scope(isolate);

            const unsigned argc = 2;
            v8::Handle<v8::Value> argv[argc] = { args[0], args[1] };
            v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, Derivate::constructor);
            v8::Local<v8::Object> instance = cons->NewInstance(argc, argv);

            args.GetReturnValue().Set(instance);
        }

    public:
        static void processBufferResult(uv_work_t * req, int status)
        {
            v8::Isolate * isolate = v8::Isolate::GetCurrent();

            v8::TryCatch try_catch;

            qdb_request * qdb_req = static_cast<qdb_request *>(req->data);
            assert(qdb_req);

            auto error_code = v8::Int32::New(isolate, static_cast<int32_t>(qdb_req->output.error));
            auto result_data = ((qdb_req->output.error == qdb_e_ok) && (qdb_req->output.content.buffer.size > 0u)) ?
                node::Buffer::New(isolate, qdb_req->output.content.buffer.begin, qdb_req->output.content.buffer.size) : node::Buffer::New(isolate, 0);

            // safe to call even on null/invalid buffers
            qdb_free_buffer(qdb_req->handle, qdb_req->output.content.buffer.begin);

            const unsigned argc = 2;
            v8::Handle<v8::Value> argv[argc] = { error_code, result_data };

            auto cb = qdb_req->callbackAsLocal();

            cb->Call(isolate->GetCurrentContext()->Global(), argc, argv);  
                    
            delete qdb_req;                     
                        
            if (try_catch.HasCaught()) 
            {
                node::FatalException(try_catch);
            }                
        }

        static void processIntegerResult(uv_work_t * req, int status)
        {
            v8::Isolate * isolate = v8::Isolate::GetCurrent();

            v8::TryCatch try_catch;

            qdb_request * qdb_req = static_cast<qdb_request *>(req->data);
            assert(qdb_req);

            auto error_code = v8::Int32::New(isolate, static_cast<int32_t>(qdb_req->output.error));
            auto result_data = (qdb_req->output.error == qdb_e_ok) ? v8::Number::New(isolate, static_cast<double>(qdb_req->output.content.value)) : v8::Number::New(isolate, 0.0);

            const unsigned argc = 2;
            v8::Handle<v8::Value> argv[argc] = { error_code, result_data };

            auto cb = qdb_req->callbackAsLocal();

            cb->Call(isolate->GetCurrentContext()->Global(), argc, argv);  
                    
            delete qdb_req;                     
                        
            if (try_catch.HasCaught()) 
            {
                node::FatalException(try_catch);
            }  
        }

        static void processVoidResult(uv_work_t * req, int status)
        {
            v8::Isolate * isolate = v8::Isolate::GetCurrent();

            v8::TryCatch try_catch;

            qdb_request * qdb_req = static_cast<qdb_request *>(req->data);
            assert(qdb_req);

            auto error_code = v8::Int32::New(isolate, static_cast<int32_t>(qdb_req->output.error));

            const unsigned argc = 1;
            v8::Handle<v8::Value> argv[argc] = { error_code };

            auto cb = qdb_req->callbackAsLocal();

            cb->Call(isolate->GetCurrentContext()->Global(), argc, argv);  
                    
            delete qdb_req;                     
                        
            if (try_catch.HasCaught()) 
            {
                node::FatalException(try_catch);
            }    
        }

    private:
        static void callback_wrapper(uv_work_t * req)
        {
            static_cast<qdb_request *>(req->data)->execute();
        }

    public:
        template <typename F>
        static void queue_work(const v8::FunctionCallbackInfo<v8::Value> & args, F f)
        {
             v8::TryCatch try_catch;

             MethodMan call(args);    

             uv_work_t * work = Derivate::spawnRequest(call, f);

             if (try_catch.HasCaught()) 
             {
                node::FatalException(try_catch);
             } 

             assert(work);

             uv_queue_work(uv_default_loop(),
                work, 
                &Entity<Derivate>::callback_wrapper,
                Derivate::processResult);

            // this is callback, the return value is undefined and the callback will get everything
            call.setUndefinedReturnValue();
        }

    private:
        qdb_handle_t _handle;
        std::unique_ptr<std::string> _alias;

    };
}