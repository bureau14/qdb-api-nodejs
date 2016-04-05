
#pragma once

#include "entity.hpp"

namespace qdb
{

    template <typename Derivate>
    class ExpirableEntity : public Entity<Derivate>
    {

    public:
        ExpirableEntity(cluster_data_ptr cd, const char * alias) : Entity<Derivate>(cd, alias) {}
        virtual ~ExpirableEntity(void) {}

    public:
        template <typename F>
        static void Init(v8::Handle<v8::Object> exports, const char * className, F init)
        {
            Entity<Derivate>::Init(exports,
                className,
                [init](v8::Local<v8::FunctionTemplate> tpl)
                {
                    // call init function of derivate
                    init(tpl);

                     // add our expiry functions then hand over to entity
                    NODE_SET_PROTOTYPE_METHOD(tpl, "expiresAt",        ExpirableEntity<Derivate>::expiresAt);
                    NODE_SET_PROTOTYPE_METHOD(tpl, "expiresFromNow",   ExpirableEntity<Derivate>::expiresFromNow);
                    NODE_SET_PROTOTYPE_METHOD(tpl, "getExpiry",        ExpirableEntity<Derivate>::getExpiry);
                });
        }

    public:
        static void expiresAt(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity<Derivate>::queue_work(args,
                [](qdb_request * qdb_req)
                {
                    qdb_req->output.error = qdb_expires_at(qdb_req->handle(),
                        qdb_req->input.alias.c_str(),
                        qdb_req->input.expiry);
                },
                Entity<Derivate>::processVoidResult,
                &ArgsEaterBinder::expiry);
        }

        static void expiresFromNow(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity<Derivate>::queue_work(args,
                [](qdb_request * qdb_req)
                {
                    qdb_req->output.error = qdb_expires_from_now(qdb_req->handle(),
                        qdb_req->input.alias.c_str(),
                        static_cast<qdb_int_t>(qdb_req->input.content.value));
                },
                Entity<Derivate>::processVoidResult,
                &ArgsEaterBinder::integer);
        }

        static void getExpiry(const v8::FunctionCallbackInfo<v8::Value> & args)
        {
            Entity<Derivate>::queue_work(args,
                [](qdb_request * qdb_req)
                {
                    qdb_req->output.error = qdb_get_expiry_time(qdb_req->handle(),
                        qdb_req->input.alias.c_str(),
                        &(qdb_req->output.content.date));
                },
                Entity<Derivate>::processDateResult,
                &ArgsEaterBinder::integer);
        }

    };

};
