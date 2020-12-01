#include "utilities.hpp"

namespace quasardb
{
namespace detail
{

void AddConstantProperty(v8::Isolate * isolate,
                         v8::Local<v8::Object> object,
                         const char * key,
                         v8::Local<v8::Value> value)
{
    // object->ForceSet(v8::String::NewFromUtf8(isolate, key, v8::NewStringType::kNormal).ToLocalChecked(), value,
    //                 static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontDelete));

    v8::Maybe<bool> maybe =
        object->DefineOwnProperty(isolate->GetCurrentContext(),
                                  v8::String::NewFromUtf8(isolate, key, v8::NewStringType::kNormal).ToLocalChecked(),
                                  value, static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontDelete));
    (void)maybe; // unused
    assert(maybe.IsJust() && maybe.FromJust());
}

inline void release_node_buffer(char * data, void * hint)
{
    if (data && hint)
    {
        qdb_release(static_cast<qdb_handle_t>(hint), data);
    }
}

} // namespace detail

v8::MaybeLocal<v8::Object> qdb_request::make_node_buffer(v8::Isolate * isolate, const void * buf, size_t length)
{
    qdb_handle_t h = handle();

    if (!h || !buf || !length)
    {
        return node::Buffer::New(isolate, static_cast<size_t>(0u));
    }

    return node::Buffer::New(isolate, static_cast<char *>(const_cast<void *>(buf)), length, detail::release_node_buffer,
                             h);
}

qdb_time_t ArgsEater::eatAndConvertDate()
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

template <typename Type, typename Func>
std::vector<Type> eatAndConvertArray(ArgsEater & eater, Func convert)
{
    using typed_vector = std::vector<Type>;

    auto arr = eater.eatArray();
    if (!arr.second) return {};

    auto len = arr.first->Length();
    typed_vector res;
    res.reserve(len);

    auto isolate = v8::Isolate::GetCurrent();

    for (auto i = 0u; i < len; ++i)
    {
        auto vi = arr.first->Get(isolate->GetCurrentContext(), i).ToLocalChecked();
        auto point = convert(vi);
        if (!point.second) return {};

        res.push_back(std::move(point.first));
    }

    return res;
}

std::vector<std::string> ArgsEater::eatAndConvertStringArray()
{
    auto isolate = v8::Isolate::GetCurrent();

    return eatAndConvertArray<std::string>(*this, [&](v8::Local<v8::Value> vi) {
        if (!vi->IsString()) return std::make_pair(std::string{}, false);

        v8::String::Utf8Value val(isolate, vi->ToString(isolate->GetCurrentContext()).ToLocalChecked());
        return std::make_pair(std::string(*val, val.length()), true);
    });
}

std::vector<column_info> ArgsEater::eatAndConvertColumnsInfoArray()
{
    auto isolate = v8::Isolate::GetCurrent();

    auto nameProp = v8::String::NewFromUtf8(isolate, "name", v8::NewStringType::kNormal).ToLocalChecked();
    auto typeProp = v8::String::NewFromUtf8(isolate, "type", v8::NewStringType::kNormal).ToLocalChecked();
    auto symtableProp = v8::String::NewFromUtf8(isolate, "symtable", v8::NewStringType::kNormal).ToLocalChecked();

    return eatAndConvertArray<column_info>(*this, [&](v8::Local<v8::Value> vi) {
        if (!vi->IsObject()) return std::make_pair(column_info{}, false);

        auto maybe_obj = vi->ToObject(isolate->GetCurrentContext());
        if (maybe_obj.IsEmpty()) return std::make_pair(column_info{}, false);

        auto obj = maybe_obj.ToLocalChecked();
        auto name = obj->Get(isolate->GetCurrentContext(), nameProp).ToLocalChecked();
        auto type = obj->Get(isolate->GetCurrentContext(), typeProp).ToLocalChecked();

        if (!name->IsString() || !type->IsNumber())
        {
            return std::make_pair(column_info{}, false);
        }

        auto name_val = v8::String::Utf8Value(isolate, name->ToString(isolate->GetCurrentContext()).ToLocalChecked());
        auto maybe_type = type->Int32Value(isolate->GetCurrentContext());
        if (maybe_type.IsNothing())
        {
            return std::make_pair(column_info{}, false);
        }

        column_info col;
        col.name = {*name_val, static_cast<size_t>(name_val.length())};
        col.type = static_cast<qdb_ts_column_type_t>(maybe_type.FromJust());

        auto symtable = obj->Get(isolate->GetCurrentContext(), symtableProp).ToLocalChecked();
        if (symtable->IsNull() || symtable->IsUndefined())
        {
            return std::make_pair(std::move(col), true);
        }
        if (!symtable->IsString())
        {
            return std::make_pair(column_info{}, false);
        }

        auto symtable_val = v8::String::Utf8Value(isolate, name->ToString(isolate->GetCurrentContext()).ToLocalChecked());
        col.symtable = {*symtable_val, static_cast<size_t>(symtable_val.length())};
        return std::make_pair(std::move(col), true);
    });
}

template <typename Type, typename Func>
std::vector<Type> eatAndConvertPointsArray(ArgsEater & eater, Func convert)
{
    using point_vector = std::vector<Type>;

    auto arr = eater.eatArray();
    if (!arr.second) return {};

    auto len = arr.first->Length();
    point_vector res;
    res.reserve(len);

    auto isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();

    auto tsProp = v8::String::NewFromUtf8(isolate, "timestamp", v8::NewStringType::kNormal).ToLocalChecked();
    auto valueProp = v8::String::NewFromUtf8(isolate, "value", v8::NewStringType::kNormal).ToLocalChecked();

    for (auto i = 0u; i < len; ++i)
    {
        auto vi = arr.first->Get(context, i).ToLocalChecked();
        if (!vi->IsObject()) return {};

        auto maybe_obj = vi->ToObject(isolate->GetCurrentContext());
        if (maybe_obj.IsEmpty()) return {};

        auto obj = maybe_obj.ToLocalChecked();
        auto date = obj->Get(context, tsProp).ToLocalChecked();
        auto value = obj->Get(context, valueProp).ToLocalChecked();

        if (!Timestamp::InstanceOf(isolate, date)) return {};

        auto maybe_timestamp = date->ToObject(context);
        if (maybe_timestamp.IsEmpty()) return {};

        auto timestamp = node::ObjectWrap::Unwrap<Timestamp>(maybe_timestamp.ToLocalChecked());
        auto point = convert(timestamp->getTimespec(), value);
        if (!point.second) return {};

        res.push_back(std::move(point.first));
    }

    return res;
}

static bool InstanceOfBuffer(v8::Isolate * isolate, v8::Local<v8::Value> val)
{
    auto context = isolate->GetCurrentContext();
    auto maybe_obj = val->ToObject(context);
    if (maybe_obj.IsEmpty())
    {
        return false;
    }

    auto obj = maybe_obj.ToLocalChecked();
    return node::Buffer::HasInstance(obj);
}

std::vector<qdb_ts_blob_point> ArgsEater::eatAndConvertBlobPointsArray()
{
    return eatAndConvertPointsArray<qdb_ts_blob_point>(*this, [&](qdb_timespec_t ts, v8::Local<v8::Value> value) {
        qdb_ts_blob_point p;
        auto isolate = v8::Isolate::GetCurrent();

        if (!InstanceOfBuffer(isolate, value)) return std::make_pair(p, false);

        p.timestamp = ts;
        p.content = node::Buffer::Data(value);
        p.content_length = node::Buffer::Length(value);
        return std::make_pair(p, true);
    });
}

std::vector<qdb_ts_string_point> ArgsEater::eatAndConvertStringPointsArray()
{
    return eatAndConvertPointsArray<qdb_ts_string_point>(*this, [&](qdb_timespec_t ts, v8::Local<v8::Value> value) {
        qdb_ts_string_point p;
        auto isolate = v8::Isolate::GetCurrent();

        if (!InstanceOfBuffer(isolate, value)) return std::make_pair(p, false);

        p.timestamp = ts;
        p.content = node::Buffer::Data(value);
        p.content_length = node::Buffer::Length(value);
        return std::make_pair(p, true);
    });
}

std::vector<qdb_ts_double_point> ArgsEater::eatAndConvertDoublePointsArray()
{
    return eatAndConvertPointsArray<qdb_ts_double_point>(*this, [](qdb_timespec_t ts, v8::Local<v8::Value> value) {
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

std::vector<qdb_ts_int64_point> ArgsEater::eatAndConvertInt64PointsArray()
{
    return eatAndConvertPointsArray<qdb_ts_int64_point>(*this, [](qdb_timespec_t ts, v8::Local<v8::Value> value) {
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

std::vector<qdb_ts_timestamp_point> ArgsEater::eatAndConvertTimestampPointsArray()
{
    return eatAndConvertPointsArray<qdb_ts_timestamp_point>(*this, [](qdb_timespec_t ts, v8::Local<v8::Value> value) {
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

std::vector<qdb_ts_symbol_point> ArgsEater::eatAndConvertSymbolPointsArray()
{
    return eatAndConvertPointsArray<qdb_ts_symbol_point>(*this, [&](qdb_timespec_t ts, v8::Local<v8::Value> value) {
        qdb_ts_symbol_point p;
        auto isolate = v8::Isolate::GetCurrent();

        if (!InstanceOfBuffer(isolate, value)) return std::make_pair(p, false);

        p.timestamp = ts;
        p.content = node::Buffer::Data(value);
        p.content_length = node::Buffer::Length(value);
        return std::make_pair(p, true);
    });
}

std::vector<qdb_ts_range_t> ArgsEater::eatAndConvertRangeArray()
{
    auto isolate = v8::Isolate::GetCurrent();

    return eatAndConvertArray<qdb_ts_range_t>(*this, [&](v8::Local<v8::Value> vi) {
        if (!vi->IsObject()) return std::make_pair(qdb_ts_range_t{}, false);

        auto maybe_obj = vi->ToObject(isolate->GetCurrentContext());
        if (maybe_obj.IsEmpty()) return std::make_pair(qdb_ts_range_t{}, false);

        auto obj = node::ObjectWrap::Unwrap<TsRange>(maybe_obj.ToLocalChecked());
        assert(obj);

        return std::make_pair(obj->nativeRange(), true);
    });
}

std::string ArgsEater::eatAndConvertTsAlias()
{
    auto holder = _method.holder();
    auto isolate = v8::Isolate::GetCurrent();
    auto tsProp = v8::String::NewFromUtf8(isolate, "timeseries", v8::NewStringType::kNormal).ToLocalChecked();

    auto ts = holder->Get(isolate->GetCurrentContext(), tsProp).ToLocalChecked();
    if (!ts->IsString()) return std::string();

    return convertString(ts->ToString(isolate->GetCurrentContext()).ToLocalChecked());
}

qdb_request::slice ArgsEater::eatAndConvertBuffer()
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

} // namespace quasardb
