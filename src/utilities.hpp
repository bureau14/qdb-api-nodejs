
#pragma once

#include <functional>
#include <utility>

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
        // qdb_error never fails and always appends a terminating null
        // the only exception is when providing an empty buffer, which is not the case here
        return std::string(qdb_error(error));
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

            struct query_content
            {
                query_content(void) : value(0)
                {
                    buffer.begin = nullptr;
                    buffer.size = 0;
                }

                std::string str;
                slice buffer;
                qdb_int value;
            };

            query_content content;
            
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

        qdb_request(qdb_handle_t h, std::function<void (qdb_request *)> exec, const v8::Handle<v8::Function> & cb, std::string a, qdb_time_t e = 0) : handle(h), callback(v8::Isolate::GetCurrent(), cb), input(a, e), _execute(exec) {}

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
    
    struct MethodMan
    {
        explicit MethodMan(const v8::FunctionCallbackInfo<v8::Value> & args) : _isolate(v8::Isolate::GetCurrent()), _scope(_isolate), _args(args) {}

    public:
        v8::Local<v8::Object> holder(void) const { return _args.Holder(); }

        template <typename Object>
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

    private:
        template <typename Checker>
        bool checkArg(int i, Checker checker) const
        {
            if (argc() < i)
            {
                return false;
            }

            return checker(_args[i]);
        }

        template <typename T, typename Checker, typename Accessor>
        std::pair<T, bool> checkArg(int i, Checker checker, Accessor acc) const
        {
            if (!checkArg(i, checker))
            {
                return std::make_pair(T(), false);
            }

            return std::make_pair(acc(i), true);            
        }

    public:
        v8::Local<v8::Object> argObject(int i) const
        {
            return _args[i]->ToObject();
        }

        // std::optional not available in VS 2013
        std::pair<v8::Local<v8::Object>, bool> checkedArgObject(int i) const
        {
            return checkArg<v8::Local<v8::Object>>(i, 
                [](v8::Local<v8::Value> v) -> bool { return !v->IsFunction() && v->IsObject(); }, 
                std::bind(&MethodMan::argObject, this, std::placeholders::_1));
        }

        v8::Local<v8::Function> argCallback(int i) const
        {
            return v8::Local<v8::Function>::Cast(_args[i]);
        }

        std::pair<v8::Local<v8::Function>, bool> checkedArgCallback(int i) const
        {
            return checkArg<v8::Local<v8::Function>>(i, 
                [](v8::Local<v8::Value> v) -> bool { return v->IsFunction(); }, 
                std::bind(&MethodMan::argCallback, this, std::placeholders::_1));
        }

        v8::Local<v8::String> argString(int i) const
        {
            return _args[i]->ToString();
        }
     
        std::pair<v8::Local<v8::String>, bool> checkedArgString(int i) const
        {
            return checkArg<v8::Local<v8::String>>(i, 
                [](v8::Local<v8::Value> v) -> bool { return v->IsString(); }, 
                std::bind(&MethodMan::argString, this, std::placeholders::_1));
        }

        double argNumber(int i) const
        {
            return _args[i]->NumberValue();
        }
     
        std::pair<double, bool> checkedArgNumber(int i) const
        {
            return checkArg<double>(i, 
                [](v8::Local<v8::Value> v) -> bool { return v->IsNumber(); }, 
                std::bind(&MethodMan::argNumber, this, std::placeholders::_1));
        }

        const v8::FunctionCallbackInfo<v8::Value> & args(void) const
        {
            return _args;
        }

        void throwException(const char * message) const
        {
            _isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(_isolate,message)));
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


    struct ArgsEater
    {
        ArgsEater(const MethodMan & meth) : _method(meth), _pos(0) {}

    private:
        template <typename Pair>
        const Pair processResult(Pair && pair)
        {
            _pos += static_cast<int>(pair.second); // increment if successful            
            return std::forward<Pair>(pair);
        }

    public:
        std::pair<double, bool> eatNumber(void) 
        {
            return processResult(_method.checkedArgNumber(_pos));
        }

        template <typename Integer>
        std::pair<Integer, bool> eatInteger(void) 
        {
            auto res = eatNumber();
            if (!res.second)
            {
                // make sure it is 0
                return std::make_pair(static_cast<Integer>(0), false);
            }

            return std::make_pair(static_cast<Integer>(res.first), true);
        }

        std::pair<v8::Local<v8::String>, bool> eatString(void) 
        {
            return processResult(_method.checkedArgString(_pos));            
        }

        std::pair<v8::Local<v8::Function>, bool> eatCallback(void) 
        {
            return processResult(_method.checkedArgCallback(_pos));  
        }

        std::pair<v8::Local<v8::Object>, bool> eatObject(void) 
        {
            return processResult(_method.checkedArgObject(_pos));  
        }

    private:
        const MethodMan & _method;
        int _pos;

    };

}