#ifndef QUOTE_H
#define QUOTE_H

#include <vector>
#include <functional>

#include <hj/sync/safe_map.hpp>

#include "util.h"
#include "market_data.h"
#include "livermore_tx.h"

namespace livermore::tx
{

static const char _tx_addr_base[] = "http://sqt.gtimg.cn";

class quote
{
  public:
    quote(const std::string &instrument, md_handler fn, int interval_ms = 500)
        : _instrument{instrument}
        , _addr{std::string(_tx_addr_base) + "/q=" + instrument}
        , _md{new market_data()}
        , _on_md_ntf{fn}
        , _interval_ms(interval_ms) {};
    ~quote();
    inline market_data *md() { return _md; }
    int                 query();

  private:
    int         _query_md();
    std::string _parse_id(const std::string &body);
    bool        _parse_md(const std::string &body, market_data *md);

  private:
    std::string  _instrument;
    std::string  _addr;
    market_data *_md = nullptr;
    md_handler   _on_md_ntf;
    int          _interval_ms;
};

} // namespace livermore::tx

#endif