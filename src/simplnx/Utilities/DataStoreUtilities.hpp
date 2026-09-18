#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/EmptyListStore.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/Utilities/StoreCopyUtilities.hpp"

#include <fmt/format.h>

#include <iterator>
#include <optional>

/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */
namespace nx::core
{
class DataStructure;

/**
 * @namespace nx::core::ArrayCreationUtilities
 * @brief Contains storage-aware array creation utilities.
 */
namespace ArrayCreationUtilities
{
// This declaration avoids an include cycle with ArrayCreationUtilities.hpp, which owns the API documentation.
SIMPLNX_EXPORT std::string ResolveStorageFormat(const DataStructure& dataStructure, const DataPath& path, DataType numericType, uint64 dataSizeBytes, const std::string& requestedFormat);
} // namespace ArrayCreationUtilities
} // namespace nx::core

/**
 * @namespace nx::core::DataStoreUtilities
 * @brief Contains storage-neutral DataStore and ListStore utilities.
 */
namespace nx::core::DataStoreUtilities
{
/**
 * @brief Returns a non-owning reference to the application's DataIOCollection.
 *
 * The Application owns the collection. The reference remains valid while that
 * process Application exists and does not extend its lifetime.
 *
 * @return Reference to the Application's DataIOCollection.
 */
SIMPLNX_EXPORT DataIOCollection& GetIOCollection();

/**
 * @brief Creates a bounded fixed-record scratch store through the first registered storage provider.
 * @param config Specifies the record layout, capacity, staging size, and cancellation flag.
 * @return Created store or a provider error.
 */
inline Result<std::unique_ptr<ITemporaryRecordStore>> CreateTemporaryRecordStore(const TemporaryRecordStoreConfig& config)
{
  return GetIOCollection().createTemporaryRecordStore(config);
}

/**
 * @brief Creates value storage for a DataArray.
 *
 * The DataStructure resolver selects the format. A registered I/O manager
 * creates the selected store once.
 *
 * Construct an in-memory DataStore directly for scratch that has no owning
 * DataStructure and DataPath. Those objects do not provide resolution context.
 *
 * @tparam T Specifies the element type.
 * @param dataStructure Contains the future array and supplies resolution context.
 * @param arrayPath Identifies the future array and its geometry ancestors.
 * @param tupleShape Specifies tuple dimensions.
 * @param componentShape Specifies component dimensions.
 * @return Created store, or null if the selected factory returns an incompatible store.
 * @throws std::runtime_error If selection or factory invocation fails.
 * @throws std::bad_alloc If allocation fails.
 */
template <class T>
std::shared_ptr<AbstractDataStore<T>> CreateDataStore(const DataStructure& dataStructure, const DataPath& arrayPath, const ShapeType& tupleShape, const ShapeType& componentShape)
{
  const uint64 requiredBytes = CalculateStoreCopyBytes(tupleShape, componentShape, sizeof(T));
  auto formatResult = ResolveNumericStorageFormat(dataStructure, arrayPath, GetDataType<T>(), requiredBytes, "");
  if(formatResult.invalid())
  {
    throw std::runtime_error(formatResult.errors().front().message);
  }

  const std::string& selectedFormat = formatResult.value();
  std::shared_ptr<IDataStore> baseStore = GetIOCollection().createDataStore(selectedFormat, GetDataType<T>(), tupleShape, componentShape);
  auto store = std::dynamic_pointer_cast<AbstractDataStore<T>>(baseStore);
  const usize tupleCount = std::accumulate(tupleShape.begin(), tupleShape.end(), static_cast<usize>(1), std::multiplies<>());
  const usize componentCount = std::accumulate(componentShape.begin(), componentShape.end(), static_cast<usize>(1), std::multiplies<>());
  if(store == nullptr || store->getDataType() != GetDataType<T>() || store->getTupleShape() != tupleShape || store->getComponentShape() != componentShape || store->getNumberOfTuples() != tupleCount ||
     store->getNumberOfComponents() != componentCount || store->getStoreType() == IDataStore::StoreType::Empty)
  {
    return nullptr;
  }

  const bool selectedMemory = selectedFormat.empty();
  const std::string actualFormat = store->getDataFormat();
  const bool actualMemoryFormat = actualFormat.empty() || actualFormat == Preferences::k_InMemoryFormat.str();
  if(selectedMemory ? (store->getStoreType() != IDataStore::StoreType::InMemory || !actualMemoryFormat) : (store->getStoreType() != IDataStore::StoreType::OutOfCore || actualFormat != selectedFormat))
  {
    return nullptr;
  }

  return store;
}

/**
 * @brief Creates a metadata store after destination planning succeeds.
 * @tparam T Specifies the element type.
 * @param destination Supplies policy and registered formats.
 * @param path Identifies the planned array.
 * @param tupleShape Specifies tuple dimensions.
 * @param componentShape Specifies component dimensions.
 * @param requestedFormat Explicit format, or empty to use destination policy.
 * @return Planned metadata store or a contextual error.
 * @throws std::bad_alloc If metadata allocation fails.
 */
template <class T>
Result<std::shared_ptr<AbstractDataStore<T>>> CreatePlannedDataStore(const DataStructure& destination, const DataPath& path, const ShapeType& tupleShape, const ShapeType& componentShape,
                                                                     const std::string& requestedFormat)
{
  static constexpr int32 k_PlannedDataStoreCreationError = -10614;
  try
  {
    const uint64 logicalBytes = CalculateStoreCopyBytes(tupleShape, componentShape, sizeof(T));
    auto formatResult = ResolveNumericStorageFormat(destination, path, GetDataType<T>(), logicalBytes, requestedFormat);
    if(formatResult.invalid())
    {
      return ConvertInvalidResult<std::shared_ptr<AbstractDataStore<T>>>(std::move(formatResult));
    }

    auto warnings = std::move(formatResult.warnings());
    auto placeholderResult = EmptyDataStore<T>::Create(tupleShape, componentShape, formatResult.value());
    if(placeholderResult.invalid())
    {
      auto result = ConvertInvalidResult<std::shared_ptr<AbstractDataStore<T>>>(std::move(placeholderResult));
      result.warnings().insert(result.warnings().begin(), std::make_move_iterator(warnings.begin()), std::make_move_iterator(warnings.end()));
      for(auto& error : result.errors())
      {
        error.message = fmt::format("Cannot create planned numeric store for array '{}': {}", path.toString(), error.message);
      }
      return result;
    }

    std::shared_ptr<AbstractDataStore<T>> store(std::move(placeholderResult.value()));
    Result<std::shared_ptr<AbstractDataStore<T>>> result{std::move(store)};
    result.warnings() = std::move(warnings);
    for(auto&& warning : placeholderResult.warnings())
    {
      result.warnings().push_back(std::move(warning));
    }
    return result;
  } catch(const std::bad_alloc&)
  {
    throw;
  } catch(const std::exception& error)
  {
    return MakeErrorResult<std::shared_ptr<AbstractDataStore<T>>>(k_PlannedDataStoreCreationError,
                                                                  fmt::format("Cannot create planned numeric store for array '{}': {}", path.toString(), error.what()));
  }
}

/**
 * @brief Create an AbstractDataStore whose backing format MIRRORS an existing store's actual format -- @p dataFormat
 * is the empty string "" for the in-memory default, or a concrete out-of-core format string. Unlike @ref
 * CreateDataStore this does NOT consult the DataStorageMode/size resolver: it uses @p dataFormat verbatim, so the
 * created store lands in exactly the same storage mode as whatever array it was taken from.
 *
 * Use this for a SCRATCH store that must inherit the storage mode of the input array it is derived from -- pass
 * @c inputArray.getDataFormat(). This keeps a streamed (out-of-core) algorithm's scratch out-of-core (so its memory
 * bound is preserved) and an in-core run's scratch in-core, whereas @ref CreateDataStore would re-resolve by
 * preference + size and could route a scratch to a different mode than the input it shadows (an empty requested
 * format is ambiguous there -- it means "let the resolver decide", not "force in-core"). Only meaningful for arrays
 * under a geometry that supports the format (Image/RectGrid); for scratch derived from such an input that always
 * holds.
 *
 * In Preflight mode returns an EmptyDataStore (shape metadata only, no allocation); in Execute mode allocates the
 * real backing store through the registered IO managers. @p dataFormat must be a format the running build has a
 * manager for -- guaranteed when it comes from an existing array's getDataFormat().
 *
 * @tparam T Primitive type (int8, float32, uint64, etc.)
 * @param dataFormat The backing format to use verbatim ("" for in-memory, else a registered out-of-core format)
 * @param tupleShape The tuple dimensions
 * @param componentShape The component dimensions
 * @param mode PREFLIGHT returns an EmptyDataStore; EXECUTE allocates real storage
 * @param chunkShapeHint Optional tuple-space chunk dimensions forwarded to the selected store factory
 * @param initializationMode Initial physical-storage policy forwarded to the selected store factory
 * @return Shared pointer to the created AbstractDataStore
 */
template <class T>
std::shared_ptr<AbstractDataStore<T>> CreateDataStoreWithFormat(const std::string& dataFormat, const ShapeType& tupleShape, const ShapeType& componentShape,
                                                                IDataAction::Mode mode = IDataAction::Mode::Execute, const std::optional<ShapeType>& chunkShapeHint = {},
                                                                DataStoreInitializationMode initializationMode = DataStoreInitializationMode::Default)
{
  switch(mode)
  {
  case IDataAction::Mode::Preflight: {
    // The placeholder validates its shape. This overload has no Result channel, so an
    // invalid shape throws like the invalid-mode case below.
    auto placeholderResult = EmptyDataStore<T>::Create(tupleShape, componentShape, std::string{});
    if(placeholderResult.invalid())
    {
      throw std::runtime_error(placeholderResult.errors().front().message);
    }
    return std::shared_ptr<AbstractDataStore<T>>(std::move(placeholderResult.value()));
  }
  case IDataAction::Mode::Execute: {
    return GetIOCollection().createDataStoreWithType<T>(dataFormat, tupleShape, componentShape, chunkShapeHint, initializationMode);
  }
  default: {
    throw std::runtime_error("Invalid mode");
  }
  }
}

/**
 * @brief Creates a ListStore whose format is resolved through the IOCollection's
 * registered format resolver.
 *
 * Execute mode applies an explicit format and then the DataStructure resolver.
 *
 * Preflight mode returns an EmptyListStore with shape metadata. It does not
 * consult the resolver or allocate backing storage.
 *
 * NeighborList tuple lengths are unknown at creation. The resolver receives
 * tupleCount * sizeof(T) as a lower-bound size estimate.
 *
 * Construct an in-memory ListStore directly for scratch that has no owning
 * DataStructure and DataPath.
 *
 * @tparam T Specifies the list element type.
 * @param dataStructure Contains the future list and supplies resolution context.
 * @param arrayPath Identifies the future list and its geometry ancestors.
 * @param tupleShape Specifies tuple dimensions.
 * @param mode Selects metadata-only preflight or backing-store execution.
 * @param dataFormat Explicit format, or an empty name to use the resolver.
 * @return Created store, or null if no manager supports the resolved format and type.
 * @throws std::runtime_error If mode is not valid.
 */
template <class T>
std::shared_ptr<AbstractListStore<T>> CreateListStore(const DataStructure& dataStructure, const DataPath& arrayPath, const ShapeType& tupleShape, IDataAction::Mode mode = IDataAction::Mode::Execute,
                                                      const std::string& dataFormat = "")
{
  switch(mode)
  {
  case IDataAction::Mode::Preflight: {
    return std::make_unique<EmptyListStore<T>>(tupleShape);
  }
  case IDataAction::Mode::Execute: {
    // Tuple count gives a lower-bound size because list lengths are not known yet.
    const uint64 numTuples = std::accumulate(tupleShape.begin(), tupleShape.end(), 1ULL, std::multiplies<>());
    const uint64 estimatedBytes = numTuples * sizeof(T);
    const std::string resolvedFormat = ArrayCreationUtilities::ResolveStorageFormat(dataStructure, arrayPath, GetDataType<T>(), estimatedBytes, dataFormat);
    // The manager registered for the resolved format creates the concrete list store.
    return GetIOCollection().createListStoreWithType<T>(resolvedFormat, tupleShape);
  }
  default: {
    throw std::runtime_error("Invalid mode");
  }
  }
}

/**
 * @brief Copies a store into a different explicit format.
 * @tparam T Specifies the element type.
 * @param dataStore Source store.
 * @param dataFormat Explicit target format name. Empty and the canonical memory name select memory.
 * @return Converted store, or null when the normalized format is unchanged or the selected value factory returns null.
 * @throws std::runtime_error If format validation, factory selection, or value transfer fails.
 * @throws std::bad_alloc If allocation fails.
 *
 * Empty sources produce independent metadata without value allocation. Populated sources retain the legacy nullable factory-result channel.
 */
template <typename T>
std::shared_ptr<AbstractDataStore<T>> ConvertDataStore(const AbstractDataStore<T>& dataStore, const std::string& dataFormat)
{
  auto formatResult = ValidateNumericStorageFormat(dataFormat);
  if(formatResult.invalid())
  {
    throw std::runtime_error(formatResult.errors().front().message);
  }
  const std::string& selectedFormat = formatResult.value();
  if(dataStore.getDataFormat() == selectedFormat)
  {
    return nullptr;
  }

  if(dataStore.getStoreType() == IDataStore::StoreType::Empty)
  {
    auto plannedResult = EmptyDataStore<T>::Create(dataStore.getTupleShape(), dataStore.getComponentShape(), selectedFormat);
    if(plannedResult.invalid())
    {
      throw std::runtime_error(plannedResult.errors().front().message);
    }
    return std::shared_ptr<AbstractDataStore<T>>(std::move(plannedResult.value()));
  }

  // Conversion has no DataStructure or DataPath, so the caller supplies the target format.
  std::shared_ptr<AbstractDataStore<T>> newStore = GetIOCollection().createDataStoreWithType<T>(selectedFormat, dataStore.getTupleShape(), dataStore.getComponentShape());
  if(newStore == nullptr)
  {
    return nullptr;
  }

  newStore->copy(dataStore);
  return newStore;
}
} // namespace nx::core::DataStoreUtilities
