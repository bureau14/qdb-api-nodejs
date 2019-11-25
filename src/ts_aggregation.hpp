#pragma once

#include "ts_range.hpp"
#include <qdb/client.h>
#include <qdb/ts.h>
#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>

namespace quasardb
{

class Aggregation : public node::ObjectWrap
{
    static const size_t ParametersCount = 2;

public:
    Aggregation(qdb_ts_aggregation_type_t type, v8::Isolate * isolate, v8::Local<v8::Object> range)
        : type(type), range_obj(isolate, range)
    {
        auto fr = node::ObjectWrap::Unwrap<TsRange>(range);
        this->range = fr->nativeRange();
    }

    virtual ~Aggregation()
    {
        range_obj.Reset();
    }

    static void Init(v8::Local<v8::Object> exports)
    {
        v8::Isolate * isolate = exports->GetIsolate();

        // Prepare constructor template
        v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, New);
        tpl->SetClassName(v8::String::NewFromUtf8(isolate, "Aggregation", v8::NewStringType::kNormal).ToLocalChecked());
        tpl->InstanceTemplate()->SetInternalFieldCount(1);

        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Signature> s = v8::Signature::New(isolate, tpl);

        auto proto = tpl->PrototypeTemplate();

        proto->SetAccessorProperty(v8::String::NewFromUtf8(isolate, "type", v8::NewStringType::kNormal).ToLocalChecked(),
                                   v8::FunctionTemplate::New(isolate, Aggregation::getType, v8::Local<v8::Value>(), s),
                                   v8::Local<v8::FunctionTemplate>(), v8::ReadOnly);
        proto->SetAccessorProperty(v8::String::NewFromUtf8(isolate, "range", v8::NewStringType::kNormal).ToLocalChecked(),
                                   v8::FunctionTemplate::New(isolate, Aggregation::getRange, v8::Local<v8::Value>(), s),
                                   v8::Local<v8::FunctionTemplate>(), v8::ReadOnly);

        // Export aggregation types
        AddAggrType(exports, "AggFirst", qdb_agg_first);
        AddAggrType(exports, "AggLast", qdb_agg_last);
        AddAggrType(exports, "AggMin", qdb_agg_min);
        AddAggrType(exports, "AggMax", qdb_agg_max);

        AddAggrType(exports, "AggArithmeticMean", qdb_agg_arithmetic_mean);
        AddAggrType(exports, "AggHarmonicMean", qdb_agg_harmonic_mean);
        AddAggrType(exports, "AggGeometricMean", qdb_agg_geometric_mean);
        AddAggrType(exports, "AggQuadraticMean", qdb_agg_quadratic_mean);

        AddAggrType(exports, "AggCount", qdb_agg_count);
        AddAggrType(exports, "AggSum", qdb_agg_sum);
        AddAggrType(exports, "AggSumOfSquares", qdb_agg_sum_of_squares);
        AddAggrType(exports, "AggSpread", qdb_agg_spread);

        AddAggrType(exports, "AggSampleVariance", qdb_agg_sample_variance);
        AddAggrType(exports, "AggSampleStddev", qdb_agg_sample_stddev);
        AddAggrType(exports, "AggPopulationVariance", qdb_agg_population_variance);
        AddAggrType(exports, "AggPopulationStddev", qdb_agg_population_stddev);

        AddAggrType(exports, "AggAbsMin", qdb_agg_abs_min);
        AddAggrType(exports, "AggAbsMax", qdb_agg_abs_max);
        AddAggrType(exports, "AggProduct", qdb_agg_product);
        AddAggrType(exports, "AggSkewness", qdb_agg_skewness);
        AddAggrType(exports, "AggKurtosis", qdb_agg_kurtosis);

        auto maybe_function = tpl->GetFunction(isolate->GetCurrentContext());
        if (maybe_function.IsEmpty()) return;

        constructor.Reset(isolate, maybe_function.ToLocalChecked());
        exports->Set(isolate->GetCurrentContext(), v8::String::NewFromUtf8(isolate, "Aggregation", v8::NewStringType::kNormal).ToLocalChecked(), maybe_function.ToLocalChecked());
    }

    static void NewInstance(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        v8::Isolate * isolate = args.GetIsolate();

        assert(args.Length() == ParametersCount);
        static const int argc = ParametersCount;

        v8::Local<v8::Value> argv[argc] = {args[0], args[1]};

        v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);
        assert(!cons.IsEmpty() && "Verify that Object::Init has been called in qdb_api.cpp:InitAll()");
        v8::MaybeLocal<v8::Object> instance = cons->NewInstance(isolate->GetCurrentContext(), argc, argv);
        args.GetReturnValue().Set(instance.ToLocalChecked());
    }

    qdb_ts_aggregation_type_t nativeType() const
    {
        return this->type;
    }

    qdb_ts_range_t nativeRange() const
    {
        return this->range;
    }

private:
    static void New(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        if (args.IsConstructCall())
        {
            if (args.Length() != ParametersCount)
            {
                throwException(args, "Wrong number of arguments");
                return;
            }

            if (!args[0]->IsNumber() || !args[1]->IsObject())
            {
                throwException(args, "Wrong type of arguments");
                return;
            }

            auto context = args.GetIsolate()->GetCurrentContext();

            auto maybe_type = args[0]->Int32Value(context);
            if (maybe_type.IsNothing())
            {
                throwException(args, "Missing type argument");
                return;
            }

            auto type = static_cast<qdb_ts_aggregation_type_t>(maybe_type.FromJust());

            auto maybe_range = args[1]->ToObject(context);
            if (maybe_range.IsEmpty())
            {
                throwException(args, "Missing range argument");
                return;
            }

            // TODO: Check that type of the class was TsRange
            auto obj = new Aggregation(type, args.GetIsolate(), maybe_range.ToLocalChecked());

            obj->Wrap(args.This());
            args.GetReturnValue().Set(args.This());
        }
        else
        {
            NewInstance(args);
        }
    }

private:
    static void throwException(const v8::FunctionCallbackInfo<v8::Value> & args, const char * msg)
    {
        auto isolate = args.GetIsolate();
        isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, msg, v8::NewStringType::kNormal).ToLocalChecked()));
    }

    template <typename F>
    static void getter(const v8::FunctionCallbackInfo<v8::Value> & args, F f)
    {
        if (args.Length() != 0)
        {
            throwException(args, "Wrong number of arguments");
            return;
        }

        auto obj = node::ObjectWrap::Unwrap<Aggregation>(args.Holder());
        assert(obj);

        f(args, obj);
    }

    static void getType(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Aggregation::getter(args, [](const v8::FunctionCallbackInfo<v8::Value> & args, Aggregation * aggr) {
            v8::Isolate * isolate = args.GetIsolate();

            args.GetReturnValue().Set(v8::Int32::New(isolate, aggr->type));
        });
    }

    static void getRange(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Aggregation::getter(args, [](const v8::FunctionCallbackInfo<v8::Value> & args, Aggregation * aggr) {
            v8::Isolate * isolate = args.GetIsolate();

            auto value = v8::Local<v8::Object>::New(isolate, aggr->range_obj);
            args.GetReturnValue().Set(value);
        });
    }

    static void AddAggrType(v8::Local<v8::Object> exports, const char * name, qdb_ts_aggregation_type_t type)
    {
        v8::Isolate * isolate = exports->GetIsolate();
        auto value = v8::Int32::New(isolate, type);
        v8::Maybe<bool> maybe =
            exports->DefineOwnProperty(isolate->GetCurrentContext(), v8::String::NewFromUtf8(isolate, name, v8::NewStringType::kNormal).ToLocalChecked(), value,
                                       static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontDelete));
        (void)maybe;
        assert(maybe.IsJust() && maybe.FromJust());
    }

private:
    qdb_ts_aggregation_type_t type;
    qdb_ts_range_t range;

    v8::Persistent<v8::Object> range_obj;

    static v8::Persistent<v8::Function> constructor;
};

} // namespace quasardb
