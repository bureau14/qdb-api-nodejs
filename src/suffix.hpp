#pragma once

#include "entry.hpp"

#include <qdb/client.h>
#include <qdb/suffix.h>

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <uv.h>

#include <memory>
#include <string>

namespace quasardb
{

class Cluster;

class Suffix : public Entry<Suffix>
{
    friend class Entry<Suffix>;
    friend class Cluster;

public:
    static const size_t ParameterCount = 1;

private:
    Suffix(cluster_data_ptr cd, const char * suffix) : Entry<Suffix>(cd, suffix)
    {
    }

    virtual ~Suffix(void)
    {
    }

public:
    static void Init(v8::Local<v8::Object> exports)
    {
        Entry<Suffix>::InitConstructorOnly(exports, "Suffix", [](v8::Local<v8::FunctionTemplate> tpl) {
            NODE_SET_PROTOTYPE_METHOD(tpl, "getEntries", getEntries);
            NODE_SET_PROTOTYPE_METHOD(tpl, "suffix", Entry<Suffix>::alias);
        });
    }

public:
    // :desc: Gets the matched aliases of the specified suffix string.
    // :args: maxCount (int) - maximum count of returned aliases.
    // callback(err, aliases) (function) - A callback function with err and aliases parameter, aliases would hold the answer.
    static void getEntries(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Entry<Suffix>::queue_work(
            args,
            [](qdb_request * qdb_req) {
                qdb_req->output.error = qdb_suffix_get(
                    qdb_req->handle(), qdb_req->input.alias.c_str(), /*max_count=*/qdb_req->input.content.value,
                    reinterpret_cast<const char ***>(const_cast<void **>(&(qdb_req->output.content.buffer.begin))),
                    &(qdb_req->output.content.buffer.size));
            },
            Entry<Suffix>::processArrayStringResult, &ArgsEaterBinder::integer);
    }

private:
    static void New(const v8::FunctionCallbackInfo<v8::Value> & args);

private:
    static v8::Persistent<v8::Function> constructor;
};

} // namespace quasardb
