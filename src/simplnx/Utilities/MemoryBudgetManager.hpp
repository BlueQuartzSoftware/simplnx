#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Types.hpp"

#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace nx::core
{

/**
 * @brief Unified memory budget manager for cache subsystems across simplnx and visualization code.
 *
 * All cache subsystems (ChunkCache, stride cache, partition cache) register their
 * allocations with this singleton. When memory pressure exceeds the budget, the
 * manager evicts the globally-oldest entry regardless of which subsystem owns it.
 *
 * Thread-safe: allocate/touch/release can be called from any thread.
 *
 * Eviction callbacks are invoked under the manager's mutex and MUST be non-blocking
 * (mark data for removal, don't do I/O or VTK operations). Callers receive the list
 * of evicted handles after the mutex is released for post-eviction cleanup.
 */
class SIMPLNX_EXPORT MemoryBudgetManager
{
public:
  using AllocationHandle = uint64;

  /**
   * @brief Callback invoked when an entry is evicted.
   *
   * The callback is invoked under the manager's internal mutex.
   * Callbacks MUST NOT call allocate/touch/release on this manager -- that
   * would deadlock (the mutex is not recursive). Only mark state for removal;
   * do no I/O.
   */
  using EvictionCallback = std::function<void()>;

  /**
   * @brief Returns the singleton instance.
   */
  static MemoryBudgetManager& instance();

  /**
   * @brief Returns a default budget of 50% of system RAM, clamped to a minimum of 1 GB.
   */
  static uint64 defaultBudgetBytes();

  /**
   * @brief Allocates a tracked entry. Evicts oldest entries if needed to stay within budget.
   * @param subsystem Name of the owning subsystem (e.g. "chunk", "stride", "partition")
   * @param key Subsystem-specific key for identification
   * @param sizeBytes Size of the allocation in bytes
   * @param onEvict Callback invoked when this entry is evicted (must be non-blocking)
   * @return Pair of (new handle, list of evicted handles for post-eviction cleanup)
   */
  std::pair<AllocationHandle, std::vector<AllocationHandle>> allocate(const std::string& subsystem, const std::string& key, uint64 sizeBytes, EvictionCallback onEvict);

  /**
   * @brief Updates the last-accessed timestamp of an allocation.
   * @param handle The allocation handle to touch
   */
  void touch(AllocationHandle handle);

  /**
   * @brief Voluntarily releases an allocation.
   * @param handle The allocation handle to release
   */
  void release(AllocationHandle handle);

  /**
   * @brief Sets the memory budget in bytes.
   */
  void setBudgetBytes(uint64 bytes);

  /**
   * @brief Returns the current memory budget in bytes.
   */
  uint64 budgetBytes() const;

  /**
   * @brief Returns the current total memory usage in bytes.
   */
  uint64 usedBytes() const;

  /**
   * @brief Clears all tracked entries and resets used bytes to zero.
   *
   * Intended for tests that share the mutable singleton -- call at the start
   * of each test case so leaked entries from a prior failure do not pollute
   * budget accounting.
   */
  void clear();

private:
  MemoryBudgetManager();
  ~MemoryBudgetManager() = default;

  MemoryBudgetManager(const MemoryBudgetManager&) = delete;
  MemoryBudgetManager& operator=(const MemoryBudgetManager&) = delete;

  struct Entry
  {
    std::string subsystem;
    std::string key;
    uint64 sizeBytes = 0;
    std::chrono::steady_clock::time_point lastAccessed;
    /// Eviction callback. MUST NOT call allocate/touch/release on this
    /// manager -- that would deadlock. Only mark state for removal; do no I/O.
    EvictionCallback onEvict;
  };

  /**
   * @brief Evicts the oldest entries until m_UsedBytes + needed <= m_BudgetBytes.
   * Must be called with m_Mutex held.
   * @param needed Number of bytes needed for a new allocation
   * @return List of evicted handles
   */
  std::vector<AllocationHandle> makeRoom(uint64 needed);

  mutable std::mutex m_Mutex;
  std::unordered_map<AllocationHandle, Entry> m_Entries;
  uint64 m_BudgetBytes = 0;
  uint64 m_UsedBytes = 0;
  AllocationHandle m_NextHandle = 1;
};

} // namespace nx::core
