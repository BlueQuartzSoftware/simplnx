#pragma once

#include "simplnx/Common/Result.hpp"

#include <algorithm>
#include <iterator>
#include <limits>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace nx::core
{
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
// Test-only, thread-local fault injection. A negative count disables the hook.
SIMPLNX_EXPORT void SetWindingWarningFailureForTesting(int count) noexcept;
SIMPLNX_EXPORT bool WindingWarningFailureConsumedForTesting() noexcept;
SIMPLNX_EXPORT void MaybeFailWindingWarningForTesting();
#endif
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

/** @brief Adds optional diagnostics without losing an existing terminal error if allocation fails. */
template <class F>
void EnrichWindingError(F&& enrich)
{
  try
  {
    std::forward<F>(enrich)();
  } catch(const std::bad_alloc&)
  {
    return;
  } catch(const std::length_error&)
  {
    return;
  }
}

/**
 * @class WindingTransferWarnings
 * @brief Retains the first 64 distinct transfer warnings and summarizes later occurrences.
 * @note This bounds retained entries, not individual message lengths or provider allocations.
 */
class WindingTransferWarnings
{
public:
  /** @brief Moves warnings from one targeted operation into the bounded accumulator. @param source Supplies the operation warnings. */
  void append(std::vector<Warning>& source
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
              ,
              bool errorEnrichment = false
#endif
  )
  {
    static_assert(std::is_nothrow_move_assignable_v<Warning>);
    usize consumed = 0;
    try
    {
      for(auto& warning : source)
      {
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
        if(errorEnrichment)
        {
          MaybeFailWindingWarningForTesting();
        }
#endif
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
        ++consumed;
      }
    } catch(...)
    {
      source.erase(source.begin(), source.begin() + static_cast<std::vector<Warning>::difference_type>(consumed));
      throw;
    }
    source.clear();
  }

  /** @brief Retains warnings from a failed transfer. */
  void appendOnError(std::vector<Warning>& source)
  {
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
    append(source, true);
#else
    append(source);
#endif
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
