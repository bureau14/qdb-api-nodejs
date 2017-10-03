#pragma once

#include "blob.hpp"
#include "cluster_data.hpp"
#include "deque.hpp"
#include "error.hpp"
#include "hset.hpp"
#include "integer.hpp"
#include "prefix.hpp"
#include "range.hpp"
#include "tag.hpp"
#include "time_series.hpp"
#include <node.h>
#include <node_object_wrap.h>
#include <mutex>

namespace quasardb
{

// cAmelCaSe :(
class Cluster : public node::ObjectWrap
{

public:
    // we have a structure we can ref count that holds important data
    // this makes sure we can keep things alive in asynchronous operations

public:
    explicit Cluster(const char * uri) : _uri(uri), _timeout(60000)
    {
    }

    virtual ~Cluster(void)
    {
    }

private:
    static void AddEntryType(v8::Local<v8::Object> exports, const char * name, qdb_entry_type_t type)
    {
        v8::Isolate * isolate = exports->GetIsolate();
        detail::AddConstantProperty(isolate, exports, name, v8::Int32::New(isolate, type));
    }

public:
    static void Init(v8::Local<v8::Object> exports)
    {
        v8::Isolate * isolate = exports->GetIsolate();

        // Prepare constructor template
        v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, New);
        tpl->SetClassName(v8::String::NewFromUtf8(isolate, "Cluster"));
        tpl->InstanceTemplate()->SetInternalFieldCount(4);

        // Prototype
        NODE_SET_PROTOTYPE_METHOD(tpl, "connect", connect);

        NODE_SET_PROTOTYPE_METHOD(tpl, "blob", blob);
        NODE_SET_PROTOTYPE_METHOD(tpl, "deque", deque);
        NODE_SET_PROTOTYPE_METHOD(tpl, "integer", integer);
        NODE_SET_PROTOTYPE_METHOD(tpl, "set", set);
        NODE_SET_PROTOTYPE_METHOD(tpl, "tag", tag);
        NODE_SET_PROTOTYPE_METHOD(tpl, "ts", ts);

        NODE_SET_PROTOTYPE_METHOD(tpl, "prefix", prefix);
        NODE_SET_PROTOTYPE_METHOD(tpl, "range", range);

        NODE_SET_PROTOTYPE_METHOD(tpl, "setTimeout", setTimeout);

        AddEntryType(exports, "ENTRY_UNINITIALIZED", qdb_entry_uninitialized);
        AddEntryType(exports, "ENTRY_BLOB", qdb_entry_blob);
        AddEntryType(exports, "ENTRY_INTEGER", qdb_entry_integer);
        AddEntryType(exports, "ENTRY_HSET", qdb_entry_hset);
        AddEntryType(exports, "ENTRY_TAG", qdb_entry_tag);
        AddEntryType(exports, "ENTRY_DEQUE", qdb_entry_deque);
        AddEntryType(exports, "ENTRY_STREAM", qdb_entry_stream);
        AddEntryType(exports, "ENTRY_TS", qdb_entry_ts);

        constructor.Reset(isolate, tpl->GetFunction());
        exports->Set(v8::String::NewFromUtf8(isolate, "Cluster"), tpl->GetFunction());
    }

private:
    static void New(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        if (args.IsConstructCall())
        {
            MethodMan call(args);

            if (args.Length() != 1)
            {
                call.throwException("Expected exactly one argument");
                return;
            }

            ArgsEater argsEater(call);

            auto str = argsEater.eatString();
            if (!str.second)
            {
                call.throwException("Expected a connection string as first argument");
                return;
            }

            v8::String::Utf8Value utf8str(str.first);

            // the cluster only owns the uri
            // when we will connect we will create a reference counted cluster_data
            // with a handle
            // because the cluster_data is reference counted and transmitted to every
            // callback we are sure it is kept alive for as long as needed
            Cluster * cl = new Cluster(*utf8str);

            cl->Wrap(args.This());
            args.GetReturnValue().Set(args.This());
        }
        else
        {
            NewInstance(args);
        }
    }

public:
    static void NewInstance(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        v8::Isolate * isolate = args.GetIsolate();
        // Invoked as plain function `MyObject(...)`, turn into construct call.
        const int argc = 1;
        assert(args.Length() == argc);
        v8::Local<v8::Value> argv[argc] = {args[0]};
        v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);
        v8::Local<v8::Object> instance = cons->NewInstance(isolate->GetCurrentContext(), argc, argv).ToLocalChecked();
        args.GetReturnValue().Set(instance);
    }

private:
    template <typename Object, size_t ArgsLength>
    struct ObjectConstructor;

    template <typename Object>
    struct ObjectConstructor<Object, 1>
    {
        static void constructObject(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            assert(args.IsConstructCall());

            MethodMan call(args);

            if (args.Length() != 1)
            {
                call.throwException("Wrong number of arguments");
                return;
            }

            ArgsEater argsEater(call);

            cluster_data_ptr data = eatClusterData(call, argsEater);
            if (!data)
            {
                call.throwException("Cluster is not connected");
                return;
            }

            auto b = new Object(data);
            assert(b);

            b->Wrap(args.This());
            args.GetReturnValue().Set(args.This());
        }
    };

    template <typename Object>
    struct ObjectConstructor<Object, 2>
    {
        static void constructObject(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            assert(args.IsConstructCall());

            MethodMan call(args);

            if (args.Length() != 2)
            {
                call.throwException("Wrong number of arguments");
                return;
            }

            ArgsEater argsEater(call);

            cluster_data_ptr data = eatClusterData(call, argsEater);
            if (!data) return;

            auto str = argsEater.eatString();
            if (!str.second)
            {
                call.throwException("Expected a string as an argument");
                return;
            }

            v8::String::Utf8Value utf8str(str.first);
            auto b = new Object(data, *utf8str);
            assert(b);

            b->Wrap(args.This());
            args.GetReturnValue().Set(args.This());
        }
    };

private:
    static cluster_data_ptr eatClusterData(MethodMan & call, ArgsEater & argsEater)
    {
        // the first parameter is a Cluster object, let's unwrap it to get access to the underlying handle
        auto obj = argsEater.eatObject();
        if (!obj.second)
        {
            call.throwException("Invalid parameter supplied to object");
            return nullptr;
        }

        cluster_data_ptr data = getClusterData(obj.first);
        if (!data)
        {
            call.throwException("Cluster is not connected");
            return nullptr;
        }

        return data;
    }

    static cluster_data_ptr getClusterData(v8::Local<v8::Object> object)
    {
        // get access to the underlying handle
        Cluster * c = ObjectWrap::Unwrap<Cluster>(object);
        assert(c);

        cluster_data_ptr data = c->data();
        if (!data)
        {
            return nullptr;
        }

        return data;
    }

private:
    template <size_t ArgsLength>
    struct ArgumentsCopier;

public:
    template <typename Object>
    static void newObject(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        static const size_t ArgsLength = Object::ParameterCount + 1;

        if (args.IsConstructCall())
        {
            ObjectConstructor<Object, ArgsLength>::constructObject(args);
        }
        else
        {
            v8::Isolate * isolate = v8::Isolate::GetCurrent();
            // Invoked as plain function `MyObject(...)`, turn into construct call.
            assert(args.Length() == ArgsLength);
            auto argv = ArgumentsCopier<ArgsLength>::copy(args);
            v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);
            args.GetReturnValue().Set(
                cons->NewInstance(isolate->GetCurrentContext(), ArgsLength, argv.data()).ToLocalChecked());
        }
    }

private:
    template <typename Object, size_t ParameterCount = Object::ParameterCount>
    struct Factory
    {
        static void create(const v8::FunctionCallbackInfo<v8::Value> & args);
    };

    template <typename Object>
    static void objectFactory(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Factory<Object>::create(args);
    }

private:
    struct connection_request
    {
        explicit connection_request(cluster_data_ptr cd) : data(cd)
        {
            assert(data);
        }

        cluster_data_ptr data;
        qdb_error_t error;

        void execute(void)
        {
            error = data->connect();
        }
    };

private:
    static void processConnectionResult(uv_work_t * req, int status)
    {
        v8::Isolate * isolate = v8::Isolate::GetCurrent();
        v8::HandleScope scope(isolate);

        v8::TryCatch try_catch(isolate);

        connection_request * conn_req = static_cast<connection_request *>(req->data);
        assert(conn_req);

        cluster_data_ptr cd = conn_req->data;
        assert(cd);

        const qdb_error_t err = (status < 0) ? qdb_e_internal_local : conn_req->error;

        // no longer needs the request object
        delete conn_req;

        // if we got any kind of error call "on error", otherwise call "on success"
        if (err == qdb_e_ok)
        {
            cd->on_success(isolate);
        }
        else
        {
            v8::Local<v8::Object> error_object = Error::MakeError(isolate, err);
            cd->on_error(isolate, error_object);
        }

        if (try_catch.HasCaught())
        {
            node::FatalException(isolate, try_catch);
        }
    }

private:
    static void callback_wrapper(uv_work_t * req)
    {
        static_cast<connection_request *>(req->data)->execute();
    }

public:
    static void connect(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        MethodMan call(args);

        if (args.Length() != 2)
        {
            call.throwException("Wrong number of arguments");
            return;
        }

        ArgsEater argsEater(call);

        auto on_success = argsEater.eatCallback();
        if (!on_success.second)
        {
            call.throwException("Expected a callback as first argument");
            return;
        }

        auto on_error = argsEater.eatCallback();
        if (!on_error.second)
        {
            call.throwException("Expected a callback as second argument");
            return;
        }

        Cluster * c = call.nativeHolder<Cluster>();
        assert(c);

        uv_work_t * work = new uv_work_t();
        work->data = new connection_request(c->new_data(on_success.first, on_error.first));

        uv_queue_work(uv_default_loop(), work, &Cluster::callback_wrapper, &Cluster::processConnectionResult);
    }

public:
    static void blob(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        objectFactory<Blob>(args);
    }

    static void deque(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        objectFactory<Deque>(args);
    }

    static void integer(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        objectFactory<Integer>(args);
    }

    static void prefix(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        objectFactory<Prefix>(args);
    }

    static void range(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        objectFactory<Range>(args);
    }

    static void set(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        objectFactory<Set>(args);
    }

    static void tag(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        objectFactory<Tag>(args);
    }

    static void ts(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        objectFactory<TimeSeries>(args);
    }

public:
    static void setTimeout(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        MethodMan call(args);

        if (args.Length() != 1)
        {
            call.throwException("Wrong number of arguments");
            return;
        }

        ArgsEater argsEater(call);

        auto timeout_value = argsEater.eatNumber();
        if (!timeout_value.second)
        {
            call.throwException("setTimeout expects a timeout value in seconds");
            return;
        }

        const int new_timeout = static_cast<int>(timeout_value.first);
        if (new_timeout < 1000)
        {
            call.throwException("The timeout value cannot be less than one second.");
            return;
        }

        Cluster * c = call.nativeHolder<Cluster>();
        assert(c);

        c->_timeout = new_timeout;

        cluster_data_ptr cd = c->data();
        if (cd)
        {
            if (cd->set_timeout(c->_timeout) != qdb_e_ok)
            {
                call.throwException("Could not set timeout.");
            }
        }
    }

public:
    const std::string & uri(void) const
    {
        return _uri;
    }

public:
    cluster_data_ptr data(void)
    {
        cluster_data_ptr res;

        {
            std::unique_lock<std::mutex> lock(_data_mutex);
            res = _data;
        }

        return res;
    }

private:
    cluster_data_ptr new_data(v8::Local<v8::Function> on_success, v8::Local<v8::Function> on_error)
    {
        cluster_data_ptr res;

        {
            std::unique_lock<std::mutex> lock(_data_mutex);
            res = _data = std::make_shared<cluster_data>(_uri, _timeout, on_success, on_error);
        }

        return res;
    }

private:
    const std::string _uri;

    mutable std::mutex _data_mutex;

    int _timeout;
    cluster_data_ptr _data;

    static v8::Persistent<v8::Function> constructor;
};

template <>
struct Cluster::ArgumentsCopier<1>
{
    static std::array<v8::Local<v8::Value>, 1> copy(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        return {{args[0]}};
    }
};

template <>
struct Cluster::ArgumentsCopier<2>
{
    static std::array<v8::Local<v8::Value>, 2> copy(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        return {{args[0], args[1]}};
    }
};

template <typename Object>
struct Cluster::Factory<Object, 0>
{
    static void create(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        v8::Isolate * isolate = v8::Isolate::GetCurrent();
        v8::HandleScope scope(isolate);

        v8::Local<v8::Object> the_cluster = args.Holder();

        static const size_t argc = 1;
        v8::Local<v8::Value> argv[argc] = {the_cluster};
        v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, Object::constructor);
        assert(!cons.IsEmpty() && "Verify that Object::Init has been called in qdb_api.cpp:InitAll()");
        args.GetReturnValue().Set(cons->NewInstance(isolate->GetCurrentContext(), argc, argv).ToLocalChecked());
    }
};

template <typename Object>
struct Cluster::Factory<Object, 1>
{
    static void create(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        v8::Isolate * isolate = v8::Isolate::GetCurrent();
        v8::HandleScope scope(isolate);

        v8::Local<v8::Object> the_cluster = args.Holder();

        static const size_t argc = 2;
        assert(args.Length() == 1);
        v8::Local<v8::Value> argv[argc] = {the_cluster, args[0]};
        v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, Object::constructor);
        assert(!cons.IsEmpty() && "Verify that Object::Init has been called in qdb_api.cpp:InitAll()");
        args.GetReturnValue().Set(cons->NewInstance(isolate->GetCurrentContext(), argc, argv).ToLocalChecked());
    }
};

} // namespace quasardb
