#include "Dream3dPreflightCache.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/EmptyStringStore.hpp"
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
// Match ReadDREAM3DFilter so cached and uncached open failures have one contract.
constexpr int32 k_FailedOpenFileIOError = -25;

/**
 * @brief Creates the most stable available cache key for a file path.
 * @param filePath Supplies a relative or absolute path.
 * @return Weakly canonical path text, or absolute path text if canonicalization fails.
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
 * @brief Imports one metadata-only DataStructure from disk.
 * @param filePath Identifies the DREAM3D file.
 * @return Imported structure or the standard open or import error.
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
 * @struct RefreshDataArrayStoreFunctor
 * @brief Rebinds one numeric array to a deep store copy.
 *
 * An EmptyDataStore copy duplicates shape metadata and allocates no bulk values.
 */
struct RefreshDataArrayStoreFunctor
{
  /**
   * @brief Replaces one dispatched numeric store.
   * @tparam T Specifies the array value type.
   * @param dataStructure Owns the array.
   * @param path Identifies the array.
   */
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
 * @struct RefreshNeighborListStoreFunctor
 * @brief Rebinds one NeighborList to a deep list-store copy.
 */
struct RefreshNeighborListStoreFunctor
{
  /**
   * @brief Replaces one dispatched list store.
   * @tparam T Specifies the neighbor value type.
   * @param dataStructure Owns the list.
   * @param path Identifies the list.
   */
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
      // A placeholder cannot expose values. Give it a new placeholder with the
      // same shape. Rebuild a materialized string store from its values.
      if(stringArray->isPlaceholder())
      {
        stringArray->setStore(std::make_shared<EmptyStringStore>(stringArray->getTupleShape()));
      }
      else
      {
        stringArray->setStore(std::make_shared<StringStore>(stringArray->values(), stringArray->getTupleShape()));
      }
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
      // Copy while the entry cannot be evicted. Preflight stores contain no bulk data.
      handout = iter->second.master;
    }
  }
  if(handout.has_value())
  {
    // Shared pointers keep source stores alive after the table lock releases.
    // Masters are immutable, so store isolation needs no table synchronization.
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
    // A direct serialized read preserves the standard open error. Failed stat
    // paths never enter the cache, so their handout already owns unique stores.
    m_Misses++;
    const std::lock_guard<std::mutex> readLock(m_ReadMutex);
    return ReadFromDisk(filePath);
  }

  // A recent same-size rewrite can hide inside network timestamp rounding.
  // Bypass the cache during the trust window.
  if(fs::file_time_type::clock::now() - mtime < k_MtimeTrustWindow)
  {
    // The uncached import already owns unique stores. Serialize its HDF5 traversal.
    m_Misses++;
    const std::lock_guard<std::mutex> readLock(m_ReadMutex);
    return ReadFromDisk(filePath);
  }

  const std::string key = MakeKey(filePath);

  // A hit uses only the table mutex and does not wait for an unrelated disk read.
  if(std::optional<DataStructure> hit = tryServeFromCache(key, fileSize, mtime); hit.has_value())
  {
    return {std::move(*hit)};
  }

  // Serialize the complete HDF5 import. This lock precedes every later table lock.
  const std::lock_guard<std::mutex> readLock(m_ReadMutex);

  // Another thread can populate the entry while this thread waits for the read lock.
  if(std::optional<DataStructure> hit = tryServeFromCache(key, fileSize, mtime); hit.has_value())
  {
    return {std::move(*hit)};
  }

  m_Misses++;
  Result<DataStructure> diskResult = ReadFromDisk(filePath);
  if(diskResult.invalid())
  {
    // A failed import must be retried by the next fetch.
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

    // Evict the least-recently-used entry to bound long-session bookkeeping.
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
