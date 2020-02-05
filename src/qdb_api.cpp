#include "cluster.hpp"
#include "ts_aggregation.hpp"
#include "ts_column.hpp"
#include "ts_point.hpp"
#include "ts_range.hpp"
#include <node.h>
#include <clocale>

void InitConstants(v8::Local<v8::Object> exports)
{
    v8::Isolate * isolate = exports->GetIsolate();

    // We forgot DontDelete before?
    quasardb::detail::AddConstantProperty(isolate, exports, "NEVER_EXPIRES",
                                          v8::Int32::New(isolate, qdb_never_expires));
    quasardb::detail::AddConstantProperty(isolate, exports, "PRESERVE_EXPIRATION",
                                          v8::Int32::New(isolate, qdb_preserve_expiration));
    quasardb::detail::AddConstantProperty(isolate, exports, "COMPRESSION_NONE", v8::Int32::New(isolate, qdb_comp_none));
    quasardb::detail::AddConstantProperty(isolate, exports, "COMPRESSION_FAST", v8::Int32::New(isolate, qdb_comp_fast));
    quasardb::detail::AddConstantProperty(isolate, exports, "COMPRESSION_BEST", v8::Int32::New(isolate, qdb_comp_best));
}

void InitAll(v8::Local<v8::Object> exports, v8::Local<v8::Object> module)
{
    std::setlocale(LC_ALL, "en_US.UTF-8");

    quasardb::Cluster::Init(exports);

    quasardb::Error::Init(exports);

    quasardb::Blob::Init(exports);
    quasardb::Integer::Init(exports);
    quasardb::Prefix::Init(exports);
    quasardb::QueryFind::Init(exports);
    quasardb::Query::Init(exports);
    quasardb::Range::Init(exports);
    quasardb::Suffix::Init(exports);
    quasardb::Tag::Init(exports);
    quasardb::TsRange::Init(exports);
    quasardb::TimeSeries::Init(exports);
    quasardb::DoublePoint::Init(exports);
    quasardb::BlobPoint::Init(exports);
    quasardb::StringPoint::Init(exports);
    quasardb::Int64Point::Init(exports);
    quasardb::TimestampPoint::Init(exports);
    quasardb::DoubleColumn::Init(exports);
    quasardb::BlobColumn::Init(exports);
    quasardb::StringColumn::Init(exports);
    quasardb::Int64Column::Init(exports);
    quasardb::TimestampColumn::Init(exports);
    quasardb::Aggregation::Init(exports);
    quasardb::Timestamp::Init(exports);

    InitConstants(exports);
}

NODE_MODULE(quasardb, InitAll)
