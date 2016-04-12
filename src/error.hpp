
#pragma once

#include <qdb/client.h>

#include "utilities.hpp"

namespace qdb
{

    class Error : public node::ObjectWrap
    {

    public:
        explicit Error(qdb_error_t e) : _error(e) {}
        virtual ~Error(void) {}

    public:
        static void Init(v8::Local<v8::Object> exports)
        {
            v8::Isolate* isolate = exports->GetIsolate();

            // Prepare constructor template
            v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, New);
            tpl->SetClassName(v8::String::NewFromUtf8(isolate, "Error"));
            tpl->InstanceTemplate()->SetInternalFieldCount(1);

            v8::HandleScope handle_scope(isolate);
            v8::Local<v8::Signature> s = v8::Signature::New(isolate, tpl);

            tpl->PrototypeTemplate()->SetAccessorProperty(v8::String::NewFromUtf8(isolate, "informational"),
                v8::FunctionTemplate::New(isolate, Error::informational, v8::Local<v8::Value>(), s),
                v8::Local<v8::FunctionTemplate>(),
                v8::ReadOnly);
            tpl->PrototypeTemplate()->SetAccessorProperty(v8::String::NewFromUtf8(isolate, "origin"),
                v8::FunctionTemplate::New(isolate, Error::origin, v8::Local<v8::Value>(), s),
                v8::Local<v8::FunctionTemplate>(),
                v8::ReadOnly);
            tpl->PrototypeTemplate()->SetAccessorProperty(v8::String::NewFromUtf8(isolate, "severity"),
                v8::FunctionTemplate::New(isolate, Error::severity, v8::Local<v8::Value>(), s),
                v8::Local<v8::FunctionTemplate>(),
                v8::ReadOnly);
            tpl->PrototypeTemplate()->SetAccessorProperty(v8::String::NewFromUtf8(isolate, "message"),
                v8::FunctionTemplate::New(isolate, Error::message, v8::Local<v8::Value>(), s),
                v8::Local<v8::FunctionTemplate>(),
                v8::ReadOnly);
            tpl->PrototypeTemplate()->SetAccessorProperty(v8::String::NewFromUtf8(isolate, "code"),
                v8::FunctionTemplate::New(isolate, Error::code, v8::Local<v8::Value>(), s),
                v8::Local<v8::FunctionTemplate>(),
                v8::ReadOnly);

            constructor.Reset(isolate, tpl->GetFunction());
            exports->Set(v8::String::NewFromUtf8(isolate, "Error"), tpl->GetFunction());
        }

    private:
        static void New(const v8::FunctionCallbackInfo<v8::Value>& args)
        {
            if (args.IsConstructCall())
            {
                MethodMan call(args);

                if (args.Length() != 1)
                {
                    call.throwException("Expected exactly one argument");
                    return;
                }

                auto code = call.checkedArgNumber(0);
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
            v8::Isolate* isolate = args.GetIsolate();
            // Invoked as plain function `MyObject(...)`, turn into construct call.
            static const int argc = 1;
            v8::Local<v8::Value> argv[argc] = { args[0] };
            v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);
            v8::Local<v8::Object> instance = cons->NewInstance(argc, argv);
            args.GetReturnValue().Set(instance);
        }

    public:
        static v8::Local<v8::Object> MakeError(v8::Isolate* isolate, qdb_error_t err)
        {
            static const int argc = 1;
            v8::Local<v8::Value> argv[argc] = { v8::Number::New(isolate, err) };
            v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);
            return cons->NewInstance(1, argv);
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
        static void informational(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Error::accessor(args, [](const v8::FunctionCallbackInfo<v8::Value> & args, Error * e)
            {
                v8::Isolate* isolate = args.GetIsolate();
                args.GetReturnValue().Set(v8::Boolean::New(isolate, QDB_SUCCESS(e->_error)));
            });
        }

        static void origin(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Error::accessor(args, [](const v8::FunctionCallbackInfo<v8::Value> & args, Error * e)
            {
                v8::Isolate* isolate = args.GetIsolate();
                args.GetReturnValue().Set(v8::Integer::New(isolate, QDB_ERROR_ORIGIN(e->_error)));
            });
        }

        static void severity(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Error::accessor(args, [](const v8::FunctionCallbackInfo<v8::Value> & args, Error * e)
            {
                v8::Isolate* isolate = args.GetIsolate();
                args.GetReturnValue().Set(v8::Integer::New(isolate, QDB_ERROR_SEVERITY(e->_error)));
            });
        }

        static void message(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Error::accessor(args, [](const v8::FunctionCallbackInfo<v8::Value> & args, Error * e)
            {
                // the string returned by qdb_error is static it is therefore safe and efficient to do this
                v8::Isolate* isolate = args.GetIsolate();
                args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, qdb_error(e->_error)));
            });
        }

        static void code(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Error::accessor(args, [](const v8::FunctionCallbackInfo<v8::Value> & args, Error * e)
            {
                // the string returned by qdb_error is static it is therefore safe and efficient to do this
                v8::Isolate* isolate = args.GetIsolate();

                // read low 16-bit for the code
                args.GetReturnValue().Set(v8::Integer::NewFromUnsigned(isolate, e->_error & 0xffff));
            });
        }

    private:
        const qdb_error_t _error;

        static v8::Persistent<v8::Function> constructor;
    };
}