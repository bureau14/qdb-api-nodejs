#include "column.hpp"
#include "entry.hpp"
#include "time_series.hpp"

#include <qdb/ts.h>

namespace quasardb
{

v8::Persistent<v8::Function> BlobColumn::constructor;
v8::Persistent<v8::Function> DoubleColumn::constructor;

v8::Local<v8::Object>
CreateColumn(v8::Isolate * isolate, v8::Local<v8::Object> owner, const char * name, qdb_ts_column_type_t type)
{
    switch (type)
    {
    case qdb_ts_column_blob:
        return BlobColumn::MakeColumn(isolate, owner, name);
        break;
    case qdb_ts_column_double:
        return DoubleColumn::MakeColumn(isolate, owner, name);
        break;
    default:
        return v8::Object::New(isolate);
    }
}

void DoubleColumn::insert(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    MethodMan call(args);

    DoubleColumn * c = call.nativeHolder<DoubleColumn>();
    assert(c);

    auto ts = c->timeSeries();
    Column<DoubleColumn>::queue_work(args,
                                     [ts](qdb_request * qdb_req) {
                                         auto & points = qdb_req->input.content.doublePoints;
                                         qdb_req->output.error = qdb_ts_double_insert(qdb_req->handle(), ts.c_str(),
                                                                                      qdb_req->input.alias.c_str(),
                                                                                      points.data(), points.size());
                                     },
                                     Entry<Column>::processVoidResult, &ArgsEaterBinder::doublePoints);
}

void BlobColumn::insert(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    MethodMan call(args);

    BlobColumn * c = call.nativeHolder<BlobColumn>();
    assert(c);

    auto ts = c->timeSeries();
    Column<BlobColumn>::queue_work(args,
                                   [ts](qdb_request * qdb_req) {
                                       auto & points = qdb_req->input.content.blobPoints;

                                       qdb_req->output.error = qdb_ts_blob_insert(qdb_req->handle(), ts.c_str(),
                                                                                  qdb_req->input.alias.c_str(),
                                                                                  points.data(), points.size());
                                   },
                                   Entry<Column>::processVoidResult, &ArgsEaterBinder::blobPoints);
}

} // quasardb namespace
