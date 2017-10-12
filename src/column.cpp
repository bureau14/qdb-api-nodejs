#include "column.hpp"
#include "entry.hpp"
#include "error.hpp"
#include "points.hpp"
#include "time_series.hpp"

#include <qdb/ts.h>

namespace quasardb
{

v8::Persistent<v8::Function> BlobColumn::constructor;
v8::Persistent<v8::Function> DoubleColumn::constructor;

void BlobColumn::insert(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    MethodMan call(args);

    BlobColumn * c = call.nativeHolder<BlobColumn>();
    assert(c);

    auto ts = c->timeSeries();
    Column<BlobColumn>::queue_work(args,
                                   [ts](qdb_request * qdb_req) {
                                       auto alias = qdb_req->input.alias.c_str();
                                       auto & points = qdb_req->input.content.blobPoints;
                                       qdb_req->output.error = qdb_ts_blob_insert(qdb_req->handle(), ts.c_str(), alias,
                                                                                  points.data(), points.size());
                                   },
                                   Entry<Column>::processVoidResult, &ArgsEaterBinder::blobPoints);
}

void BlobColumn::ranges(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    MethodMan call(args);

    BlobColumn * c = call.nativeHolder<BlobColumn>();
    assert(c);

    auto ts = c->timeSeries();
    BlobColumn::queue_work(args,
                           [ts](qdb_request * qdb_req) {
                               auto alias = qdb_req->input.alias.c_str();
                               auto & ranges = qdb_req->input.content.ranges;
                               auto bufp = reinterpret_cast<qdb_ts_blob_point **>(
                                   const_cast<void **>(&(qdb_req->output.content.buffer.begin)));
                               auto count = &(qdb_req->output.content.buffer.size);

                               qdb_req->output.error = qdb_ts_blob_get_ranges(
                                   qdb_req->handle(), ts.c_str(), alias, ranges.data(), ranges.size(), bufp, count);
                           },
                           BlobColumn::processBlobPointArrayResult, &ArgsEaterBinder::ranges);
}

void DoubleColumn::insert(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    MethodMan call(args);

    DoubleColumn * c = call.nativeHolder<DoubleColumn>();
    assert(c);

    auto ts = c->timeSeries();
    Column<DoubleColumn>::queue_work(args,
                                     [ts](qdb_request * qdb_req) {
                                         auto alias = qdb_req->input.alias.c_str();
                                         auto & points = qdb_req->input.content.doublePoints;

                                         qdb_req->output.error = qdb_ts_double_insert(
                                             qdb_req->handle(), ts.c_str(), alias, points.data(), points.size());
                                     },
                                     Entry<Column>::processVoidResult, &ArgsEaterBinder::doublePoints);
}

void DoubleColumn::ranges(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    MethodMan call(args);

    DoubleColumn * c = call.nativeHolder<DoubleColumn>();
    assert(c);

    auto ts = c->timeSeries();
    DoubleColumn::queue_work(args,
                             [ts](qdb_request * qdb_req) {
                                 auto alias = qdb_req->input.alias.c_str();
                                 auto & ranges = qdb_req->input.content.ranges;
                                 auto bufp = reinterpret_cast<qdb_ts_double_point **>(
                                     const_cast<void **>(&(qdb_req->output.content.buffer.begin)));
                                 auto count = &(qdb_req->output.content.buffer.size);

                                 qdb_req->output.error = qdb_ts_double_get_ranges(
                                     qdb_req->handle(), ts.c_str(), alias, ranges.data(), ranges.size(), bufp, count);
                             },
                             DoubleColumn::processDoublePointArrayResult, &ArgsEaterBinder::ranges);
}

void BlobColumn::processBlobPointArrayResult(uv_work_t * req, int status)
{
    processResult<2>(req, status, [&](v8::Isolate * isolate, qdb_request * qdb_req) {
        v8::Handle<v8::Array> array;

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

                    if (!obj.IsEmpty()) array->Set(static_cast<uint32_t>(i), obj);
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

void DoubleColumn::processDoublePointArrayResult(uv_work_t * req, int status)
{
    processResult<2>(req, status, [&](v8::Isolate * isolate, qdb_request * qdb_req) {
        v8::Handle<v8::Array> array;

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
                    if (!obj.IsEmpty()) array->Set(static_cast<uint32_t>(i), obj);
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

std::pair<v8::Local<v8::Object>, bool>
CreateColumn(v8::Isolate * isolate, v8::Local<v8::Object> owner, const char * name, qdb_ts_column_type_t type)
{
    switch (type)
    {
    case qdb_ts_column_blob:
        return std::make_pair(BlobColumn::MakeColumn(isolate, owner, name), true);
        break;
    case qdb_ts_column_double:
        return std::make_pair(DoubleColumn::MakeColumn(isolate, owner, name), true);
        break;
    case qdb_ts_column_uninitialized:
        return std::make_pair(Error::MakeError(isolate, qdb_e_uninitialized), false);
    }

    return std::make_pair(Error::MakeError(isolate, qdb_e_not_implemented), false);
}

} // quasardb namespace
