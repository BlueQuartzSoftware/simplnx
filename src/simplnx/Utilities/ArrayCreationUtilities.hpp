#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/MemoryUtilities.hpp"
#include "simplnx/Utilities/StringInterpretationUtilities.hpp"

#include <fmt/format.h>

#include <numeric>

namespace nx::core::ArrayCreationUtilities
{
SIMPLNX_EXPORT bool CheckMemoryRequirement(DataStructure& dataStructure, uint64 requiredMemory, std::string& format);

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
Result<> CreateArray(DataStructure& dataStructure, const std::vector<usize>& tupleShape, const std::vector<usize>& compShape, const DataPath& path, IDataAction::Mode mode, std::string dataFormat = "",
                     std::string fillValue = "0")
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
  if(!CheckMemoryRequirement(dataStructure, requiredMemory, dataFormat))
  {
    uint64 totalMemory = requiredMemory + dataStructure.memoryUsage();
    uint64 availableMemory = Memory::GetTotalMemory();
    return MakeErrorResult(-264, fmt::format("CreateArray: Cannot create DataArray '{}'.\n\tTotal memory required for DataStructure: '{}' Bytes.\n\tTotal reported memory: '{}' Bytes", name,
                                             totalMemory, availableMemory));
  }

  Result<T> conversionResult = StringInterpretationUtilities::Convert<T>(fillValue);
  if(conversionResult.invalid())
  {
    return ConvertResult(std::move(conversionResult));
  }

  auto store = DataStoreUtilities::CreateDataStore<T>(tupleShape, compShape, mode, dataFormat);
  if(nullptr == store)
  {
    return MakeErrorResult(-265, fmt::format("CreateArray: Unable to create DataStore<T> at '{}' of DataStore format '{}'", path.toString(), dataFormat));
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
      std::string amShape = fmt::format("Attribute Matrix Tuple Dims: {}", fmt::join(attrMatrixPtr->getShape(), " x "));
      std::string arrayShape = fmt::format("Data Array Tuple Shape: {}", fmt::join(store->getTupleShape(), " x "));
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