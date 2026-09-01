#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/MemoryUtilities.hpp"
#include "simplnx/Utilities/StringInterpretationUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <numeric>

/**
 * @namespace nx::core::ArrayCreationUtilities
 * @brief Contains storage-aware array creation utilities.
 */
namespace nx::core::ArrayCreationUtilities
{
/**
 * @brief Tests projected in-memory use against cached total physical memory.
 * @param dataStructure Supplies current in-memory usage.
 * @param requiredMemory Specifies bytes for the new allocation.
 * @return True only when projected use is less than total physical memory.
 * @pre The usage sum fits in uint64.
 */
SIMPLNX_EXPORT bool CheckMemoryRequirement(const DataStructure& dataStructure, uint64 requiredMemory);

/**
 * @brief Resolves a storage format with one shared priority order.
 * @param dataStructure Contains or will contain the object.
 * @param path Identifies the object.
 * @param numericType Specifies the element data type.
 * @param dataSizeBytes Total bytes, or zero when size is unknown.
 * @param requestedFormat Explicit format, or an empty name to use the resolver.
 * @return Registered format name, or an empty name for the in-memory default.
 *
 * A nonempty requestedFormat wins. The DataStructure resolver handles the
 * remaining automatic request.
 * All array and list creation routes use this function to keep that order consistent.
 */
SIMPLNX_EXPORT std::string ResolveStorageFormat(const DataStructure& dataStructure, const DataPath& path, DataType numericType, uint64 dataSizeBytes, const std::string& requestedFormat);

/**
 * @brief Tests whether projected in-memory use exceeds available bytes without addition overflow.
 * @param currentUsageBytes Current in-memory use in bytes.
 * @param requiredMemory Bytes for the new allocation.
 * @param availableBytes Currently available physical-memory bytes.
 * @return True only when projected use is greater than availableBytes.
 */
[[nodiscard]] SIMPLNX_EXPORT bool WouldExceedAvailableMemory(uint64 currentUsageBytes, uint64 requiredMemory, uint64 availableBytes);

/**
 * @brief Creates a DataArray with resolved backing storage.
 * @tparam T Specifies the element type.
 * @param dataStructure Receives the DataArray.
 * @param tupleShape Specifies tuple dimensions.
 * @param compShape Specifies component dimensions.
 * @param path Identifies the new DataArray.
 * @param mode Selects metadata-only preflight or backing-store execution.
 * @param dataFormat Explicit format, or an empty name to use the resolver.
 * @param fillValue Optional value text validated for T.
 * @return Valid result with possible preflight warnings, or a validation, memory, format, conversion, or insertion error.
 * @throws std::runtime_error If mode is not valid.
 * @pre Shape products and the byte count fit in usize and uint64.
 */
template <class T>
Result<> CreateArray(DataStructure& dataStructure, const ShapeType& tupleShape, const ShapeType& compShape, const DataPath& path, IDataAction::Mode mode, const std::string& dataFormat = "",
                     std::string fillValue = "")
{
  auto parentPath = path.getParent();

  std::optional<DataObject::IdType> dataObjectId;

  DataObject* parentObjectPtr = nullptr;
  if(parentPath.getLength() != 0)
  {
    parentObjectPtr = dataStructure.getData(parentPath);
    if(parentObjectPtr == nullptr)
    {
      return MakeErrorResult(-260, fmt::format("CreateArray: Parent object '{}' does not exist", parentPath.toString()));
    }

    dataObjectId = parentObjectPtr->getId();
  }

  if(tupleShape.empty())
  {
    return MakeErrorResult(-261, fmt::format("CreateArray: Tuple Shape was empty. Please set the number of tuples."));
  }

  if(compShape.empty())
  {
    return MakeErrorResult(-262, fmt::format("CreateArray: Component Shape was empty. Please set the number of components."));
  }
  const usize numComponents = std::accumulate(compShape.cbegin(), compShape.cend(), static_cast<usize>(1), std::multiplies<>());
  if(numComponents == 0 && mode == IDataAction::Mode::Execute)
  {
    return MakeErrorResult(-263, fmt::format("CreateArray: Number of components is ZERO. Please set the number of components."));
  }

  const usize last = path.getLength() - 1;

  std::string name = path[last];

  const usize numTuples = std::accumulate(tupleShape.cbegin(), tupleShape.cend(), static_cast<usize>(1), std::multiplies<>());
  uint64 requiredMemory = numTuples * numComponents * sizeof(T);

  // Resolve once so preflight memory reporting and execution select the same format.
  const std::string resolvedFormat = ResolveStorageFormat(dataStructure, path, GetDataType<T>(), requiredMemory, dataFormat);
  const bool isInCore = resolvedFormat.empty() || resolvedFormat == Preferences::k_InMemoryFormat.str();

  // Accumulates any non-blocking warnings to return on the success path.
  Result<> result;

  if(mode == IDataAction::Mode::Execute)
  {
    // Execute rejects an in-memory array whose projected use reaches total physical memory.
    if(isInCore && !CheckMemoryRequirement(dataStructure, requiredMemory))
    {
      uint64 totalMemory = requiredMemory + dataStructure.memoryUsage();
      uint64 availableMemory = Memory::GetTotalMemory();
      return MakeErrorResult(-264, fmt::format("Cannot create array '{}': the DataStructure would require {} bytes total, "
                                               "but only {} bytes of RAM are available. Consider enabling out-of-core "
                                               "storage or lowering the size thresholds in Preferences so that large "
                                               "arrays are stored on disk instead of in memory.",
                                               path.toString(), totalMemory, availableMemory));
    }
  }
  else
  {
    // Preflight warns about projected in-memory use but does not block the pipeline.
    // Out-of-core arrays do not contribute resident bytes to this warning.
    constexpr double k_BytesPerGiB = 1024.0 * 1024.0 * 1024.0;
    // One memory snapshot drives both the condition and its diagnostic values.
    // Clamp availability because operating-system reporting can briefly show used memory above total memory.
    const Memory::SystemMemoryInfo info = Memory::GetSystemMemoryInfo();
    const double availableGiB = std::max(0.0, info.totalGB - info.usedGB);
    const uint64 availableBytes = static_cast<uint64>(availableGiB * k_BytesPerGiB);
    if(isInCore && WouldExceedAvailableMemory(dataStructure.memoryUsage(), requiredMemory, availableBytes))
    {
      const double requiredGiB = static_cast<double>(requiredMemory) / k_BytesPerGiB;
      const double projectedGiB = (static_cast<double>(dataStructure.memoryUsage()) + static_cast<double>(requiredMemory)) / k_BytesPerGiB;
      result.warnings().emplace_back(Warning{-271, fmt::format("Creating array '{}' (~{:.1f} GB) would bring in-core memory to ~{:.1f} GB, "
                                                               "above this machine's currently-available ~{:.1f} GB. It may swap badly or "
                                                               "fail. Consider out-of-core storage for this data.",
                                                               path.toString(), requiredGiB, projectedGiB, availableGiB)});
    }
  }

  // Preflight creates metadata only. Execute passes the resolved format to its registered manager.
  std::shared_ptr<AbstractDataStore<T>> store;
  switch(mode)
  {
  case IDataAction::Mode::Preflight: {
    // Preserve an OOC format so memoryUsage() reports zero resident bytes.
    // Normalize explicit in-memory format to the empty sentinel used by EmptyDataStore.
    store = std::make_unique<EmptyDataStore<T>>(tupleShape, compShape, isInCore ? std::string{} : resolvedFormat);
    break;
  }
  case IDataAction::Mode::Execute: {
    // The registered manager for resolvedFormat creates the concrete store.
    store = DataStoreUtilities::GetIOCollection().createDataStoreWithType<T>(resolvedFormat, tupleShape, compShape);
    break;
  }
  default: {
    throw std::runtime_error("Invalid mode");
  }
  }
  if(nullptr == store)
  {
    // Include manager capabilities so the error distinguishes an unknown format from an unsupported store type.
    return MakeErrorResult(-265, fmt::format("CreateArray: Unable to create DataStore<T> at '{}' of DataStore format '{}'.\n{}", path.toString(), resolvedFormat,
                                             DataStoreUtilities::GetIOCollection().generateManagerListString()));
  }
  if(!fillValue.empty())
  {
    Result<T> conversionResult = StringInterpretationUtilities::Convert<T>(fillValue);

    if(conversionResult.invalid())
    {
      return ConvertResult(std::move(conversionResult));
    }
    if(mode == IDataAction::Mode::Execute)
    {
      store->fill(conversionResult.value());
      {
        // Only resident DataStore records an initialization value for later resize operations.
        std::weak_ptr<DataStore<T>> weakDataStorePtr = std::dynamic_pointer_cast<DataStore<T>>(store);
        if(auto dataStorePtr = weakDataStorePtr.lock(); dataStorePtr != nullptr)
        {
          dataStorePtr->setInitValue(conversionResult.value());
        }
      }
    }
  }

  auto dataArray = DataArray<T>::Create(dataStructure, name, store, dataObjectId);
  if(dataArray == nullptr)
  {
    if(dataStructure.getId(path).has_value())
    {
      return MakeErrorResult(-266, fmt::format("CreateArray: Cannot create Data Array at path '{}' because it already exists. Choose a different name.", path.toString()));
    }

    if(parentObjectPtr == nullptr)
    {
      return MakeErrorResult(-267, fmt::format("CreateArray: Parent object '{}' does not exist", parentPath.toString()));
    }
    if(parentObjectPtr->getDataObjectType() == DataObject::Type::AttributeMatrix)
    {
      auto* attrMatrixPtr = dynamic_cast<AttributeMatrix*>(parentObjectPtr);
      std::string amShape = fmt::format("Attribute Matrix Tuple Dims: {}", StringUtilities::formatDimensions(attrMatrixPtr->getShape()));
      std::string arrayShape = fmt::format("Data Array Tuple Shape: {}", StringUtilities::formatDimensions(store->getTupleShape()));
      return MakeErrorResult(-268,
                             fmt::format("CreateArray: Unable to create Data Array '{}' inside Attribute matrix '{}'. Mismatch of tuple dimensions. The created Data Array must have the same tuple "
                                         "dimensions or the same total number of tuples.\n{}\n{}",
                                         name, dataStructure.getDataPathsForId(parentObjectPtr->getId()).front().toString(), amShape, arrayShape));
    }

    return MakeErrorResult(-269, fmt::format("CreateArray: Unable to create DataArray at '{}'", path.toString()));
  }

  return result;
}
} // namespace nx::core::ArrayCreationUtilities
