#ifndef UTIL_H
#define UTIL_H

#include <string>

#include "market_data.h"
#include "error.h"

namespace livermore::tx
{
namespace util
{
void copy_from(market_data *dst, const market_data *src);
void reset(market_data *md);
bool is_equal(const market_data *lhs,
              const market_data *rhs,
              std::string       &memo);

err_t parse(std::vector<std::string> &instruments, const std::string &filepath);

} // namespace util
} // namespace livermore::tx

#endif