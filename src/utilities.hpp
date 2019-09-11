#pragma once

#include "cluster_data.hpp"
#include "time.hpp"
#include "ts_aggregation.hpp"
#include "ts_range.hpp"
#include <qdb/batch.h>
#include <qdb/client.h>
#include <qdb/integer.h>
#include <qdb/query.h>
#include <qdb/ts.h>
#include <node.h>
#include <node_buffer.h>
#include <array>
#include <functional>
#include <utility>
#include <vector>

namespace quasardb
{
namespace detail
{

static inline void
AddConstantProperty(v8::Isolate * isolate, v8::Local<v8::Object> object, const char * key, v8::Local<v8::Value> value)
{
    // object->ForceSet(v8::String::NewFromUtf8(isolate, key), value,
    //                 static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontDelete));

    v8::Maybe<bool> maybe =
        object->DefineOwnProperty(isolate->GetCurrentContext(), v8::String::NewFromUtf8(isolate, key), value,
                                  static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontDelete));
    (void)maybe; // unused
    assert(maybe.IsJust() && maybe.FromJust());
}

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

template <>
struct NewObject<v8::Date>
{

    template <typename P>
    v8::Local<v8::Value> operator()(v8::Isolate * i, P && p)
    {
        // strings don't use new, they use newfromutf8
        return v8::Date::New(i->GetCurrentContext(), qdb_timespec_to_ms(std::forward<P>(p))).ToLocalChecked();
    }
};

template <>
struct NewObject<Timestamp>
{

    template <typename P>
    v8::Local<v8::Value> operator()(v8::Isolate * i, P && p)
    {
        return Timestamp::NewFromTimespec(i, std::forward<P>(p));
    }
};

inline void release_node_buffer(char * data, void * hint)
{
    if (data && hint)
    {
        qdb_release(static_cast<qdb_handle_t>(hint), data);
    }
}

} // namespace detail

// POD for storing info about columns from js calls
struct column_info
{
    column_info(std::string name = std::string(), qdb_ts_column_type_t type = qdb_ts_column_uninitialized)
        : name(name), type(type)
    {
    }

    std::string name;
    qdb_ts_column_type_t type;
};

struct qdb_request
{
    struct slice
    {
        const void * begin;
        size_t size;
    };

    struct query
    {
        query(std::string a = "") : alias(a), expiry(0)
        {
        }

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

            // Time series
            // TODO(denisb): consider to move it all to err slice ?
            std::vector<column_info> columns;

            std::vector<qdb_ts_blob_point> blob_points;
            std::vector<qdb_ts_double_point> double_points;
            std::vector<qdb_ts_int64_point> int64_points;
            std::vector<qdb_ts_timestamp_point> timestamp_points;

            std::vector<qdb_ts_range_t> ranges;

            std::vector<qdb_ts_blob_aggregation_t> blob_aggrs;
            std::vector<qdb_ts_double_aggregation_t> double_aggrs;
            std::vector<qdb_ts_int64_aggregation_t> int64_aggrs;
            std::vector<qdb_ts_timestamp_aggregation_t> timestamp_aggrs;
        };

        query_content content;

        qdb_time_t expiry;
    };

    struct result
    {
        result(qdb_error_t err = qdb_e_uninitialized) : error(err)
        {
        }

        union {
            slice buffer;
            qdb_uint_t uvalue;
            qdb_int_t value;
            qdb_entry_metadata_t entry_metadata;
            qdb_entry_type_t entry_type;
            qdb_time_t date;
        } content;

        struct
        {
            std::vector<qdb_operation_t> operations;
            qdb_size_t success_count;
        } batch;

        qdb_query_result_t * query_result;

        qdb_error_t error;
    };

    explicit qdb_request(qdb_error_t err) : _cluster_data(nullptr), output(err)
    {
    }

    qdb_request(cluster_data_ptr cd, std::function<void(qdb_request *)> exec, std::string a)
        : _cluster_data(cd), input(a), _execute(exec)
    {
    }

    ~qdb_request(void)
    {
        callback.Reset();
        holder.Reset();
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
    v8::MaybeLocal<v8::Object> make_node_buffer(v8::Isolate * isolate, const void * buf, size_t length)
    {
        qdb_handle_t h = handle();

        if (!h || !buf || !length)
        {
            return node::Buffer::New(isolate, static_cast<size_t>(0u));
        }

        return node::Buffer::New(isolate, static_cast<char *>(const_cast<void *>(buf)), length,
                                 detail::release_node_buffer, h);
    }

    v8::MaybeLocal<v8::Object> make_node_buffer(v8::Isolate * isolate)
    {
        return make_node_buffer(isolate, output.content.buffer.begin, output.content.buffer.size);
    }

public:
    v8::Persistent<v8::Function> callback;
    v8::Persistent<v8::Object> holder;

    v8::Local<v8::Function> callbackAsLocal(void)
    {
        if (callback.IsWeak())
        {
            return v8::Local<v8::Function>::New(v8::Isolate::GetCurrent(), callback);
        }

        return *reinterpret_cast<v8::Local<v8::Function> *>(const_cast<v8::Persistent<v8::Function> *>(&callback));
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
    std::function<void(qdb_request *)> _execute;

    // prevent copy
    qdb_request(const qdb_request &)
    {
    }
};

struct MethodMan
{
    explicit MethodMan(const v8::FunctionCallbackInfo<v8::Value> & args)
        : _isolate(args.GetIsolate()), _scope(_isolate), _args(args)
    {
    }

public:
    v8::Local<v8::Object> holder(void) const
    {
        return _args.Holder();
    }

    template <typename Object>
    Object * nativeHolder(void) const
    {
        return node::ObjectWrap::Unwrap<Object>(holder());
    }

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
        auto maybe_arg = _args[i]->ToObject(_args.GetIsolate()->GetCurrentContext());
        if (maybe_arg.IsEmpty())
        {
            // FIXME(Marek): What should we do here?
            return {};
        }

        return maybe_arg.ToLocalChecked();
    }

    // std::optional not available in VS 2013
    std::pair<v8::Local<v8::Object>, bool> checkedArgObject(int i) const
    {
        return checkArg<v8::Local<v8::Object>>(
            i, [](v8::Local<v8::Value> v) -> bool { return !v->IsFunction() && v->IsObject(); }, &MethodMan::argObject);
    }

    v8::Local<v8::Function> argCallback(int i) const
    {
        return v8::Local<v8::Function>::Cast(_args[i]);
    }

    std::pair<v8::Local<v8::Function>, bool> checkedArgCallback(int i) const
    {
        return checkArg<v8::Local<v8::Function>>(i, [](v8::Local<v8::Value> v) -> bool { return v->IsFunction(); },
                                                 &MethodMan::argCallback);
    }

    v8::Local<v8::String> argString(int i) const
    {
        return _args[i]->ToString(_args.GetIsolate());
    }

    std::pair<v8::Local<v8::String>, bool> checkedArgString(int i) const
    {
        return checkArg<v8::Local<v8::String>>(i, [](v8::Local<v8::Value> v) -> bool { return v->IsString(); },
                                               &MethodMan::argString);
    }

    v8::Local<v8::Array> argArray(int i) const
    {
        return v8::Local<v8::Array>::Cast(_args[i]);
    }

    std::pair<v8::Local<v8::Array>, bool> checkedArgArray(int i) const
    {
        return checkArg<v8::Local<v8::Array>>(i, [](v8::Local<v8::Value> v) -> bool { return v->IsArray(); },
                                              &MethodMan::argArray);
    }

    double argNumber(int i) const
    {
        auto maybe_arg = _args[i]->NumberValue(_args.GetIsolate()->GetCurrentContext());
        if (maybe_arg.IsNothing())
        {
            // FIXME(Marek): What should we do here?
            return {};
        }

        return maybe_arg.FromJust();
    }

    std::pair<double, bool> checkedArgNumber(int i) const
    {
        return checkArg<double>(i, [](v8::Local<v8::Value> v) -> bool { return v->IsNumber(); }, &MethodMan::argNumber);
    }

    v8::Local<v8::Date> argDate(int i) const
    {
        return v8::Local<v8::Date>::Cast(_args[i]);
    }

    std::pair<v8::Local<v8::Date>, bool> checkedArgDate(int i) const
    {
        return checkArg<v8::Local<v8::Date>>(i, [](v8::Local<v8::Value> v) -> bool { return v->IsDate(); },
                                             &MethodMan::argDate);
    }

    const v8::FunctionCallbackInfo<v8::Value> & args(void) const
    {
        return _args;
    }

    void throwException(const char * message) const
    {
        _isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(_isolate, message)));
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
    explicit ArgsEater(const MethodMan & meth) : _method(meth), _pos(0)
    {
    }

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
            return static_cast<qdb_time_t>(date.first->ValueOf());
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

    std::string convertString(const v8::Local<v8::String> & s)
    {
        v8::String::Utf8Value val(v8::Isolate::GetCurrent(), s);
        return std::string(*val, val.length());
    }

    std::string eatAndConvertString(void)
    {
        auto str = eatString();
        if (str.second) return convertString(str.first);

        return std::string();
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

            auto isolate = v8::Isolate::GetCurrent();

            for (auto i = 0u; i < len; ++i)
            {
                auto vi = arr.first->Get(i);
                if (!vi->IsString()) return string_vector();

                v8::String::Utf8Value val(v8::Isolate::GetCurrent(), vi->ToString(isolate));
                res.push_back(std::string(*val, val.length()));
            }
        }

        return res;
    }

    // Expected array of JS objects which has two properties:
    //	name - string, column name
    //	type - integer, column type
    std::vector<column_info> eatAndConvertColumnsInfoArray(void)
    {
        using column_vector = std::vector<column_info>;
        column_vector res;

        auto arr = eatArray();

        if (arr.second)
        {
            auto len = arr.first->Length();
            res.reserve(len);

            auto isolate = v8::Isolate::GetCurrent();

            auto nameProp = v8::String::NewFromUtf8(isolate, "name");
            auto typeProp = v8::String::NewFromUtf8(isolate, "type");

            for (auto i = 0u; i < len; ++i)
            {
                auto vi = arr.first->Get(i);
                if (!vi->IsObject()) return column_vector();

                auto maybe_obj = vi->ToObject(isolate->GetCurrentContext());
                if (maybe_obj.IsEmpty())
                {
                    return column_vector();
                }
                auto obj = maybe_obj.ToLocalChecked();
                auto name = obj->Get(nameProp);
                auto type = obj->Get(typeProp);

                if (!name->IsString() || !type->IsNumber())
                {
                    return column_vector();
                }

                v8::String::Utf8Value sval(isolate, name->ToString(isolate));

                auto maybe_type = type->Int32Value(isolate->GetCurrentContext());
                if (maybe_type.IsNothing())
                {
                    return column_vector();
                }

                res.emplace_back(std::string(*sval, sval.length()), qdb_ts_column_type_t(maybe_type.FromJust()));
            }
        }

        return res;
    }

    template <typename Type, typename Func>
    std::vector<Type> eatAndConvertPointsArray(Func f)
    {
        using point_vector = std::vector<Type>;
        point_vector res;

        auto arr = eatArray();
        if (arr.second)
        {
            auto len = arr.first->Length();
            res.reserve(len);

            auto tsProp = v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), "timestamp");
            auto valueProp = v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), "value");

            auto isolate = v8::Isolate::GetCurrent();
            auto context = isolate->GetCurrentContext();

            for (auto i = 0u; i < len; ++i)
            {
                auto vi = arr.first->Get(i);
                if (!vi->IsObject()) return point_vector();

                auto maybe_obj = vi->ToObject(isolate->GetCurrentContext());
                if (maybe_obj.IsEmpty()) return point_vector();

                auto obj = maybe_obj.ToLocalChecked();
                auto date = obj->Get(tsProp);
                auto value = obj->Get(valueProp);

                if (!Timestamp::InstanceOf(isolate, date)) return point_vector();

                auto maybe_timestamp = date->ToObject(context);
                if (maybe_timestamp.IsEmpty())
                {
                    return point_vector();
                }

                auto timestamp = node::ObjectWrap::Unwrap<Timestamp>(maybe_timestamp.ToLocalChecked());
                auto point = f(timestamp->getTimespec(), value);

                if (!point.second) return point_vector();
                res.push_back(point.first);
            }
        }

        return res;
    }

    std::vector<qdb_ts_double_point> eatAndConvertDoublePointsArray(void)
    {
        return eatAndConvertPointsArray<qdb_ts_double_point>([](qdb_timespec_t ts, v8::Local<v8::Value> value) {
            qdb_ts_double_point p;

            if (!value->IsNumber()) return std::make_pair(p, false);

            auto isolate = v8::Isolate::GetCurrent();

            p.timestamp = ts;

            auto maybe_value = value->NumberValue(isolate->GetCurrentContext());
            if (maybe_value.IsNothing())
            {
                return std::make_pair(p, false);
            }

            p.value = maybe_value.FromJust();
            return std::make_pair(p, true);
        });
    }

    std::vector<qdb_ts_blob_point> eatAndConvertBlobPointsArray(void)
    {
        return eatAndConvertPointsArray<qdb_ts_blob_point>([](qdb_timespec_t ts, v8::Local<v8::Value> value) {
            qdb_ts_blob_point p;

            if (!value->IsObject()) return std::make_pair(p, false);

            p.timestamp = ts;
            p.content = node::Buffer::Data(value);
            p.content_length = node::Buffer::Length(value);
            return std::make_pair(p, true);
        });
    }

    std::vector<qdb_ts_int64_point> eatAndConvertInt64PointsArray(void)
    {
        return eatAndConvertPointsArray<qdb_ts_int64_point>([](qdb_timespec_t ts, v8::Local<v8::Value> value) {
            qdb_ts_int64_point p;

            if (!value->IsNumber()) return std::make_pair(p, false);

            auto isolate = v8::Isolate::GetCurrent();

            p.timestamp = ts;

            auto maybe_value = value->IntegerValue(isolate->GetCurrentContext());
            if (maybe_value.IsNothing())
            {
                return std::make_pair(p, false);
            }

            p.value = maybe_value.FromJust();
            return std::make_pair(p, true);
        });
    }

    std::vector<qdb_ts_timestamp_point> eatAndConvertTimestampPointsArray(void)
    {
        return eatAndConvertPointsArray<qdb_ts_timestamp_point>([](qdb_timespec_t ts, v8::Local<v8::Value> value) {
            qdb_ts_timestamp_point p;
            auto isolate = v8::Isolate::GetCurrent();
            auto context = isolate->GetCurrentContext();

            if (!Timestamp::InstanceOf(isolate, value)) return std::make_pair(p, false);

            auto maybe_timestamp_value = value->ToObject(context);
            if (maybe_timestamp_value.IsEmpty())
            {
                return std::make_pair(p, false);
            }

            auto timestamp_value = node::ObjectWrap::Unwrap<Timestamp>(maybe_timestamp_value.ToLocalChecked());

            p.timestamp = ts;
            p.value = timestamp_value->getTimespec();

            return std::make_pair(p, true);
        });
    }

    std::vector<qdb_ts_range_t> eatAndConvertRangeArray(void)
    {
        using range_vector = std::vector<qdb_ts_range_t>;
        range_vector res;

        auto arr = eatArray();
        if (arr.second)
        {
            auto len = arr.first->Length();
            res.reserve(len);

            auto isolate = v8::Isolate::GetCurrent();

            for (auto i = 0u; i < len; ++i)
            {
                auto vi = arr.first->Get(i);
                if (!vi->IsObject()) return range_vector();

                auto maybe_obj = vi->ToObject(isolate->GetCurrentContext());
                if (maybe_obj.IsEmpty()) return range_vector();

                auto obj = node::ObjectWrap::Unwrap<TsRange>(maybe_obj.ToLocalChecked());
                assert(obj);

                res.push_back(obj->nativeRange());
            }
        }

        return res;
    }

    template <typename Type>
    std::vector<Type> eatAndConvertAggrArray(void)
    {
        using aggr_vector = std::vector<Type>;
        aggr_vector res;

        auto arr = eatArray();
        if (arr.second)
        {
            auto len = arr.first->Length();
            res.reserve(len);

            auto isolate = v8::Isolate::GetCurrent();

            for (auto i = 0u; i < len; ++i)
            {
                auto vi = arr.first->Get(i);
                if (!vi->IsObject()) return aggr_vector();

                auto maybe_obj = vi->ToObject(isolate->GetCurrentContext());
                if (maybe_obj.IsEmpty()) return aggr_vector();

                auto obj = maybe_obj.ToLocalChecked();

                auto aggr = node::ObjectWrap::Unwrap<Aggregation>(obj);
                assert(aggr);

                Type item;
                item.type = aggr->nativeType();
                item.range = aggr->nativeRange();
                item.count = 0;

                res.push_back(item);
            }
        }

        return res;
    }

    v8::Local<v8::Object> eatHolder(void)
    {
        return _method.holder();
    }

    std::string eatAndConvertTsAlias(void)
    {
        auto holder = _method.holder();
        auto tsProp = v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), "timeseries");

        auto ts = holder->Get(tsProp);
        if (!ts->IsString()) return std::string();

        auto isolate = v8::Isolate::GetCurrent();
        return convertString(ts->ToString(isolate));
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
    explicit ArgsEaterBinder(const MethodMan & meth) : _eater(meth)
    {
    }

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

    qdb_request & columnsInfo(qdb_request & req)
    {
        req.input.content.columns = _eater.eatAndConvertColumnsInfoArray();
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

    qdb_request & holder(qdb_request & req)
    {
        req.holder.Reset(v8::Isolate::GetCurrent(), _eater.eatHolder());
        return req;
    }

    qdb_request & doublePoints(qdb_request & req)
    {
        req.input.content.double_points = _eater.eatAndConvertDoublePointsArray();
        return req;
    }

    qdb_request & blobPoints(qdb_request & req)
    {
        req.input.content.blob_points = _eater.eatAndConvertBlobPointsArray();
        return req;
    }

    qdb_request & int64Points(qdb_request & req)
    {
        req.input.content.int64_points = _eater.eatAndConvertInt64PointsArray();
        return req;
    }

    qdb_request & timestampPoints(qdb_request & req)
    {
        req.input.content.timestamp_points = _eater.eatAndConvertTimestampPointsArray();
        return req;
    }

    qdb_request & ranges(qdb_request & req)
    {
        req.input.content.ranges = _eater.eatAndConvertRangeArray();
        return req;
    }

    qdb_request & blobAggregations(qdb_request & req)
    {
        req.input.content.blob_aggrs = _eater.eatAndConvertAggrArray<qdb_ts_blob_aggregation_t>();
        return req;
    }

    qdb_request & doubleAggregations(qdb_request & req)
    {
        req.input.content.double_aggrs = _eater.eatAndConvertAggrArray<qdb_ts_double_aggregation_t>();
        return req;
    }

    qdb_request & int64Aggregations(qdb_request & req)
    {
        req.input.content.int64_aggrs = _eater.eatAndConvertAggrArray<qdb_ts_int64_aggregation_t>();
        return req;
    }

    qdb_request & timestampAggregations(qdb_request & req)
    {
        req.input.content.timestamp_aggrs = _eater.eatAndConvertAggrArray<qdb_ts_timestamp_aggregation_t>();
        return req;
    }

    qdb_request & tsAlias(qdb_request & req)
    {
        req.input.content.str = _eater.eatAndConvertTsAlias();
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

template <size_t ArgsLength>
struct ArgumentsCopier;

template <>
struct ArgumentsCopier<1>
{
    static inline std::array<v8::Local<v8::Value>, 1> copy(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        return {{args[0]}};
    }
};

template <>
struct ArgumentsCopier<2>
{
    static inline std::array<v8::Local<v8::Value>, 2> copy(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        return {{args[0], args[1]}};
    }
};

} // namespace quasardb
