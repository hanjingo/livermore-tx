#include "quote.h"

#include <thread>
#include <chrono>

#include <hj/log/logger.hpp>
#include <hj/net/http/http_client.hpp>
#include <hj/util/string_util.hpp>
#include <hj/encoding/fmt.hpp>
#include <fmt/core.h>
#include <hj/encoding/bytes.hpp>

namespace livermore::tx
{

quote::~quote()
{
    if(_md != nullptr)
    {
        delete _md;
        _md = nullptr;
    }

    _on_md_ntf = nullptr;
}

int quote::query()
{
    return _query_md();
}

int quote::_query_md()
{
    hj::http_client cli{_tx_addr_base};
    auto            resp = cli.Get(_addr);
    LOG_TRACE("_query_md with body: {}", resp->body);
    auto        instruments = hj::string_util::split(resp->body, ";");
    std::string id;
    for(std::string str : instruments)
    {
        id = _parse_id(str);
        if(_instrument != id)
            continue;

        if(!_parse_md(str, _md))
            continue;

        _on_md_ntf(_md);
    }

    return OK;
}

std::string quote::_parse_id(const std::string &body)
{
    // Extract instrument id from a body like: v_sz000001="..."
    // Accept optional leading "v_" and capture the id up to the '='.
    LOG_TRACE("_parse_id with body: {}", body);
    std::smatch match;
    // pattern: optional v_ prefix, then capture alnum/underscore sequence, followed by '='
    static const std::regex pattern(R"((?:v_)?([A-Za-z0-9_]+)=)");
    if(std::regex_search(body, match, pattern) && match.size() > 1)
    {
        LOG_TRACE("_parse_id matched: {}", match[1].str());
        return match[1].str();
    }

    return std::string();
}

bool quote::_parse_md(const std::string &body, market_data *md)
{
    util::reset(md);
    auto params = hj::string_util::split(body, "~");
    if(params.size() < 38)
        return false;

    LOG_TRACE("_parse_md with params size={}", params.size());
    for(auto itr = params.begin(); itr != params.end(); ++itr)
        *itr = hj::string_util::regex_replace(*itr, "~", "");

    LOG_TRACE("after clean, params:");
    for(size_t i = 0; i < params.size(); ++i)
        LOG_TRACE("param[{}]: {}", i, params[i]);

    // Extract instrument id (e.g. sz000001) from params[0]
    auto id = _parse_id(params[0]);
    if(id.empty())
        return false;

    LOG_TRACE("parsed market data for instrument id={}", id);
    size_t sz = 9;
    hj::string_to_bytes(md->exchange_id, sz, id);
    sz = 21;
    hj::string_to_bytes(md->instrument_name, sz, params[1]);
    sz = 31;
    hj::string_to_bytes(md->instrument_id, sz, params[2]);
    md->last_price      = std::stod(params[3]);
    md->pre_close_price = std::stod(params[4]);
    md->open_price      = std::stod(params[5]);
    md->volume          = std::stod(params[6]) * 100.0;

#ifdef USE_DEEP_DATA
    md->bid_price1  = std::stod(params[9]);
    md->bid_volume1 = std::stod(params[10]);
    md->bid_price2  = std::stod(params[11]);
    md->bid_volume2 = std::stod(params[12]);
    md->bid_price3  = std::stod(params[13]);
    md->bid_volume3 = std::stod(params[14]);
    md->bid_price4  = std::stod(params[15]);
    md->bid_volume4 = std::stod(params[16]);
    md->bid_price5  = std::stod(params[17]);
    md->bid_volume5 = std::stod(params[18]);
    md->ask_price1  = std::stod(params[19]);
    md->ask_volume1 = std::stod(params[20]);
    md->ask_price2  = std::stod(params[21]);
    md->ask_volume2 = std::stod(params[22]);
    md->ask_price3  = std::stod(params[23]);
    md->ask_volume3 = std::stod(params[24]);
    md->ask_price4  = std::stod(params[25]);
    md->ask_volume4 = std::stod(params[26]);
    md->ask_price5  = std::stod(params[27]);
    md->ask_volume5 = std::stod(params[28]);
#endif

    sz = 9;
    hj::string_to_bytes(md->trading_day, sz, params[29].substr(0, 8));
    sz          = 9;
    auto tm_str = fmt::format("{}:{}:{}",
                              params[29].substr(8, 2),
                              params[29].substr(10, 2),
                              params[29].substr(12, 2));
    hj::string_to_bytes(md->action_time, sz, tm_str);
    md->highest_price = std::stod(params[33]);
    md->lowest_price  = std::stod(params[34]);
    md->turnover      = std::stod(params[37]) * 10000.0;
    LOG_DEBUG("parse_md succ");
    return true;
}

}