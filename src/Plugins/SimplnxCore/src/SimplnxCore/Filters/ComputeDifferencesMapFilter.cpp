#include "ComputeDifferencesMapFilter.hpp"

#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/DataPathSelectionParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <optional>

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <vector>

namespace nx::core
{
namespace
{
constexpr int32 k_MissingInputArray = -567;
constexpr int32 k_ComponentCountMismatchError = -90003;
constexpr int32 k_InvalidNumTuples = -90004;

IFilter::PreflightResult validateArrayTypes(const DataStructure& dataStructure, const std::vector<DataPath>& dataPaths)
{
  std::optional<DataType> dataType = {};
  for(const auto& dataPath : dataPaths)
  {
    if(auto dataArray = dataStructure.getDataAs<IDataArray>(dataPath))
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

WarningCollection warnOnUnsignedTypes(const DataStructure& dataStructure, const std::vector<DataPath>& paths)
{
  WarningCollection results;
  for(const auto& dataPath : paths)
  {
    if(dataStructure.getDataAs<UInt8Array>(dataPath))
    {
      std::string ss = fmt::format("Selected Attribute Arrays are of type uint8_t. Using unsigned integer types may result in underflow leading to extremely large values!");
      results.push_back(Warning{-90004, ss});
    }
    if(dataStructure.getDataAs<UInt16Array>(dataPath))
    {
      std::string ss = fmt::format("Selected Attribute Arrays are of type uint16_t. Using unsigned integer types may result in underflow leading to extremely large values!");
      results.push_back(Warning{-90005, ss});
    }
    if(dataStructure.getDataAs<UInt32Array>(dataPath))
    {
      std::string ss = fmt::format("Selected Attribute Arrays are of type uint32_t. Using unsigned integer types may result in underflow leading to extremely large values!");
      results.push_back(Warning{-90006, ss});
    }
    if(dataStructure.getDataAs<UInt64Array>(dataPath))
    {
      std::string ss = fmt::format("Selected Attribute Arrays are of type uint64_t. Using unsigned integer types may result in underflow leading to extremely large values!");
      results.push_back(Warning{-90007, ss});
    }
  }
  return results;
}

struct ExecuteFindDifferenceMapFunctor
{
  template <typename DataType>
  void operator()(IDataArray* firstArrayPtr, IDataArray* secondArrayPtr, IDataArray* differenceMapPtr)
  {
    using store_type = AbstractDataStore<DataType>;

    auto& firstArray = firstArrayPtr->getIDataStoreRefAs<store_type>();
    auto& secondArray = secondArrayPtr->getIDataStoreRefAs<store_type>();
    auto& differenceMap = differenceMapPtr->getIDataStoreRefAs<store_type>();

    usize numTuples = firstArray.getNumberOfTuples();
    int32 numComps = firstArray.getNumberOfComponents();

    for(usize i = 0; i < numTuples; i++)
    {
      for(int32 j = 0; j < numComps; j++)
      {
        auto firstVal = firstArray[numComps * i + j];
        auto secondVal = secondArray[numComps * i + j];
        auto diffVal = firstVal > secondVal ? firstVal - secondVal : secondVal - firstVal;
        differenceMap[numComps * i + j] = diffVal;
      }
    }
  }
};
} // namespace

//------------------------------------------------------------------------------
std::string ComputeDifferencesMapFilter::name() const
{
  return FilterTraits<ComputeDifferencesMapFilter>::name;
}

//------------------------------------------------------------------------------
std::string ComputeDifferencesMapFilter::className() const
{
  return FilterTraits<ComputeDifferencesMapFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeDifferencesMapFilter::uuid() const
{
  return FilterTraits<ComputeDifferencesMapFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeDifferencesMapFilter::humanName() const
{
  return "Compute Differences Map";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeDifferencesMapFilter::defaultTags() const
{
  return {className(), "Statistics", "SimplnxCore", "Find"};
}

//------------------------------------------------------------------------------
Parameters ComputeDifferencesMapFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<DataPathSelectionParameter>(k_FirstInputArrayPath_Key, "First Input Array", "DataPath to the first input DataArray", DataPath{}));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_SecondInputArrayPath_Key, "Second Input Array", "DataPath to the second input DataArray", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DifferenceMapArrayPath_Key, "Difference Map", "DataPath for created Difference Map DataArray", DataPath{}));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeDifferencesMapFilter::clone() const
{
  return std::make_unique<ComputeDifferencesMapFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeDifferencesMapFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  auto firstInputArrayPath = args.value<DataPath>(k_FirstInputArrayPath_Key);
  auto secondInputArrayPath = args.value<DataPath>(k_SecondInputArrayPath_Key);
  auto differenceMapArrayPath = args.value<DataPath>(k_DifferenceMapArrayPath_Key);

  std::vector<DataPath> dataArrayPaths;

  auto* firstInputArray = dataStructure.getDataAs<IDataArray>(firstInputArrayPath);
  if(firstInputArray == nullptr)
  {
    std::string ss = fmt::format("Could not find input array at path {}", firstInputArrayPath.toString());
    return {nonstd::make_unexpected(std::vector<Error>{Error{-k_MissingInputArray, ss}})};
  }

  dataArrayPaths.push_back(firstInputArrayPath);

  auto* secondInputArray = dataStructure.getDataAs<IDataArray>(secondInputArrayPath);
  if(secondInputArray == nullptr)
  {
    std::string ss = fmt::format("Could not find input array at path {}", secondInputArrayPath.toString());
    return {nonstd::make_unexpected(std::vector<Error>{Error{-k_MissingInputArray, ss}})};
  }
  dataArrayPaths.push_back(secondInputArrayPath);

  if(!dataArrayPaths.empty())
  {
    auto result = validateArrayTypes(dataStructure, dataArrayPaths);
    if(result.outputActions.invalid())
    {
      return result;
    }
  }

  WarningCollection warnings;
  if(!dataArrayPaths.empty())
  {
    warnings = warnOnUnsignedTypes(dataStructure, dataArrayPaths);
  }

  // Safe to check array component dimensions since we won't get here if the pointers are null
  if(firstInputArray->getNumberOfComponents() != secondInputArray->getNumberOfComponents())
  {
    std::string ss = fmt::format("Selected Attribute Arrays must have the same component dimensions");
    return {nonstd::make_unexpected(std::vector<Error>{Error{nx::core::k_ComponentCountMismatchError, ss}})};
  }

  // validate the number of tuples
  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataArrayPaths);
  if(!tupleValidityCheck)
  {
    return {
        MakeErrorResult<OutputActions>(k_InvalidNumTuples, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  // At this point we have two valid arrays of the same type and component dimensions, so we
  // are safe to make the output array with the correct type and component dimensions
  DataType dataType = firstInputArray->getDataType();
  auto action = std::make_unique<CreateArrayAction>(dataType, firstInputArray->getIDataStore()->getTupleShape(), firstInputArray->getIDataStore()->getComponentShape(), differenceMapArrayPath,
                                                    firstInputArray->getDataFormat());

  //
  nx::core::Result<OutputActions> actions;
  actions.value().appendAction(std::move(action));
  actions.m_Warnings = warnings;

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ComputeDifferencesMapFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  auto* firstInputArray = dataStructure.getDataAs<IDataArray>(args.value<DataPath>(k_FirstInputArrayPath_Key));
  auto* secondInputArray = dataStructure.getDataAs<IDataArray>(args.value<DataPath>(k_SecondInputArrayPath_Key));
  auto* differenceMapArray = dataStructure.getDataAs<IDataArray>(args.value<DataPath>(k_DifferenceMapArrayPath_Key));

  ExecuteDataFunction(ExecuteFindDifferenceMapFunctor{}, firstInputArray->getDataType(), firstInputArray, secondInputArray, differenceMapArray);

  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_FirstInputArrayPathKey = "FirstInputArrayPath";
constexpr StringLiteral k_SecondInputArrayPathKey = "SecondInputArrayPath";
constexpr StringLiteral k_DifferenceMapArrayPathKey = "DifferenceMapArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeDifferencesMapFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeDifferencesMapFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FirstInputArrayPathKey, k_FirstInputArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SecondInputArrayPathKey, k_SecondInputArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayCreationFilterParameterConverter>(args, json, SIMPL::k_DifferenceMapArrayPathKey, k_DifferenceMapArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
