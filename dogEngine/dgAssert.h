#ifndef ASSERT_H
#define ASSERT_H
#include "dgLog.hpp"

namespace dg {
#define DGASSERT(condition)                                                                        \
  if (!(condition)) { DG_ERROR("Assert failed in line: {} of file: {}.", __LINE__, __FILE__); }
}// namespace dg

#endif//ASSERT_H