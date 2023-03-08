#pragma once

namespace complex
{
template <typename Iter>
typename Iter::difference_type distance(Iter first, Iter last)
{
  return last - first;
}
} // namespace complex
