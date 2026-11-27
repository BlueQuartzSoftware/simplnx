#include "Dream3dPreflightCache.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/IO/HDF5/RectGridGeomIO.hpp"
#include <memory>
#include <utility>

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
#include "simplnx/Utilities/Parsing/DREAM3D/ImageMetadataImport.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"
#include "simplnx/Utilities/StoreCopyUtilities.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <system_error>
#include <unordered_set>

namespace fs = std::filesystem;

namespace nx::core::dream3d
{
namespace
{
// Match ReadDREAM3DFilter so cached and uncached open failures have one contract.
constexpr int32 k_FailedOpenFileIOError = -25;
constexpr int32 k_CacheStoreCopyError = -6210;

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
thread_local bool GForceFileMetadataFailure = false;
#endif

struct FileMetadata
{
  uint64 file_size = 0;
  fs::file_time_type mtime;
};

std::optional<FileMetadata> ReadFileMetadata(const fs::path& filePath, std::error_code& errorCode)
{
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
  if(GForceFileMetadataFailure)
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
  return FileMetadata{.file_size = static_cast<uint64>(fileSize), .mtime = mtime};
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
  auto fileReader = nx::core::hdf5::FileIO::ReadFile(filePath);
  if(!fileReader.isValid())
  {
    return MakeErrorResult<NeutralMetadataHandout>(k_FailedOpenFileIOError, fmt::format("Failed to open the HDF5 file at the specified path: '{}'", filePath.string()));
  }
  auto version = GetFileVersion(fileReader);
  DeferredImageMetadataType deferredImageMetadata;
  auto result = neutral ? detail::ImportMetadata(fileReader, std::make_shared<InMemoryFormatResolver>(), deferredImageMetadata) : ImportDataStructureFromFile(fileReader, true);
  if(result.invalid())
  {
    return ConvertInvalidResult<NeutralMetadataHandout>(std::move(result));
  }
  NeutralMetadataHandout handout{.data_structure = std::move(result.value()), .file_version = std::move(version), .deferred_image_metadata = std::move(deferredImageMetadata)};
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
    auto* dataArrayPtr = dataStructure.getDataAs<DataArray<T>>(path);
    if(dataArrayPtr == nullptr || dataArrayPtr->getIDataStore() == nullptr)
    {
      return MakeErrorResult(k_CacheStoreCopyError, fmt::format("Cannot prepare cached numeric array '{}': the array or its source store is unavailable.", path.toString()));
    }

    if(dataArrayPtr->getStoreType() == IDataStore::StoreType::Empty)
    {
      auto plannedResult = data_store_utilities::CreatePlannedDataStore<T>(dataStructure, path, dataArrayPtr->getTupleShape(), dataArrayPtr->getComponentShape(), "");
      if(plannedResult.invalid())
      {
        return ConvertResult(std::move(plannedResult));
      }
      auto warnings = std::move(plannedResult.warnings());
      auto replaceResult = dataArrayPtr->setDataStore(std::move(plannedResult.value()));
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
      const auto bytes = CalculateStoreCopyBytes(dataArrayPtr->getTupleShape(), dataArrayPtr->getComponentShape(), sizeof(T));
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
      std::shared_ptr<IDataStore> freshBase(dataArrayPtr->getIDataStore()->deepCopy(selectedFormat));
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

    auto replaceResult = dataArrayPtr->setDataStore(std::move(freshStore));
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
    const auto format = array_creation_utilities::ResolveStorageFormat(dataStructure, path, GetDataType<T>(), bytes, "");
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
    auto* objectPtr = dataStructure.getData(path);
    if(objectPtr == nullptr || !visitedIds.insert(objectPtr->getId()).second)
    {
      continue;
    }

    if(auto* dataArrayPtr = dynamic_cast<IDataArray*>(objectPtr); dataArrayPtr != nullptr)
    {
      auto refreshResult = ExecuteDataFunction(RefreshDataArrayStoreFunctor{}, dataArrayPtr->getDataType(), dataStructure, path);
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
    else if(auto* stringArrayPtr = dynamic_cast<StringArray*>(objectPtr); stringArrayPtr != nullptr)
    {
      // A placeholder cannot expose values. Give it a new placeholder with the
      // same shape. Rebuild a materialized string store from its values.
      if(stringArrayPtr->isPlaceholder())
      {
        stringArrayPtr->setStore(std::make_shared<EmptyStringStore>(stringArrayPtr->getTupleShape()));
      }
      else
      {
        try
        {
          stringArrayPtr->setStore(std::make_shared<StringStore>(stringArrayPtr->values(), stringArrayPtr->getTupleShape()));
        } catch(const std::invalid_argument& error)
        {
          auto refreshResult = MakeErrorResult(-94131, fmt::format("Cannot refresh strings at '{}': {}. Reload the source with consistent tuple dimensions.", path.toString(), error.what()));
          refreshResult.warnings().insert(refreshResult.warnings().begin(), std::make_move_iterator(result.warnings().begin()), std::make_move_iterator(result.warnings().end()));
          return refreshResult;
        }
      }
    }
    else if(auto* neighborListPtr = dynamic_cast<INeighborList*>(objectPtr); neighborListPtr != nullptr)
    {
      ExecuteDataFunctionNoBool(RefreshNeighborListStoreFunctor{}, neighborListPtr->getDataType(), dataStructure, path);
    }
  }
  return result;
}

Result<NeutralMetadataHandout> Dream3dPreflightCache::PrepareHandout(NeutralMetadataHandout handout, const fs::path& filePath)
{
  auto refreshResult = RefreshStores(handout.data_structure);
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

Result<std::optional<NeutralMetadataHandout>> Dream3dPreflightCache::tryServeFromCache(const CacheKeyType& key, uint64 fileSize, const fs::file_time_type& mtime, const fs::path& filePath)
{
  std::optional<NeutralMetadataHandout> handout;
  std::vector<Warning> diskWarnings;
  {
    const std::lock_guard<std::mutex> lock(m_Mutex);
    auto iter = m_Entries.find(key);
    if(iter != m_Entries.end() && iter->second.file_size == fileSize && iter->second.mtime == mtime)
    {
      m_Hits++;
      iter->second.last_used_tick = ++m_Tick;
      handout = iter->second.master;
      diskWarnings = iter->second.disk_warnings;
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
  auto structure = std::move(result.value().data_structure);
  return ConvertResultTo<DataStructure>(ConvertResult(std::move(result)), std::move(structure));
}

Result<> NeutralMetadataHandout::applyImageMetadata(const fs::path& filePath, const std::vector<DataPath>& selectedPaths, bool useDefaultGeometryValues)
{
  Result<> result;
  std::unordered_set<DataObject::IdType> checkedIds;
  for(const auto& path : selectedPaths)
  {
    auto* geometryPtr = data_structure.getDataAs<ImageGeom>(path);
    if(geometryPtr == nullptr || !checkedIds.insert(geometryPtr->getId()).second)
    {
      continue;
    }
    const auto metadata = deferred_image_metadata.find(geometryPtr->getId());
    if(metadata == deferred_image_metadata.end())
    {
      continue;
    }
    result = MergeResults(std::move(result), detail::ApplyImportedImageMetadata(*geometryPtr, filePath, metadata->second, useDefaultGeometryValues));
    if(result.invalid())
    {
      return result;
    }
  }
  return result;
}

Result<NeutralMetadataHandout> Dream3dPreflightCache::fetchNeutralMetadata(const fs::path& filePath)
{
  auto result = fetchSelectableMetadata(filePath);
  if(result.invalid())
  {
    return result;
  }
  auto& handout = result.value();
  auto validation = hdf5::RectGridGeomIO::ValidateImportedBounds(handout.data_structure, filePath);
  if(validation.valid())
  {
    validation = handout.applyImageMetadata(filePath, handout.data_structure.getAllDataPaths(), false);
  }
  PrependWarnings(validation, result.warnings());
  if(validation.invalid())
  {
    return ConvertInvalidResult<NeutralMetadataHandout>(std::move(validation));
  }
  result.warnings() = std::move(validation.warnings());
  handout.deferred_image_metadata.clear();
  return result;
}

Result<NeutralMetadataHandout> Dream3dPreflightCache::fetchSelectableMetadata(const fs::path& filePath)
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

  const CacheKeyType key{MakeKey(filePath), neutral};
  const auto serve = [&]() { return tryServeFromCache(key, metadata->file_size, metadata->mtime, filePath); };
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
    entry.disk_warnings = neutral ? warnings : std::vector<Warning>{};
    entry.file_size = metadata->file_size;
    entry.mtime = metadata->mtime;
    entry.last_used_tick = ++m_Tick;
    while(m_Entries.size() > k_Capacity)
    {
      auto victim = std::min_element(m_Entries.begin(), m_Entries.end(), [](const auto& a, const auto& b) { return a.second.last_used_tick < b.second.last_used_tick; });
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
  GForceFileMetadataFailure = forceFailure;
}
#endif

void Dream3dPreflightCache::invalidate(const fs::path& filePath)
{
  const std::lock_guard<std::mutex> lock(m_Mutex);
  const auto key = MakeKey(filePath);
  m_Entries.erase(CacheKeyType{key, false});
  m_Entries.erase(CacheKeyType{key, true});
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
} // namespace nx::core::dream3d
