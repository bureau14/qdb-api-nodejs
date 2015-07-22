
#pragma once

#include <functional>
#include <utility>

#include <node.h>

#include <qdb/client.h>
#include <qdb/integer.h>

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
     //   return std::string(qdb_error(error));
        return std::string();
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
            query(std::string a = "") : alias(a), expiry(0) {}
            
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
                qdb_time_t date;
            } content;

            qdb_error_t error;      
        };

        explicit qdb_request (qdb_error_t err) : _handle(nullptr), output(err) {}

        qdb_request(std::shared_ptr<qdb_handle_t> h, std::function<void (qdb_request *)> exec, std::string a) : _handle(h), input(a), _execute(exec) {}

    private:
        // make sure the handle is alive for the duration of the request
        std::shared_ptr<qdb_handle_t> _handle;

    public:
        qdb_handle_t handle(void) const
        {
            return _handle ? *_handle : static_cast<qdb_handle_t>(0);
        }

    public:
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
    
    private:
        std::function<void (qdb_request *)> _execute;

        // prevent copy
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

            return std::make_pair((this->*acc)(i), true);
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
                &MethodMan::argObject);
        }

        v8::Local<v8::Function> argCallback(int i) const
        {
            return v8::Local<v8::Function>::Cast(_args[i]);
        }

        std::pair<v8::Local<v8::Function>, bool> checkedArgCallback(int i) const
        {
            return checkArg<v8::Local<v8::Function>>(i, 
                [](v8::Local<v8::Value> v) -> bool { return v->IsFunction(); }, 
                &MethodMan::argCallback);
        }

        v8::Local<v8::String> argString(int i) const
        {
            return _args[i]->ToString();
        }
     
        std::pair<v8::Local<v8::String>, bool> checkedArgString(int i) const
        {
            return checkArg<v8::Local<v8::String>>(i, 
                [](v8::Local<v8::Value> v) -> bool { return v->IsString(); }, 
                &MethodMan::argString);
        }

        double argNumber(int i) const
        {
            return _args[i]->NumberValue();
        }
     
        std::pair<double, bool> checkedArgNumber(int i) const
        {
            return checkArg<double>(i, 
                [](v8::Local<v8::Value> v) -> bool { return v->IsNumber(); }, 
                &MethodMan::argNumber);
        }

        v8::Local<v8::Date> argDate(int i) const
        {
            return v8::Local<v8::Date>::Cast(_args[i]);
        }

        std::pair<v8::Local<v8::Date>, bool> checkedArgDate(int i) const
        {            
            return checkArg<v8::Local<v8::Date>>(i, 
                [](v8::Local<v8::Value> v) -> bool { return v->IsDate(); }, 
                &MethodMan::argDate);
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
        explicit ArgsEater(const MethodMan & meth) : _method(meth), _pos(0) {}

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

        std::pair<v8::Local<v8::Date>, bool> eatDate(void)
        {
            return processResult(_method.checkedArgDate(_pos));  
        }

    public:
        qdb_int eatAndConvertInteger(void)
        {
            return eatInteger<qdb_int>().first;
        }

        qdb_time_t eatAndConvertDate(void)
        {
            // we get a Date object a convert that to qdb_time_t
            auto date = eatDate();

            qdb_time_t res = 0;

            if (date.second)
            {
                res = static_cast<qdb_time_t>(date.first->ValueOf() / 1000.0);               
            }

            return res;
        }

        std::string eatAndConvertString(void)
        {
            std::string res;

            auto str = eatString();

            if (str.second)
            {
                v8::String::Utf8Value val(str.first);
                res = std::string(*val, val.length());
            }

            return res;
        }

        qdb_request::slice eatAndConvertBuffer(void)
        {
            auto buf = eatObject();

            qdb_request::slice res;

            if (buf.second)
            {
                res.begin = node::Buffer::Data(buf.first);
                res.size = node::Buffer::Length(buf.first);   
            }
            else
            {
                res.begin = nullptr;
                res.size = 0;
            }

            return res;
        }

    private:
        const MethodMan & _method;
        int _pos;

    };


    class ArgsEaterBinder
    {
    public:
        explicit ArgsEaterBinder(const MethodMan & meth) : _eater(meth) {}

    public:
        qdb_request & none(qdb_request & req)
        {
            return req;
        }

        qdb_request & integer(qdb_request & req)
        {
            req.input.content.value = _eater.eatAndConvertInteger();
            return req;
        }

        qdb_request & string(qdb_request & req)
        {
            req.input.content.str = _eater.eatAndConvertString();
            return req;
        }

        qdb_request & buffer(qdb_request & req)
        {
            req.input.content.buffer = _eater.eatAndConvertBuffer();
            return req;
        }

        qdb_request & expiry(qdb_request & req)
        {
            req.input.expiry = _eater.eatAndConvertDate();
            return req;
        }

    public:
        bool bindCallback(qdb_request & req)
        {
            auto callback = _eater.eatCallback();

            if (callback.second)
            {
                req.callback.Reset(v8::Isolate::GetCurrent(), callback.first);
            }

            return callback.second;
        }

    public:
        qdb_request & eatThem(qdb_request & req)
        {
            return req;
        }

        template <typename Last>
        qdb_request & eatThem(qdb_request & req, Last last)
        {
            return (this->*last)(req);
        }

        template <typename First, typename... Tail>
        qdb_request & eatThem(qdb_request & req, First f, Tail... tail)
        {
            return eatThem((this->*f)(req), tail...);
        }

    private:
        ArgsEater _eater;
    };

}