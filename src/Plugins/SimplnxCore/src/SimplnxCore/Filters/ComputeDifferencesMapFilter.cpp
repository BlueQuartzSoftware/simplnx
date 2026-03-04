#include "ComputeDifferencesMapFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeDifferencesMap.hpp"

#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <optional>
#include <vector>

namespace nx::core
{

namespace
{
constexpr int32 k_ComponentCountMismatchError = -90003;
constexpr int32 k_InvalidNumTuples = -90004;

IFilter::PreflightResult validateArrayTypes(const DataStructure& dataStructure, const std::vector<DataPath>& dataPaths)
{
  std::optional<DataType> dataType = {};
  for(const auto& dataPath : dataPaths)
  {
    if(auto dataArray = dataStructure.getDataAs<AbstractDataArray>(dataPath))
    {
      if(!dataType.has_value())
      {
        dataType = dataArray->getDataType();
      }
      else if(dataType != dataArray->getDataType())
      {
        std::string ss = fmt::format("Selected Attribute Arrays must all be of the same type");
        return {MakeErrorResult<OutputActions>(-90001, ss)};
      }
    }
    else
    {
      std::string ss = fmt::format("Selected DataPath must point to a DataArray");
      return {MakeErrorResult<OutputActions>(-90002, ss)};
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
  params.insert(std::make_unique<ArraySelectionParameter>(k_FirstInputArrayPath_Key, "First Input Array", "DataPath to the first input DataArray", DataPath{}, GetAllDataTypes()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SecondInputArrayPath_Key, "Second Input Array", "DataPath to the second input DataArray", DataPath{}, GetAllDataTypes()));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DifferenceMapArrayPath_Key, "Difference Map", "DataPath for created Difference Map DataArray", DataPath{}));
  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeDifferencesMapFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeDifferencesMapFilter::clone() const
{
  return std::make_unique<ComputeDifferencesMapFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeDifferencesMapFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto firstInputArrayPath = filterArgs.value<DataPath>(k_FirstInputArrayPath_Key);
  auto secondInputArrayPath = filterArgs.value<DataPath>(k_SecondInputArrayPath_Key);
  auto differenceMapArrayPath = filterArgs.value<DataPath>(k_DifferenceMapArrayPath_Key);

  std::vector<DataPath> dataArrayPaths;

  const auto& firstInputArray = dataStructure.getDataRefAs<AbstractDataArray>(firstInputArrayPath);
  dataArrayPaths.push_back(firstInputArrayPath);

  const auto& secondInputArray = dataStructure.getDataRefAs<AbstractDataArray>(secondInputArrayPath);
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

  if(firstInputArray.getNumberOfComponents() != secondInputArray.getNumberOfComponents())
  {
    std::string ss = fmt::format("Selected Attribute Arrays must have the same component dimensions");
    return {MakeErrorResult<OutputActions>(nx::core::k_ComponentCountMismatchError, ss)};
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
  DataType dataType = firstInputArray.getDataType();
  auto action = std::make_unique<CreateArrayAction>(dataType, firstInputArray.getIDataStore()->getTupleShape(), firstInputArray.getIDataStore()->getComponentShape(), differenceMapArrayPath,
                                                    firstInputArray.getDataFormat());

  //
  nx::core::Result<OutputActions> actions;
  actions.value().appendAction(std::move(action));
  actions.m_Warnings = warnings;

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ComputeDifferencesMapFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ComputeDifferencesMapInputValues inputValues;

  inputValues.DifferenceMapArrayPath = filterArgs.value<ArrayCreationParameter::ValueType>(k_DifferenceMapArrayPath_Key);
  inputValues.FirstInputArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_FirstInputArrayPath_Key);
  inputValues.SecondInputArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_SecondInputArrayPath_Key);
  return ComputeDifferencesMap(dataStructure, messageHandler, shouldCancel, &inputValues)();

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
