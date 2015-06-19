
#pragma once

#include <memory>
#include <string>

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <uv.h>

#include <qdb/client.h>
#include <qdb/tag.h>

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
        // remove the alias
        static void remove(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            queue_work(args, 
                eatNoParam,
                [](qdb_request * qdb_req)
                {
                    qdb_req->output.error = qdb_remove(qdb_req->handle, qdb_req->input.alias.c_str());
                }, 
                processVoidResult); 
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
        template <typename F, typename Params>
        static uv_work_t * spawnRequest(const MethodMan & call, Params p, F f)
        {
            Blob * pthis = call.nativeHolder<Blob>();
            assert(pthis);

            // first position must be the object to process
            ArgsEater eater(call);

            qdb_request::query res = p(eater);

            auto callback = eater.eatCallback();

            if (!callback.second)
            {
                call.throwException("callback expected");
                return nullptr;                  
            }            

            qdb_request * qdb_req = new qdb_request(pthis->qdb_handle(), f, callback.first, pthis->native_alias(), res.expiry);

            qdb_req->input.content = res.content;

            uv_work_t * req = new uv_work_t();
            req->data = qdb_req;

            return req;
        }

        static qdb_request::query eatNoParam(ArgsEater & eater)
        {
            qdb_request::query res;

            return res;
        }

        static qdb_request::query eatStringParams(ArgsEater & eater)
        {
            qdb_request::query res;

            auto str = eater.eatString();

            if (str.second)
            {
                v8::String::Utf8Value val(str.first);
                res.content.str = std::string(*val, val.length());
            }

            return res;
        }

        static qdb_request::query eatBufExpiryParams(ArgsEater & eater)
        {
            qdb_request::query res;

            auto buf = eater.eatObject();

            if (buf.second)
            {
                res.content.buffer.begin = node::Buffer::Data(buf.first);
                res.content.buffer.size = node::Buffer::Length(buf.first);   
            }

            res.expiry = eater.eatInteger<qdb_time_t>().first;     

            return res;     
        }

        static qdb_request::query eatBufParams(ArgsEater & eater)
        {
            qdb_request::query res;

            auto buf = eater.eatObject();

            if (buf.second)
            {
                res.content.buffer.begin = node::Buffer::Data(buf.first);
                res.content.buffer.size = node::Buffer::Length(buf.first);   
            }

            return res; 
        }

        static qdb_request::query eatIntExpiryParams(ArgsEater & eater)
        {
            qdb_request::query res;

            res.content.value = eater.eatInteger<qdb_int>().first;
            res.expiry = eater.eatInteger<qdb_time_t>().first;    

            return res;
        }

    public:
        static void addTag(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            queue_work(args, 
                eatStringParams,
                [](qdb_request * qdb_req)
                {
                    qdb_req->output.error = qdb_add_tag(qdb_req->handle, qdb_req->input.alias.c_str(), qdb_req->input.content.str.c_str());
                }, 
                processVoidResult);
        }

        static void removeTag(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            queue_work(args, 
                eatStringParams,
                [](qdb_request * qdb_req)
                {
                    qdb_req->output.error = qdb_remove_tag(qdb_req->handle, qdb_req->input.alias.c_str(), qdb_req->input.content.str.c_str());
                }, 
                processVoidResult);
        }

        static void hasTag(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            queue_work(args, 
                eatStringParams,
                [](qdb_request * qdb_req)
                {
                    qdb_req->output.error = qdb_has_tag(qdb_req->handle, qdb_req->input.alias.c_str(), qdb_req->input.content.str.c_str());
                }, 
                processVoidResult);
        }

        static void getTags(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            queue_work(args, 
                eatNoParam,
                [](qdb_request * qdb_req)
                {
                    qdb_req->output.error = qdb_get_tags(qdb_req->handle, 
                        qdb_req->input.alias.c_str(),
                        reinterpret_cast<const char ***>(const_cast<char **>(&(qdb_req->output.content.buffer.begin))),
                        &(qdb_req->output.content.buffer.size));

                }, 
                processArrayStringResult);            
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
            NODE_SET_PROTOTYPE_METHOD(tpl, "alias",         Entity<Derivate>::alias);
            NODE_SET_PROTOTYPE_METHOD(tpl, "remove",        Entity<Derivate>::remove);
            NODE_SET_PROTOTYPE_METHOD(tpl, "addTag",        Entity<Derivate>::addTag);
            NODE_SET_PROTOTYPE_METHOD(tpl, "removeTag",     Entity<Derivate>::removeTag);
            NODE_SET_PROTOTYPE_METHOD(tpl, "hasTag",        Entity<Derivate>::hasTag);
            NODE_SET_PROTOTYPE_METHOD(tpl, "getTags",       Entity<Derivate>::getTags);

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

        static void processArrayStringResult(uv_work_t * req, int status)
        {
            // build an array out of the buffer

            v8::Isolate * isolate = v8::Isolate::GetCurrent();

            v8::Handle<v8::Array> array;

            v8::TryCatch try_catch;

            qdb_request * qdb_req = static_cast<qdb_request *>(req->data);
            assert(qdb_req);

            auto error_code = v8::Int32::New(isolate, static_cast<int32_t>(qdb_req->output.error));
            if (qdb_req->output.error == qdb_e_ok)
            {
                const char ** entries = reinterpret_cast<const char **>(const_cast<char *>(qdb_req->output.content.buffer.begin));
                const size_t entries_count = qdb_req->output.content.buffer.size;

                array = v8::Array::New(isolate, static_cast<int>(entries_count));
                if(!array.IsEmpty())
                {
                    for(size_t i = 0; i < entries_count; ++i)
                    {
                        auto str = v8::String::NewFromUtf8(isolate, entries[i]);
                        array->Set(static_cast<uint32_t>(i), str);
                    }
                }
                else
                {
                    error_code = v8::Int32::New(isolate, static_cast<int32_t>(qdb_e_no_memory));                    
                }

                // safe to call even on null/invalid buffers
                qdb_free_results(qdb_req->handle, entries, entries_count);
            }

            const unsigned argc = 2;
            v8::Handle<v8::Value> argv[argc] = { error_code, array };

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
        template <typename Params, typename F, typename Proc>
        static void queue_work(const v8::FunctionCallbackInfo<v8::Value> & args, Params params, F f, Proc process)
        {
             v8::TryCatch try_catch;

             MethodMan call(args);    

             uv_work_t * work = spawnRequest(call, params, f);

             if (try_catch.HasCaught()) 
             {
                node::FatalException(try_catch);
             } 

             assert(work);

             uv_queue_work(uv_default_loop(),
                work, 
                &Entity<Derivate>::callback_wrapper,
                process);

            // this is callback, the return value is undefined and the callback will get everything
            call.setUndefinedReturnValue();
        }

    private:
        qdb_handle_t _handle;
        std::unique_ptr<std::string> _alias;

    };
}