#pragma once

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>

#include <qdb/client.h>
#include <qdb/ts.h>

#include "time.hpp"

namespace quasardb
{

class FilteredRange : public node::ObjectWrap
{
public:
    explicit FilteredRange(qdb_ts_range_t range)
    {
        this->filtered_range.range = range;
        this->filtered_range.filter.type = qdb_ts_filter_none;
    }

    FilteredRange(qdb_ts_range_t range, qdb_ts_filter_t filter, v8::Isolate * isolate, v8::Local<v8::Object> obj)
        : filter_obj(isolate, obj)
    {
        this->filtered_range.range = range;
        this->filtered_range.filter = filter;
    }

    virtual ~FilteredRange()
    {
        filter_obj.Reset();
    }

    static void Init(v8::Local<v8::Object> exports)
    {
        v8::Isolate * isolate = exports->GetIsolate();

        // Prepare constructor template
        v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, New);
        tpl->SetClassName(v8::String::NewFromUtf8(isolate, "TsRange"));
        tpl->InstanceTemplate()->SetInternalFieldCount(1);

        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Signature> s = v8::Signature::New(isolate, tpl);

        auto proto = tpl->PrototypeTemplate();

        proto->SetAccessorProperty(v8::String::NewFromUtf8(isolate, "begin"),
                                   v8::FunctionTemplate::New(isolate, FilteredRange::begin, v8::Local<v8::Value>(), s),
                                   v8::Local<v8::FunctionTemplate>(), v8::ReadOnly);
        proto->SetAccessorProperty(v8::String::NewFromUtf8(isolate, "end"),
                                   v8::FunctionTemplate::New(isolate, FilteredRange::end, v8::Local<v8::Value>(), s),
                                   v8::Local<v8::FunctionTemplate>(), v8::ReadOnly);
        proto->SetAccessorProperty(v8::String::NewFromUtf8(isolate, "filter"),
                                   v8::FunctionTemplate::New(isolate, FilteredRange::filter, v8::Local<v8::Value>(), s),
                                   v8::Local<v8::FunctionTemplate>(), v8::ReadOnly);

        // Function to create filters
        NODE_SET_METHOD(exports, "FilterUnique", filterUnique);
        NODE_SET_METHOD(exports, "FilterSample", filterSample);
        NODE_SET_METHOD(exports, "FilterDoubleInside", filterDoubleRange<qdb_ts_filter_double_inside_range>);
        NODE_SET_METHOD(exports, "FilterDoubleOutside", filterDoubleRange<qdb_ts_filter_double_outside_range>);

        // Export filter types
        AddTsType<qdb_ts_filter_type_t>(exports, "TS_FILTER_NONE", qdb_ts_filter_none);
        AddTsType<qdb_ts_filter_type_t>(exports, "TS_FILTER_UNIQUE", qdb_ts_filter_unique);
        AddTsType<qdb_ts_filter_type_t>(exports, "TS_FILTER_SAMPLE", qdb_ts_filter_sample);
        AddTsType<qdb_ts_filter_type_t>(exports, "TS_FILTER_DOUBLE_INSIDE_RANGE", qdb_ts_filter_double_inside_range);
        AddTsType<qdb_ts_filter_type_t>(exports, "TS_FILTER_DOUBLE_OUTSIDE_RANGE", qdb_ts_filter_double_outside_range);

        constructor.Reset(isolate, tpl->GetFunction());
        exports->Set(v8::String::NewFromUtf8(isolate, "TsRange"), tpl->GetFunction());
    }

    // Two arguments for regular range, three for range with filter
    static void NewInstance(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        v8::Isolate * isolate = args.GetIsolate();

        assert(args.Length() >= 2 && args.Length() <= 3);
        static const int argcmax = 3;

        v8::Local<v8::Value> argv[argcmax] = {args[0], args[1]};
        if (args.Length() > 2)
        {
            argv[2] = args[2];
        }

        v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);
        assert(!cons.IsEmpty() && "Verify that Object::Init has been called in qdb_api.cpp:InitAll()");
        v8::MaybeLocal<v8::Object> instance = cons->NewInstance(isolate->GetCurrentContext(), args.Length(), argv);
        args.GetReturnValue().Set(instance.ToLocalChecked());
    }

    qdb_ts_filtered_range_t nativeValue() const
    {
        return this->filtered_range;
    }

private:
    static void New(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        if (args.IsConstructCall())
        {
            // Can be simple range with only dates arguments
            // or range with filter
            if (args.Length() < 2 || args.Length() > 3)
            {
                throwException(args, "Wrong number of arguments");
                return;
            }

            bool badFilter = args.Length() == 3 && !args[2]->IsObject();
            if (!args[0]->IsDate() || !args[1]->IsDate() || badFilter)
            {
                throwException(args, "Wrong type of arguments");
                return;
            }

            auto beginMs = v8::Local<v8::Date>::Cast(args[0])->ValueOf();
            auto endMs = v8::Local<v8::Date>::Cast(args[1])->ValueOf();

            qdb_ts_range_t range;
            range.begin = ms_to_qdb_timespec(beginMs);
            range.end = ms_to_qdb_timespec(endMs);

            FilteredRange * obj = nullptr;
            if (args.Length() != 3)
            {
                // Range without filter
                obj = new FilteredRange(range);
            }
            else
            {
                auto filter = convertFilter(args[2]->ToObject());
                obj = new FilteredRange(range, filter, args.GetIsolate(), args[2]->ToObject());
            }

            obj->Wrap(args.This());
            args.GetReturnValue().Set(args.This());
        }
        else
        {
            NewInstance(args);
        }
    }

    static qdb_ts_filter_t convertFilter(v8::Local<v8::Object> obj)
    {
        qdb_ts_filter_t filter;

        auto typeProp = v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), "type");
        auto type = static_cast<qdb_ts_filter_type>(obj->Get(typeProp)->Int32Value());

        filter.type = type;
        switch (type)
        {

        case qdb_ts_filter_sample:
        {
            auto ssProp = v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), "samples");
            auto samples = obj->Get(ssProp);
            filter.params.sample.size = static_cast<size_t>(samples->NumberValue());
            break;
        }

        case qdb_ts_filter_double_inside_range:
        case qdb_ts_filter_double_outside_range:
        {
            auto minProp = v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), "min");
            auto maxProp = v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), "max");
            auto min = obj->Get(minProp);
            auto max = obj->Get(maxProp);

            filter.params.double_range.min = min->NumberValue();
            filter.params.double_range.max = max->NumberValue();
            break;
        }

        default:
            return filter;
        };

        return filter;
    }

private:
    static void throwException(const v8::FunctionCallbackInfo<v8::Value> & args, const char * msg)
    {
        auto isolate = args.GetIsolate();
        isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, msg)));
    }

    template <typename F>
    static void getter(const v8::FunctionCallbackInfo<v8::Value> & args, F f)
    {
        if (args.Length() != 0)
        {
            throwException(args, "Wrong number of arguments");
            return;
        }

        auto obj = node::ObjectWrap::Unwrap<FilteredRange>(args.Holder());
        assert(obj);

        f(args, obj);
    }

    static void begin(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        FilteredRange::getter(args, [](const v8::FunctionCallbackInfo<v8::Value> & args, FilteredRange * fr) {
            v8::Isolate * isolate = args.GetIsolate();
            auto ms = qdb_timespec_to_ms(fr->filtered_range.range.begin);
            auto value = v8::Date::New(isolate, ms);
            args.GetReturnValue().Set(value);
        });
    }

    static void end(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        FilteredRange::getter(args, [](const v8::FunctionCallbackInfo<v8::Value> & args, FilteredRange * fr) {
            v8::Isolate * isolate = args.GetIsolate();
            auto ms = qdb_timespec_to_ms(fr->filtered_range.range.end);
            auto value = v8::Date::New(isolate, ms);
            args.GetReturnValue().Set(value);
        });
    }

    static void filter(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        FilteredRange::getter(args, [](const v8::FunctionCallbackInfo<v8::Value> & args, FilteredRange * fr) {
            v8::Isolate * isolate = args.GetIsolate();

            auto value = v8::Local<v8::Object>::New(isolate, fr->filter_obj);
            args.GetReturnValue().Set(value);
        });
    }

    static void filterUnique(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        v8::Isolate * isolate = args.GetIsolate();
        v8::Local<v8::Object> obj = v8::Object::New(isolate);

        if (args.Length() != 0)
        {
            throwException(args, "Wrong number of arguments");
            return;
        }

        obj->Set(v8::String::NewFromUtf8(isolate, "type"), v8::Int32::New(isolate, qdb_ts_filter_unique));
        args.GetReturnValue().Set(obj);
    }

    static void filterSample(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        v8::Isolate * isolate = args.GetIsolate();
        v8::Local<v8::Object> obj = v8::Object::New(isolate);

        if (args.Length() != 1)
        {
            throwException(args, "Wrong number of arguments");
            return;
        }

        if (!args[0]->IsNumber())
        {
            throwException(args, "Wrong type of arguments");
            return;
        }

        obj->Set(v8::String::NewFromUtf8(isolate, "type"), v8::Int32::New(isolate, qdb_ts_filter_sample));
        obj->Set(v8::String::NewFromUtf8(isolate, "samples"), args[0]);
        args.GetReturnValue().Set(obj);
    }

    template <qdb_ts_filter_type_t type>
    static void filterDoubleRange(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        v8::Isolate * isolate = args.GetIsolate();
        v8::Local<v8::Object> obj = v8::Object::New(isolate);

        if (args.Length() != 2)
        {
            throwException(args, "Wrong number of arguments");
            return;
        }

        if (!args[0]->IsNumber() || !args[1]->IsNumber())
        {
            throwException(args, "Wrong type of arguments");
            return;
        }

        obj->Set(v8::String::NewFromUtf8(isolate, "type"), v8::Int32::New(isolate, type));
        obj->Set(v8::String::NewFromUtf8(isolate, "min"), args[0]);
        obj->Set(v8::String::NewFromUtf8(isolate, "max"), args[1]);
        args.GetReturnValue().Set(obj);
    }

    template <typename Type>
    static void AddTsType(v8::Local<v8::Object> exports, const char * name, Type t)
    {
        v8::Isolate * isolate = exports->GetIsolate();
        auto value = v8::Int32::New(isolate, t);
        v8::Maybe<bool> maybe =
            exports->DefineOwnProperty(isolate->GetCurrentContext(), v8::String::NewFromUtf8(isolate, name), value,
                                       static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontDelete));
        (void)maybe;
        assert(maybe.IsJust() && maybe.FromJust());
    }

private:
    qdb_ts_filtered_range_t filtered_range;
    v8::Persistent<v8::Object> filter_obj;

    static v8::Persistent<v8::Function> constructor;
}; // FilteredRange

} // namespace quasardb
