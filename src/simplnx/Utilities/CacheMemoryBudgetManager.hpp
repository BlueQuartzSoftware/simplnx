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
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

/**
 * @class CacheMemoryBudgetManager
 * @brief Coordinates one memory budget across registered cache subsystems.
 *
 * Cache subsystems register allocation size and recency with this singleton.
 * Direct entry eviction selects the globally oldest registered entry.
 * A delegated subsystem receives a non-blocking byte request and releases its
 * own entries later. Accounting can temporarily exceed the budget until that release.
 *
 * @note The budget covers only registered cache entries. It does not limit process
 * memory, resident arrays, working buffers, rendering allocations, or allocator-retained pages.
 *
 * All public methods are thread-safe.
 *
 * Eviction callbacks and delegated handlers run under the manager mutex. They
 * can only update an atomic or a leaf mark queue. They must not re-enter this manager.
 */
class SIMPLNX_EXPORT CacheMemoryBudgetManager
{
public:
  /**
   * @brief Defines an opaque registration identifier.
   */
  using AllocationHandle = uint64;

  /**
   * @brief Callback invoked when an entry is evicted.
   *
   * The callback runs under the manager mutex. It must only mark state for
   * later removal. Manager calls, structural cache locks, and I/O can deadlock or block allocation.
   * Captured objects must remain valid until the registration is released or cleared.
   */
  using EvictionCallback = std::function<void()>;

  /**
   * @brief Names a non-blocking request to free retained cache bytes.
   *
   * The manager invokes this handler while holding its mutex. The handler must
   * record the request. It must not take subsystem locks, perform I/O, or call
   * this manager. The subsystem later calls release() to reconcile accounting.
   * The argument requests the minimum number of retained bytes to release.
   */
  using SubsystemEvictionHandler = std::function<void(uint64 bytesToFree)>;

  /**
   * @brief Registers or replaces a delegated eviction handler for a cache subsystem.
   * @param subsystem Identifies the cache subsystem.
   * @param handler Replaces the stored handler for subsystem.
   * @pre handler is valid.
   *
   * A subsystem without a handler uses per-entry eviction callbacks.
   * Captured objects must remain valid until replacement or the end of cache activity.
   */
  void registerSubsystem(const std::string& subsystem, SubsystemEvictionHandler handler);

  /**
   * @brief Returns the singleton instance.
   * @return Process-wide cache budget manager.
   */
  static CacheMemoryBudgetManager& instance();

  /**
   * @brief Calculates the default cache budget.
   * @return Half of system RAM, with a 1-GiB floor and maxBudgetBytes() ceiling.
   */
  static uint64 defaultBudgetBytes();

  /**
   * @brief Calculates the maximum permitted cache budget.
   * @return Budget that reserves operating-system and application headroom.
   *
   * cap = max( min(totalRAM - 6 GiB, 0.95 * totalRAM), 1 GiB )
   *
   * The 6-GiB reserve applies below approximately 120 GiB. The 95-percent limit
   * applies above that value. Unknown or very small memory uses the 1-GiB floor.
   */
  static uint64 maxBudgetBytes();

  /**
   * @brief Registers an allocation and makes room under the shared budget.
   * @param subsystem Identifies the owning subsystem.
   * @param key Identifies the entry within its subsystem.
   * @param sizeBytes Specifies allocation size in bytes.
   * @param onEvict Marks the entry for deferred removal after direct eviction.
   * @return New handle and handles that direct entry eviction removed.
   *
   * A delegated handler can defer removal. In that case, this method registers
   * the new entry before the subsystem later calls release().
   * If no entry can be evicted, an allocation can temporarily exceed the budget.
   */
  std::pair<AllocationHandle, std::vector<AllocationHandle>> allocate(const std::string& subsystem, const std::string& key, uint64 sizeBytes, EvictionCallback onEvict);

  /**
   * @brief Marks a registered allocation as most recently used.
   * @param handle Identifies the allocation. An unknown handle has no effect.
   */
  void touch(AllocationHandle handle);

  /**
   * @brief Releases a registered allocation.
   * @param handle Identifies the allocation. An unknown handle has no effect.
   */
  void release(AllocationHandle handle);

  /**
   * @brief Sets the cache memory budget in bytes, clamped to maxBudgetBytes().
   *
   * Only the upper bound is clamped. Tests can set a value below the 1-GiB
   * default floor. Existing entries are not evicted until a later allocation.
   * @param bytes Requested budget in bytes.
   * @return True if the requested value exceeded the cap and was reduced.
   */
  bool setBudgetBytes(uint64 bytes);

  /**
   * @brief Returns the current cache memory budget in bytes.
   * @return Current shared budget in bytes.
   */
  uint64 budgetBytes() const;

  /**
   * @brief Returns the total bytes currently registered by cache subsystems.
   * @return Registered cache bytes. Deferred subsystem requests remain included until release().
   */
  uint64 usedBytes() const;

  /**
   * @brief Clears all tracked entries and resets used bytes to zero.
   *
   * This test utility does not invoke eviction callbacks, clear subsystem
   * handlers, reset the budget, or reuse allocation handles.
   */
  void clear();

private:
  CacheMemoryBudgetManager();
  ~CacheMemoryBudgetManager() = default;

  CacheMemoryBudgetManager(const CacheMemoryBudgetManager&) = delete;
  CacheMemoryBudgetManager& operator=(const CacheMemoryBudgetManager&) = delete;

  /**
   * @struct Entry
   * @brief Stores one cache registration and its direct-eviction callback.
   */
  struct Entry
  {
    std::string subsystem;
    std::string key;
    uint64 sizeBytes = 0;
    std::chrono::steady_clock::time_point lastAccessed;
    EvictionCallback onEvict;
  };

  /**
   * @brief Makes room for a new registration.
   * @param needed Required bytes for the new registration.
   * @return Handles removed through direct entry eviction.
   * @pre m_Mutex is held.
   *
   * Direct eviction removes globally oldest entries. A delegated handler records
   * one byte request and stops this pass until its subsystem releases entries.
   */
  std::vector<AllocationHandle> makeRoom(uint64 needed);

  /**
   * @brief Returns total physical system RAM in bytes (0 if it cannot be read).
   * @return Physical system RAM in bytes, or zero when detection fails.
   *
   * Delegates directly to the OS via Memory::GetTotalMemory(); it is not cached,
   * so do not call it on hot paths.
   */
  static uint64 totalSystemRamBytes();

  mutable std::mutex m_Mutex;
  std::unordered_map<AllocationHandle, Entry> m_Entries;
  std::unordered_map<std::string, SubsystemEvictionHandler> m_SubsystemHandlers;
  uint64 m_BudgetBytes = 0;
  uint64 m_UsedBytes = 0;
  AllocationHandle m_NextHandle = 1;
};

} // namespace nx::core
