#include "livermore_tx.h"

#include <hj/util/once.hpp>
#include <hj/sync/object_pool.hpp>
#include <hj/util/defer.hpp>

#include "util.h"
#include "quote.h"

#include <iostream>

static err_handler g_err_handler = nullptr;
static hj::safe_map<std::string, livermore::tx::quote *> g_quote_pool;

C_STYLE_EXPORT void livermore_tx_version(sdk_context ctx)
{
    if(sizeof(ctx) != ctx.sz)
        return;

    if(ctx.user_data == NULL)
        return;

    auto ret   = static_cast<livermore_tx_param_version *>(ctx.user_data);
    ret->major = MAJOR_VERSION;
    ret->minor = MINOR_VERSION;
    ret->patch = PATCH_VERSION;

    if(ctx.cb != NULL)
        ctx.cb(static_cast<void *>(ret));
}

C_STYLE_EXPORT void livermore_tx_init(sdk_context ctx)
{
    if(sizeof(ctx) != ctx.sz)
        return;

    ONCE(if(ctx.user_data == NULL) return;

         auto ret      = static_cast<livermore_tx_param_init *>(ctx.user_data);
         ret->result   = OK;
         g_err_handler = ret->err_fn;
         if(ctx.cb != NULL) ctx.cb(static_cast<void *>(ret));)
}

C_STYLE_EXPORT void livermore_tx_quit(sdk_context ctx)
{
    if(sizeof(ctx) != ctx.sz)
        return;

    if(ctx.user_data == NULL)
        return;

    ONCE(
        // cleanup
        auto ret    = static_cast<livermore_tx_param_quit *>(ctx.user_data);
        ret->result = OK;

        g_err_handler = nullptr;
        g_quote_pool.range(
            [](const std::string &key, livermore::tx::quote *&q) {
                delete q;
                return true;
            });
        g_quote_pool.clear();

        if(ctx.cb != NULL) ctx.cb(static_cast<void *>(ret));)
}

C_STYLE_EXPORT void livermore_tx_subscribe(sdk_context ctx)
{
    if(sizeof(ctx) != ctx.sz)
        return;

    auto ret    = static_cast<livermore_tx_param_subscribe *>(ctx.user_data);
    ret->result = OK;
    if(ret == nullptr || ret->instrument == nullptr || ret->md_fn == nullptr)
    {
        ret->result = TX_ERR_INVALID_PARAM;
        return;
    }

    if(g_err_handler == nullptr)
    {
        ret->result = TX_ERR_NOT_INITIALIZED;
        return;
    }

    std::string topic(ret->instrument);
    if(g_quote_pool.find(topic) != std::nullopt)
    {
        ret->result = TX_ERR_ALREADY_INITIALIZED;
        return;
    }

    g_quote_pool.insert(topic, new livermore::tx::quote(topic, ret->md_fn));
    if(ctx.cb != NULL)
        ctx.cb(static_cast<void *>(ret));
}

C_STYLE_EXPORT void livermore_tx_unsubscribe(sdk_context ctx)
{
    if(sizeof(ctx) != ctx.sz)
        return;

    auto ret    = static_cast<livermore_tx_param_unsubscribe *>(ctx.user_data);
    ret->result = OK;
    if(ret == nullptr || ret->instrument == nullptr)
    {
        ret->result = TX_ERR_INVALID_PARAM;
        return;
    }

    std::string topic(ret->instrument);
    auto        elem = g_quote_pool.find(topic);
    if(elem == std::nullopt)
    {
        ret->result = OK;
        return;
    }

    delete elem.value();
    g_quote_pool.erase(std::move(topic));
    if(ctx.cb != NULL)
        ctx.cb(static_cast<void *>(ret));
}

C_STYLE_EXPORT void livermore_tx_query(sdk_context ctx)
{
    if(sizeof(ctx) != ctx.sz)
        return;

    auto ret = static_cast<livermore_tx_param_query *>(ctx.user_data);
    if(ret == nullptr || ret->instrument == nullptr)
    {
        ret->result = TX_ERR_INVALID_PARAM;
        return;
    }
    ret->result = OK;

    auto cli = g_quote_pool.find(std::string(ret->instrument));
    if(cli == std::nullopt || cli.value() == nullptr)
    {
        ret->result = TX_ERR_NOT_SUBSCRIBED;
        return;
    }
    ret->result = cli.value()->query();
    ret->md     = static_cast<void *>(cli.value()->md());
}