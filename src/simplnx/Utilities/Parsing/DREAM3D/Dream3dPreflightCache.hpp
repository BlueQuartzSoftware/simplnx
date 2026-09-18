#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/simplnx_export.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <map>
#include <mutex>
#include <optional>
#include <string>

namespace nx::core::DREAM3D
{
/**
 * @class Dream3dPreflightCache
 * @brief Caches metadata-only DataStructures from DREAM3D files.
 *
 * Parameter edits can preflight the same ReadDREAM3DFilter many times. An HDF5
 * metadata traversal performs many small reads. Network storage can make that
 * traversal slow. A valid cache hit needs only filesystem metadata checks.
 *
 * A canonical path identifies each entry. File size and modification time form
 * its validation token. Recently modified files bypass the cache because a
 * network filesystem can round modification times.
 *
 * A fetch returns a DataStructure copy with independent array stores. Therefore,
 * one preflight cannot mutate the cached master or another handout. Ordinary
 * numeric datasets remain metadata placeholders. Small legacy Statistics
 * attributes remain populated because their values have no deferred reload path.
 *
 * The singleton supports concurrent callers. It serializes complete HDF5 imports
 * and protects the in-memory entry table with a separate mutex.
 */
class SIMPLNX_EXPORT Dream3dPreflightCache
{
public:
  /**
   * @brief Specifies the maximum entry count.
   *
   * Insertion evicts the least-recently-used entry when the table exceeds this value.
   * The limit bounds long-session bookkeeping, not bulk data memory.
   */
  static constexpr usize k_Capacity = 8;

  /**
   * @brief Specifies the modification-time trust window.
   *
   * A younger file bypasses the cache. This prevents a same-size rewrite from
   * hiding inside coarse SMB or NFS timestamp resolution.
   */
  static constexpr std::chrono::seconds k_MtimeTrustWindow{2};

  /**
   * @brief Gets the process-wide cache.
   * @return Shared singleton instance.
   *
   * All filter and pipeline instances share entries across preflight worker threads.
   */
  static Dream3dPreflightCache& Instance();

  /**
   * @brief Gets an isolated metadata-only DataStructure for one file.
   * @param filePath Identifies the DREAM3D file.
   * @return Independent handout, or the underlying file-open or import error.
   *
   * A matching trusted token serves a cache hit. Other cases serialize a disk
   * import. Failed and recently modified reads are not cached.
   */
  Result<DataStructure> fetch(const std::filesystem::path& filePath);

  /**
   * @brief Removes one file entry if present.
   * @param filePath Identifies the entry through canonical path resolution.
   *
   * The next fetch reads the file even if its validation token did not change.
   */
  void invalidate(const std::filesystem::path& filePath);

  /**
   * @brief Removes all entries for test or application reset.
   */
  void clear();

  /**
   * @brief Gets cache hits since resetStats().
   * @return Atomic hit count.
   *
   * Tests use counters instead of timing to verify I/O behavior deterministically.
   */
  uint64 hitCount() const;

  /**
   * @brief Gets disk-read attempts since resetStats().
   * @return Atomic miss count.
   */
  uint64 missCount() const;

  /**
   * @brief Resets hit and miss counters for test isolation.
   *
   * Concurrent fetches can increment a counter during the two independent stores.
   */
  void resetStats();

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
  /**
   * @brief Forces the calling thread's filesystem metadata lookup to fail.
   * @param forceFailure True to exercise the readable-file stat bypass.
   * @note Available only in test builds. The setting is thread-local and does not change cache state.
   */
  static void SetForceFileMetadataFailure(bool forceFailure);
#endif

private:
  Dream3dPreflightCache() = default;

  /**
   * @brief Replaces each array store with an independent handout store.
   * @param dataStructure Supplies the handout to isolate in place.
   * @return Planning and numeric replacement warnings or errors.
   *
   * DataStructure copy construction shares DataObject stores. Numeric placeholders
   * receive fresh plans, populated numeric stores receive policy-selected value
   * copies, and list/string stores receive independent copies.
   */
  static Result<> RefreshStores(DataStructure& dataStructure);

  /**
   * @brief Applies current policy and store isolation to a copied handout.
   * @param handout Supplies the copied metadata graph.
   * @param filePath Identifies the source file for error context.
   * @return Prepared handout or contextual refresh error.
   */
  static Result<DataStructure> PrepareHandout(DataStructure handout, const std::filesystem::path& filePath);

  /**
   * @brief Serves one valid cached entry and refreshes its recency.
   * @param key Specifies the canonical file key.
   * @param fileSize Specifies the current file size.
   * @param mtime Specifies the current modification time.
   * @param filePath Identifies the source file for handout error context.
   * @return Isolated handout on a hit, std::nullopt on a miss, or a preparation error.
   * @pre The caller does not hold m_Mutex.
   *
   * The method copies the master under m_Mutex, then isolates stores after it
   * releases the table lock. Both fast and post-read recheck paths use it.
   */
  Result<std::optional<DataStructure>> tryServeFromCache(const std::string& key, uint64 fileSize, const std::filesystem::file_time_type& mtime, const std::filesystem::path& filePath);

  /**
   * @struct Entry
   * @brief Stores one immutable metadata master, validation token, and LRU tick.
   *
   * Numeric content normally consists of placeholders. Small legacy Statistics
   * arrays can contain authoritative eager values that each handout copies.
   */
  struct Entry
  {
    DataStructure master;
    uint64 fileSize = 0;
    std::filesystem::file_time_type mtime;
    uint64 lastUsedTick = 0;
  };

  /**
   * @brief Serializes complete metadata imports through non-thread-safe HDF5.
   *
   * This mutex is separate from m_Mutex, which protects only the entry table.
   * Therefore, a cache hit does not wait for an unrelated disk read. When code
   * needs both locks, it gets m_ReadMutex before m_Mutex. Hit paths get only m_Mutex.
   */
  std::mutex m_ReadMutex;
  mutable std::mutex m_Mutex;
  std::map<std::string, Entry> m_Entries;
  uint64 m_Tick = 0;
  std::atomic<uint64> m_Hits{0};
  std::atomic<uint64> m_Misses{0};
};
} // namespace nx::core::DREAM3D
