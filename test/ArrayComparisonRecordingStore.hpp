#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataStore.hpp"

#include <utility>
#include <vector>

namespace nx::core::UnitTest::ArrayComparison
{
inline constexpr int32 k_InjectedReadError = -98781;
inline constexpr int32 k_InjectedReadWarning = 98782;

/**
 * @class RecordingStore
 * @brief Records real resident reads and can fail one selected call.
 * @tparam T Stored scalar type.
 */
template <typename T>
class RecordingStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  ~RecordingStore() override = default;
  RecordingStore(const RecordingStore&) = default;
  RecordingStore(RecordingStore&&) = default;
  RecordingStore& operator=(const RecordingStore&) = delete;
  RecordingStore& operator=(RecordingStore&&) = default;

  mutable std::vector<std::pair<usize, usize>> reads;
  usize failingReadCall = 0;
  bool warnOnRead = false;

  /**
   * @brief Records each range before reading values or returning the selected error.
   * @param startIndex First flat value index.
   * @param buffer Receives real resident values on success.
   * @return Known injected error or the resident read result, with an optional known warning.
   */
  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    reads.emplace_back(startIndex, buffer.size());
    auto result = reads.size() == failingReadCall ? MakeErrorResult(k_InjectedReadError, "Injected floating-array read error") : DataStore<T>::copyIntoBuffer(startIndex, buffer);
    if(warnOnRead)
    {
      result.warnings().push_back({k_InjectedReadWarning, "Injected floating-array read warning"});
    }
    return result;
  }
};
} // namespace nx::core::UnitTest::ArrayComparison
