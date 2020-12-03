#pragma once

#include "cluster_data.hpp"
#include "error.hpp"
#include "utilities.hpp"
#include <qdb/client.h>
#include <qdb/tag.h>
#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <uv.h>
#include <algorithm>
#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>

namespace quasardb
{

template <typename... T>
/*constexpr*/ auto make_value_array(T &&... t) -> std::array<v8::Local<v8::Value>, sizeof...(T)>
{
    return {{std::forward<T>(t)...}};
}

namespace detail
{

static void callback_wrapper(uv_work_t * req)
{
    static_cast<qdb_request *>(req->data)->execute();
}

template <typename SpawnRequest, typename F, typename... Params>
static void queue_work(const v8::FunctionCallbackInfo<v8::Value> & args,
                       SpawnRequest spawnRequest,
                       F f,
                       uv_after_work_cb after_work_cb,
                       Params... p)
{
    v8::Isolate * isolate = v8::Isolate::GetCurrent();
    v8::TryCatch try_catch(isolate);

    MethodMan call(args);

    uv_work_t * work = spawnRequest(call, f, p...);

    if (try_catch.HasCaught())
    {
        v8::HandleScope scope(isolate);
        node::FatalException(isolate, try_catch);
    }

    assert(work);

    uv_queue_work(uv_default_loop(), work, detail::callback_wrapper, after_work_cb);

    // this is callback, the return value is undefined and the callback will get everything
    call.setUndefinedReturnValue();
}

} // namespace detail

template <typename Derivate>
class Entry : public node::ObjectWrap
{
public:
    static const int FieldsCount = 2;

public:
    // if an entry is expirable, we will register the expiry management functions
    // this simplifies organization here
    // in other APIs generally we have an ExpirableEntry that inherits from Entry
    Entry(cluster_data_ptr cd, const char * alias) : _cluster_data(cd), _alias(new std::string(alias))
    {
    }

    virtual ~Entry(void)
    {
    }

public:
    //:desc: Returns the alias of the entity
    //:returns: A string representing the alias of the Entity
    static void alias(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        MethodMan this_call(args);

        Derivate * pthis = this_call.nativeHolder<Derivate>();

        assert(pthis);

        this_call.template setReturnValue<v8::String>(pthis->native_alias().c_str());
    }

public:
    //:desc: Removes the Entity
    //:args: callback(err) (function) - A callback function with error parameter
    //:returns: A string representing the alias of the Entity
    static void remove(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        queue_work(args,
                   [](qdb_request * qdb_req) //
                   { qdb_req->output.error = qdb_remove(qdb_req->handle(), qdb_req->input.alias.c_str()); },
                   processVoidResult, &ArgsEaterBinder::none);
    }

public:
    qdb_handle_ptr qdb_handle(void) const
    {
        return _cluster_data->handle();
    }

    const std::string & native_alias(void) const
    {
        assert(_alias);
        return *_alias;
    }

private:
    template <typename F, typename... Params>
    static uv_work_t * spawnRequest(const MethodMan & call, F f, Params... p)
    {
        Derivate * pthis = call.nativeHolder<Derivate>();
        assert(pthis);

        qdb_request * qdb_req = new qdb_request(pthis->_cluster_data, f, pthis->native_alias());

        ArgsEaterBinder eaterBinder(call);
        eaterBinder.eatThem(*qdb_req, p...);

        uv_work_t * req = nullptr;

        if (eaterBinder.bindCallback(*qdb_req))
        {
            req = new uv_work_t();
            req->data = qdb_req;
        }
        else
        {
            delete qdb_req;
            call.throwException("callback expected");
        }

        return req;
    }

private:
    static std::vector<const char *> convertStrings(const std::vector<std::string> & strs)
    {
        std::vector<const char *> cstrs;
        cstrs.reserve(strs.size());

        for (const auto & s : strs)
        {
            cstrs.push_back(s.c_str());
        }

        return cstrs;
    }

public:
    // tag management function
    //:desc: Attaches the Entity to the specified tag. Errors if the tag is already assigned.
    //:args: tagName (String) - The name of the tag.
    // callback(err) (function) - A callback or anonymous function with error parameter.
    static void attachTag(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        queue_work(args,
                   [](qdb_request * qdb_req) //
                   {
                       qdb_req->output.error = qdb_attach_tag(qdb_req->handle(), qdb_req->input.alias.c_str(),
                                                              qdb_req->input.content.str.c_str());
                   },
                   processVoidResult, &ArgsEaterBinder::string);
    }

    //:desc: Attaches the Entity to the specified tags. Errors if any of the tags is already assigned.
    //:args: tagNames (String[]) - Array of names of the tags (Array of Strings).
    // callback(err) (function) - A callback or anonymous function with error parameter.
    static void attachTags(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        queue_work(args,
                   [](qdb_request * qdb_req) //
                   {
                       std::vector<const char *> tags = convertStrings(qdb_req->input.content.strs);

                       qdb_req->output.error =
                           qdb_attach_tags(qdb_req->handle(), qdb_req->input.alias.c_str(), tags.data(), tags.size());
                   },
                   processVoidResult, &ArgsEaterBinder::strings);
    }

    static void getMetadata(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        queue_work(args,
                   [](qdb_request * qdb_req) //
                   {
                       qdb_req->output.error = qdb_get_metadata(qdb_req->handle(), qdb_req->input.alias.c_str(),
                                                                &qdb_req->output.content.entry_metadata);
                   },
                   processEntryMetadataResult);
    }

    //:desc: Detaches the Entity from the specified tag. Errors if the tag is not assigned.
    //:args: tagName (String) - The name of the tag.
    // callback(err) (function) - A callback or anonymous function with error parameter.
    static void detachTag(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        queue_work(args,
                   [](qdb_request * qdb_req) //
                   {
                       qdb_req->output.error = qdb_detach_tag(qdb_req->handle(), qdb_req->input.alias.c_str(),
                                                              qdb_req->input.content.str.c_str());
                   },
                   processVoidResult, &ArgsEaterBinder::string);
    }

    //:desc: Detaches the Entity from the specified tags. Errors if any of the tags is not assigned.
    //:args: tagNames (String[]) - Array of names of the tags (Array of Strings).
    // callback(err) (function) - A callback or anonymous function with error parameter.
    static void detachTags(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        queue_work(args,
                   [](qdb_request * qdb_req) //
                   {
                       std::vector<const char *> tags = convertStrings(qdb_req->input.content.strs);

                       qdb_req->output.error =
                           qdb_detach_tags(qdb_req->handle(), qdb_req->input.alias.c_str(), tags.data(), tags.size());
                   },
                   processVoidResult, &ArgsEaterBinder::strings);
    }

    //:desc: Determines if the Entity has the specified tag.
    //:args: tagName (String) - The name of the tag.
    // callback(err) (function) - A callback or anonymous function with error parameter.
    static void hasTag(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        queue_work(args,
                   [](qdb_request * qdb_req) //
                   {
                       qdb_req->output.error = qdb_has_tag(qdb_req->handle(), qdb_req->input.alias.c_str(),
                                                           qdb_req->input.content.str.c_str());
                   },
                   processVoidResult, &ArgsEaterBinder::string);
    }

    //:desc: Determines if the Entity has the specified tags.
    //:args: tagNames (String[]) - Array of names of the tags (Array of Strings).
    // callback(err, success_count, result) (function) - A callback or anonymous function with: error parameter, number of specified tags assigned to the Entity and query result. Result is an Object with as many fields as the length of tagNames array, each having a bool value true (tag assigned) or false (otherwise).
    static void hasTags(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        queue_work(args,
                   [](qdb_request * qdb_req) //
                   {
                       auto & tags = qdb_req->input.content.strs;
                       auto & ops = qdb_req->output.batch.operations;
                       ops.resize(tags.size());

                       qdb_req->output.error = qdb_init_operations(ops.data(), ops.size());
                       if (!QDB_SUCCESS(qdb_req->output.error)) return;

                       std::transform(tags.cbegin(), tags.cend(), ops.begin(), [qdb_req](const std::string & tag) {
                           qdb_operation_t op;
                           op.type = qdb_op_has_tag;
                           op.alias = qdb_req->input.alias.c_str();
                           op.has_tag.tag = tag.c_str();
                           return op;
                       });

                       qdb_req->output.batch.success_count = qdb_run_batch(qdb_req->handle(), ops.data(), ops.size());
                       qdb_req->output.error = qdb_e_ok;
                   },
                   processBatchHasTagResult, &ArgsEaterBinder::strings);
    }

    //:desc: Gets an array of tag objects associated with the Entity.
    //:args: callback(err, tags) (function) - A callback or anonymous function with error and array of tags parameters.
    static void getTags(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        queue_work(args,
                   [](qdb_request * qdb_req) //
                   {
                       auto buffer_begin = reinterpret_cast<const char ***>(
                           const_cast<void **>(&(qdb_req->output.content.buffer.begin)));
                       qdb_req->output.error = qdb_get_tags(qdb_req->handle(), qdb_req->input.alias.c_str(),
                                                            buffer_begin, &(qdb_req->output.content.buffer.size));
                   },
                   processArrayStringResult, &ArgsEaterBinder::none);
    }

public:
    template <typename F>
    static void Init(v8::Local<v8::Object> exports, const char * className, F init)
    {
        InitConstructorOnly(exports, className, [init](v8::Local<v8::FunctionTemplate> tpl) {
            NODE_SET_PROTOTYPE_METHOD(tpl, "alias", Entry<Derivate>::alias);
            NODE_SET_PROTOTYPE_METHOD(tpl, "remove", Entry<Derivate>::remove);
            NODE_SET_PROTOTYPE_METHOD(tpl, "attachTag", Entry<Derivate>::attachTag);
            NODE_SET_PROTOTYPE_METHOD(tpl, "attachTags", Entry<Derivate>::attachTags);
            NODE_SET_PROTOTYPE_METHOD(tpl, "getMetadata", Entry<Derivate>::getMetadata);
            NODE_SET_PROTOTYPE_METHOD(tpl, "getTags", Entry<Derivate>::getTags);
            NODE_SET_PROTOTYPE_METHOD(tpl, "hasTag", Entry<Derivate>::hasTag);
            NODE_SET_PROTOTYPE_METHOD(tpl, "hasTags", Entry<Derivate>::hasTags);
            NODE_SET_PROTOTYPE_METHOD(tpl, "detachTag", Entry<Derivate>::detachTag);
            NODE_SET_PROTOTYPE_METHOD(tpl, "detachTags", Entry<Derivate>::detachTags);

            init(tpl);
        });
    }

    template <typename F>
    static void InitConstructorOnly(v8::Local<v8::Object> exports, const char * className, F init)
    {
        v8::Isolate * isolate = exports->GetIsolate();

        // Prepare constructor template
        v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, Derivate::New);

        tpl->SetClassName(v8::String::NewFromUtf8(isolate, className, v8::NewStringType::kNormal).ToLocalChecked());
        tpl->InstanceTemplate()->SetInternalFieldCount(Entry<Derivate>::FieldsCount);

        init(tpl);

        auto maybe_function = tpl->GetFunction(isolate->GetCurrentContext());
        if (maybe_function.IsEmpty()) return;

        Derivate::constructor.Reset(isolate, maybe_function.ToLocalChecked());
        exports->Set(isolate->GetCurrentContext(), v8::String::NewFromUtf8(isolate, className, v8::NewStringType::kNormal).ToLocalChecked(), maybe_function.ToLocalChecked());
    }

    static void NewInstance(const v8::FunctionCallbackInfo<v8::Value> & args)
    {
        v8::Isolate * isolate = args.GetIsolate();

        const unsigned argc = 2;
        v8::Local<v8::Value> argv[argc] = {args[0], args[1]};
        v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, Derivate::constructor);
        v8::Local<v8::Object> instance = cons->NewInstance(isolate->GetCurrentContext(), argc, argv).ToLocalChecked();

        args.GetReturnValue().Set(instance);
    }

private:
    static void processCallAndCleanUp(v8::Isolate * isolate,
                                      v8::TryCatch & try_catch,
                                      uv_work_t * req,
                                      qdb_request * qdb_req,
                                      unsigned int argc,
                                      v8::Local<v8::Value> argv[])
    {
        qdb_error_t err = qdb_e_ok;

        if (argc > 0)
        {
            auto maybe_err = argv[0]->NumberValue(isolate->GetCurrentContext());
            if (maybe_err.IsJust())
            {
                err = static_cast<qdb_error_t>(static_cast<unsigned int>(maybe_err.FromJust()));
            }
        }

        if (QDB_SUCCESS(err))
        {
            auto cb = qdb_req->callbackAsLocal();
            cb->Call(isolate->GetCurrentContext(), isolate->GetCurrentContext()->Global(), argc, argv);
        }
        else
        {
            // create the error object and pass it to the callback
            v8::Local<v8::Object> error_object = Error::MakeError(isolate, err);
            qdb_req->on_error(isolate, error_object);
        }

        delete qdb_req;
        delete req;

        if (try_catch.HasCaught())
        {
            node::FatalException(isolate, try_catch);
        }
    }

protected:
    static auto processErrorCode(v8::Isolate * isolate, int status, const qdb_request * req) -> v8::Local<v8::Value>
    {
        if (status < 0)
        {
            return Error::MakeError(isolate, qdb_e_internal_local);
        }

        // only when the error is strictly ok, we avoid to create an error object
        // in other cases, even if it is informational we want to create it
        if ((req->output.error == qdb_e_ok) || (req->output.error == qdb_e_ok_created))
        {
            // nullptr for success
            return v8::Null(isolate);
        }

        return Error::MakeError(isolate, req->output.error); // req->output.error);
    }

    template <size_t Argc, typename Proc>
    static void processResult(uv_work_t * req, int status, Proc process)
    {
        v8::Isolate * isolate = v8::Isolate::GetCurrent();
        v8::HandleScope scope(isolate);

        v8::TryCatch try_catch(isolate);

        qdb_request * qdb_req = static_cast<qdb_request *>(req->data);
        assert(qdb_req);

        std::array<v8::Local<v8::Value>, Argc> args = process(isolate, qdb_req);

        processCallAndCleanUp(isolate, try_catch, req, qdb_req, static_cast<unsigned int>(args.size()), args.data());
    }

public:
    static void processBufferResult(uv_work_t * req, int status)
    {
        processResult<2>(req, status, [&](v8::Isolate * isolate, qdb_request * qdb_req) {
            // don't free the buffer qdb_req->output.content.buffer.begin
            // we handed over ownership in make_node_buffer
            const auto error_code = processErrorCode(isolate, status, qdb_req);
            const auto result_data =
                ((status >= 0) && (qdb_req->output.error == qdb_e_ok) && (qdb_req->output.content.buffer.size > 0u))
                    ? qdb_req->make_node_buffer(isolate).ToLocalChecked()
                    : node::Buffer::New(isolate, 0).ToLocalChecked();

            return make_value_array(error_code, result_data);
        });
    }

    // build an array out of the buffer
    static void processArrayStringResult(uv_work_t * req, int status)
    {
        processResult<2>(req, status, [&](v8::Isolate * isolate, qdb_request * qdb_req) {
            v8::Local<v8::Array> array;

            auto error_code = processErrorCode(isolate, status, qdb_req);
            if ((qdb_req->output.error == qdb_e_ok) && (status >= 0))
            {
                const char ** entries =
                    reinterpret_cast<const char **>(const_cast<void *>(qdb_req->output.content.buffer.begin));
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
                        auto str = v8::String::NewFromUtf8(isolate, entries[i], v8::NewStringType::kNormal).ToLocalChecked();
                        array->Set(isolate->GetCurrentContext(), static_cast<uint32_t>(i), str);
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

    static void
    query_set_rows(v8::Isolate * isolate, const qdb_query_result_t * result, v8::Local<v8::Object> & final_result)
    {
        auto rows_prop = v8::String::NewFromUtf8(isolate, "rows", v8::NewStringType::kNormal).ToLocalChecked();
        auto rows_count_prop = v8::String::NewFromUtf8(isolate, "row_count", v8::NewStringType::kNormal).ToLocalChecked();

        const auto column_count = result->column_count;
        const auto row_count = result->row_count;
        v8::Local<v8::Array> rows = v8::Array::New(isolate, static_cast<int>(row_count));
        for (size_t i = 0; i < row_count; ++i)
        {
            v8::Local<v8::Array> columns = v8::Array::New(isolate, static_cast<int>(column_count));
            for (size_t j = 0; j < column_count; ++j)
            {
                auto pt = result->rows[i][j];
                switch (pt.type)
                {
                case qdb_query_result_double:
                    columns->Set(isolate->GetCurrentContext(), j, v8::Number::New(isolate, pt.payload.double_.value));
                    break;
                case qdb_query_result_blob:
                {
                    std::string blob{reinterpret_cast<const char *>(pt.payload.blob.content),
                                     pt.payload.blob.content_length};
                    columns->Set(isolate->GetCurrentContext(), j, v8::String::NewFromUtf8(isolate, blob.c_str(), v8::NewStringType::kNormal).ToLocalChecked());
                    break;
                }
                case qdb_query_result_int64:
                    columns->Set(isolate->GetCurrentContext(), j, v8::Number::New(isolate, pt.payload.int64_.value));
                    break;
                case qdb_query_result_timestamp:
                {
                    auto timestamp = Timestamp::NewFromTimespec(isolate, pt.payload.timestamp.value);

                    columns->Set(isolate->GetCurrentContext(), j, timestamp);
                    break;
                }
                case qdb_query_result_count:
                {
                    columns->Set(isolate->GetCurrentContext(), j, v8::Number::New(isolate, pt.payload.count.value));
                    break;
                }
                case qdb_query_result_string:
                {
                    std::string str{reinterpret_cast<const char *>(pt.payload.string.content),
                                     pt.payload.string.content_length};
                    columns->Set(isolate->GetCurrentContext(), j, v8::String::NewFromUtf8(isolate, str.c_str(), v8::NewStringType::kNormal).ToLocalChecked());
                    break;
                }
                case qdb_query_result_symbol:
                {
                    std::string str{reinterpret_cast<const char *>(pt.payload.symbol.content),
                                     pt.payload.symbol.content_length};
                    columns->Set(isolate->GetCurrentContext(), j, v8::String::NewFromUtf8(isolate, str.c_str(), v8::NewStringType::kNormal).ToLocalChecked());
                    break;
                }
                }
            }
            rows->Set(isolate->GetCurrentContext(), i, columns);
        }
        final_result->Set(isolate->GetCurrentContext(), rows_prop, rows);
        final_result->Set(isolate->GetCurrentContext(), rows_count_prop, v8::Number::New(isolate, row_count));
    }

    static void query_set_columns_names(v8::Isolate * isolate,
                                        const qdb_query_result_t * result,
                                        v8::Local<v8::Object> & final_result)
    {
        auto columns_names_prop = v8::String::NewFromUtf8(isolate, "column_names", v8::NewStringType::kNormal).ToLocalChecked();
        auto columns_count_prop = v8::String::NewFromUtf8(isolate, "column_count", v8::NewStringType::kNormal).ToLocalChecked();

        const auto column_count = result->column_count;
        v8::Local<v8::Array> column_names = v8::Array::New(isolate, static_cast<int>(column_count));
        for (size_t i = 0; i < column_count; ++i)
        {
            std::string name{result->column_names[i].data, result->column_names[i].length};
            column_names->Set(isolate->GetCurrentContext(), i, v8::String::NewFromUtf8(isolate, name.c_str(), v8::NewStringType::kNormal).ToLocalChecked());
        }
        final_result->Set(isolate->GetCurrentContext(), columns_count_prop, v8::Number::New(isolate, column_count));
        final_result->Set(isolate->GetCurrentContext(), columns_names_prop, column_names);
    }

    static v8::Local<v8::Object>
    query_make_result(v8::Isolate * isolate, qdb_query_result_t * result, v8::Local<v8::Object> & final_result)
    {
        auto scanned_point_count_prop = v8::String::NewFromUtf8(isolate, "scanned_point_count", v8::NewStringType::kNormal).ToLocalChecked();
        auto error_msg_prop = v8::String::NewFromUtf8(isolate, "error_message", v8::NewStringType::kNormal).ToLocalChecked();

        final_result->Set(isolate->GetCurrentContext(), scanned_point_count_prop, v8::Number::New(isolate, result->scanned_point_count));
        std::string err_msg{result->error_message.data, result->error_message.length};
        final_result->Set(isolate->GetCurrentContext(), error_msg_prop, v8::String::NewFromUtf8(isolate, err_msg.c_str(), v8::NewStringType::kNormal).ToLocalChecked());

        query_set_columns_names(isolate, result, final_result);
        query_set_rows(isolate, result, final_result);

        return {};
    }

    // build an array out of the buffer
    static void processQueryResult(uv_work_t * req, int status)
    {
        processResult<2>(req, status, [&](v8::Isolate * isolate, qdb_request * qdb_req) {
            v8::Local<v8::Object> final_result = v8::Object::New(isolate);

            auto error_code = processErrorCode(isolate, status, qdb_req);
            if ((qdb_req->output.error == qdb_e_ok) && (status >= 0))
            {
                auto err = query_make_result(isolate, qdb_req->output.query_result, final_result);
                if (!err.IsEmpty())
                {
                    error_code = err;
                }

                // safe to call even on null/invalid buffers
                qdb_release(qdb_req->handle(), qdb_req->output.query_result);
            }

            return make_value_array(error_code, final_result);
        });
    }

    static void processIntegerResult(uv_work_t * req, int status)
    {
        processResult<2>(req, status, [&](v8::Isolate * isolate, qdb_request * qdb_req) {
            const auto error_code = processErrorCode(isolate, status, qdb_req);
            auto result_data = ((status >= 0) && (qdb_req->output.error == qdb_e_ok))
                                   ? v8::Number::New(isolate, static_cast<double>(qdb_req->output.content.value))
                                   : v8::Number::New(isolate, 0.0);

            return make_value_array(error_code, result_data);
        });
    }

    static void processUintegerResult(uv_work_t * req, int status)
    {
        processResult<2>(req, status, [&](v8::Isolate * isolate, qdb_request * qdb_req) {
            const auto error_code = processErrorCode(isolate, status, qdb_req);
            auto result_data = ((status >= 0) && (qdb_req->output.error == qdb_e_ok))
                                   ? v8::Number::New(isolate, static_cast<double>(qdb_req->output.content.uvalue))
                                   : v8::Number::New(isolate, 0.0);

            return make_value_array(error_code, result_data);
        });
    }

    static void processDateResult(uv_work_t * req, int status)
    {
        processResult<2>(req, status, [&](v8::Isolate * isolate, qdb_request * qdb_req) {
            const auto error_code = processErrorCode(isolate, status, qdb_req);
            if ((status >= 0) && (qdb_req->output.error == qdb_e_ok) && (qdb_req->output.content.value > 0))
            {
                double millis = static_cast<double>(qdb_req->output.content.value);

                auto maybe_date = v8::Date::New(isolate->GetCurrentContext(), millis);
                if (maybe_date.IsEmpty())
                {
                    assert("Cannot create new date");
                    return make_value_array(error_code, v8::Undefined(isolate));
                }

                return make_value_array(error_code, maybe_date.ToLocalChecked());
            }
            return make_value_array(error_code, v8::Undefined(isolate));
        });
    }

    static void processEntryMetadataResult(uv_work_t * req, int status)
    {
        processResult<2>(req, status, [&](v8::Isolate * isolate, qdb_request * qdb_req) {
            const auto error_code = processErrorCode(isolate, status, qdb_req);
            if ((status < 0) || (qdb_req->output.error != qdb_e_ok))
            {
                return make_value_array(error_code, v8::Undefined(isolate));
            }

            auto reference = v8::Array::New(isolate, 4);
            for (int32_t i = 0; i < 4; ++i)
            {
                reference->Set(
                    isolate->GetCurrentContext(),
                    i, 
                    v8::Number::New(isolate, static_cast<double>(qdb_req->output.content.entry_metadata.reference.data[i])));
            }

            auto meta = v8::Object::New(isolate);

            meta->Set(isolate->GetCurrentContext(), v8::String::NewFromUtf8(isolate, "reference", v8::NewStringType::kNormal).ToLocalChecked(), reference);
            meta->Set(isolate->GetCurrentContext(), v8::String::NewFromUtf8(isolate, "type", v8::NewStringType::kNormal).ToLocalChecked(),
                      v8::Integer::New(isolate, qdb_req->output.content.entry_metadata.type));
            meta->Set(isolate->GetCurrentContext(), v8::String::NewFromUtf8(isolate, "size", v8::NewStringType::kNormal).ToLocalChecked(),
                      v8::Number::New(isolate, static_cast<double>(qdb_req->output.content.entry_metadata.size)));

            {
                double millis = qdb_timespec_to_ms(qdb_req->output.content.entry_metadata.modification_time);

                auto maybe_date = v8::Date::New(isolate->GetCurrentContext(), millis);
                if (maybe_date.IsEmpty())
                {
                    assert("Cannot create new date");
                    return make_value_array(error_code, meta);
                }

                meta->Set(isolate->GetCurrentContext(), v8::String::NewFromUtf8(isolate, "modification_time", v8::NewStringType::kNormal).ToLocalChecked(),
                          (millis > 0) ? maybe_date.ToLocalChecked() : v8::Local<v8::Value>(v8::Undefined(isolate)));
            }

            {
                double millis = qdb_timespec_to_ms(qdb_req->output.content.entry_metadata.expiry_time);

                auto maybe_date = v8::Date::New(isolate->GetCurrentContext(), millis);
                if (maybe_date.IsEmpty())
                {
                    assert("Cannot create new date");
                    return make_value_array(error_code, meta);
                }

                meta->Set(isolate->GetCurrentContext(), v8::String::NewFromUtf8(isolate, "expiry_time", v8::NewStringType::kNormal).ToLocalChecked(),
                          (millis > 0) ? maybe_date.ToLocalChecked() : v8::Local<v8::Value>(v8::Undefined(isolate)));
            }

            return make_value_array(error_code, meta);
        });
    }

    static void processEntryTypeResult(uv_work_t * req, int status)
    {
        processResult<2>(req, status, [&](v8::Isolate * isolate, qdb_request * qdb_req) {
            const auto error_code = processErrorCode(isolate, status, qdb_req);
            auto result_data = ((status >= 0) && (qdb_req->output.error == qdb_e_ok))
                                   ? v8::Integer::New(isolate, qdb_req->output.content.entry_type)
                                   : v8::Integer::New(isolate, qdb_entry_uninitialized);

            return make_value_array(error_code, result_data);
        });
    }

    static void processVoidResult(uv_work_t * req, int status)
    {
        processResult<1>(req, status, [&](v8::Isolate * isolate, qdb_request * qdb_req) {
            const auto error_code = processErrorCode(isolate, status, qdb_req);
            return make_value_array(error_code);
        });
    }

    static void processBatchHasTagResult(uv_work_t * req, int status)
    {
        processResult<3>(req, status, [&](v8::Isolate * isolate, qdb_request * qdb_req) {
            const auto error_code = processErrorCode(isolate, status, qdb_req);
            auto success_count = v8::Number::New(isolate, static_cast<double>(qdb_req->output.batch.success_count));
            v8::Local<v8::Object> obj = v8::Object::New(isolate);

            for (const auto & op : qdb_req->output.batch.operations)
            {
                obj->Set(isolate->GetCurrentContext(), v8::String::NewFromUtf8(isolate, op.has_tag.tag, v8::NewStringType::kNormal).ToLocalChecked(),
                         v8::Boolean::New(isolate, qdb_e_ok == op.error));
            }

            return make_value_array(error_code, success_count, obj);
        });
    }

protected:
    template <typename F, typename... Params>
    static void
    queue_work(const v8::FunctionCallbackInfo<v8::Value> & args, F f, uv_after_work_cb after_work_cb, Params... p)
    {
        detail::queue_work(args, spawnRequest<F, Params...>, f, after_work_cb, p...);
    }

public:
    cluster_data_ptr cluster_data(void)
    {
        cluster_data_ptr res;
        {
            std::lock_guard<std::mutex> lock(_data_mutex);
            res = _cluster_data;
        }

        return res;
    }

private:
    std::mutex _data_mutex;
    cluster_data_ptr _cluster_data;
    std::unique_ptr<std::string> _alias;
};

} // namespace quasardb
