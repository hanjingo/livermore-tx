#ifndef QUOTE_H
#define QUOTE_H

#include <vector>
#include <unordered_map>
#include <functional>

#include "error.h"
#include "market_data.h"
#include "util.h"
#include "shm.h"

namespace livermore::tx
{

static const char _tx_addr_base[] = "http://sqt.gtimg.cn";

class quote
{
  public:
    using market_data_shm                  = shm<market_data>;
    using fn_subscribe_market_data_rsp_t   = std::function<void()>;
    using fn_unsubscribe_market_data_rsp_t = std::function<void()>;
    using fn_merket_data_ntf_t =
        std::function<void(std::vector<market_data_shm *> &md)>;

  public:
    quote(int interval_ms = 500)
        : _query_interval_ms(interval_ms) {};
    ~quote();
    err_t init();
    err_t subscribe_market_data(const std::vector<std::string> &instruments);
    err_t unsubscribe_market_data(const std::vector<std::string> &instruments);
    err_t wait(int ms = -1);

  public:
    virtual void register_cb();
    virtual void on_subscribe_market_data_rsp();
    virtual void on_unsubscribe_market_data_rsp();
    virtual void on_market_data_ntf(std::vector<market_data_shm *> &mds);

  private:
    err_t       _query_md(int ms = 500);
    std::string _parse_id(const std::string &body);
    bool        _parse_md(const std::string &body, market_data_shm *md);

  private:
    std::string                                        _addr = "/q=";
    std::unordered_map<std::string, market_data_shm *> _m_md;
    int                                                _query_interval_ms;

    fn_subscribe_market_data_rsp_t   _on_sub_rsp;
    fn_unsubscribe_market_data_rsp_t _on_unsub_rsp;
    fn_merket_data_ntf_t             _on_md_ntf;
};

} // namespace livermore::tx

#endif // quote_H