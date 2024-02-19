#ifndef DGUTIL_HPP
#define DGUTIL_HPP
#include <stddef.h>// for size_t
namespace dg {
template<class integral>
constexpr bool is_aligned(integral x, size_t a) noexcept {
  return (x & (integral(a) - 1)) == 0;
}

template<class integral>
constexpr integral align_up(integral x, size_t a) noexcept {
  return integral((x + (integral(a) - 1)) & ~integral(a - 1));
}

template<class integral>
constexpr integral align_down(integral x, size_t a) noexcept {
  return integral(x & ~integral(a - 1));
}
}// namespace dg

#endif//DGUTIL_HPP