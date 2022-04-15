#pragma once

#include "blob.hpp"
#include "cluster_data.hpp"
#include "error.hpp"
#include "integer.hpp"
#include "prefix.hpp"
#include "query.hpp"
#include "query_find.hpp"
#include "range.hpp"
#include "suffix.hpp"
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

    explicit Cluster(
        const char * uri, const char * cluster_public_key_file = "", const char * user_private_key_file = "")
        : _uri{uri}
        , _user_private_key_file{user_private_key_file}
        , _cluster_public_key_file{cluster_public_key_file}
        , _timeout{60000}
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
        auto maybe_classname = v8::String::NewFromUtf8(isolate, "Cluster", v8::NewStringType::kNormal);
        if (maybe_classname.IsEmpty()) return;

        tpl->SetClassName(maybe_classname.ToLocalChecked());
        tpl->InstanceTemplate()->SetInternalFieldCount(4);

        // Prototype
        NODE_SET_PROTOTYPE_METHOD(tpl, "connect", connect);

        NODE_SET_PROTOTYPE_METHOD(tpl, "blob", blob);
        NODE_SET_PROTOTYPE_METHOD(tpl, "integer", integer);
        NODE_SET_PROTOTYPE_METHOD(tpl, "tag", tag);
        NODE_SET_PROTOTYPE_METHOD(tpl, "ts", ts);
        NODE_SET_PROTOTYPE_METHOD(tpl, "prefix", prefix);
        NODE_SET_PROTOTYPE_METHOD(tpl, "queryFind", queryFind);
        NODE_SET_PROTOTYPE_METHOD(tpl, "query", query);
        NODE_SET_PROTOTYPE_METHOD(tpl, "range", range);

        NODE_SET_PROTOTYPE_METHOD(tpl, "getTimeout", getTimeout);
        NODE_SET_PROTOTYPE_METHOD(tpl, "setTimeout", setTimeout);
        NODE_SET_PROTOTYPE_METHOD(tpl, "suffix", suffix);

        AddEntryType(exports, "ENTRY_UNINITIALIZED", qdb_entry_uninitialized);
        AddEntryType(exports, "ENTRY_BLOB", qdb_entry_blob);
        AddEntryType(exports, "ENTRY_INTEGER", qdb_entry_integer);
        AddEntryType(exports, "ENTRY_TAG", qdb_entry_tag);
        AddEntryType(exports, "ENTRY_DEQUE", qdb_entry_deque);
        AddEntryType(exports, "ENTRY_STREAM", qdb_entry_stream);
        AddEntryType(exports, "ENTRY_TS", qdb_entry_ts);

        auto maybe_function = tpl->GetFunction(isolate->GetCurrentContext());
        if (maybe_function.IsEmpty()) return;

        constructor.Reset(isolate, maybe_function.ToLocalChecked());
        exports->Set(isolate->GetCurrentContext(),
            v8::String::NewFromUtf8(isolate, "Cluster", v8::NewStringType::kNormal).ToLocalChecked(),
            maybe_function.ToLocalChecked());
    }

private:
    static void New(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        if (args.IsConstructCall())
        {
            MethodMan call(args);

            if (args.Length() != 1 && args.Length() != 3)
            {
                call.throwException("Expected either 1 or 3 argument(s)");
                return;
            }

            ArgsEater argsEater(call);

            auto uri = argsEater.eatString();
            if (!uri.second)
            {
                call.throwException("Expected a connection string as first argument");
                return;
            }

            v8::String::Utf8Value uri_utf8(args.GetIsolate(), uri.first);

            if (args.Length() == 3)
            {
                auto cluster_public_key_file = argsEater.eatString();
                if (!cluster_public_key_file.second)
                {
                    call.throwException("Expected a cluster public key filepath string as second argument");
                    return;
                }
                auto user_credentials_file = argsEater.eatString();
                if (!cluster_public_key_file.second)
                {
                    call.throwException("Expected a user credentials filepath string as third argument");
                }

                v8::String::Utf8Value cluster_public_key_file_utf8{args.GetIsolate(), cluster_public_key_file.first};
                v8::String::Utf8Value user_credentials_file_utf8{args.GetIsolate(), user_credentials_file.first};

                Cluster * cl = new Cluster(*uri_utf8, *cluster_public_key_file_utf8, *user_credentials_file_utf8);

                cl->Wrap(args.This());
                args.GetReturnValue().Set(args.This());
            }
            else
            {
                // the cluster only owns the uri
                // when we will connect we will create a reference counted cluster_data
                // with a handle
                // because the cluster_data is reference counted and transmitted to every
                // callback we are sure it is kept alive for as long as needed
                Cluster * cl = new Cluster(*uri_utf8);

                cl->Wrap(args.This());
                args.GetReturnValue().Set(args.This());
            }
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

            v8::String::Utf8Value utf8str(args.GetIsolate(), str.first);
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
        explicit connection_request(cluster_data_ptr cd)
            : data(cd)
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
    // :desc: Connects to a quasardb cluster. The successful function is run when the connection is made. The
    // failure callback is called for major errors such as disconnections from the cluster after the connection is
    // successful.
    // :args: callback() (function) - A callback or anonymous function without parameters.
    // callback_on_failure(err) (function) - A callback or anonymous function with error parameter.

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
    // :desc: Creates a Blob associated with the specified alias. No query is performed at this point.
    // :args: alias (String) - the alias of the blob in the database.
    // :returns: the Blob
    static void blob(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        objectFactory<Blob>(args);
    }

    // :desc: Creates an Integer associated with the specified alias. No query is performed at this point.
    // :args: alias (String) - the alias of the integer in the database.
    // :returns: the Integer

    static void integer(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        objectFactory<Integer>(args);
    }

    // :desc: Retrieves aliases with the input prefix argument
    // :args: prefix (String) - The prefix to search for.
    // :returns: the Prefix

    static void prefix(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        objectFactory<Prefix>(args);
    }

    // :desc: Retrieves list of aliases that matches the query condition
    // :args: Query (String) - the query string
    // :returns: List of matched aliases.
    static void queryFind(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        objectFactory<QueryFind>(args);
    }

    // :desc: Perform an sql-like query
    // :args: Query (String) - the query string
    // :returns: query result structure.
    static void query(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        objectFactory<Query>(args);
    }

    // :desc: Returns a range object which supports BlobScan, BlobScanRegex
    // :returns: The Range object

    static void range(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        objectFactory<Range>(args);
    }

    // :desc: Retrieves aliases with the input suffix argument
    // :args: suffix (String) - The prefix to search for.
    // :returns: the Suffix

    static void suffix(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        objectFactory<Suffix>(args);
    }

    // :desc: Creates a Tag with the specified name.
    // :args: tagName (String) - the name of the tag in the database.
    // :returns: the Tag

    static void tag(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        objectFactory<Tag>(args);
    }

    // :desc: Creates a timeseries object with the given alias
    // :args: alias (String) - the name of the timeseries
    // :returns: the ts object

    static void ts(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        objectFactory<TimeSeries>(args);
    }

public:
    // :desc: Returns the current set timeout in milliseconds
    // :returns: Current set timeout in milliseconds

    static void getTimeout(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        MethodMan call(args);

        if (args.Length() != 0)
        {
            call.throwException("Wrong number of arguments");
            return;
        }

        Cluster * c = call.nativeHolder<Cluster>();
        assert(c);

        call.template setReturnValue<v8::Integer>(c->_timeout);
    }

    // :desc: Set the current timeout in milliseconds
    // :args: timeout (Integer) - this timeout is in milliseconds

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
            res = _data = std::make_shared<cluster_data>(
                _uri, _user_private_key_file, _cluster_public_key_file, _timeout, on_success, on_error);
        }

        return res;
    }

private:
    const std::string _uri;
    const std::string _user_private_key_file;
    const std::string _cluster_public_key_file;

    mutable std::mutex _data_mutex;

    int _timeout;
    cluster_data_ptr _data;

    static v8::Persistent<v8::Function> constructor;
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
