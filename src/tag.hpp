
#pragma once

#include <memory>
#include <string>

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <uv.h>

#include <qdb/client.h>

#include "entity.hpp"

namespace qdb
{

class Cluster;

class Tag : public Entity<Tag>
{
    friend class Entity<Tag>;
    friend class Cluster;

private:
    Tag(cluster_data_ptr cd, const char * alias) : Entity<Tag>(cd, alias) {}
    virtual ~Tag(void) {}

public:
    static void Init(v8::Local<v8::Object> exports)
    {
        Entity<Tag>::Init(exports, "Tag", [](v8::Local<v8::FunctionTemplate> tpl)
        {
            NODE_SET_PROTOTYPE_METHOD(tpl, "getEntries", getEntries);
        });
    }

public:
    static void getEntries(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Entity<Tag>::queue_work(
            args,
            [](qdb_request * qdb_req)
        {
            qdb_req->output.error = qdb_get_tagged(
                qdb_req->handle(), 
                qdb_req->input.alias.c_str(),
                reinterpret_cast<const char ***>(const_cast<char **>(&(qdb_req->output.content.buffer.begin))),
                &(qdb_req->output.content.buffer.size));
        }, Entity<Tag>::processArrayStringResult);
    }

private:
    static void New(const v8::FunctionCallbackInfo<v8::Value> & args);

private:
    static v8::Persistent<v8::Function> constructor;
};

} // namespace qdb

