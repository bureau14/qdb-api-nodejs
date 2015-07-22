
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

namespace detail
{

    inline std::shared_ptr<qdb_handle_t> make_shared_qdb_handle(qdb_handle_t h)
    {
        if (!h)
        {
            return std::shared_ptr<qdb_handle_t>();
        }

        auto custom_deleter = [](qdb_handle_t * h)
        {
            if (h)
            {
                qdb_close(*h);
            }
        };

        return std::shared_ptr<qdb_handle_t>(new qdb_handle_t(h), custom_deleter);
    }

}

    // cAmelCaSe :(
    class Cluster : public node::ObjectWrap 
    {

    public:
        explicit Cluster(qdb_handle_t h) : _handle(detail::make_shared_qdb_handle(h)) {}
        virtual ~Cluster(void) {}

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
            if (args.IsConstructCall()) 
            {
                MethodMan call(args);

                if (args.Length() != 1) 
                {
                    call.throwException("Wrong number of arguments");
                    return;
                }

                auto str = call.checkedArgString(0);
                if (!str.second)
                {
                    call.throwException("Expected a connection string as an argument");
                    return;
                }

                v8::String::Utf8Value utf8str(str.first);

                // create and open handle
                qdb_handle_t h = qdb_open_tcp();
                const qdb_error_t err = qdb_connect(h, *utf8str);
                if (err != qdb_e_ok)
                {
                    qdb_close(h);
                    call.throwException(qdb_error(err));;
                    return;    
                }

                // handle is now owned by the cluster object
                Cluster * cl = new Cluster(h);

                cl->Wrap(args.This());
                args.GetReturnValue().Set(args.This());
            } 
            else 
            {
                v8::Isolate* isolate = v8::Isolate::GetCurrent();
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
            if (args.IsConstructCall()) 
            {
                MethodMan call(args);

                if (args.Length() != 2) 
                {
                    call.throwException("Wrong number of arguments");
                    return;
                }

                // the first parameter is a Cluster object, let's unwrap it to get access to the underlying handle
                auto obj = call.checkedArgObject(0);

                if (!obj.second)
                {
                    call.throwException("Invalid parameter supplied to object");
                    return;
                } 

                auto str = call.checkedArgString(1);
                if (!str.second)
                {
                    call.throwException("Expected a string as an argument");
                    return;
                }

                v8::String::Utf8Value utf8str(str.first);

                // get access to the underlying handle
                Cluster * c = ObjectWrap::Unwrap<Cluster>(obj.first);

                Object * b = new Object(c->qdb_handle(), *utf8str);

                b->Wrap(args.This());
                args.GetReturnValue().Set(args.This());
            } 
            else 
            {
                v8::Isolate* isolate = v8::Isolate::GetCurrent();
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
        std::shared_ptr<qdb_handle_t> qdb_handle(void) const
        {
            return _handle;
        }

    private:
        std::shared_ptr<qdb_handle_t> _handle;

        static v8::Persistent<v8::Function> constructor;
    };



}

