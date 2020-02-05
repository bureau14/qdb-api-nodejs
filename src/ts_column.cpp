#include "ts_column.hpp"
#include "entry.hpp"
#include "error.hpp"
#include "time_series.hpp"
#include "ts_point.hpp"
#include <qdb/ts.h>

namespace quasardb
{

v8::Persistent<v8::Function> BlobColumn::constructor;
v8::Persistent<v8::Function> StringColumn::constructor;
v8::Persistent<v8::Function> DoubleColumn::constructor;
v8::Persistent<v8::Function> Int64Column::constructor;
v8::Persistent<v8::Function> TimestampColumn::constructor;

void BlobColumn::insert(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    Column<BlobColumn>::queue_work(
        args,
        [](qdb_request * qdb_req) {
            const auto alias = qdb_req->input.alias.c_str();
            const auto ts = qdb_req->input.content.str.c_str();
            const auto & points = qdb_req->input.content.blob_points;

            // FIXME(Marek): It's a poor man's hack, because ArgsEaterBinder::blobPoints returns an empty collection
            // when an incorrect input has been given. But C API accepts 0-sized inputs.
            if (points.empty())
            {
                qdb_req->output.error = qdb_e_invalid_argument;
            }
            else
            {
                qdb_req->output.error = qdb_ts_blob_insert(qdb_req->handle(), ts, alias, points.data(), points.size());
            }
        },
        Entry<Column>::processVoidResult, &ArgsEaterBinder::tsAlias, &ArgsEaterBinder::blobPoints);
}

void BlobColumn::ranges(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    BlobColumn::queue_work(
        args,
        [](qdb_request * qdb_req) {
            const auto alias = qdb_req->input.alias.c_str();
            const auto ts = qdb_req->input.content.str.c_str();
            auto & ranges = qdb_req->input.content.ranges;
            auto bufp =
                reinterpret_cast<qdb_ts_blob_point **>(const_cast<void **>(&(qdb_req->output.content.buffer.begin)));
            auto count = &(qdb_req->output.content.buffer.size);

            qdb_req->output.error =
                qdb_ts_blob_get_ranges(qdb_req->handle(), ts, alias, ranges.data(), ranges.size(), bufp, count);
        },
        BlobColumn::processBlobPointArrayResult, &ArgsEaterBinder::tsAlias, &ArgsEaterBinder::ranges);
}

void BlobColumn::aggregate(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    BlobColumn::queue_work(
        args,
        [](qdb_request * qdb_req) {
            const auto alias = qdb_req->input.alias.c_str();
            const auto ts = qdb_req->input.content.str.c_str();
            qdb_ts_blob_aggregation_t * aggrs = qdb_req->input.content.blob_aggrs.data();
            const qdb_size_t count = qdb_req->input.content.blob_aggrs.size();

            qdb_req->output.error = qdb_ts_blob_aggregate(qdb_req->handle(), ts, alias, aggrs, count);
        },
        BlobColumn::processBlobAggregateResult, &ArgsEaterBinder::tsAlias, &ArgsEaterBinder::blobAggregations);
}


void StringColumn::insert(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    Column<StringColumn>::queue_work(
        args,
        [](qdb_request * qdb_req) {
            const auto alias = qdb_req->input.alias.c_str();
            const auto ts = qdb_req->input.content.str.c_str();
            const auto & points = qdb_req->input.content.string_points;

            // FIXME(Marek): It's a poor man's hack, because ArgsEaterBinder::stringPoints returns an empty collection
            // when an incorrect input has been given. But C API accepts 0-sized inputs.
            if (points.empty())
            {
                qdb_req->output.error = qdb_e_invalid_argument;
            }
            else
            {
                qdb_req->output.error = qdb_ts_string_insert(qdb_req->handle(), ts, alias, points.data(), points.size());
            }
        },
        Entry<Column>::processVoidResult, &ArgsEaterBinder::tsAlias, &ArgsEaterBinder::stringPoints);
}

void StringColumn::ranges(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    StringColumn::queue_work(
        args,
        [](qdb_request * qdb_req) {
            const auto alias = qdb_req->input.alias.c_str();
            const auto ts = qdb_req->input.content.str.c_str();
            auto & ranges = qdb_req->input.content.ranges;
            auto bufp =
                reinterpret_cast<qdb_ts_string_point **>(const_cast<void **>(&(qdb_req->output.content.buffer.begin)));
            auto count = &(qdb_req->output.content.buffer.size);

            qdb_req->output.error =
                qdb_ts_string_get_ranges(qdb_req->handle(), ts, alias, ranges.data(), ranges.size(), bufp, count);
        },
        StringColumn::processStringPointArrayResult, &ArgsEaterBinder::tsAlias, &ArgsEaterBinder::ranges);
}

void StringColumn::aggregate(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    StringColumn::queue_work(
        args,
        [](qdb_request * qdb_req) {
            const auto alias = qdb_req->input.alias.c_str();
            const auto ts = qdb_req->input.content.str.c_str();
            qdb_ts_string_aggregation_t * aggrs = qdb_req->input.content.string_aggrs.data();
            const qdb_size_t count = qdb_req->input.content.string_aggrs.size();

            qdb_req->output.error = qdb_ts_string_aggregate(qdb_req->handle(), ts, alias, aggrs, count);
        },
        StringColumn::processStringAggregateResult, &ArgsEaterBinder::tsAlias, &ArgsEaterBinder::stringAggregations);
}

void DoubleColumn::insert(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    Column<DoubleColumn>::queue_work(args,
                                     [](qdb_request * qdb_req) {
                                         const auto alias = qdb_req->input.alias.c_str();
                                         const auto ts = qdb_req->input.content.str.c_str();
                                         const auto & points = qdb_req->input.content.double_points;

                                         // FIXME(Marek): It's a poor man's hack, because ArgsEaterBinder::blobPoints
                                         // returns an empty collection when an incorrect input has been given. But C
                                         // API accepts 0-sized inputs.
                                         if (points.empty())
                                         {
                                             qdb_req->output.error = qdb_e_invalid_argument;
                                         }
                                         else
                                         {
                                             qdb_req->output.error = qdb_ts_double_insert(qdb_req->handle(), ts, alias,
                                                                                          points.data(), points.size());
                                         }
                                     },
                                     Entry<Column>::processVoidResult, &ArgsEaterBinder::tsAlias,
                                     &ArgsEaterBinder::doublePoints);
}

void DoubleColumn::ranges(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    DoubleColumn::queue_work(
        args,
        [](qdb_request * qdb_req) {
            const auto alias = qdb_req->input.alias.c_str();
            const auto ts = qdb_req->input.content.str.c_str();
            auto & ranges = qdb_req->input.content.ranges;
            auto bufp =
                reinterpret_cast<qdb_ts_double_point **>(const_cast<void **>(&(qdb_req->output.content.buffer.begin)));
            auto count = &(qdb_req->output.content.buffer.size);

            qdb_req->output.error =
                qdb_ts_double_get_ranges(qdb_req->handle(), ts, alias, ranges.data(), ranges.size(), bufp, count);
        },
        DoubleColumn::processDoublePointArrayResult, &ArgsEaterBinder::tsAlias, &ArgsEaterBinder::ranges);
}

void DoubleColumn::aggregate(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    DoubleColumn::queue_work(
        args,
        [](qdb_request * qdb_req) {
            const auto alias = qdb_req->input.alias.c_str();
            const auto ts = qdb_req->input.content.str.c_str();
            qdb_ts_double_aggregation_t * aggrs = qdb_req->input.content.double_aggrs.data();
            const qdb_size_t count = qdb_req->input.content.double_aggrs.size();

            qdb_req->output.error = qdb_ts_double_aggregate(qdb_req->handle(), ts, alias, aggrs, count);
        },
        DoubleColumn::processDoubleAggregateResult, &ArgsEaterBinder::tsAlias, &ArgsEaterBinder::doubleAggregations);
}

void BlobColumn::processBlobPointArrayResult(uv_work_t * req, int status)
{
    processResult<2>(req, status, [&](v8::Isolate * isolate, qdb_request * qdb_req) {
        v8::Local<v8::Array> array;

        auto error_code = processErrorCode(isolate, status, qdb_req);
        if ((qdb_req->output.error == qdb_e_ok) && (status >= 0))
        {
            qdb_ts_blob_point * entries =
                reinterpret_cast<qdb_ts_blob_point *>(const_cast<void *>(qdb_req->output.content.buffer.begin));
            const size_t entries_count = qdb_req->output.content.buffer.size;

            array = v8::Array::New(isolate, static_cast<int>(entries_count));
            if (array.IsEmpty())
            {
                error_code = Error::MakeError(isolate, qdb_e_no_memory_local);
            }
            else
            {
                for (size_t i = 0; i < entries_count; ++i)
                {
                    auto obj = BlobPoint::MakePointWithCopy(isolate, entries[i].timestamp, entries[i].content,
                                                            entries[i].content_length);

                    if (!obj.IsEmpty()) array->Set(isolate->GetCurrentContext(), static_cast<uint32_t>(i), obj);
                }
            }

            // safe to call even on null/invalid buffers
            qdb_release(qdb_req->handle(), entries);
        }
        else
        {
            // provide an empty array
            array = v8::Array::New(isolate, 0);
        }

        return make_value_array(error_code, array);
    });
}

void BlobColumn::processBlobAggregateResult(uv_work_t * req, int status)
{
    processResult<2>(req, status, [&](v8::Isolate * isolate, qdb_request * qdb_req) {
        v8::Local<v8::Array> array;

        auto error_code = processErrorCode(isolate, status, qdb_req);
        if ((qdb_req->output.error == qdb_e_ok) && (status >= 0))
        {
            const auto & aggrs = qdb_req->input.content.blob_aggrs;
            array = v8::Array::New(isolate, static_cast<int>(aggrs.size()));

            if (array.IsEmpty())
            {
                error_code = Error::MakeError(isolate, qdb_e_no_memory_local);
            }
            else
            {
                auto resprop = v8::String::NewFromUtf8(isolate, "result", v8::NewStringType::kNormal).ToLocalChecked();
                auto cntprop = v8::String::NewFromUtf8(isolate, "count", v8::NewStringType::kNormal).ToLocalChecked();

                for (size_t i = 0; i < aggrs.size(); ++i)
                {
                    const auto & point = aggrs[i].result;
                    auto result =
                        BlobPoint::MakePointWithCopy(isolate, point.timestamp, point.content, point.content_length);

                    auto obj = v8::Object::New(isolate);
                    obj->Set(isolate->GetCurrentContext(), resprop, result);
                    obj->Set(isolate->GetCurrentContext(), cntprop,
                             v8::Number::New(isolate, static_cast<double>(aggrs[i].count)));
                    if (!obj.IsEmpty()) array->Set(isolate->GetCurrentContext(), static_cast<uint32_t>(i), obj);

                    // safe to call even on null/invalid buffers
                    qdb_release(qdb_req->handle(), point.content);
                }
            }
        }
        else
        {
            // provide an empty array
            array = v8::Array::New(isolate, 0);
        }

        return make_value_array(error_code, array);
    });
}


void StringColumn::processStringPointArrayResult(uv_work_t * req, int status)
{
    processResult<2>(req, status, [&](v8::Isolate * isolate, qdb_request * qdb_req) {
        v8::Local<v8::Array> array;

        auto error_code = processErrorCode(isolate, status, qdb_req);
        if ((qdb_req->output.error == qdb_e_ok) && (status >= 0))
        {
            qdb_ts_string_point * entries =
                reinterpret_cast<qdb_ts_string_point *>(const_cast<void *>(qdb_req->output.content.buffer.begin));
            const size_t entries_count = qdb_req->output.content.buffer.size;

            array = v8::Array::New(isolate, static_cast<int>(entries_count));
            if (array.IsEmpty())
            {
                error_code = Error::MakeError(isolate, qdb_e_no_memory_local);
            }
            else
            {
                for (size_t i = 0; i < entries_count; ++i)
                {
                    auto obj = StringPoint::MakePointWithCopy(isolate, entries[i].timestamp, entries[i].content,
                                                            entries[i].content_length);

                    if (!obj.IsEmpty()) array->Set(isolate->GetCurrentContext(), static_cast<uint32_t>(i), obj);
                }
            }

            // safe to call even on null/invalid buffers
            qdb_release(qdb_req->handle(), entries);
        }
        else
        {
            // provide an empty array
            array = v8::Array::New(isolate, 0);
        }

        return make_value_array(error_code, array);
    });
}

void StringColumn::processStringAggregateResult(uv_work_t * req, int status)
{
    processResult<2>(req, status, [&](v8::Isolate * isolate, qdb_request * qdb_req) {
        v8::Local<v8::Array> array;

        auto error_code = processErrorCode(isolate, status, qdb_req);
        if ((qdb_req->output.error == qdb_e_ok) && (status >= 0))
        {
            const auto & aggrs = qdb_req->input.content.string_aggrs;
            array = v8::Array::New(isolate, static_cast<int>(aggrs.size()));

            if (array.IsEmpty())
            {
                error_code = Error::MakeError(isolate, qdb_e_no_memory_local);
            }
            else
            {
                auto resprop = v8::String::NewFromUtf8(isolate, "result", v8::NewStringType::kNormal).ToLocalChecked();
                auto cntprop = v8::String::NewFromUtf8(isolate, "count", v8::NewStringType::kNormal).ToLocalChecked();

                for (size_t i = 0; i < aggrs.size(); ++i)
                {
                    const auto & point = aggrs[i].result;
                    auto result =
                        StringPoint::MakePointWithCopy(isolate, point.timestamp, point.content, point.content_length);

                    auto obj = v8::Object::New(isolate);
                    obj->Set(isolate->GetCurrentContext(), resprop, result);
                    obj->Set(isolate->GetCurrentContext(), cntprop,
                             v8::Number::New(isolate, static_cast<double>(aggrs[i].count)));
                    if (!obj.IsEmpty()) array->Set(isolate->GetCurrentContext(), static_cast<uint32_t>(i), obj);

                    // safe to call even on null/invalid buffers
                    qdb_release(qdb_req->handle(), point.content);
                }
            }
        }
        else
        {
            // provide an empty array
            array = v8::Array::New(isolate, 0);
        }

        return make_value_array(error_code, array);
    });
}

void DoubleColumn::processDoublePointArrayResult(uv_work_t * req, int status)
{
    processResult<2>(req, status, [&](v8::Isolate * isolate, qdb_request * qdb_req) {
        v8::Local<v8::Array> array;

        auto error_code = processErrorCode(isolate, status, qdb_req);
        if ((qdb_req->output.error == qdb_e_ok) && (status >= 0))
        {
            qdb_ts_double_point * entries =
                reinterpret_cast<qdb_ts_double_point *>(const_cast<void *>(qdb_req->output.content.buffer.begin));
            const size_t entries_count = qdb_req->output.content.buffer.size;

            array = v8::Array::New(isolate, static_cast<int>(entries_count));
            if (array.IsEmpty())
            {
                error_code = Error::MakeError(isolate, qdb_e_no_memory_local);
            }
            else
            {
                for (size_t i = 0; i < entries_count; ++i)
                {
                    auto obj = DoublePoint::MakePoint(isolate, entries[i].timestamp, entries[i].value);
                    if (!obj.IsEmpty()) array->Set(isolate->GetCurrentContext(), static_cast<uint32_t>(i), obj);
                }
            }

            // safe to call even on null/invalid buffers
            qdb_release(qdb_req->handle(), entries);
        }
        else
        {
            // provide an empty array
            array = v8::Array::New(isolate, 0);
        }

        return make_value_array(error_code, array);
    });
}

void DoubleColumn::processDoubleAggregateResult(uv_work_t * req, int status)
{
    processResult<2>(req, status, [&](v8::Isolate * isolate, qdb_request * qdb_req) {
        v8::Local<v8::Array> array;

        auto error_code = processErrorCode(isolate, status, qdb_req);
        if ((qdb_req->output.error == qdb_e_ok) && (status >= 0))
        {
            const auto & aggrs = qdb_req->input.content.double_aggrs;
            array = v8::Array::New(isolate, static_cast<int>(aggrs.size()));
            if (array.IsEmpty())
            {
                error_code = Error::MakeError(isolate, qdb_e_no_memory_local);
            }
            else
            {
                auto resprop = v8::String::NewFromUtf8(isolate, "result", v8::NewStringType::kNormal).ToLocalChecked();
                auto cntprop = v8::String::NewFromUtf8(isolate, "count", v8::NewStringType::kNormal).ToLocalChecked();

                for (size_t i = 0; i < aggrs.size(); ++i)
                {
                    const auto & point = aggrs[i].result;
                    auto result = DoublePoint::MakePoint(isolate, point.timestamp, point.value);

                    auto obj = v8::Object::New(isolate);
                    obj->Set(isolate->GetCurrentContext(), resprop, result);
                    obj->Set(isolate->GetCurrentContext(), cntprop,
                             v8::Number::New(isolate, static_cast<double>(aggrs[i].count)));

                    if (!obj.IsEmpty()) array->Set(isolate->GetCurrentContext(), static_cast<uint32_t>(i), obj);
                }
            }
        }
        else
        {
            // provide an empty array
            array = v8::Array::New(isolate, 0);
        }

        return make_value_array(error_code, array);
    });
}

void Int64Column::insert(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    Column<Int64Column>::queue_work(
        args,
        [](qdb_request * qdb_req) {
            const auto alias = qdb_req->input.alias.c_str();
            const auto ts = qdb_req->input.content.str.c_str();
            const auto & points = qdb_req->input.content.int64_points;

            // FIXME(Marek): It's a poor man's hack, because ArgsEaterBinder::blobPoints returns an empty collection
            // when an incorrect input has been given. But C API accepts 0-sized inputs.
            if (points.empty())
            {
                qdb_req->output.error = qdb_e_invalid_argument;
            }
            else
            {
                qdb_req->output.error = qdb_ts_int64_insert(qdb_req->handle(), ts, alias, points.data(), points.size());
            }
        },
        Entry<Column>::processVoidResult, &ArgsEaterBinder::tsAlias, &ArgsEaterBinder::int64Points);
}

void Int64Column::ranges(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    Int64Column::queue_work(
        args,
        [](qdb_request * qdb_req) {
            const auto alias = qdb_req->input.alias.c_str();
            const auto ts = qdb_req->input.content.str.c_str();
            auto & ranges = qdb_req->input.content.ranges;
            auto bufp =
                reinterpret_cast<qdb_ts_int64_point **>(const_cast<void **>(&(qdb_req->output.content.buffer.begin)));
            auto count = &(qdb_req->output.content.buffer.size);

            qdb_req->output.error =
                qdb_ts_int64_get_ranges(qdb_req->handle(), ts, alias, ranges.data(), ranges.size(), bufp, count);
        },
        Int64Column::processInt64PointArrayResult, &ArgsEaterBinder::tsAlias, &ArgsEaterBinder::ranges);
}

void Int64Column::aggregate(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    Int64Column::queue_work(
        args,
        [](qdb_request * qdb_req) {
            const auto alias = qdb_req->input.alias.c_str();
            const auto ts = qdb_req->input.content.str.c_str();
            qdb_ts_int64_aggregation_t * aggrs = qdb_req->input.content.int64_aggrs.data();
            const qdb_size_t count = qdb_req->input.content.int64_aggrs.size();

            qdb_req->output.error = qdb_ts_int64_aggregate(qdb_req->handle(), ts, alias, aggrs, count);
        },
        Int64Column::processInt64AggregateResult, &ArgsEaterBinder::tsAlias, &ArgsEaterBinder::int64Aggregations);
}

void Int64Column::processInt64PointArrayResult(uv_work_t * req, int status)
{
    processResult<2>(req, status, [&](v8::Isolate * isolate, qdb_request * qdb_req) {
        v8::Local<v8::Array> array;

        auto error_code = processErrorCode(isolate, status, qdb_req);
        if ((qdb_req->output.error == qdb_e_ok) && (status >= 0))
        {
            qdb_ts_int64_point * entries =
                reinterpret_cast<qdb_ts_int64_point *>(const_cast<void *>(qdb_req->output.content.buffer.begin));
            const size_t entries_count = qdb_req->output.content.buffer.size;

            array = v8::Array::New(isolate, static_cast<int>(entries_count));
            if (array.IsEmpty())
            {
                error_code = Error::MakeError(isolate, qdb_e_no_memory_local);
            }
            else
            {
                for (size_t i = 0; i < entries_count; ++i)
                {
                    auto obj = Int64Point::MakePoint(isolate, entries[i].timestamp, entries[i].value);
                    if (!obj.IsEmpty()) array->Set(isolate->GetCurrentContext(), static_cast<uint32_t>(i), obj);
                }
            }

            // safe to call even on null/invalid buffers
            qdb_release(qdb_req->handle(), entries);
        }
        else
        {
            // provide an empty array
            array = v8::Array::New(isolate, 0);
        }

        return make_value_array(error_code, array);
    });
}

void Int64Column::processInt64AggregateResult(uv_work_t * req, int status)
{
    processResult<2>(req, status, [&](v8::Isolate * isolate, qdb_request * qdb_req) {
        v8::Local<v8::Array> array;

        auto error_code = processErrorCode(isolate, status, qdb_req);
        if ((qdb_req->output.error == qdb_e_ok) && (status >= 0))
        {
            const auto & aggrs = qdb_req->input.content.int64_aggrs;
            array = v8::Array::New(isolate, static_cast<int>(aggrs.size()));
            if (array.IsEmpty())
            {
                error_code = Error::MakeError(isolate, qdb_e_no_memory_local);
            }
            else
            {
                auto resprop = v8::String::NewFromUtf8(isolate, "result", v8::NewStringType::kNormal).ToLocalChecked();
                auto cntprop = v8::String::NewFromUtf8(isolate, "count", v8::NewStringType::kNormal).ToLocalChecked();

                for (size_t i = 0; i < aggrs.size(); ++i)
                {
                    const auto & point = aggrs[i].result;
                    auto result = Int64Point::MakePoint(isolate, point.timestamp, point.value);

                    auto obj = v8::Object::New(isolate);
                    obj->Set(isolate->GetCurrentContext(), resprop, result);
                    obj->Set(isolate->GetCurrentContext(), cntprop,
                             v8::Number::New(isolate, static_cast<double>(aggrs[i].count)));

                    if (!obj.IsEmpty()) array->Set(isolate->GetCurrentContext(), static_cast<uint32_t>(i), obj);
                }
            }
        }
        else
        {
            // provide an empty array
            array = v8::Array::New(isolate, 0);
        }

        return make_value_array(error_code, array);
    });
}

void TimestampColumn::insert(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    Column<TimestampColumn>::queue_work(
        args,
        [](qdb_request * qdb_req) {
            const auto alias = qdb_req->input.alias.c_str();
            const auto ts = qdb_req->input.content.str.c_str();
            const auto & points = qdb_req->input.content.timestamp_points;

            // FIXME(Marek): It's a poor man's hack, because ArgsEaterBinder::blobPoints returns an empty collection
            // when an incorrect input has been given. But C API accepts 0-sized inputs.
            if (points.empty())
            {
                qdb_req->output.error = qdb_e_invalid_argument;
            }
            else
            {
                qdb_req->output.error =
                    qdb_ts_timestamp_insert(qdb_req->handle(), ts, alias, points.data(), points.size());
            }
        },
        Entry<Column>::processVoidResult, &ArgsEaterBinder::tsAlias, &ArgsEaterBinder::timestampPoints);
}

void TimestampColumn::ranges(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    TimestampColumn::queue_work(
        args,
        [](qdb_request * qdb_req) {
            const auto alias = qdb_req->input.alias.c_str();
            const auto ts = qdb_req->input.content.str.c_str();
            auto & ranges = qdb_req->input.content.ranges;
            auto bufp = reinterpret_cast<qdb_ts_timestamp_point **>(
                const_cast<void **>(&(qdb_req->output.content.buffer.begin)));
            auto count = &(qdb_req->output.content.buffer.size);

            qdb_req->output.error =
                qdb_ts_timestamp_get_ranges(qdb_req->handle(), ts, alias, ranges.data(), ranges.size(), bufp, count);
        },
        TimestampColumn::processTimestampPointArrayResult, &ArgsEaterBinder::tsAlias, &ArgsEaterBinder::ranges);
}

void TimestampColumn::aggregate(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    TimestampColumn::queue_work(args,
                                [](qdb_request * qdb_req) {
                                    const auto alias = qdb_req->input.alias.c_str();
                                    const auto ts = qdb_req->input.content.str.c_str();
                                    qdb_ts_timestamp_aggregation_t * aggrs =
                                        qdb_req->input.content.timestamp_aggrs.data();
                                    const qdb_size_t count = qdb_req->input.content.timestamp_aggrs.size();

                                    qdb_req->output.error =
                                        qdb_ts_timestamp_aggregate(qdb_req->handle(), ts, alias, aggrs, count);
                                },
                                TimestampColumn::processTimestampAggregateResult, &ArgsEaterBinder::tsAlias,
                                &ArgsEaterBinder::timestampAggregations);
}

void TimestampColumn::processTimestampPointArrayResult(uv_work_t * req, int status)
{
    processResult<2>(req, status, [&](v8::Isolate * isolate, qdb_request * qdb_req) {
        v8::Local<v8::Array> array;

        auto error_code = processErrorCode(isolate, status, qdb_req);
        if ((qdb_req->output.error == qdb_e_ok) && (status >= 0))
        {
            qdb_ts_timestamp_point * entries =
                reinterpret_cast<qdb_ts_timestamp_point *>(const_cast<void *>(qdb_req->output.content.buffer.begin));
            const size_t entries_count = qdb_req->output.content.buffer.size;

            array = v8::Array::New(isolate, static_cast<int>(entries_count));
            if (array.IsEmpty())
            {
                error_code = Error::MakeError(isolate, qdb_e_no_memory_local);
            }
            else
            {
                for (size_t i = 0; i < entries_count; ++i)
                {
                    auto obj = TimestampPoint::MakePoint(isolate, entries[i].timestamp, entries[i].value);
                    if (!obj.IsEmpty()) array->Set(isolate->GetCurrentContext(), static_cast<uint32_t>(i), obj);
                }
            }

            // safe to call even on null/invalid buffers
            qdb_release(qdb_req->handle(), entries);
        }
        else
        {
            // provide an empty array
            array = v8::Array::New(isolate, 0);
        }

        return make_value_array(error_code, array);
    });
}

void TimestampColumn::processTimestampAggregateResult(uv_work_t * req, int status)
{
    processResult<2>(req, status, [&](v8::Isolate * isolate, qdb_request * qdb_req) {
        v8::Local<v8::Array> array;

        auto error_code = processErrorCode(isolate, status, qdb_req);
        if ((qdb_req->output.error == qdb_e_ok) && (status >= 0))
        {
            const auto & aggrs = qdb_req->input.content.timestamp_aggrs;
            array = v8::Array::New(isolate, static_cast<int>(aggrs.size()));
            if (array.IsEmpty())
            {
                error_code = Error::MakeError(isolate, qdb_e_no_memory_local);
            }
            else
            {
                auto resprop = v8::String::NewFromUtf8(isolate, "result", v8::NewStringType::kNormal).ToLocalChecked();
                auto cntprop = v8::String::NewFromUtf8(isolate, "count", v8::NewStringType::kNormal).ToLocalChecked();

                for (size_t i = 0; i < aggrs.size(); ++i)
                {
                    const auto & point = aggrs[i].result;
                    auto result = TimestampPoint::MakePoint(isolate, point.timestamp, point.value);

                    auto obj = v8::Object::New(isolate);
                    obj->Set(isolate->GetCurrentContext(), resprop, result);
                    obj->Set(isolate->GetCurrentContext(), cntprop,
                             v8::Number::New(isolate, static_cast<double>(aggrs[i].count)));

                    if (!obj.IsEmpty()) array->Set(isolate->GetCurrentContext(), static_cast<uint32_t>(i), obj);
                }
            }
        }
        else
        {
            // provide an empty array
            array = v8::Array::New(isolate, 0);
        }

        return make_value_array(error_code, array);
    });
}

std::pair<v8::Local<v8::Object>, bool>
CreateColumn(v8::Isolate * isolate, v8::Local<v8::Object> owner, const char * name, qdb_ts_column_type_t type)
{
    switch (type)
    {
    case qdb_ts_column_blob:
        return std::make_pair(BlobColumn::MakeColumn(isolate, owner, name), true);
    case qdb_ts_column_string:
        return std::make_pair(StringColumn::MakeColumn(isolate, owner, name), true);
    case qdb_ts_column_double:
        return std::make_pair(DoubleColumn::MakeColumn(isolate, owner, name), true);
    case qdb_ts_column_timestamp:
        return std::make_pair(TimestampColumn::MakeColumn(isolate, owner, name), true);
    case qdb_ts_column_int64:
        return std::make_pair(Int64Column::MakeColumn(isolate, owner, name), true);
    case qdb_ts_column_uninitialized:
        return std::make_pair(Error::MakeError(isolate, qdb_e_uninitialized), false);
    }

    return std::make_pair(Error::MakeError(isolate, qdb_e_not_implemented), false);
}

} // namespace quasardb
