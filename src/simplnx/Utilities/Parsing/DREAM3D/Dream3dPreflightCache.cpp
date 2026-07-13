#include "Dream3dPreflightCache.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/DataStructure/StringStore.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <fmt/core.h>

#include <algorithm>
#include <optional>

namespace fs = std::filesystem;

namespace nx::core::DREAM3D
{
namespace
{
// Matches ReadDREAM3DFilter's open-failure code and message so cache fetches
// surface the same user-visible error for a missing or unreadable file.
constexpr int32 k_FailedOpenFileIOError = -25;

/**
 * @brief Canonicalizes a path for use as a cache key so the same file reached
 * via different spellings (relative vs absolute, redundant separators) maps to
 * one entry. Falls back to fs::absolute when canonicalization fails (e.g. the
 * file does not exist yet).
 */
std::string MakeKey(const fs::path& filePath)
{
  std::error_code errorCode;
  fs::path canonical = fs::weakly_canonical(filePath, errorCode);
  if(errorCode)
  {
    canonical = fs::absolute(filePath, errorCode);
  }
  return canonical.string();
}

/**
 * @brief Reads the preflight (metadata-only) DataStructure from disk. This is
 * the cache's miss path: open the file and import metadata-only (empty) data
 * stores.
 */
Result<DataStructure> ReadFromDisk(const fs::path& filePath)
{
  auto fileReader = nx::core::HDF5::FileIO::ReadFile(filePath);
  if(!fileReader.isValid())
  {
    return MakeErrorResult<DataStructure>(k_FailedOpenFileIOError, fmt::format("Failed to open the HDF5 file at the specified path: '{}'", filePath.string()));
  }
  return ImportDataStructureFromFile(fileReader, true);
}

/**
 * @brief Rebinds a DataArray<T> to a deep copy of its current store. Deep
 * copying an EmptyDataStore allocates nothing; it only duplicates the shape
 * metadata, which is exactly what preflight handouts need.
 */
struct RefreshDataArrayStoreFunctor
{
  template <typename T>
  void operator()(DataStructure& dataStructure, const DataPath& path) const
  {
    auto& dataArray = dataStructure.getDataRefAs<DataArray<T>>(path);
    std::shared_ptr<IDataStore> freshBase(dataArray.getIDataStore()->deepCopy());
    auto freshStore = std::dynamic_pointer_cast<AbstractDataStore<T>>(freshBase);
    dataArray.setDataStore(std::move(freshStore));
  }
};

/**
 * @brief Rebinds a NeighborList<T> to a deep copy of its list store, for the
 * same isolation reason as RefreshDataArrayStoreFunctor.
 */
struct RefreshNeighborListStoreFunctor
{
  template <typename T>
  void operator()(DataStructure& dataStructure, const DataPath& path) const
  {
    auto& neighborList = dataStructure.getDataRefAs<NeighborList<T>>(path);
    neighborList.setStore(std::shared_ptr<AbstractListStore<T>>(neighborList.getStore()->deepCopy()));
  }
};
} // namespace

Dream3dPreflightCache& Dream3dPreflightCache::Instance()
{
  static Dream3dPreflightCache instance;
  return instance;
}

void Dream3dPreflightCache::RefreshStores(DataStructure& dataStructure)
{
  for(const auto& path : dataStructure.getAllDataPaths())
  {
    if(auto* dataArray = dataStructure.getDataAs<IDataArray>(path); dataArray != nullptr)
    {
      ExecuteDataFunction(RefreshDataArrayStoreFunctor{}, dataArray->getDataType(), dataStructure, path);
    }
    else if(auto* stringArray = dataStructure.getDataAs<StringArray>(path); stringArray != nullptr)
    {
      // StringArray exposes no store getter; rebuilding from its values gives
      // an equivalent, independent store. Preflight string stores hold only
      // placeholder strings, so this copies almost nothing.
      stringArray->setStore(std::make_shared<StringStore>(stringArray->values(), stringArray->getTupleShape()));
    }
    else if(auto* neighborList = dataStructure.getDataAs<INeighborList>(path); neighborList != nullptr)
    {
      ExecuteDataFunctionNoBool(RefreshNeighborListStoreFunctor{}, neighborList->getDataType(), dataStructure, path);
    }
  }
}

std::optional<DataStructure> Dream3dPreflightCache::tryServeFromCache(const std::string& key, uint64 fileSize, const fs::file_time_type& mtime)
{
  std::optional<DataStructure> handout;
  {
    const std::lock_guard<std::mutex> lock(m_Mutex);
    auto iter = m_Entries.find(key);
    if(iter != m_Entries.end() && iter->second.fileSize == fileSize && iter->second.mtime == mtime)
    {
      m_Hits++;
      iter->second.lastUsedTick = ++m_Tick;
      // Copy under the lock (the entry could be evicted after release); the
      // copy is cheap because preflight stores hold no bulk data.
      handout = iter->second.master;
    }
  }
  if(handout.has_value())
  {
    // Refresh outside the lock: the handout's shared_ptrs keep the source
    // stores alive on their own, and masters are never mutated in place, so
    // isolating the copy needs no synchronization with other fetches.
    RefreshStores(*handout);
  }
  return handout;
}

Result<DataStructure> Dream3dPreflightCache::fetch(const fs::path& filePath)
{
  std::error_code errorCode;
  const uint64 fileSize = static_cast<uint64>(fs::file_size(filePath, errorCode));
  const auto mtime = fs::last_write_time(filePath, errorCode);
  if(errorCode)
  {
    // Stat failed (file missing/unreachable): fall through to a direct read so
    // the caller receives the normal open-failure error for a missing or
    // unreadable file.
    // Freshly read from disk and never cached, so no refresh is needed: this
    // handout already owns stores no other handout can reach. The read is still
    // serialized because the HDF5 C library is not safe to call concurrently.
    m_Misses++;
    const std::lock_guard<std::mutex> readLock(m_ReadMutex);
    return ReadFromDisk(filePath);
  }

  // Never trust entries for a file modified within the mtime rounding window
  // of network filesystems: a same-size rewrite inside that window would be
  // indistinguishable from the cached state. Serve a fresh read instead.
  if(fs::file_time_type::clock::now() - mtime < k_MtimeTrustWindow)
  {
    // Freshly read from disk and never cached, so no refresh is needed: this
    // handout already owns stores no other handout can reach. The read is still
    // serialized because the HDF5 C library is not safe to call concurrently.
    m_Misses++;
    const std::lock_guard<std::mutex> readLock(m_ReadMutex);
    return ReadFromDisk(filePath);
  }

  const std::string key = MakeKey(filePath);

  // Fast path: a valid cached entry needs only m_Mutex and never blocks on a
  // disk read in flight for some other file, keeping the per-edit path light.
  if(std::optional<DataStructure> hit = tryServeFromCache(key, fileSize, mtime); hit.has_value())
  {
    return {std::move(*hit)};
  }

  // Miss: serialize the disk read with m_ReadMutex because the HDF5 C library
  // is not safe to call concurrently in this build. Per the lock ordering,
  // m_ReadMutex is acquired here BEFORE any m_Mutex acquisition below.
  const std::lock_guard<std::mutex> readLock(m_ReadMutex);

  // Re-check under m_Mutex: another thread may have populated this entry while
  // we waited for m_ReadMutex. If so, serve it and skip the redundant (and, on
  // high-latency storage, expensive) read.
  if(std::optional<DataStructure> hit = tryServeFromCache(key, fileSize, mtime); hit.has_value())
  {
    return {std::move(*hit)};
  }

  m_Misses++;
  Result<DataStructure> diskResult = ReadFromDisk(filePath);
  if(diskResult.invalid())
  {
    // Failures are never cached: the next fetch must retry the file.
    return diskResult;
  }

  DataStructure handout;
  {
    const std::lock_guard<std::mutex> lock(m_Mutex);
    Entry& entry = m_Entries[key];
    entry.master = std::move(diskResult.value());
    entry.fileSize = fileSize;
    entry.mtime = mtime;
    entry.lastUsedTick = ++m_Tick;

    // Bound the table: evict the least-recently-used entry. The cap exists to
    // bound bookkeeping in long GUI sessions that touch many files; preflight
    // masters themselves are only kilobytes.
    while(m_Entries.size() > k_Capacity)
    {
      auto victim = std::min_element(m_Entries.begin(), m_Entries.end(), [](const auto& a, const auto& b) { return a.second.lastUsedTick < b.second.lastUsedTick; });
      m_Entries.erase(victim);
    }

    handout = entry.master;
  }
  RefreshStores(handout);
  return {std::move(handout)};
}

void Dream3dPreflightCache::invalidate(const fs::path& filePath)
{
  const std::lock_guard<std::mutex> lock(m_Mutex);
  m_Entries.erase(MakeKey(filePath));
}

void Dream3dPreflightCache::clear()
{
  const std::lock_guard<std::mutex> lock(m_Mutex);
  m_Entries.clear();
}

uint64 Dream3dPreflightCache::hitCount() const
{
  return m_Hits.load();
}

uint64 Dream3dPreflightCache::missCount() const
{
  return m_Misses.load();
}

void Dream3dPreflightCache::resetStats()
{
  m_Hits.store(0);
  m_Misses.store(0);
}
} // namespace nx::core::DREAM3D
