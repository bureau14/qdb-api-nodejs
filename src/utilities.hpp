
#pragma once

#include <functional>
#include <utility>
#include <vector>

#include <node.h>
#include <node_buffer.h>

#include <qdb/client.h>
#include <qdb/integer.h>

#include "cluster_data.hpp"

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

    inline void release_node_buffer(char * data, void * hint)
    {
        if (data && hint)
        {
            qdb_free_buffer(static_cast<qdb_handle_t>(hint), data);
        }
    }

} // namespace detail

    struct qdb_request
    {
        struct slice
        {
            const void * begin;
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
                std::vector<std::string> strs;
                slice buffer;
                qdb_int_t value;
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
                qdb_int_t value;
                qdb_time_t date;
            } content;

            qdb_error_t error;
        };

        explicit qdb_request (qdb_error_t err) : _cluster_data(nullptr), output(err) {}

        qdb_request(cluster_data_ptr cd, std::function<void (qdb_request *)> exec, std::string a) : _cluster_data(cd), input(a), _execute(exec) {}

        ~qdb_request(void)
        {
            callback.Reset();
        }

    private:
        // make sure the handle is alive for the duration of the request
        cluster_data_ptr _cluster_data;

    public:
        qdb_handle_t handle(void)
        {
            return _cluster_data ? static_cast<qdb_handle_t>(_cluster_data->handle().get()) : nullptr;
        }

        void on_error(v8::Isolate * isolate, const v8::Local<v8::Object> & error_object)
        {
            if (_cluster_data)
            {
                _cluster_data->on_error(isolate, error_object);
            }
        }

    public:
        v8::MaybeLocal<v8::Object> make_node_buffer(v8::Isolate* isolate, const void * buf, size_t length)
        {
            qdb_handle_t h = handle();

            if (!h || !buf || !length)
            {
                return node::Buffer::New(isolate, static_cast<size_t>(0u));
            }

            return node::Buffer::New(isolate,
                static_cast<char *>(const_cast<void *>(buf)),
                length,
                detail::release_node_buffer,
                h);
        }

        v8::MaybeLocal<v8::Object> make_node_buffer(v8::Isolate* isolate)
        {
            return make_node_buffer(isolate, output.content.buffer.begin, output.content.buffer.size);
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
        explicit MethodMan(const v8::FunctionCallbackInfo<v8::Value> & args) : _isolate(args.GetIsolate()), _scope(_isolate), _args(args) {}

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

        v8::Local<v8::Array> argArray(int i) const
        {
            return v8::Local<v8::Array>::Cast(_args[i]);
        }

        std::pair<v8::Local<v8::Array>, bool> checkedArgArray(int i) const
        {
            return checkArg<v8::Local<v8::Array>>(i,
                [](v8::Local<v8::Value> v) -> bool { return v->IsArray(); },
                &MethodMan::argArray);
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

        std::pair<v8::Local<v8::Array>, bool> eatArray(void)
        {
            return processResult(_method.checkedArgArray(_pos));
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
        qdb_int_t eatAndConvertInteger(void)
        {
            return eatInteger<qdb_int_t>().first;
        }

        qdb_time_t eatAndConvertDate(void)
        {
            // we get a Date object a convert that to qdb_time_t
            const auto date = _method.checkedArgDate(_pos);

            if (date.second)
            {
                ++_pos;
                return static_cast<qdb_time_t>(date.first->ValueOf() / 1000.0);
            }

            // it might be a "special date"
            const auto number = _method.checkedArgNumber(_pos);
            if (!number.second)
            {
                // nope
                return static_cast<qdb_time_t>(0);
            }

            const qdb_time_t res = static_cast<qdb_time_t>(number.first);

            // only special values allowed
            if ((res != qdb_never_expires) && (res != qdb_preserve_expiration))
            {
                return static_cast<qdb_time_t>(0);
            }

            // this is a special value, increment position index
            ++_pos;

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

        std::vector<std::string> eatAndConvertStringArray(void)
        {
            using string_vector = std::vector<std::string>;
            string_vector res;

            auto arr = eatArray();

            if (arr.second)
            {
                auto len = arr.first->Length();
                res.reserve(len);

                for (auto i = 0u; i < len; ++i)
                {
                    auto vi = arr.first->Get(i);
                    if (!vi->IsString()) return string_vector();

                    v8::String::Utf8Value val(vi->ToString());
                    res.push_back(std::string(*val, val.length()));
                }
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

        qdb_request & strings(qdb_request & req)
        {
            req.input.content.strs = _eater.eatAndConvertStringArray();
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