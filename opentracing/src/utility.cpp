#include "utility.h"
#include <algorithm>
#include <cctype>
#include <string>

namespace ngx_opentracing {
//------------------------------------------------------------------------------
// to_ngx_str
//------------------------------------------------------------------------------
ngx_str_t to_ngx_str(ngx_pool_t *pool, const std::string &s) {
  ngx_str_t result;
  result.data = static_cast<unsigned char *>(ngx_palloc(pool, s.size()));
  if (!result.data) return {0, nullptr};
  result.len = s.size();
  std::copy(s.begin(), s.end(), result.data);
  return result;
}

//------------------------------------------------------------------------------
// to_lower_ngx_str
//------------------------------------------------------------------------------
ngx_str_t to_lower_ngx_str(ngx_pool_t *pool, const std::string &s) {
  ngx_str_t result;
  result.data = reinterpret_cast<unsigned char *>(ngx_palloc(pool, s.size()));
  if (!result.data) return {0, nullptr};
  result.len = s.size();
  std::transform(s.begin(), s.end(), result.data,
                 [](char c) { return std::tolower(c); });
  return result;
}

//------------------------------------------------------------------------------
// to_system_timestamp
//------------------------------------------------------------------------------
std::chrono::system_clock::time_point to_system_timestamp(
    time_t epoch_seconds, ngx_msec_t epoch_milliseconds) {
  auto epoch_duration = std::chrono::seconds{epoch_seconds} +
                        std::chrono::milliseconds{epoch_milliseconds};
  return std::chrono::system_clock::from_time_t(std::time_t{0}) +
         epoch_duration;
}
}  // namespace ngx_opentracing
