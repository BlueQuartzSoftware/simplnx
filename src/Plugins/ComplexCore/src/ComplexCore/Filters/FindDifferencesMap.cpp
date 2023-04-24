#include "FindDifferencesMap.hpp"

#include "complex/DataStructure/AbstractDataStore.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

#include <optional>
#include <vector>

namespace complex
{
namespace
{
constexpr int32 k_MissingInputArray = -567;
constexpr int32 k_ComponentCountMismatchError = -90003;
constexpr int32 k_TupleCountMismatchError = -90004;

IFilter::PreflightResult validateArrayTypes(const DataStructure& data, const std::vector<DataPath>& dataPaths)
{
  std::optional<DataType> dataType = {};
  for(const auto& dataPath : dataPaths)
  {
    if(auto dataArray = data.getDataAs<IDataArray>(dataPath))
    {
      if(!dataType.has_value())
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

WarningCollection warnOnUnsignedTypes(const DataStructure& data, const std::vector<DataPath>& paths)
{
  WarningCollection results;
  for(const auto& dataPath : paths)
  {
    if(data.getDataAs<UInt8Array>(dataPath))
    {
      std::string ss = fmt::format("Selected Attribute Arrays are of type uint8_t. Using unsigned integer types may result in underflow leading to extremely large values!");
      results.push_back(Warning{-90004, ss});
    }
    if(data.getDataAs<UInt16Array>(dataPath))
    {
      std::string ss = fmt::format("Selected Attribute Arrays are of type uint16_t. Using unsigned integer types may result in underflow leading to extremely large values!");
      results.push_back(Warning{-90005, ss});
    }
    if(data.getDataAs<UInt32Array>(dataPath))
    {
      std::string ss = fmt::format("Selected Attribute Arrays are of type uint32_t. Using unsigned integer types may result in underflow leading to extremely large values!");
      results.push_back(Warning{-90006, ss});
    }
    if(data.getDataAs<UInt64Array>(dataPath))
    {
      std::string ss = fmt::format("Selected Attribute Arrays are of type uint64_t. Using unsigned integer types may result in underflow leading to extremely large values!");
      results.push_back(Warning{-90007, ss});
    }
  }
  return results;
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
        auto firstVal = firstArray->getValue(numComps * i + j);
        auto secondVal = secondArray->getValue(numComps * i + j);
        auto diffVal = firstVal > secondVal ? firstVal - secondVal : secondVal - firstVal;
        differenceMap->setValue(numComps * i + j, diffVal);
      }
    }
  }

private:
  IDataArray* m_FirstArray;
  IDataArray* m_SecondArray;
  IDataArray* m_DifferenceMap;
};

struct ExecuteFindDifferenceMapFunctor
{
  template <typename DataType>
  void operator()(IDataArray* firstArrayPtr, IDataArray* secondArrayPtr, IDataArray* differenceMapPtr)
  {
    size_t numTuples = firstArrayPtr->getNumberOfTuples();
    FindDifferenceMapImpl<DataType> serial(firstArrayPtr, secondArrayPtr, differenceMapPtr);
    serial.generate(0, numTuples);
  }
};
} // namespace

//------------------------------------------------------------------------------
std::string FindDifferencesMap::name() const
{
  return FilterTraits<FindDifferencesMap>::name;
}

//------------------------------------------------------------------------------
std::string FindDifferencesMap::className() const
{
  return FilterTraits<FindDifferencesMap>::className;
}

//------------------------------------------------------------------------------
Uuid FindDifferencesMap::uuid() const
{
  return FilterTraits<FindDifferencesMap>::uuid;
}

//------------------------------------------------------------------------------
std::string FindDifferencesMap::humanName() const
{
  return "Find Differences Map";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindDifferencesMap::defaultTags() const
{
  return {"Statistics", "ComplexCore"};
}

//------------------------------------------------------------------------------
Parameters FindDifferencesMap::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<DataPathSelectionParameter>(k_FirstInputArrayPath_Key, "First Input Array", "DataPath to the first input DataArray", DataPath{}));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_SecondInputArrayPath_Key, "Second Input Array", "DataPath to the second input DataArray", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DifferenceMapArrayPath_Key, "Difference Map", "DataPath for created Difference Map DataArray", DataPath{}));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindDifferencesMap::clone() const
{
  return std::make_unique<FindDifferencesMap>();
}

//------------------------------------------------------------------------------
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

  WarningCollection warnings;
  if(!dataArrayPaths.empty())
  {
    warnings = warnOnUnsignedTypes(data, dataArrayPaths);
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
  complex::Result<OutputActions> actions;
  actions.value().appendAction(std::move(action));
  actions.m_Warnings = warnings;

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindDifferencesMap::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                         const std::atomic_bool& shouldCancel) const
{
  auto firstInputArray = data.getDataAs<IDataArray>(args.value<DataPath>(k_FirstInputArrayPath_Key));
  auto secondInputArray = data.getDataAs<IDataArray>(args.value<DataPath>(k_SecondInputArrayPath_Key));
  auto differenceMapArray = data.getDataAs<IDataArray>(args.value<DataPath>(k_DifferenceMapArrayPath_Key));

  auto dataType = firstInputArray->getDataType();
  ExecuteDataFunction(ExecuteFindDifferenceMapFunctor{}, dataType, firstInputArray, secondInputArray, differenceMapArray);

  return {};
}
} // namespace complex
