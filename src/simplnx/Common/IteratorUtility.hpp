#pragma once

namespace nx::core
{
template <typename Iter>
typename Iter::difference_type distance(Iter first, Iter last)
{
  return last - first;
}
} // namespace nx::core
