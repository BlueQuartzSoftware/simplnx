#include "Dream3dPreflightCache.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/EmptyStringStore.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/DataStructure/IO/Generic/InMemoryFormatResolver.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/DataStructure/StringStore.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIOInternal.hpp"
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
 * @param neutral Selects local resident metadata policy.
 * @return Imported structure, source version, and disk diagnostics.
 */
Result<NeutralMetadataHandout> ReadFromDisk(const fs::path& filePath, bool neutral)
{
  auto fileReader = nx::core::HDF5::FileIO::ReadFile(filePath);
  if(!fileReader.isValid())
  {
    return MakeErrorResult<NeutralMetadataHandout>(k_FailedOpenFileIOError, fmt::format("Failed to open the HDF5 file at the specified path: '{}'", filePath.string()));
  }
  auto version = GetFileVersion(fileReader);
  auto result = neutral ? detail::ImportMetadata(fileReader, std::make_shared<InMemoryFormatResolver>()) : ImportDataStructureFromFile(fileReader, true);
  if(result.invalid())
  {
    return ConvertInvalidResult<NeutralMetadataHandout>(std::move(result));
  }
  NeutralMetadataHandout handout{std::move(result.value()), std::move(version)};
  return ConvertResultTo<NeutralMetadataHandout>(ConvertResult(std::move(result)), std::move(handout));
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

Result<NeutralMetadataHandout> Dream3dPreflightCache::PrepareHandout(NeutralMetadataHandout handout, const fs::path& filePath)
{
  auto refreshResult = RefreshStores(handout.dataStructure);
  if(refreshResult.invalid())
  {
    for(auto& error : refreshResult.errors())
    {
      error.message = fmt::format("Cannot prepare cached metadata handout for file '{}': {}", filePath.string(), error.message);
    }
    return ConvertInvalidResult<NeutralMetadataHandout>(std::move(refreshResult));
  }
  return ConvertResultTo<NeutralMetadataHandout>(std::move(refreshResult), std::move(handout));
}

Result<std::optional<NeutralMetadataHandout>> Dream3dPreflightCache::tryServeFromCache(const CacheKey& key, uint64 fileSize, const fs::file_time_type& mtime, const fs::path& filePath)
{
  std::optional<NeutralMetadataHandout> handout;
  std::vector<Warning> diskWarnings;
  {
    const std::lock_guard<std::mutex> lock(m_Mutex);
    auto iter = m_Entries.find(key);
    if(iter != m_Entries.end() && iter->second.fileSize == fileSize && iter->second.mtime == mtime)
    {
      m_Hits++;
      iter->second.lastUsedTick = ++m_Tick;
      handout = iter->second.master;
      diskWarnings = iter->second.diskWarnings;
    }
  }
  if(!handout.has_value())
  {
    return {std::nullopt};
  }
  // Shared owners keep stores alive after the table lock releases. Isolation never changes the immutable master.
  auto prepared = PrepareHandout(std::move(*handout), filePath);
  PrependWarnings(prepared, diskWarnings);
  if(prepared.invalid())
  {
    return ConvertInvalidResult<std::optional<NeutralMetadataHandout>>(std::move(prepared));
  }
  std::optional<NeutralMetadataHandout> value{std::move(prepared.value())};
  return ConvertResultTo<std::optional<NeutralMetadataHandout>>(ConvertResult(std::move(prepared)), std::move(value));
}

Result<DataStructure> Dream3dPreflightCache::fetch(const fs::path& filePath)
{
  auto result = fetchMetadata(filePath, false);
  if(result.invalid())
  {
    return ConvertInvalidResult<DataStructure>(std::move(result));
  }
  auto structure = std::move(result.value().dataStructure);
  return ConvertResultTo<DataStructure>(ConvertResult(std::move(result)), std::move(structure));
}

Result<NeutralMetadataHandout> Dream3dPreflightCache::fetchNeutralMetadata(const fs::path& filePath)
{
  return fetchMetadata(filePath, true);
}

Result<NeutralMetadataHandout> Dream3dPreflightCache::fetchMetadata(const fs::path& filePath, bool neutral)
{
  std::error_code metadataError;
  const auto metadata = ReadFileMetadata(filePath, metadataError);
  if(!metadata.has_value() || fs::file_time_type::clock::now() - metadata->mtime < k_MtimeTrustWindow)
  {
    // Stat failures and recent files bypass both modes without changing their policy or diagnostic contracts.
    m_Misses++;
    const std::lock_guard<std::mutex> readLock(m_ReadMutex);
    auto disk = ReadFromDisk(filePath, neutral);
    if(disk.invalid())
    {
      return disk;
    }
    auto warnings = std::move(disk.warnings());
    auto result = PrepareHandout(std::move(disk.value()), filePath);
    PrependWarnings(result, warnings);
    return result;
  }

  const CacheKey key{MakeKey(filePath), neutral};
  const auto serve = [&]() { return tryServeFromCache(key, metadata->fileSize, metadata->mtime, filePath); };
  auto hit = serve();
  if(hit.invalid())
  {
    return ConvertInvalidResult<NeutralMetadataHandout>(std::move(hit));
  }
  if(hit.value().has_value())
  {
    auto value = std::move(*hit.value());
    return ConvertResultTo<NeutralMetadataHandout>(ConvertResult(std::move(hit)), std::move(value));
  }

  // The read lock precedes any later table lock. Recheck after waiting for another disk import.
  const std::lock_guard<std::mutex> readLock(m_ReadMutex);
  hit = serve();
  if(hit.invalid())
  {
    return ConvertInvalidResult<NeutralMetadataHandout>(std::move(hit));
  }
  if(hit.value().has_value())
  {
    auto value = std::move(*hit.value());
    return ConvertResultTo<NeutralMetadataHandout>(ConvertResult(std::move(hit)), std::move(value));
  }
  m_Misses++;
  auto disk = ReadFromDisk(filePath, neutral);
  if(disk.invalid())
  {
    return disk;
  }
  auto warnings = std::move(disk.warnings());
  NeutralMetadataHandout handout;
  {
    const std::lock_guard<std::mutex> lock(m_Mutex);
    Entry& entry = m_Entries[key];
    entry.master = std::move(disk.value());
    // Ordinary warm hits retain their existing behavior. Neutral hits replay only original disk warnings.
    entry.diskWarnings = neutral ? warnings : std::vector<Warning>{};
    entry.fileSize = metadata->fileSize;
    entry.mtime = metadata->mtime;
    entry.lastUsedTick = ++m_Tick;
    while(m_Entries.size() > k_Capacity)
    {
      auto victim = std::min_element(m_Entries.begin(), m_Entries.end(), [](const auto& a, const auto& b) { return a.second.lastUsedTick < b.second.lastUsedTick; });
      m_Entries.erase(victim);
    }
    handout = entry.master;
  }
  auto result = PrepareHandout(std::move(handout), filePath);
  PrependWarnings(result, warnings);
  return result;
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
  const auto key = MakeKey(filePath);
  m_Entries.erase(CacheKey{key, false});
  m_Entries.erase(CacheKey{key, true});
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
