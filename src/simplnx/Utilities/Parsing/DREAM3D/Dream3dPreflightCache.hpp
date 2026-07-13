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
 * @brief Process-wide cache of preflight (metadata-only) DataStructures imported
 * from .dream3d files, keyed by canonical file path and validated by file size
 * and modification time on every fetch.
 *
 * Why this exists: a pipeline re-preflights whenever any filter parameter
 * changes, and every preflight of a ReadDREAM3DFilter re-imports the HDF5
 * metadata tree of its input file. That traversal is hundreds of small,
 * latency-bound reads (object headers, B-tree nodes, attributes). On local
 * disks it is milliseconds; on high-latency storage such as NAS/SMB mounts it
 * can take many seconds — repeated on every edit, for metadata that has not
 * changed. Caching the imported preflight structure reduces every fetch after
 * the first to a single filesystem stat.
 *
 * fetch() never hands out the cached master DataStructure itself; it returns
 * a copy whose arrays own freshly created store instances, so mutations made
 * by one preflight pass can never bleed into another pass or into the master.
 * Preflight stores are metadata-only, so allocating fresh copies costs no
 * bulk data.
 *
 * Staleness guarantee: every fetch stats the file and compares (size, mtime)
 * against the values captured when the entry was populated; any mismatch
 * evicts the entry and re-reads from disk. Files whose mtime is younger than
 * k_MtimeTrustWindow are never served from the cache because network
 * filesystems round mtimes coarsely enough that a very recent rewrite could
 * otherwise go undetected.
 */
class SIMPLNX_EXPORT Dream3dPreflightCache
{
public:
  /**
   * @brief The maximum number of entries the cache holds. Once a fetch would
   * grow the table past this size, the least-recently-used entry (by fetch
   * recency, tracked per-entry and updated on every hit or insert) is evicted
   * first. Preflight structures store only shapes and names (kilobytes), so
   * this bounds bookkeeping, not memory pressure.
   */
  static constexpr usize k_Capacity = 8;

  /**
   * @brief Files modified more recently than this are never served from the
   * cache. Guards against coarse mtime granularity on network filesystems
   * (SMB/NFS commonly round to 1-2 s), where a same-size rewrite inside the
   * rounding window would otherwise be indistinguishable from the cached state.
   */
  static constexpr std::chrono::seconds k_MtimeTrustWindow{2};

  /**
   * @brief Returns the process-wide cache instance. A singleton because the
   * cache must be shared across every filter instance and pipeline that reads
   * the same file, and because preflight runs on worker threads that have no
   * shared context other than process globals.
   */
  static Dream3dPreflightCache& Instance();

  /**
   * @brief Returns a preflight DataStructure for the given file.
   *
   * Stats the file first: on a (size, mtime) match with a cached entry this
   * is a pure in-memory operation; otherwise the file is opened and its
   * metadata imported exactly as ImportDataStructureFromFile(reader, true)
   * would, and the result cached. Errors (missing/unreadable file, malformed
   * content) are returned unchanged from the underlying reader so callers keep
   * their existing error contracts; failures are never cached.
   * @param filePath Path to the .dream3d file.
   * @return Result<DataStructure> An isolated handout: every array's store is
   * a fresh instance, never shared with the cached master or any other
   * handout, or import errors.
   */
  Result<DataStructure> fetch(const std::filesystem::path& filePath);

  /**
   * @brief Removes the entry for the given file, if present. Exists so tests
   * and callers that know a file changed can force the next fetch to re-read
   * without waiting for stat-based detection.
   * @param filePath Path whose entry should be dropped.
   */
  void invalidate(const std::filesystem::path& filePath);

  /**
   * @brief Removes all entries. Primarily for test isolation.
   */
  void clear();

  /**
   * @brief Number of fetches served from the cache since the last resetStats().
   * Tests assert on these counters instead of wall-clock time so the I/O
   * behavior is verified deterministically on any machine.
   */
  uint64 hitCount() const;

  /**
   * @brief Number of fetches that read the file since the last resetStats().
   */
  uint64 missCount() const;

  /**
   * @brief Resets hit/miss counters to zero. For test isolation.
   */
  void resetStats();

private:
  Dream3dPreflightCache() = default;

  /**
   * @brief Replaces every array store in the given structure with a freshly
   * allocated copy.
   *
   * Why: DataStructure copy-construction shallow-copies each DataObject, so a
   * copied array still points at the SAME store instance as its source. A
   * handout built by plain copy would therefore share mutable state with the
   * cached master and with every other handout — mutating one preflight pass
   * could corrupt another. Replacing the stores severs that link. Preflight
   * stores are metadata-only (empty stores holding shapes), so the copies
   * allocate no bulk data.
   * @param dataStructure The handout to isolate, modified in place.
   */
  static void RefreshStores(DataStructure& dataStructure);

  /**
   * @brief Looks up a cached entry for the given key and, if it is still valid
   * for the supplied (size, mtime) token, records a hit, refreshes its recency,
   * and returns an isolated handout copy; returns nullopt on a miss.
   *
   * Acquires m_Mutex internally (the caller must not already hold it) and, on a
   * hit, releases it before isolating the copy via RefreshStores so that work
   * never runs under the table lock. Shared by the fast path and the miss
   * path's post-read-lock re-check so both consult the cache identically.
   * @param key Canonical cache key for the file.
   * @param fileSize Current on-disk size, compared against the cached token.
   * @param mtime Current on-disk modification time, compared against the token.
   * @return An isolated handout on a hit, or nullopt on a miss.
   */
  std::optional<DataStructure> tryServeFromCache(const std::string& key, uint64 fileSize, const std::filesystem::file_time_type& mtime);

  /**
   * @brief One cached file: the immutable master structure plus the (size,
   * mtime) token captured when it was read, used to detect on-disk changes.
   */
  struct Entry
  {
    DataStructure master;
    uint64 fileSize = 0;
    std::filesystem::file_time_type mtime;
    uint64 lastUsedTick = 0;
  };

  /**
   * @brief Serializes the cache's own metadata reads from disk so no two HDF5
   * imports run concurrently. The bundled HDF5 C library is built without its
   * thread-safe option, so its API cannot be called from multiple threads at
   * once; every cache miss reads the file through this mutex to honor that
   * constraint. It is deliberately distinct from m_Mutex, which guards only the
   * in-memory entry table: a cache hit takes m_Mutex alone and never blocks on
   * an in-flight read, keeping the common per-edit fetch path lock-light.
   *
   * Lock ordering: whenever both are held, m_ReadMutex is acquired before
   * m_Mutex, never the reverse. Hits take only m_Mutex; the miss path takes
   * m_ReadMutex first and then briefly m_Mutex to consult and update the table.
   */
  std::mutex m_ReadMutex;
  mutable std::mutex m_Mutex;
  std::map<std::string, Entry> m_Entries;
  uint64 m_Tick = 0;
  std::atomic<uint64> m_Hits{0};
  std::atomic<uint64> m_Misses{0};
};
} // namespace nx::core::DREAM3D
