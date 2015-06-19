
#pragma once

#include <node.h>
#include <node_object_wrap.h>

#include "blob.hpp"
#include "integer.hpp"
#include "queue.hpp"
#include "hset.hpp"
#include "tag.hpp"

namespace qdb
{

    // cAmelCaSe :(
    class Cluster : public node::ObjectWrap 
    {

    public:
        explicit Cluster(qdb_handle_t h) : _handle(h) {}

        virtual ~Cluster(void)
        {
            // close a nullptr handle is safe
            qdb_close(_handle);
            _handle = nullptr;
        }

    public:
        static void Init(v8::Handle<v8::Object> exports)
        {
            v8::Isolate* isolate = v8::Isolate::GetCurrent();

            // Prepare constructor template
            v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, New);
            tpl->SetClassName(v8::String::NewFromUtf8(isolate, "Cluster"));
            tpl->InstanceTemplate()->SetInternalFieldCount(1);

            // Prototype
            NODE_SET_PROTOTYPE_METHOD(tpl, "blob", blob);
            NODE_SET_PROTOTYPE_METHOD(tpl, "integer", integer);
            NODE_SET_PROTOTYPE_METHOD(tpl, "queue", queue);
            NODE_SET_PROTOTYPE_METHOD(tpl, "set", set);
            NODE_SET_PROTOTYPE_METHOD(tpl, "tag", tag);

            constructor.Reset(isolate, tpl->GetFunction());
            exports->Set(v8::String::NewFromUtf8(isolate, "Cluster"), tpl->GetFunction());
        }

    private:
        static void New(const v8::FunctionCallbackInfo<v8::Value>& args)
        {
            v8::Isolate* isolate = v8::Isolate::GetCurrent();
            v8::HandleScope scope(isolate);

            if (args.IsConstructCall()) 
            {

                if (args.Length() != 1) 
                {
                    isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Wrong number of arguments")));
                    return;
                }
                
                v8::String::Utf8Value uri(args[0]->ToString());

                // create and open handle
                qdb_handle_t h = qdb_open_tcp();
                const qdb_error_t err = qdb_connect(h, *uri);
                if (err != qdb_e_ok)
                {
                    std::string err_str = qdb::make_error_string(err);
                    isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, err_str.c_str())));
                    return;
                }

                // handle is now owned by the cluster object
                Cluster * cl = new Cluster(h);

                cl->Wrap(args.This());
                args.GetReturnValue().Set(args.This());
            } 
            else 
            {
                // Invoked as plain function `MyObject(...)`, turn into construct call.
                const int argc = 1;
                v8::Local<v8::Value> argv[argc] = { args[0] };
                v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);
                args.GetReturnValue().Set(cons->NewInstance(argc, argv));
            }
        }

    public:
        template <typename Object>
        static void newObject(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            v8::Isolate* isolate = v8::Isolate::GetCurrent();
            v8::HandleScope scope(isolate);

            if (args.IsConstructCall()) 
            {
                if (args.Length() != 2) 
                {
                    isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Wrong number of arguments")));
                    return;
                }
        
                // the first parameter is a Cluster object, let's unwrap it to get access to the underlying handle
                if (!args[0]->IsObject())
                {
                    isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Invalid parameter supplied to object")));
                    return;
                }

                v8::String::Utf8Value alias(args[1]->ToString());

                // get access to the underlying handle
                Cluster * c = ObjectWrap::Unwrap<Cluster>(args[0]->ToObject());

                Object * b = new Object(*c, *alias);

                b->Wrap(args.This());
                args.GetReturnValue().Set(args.This());
            } 
            else 
            {
                // Invoked as plain function `MyObject(...)`, turn into construct call.
                const int argc = 2;
                v8::Local<v8::Value> argv[argc] = { args[0], args[1] };
                v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);
                args.GetReturnValue().Set(cons->NewInstance(argc, argv));
            }      
        }

    private:
        template <typename Object>
        static void objectFactory(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            v8::Isolate* isolate = v8::Isolate::GetCurrent();
            v8::HandleScope scope(isolate);

            v8::Local<v8::Object> the_cluster = args.Holder();

            const int argc = 2;
            v8::Local<v8::Value> argv[argc] = { the_cluster, args[0] };
            v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, Object::constructor);
            args.GetReturnValue().Set(cons->NewInstance(argc, argv));        
        }

    public:
        static void blob(const v8::FunctionCallbackInfo<v8::Value> & args) 
        {
            objectFactory<Blob>(args);
        }

        static void integer(const v8::FunctionCallbackInfo<v8::Value> & args) 
        {
            objectFactory<Integer>(args);
        }
        
        static void queue(const v8::FunctionCallbackInfo<v8::Value> & args) 
        {
            objectFactory<Queue>(args);
        }

        static void set(const v8::FunctionCallbackInfo<v8::Value> & args) 
        {
            objectFactory<Set>(args);
        }

        static void tag(const v8::FunctionCallbackInfo<v8::Value> & args) 
        {
            objectFactory<Tag>(args);
        }

    public:
        operator qdb_handle_t ()
        {
            return _handle;
        }

    private:
        qdb_handle_t _handle;

        static v8::Persistent<v8::Function> constructor;
    };



}

