
#pragma once

#include <functional>

#include <node.h>

#include <qdb/client.h>

namespace qdb
{

namespace detail
{

    template <typename T>
    struct NewObject
    {
        template <typename P>
        v8::Local<T> operator()(v8::Isolate * i, P && p)
        {
            return T::New(i, std::forward<P>(p));
        }
    };

    template <>
    struct NewObject<v8::String>
    {
    
        template <typename P>
        v8::Local<v8::String> operator()(v8::Isolate * i, P && p)
        {
            // strings don't use new, they use newfromutf8
            return v8::String::NewFromUtf8(i, std::forward<P>(p));
        }
    
    };
}

    // client.hpp has some try/catch which prevents inclusion as exception are disabled in node.js
    // we however like the make_error_string convenience function very much and therefore implement it here again
    inline std::string make_error_string(qdb_error_t error)
    {
        static const size_t buf_size = 1024;
        char buf[buf_size];
        // qdb_error never fails and always appends a terminating null
        // the only exception is when providing an empty buffer, which is not the case here
        return std::string(qdb_error(error, buf, buf_size));
    }

    struct qdb_request
    {

        struct slice
        {
            const char * begin;
            size_t size;
        };

        struct query
        {
            query(std::string a = "", qdb_time_t e = 0) : alias(a), expiry(e) {}
            
            std::string alias;

            union 
            {
                slice buffer;
                qdb_int value;
            } content;
            
            qdb_time_t expiry;    
        };

        struct result
        {
            result(qdb_error_t err = qdb_e_uninitialized) : error(err) {}

            union 
            {
                slice buffer;
                qdb_int value;
            } content;

            qdb_error_t error;      
        };

        explicit qdb_request (qdb_error_t err) : handle(nullptr), output(err) {}

        qdb_request(qdb_handle_t h, std::function<void (qdb_request *)> exec, v8::Handle<v8::Function> cb, std::string a, qdb_time_t e = 0) : handle(h), callback(v8::Isolate::GetCurrent(), cb), input(a, e), _execute(exec) {}

        qdb_handle_t handle;



        v8::Persistent<v8::Function> callback;

        v8::Local<v8::Function> callbackAsLocal(void)
        {
            if (callback.IsWeak()) 
            {
                return v8::Local<v8::Function>::New(v8::Isolate::GetCurrent(), callback);
            }
        
            return *reinterpret_cast<v8::Local<v8::Function>*>(const_cast<v8::Persistent<v8::Function>*>(&callback));
        }

        void execute(void)
        {
            if ((output.error == qdb_e_uninitialized) && (_execute))
            {
                _execute(this);
            }
        }

        query input;
        result output;

    // prevent copy
    private:
        std::function<void (qdb_request *)> _execute;

        qdb_request(const qdb_request &) {}

    };

    template <typename Object>
    struct MethodMan
    {
        explicit MethodMan(const v8::FunctionCallbackInfo<v8::Value> & args) : _isolate(v8::Isolate::GetCurrent()), _scope(_isolate), _args(args) {}

    private:
        // prevent copy
        MethodMan(const MethodMan &) {}

    public:
        v8::Local<v8::Object> holder(void) const { return _args.Holder(); }

        Object * nativeHolder(void) const { return node::ObjectWrap::Unwrap<Object>(holder()); }

        template <typename T, typename P>
        void setReturnValue(P && p)
        {
            _args.GetReturnValue().Set(typename detail::NewObject<T>()(_isolate, std::forward<P>(p)));
        }

        void setUndefinedReturnValue(void)
        {
            _args.GetReturnValue().SetUndefined();
        }

        v8::Local<v8::Function> argCallback(int i) const
        {
            return v8::Local<v8::Function>::Cast(_args[i]);
        }

        v8::Local<v8::String> argString(int i) const
        {
            return _args[i]->ToString();
        }
        
        const v8::FunctionCallbackInfo<v8::Value> & args(void) const
        {
            return _args;
        }

        bool checkArgc(int expected) const
        {
            if (argc() < expected)
            {
                _isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(_isolate, "Wrong number of arguments")));
                return false;
            }
            
            return true;
        }

        int argc(void) const
        {
            return _args.Length();
        }

    private:
        v8::Isolate * _isolate;
        v8::HandleScope _scope;

        const v8::FunctionCallbackInfo<v8::Value> & _args;
    };

}