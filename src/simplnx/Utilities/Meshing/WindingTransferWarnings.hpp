#pragma once

#include "simplnx/Common/Result.hpp"

#include <algorithm>
#include <iterator>
#include <limits>
#include <vector>

#include <fmt/format.h>

namespace nx::core
{
/**
 * @brief Increments a warning count without wrapping.
 * @param count Supplies the current count.
 * @return The incremented count, or the maximum if already saturated.
 */
constexpr uint64 IncrementWindingWarningCount(uint64 count) noexcept
{
  return count == std::numeric_limits<uint64>::max() ? count : count + 1;
}
static_assert(IncrementWindingWarningCount(0) == 1);
static_assert(IncrementWindingWarningCount(std::numeric_limits<uint64>::max() - 1) == std::numeric_limits<uint64>::max());
static_assert(IncrementWindingWarningCount(std::numeric_limits<uint64>::max()) == std::numeric_limits<uint64>::max());

/**
 * @class WindingTransferWarnings
 * @brief Retains the first 64 distinct transfer warnings and summarizes later occurrences.
 * @note This bounds retained entries, not individual message lengths or provider allocations.
 */
class WindingTransferWarnings
{
public:
  /** @brief Moves warnings from one targeted operation into the bounded accumulator. @param source Supplies the operation warnings. */
  void append(std::vector<Warning>& source)
  {
    for(auto& warning : source)
    {
      const auto duplicate = std::find_if(m_Warnings.begin(), m_Warnings.end(), [&warning](const Warning& retained) { return retained.code == warning.code && retained.message == warning.message; });
      if(duplicate != m_Warnings.end())
      {
        m_Repeated = IncrementWindingWarningCount(m_Repeated);
      }
      else if(m_Warnings.size() < k_MaxWarnings)
      {
        m_Warnings.push_back(std::move(warning));
      }
      else
      {
        m_Additional = IncrementWindingWarningCount(m_Additional);
      }
    }
    source.clear();
  }

  /** @brief Prepends retained details and any summary to the returned result. @param result Receives the warnings. */
  void attach(Result<>& result)
  {
    if(m_Repeated != 0 || m_Additional != 0)
    {
      const auto countText = [](uint64 count) { return count == std::numeric_limits<uint64>::max() ? fmt::format("{} or more", count) : fmt::format("{}", count); };
      m_Warnings.push_back({.code = -65796,
                            .message = fmt::format("Triangle winding retained at most 64 distinct transfer warnings; suppressed {} repeated and {} additional warning occurrences.",
                                                   countText(m_Repeated), countText(m_Additional))});
    }
    if(!m_Warnings.empty())
    {
      result.warnings().insert(result.warnings().begin(), std::make_move_iterator(m_Warnings.begin()), std::make_move_iterator(m_Warnings.end()));
    }
  }

private:
  static constexpr usize k_MaxWarnings = 64;
  std::vector<Warning> m_Warnings;
  uint64 m_Repeated = 0;
  uint64 m_Additional = 0;
};

} // namespace nx::core
