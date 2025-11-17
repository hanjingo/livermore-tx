#include <iostream>

#include <hj/os/dll.h>
#include <hj/util/defer.hpp>

#include "../../src/livermore_tx.h"
#include "../../src/market_data.h"

void on_err(int err_code, const char *err_msg)
{
    std::cerr << "livermore-tx error [" << err_code << "]: " << err_msg
              << std::endl;
}

void on_md(void *md)
{
    if(md == nullptr)
        return;

    auto ptr = static_cast<market_data *>(md);
    std::cout << "on market data with instrument id: " << ptr->instrument_id
              << ", trading_day: " << ptr->trading_day
              << ", instrument_name: " << ptr->instrument_name
              << ", instrument_id: " << ptr->instrument_id
              << ", exchange_id: " << ptr->exchange_id
              << ", last_price: " << ptr->last_price
              << ", pre_close_price: " << ptr->pre_close_price
              << ", open_price: " << ptr->open_price
              << ", pre_settlement_price: " << ptr->pre_settlement_price
              << ", highest_price: " << ptr->highest_price
              << ", lowest_price: " << ptr->lowest_price
              << ", close_price: " << ptr->close_price
              << ", settlement_price: " << ptr->settlement_price
              << ", upper_limit_price: " << ptr->upper_limit_price
              << ", lower_limit_price: " << ptr->lower_limit_price << std::endl;
}

int main(int argc, char *argv[])
{
    std::cout << "livermore-tx example running..." << std::endl;

    std::string dll_path = std::string("./liblivermore-tx") + DLL_EXT;
    std::cout << "loading livermore-tx dynamic library from: " << dll_path
              << std::endl;
    auto dll = dll_open(dll_path.c_str(), DLL_RTLD_NOW);
    if(!dll)
    {
        std::cerr << "Failed to open livermore-tx dynamic library."
                  << std::endl;
        return -1;
    }
    DEFER(dll_close(dll);)

    void       *tmp = nullptr;
    sdk_context ctx;
    // version
    tmp = dll_get(dll, "livermore_tx_version");
    if(!tmp)
    {
        std::cerr << "Failed to get livermore_tx_version symbol." << std::endl;
        return -1;
    }
    auto                       version_fn = reinterpret_cast<sdk_api>(tmp);
    livermore_tx_param_version version_info;
    version_info.major = 0;
    version_info.minor = 0;
    version_info.patch = 0;
    ctx.sz             = sizeof(sdk_context);
    ctx.user_data      = &version_info;
    ctx.cb             = nullptr;
    version_fn(&ctx);
    std::cout << "livermore-tx version: " << version_info.major << "."
              << version_info.minor << "." << version_info.patch << std::endl;

    // init
    tmp = dll_get(dll, "livermore_tx_init");
    if(!tmp)
    {
        std::cerr << "Failed to get livermore_tx_init symbol." << std::endl;
        return -1;
    }
    auto                    init_fn = reinterpret_cast<sdk_api>(tmp);
    livermore_tx_param_init init_info;
    init_info.err_fn = on_err;
    init_info.md_fn  = on_md;
    init_info.result = -1;
    ctx.sz           = sizeof(sdk_context);
    ctx.user_data    = &init_info;
    ctx.cb           = nullptr;
    init_fn(&ctx);
    if(init_info.result != OK)
    {
        std::cerr << "Failed to initialize livermore-tx, err code: "
                  << init_info.result << std::endl;
        return -1;
    }
    std::cout << "livermore-tx initialized successfully." << std::endl;

    // subscribe
    tmp = dll_get(dll, "livermore_tx_subscribe");
    if(!tmp)
    {
        std::cerr << "Failed to get livermore_tx_subscribe symbol."
                  << std::endl;
        return -1;
    }
    auto                         subscribe_fn = reinterpret_cast<sdk_api>(tmp);
    livermore_tx_param_subscribe sub_info;
    sub_info.instrument = "sz000001";
    sub_info.md_fn      = on_md;
    sub_info.result     = -1;
    ctx.sz              = sizeof(sdk_context);
    ctx.user_data       = &sub_info;
    ctx.cb              = nullptr;
    subscribe_fn(&ctx);
    if(sub_info.result != OK)
    {
        std::cerr << "Failed to subscribe instrument, err code: "
                  << sub_info.result << std::endl;
        return -1;
    }
    std::cout << "subscribed instrument " << sub_info.instrument
              << " successfully." << std::endl;

    // query
    tmp = dll_get(dll, "livermore_tx_query");
    if(!tmp)
    {
        std::cerr << "Failed to get livermore_tx_query symbol." << std::endl;
        return -1;
    }
    auto                     query_fn = reinterpret_cast<sdk_api>(tmp);
    livermore_tx_param_query query_info;
    query_info.instrument = "sz000001";
    query_info.md         = nullptr;
    query_info.result     = -1;
    ctx.sz                = sizeof(sdk_context);
    ctx.user_data         = &query_info;
    ctx.cb                = nullptr;
    query_fn(&ctx);
    if(query_info.result != OK || query_info.md == nullptr)
    {
        std::cerr << "Failed to query instrument, err code: "
                  << query_info.result << std::endl;
        return -1;
    }
    market_data *md = static_cast<market_data *>(query_info.md);
    std::cout << "queried instrument " << query_info.instrument
              << " successfully, last price: " << md->last_price << std::endl;

    return 0;
}