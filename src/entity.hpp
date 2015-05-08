
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
            MethodMan<Derivate> this_call(args);
            
            Derivate * pthis = this_call.nativeHolder();

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
        template <typename F>
        static void queue_work(const v8::FunctionCallbackInfo<v8::Value> & args, F f)
        {
             MethodMan<Derivate> call(args);        

             uv_queue_work(uv_default_loop(),
                Derivate::spawnRequest(call, f), 
                [](uv_work_t * req)
                {
                    static_cast<qdb_request *>(req->data)->execute();
                },
                Derivate::processResult);

            // this is callback, the return value is undefined and the callback will get everything
            call.setUndefinedReturnValue();
        }

    private:
        qdb_handle_t _handle;
        std::unique_ptr<std::string> _alias;

    };
}