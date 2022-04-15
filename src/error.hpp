#pragma once

#include "utilities.hpp"
#include <qdb/client.h>

namespace quasardb
{

class Error : public node::ObjectWrap
{
public:
    explicit Error(qdb_error_t e)
        : _error(e)
    {
    }

    virtual ~Error(void)
    {
    }

private:
    static void AddErrorOrigin(v8::Local<v8::Object> exports, const char * name, qdb_error_origin_t origin)
    {
        v8::Isolate * isolate = exports->GetIsolate();
        detail::AddConstantProperty(isolate, exports, name, v8::Int32::New(isolate, origin));
    }

    static void AddErrorSeverity(v8::Local<v8::Object> exports, const char * name, qdb_error_severity_t severity)
    {
        v8::Isolate * isolate = exports->GetIsolate();
        detail::AddConstantProperty(isolate, exports, name, v8::Int32::New(isolate, severity));
    }

    static void AddErrorCode(v8::Local<v8::Object> exports, const char * name, qdb_error_t err)
    {
        static const int code_mask = 0xFFFF;

        v8::Isolate * isolate = exports->GetIsolate();
        detail::AddConstantProperty(isolate, exports, name, v8::Int32::New(isolate, err & code_mask));
    }

public:
    static void Init(v8::Local<v8::Object> exports)
    {
        v8::Isolate * isolate = exports->GetIsolate();

        // Prepare constructor template
        v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, New);
        tpl->SetClassName(v8::String::NewFromUtf8(isolate, "Error", v8::NewStringType::kNormal).ToLocalChecked());
        tpl->InstanceTemplate()->SetInternalFieldCount(1);

        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Signature> s = v8::Signature::New(isolate, tpl);

        auto proto = tpl->PrototypeTemplate();

        proto->SetAccessorProperty(
            v8::String::NewFromUtf8(isolate, "informational", v8::NewStringType::kNormal).ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, Error::informational, v8::Local<v8::Value>(), s),
            v8::Local<v8::FunctionTemplate>(), v8::ReadOnly);
        proto->SetAccessorProperty(
            v8::String::NewFromUtf8(isolate, "origin", v8::NewStringType::kNormal).ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, Error::origin, v8::Local<v8::Value>(), s),
            v8::Local<v8::FunctionTemplate>(), v8::ReadOnly);
        proto->SetAccessorProperty(
            v8::String::NewFromUtf8(isolate, "severity", v8::NewStringType::kNormal).ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, Error::severity, v8::Local<v8::Value>(), s),
            v8::Local<v8::FunctionTemplate>(), v8::ReadOnly);
        proto->SetAccessorProperty(
            v8::String::NewFromUtf8(isolate, "message", v8::NewStringType::kNormal).ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, Error::message, v8::Local<v8::Value>(), s),
            v8::Local<v8::FunctionTemplate>(), v8::ReadOnly);
        proto->SetAccessorProperty(
            v8::String::NewFromUtf8(isolate, "code", v8::NewStringType::kNormal).ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, Error::code, v8::Local<v8::Value>(), s),
            v8::Local<v8::FunctionTemplate>(), v8::ReadOnly);

        AddErrorOrigin(exports, "E_ORIGIN_SYSTEM_REMOTE", qdb_e_origin_system_remote);
        AddErrorOrigin(exports, "E_ORIGIN_SYSTEM_LOCAL", qdb_e_origin_system_local);
        AddErrorOrigin(exports, "E_ORIGIN_CONNECTION", qdb_e_origin_connection);
        AddErrorOrigin(exports, "E_ORIGIN_INPUT", qdb_e_origin_input);
        AddErrorOrigin(exports, "E_ORIGIN_OPERATION", qdb_e_origin_operation);
        AddErrorOrigin(exports, "E_ORIGIN_PROTOCOL", qdb_e_origin_protocol);

        AddErrorSeverity(exports, "E_SEVERITY_UNRECOVERABLE", qdb_e_severity_unrecoverable);
        AddErrorSeverity(exports, "E_SEVERITY_ERROR", qdb_e_severity_error);
        AddErrorSeverity(exports, "E_SEVERITY_WARNING", qdb_e_severity_warning);
        AddErrorSeverity(exports, "E_SEVERITY_INFO", qdb_e_severity_info);

        AddErrorCode(exports, "E_UNINITIALIZED", qdb_e_uninitialized);
        AddErrorCode(exports, "E_ALIAS_NOT_FOUND", qdb_e_alias_not_found);
        AddErrorCode(exports, "E_ALIAS_ALREADY_EXISTS", qdb_e_alias_already_exists);
        AddErrorCode(exports, "E_OUT_OF_BOUNDS", qdb_e_out_of_bounds);
        AddErrorCode(exports, "E_SKIPPED", qdb_e_skipped);
        AddErrorCode(exports, "E_INCOMPATIBLE_TYPE", qdb_e_incompatible_type);
        AddErrorCode(exports, "E_CONTAINER_EMPTY", qdb_e_container_empty);
        AddErrorCode(exports, "E_CONTAINER_FULL", qdb_e_container_full);
        AddErrorCode(exports, "E_ELEMENT_NOT_FOUND", qdb_e_element_not_found);
        AddErrorCode(exports, "E_ELEMENT_ALREADY_EXISTS", qdb_e_element_already_exists);
        AddErrorCode(exports, "E_OVERFLOW", qdb_e_overflow);
        AddErrorCode(exports, "E_UNDERFLOW", qdb_e_underflow);
        AddErrorCode(exports, "E_TAG_ALREADY_SET", qdb_e_tag_already_set);
        AddErrorCode(exports, "E_TAG_NOT_SET", qdb_e_tag_not_set);
        AddErrorCode(exports, "E_TIMEOUT", qdb_e_timeout);
        AddErrorCode(exports, "E_CONNECTION_REFUSED", qdb_e_connection_refused);
        AddErrorCode(exports, "E_CONNECTION_RESET", qdb_e_connection_reset);
        AddErrorCode(exports, "E_UNSTABLE_CLUSTER", qdb_e_unstable_cluster);
        AddErrorCode(exports, "E_TRY_AGAIN", qdb_e_try_again);
        AddErrorCode(exports, "E_CONFLICT", qdb_e_conflict);
        AddErrorCode(exports, "E_NOT_CONNECTED", qdb_e_not_connected);
        AddErrorCode(exports, "E_RESOURCE_LOCKED", qdb_e_resource_locked);
        AddErrorCode(exports, "E_SYSTEM_REMOTE", qdb_e_system_remote);
        AddErrorCode(exports, "E_SYSTEM_LOCAL", qdb_e_system_local);
        AddErrorCode(exports, "E_INTERNAL_REMOTE", qdb_e_internal_remote);
        AddErrorCode(exports, "E_INTERNAL_LOCAL", qdb_e_internal_local);
        AddErrorCode(exports, "E_NO_MEMORY_REMOTE", qdb_e_no_memory_remote);
        AddErrorCode(exports, "E_NO_MEMORY_LOCAL", qdb_e_no_memory_local);
        AddErrorCode(exports, "E_INVALID_PROTOCOL", qdb_e_invalid_protocol);
        AddErrorCode(exports, "E_HOST_NOT_FOUND", qdb_e_host_not_found);
        AddErrorCode(exports, "E_BUFFER_TOO_SMALL", qdb_e_buffer_too_small);
        AddErrorCode(exports, "E_NOT_IMPLEMENTED", qdb_e_not_implemented);
        AddErrorCode(exports, "E_INVALID_VERSION", qdb_e_invalid_version);
        AddErrorCode(exports, "E_INVALID_ARGUMENT", qdb_e_invalid_argument);
        AddErrorCode(exports, "E_INVALID_HANDLE", qdb_e_invalid_handle);
        AddErrorCode(exports, "E_RESERVED_ALIAS", qdb_e_reserved_alias);
        AddErrorCode(exports, "E_UNMATCHED_CONTENT", qdb_e_unmatched_content);
        AddErrorCode(exports, "E_INVALID_ITERATOR", qdb_e_invalid_iterator);
        AddErrorCode(exports, "E_ENTRY_TOO_LARGE", qdb_e_entry_too_large);
        AddErrorCode(exports, "E_TRANSACTION_PARTIAL_FAILURE", qdb_e_transaction_partial_failure);
        AddErrorCode(exports, "E_OPERATION_DISABLED", qdb_e_operation_disabled);
        AddErrorCode(exports, "E_OPERATION_NOT_PERMITTED", qdb_e_operation_not_permitted);
        AddErrorCode(exports, "E_ITERATOR_END", qdb_e_iterator_end);
        AddErrorCode(exports, "E_INVALID_REPLY", qdb_e_invalid_reply);
        AddErrorCode(exports, "E_OK_CREATED", qdb_e_ok_created);
        AddErrorCode(exports, "E_NO_SPACE_LEFT", qdb_e_no_space_left);
        AddErrorCode(exports, "E_QUOTA_EXCEEDED", qdb_e_quota_exceeded);
        AddErrorCode(exports, "E_ALIAS_TOO_LONG", qdb_e_alias_too_long);
        AddErrorCode(exports, "E_CLOCK_SKEW", qdb_e_clock_skew);
        AddErrorCode(exports, "E_ACCESS_DENIED", qdb_e_access_denied);
        AddErrorCode(exports, "E_LOGIN_FAILED", qdb_e_login_failed);
        AddErrorCode(exports, "E_COLUMN_NOT_FOUND", qdb_e_column_not_found);
        AddErrorCode(exports, "E_QUERY_TOO_COMPLEX", qdb_e_query_too_complex);
        AddErrorCode(exports, "E_INVALID_CRYPTO_KEY", qdb_e_invalid_crypto_key);
        AddErrorCode(exports, "E_INVALID_QUERY", qdb_e_invalid_query);
        AddErrorCode(exports, "E_INVALID_REGEX", qdb_e_invalid_regex);
        AddErrorCode(exports, "E_UNKNOWN_USER", qdb_e_unknown_user);
        AddErrorCode(exports, "E_INTERRUPTED", qdb_e_interrupted);
        AddErrorCode(exports, "E_NETWORK_INBUF_TOO_SMALL", qdb_e_network_inbuf_too_small);
        AddErrorCode(exports, "E_NETWORK_ERROR", qdb_e_network_error);
        AddErrorCode(exports, "E_DATA_CORRUPTION", qdb_e_data_corruption);

        auto maybe_function = tpl->GetFunction(isolate->GetCurrentContext());
        if (maybe_function.IsEmpty()) return;

        constructor.Reset(isolate, maybe_function.ToLocalChecked());
        exports->Set(isolate->GetCurrentContext(),
            v8::String::NewFromUtf8(isolate, "Error", v8::NewStringType::kNormal).ToLocalChecked(),
            maybe_function.ToLocalChecked());
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

            auto code = argsEater.eatNumber();
            if (!code.second)
            {
                call.throwException("Expected an error code as first argument");
                return;
            }

            const qdb_error_t err = static_cast<qdb_error_t>(static_cast<unsigned long>(code.first));

            Error * e = new Error(err);

            e->Wrap(args.This());
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
        static const int argc = 1;
        v8::Local<v8::Value> argv[argc] = {args[0]};
        v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);
        v8::MaybeLocal<v8::Object> instance = cons->NewInstance(isolate->GetCurrentContext(), argc, argv);
        args.GetReturnValue().Set(instance.ToLocalChecked());
    }

public:
    static v8::Local<v8::Object> MakeError(v8::Isolate * isolate, qdb_error_t err)
    {
        static const int argc = 1;
        v8::Local<v8::Value> argv[argc] = {v8::Number::New(isolate, err)};
        v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);
        return cons->NewInstance(isolate->GetCurrentContext(), 1, argv).ToLocalChecked();
    }

private:
    template <typename F>
    static void accessor(const v8::FunctionCallbackInfo<v8::Value> & args, F f)
    {
        MethodMan call(args);

        if (args.Length() != 0)
        {
            call.throwException("Wrong number of arguments");
            return;
        }

        Error * e = call.nativeHolder<Error>();
        assert(e);

        f(args, e);
    }

public:
    // :desc: Determines if the error is an informational error.
    // :returns: True if the error is informational, false otherwise.
    static void informational(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Error::accessor(args,
            [](const v8::FunctionCallbackInfo<v8::Value> & args, Error * e)
            {
                v8::Isolate * isolate = args.GetIsolate();
                args.GetReturnValue().Set(v8::Boolean::New(isolate, QDB_SUCCESS(e->_error)));
            });
    }

    // :desc: Gets the origin of the error
    // :returns: A string containing the origin of the error.
    static void origin(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Error::accessor(args,
            [](const v8::FunctionCallbackInfo<v8::Value> & args, Error * e)
            {
                v8::Isolate * isolate = args.GetIsolate();
                args.GetReturnValue().Set(v8::Integer::New(isolate, QDB_ERROR_ORIGIN(e->_error)));
            });
    }

    // :desc: Gets the severity of the errror
    // :returns: A string containing the severity of the error.
    static void severity(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Error::accessor(args,
            [](const v8::FunctionCallbackInfo<v8::Value> & args, Error * e)
            {
                v8::Isolate * isolate = args.GetIsolate();
                args.GetReturnValue().Set(v8::Integer::New(isolate, QDB_ERROR_SEVERITY(e->_error)));
            });
    }

    // :desc: Gets a description of the error.
    // :returns: A string containing the error message.
    static void message(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Error::accessor(args,
            [](const v8::FunctionCallbackInfo<v8::Value> & args, Error * e)
            {
                // the string returned by qdb_error is static it is therefore safe and efficient to do this
                v8::Isolate * isolate = args.GetIsolate();
                args.GetReturnValue().Set(
                    v8::String::NewFromUtf8(isolate, qdb_error(e->_error), v8::NewStringType::kNormal)
                        .ToLocalChecked());
            });
    }

    // :desc: Gets the error code.
    // :returns: An integer with the error code.
    static void code(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Error::accessor(args,
            [](const v8::FunctionCallbackInfo<v8::Value> & args, Error * e)
            {
                // the string returned by qdb_error is static it is therefore safe and efficient to do this
                v8::Isolate * isolate = args.GetIsolate();

                // read low 16-bit for the code
                args.GetReturnValue().Set(v8::Integer::NewFromUnsigned(isolate, e->_error & 0xffff));
            });
    }

private:
    const qdb_error_t _error;

    static v8::Persistent<v8::Function> constructor;
};

} // namespace quasardb
