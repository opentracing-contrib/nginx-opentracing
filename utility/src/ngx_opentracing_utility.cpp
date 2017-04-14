#include <ngx_opentracing_utility.h>
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
}  // namespace ngx_opentracing
