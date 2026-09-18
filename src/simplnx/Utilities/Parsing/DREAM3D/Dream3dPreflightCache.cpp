#include "Dream3dPreflightCache.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/EmptyStringStore.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/DataStructure/StringStore.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"
#include "simplnx/Utilities/StoreCopyUtilities.hpp"

#include <fmt/core.h>

#include <algorithm>
#include <iterator>
#include <optional>
#include <system_error>
#include <unordered_set>

namespace fs = std::filesystem;

namespace nx::core::DREAM3D
{
namespace
{
// Match ReadDREAM3DFilter so cached and uncached open failures have one contract.
constexpr int32 k_FailedOpenFileIOError = -25;
constexpr int32 k_CacheStoreCopyError = -6210;

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
thread_local bool g_ForceFileMetadataFailure = false;
#endif

struct FileMetadata
{
  uint64 fileSize = 0;
  fs::file_time_type mtime;
};

std::optional<FileMetadata> ReadFileMetadata(const fs::path& filePath, std::error_code& errorCode)
{
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
  if(g_ForceFileMetadataFailure)
  {
    errorCode = std::make_error_code(std::errc::io_error);
    return std::nullopt;
  }
#endif

  const auto fileSize = fs::file_size(filePath, errorCode);
  if(errorCode)
  {
    return std::nullopt;
  }
  const auto mtime = fs::last_write_time(filePath, errorCode);
  if(errorCode)
  {
    return std::nullopt;
  }
  return FileMetadata{static_cast<uint64>(fileSize), mtime};
}

template <class T>
void PrependWarnings(Result<T>& result, std::vector<Warning>& warnings)
{
  result.warnings().insert(result.warnings().begin(), std::make_move_iterator(warnings.begin()), std::make_move_iterator(warnings.end()));
}

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
 * @brief Replans one numeric placeholder or policy-copies populated values.
 */
struct RefreshDataArrayStoreFunctor
{
  /**
   * @brief Replaces one dispatched numeric store.
   * @tparam T Specifies the array value type.
   * @param dataStructure Owns the array.
   * @param path Identifies the array.
   * @return Planning, copy, and replacement errors and warnings.
   */
  template <typename T>
  Result<> operator()(DataStructure& dataStructure, const DataPath& path) const
  {
    auto* dataArray = dataStructure.getDataAs<DataArray<T>>(path);
    if(dataArray == nullptr || dataArray->getIDataStore() == nullptr)
    {
      return MakeErrorResult(k_CacheStoreCopyError, fmt::format("Cannot prepare cached numeric array '{}': the array or its source store is unavailable.", path.toString()));
    }

    if(dataArray->getStoreType() == IDataStore::StoreType::Empty)
    {
      auto plannedResult = DataStoreUtilities::CreatePlannedDataStore<T>(dataStructure, path, dataArray->getTupleShape(), dataArray->getComponentShape(), "");
      if(plannedResult.invalid())
      {
        return ConvertResult(std::move(plannedResult));
      }
      auto warnings = std::move(plannedResult.warnings());
      auto replaceResult = dataArray->setDataStore(std::move(plannedResult.value()));
      PrependWarnings(replaceResult, warnings);
      if(replaceResult.invalid())
      {
        for(auto& error : replaceResult.errors())
        {
          error.message = fmt::format("Cannot install the planned store for cached numeric array '{}': {}", path.toString(), error.message);
        }
      }
      return replaceResult;
    }

    Result<std::string> formatResult;
    try
    {
      const auto bytes = CalculateStoreCopyBytes(dataArray->getTupleShape(), dataArray->getComponentShape(), sizeof(T));
      formatResult = ResolveNumericStorageFormat(dataStructure, path, GetDataType<T>(), bytes, "");
    } catch(const std::bad_alloc&)
    {
      throw;
    } catch(const std::exception& error)
    {
      return MakeErrorResult(k_CacheStoreCopyError, fmt::format("Cannot size populated cached numeric array '{}': {}", path.toString(), error.what()));
    }
    if(formatResult.invalid())
    {
      return ConvertResult(std::move(formatResult));
    }

    const std::string selectedFormat = formatResult.value();
    auto warnings = std::move(formatResult.warnings());
    std::shared_ptr<AbstractDataStore<T>> freshStore;
    try
    {
      std::shared_ptr<IDataStore> freshBase(dataArray->getIDataStore()->deepCopy(selectedFormat));
      freshStore = std::dynamic_pointer_cast<AbstractDataStore<T>>(freshBase);
      if(freshStore == nullptr)
      {
        throw std::runtime_error("the selected factory returned an incompatible numeric store");
      }
    } catch(const std::bad_alloc&)
    {
      throw;
    } catch(const std::exception& error)
    {
      auto result = MakeErrorResult(k_CacheStoreCopyError, fmt::format("Cannot copy populated cached numeric array '{}' into selected format '{}': {}", path.toString(),
                                                                       selectedFormat.empty() ? "in-memory" : selectedFormat, error.what()));
      result.warnings() = std::move(warnings);
      return result;
    }

    auto replaceResult = dataArray->setDataStore(std::move(freshStore));
    PrependWarnings(replaceResult, warnings);
    if(replaceResult.invalid())
    {
      for(auto& error : replaceResult.errors())
      {
        error.message = fmt::format("Cannot install the copied store for cached numeric array '{}': {}", path.toString(), error.message);
      }
    }
    return replaceResult;
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
    // Tuple metadata supplies a lower-bound policy estimate without list reads.
    const auto bytes = CalculateStoreCopyBytes(neighborList.getTupleShape(), ShapeType{1}, sizeof(T));
    const auto format = ArrayCreationUtilities::ResolveStorageFormat(dataStructure, path, GetDataType<T>(), bytes, "");
    neighborList.setStore(std::shared_ptr<AbstractListStore<T>>(neighborList.getStore()->deepCopy(format)));
  }
};
} // namespace

Dream3dPreflightCache& Dream3dPreflightCache::Instance()
{
  static Dream3dPreflightCache instance;
  return instance;
}

Result<> Dream3dPreflightCache::RefreshStores(DataStructure& dataStructure)
{
  Result<> result;
  std::unordered_set<DataObject::IdType> visitedIds;
  for(const auto& path : dataStructure.getAllDataPaths())
  {
    auto* object = dataStructure.getData(path);
    if(object == nullptr || !visitedIds.insert(object->getId()).second)
    {
      continue;
    }

    if(auto* dataArray = dynamic_cast<IDataArray*>(object); dataArray != nullptr)
    {
      auto refreshResult = ExecuteDataFunction(RefreshDataArrayStoreFunctor{}, dataArray->getDataType(), dataStructure, path);
      if(refreshResult.invalid())
      {
        refreshResult.warnings().insert(refreshResult.warnings().begin(), std::make_move_iterator(result.warnings().begin()), std::make_move_iterator(result.warnings().end()));
        return refreshResult;
      }
      for(auto&& warning : refreshResult.warnings())
      {
        result.warnings().push_back(std::move(warning));
      }
    }
    else if(auto* stringArray = dynamic_cast<StringArray*>(object); stringArray != nullptr)
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
    else if(auto* neighborList = dynamic_cast<INeighborList*>(object); neighborList != nullptr)
    {
      ExecuteDataFunctionNoBool(RefreshNeighborListStoreFunctor{}, neighborList->getDataType(), dataStructure, path);
    }
  }
  return result;
}

Result<DataStructure> Dream3dPreflightCache::PrepareHandout(DataStructure handout, const fs::path& filePath)
{
  auto refreshResult = RefreshStores(handout);
  if(refreshResult.invalid())
  {
    for(auto& error : refreshResult.errors())
    {
      error.message = fmt::format("Cannot prepare cached metadata handout for file '{}': {}", filePath.string(), error.message);
    }
    return ConvertInvalidResult<DataStructure>(std::move(refreshResult));
  }
  return ConvertResultTo<DataStructure>(std::move(refreshResult), std::move(handout));
}

Result<std::optional<DataStructure>> Dream3dPreflightCache::tryServeFromCache(const std::string& key, uint64 fileSize, const fs::file_time_type& mtime, const fs::path& filePath)
{
  std::optional<DataStructure> handout;
  {
    const std::lock_guard<std::mutex> lock(m_Mutex);
    auto iter = m_Entries.find(key);
    if(iter != m_Entries.end() && iter->second.fileSize == fileSize && iter->second.mtime == mtime)
    {
      m_Hits++;
      iter->second.lastUsedTick = ++m_Tick;
      // Copy while the entry cannot be evicted. Ordinary bulk datasets remain placeholders;
      // small eager Statistics stores are shared only until handout preparation below.
      handout = iter->second.master;
    }
  }
  if(handout.has_value())
  {
    // Shared pointers keep source stores alive after the table lock releases.
    // Masters are immutable, so store isolation needs no table synchronization.
    auto preparedResult = PrepareHandout(std::move(*handout), filePath);
    if(preparedResult.invalid())
    {
      return ConvertInvalidResult<std::optional<DataStructure>>(std::move(preparedResult));
    }
    Result<std::optional<DataStructure>> result{std::optional<DataStructure>{std::move(preparedResult.value())}};
    result.warnings() = std::move(preparedResult.warnings());
    return result;
  }
  return {std::nullopt};
}

Result<DataStructure> Dream3dPreflightCache::fetch(const fs::path& filePath)
{
  std::error_code metadataError;
  const auto metadata = ReadFileMetadata(filePath, metadataError);
  if(!metadata.has_value())
  {
    // A direct serialized read preserves the standard open error. Failed stat
    // paths never enter the cache, but their stores still receive current handout policy.
    m_Misses++;
    const std::lock_guard<std::mutex> readLock(m_ReadMutex);
    auto diskResult = ReadFromDisk(filePath);
    if(diskResult.invalid())
    {
      return diskResult;
    }
    auto diskWarnings = std::move(diskResult.warnings());
    auto preparedResult = PrepareHandout(std::move(diskResult.value()), filePath);
    PrependWarnings(preparedResult, diskWarnings);
    return preparedResult;
  }
  const uint64 fileSize = metadata->fileSize;
  const auto mtime = metadata->mtime;

  // A recent same-size rewrite can hide inside network timestamp rounding.
  // Bypass the cache during the trust window.
  if(fs::file_time_type::clock::now() - mtime < k_MtimeTrustWindow)
  {
    // Serialize its HDF5 traversal, then apply current handout policy.
    m_Misses++;
    const std::lock_guard<std::mutex> readLock(m_ReadMutex);
    auto diskResult = ReadFromDisk(filePath);
    if(diskResult.invalid())
    {
      return diskResult;
    }
    auto diskWarnings = std::move(diskResult.warnings());
    auto preparedResult = PrepareHandout(std::move(diskResult.value()), filePath);
    PrependWarnings(preparedResult, diskWarnings);
    return preparedResult;
  }

  const std::string key = MakeKey(filePath);

  // A hit uses only the table mutex and does not wait for an unrelated disk read.
  auto hit = tryServeFromCache(key, fileSize, mtime, filePath);
  if(hit.invalid())
  {
    return ConvertInvalidResult<DataStructure>(std::move(hit));
  }
  if(hit.value().has_value())
  {
    Result<DataStructure> result{std::move(*hit.value())};
    result.warnings() = std::move(hit.warnings());
    return result;
  }

  // Serialize the complete HDF5 import. This lock precedes every later table lock.
  const std::lock_guard<std::mutex> readLock(m_ReadMutex);

  // Another thread can populate the entry while this thread waits for the read lock.
  hit = tryServeFromCache(key, fileSize, mtime, filePath);
  if(hit.invalid())
  {
    return ConvertInvalidResult<DataStructure>(std::move(hit));
  }
  if(hit.value().has_value())
  {
    Result<DataStructure> result{std::move(*hit.value())};
    result.warnings() = std::move(hit.warnings());
    return result;
  }

  m_Misses++;
  Result<DataStructure> diskResult = ReadFromDisk(filePath);
  if(diskResult.invalid())
  {
    // A failed import must be retried by the next fetch.
    return diskResult;
  }
  auto diskWarnings = std::move(diskResult.warnings());

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
  auto preparedResult = PrepareHandout(std::move(handout), filePath);
  PrependWarnings(preparedResult, diskWarnings);
  return preparedResult;
}

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
void Dream3dPreflightCache::SetForceFileMetadataFailure(bool forceFailure)
{
  g_ForceFileMetadataFailure = forceFailure;
}
#endif

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
