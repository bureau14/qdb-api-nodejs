
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
    //:desc: Add a value to the front of the queue.
    //:args: content (Buffer) - The value to add to the queue.
    // callback(err) (function) - A callback or anonymous function with error parameter.
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

    //:desc: Add a value to the back of the queue.
    //:args: content (Buffer) - The value to add to the queue.
    // callback(err) (function) - A callback or anonymous function with error parameter.
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

    //:desc: Remove the value at the front of the queue and return it.
    //:args: callback(err, data) (function) - A callback or anonymous function with error and data parameters.
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

    //:desc: Remove the value at the end of the queue and return it.
    //:args: callback(err, data) (function) - A callback or anonymous function with error and data parameters.
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

    //:desc: Retrieves the value at the front of the queue, without removing it.
    //:args: callback(err, data) (function) - A callback or anonymous function with error and data parameters.
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

    //:desc: Retrieves the value at the end of the queue, without removing it.
    //:args: callback(err, data) (function) - A callback or anonymous function with error and data parameters.
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

    //:desc: Returns the size of the Deque.
    //:args: callback(err, size) (function) - A callback or anonymous function with error and size parameters.
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

    //:sign: at
    //:desc: Retrieves the value at the index in the queue. The item at the index must exist or it will throw an error.
    //:args: index (Integer) - The index of the object in the Deque.
    // callback(err, data) (function) - A callback or anonymous function with error and data parameters.
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

    //:desc: Sets the value at the index in the deque.
    //:args: index (Integer) - The index of the object in the Deque.
    // callback(err, data) (function) - A callback or anonymous function with error and data parameters.
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