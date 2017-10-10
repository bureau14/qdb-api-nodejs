#include "column.hpp"
#include "entry.hpp"
#include "time_series.hpp"

#include <qdb/ts.h>

namespace quasardb
{

v8::Persistent<v8::Function> Column::constructor;

void Column::insert(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    MethodMan call(args);

    Column * c = call.nativeHolder<Column>();
    assert(c);

    // TODO: Change to static polymorphis ?
    switch (c->type)
    {
    case qdb_ts_column_blob:
        Column::insertBlobs(c->ts, args);
        break;
    case qdb_ts_column_double:
        Column::insertDoubles(c->ts, args);
        break;
    default:
        return;
    }
}

void Column::insertDoubles(const std::string & ts, const v8::FunctionCallbackInfo<v8::Value> & args)
{
    Entry<Column>::queue_work(args,
                              [ts](qdb_request * qdb_req) {
                                  auto & points = qdb_req->input.content.doublePoints;
                                  qdb_req->output.error =
                                      qdb_ts_double_insert(qdb_req->handle(), ts.c_str(), qdb_req->input.alias.c_str(),
                                                           points.data(), points.size());
                              },
                              Entry<Column>::processVoidResult, &ArgsEaterBinder::doublePoints);
}

void Column::insertBlobs(const std::string & ts, const v8::FunctionCallbackInfo<v8::Value> & args)
{
    Entry<Column>::queue_work(args,
                              [ts](qdb_request * qdb_req) {
                                  auto & points = qdb_req->input.content.blobPoints;

                                  qdb_req->output.error =
                                      qdb_ts_blob_insert(qdb_req->handle(), ts.c_str(), qdb_req->input.alias.c_str(),
                                                         points.data(), points.size());
                              },
                              Entry<Column>::processVoidResult, &ArgsEaterBinder::blobPoints);
}

} // quasardb namespace
