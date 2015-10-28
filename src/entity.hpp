
#pragma once

#include <memory>
#include <string>

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <uv.h>

#include <qdb/client.h>
#include <qdb/tag.h>

#include "cluster_data.hpp"
#include "utilities.hpp"

namespace qdb
{

    template <typename Derivate>
    class Entity : public node::ObjectWrap
    {
    public:
        static const int FieldsCount = 2;

    public:
        // if an entry is expirable, we will register the expiry management functions
        // this simplifies organization here
        // in other APIs generally we have an ExpirableEntry that inherits from Entity
        Entity(cluster_data_ptr cd, const char * alias) : _cluster_data(cd), _alias(new std::string(alias)) {}
        virtual ~Entity(void) {}

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
                [](qdb_request * qdb_req)
                {
                    qdb_req->output.error = qdb_remove(qdb_req->handle(), qdb_req->input.alias.c_str());
                },
                processVoidResult,
                &ArgsEaterBinder::none);
        }

    public:
        qdb_handle_ptr qdb_handle(void) const
        {
            return _cluster_data->handle();
        }

        const std::string & native_alias(void) const
        {
            assert(_alias);
            return *_alias;
        }

    public:
        template <typename F, typename... Params>
        static uv_work_t * spawnRequest(const MethodMan & call, F f, Params... p)
        {
            Derivate * pthis = call.nativeHolder<Derivate>();
            assert(pthis);

            qdb_request * qdb_req = new qdb_request(pthis->_cluster_data, f, pthis->native_alias());

            ArgsEaterBinder eaterBinder(call);

            eaterBinder.eatThem(*qdb_req, p...);

            uv_work_t * req = nullptr;

            if (eaterBinder.bindCallback(*qdb_req))
            {
                req = new uv_work_t();
                req->data = qdb_req;
            }
            else
            {
                delete qdb_req;
                call.throwException("callback expected");
            }

            return req;
        }

    public:
        // tag management function
        static void addTag(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            queue_work(args,
                [](qdb_request * qdb_req)
                {
                    qdb_req->output.error = qdb_add_tag(qdb_req->handle(), qdb_req->input.alias.c_str(), qdb_req->input.content.str.c_str());
                },
                processVoidResult,
                &ArgsEaterBinder::string);
        }

        static void removeTag(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            queue_work(args,
                [](qdb_request * qdb_req)
                {
                    qdb_req->output.error = qdb_remove_tag(qdb_req->handle(), qdb_req->input.alias.c_str(), qdb_req->input.content.str.c_str());
                },
                processVoidResult,
                &ArgsEaterBinder::string);
        }

        static void hasTag(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            queue_work(args,
                [](qdb_request * qdb_req)
                {
                    qdb_req->output.error = qdb_has_tag(qdb_req->handle(), qdb_req->input.alias.c_str(), qdb_req->input.content.str.c_str());
                },
                processVoidResult,
                &ArgsEaterBinder::string);
        }

        static void getTags(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            queue_work(args,
                [](qdb_request * qdb_req)
                {
                    qdb_req->output.error = qdb_get_tags(qdb_req->handle(),
                        qdb_req->input.alias.c_str(),
                        reinterpret_cast<const char ***>(const_cast<char **>(&(qdb_req->output.content.buffer.begin))),
                        &(qdb_req->output.content.buffer.size));

                },
                processArrayStringResult,
                &ArgsEaterBinder::none);
        }

    public:
        template <typename F>
        static void Init(v8::Local<v8::Object> exports, const char * className, F init)
        {
            v8::Isolate* isolate = v8::Isolate::GetCurrent();
            v8::HandleScope scope(isolate);            

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
            v8::Local<v8::Value> argv[argc] = { args[0], args[1] };
            v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, Derivate::constructor);
            v8::Local<v8::Object> instance = cons->NewInstance(argc, argv);

            args.GetReturnValue().Set(instance);
        }

    private:
        static void processCallAndCleanUp(v8::Isolate * isolate,
            v8::TryCatch & try_catch,
            qdb_request * qdb_req,
            unsigned int argc,
            v8::Local<v8::Value> argv[])
        {
            const qdb_error_t err = (argc > 0) ? static_cast<qdb_error_t>(static_cast<unsigned int>(argv[0]->NumberValue())) : qdb_e_ok;

            if (!fatal_error(err))
            {
                auto cb = qdb_req->callbackAsLocal();
                cb->Call(isolate->GetCurrentContext()->Global(), argc, argv);
            }
            else
            {
                qdb_req->on_error(isolate, err);
            }

            delete qdb_req;

            if (try_catch.HasCaught())
            {
                node::FatalException(isolate, try_catch);
            }
        }

        static auto processErrorCode(v8::Isolate * isolate, int status, const qdb_request * req) -> decltype(v8::Int32::New(isolate, 0))
        {
            return (status < 0) ? v8::Int32::New(isolate, qdb_e_internal) : v8::Int32::New(isolate, static_cast<int32_t>(req->output.error));
        }

    public:
        static void processBufferResult(uv_work_t * req, int status)
        {
            v8::Isolate * isolate = v8::Isolate::GetCurrent();
            v8::HandleScope scope(isolate);

            v8::TryCatch try_catch;

            qdb_request * qdb_req = static_cast<qdb_request *>(req->data);
            assert(qdb_req);

            const auto error_code = processErrorCode(isolate, status, qdb_req);
            const auto result_data = ((status >= 0) && (qdb_req->output.error == qdb_e_ok) && (qdb_req->output.content.buffer.size > 0u)) ?
                qdb_req->make_node_buffer(isolate).ToLocalChecked() : node::Buffer::New(isolate, 0).ToLocalChecked();

            // don't free the buffer qdb_req->output.content.buffer.begin
            // we handed over ownership in make_node_buffer            

            static const unsigned int argc = 2;
            v8::Local<v8::Value> argv[argc] = { error_code, result_data };

            processCallAndCleanUp(isolate, try_catch, qdb_req, argc, argv);
        }

        static void processArrayStringResult(uv_work_t * req, int status)
        {
            // build an array out of the buffer
            v8::Isolate * isolate = v8::Isolate::GetCurrent();
            v8::HandleScope scope(isolate);

            v8::Handle<v8::Array> array;

            v8::TryCatch try_catch;

            qdb_request * qdb_req = static_cast<qdb_request *>(req->data);
            assert(qdb_req);

            auto error_code = processErrorCode(isolate, status, qdb_req);
            if ((qdb_req->output.error == qdb_e_ok) && (status >= 0))
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
                qdb_free_results(qdb_req->handle(), entries, entries_count);
            }
            else
            {
                // provide an empty array
                array = v8::Array::New(isolate, 0);
            }

            static const unsigned int argc = 2;
            v8::Local<v8::Value> argv[argc] = { error_code, array };

            processCallAndCleanUp(isolate, try_catch, qdb_req, argc, argv);
        }

        static void processIntegerResult(uv_work_t * req, int status)
        {
            v8::Isolate * isolate = v8::Isolate::GetCurrent();
            v8::HandleScope scope(isolate);

            v8::TryCatch try_catch;

            qdb_request * qdb_req = static_cast<qdb_request *>(req->data);
            assert(qdb_req);

            const auto error_code = processErrorCode(isolate, status, qdb_req);
            auto result_data = ((status >= 0) && (qdb_req->output.error == qdb_e_ok)) ?
                v8::Number::New(isolate, static_cast<double>(qdb_req->output.content.value)) : v8::Number::New(isolate, 0.0);

            static const unsigned int argc = 2;
            v8::Local<v8::Value> argv[argc] = { error_code, result_data };

            processCallAndCleanUp(isolate, try_catch, qdb_req, argc, argv);
        }

        static void processDateResult(uv_work_t * req, int status)
        {
            v8::Isolate * isolate = v8::Isolate::GetCurrent();
            v8::HandleScope scope(isolate);

            v8::TryCatch try_catch;

            qdb_request * qdb_req = static_cast<qdb_request *>(req->data);
            assert(qdb_req);

            const auto error_code = processErrorCode(isolate, status, qdb_req);
            auto result_data = ((status >= 0) && (qdb_req->output.error == qdb_e_ok)) ?
                v8::Date::New(isolate, static_cast<double>(qdb_req->output.content.value) * 1000.0) : v8::Date::New(isolate, 0.0);

            static const unsigned int argc = 2;
            v8::Local<v8::Value> argv[argc] = { error_code, result_data };

            processCallAndCleanUp(isolate, try_catch, qdb_req, argc, argv);
        }

        static void processVoidResult(uv_work_t * req, int status)
        {
            v8::Isolate * isolate = v8::Isolate::GetCurrent();
            v8::HandleScope scope(isolate);

            v8::TryCatch try_catch;

            qdb_request * qdb_req = static_cast<qdb_request *>(req->data);
            assert(qdb_req);

            static const unsigned int argc = 1;
            v8::Local<v8::Value> argv[argc] = { processErrorCode(isolate, status, qdb_req) };

            processCallAndCleanUp(isolate, try_catch, qdb_req, argc, argv);
        }

    private:
        static void callback_wrapper(uv_work_t * req)
        {
            static_cast<qdb_request *>(req->data)->execute();
        }

    public:
        template <typename Proc, typename F, typename... Params>
        static void queue_work(const v8::FunctionCallbackInfo<v8::Value> & args, F f, Proc process, Params... p)
        {
             v8::TryCatch try_catch;

             MethodMan call(args);

             uv_work_t * work = spawnRequest(call, f, p...);

             if (try_catch.HasCaught())
             {
                v8::Isolate * isolate = v8::Isolate::GetCurrent();
                v8::HandleScope scope(isolate);
                node::FatalException(isolate, try_catch);
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
        cluster_data_ptr _cluster_data;
        std::unique_ptr<std::string> _alias;

    };
}