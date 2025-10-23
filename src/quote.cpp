#include "quote.h"

#include <hj/log/logger.hpp>
#include <hj/net/http/http_client.hpp>
#include <hj/util/string_util.hpp>
#include <hj/encoding/fmt.hpp>
#include <hj/encoding/bytes.hpp>

namespace livermore::tx
{
quote::~quote()
{
    for(auto itr = _m_md.begin(); itr != _m_md.end(); ++itr)
    {
        delete itr->second;
        itr->second = nullptr;
    }
}

err_t quote::init()
{
    register_cb();

    return OK;
}

err_t quote::subscribe_market_data(const std::vector<std::string> &instruments)
{
    for(int i = 0; i < instruments.size(); ++i)
    {
        if(_m_md.find(instruments[i]) != _m_md.end())
            continue;

        _m_md.emplace(
            instruments[i],
            new shm<market_data>(instruments[i], sizeof(market_data)));
        _addr.append(instruments[i]);
        _addr.append(",");
        LOG_DEBUG("sub topic={}", instruments[i]);
    }

    _addr = hj::string_util::trim_right(_addr, ",");
    LOG_DEBUG("subscribe_market_data succ with _addr={}", _addr);
    return OK;
}

err_t quote::unsubscribe_market_data(
    const std::vector<std::string> &instruments)
{
    for(int i = 0; i < instruments.size(); ++i)
    {
        if(_m_md.find(instruments[i]) == _m_md.end())
            continue;

        delete _m_md[instruments[i]];
        _m_md[instruments[i]] = nullptr;
        _m_md.erase(instruments[i]);
        LOG_DEBUG("unsub topic={}", instruments[i]);
    }

    // reset _addr
    _addr = "/q=";
    for(auto itr = _m_md.begin(); itr != _m_md.end(); ++itr)
    {
        _addr.append(itr->first);
        _addr.append(",");
        LOG_DEBUG("_addr={}", _addr);
    }

    _addr = hj::string_util::trim_right(_addr, ",");
    LOG_DEBUG("unsubscribe_market_data succ with _addr={}", _addr);
    return OK;
}

err_t quote::wait(int ms)
{
    std::chrono::milliseconds interval{1};
    while(ms > 0 || ms == -1)
    {
        _query_md(_query_interval_ms);
        std::this_thread::sleep_for(interval);
        ms = (ms > 0 ? ms - 1 : ms);
    }

    return OK;
}

err_t quote::_query_md(int ms)
{
    hj::http_client cli{_tx_addr_base};
    auto            resp = cli.Get(_addr);
    LOG_TRACE("_query_md with body: {}", resp->body);
    auto instruments = hj::string_util::split(resp->body, ";");
    std::vector<shm<market_data> *> mds;
    std::string                     id;
    for(std::string str : instruments)
    {
        id       = _parse_id(str);
        auto itr = _m_md.find(id);
        if(itr == _m_md.end())
            continue;

        if(!_parse_md(str, itr->second))
            continue;

        mds.push_back(itr->second);
    }
    _on_md_ntf(mds);
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
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

bool quote::_parse_md(const std::string &body, shm<market_data> *md)
{
    util::reset(md->data());
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
    hj::string_to_bytes(md->data()->exchange_id, sz, id);
    sz = 21;
    hj::string_to_bytes(md->data()->instrument_name, sz, params[1]);
    sz = 31;
    hj::string_to_bytes(md->data()->instrument_id, sz, params[2]);
    md->data()->last_price      = std::stod(params[3]);
    md->data()->pre_close_price = std::stod(params[4]);
    md->data()->open_price      = std::stod(params[5]);
    md->data()->volume          = std::stod(params[6]) * 100.0;

#ifdef USE_DEEP_DATA
    md->data()->bid_price1  = std::stod(params[9]);
    md->data()->bid_volume1 = std::stod(params[10]);
    md->data()->bid_price2  = std::stod(params[11]);
    md->data()->bid_volume2 = std::stod(params[12]);
    md->data()->bid_price3  = std::stod(params[13]);
    md->data()->bid_volume3 = std::stod(params[14]);
    md->data()->bid_price4  = std::stod(params[15]);
    md->data()->bid_volume4 = std::stod(params[16]);
    md->data()->bid_price5  = std::stod(params[17]);
    md->data()->bid_volume5 = std::stod(params[18]);
    md->data()->ask_price1  = std::stod(params[19]);
    md->data()->ask_volume1 = std::stod(params[20]);
    md->data()->ask_price2  = std::stod(params[21]);
    md->data()->ask_volume2 = std::stod(params[22]);
    md->data()->ask_price3  = std::stod(params[23]);
    md->data()->ask_volume3 = std::stod(params[24]);
    md->data()->ask_price4  = std::stod(params[25]);
    md->data()->ask_volume4 = std::stod(params[26]);
    md->data()->ask_price5  = std::stod(params[27]);
    md->data()->ask_volume5 = std::stod(params[28]);
#endif

    sz = 9;
    hj::string_to_bytes(md->data()->trading_day, sz, params[29].substr(0, 8));
    sz          = 9;
    auto tm_str = hj::fmt("{}:{}:{}",
                          params[29].substr(8, 2),
                          params[29].substr(10, 2),
                          params[29].substr(12, 2));
    hj::string_to_bytes(md->data()->action_time, sz, tm_str);
    md->data()->highest_price = std::stod(params[33]);
    md->data()->lowest_price  = std::stod(params[34]);
    md->data()->turnover      = std::stod(params[37]) * 10000.0;
    LOG_DEBUG("parse_md succ");
    return true;
}

void quote::register_cb()
{
    _on_sub_rsp   = std::bind(&quote::on_subscribe_market_data_rsp, this);
    _on_unsub_rsp = std::bind(&quote::on_unsubscribe_market_data_rsp, this);
    _on_md_ntf =
        std::bind(&quote::on_market_data_ntf, this, std::placeholders::_1);
}

void quote::on_subscribe_market_data_rsp()
{
    LOG_DEBUG("on_subscribe_market_data_rsp");
}

void quote::on_unsubscribe_market_data_rsp()
{
    LOG_DEBUG("on_unsubscribe_market_data_rsp");
}

void quote::on_market_data_ntf(std::vector<market_data_shm *> &mds)
{
    // LOG_DEBUG("on_market_data_ntf with mds.size() ={} ", mds.size());
    for(market_data_shm *md : mds)
    {
        md->write();
        // LOG_DEBUG("write market data to shared memory {}", util::fmt(md->data()));
    }
}

}