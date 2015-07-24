
#pragma once

#include <string>
#include <memory>

#include <node.h>
#include <node_object_wrap.h>

#include <qdb/client.h>

namespace qdb
{

    using qdb_handle_ptr = std::shared_ptr<void>;

    struct cluster_data 
    {
        
    private:
        void bindCallbacks(v8::Local<v8::Function> os, v8::Local<v8::Function> oe)
        {
            v8::Isolate* isolate = v8::Isolate::GetCurrent();
            _on_success.Reset(isolate, os);
            _on_error.Reset(isolate, oe);            
        }

    public:
        cluster_data(std::string uri, v8::Local<v8::Function> os, v8::Local<v8::Function> oe) : _uri(uri) 
        {
            bindCallbacks(os, oe);
        }

    private:
        template <typename Callback>
        v8::Local<v8::Function> callbackAsLocal(v8::Isolate * isolate, Callback & callback)
        {
            if (callback.IsWeak()) 
            {
                return v8::Local<v8::Function>::New(isolate, callback);
            }

            return *reinterpret_cast<v8::Local<v8::Function>*>(const_cast<v8::Persistent<v8::Function>*>(&callback));
        }

    public:
        void on_success(v8::Isolate * isolate)
        {
            v8::Local<v8::Function> c = callbackAsLocal(isolate, _on_success);

            c->Call(isolate->GetCurrentContext()->Global(), 0, nullptr);
        }

        // can also be called by other objects
        void on_error(v8::Isolate * isolate, qdb_error_t err)
        {
            static const unsigned int argc = 1;
            v8::Handle<v8::Value> argv[argc] = { v8::Int32::New(isolate, err) };             

            v8::Local<v8::Function> c = callbackAsLocal(isolate, _on_error);

            c->Call(isolate->GetCurrentContext()->Global(), argc, argv);
        }

    private:
        qdb_handle_ptr make_shared_qdb_handle(void)
        {
            auto custom_deleter = [](qdb_handle_t h)
            {
                qdb_close(h);
            };

            return qdb_handle_ptr(qdb_open_tcp(), custom_deleter);
        }

    public:
        const std::string & uri(void) const
        {
            return _uri;
        }

        qdb_error_t connect()
        {
            // create new handle
            _handle = make_shared_qdb_handle();
            if (!_handle)
            {
                return qdb_e_no_memory;
            }

            return qdb_connect(_handle.get(), _uri.c_str());
        }

        qdb_handle_ptr handle(void)
        {
            return _handle;
        }

    private:
        std::string _uri;
        v8::Persistent<v8::Function> _on_success;
        v8::Persistent<v8::Function> _on_error;

        qdb_handle_ptr _handle;

    private:
        // prevent copy
        cluster_data(const cluster_data &) {}
    };

    using cluster_data_ptr = std::shared_ptr<cluster_data>;

}