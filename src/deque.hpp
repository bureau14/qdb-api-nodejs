
#pragma once

#include <memory>
#include <string>

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <uv.h>

#include <qdb/deque.h>

#include "entry.hpp"

namespace quasardb
{

class Deque : public Entry<Deque>
{

    friend class Entry<Deque>;
    friend class Cluster;

public:
    static const size_t ParameterCount = 1;

private:
    Deque(cluster_data_ptr cd, const char * alias) : Entry<Deque>(cd, alias)
    {
    }
    virtual ~Deque(void)
    {
    }

public:
    static void Init(v8::Local<v8::Object> exports)
    {
        Entry<Deque>::Init(exports, "Deque", [](v8::Local<v8::FunctionTemplate> tpl) {
            NODE_SET_PROTOTYPE_METHOD(tpl, "pushFront", pushFront);
            NODE_SET_PROTOTYPE_METHOD(tpl, "pushBack", pushBack);
            NODE_SET_PROTOTYPE_METHOD(tpl, "popFront", popFront);
            NODE_SET_PROTOTYPE_METHOD(tpl, "popBack", popBack);
            NODE_SET_PROTOTYPE_METHOD(tpl, "front", front);
            NODE_SET_PROTOTYPE_METHOD(tpl, "back", back);
            NODE_SET_PROTOTYPE_METHOD(tpl, "size", size);
            NODE_SET_PROTOTYPE_METHOD(tpl, "getAt", getAt);
            NODE_SET_PROTOTYPE_METHOD(tpl, "setAt", setAt);
        });
    }

public:
    static void pushFront(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Entry<Deque>::queue_work(args,
                                 [](qdb_request * qdb_req) {
                                     qdb_req->output.error = qdb_deque_push_front(
                                         qdb_req->handle(), qdb_req->input.alias.c_str(),
                                         qdb_req->input.content.buffer.begin, qdb_req->input.content.buffer.size);
                                 },
                                 Entry<Deque>::processVoidResult, &ArgsEaterBinder::buffer);
    }

    static void pushBack(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Entry<Deque>::queue_work(args,
                                 [](qdb_request * qdb_req) {
                                     qdb_req->output.error = qdb_deque_push_back(
                                         qdb_req->handle(), qdb_req->input.alias.c_str(),
                                         qdb_req->input.content.buffer.begin, qdb_req->input.content.buffer.size);
                                 },
                                 Entry<Deque>::processVoidResult, &ArgsEaterBinder::buffer);
    }

    static void popFront(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Entry<Deque>::queue_work(args,
                                 [](qdb_request * qdb_req) {
                                     qdb_req->output.error =
                                         qdb_deque_pop_front(qdb_req->handle(), qdb_req->input.alias.c_str(),
                                                             &(qdb_req->output.content.buffer.begin),
                                                             &(qdb_req->output.content.buffer.size));
                                 },
                                 Entry<Deque>::processBufferResult, &ArgsEaterBinder::buffer);
    }

    static void popBack(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Entry<Deque>::queue_work(args,
                                 [](qdb_request * qdb_req) {
                                     qdb_req->output.error =
                                         qdb_deque_pop_back(qdb_req->handle(), qdb_req->input.alias.c_str(),
                                                            &(qdb_req->output.content.buffer.begin),
                                                            &(qdb_req->output.content.buffer.size));
                                 },
                                 Entry<Deque>::processBufferResult, &ArgsEaterBinder::buffer);
    }

    static void front(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Entry<Deque>::queue_work(args,
                                 [](qdb_request * qdb_req) {
                                     qdb_req->output.error =
                                         qdb_deque_front(qdb_req->handle(), qdb_req->input.alias.c_str(),
                                                         &(qdb_req->output.content.buffer.begin),
                                                         &(qdb_req->output.content.buffer.size));
                                 },
                                 Entry<Deque>::processBufferResult, &ArgsEaterBinder::buffer);
    }

    static void back(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Entry<Deque>::queue_work(args,
                                 [](qdb_request * qdb_req) {
                                     qdb_req->output.error =
                                         qdb_deque_back(qdb_req->handle(), qdb_req->input.alias.c_str(),
                                                        &(qdb_req->output.content.buffer.begin),
                                                        &(qdb_req->output.content.buffer.size));
                                 },
                                 Entry<Deque>::processBufferResult, &ArgsEaterBinder::buffer);
    }

    static void size(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Entry<Deque>::queue_work(args,
                                 [](qdb_request * qdb_req) {
                                     qdb_size_t got_size = 0;
                                     qdb_req->output.error =
                                         qdb_deque_size(qdb_req->handle(), qdb_req->input.alias.c_str(), &got_size);
                                     qdb_req->output.content.value = static_cast<qdb_int_t>(got_size);
                                 },
                                 Entry<Deque>::processIntegerResult);
    }

    static void getAt(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Entry<Deque>::queue_work(
            args,
            [](qdb_request * qdb_req) {
                qdb_req->output.error =
                    qdb_deque_get_at(qdb_req->handle(), qdb_req->input.alias.c_str(), qdb_req->input.content.value,
                                     &(qdb_req->output.content.buffer.begin), &(qdb_req->output.content.buffer.size));
            },
            Entry<Deque>::processBufferResult, &ArgsEaterBinder::integer);
    }

    static void setAt(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        Entry<Deque>::queue_work(args,
                                 [](qdb_request * qdb_req) {
                                     qdb_req->output.error = qdb_deque_set_at(
                                         qdb_req->handle(), qdb_req->input.alias.c_str(), qdb_req->input.content.value,
                                         qdb_req->input.content.buffer.begin, qdb_req->input.content.buffer.size);
                                 },
                                 Entry<Deque>::processVoidResult, &ArgsEaterBinder::integer, &ArgsEaterBinder::buffer);
    }

private:
    static void New(const v8::FunctionCallbackInfo<v8::Value> & args);

private:
    static v8::Persistent<v8::Function> constructor;
};
}