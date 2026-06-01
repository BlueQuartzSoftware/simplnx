#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/SimplnxConfig.hpp"
#ifdef SIMPLNX_USE_OOC
#include "SimplnxOoc/OocDataIOManager.hpp"
#endif

#include "simplnx/Common/Result.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/MemoryUtilities.hpp"
#include "simplnx/Utilities/StringInterpretationUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>

#include <numeric>

namespace nx::core::ArrayCreationUtilities
{
/**
 * @brief Checks whether an in-core allocation of the requested size fits within available system memory.
 * @param dataStructure The DataStructure whose current memory usage is added to the requirement
 * @param requiredMemory Size in bytes of the new in-core allocation being considered
 * @return true if the combined memory requirement fits within available memory, false otherwise
 */
SIMPLNX_EXPORT bool CheckMemoryRequirement(const DataStructure& dataStructure, uint64 requiredMemory);

/**
 * @brief Creates a DataArray with the given properties
 * @tparam T Primitive Type (int, float, ...)
 * @param dataStructure The DataStructure to use
 * @param tupleShape The Tuple Dimensions
 * @param nComp The number of components in the DataArray
 * @param path The DataPath to where the data will be stored.
 * @param mode The mode to assume: PREFLIGHT or EXECUTE. Preflight will NOT allocate any storage. EXECUTE will allocate the memory/storage
 * @return
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

  // Validate Number of Components
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

  // Resolve the storage format. When OOC is compiled in (SIMPLNX_USE_OOC), this
  // is a direct call to SimplnxOoc::resolveFormat, the single decision point for
  // whether an array uses in-core or OOC storage; it considers parent geometry
  // type, user preferences, and data size. When OOC is not compiled in, the
  // call is gated out and all arrays default to in-core.
  //
  // SimplnxOoc::resolveFormat always returns in-core for arrays under
  // unstructured/poly geometries because OOC support for those geometry types
  // has been deferred. See SimplnxOoc::resolveFormat for the full rationale.
  std::string resolvedFormat;
  if(mode == IDataAction::Mode::Execute)
  {
    if(!dataFormat.empty())
    {
      // User explicitly chose a format via the filter UI — skip format resolution.
      // Both k_InMemoryFormat and any other registered format name (e.g., "HDF5-OOC")
      // pass through unchanged; the DataStore factory in DataIOCollection routes
      // k_InMemoryFormat to the built-in core manager directly.
      resolvedFormat = dataFormat;
    }
    else
    {
      // No per-filter override — call SimplnxOoc::resolveFormat directly (it consults
      // user preferences, size thresholds, and geometry type). It returns either "" for
      // "default in-memory" or a format name like "HDF5-OOC". When OOC is not compiled
      // in, the call is gated out and everything stays in-core.
#ifdef SIMPLNX_USE_OOC
      resolvedFormat = SimplnxOoc::resolveFormat(dataStructure, path, GetDataType<T>(), requiredMemory);
#else
      resolvedFormat = "";
#endif
    }

    // Only check RAM availability for in-core arrays. OOC arrays go to disk
    // and do not consume RAM for their primary storage. "In-core" means either
    // the empty/unset sentinel (resolver defaulted) or the explicit k_InMemoryFormat
    // constant (user forced in-memory).
    const bool isInCore = resolvedFormat.empty() || resolvedFormat == Preferences::k_InMemoryFormat.str();
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

  auto store = DataStoreUtilities::CreateDataStore<T>(tupleShape, compShape, mode, resolvedFormat);
  if(nullptr == store)
  {
    // No registered IO manager could produce a DataStore<T> for this format.
    // Include the full manager capability list so the user can tell whether
    // the format is a typo, whether the required plugin is missing, or whether
    // the format simply does not support this store type.
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
        // Only base data store has initialization value
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

  return {};
}
} // namespace nx::core::ArrayCreationUtilities
