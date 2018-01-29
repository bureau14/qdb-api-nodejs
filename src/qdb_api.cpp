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
    quasardb::Deque::Init(exports);
    quasardb::Integer::Init(exports);
    quasardb::Prefix::Init(exports);
    quasardb::QueryFind::Init(exports);
    quasardb::Range::Init(exports);
    quasardb::Set::Init(exports);
    quasardb::Suffix::Init(exports);
    quasardb::Tag::Init(exports);
    quasardb::FilteredRange::Init(exports);
    quasardb::TimeSeries::Init(exports);
    quasardb::DoublePoint::Init(exports);
    quasardb::BlobPoint::Init(exports);
    quasardb::DoubleColumn::Init(exports);
    quasardb::BlobColumn::Init(exports);
    quasardb::Aggregation::Init(exports);

    InitConstants(exports);
}

NODE_MODULE(quasardb, InitAll)
