#include "time_series.hpp"
#include "cluster.hpp"
#include "entry.hpp"
#include "ts_column.hpp"

namespace quasardb
{

v8::Persistent<v8::Function> TimeSeries::constructor;

void TimeSeries::New(const v8::FunctionCallbackInfo<v8::Value> & args)
{
    Cluster::newObject<TimeSeries>(args);
}

void TimeSeries::processArrayColumnsInfoResult(uv_work_t * req, int status)
{
    processResult<2>(req, status,
        [&](v8::Isolate * isolate, qdb_request * qdb_req)
        {
            v8::Local<v8::Array> array;

            auto error_code = processErrorCode(isolate, status, qdb_req);
            if ((qdb_req->output.error == qdb_e_ok) && (status >= 0))
            {
                auto entries = reinterpret_cast<qdb_ts_column_info_ex_t *>(
                    const_cast<void *>(qdb_req->output.content.buffer.begin));
                const size_t entries_count = qdb_req->output.content.buffer.size;

                array = v8::Array::New(isolate, static_cast<int>(entries_count));
                if (array.IsEmpty())
                {
                    error_code = Error::MakeError(isolate, qdb_e_no_memory_local);
                }
                else
                {
                    assert(!qdb_req->holder.IsEmpty() && "Verify that appropriate argument eater has been used");

                    auto owner = v8::Local<v8::Object>::New(isolate, qdb_req->holder);
                    for (size_t i = 0; i < entries_count; ++i)
                    {
                        auto obj_ok = CreateColumn(isolate, owner, entries[i].name, entries[i].type);
                        if (!obj_ok.second)
                        {
                            error_code = obj_ok.first;
                            break;
                        }

                        if (!obj_ok.first.IsEmpty())
                        {
                            array->Set(isolate->GetCurrentContext(), static_cast<uint32_t>(i), obj_ok.first);
                        }
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

void TimeSeries::processColumnsCreateResult(uv_work_t * req, int status)
{
    processResult<2>(req, status,
        [&](v8::Isolate * isolate, qdb_request * qdb_req)
        {
            v8::Local<v8::Array> array;

            auto error_code = processErrorCode(isolate, status, qdb_req);
            if ((qdb_req->output.error == qdb_e_ok) && (status >= 0))
            {
                const auto & columns = qdb_req->input.content.columns;
                array = v8::Array::New(isolate, static_cast<int>(columns.size()));
                if (array.IsEmpty())
                {
                    error_code = Error::MakeError(isolate, qdb_e_no_memory_local);
                }
                else
                {
                    assert(!qdb_req->holder.IsEmpty() && "Verify that appropriate argument eater has been used");

                    auto owner = v8::Local<v8::Object>::New(isolate, qdb_req->holder);
                    for (size_t i = 0; i < columns.size(); ++i)
                    {
                        auto const & column = columns[i];
                        auto obj_ok = CreateColumn(isolate, owner, column.name.c_str(), column.type);
                        if (!obj_ok.second)
                        {
                            error_code = obj_ok.first;
                            break;
                        }

                        if (!obj_ok.first.IsEmpty())
                        {
                            array->Set(isolate->GetCurrentContext(), static_cast<uint32_t>(i), obj_ok.first);
                        }
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

} // namespace quasardb
