#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Types.hpp"

#include <chrono>
#include <functional>
#include <mutex>
#include <optional>
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
 * Direct entry eviction selects the globally oldest unpinned entry.
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
   * @struct AllocationOptions
   * @brief Specifies optional state for a new cache registration.
   */
  struct AllocationOptions
  {
    bool initiallyPinned = false; ///< Excludes the entry from eviction until a matching unpin() call.
  };

  /**
   * @enum PinResult
   * @brief Identifies the result of a budget-aware pin attempt.
   */
  enum class PinResult
  {
    Success,       ///< The manager added the pin.
    UnknownHandle, ///< The allocation is not registered.
    BudgetExceeded ///< Unique pinned bytes cannot fit within the current budget.
  };

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
   * @brief Registers an allocation with optional initial pin state.
   * @param subsystem Identifies the owning subsystem.
   * @param key Identifies the entry within its subsystem.
   *
   * @param[in] sizeBytes Allocation size in bytes.
   * @param onEvict Marks the entry for deferred removal after direct eviction.
   * @param options Specifies initial registration state.
   *
   * @return New handle and handles that direct entry eviction removed.
   * @note A pinned registration remains in usedBytes() but is not an eviction candidate.
   * @note An all-pinned cache can exceed the budget after this registration.
   */
  std::pair<AllocationHandle, std::vector<AllocationHandle>> allocate(const std::string& subsystem, const std::string& key, uint64 sizeBytes, EvictionCallback onEvict,
                                                                      const AllocationOptions& options);

  /**
   * @brief Registers a temporary pinned allocation if unique pinned bytes remain within the budget.
   * @param subsystem Identifies the requesting subsystem.
   * @param key Identifies the temporary allocation.
   * @param sizeBytes Specifies the reserved capacity in bytes.
   * @return New handle, or no value when pinned residency cannot admit the reservation.
   *
   * This method requests eviction before it registers the reservation.
   * A delegated subsystem can reconcile its accounting later.
   * The caller must call release() after the temporary allocation becomes inactive.
   */
  std::optional<AllocationHandle> reservePinned(const std::string& subsystem, const std::string& key, uint64 sizeBytes);

  /**
   * @brief Marks a registered allocation as most recently used.
   * @param handle Identifies the allocation. An unknown handle has no effect.
   */
  void touch(AllocationHandle handle);

  /**
   * @brief Increments the pin count for a registered allocation.
   * @param handle Identifies the allocation.
   * @return True if the handle exists.
   * @note The call updates recency.
   * Pinned entries remain registered but are not eviction candidates.
   */
  bool pin(AllocationHandle handle);

  /**
   * @brief Pins an allocation if total unique pinned bytes remain within the budget.
   * @param handle Identifies the allocation.
   * @return Result that distinguishes budget rejection from an unknown handle.
   *
   * A nested pin does not add unique pinned bytes and succeeds for a valid handle.
   * The check and first pin use one manager lock to prevent concurrent over-admission.
   */
  PinResult pinWithinBudget(AllocationHandle handle);

  /**
   * @brief Decrements the pin count for a registered allocation.
   * @param handle Identifies the allocation.
   * @return True if the handle exists. A zero pin count remains unchanged.
   *
   * @note The final matching call updates recency before the entry becomes an eviction candidate.
   */
  bool unpin(AllocationHandle handle);

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
   * @brief Returns bytes held by allocations with one or more pins.
   * @return Unique pinned allocation bytes. Nested pins do not duplicate the byte count.
   */
  uint64 pinnedBytes() const;

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
   * @brief Stores one cache registration, pin count, and direct-eviction callback.
   */
  struct Entry
  {
    std::string subsystem;
    std::string key;
    uint64 sizeBytes = 0;
    uint64 pinCount = 0;
    std::chrono::steady_clock::time_point lastAccessed;
    EvictionCallback onEvict;
  };

  /**
   * @brief Makes room for a new registration.
   * @param needed Required bytes for the new registration.
   * @return Handles removed through direct entry eviction.
   * @pre m_Mutex is held.
   *
   * Direct eviction removes globally oldest unpinned entries. A delegated handler
   * records one byte request and stops until its subsystem releases entries.
   * An all-pinned cache can exceed the
   * budget after the new registration.
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
  uint64 m_PinnedBytes = 0;
  AllocationHandle m_NextHandle = 1;
};

} // namespace nx::core
