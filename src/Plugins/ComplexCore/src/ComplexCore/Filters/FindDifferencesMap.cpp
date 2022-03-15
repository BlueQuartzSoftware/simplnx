#include "FindDifferencesMap.hpp"

#include <vector>

#include "complex/DataStructure/AbstractDataStore.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"

namespace complex
{
namespace
{
constexpr int32 k_MissingInputArray = -567;
constexpr int32 k_InputArrayTypeError = -90000;
constexpr int32 k_ComponentCountMismatchError = -90003;
constexpr int32 k_TupleCountMismatchError = -90004;

IFilter::PreflightResult validateArrayTypes(const DataStructure& data, const std::vector<DataPath>& dataPaths)
{
  DataType dataType = DataType::error;
  for(const auto& dataPath : dataPaths)
  {
    if(data.getDataAs<BoolArray>(dataPath) != nullptr)
    {
      std::string ss = fmt::format("Selected Attribute Arrays cannot be of type bool");
      return {nonstd::make_unexpected(std::vector<Error>{Error{k_InputArrayTypeError, ss}})};
    }
    if(auto dataArray = data.getDataAs<IDataArray>(dataPath))
    {
      if(dataType == DataType::error)
      {
        dataType = dataArray->getDataType();
      }
      else if(dataType != dataArray->getDataType())
      {
        std::string ss = fmt::format("Selected Attribute Arrays must all be of the same type");
        return {nonstd::make_unexpected(std::vector<Error>{Error{-90001, ss}})};
      }
    }
    else
    {
      std::string ss = fmt::format("Selected DataPath must point to a DataArray");
      return {nonstd::make_unexpected(std::vector<Error>{Error{-90002, ss}})};
    }
  }
  return {};
}

IFilter::PreflightResult warnOnUnsignedTypes(const DataStructure& data, const std::vector<DataPath>& paths)
{
  for(const auto& dataPath : paths)
  {
    if(data.getDataAs<UInt8Array>(dataPath))
    {
      std::string ss = fmt::format("Selected Attribute Arrays are of type uint8_t. Using unsigned integer types may result in underflow leading to extremely large values!");
      return {nonstd::make_unexpected(std::vector<Error>{Error{-90004, ss}})};
    }
    if(data.getDataAs<UInt16Array>(dataPath))
    {
      std::string ss = fmt::format("Selected Attribute Arrays are of type uint16_t. Using unsigned integer types may result in underflow leading to extremely large values!");
      return {nonstd::make_unexpected(std::vector<Error>{Error{-90005, ss}})};
    }
    if(data.getDataAs<UInt32Array>(dataPath))
    {
      std::string ss = fmt::format("Selected Attribute Arrays are of type uint32_t. Using unsigned integer types may result in underflow leading to extremely large values!");
      return {nonstd::make_unexpected(std::vector<Error>{Error{-90006, ss}})};
    }
    if(data.getDataAs<UInt64Array>(dataPath))
    {
      std::string ss = fmt::format("Selected Attribute Arrays are of type uint64_t. Using unsigned integer types may result in underflow leading to extremely large values!");
      return {nonstd::make_unexpected(std::vector<Error>{Error{-90007, ss}})};
    }
  }
  return {};
}

/**
 * @brief The FindDifferenceMapImpl class implements a threaded algorithm that computes the difference map
 * between two arrays
 */
template <typename DataType>
class FindDifferenceMapImpl
{
  using store_type = AbstractDataStore<DataType>;

public:
  FindDifferenceMapImpl(IDataArray* firstArray, IDataArray* secondArray, IDataArray* differenceMap)
  : m_FirstArray(firstArray)
  , m_SecondArray(secondArray)
  , m_DifferenceMap(differenceMap)
  {
  }
  virtual ~FindDifferenceMapImpl() = default;

  void generate(size_t start, size_t end) const
  {
    auto firstArray = dynamic_cast<store_type*>(m_FirstArray->getIDataStore());
    auto secondArray = dynamic_cast<store_type*>(m_SecondArray->getIDataStore());
    auto differenceMap = dynamic_cast<store_type*>(m_DifferenceMap->getIDataStore());

    int32 numComps = firstArray->getNumberOfComponents();

    for(usize i = start; i < end; i++)
    {
      for(int32 j = 0; j < numComps; j++)
      {
        differenceMap->setValue(numComps * i + j, firstArray->getValue(numComps * i + j) - secondArray->getValue(numComps * i + j));
      }
    }
  }

private:
  IDataArray* m_FirstArray;
  IDataArray* m_SecondArray;
  IDataArray* m_DifferenceMap;
};

template <typename DataType>
void ExecuteFindDifferenceMap(IDataArray* firstArrayPtr, IDataArray* secondArrayPtr, IDataArray* differenceMapPtr)
{
  size_t numTuples = firstArrayPtr->getNumberOfTuples();

  {
    FindDifferenceMapImpl<DataType> serial(firstArrayPtr, secondArrayPtr, differenceMapPtr);
    serial.generate(0, numTuples);
  }
}
} // namespace

std::string FindDifferencesMap::name() const
{
  return FilterTraits<FindDifferencesMap>::name;
}

std::string FindDifferencesMap::className() const
{
  return FilterTraits<FindDifferencesMap>::className;
}

Uuid FindDifferencesMap::uuid() const
{
  return FilterTraits<FindDifferencesMap>::uuid;
}

std::string FindDifferencesMap::humanName() const
{
  return "Find Differences Map";
}

Parameters FindDifferencesMap::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<DataPathSelectionParameter>(k_FirstInputArrayPath_Key, "First Input Array", "DataPath to the first input DataArray", DataPath{}));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_SecondInputArrayPath_Key, "Second Input Array", "DataPath to the second input DataArray", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DifferenceMapArrayPath_Key, "Difference Map", "DataPath for created Difference Map DataArray", DataPath{}));
  return params;
}

IFilter::UniquePointer FindDifferencesMap::clone() const
{
  return std::make_unique<FindDifferencesMap>();
}

IFilter::PreflightResult FindDifferencesMap::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto firstInputArrayPath = args.value<DataPath>(k_FirstInputArrayPath_Key);
  auto secondInputArrayPath = args.value<DataPath>(k_SecondInputArrayPath_Key);
  auto differenceMapArrayPath = args.value<DataPath>(k_DifferenceMapArrayPath_Key);

  std::vector<DataPath> dataArrayPaths;

  auto* firstInputArray = data.getDataAs<IDataArray>(firstInputArrayPath);
  if(firstInputArray == nullptr)
  {
    std::string ss = fmt::format("Could not find input array at path {}", firstInputArrayPath.toString());
    return {nonstd::make_unexpected(std::vector<Error>{Error{-k_MissingInputArray, ss}})};
  }

  dataArrayPaths.push_back(firstInputArrayPath);

  auto* secondInputArray = data.getDataAs<IDataArray>(secondInputArrayPath);
  if(secondInputArray == nullptr)
  {
    std::string ss = fmt::format("Could not find input array at path {}", secondInputArrayPath.toString());
    return {nonstd::make_unexpected(std::vector<Error>{Error{-k_MissingInputArray, ss}})};
  }
  dataArrayPaths.push_back(secondInputArrayPath);

  if(!dataArrayPaths.empty())
  {
    auto result = validateArrayTypes(data, dataArrayPaths);
    if(result.outputActions.invalid())
    {
      return result;
    }
  }

  if(!dataArrayPaths.empty())
  {
    auto result = warnOnUnsignedTypes(data, dataArrayPaths);
    if(result.outputActions.invalid())
    {
      return result;
    }
  }

  // Safe to check array component dimensions since we won't get here if the pointers are null
  if(firstInputArray->getNumberOfComponents() != secondInputArray->getNumberOfComponents())
  {
    std::string ss = fmt::format("Selected Attribute Arrays must have the same component dimensions");
    return {nonstd::make_unexpected(std::vector<Error>{Error{complex::k_ComponentCountMismatchError, ss}})};
  }

  if(!data.validateNumberOfTuples(dataArrayPaths))
  {
    std::string ss = fmt::format("Tuple count not consistent between input arrays.");
    return {nonstd::make_unexpected(std::vector<Error>{Error{complex::k_TupleCountMismatchError, ss}})};
  }

  // At this point we have two valid arrays of the same type and component dimensions, so we
  // are safe to make the output array with the correct type and component dimensions
  DataType dataType = firstInputArray->getDataType();
  auto action = std::make_unique<CreateArrayAction>(dataType, firstInputArray->getIDataStore()->getTupleShape(), firstInputArray->getIDataStore()->getComponentShape(), differenceMapArrayPath);

  //
  OutputActions actions;
  actions.actions.push_back(std::move(action));

  return {std::move(actions)};
}

Result<> FindDifferencesMap::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                         const std::atomic_bool& shouldCancel) const
{
  auto firstInputArrayPath = args.value<DataPath>(k_FirstInputArrayPath_Key);
  auto secondInputArrayPath = args.value<DataPath>(k_SecondInputArrayPath_Key);
  auto differenceMapArrayPath = args.value<DataPath>(k_DifferenceMapArrayPath_Key);

  auto firstInputArray = data.getDataAs<IDataArray>(firstInputArrayPath);
  auto secondInputArray = data.getDataAs<IDataArray>(secondInputArrayPath);
  auto differenceMapArray = data.getDataAs<IDataArray>(differenceMapArrayPath);

  auto dataType = firstInputArray->getDataType();

  switch(dataType)
  {
  case DataType::int8:
    ExecuteFindDifferenceMap<int8>(firstInputArray, secondInputArray, differenceMapArray);
    break;
  case DataType::int16:
    ExecuteFindDifferenceMap<int16>(firstInputArray, secondInputArray, differenceMapArray);
    break;
  case DataType::int32:
    ExecuteFindDifferenceMap<int32>(firstInputArray, secondInputArray, differenceMapArray);
    break;
  case DataType::int64:
    ExecuteFindDifferenceMap<int64>(firstInputArray, secondInputArray, differenceMapArray);
    break;
  case DataType::uint8:
    ExecuteFindDifferenceMap<uint8>(firstInputArray, secondInputArray, differenceMapArray);
    break;
  case DataType::uint16:
    ExecuteFindDifferenceMap<uint16>(firstInputArray, secondInputArray, differenceMapArray);
    break;
  case DataType::uint32:
    ExecuteFindDifferenceMap<uint32>(firstInputArray, secondInputArray, differenceMapArray);
    break;
  case DataType::uint64:
    ExecuteFindDifferenceMap<uint64>(firstInputArray, secondInputArray, differenceMapArray);
    break;
  case DataType::float32:
    ExecuteFindDifferenceMap<float32>(firstInputArray, secondInputArray, differenceMapArray);
    break;
  case DataType::float64:
    ExecuteFindDifferenceMap<float64>(firstInputArray, secondInputArray, differenceMapArray);
    break;
  case DataType::boolean:
    ExecuteFindDifferenceMap<bool>(firstInputArray, secondInputArray, differenceMapArray);
    break;
  case DataType::error:
    std::string ss = fmt::format("Cannot handle indeterminate array types");
    return {nonstd::make_unexpected(std::vector<Error>{Error{-90006, ss}})};
  }

  return {};
}
} // namespace complex
