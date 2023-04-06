#include "FindArrayUniqueValuesFilter.hpp"

#include "ComplexCore/Filters/Algorithms/FindArrayUniqueValues.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
OutputActions FindArrayUniqueValuesFilter::createCompatibleArrays(const DataStructure& data, const Arguments& args, usize numBins, std::vector<usize> tupleDims) const
{
  auto inputArrayPath = args.value<DataPath>(k_SelectedArrayPath_Key);
  auto* inputArray = data.getDataAs<IDataArray>(inputArrayPath);
  auto destinationAttributeMatrixValue = args.value<DataPath>(k_DestinationAttributeMatrix_Key);
  DataType dataType = inputArray->getDataType();

  OutputActions actions;

  auto amAction = std::make_unique<CreateAttributeMatrixAction>(destinationAttributeMatrixValue, tupleDims);
  actions.actions.push_back(std::move(amAction));

  return std::move(actions);
}

//------------------------------------------------------------------------------
std::string FindArrayUniqueValuesFilter::name() const
{
  return FilterTraits<FindArrayUniqueValuesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindArrayUniqueValuesFilter::className() const
{
  return FilterTraits<FindArrayUniqueValuesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindArrayUniqueValuesFilter::uuid() const
{
  return FilterTraits<FindArrayUniqueValuesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindArrayUniqueValuesFilter::humanName() const
{
  return "Find Attribute Array Unique Values";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindArrayUniqueValuesFilter::defaultTags() const
{
  return {"ComplexCore", "Statistics"};
}

//------------------------------------------------------------------------------
Parameters FindArrayUniqueValuesFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Required Input Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array to Compute Statistics", "Input Attribute Array for which to compute statistics", DataPath{},
                                                          complex::GetAllDataTypes(), ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask", "Whether to use a boolean mask array to ignore certain points flagged as false from the statistics", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "The path to the data array that specifies if the point is to be counted in the statistics", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Attribute Matrix"});
  params.insert(
      std::make_unique<DataGroupCreationParameter>(k_DestinationAttributeMatrix_Key, "Destination Attribute Matrix", "Attribute Matrix in which to store the computed unique values", DataPath{}));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindArrayUniqueValuesFilter::clone() const
{
  return std::make_unique<FindArrayUniqueValuesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindArrayUniqueValuesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pDestinationAttributeMatrixValue = filterArgs.value<DataPath>(k_DestinationAttributeMatrix_Key);

  PreflightResult preflightResult;
  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<usize> tDims = {1};
  std::vector<usize> compDims = {1};

  const auto* inputArrayPtr = dataStructure.getDataAs<IDataArray>(pSelectedArrayPathValue);

  if(inputArrayPtr == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-57202, fmt::format("Could not find selected input array at path '{}' ", pSelectedArrayPathValue.toString())), {}};
  }

  if(inputArrayPtr->getNumberOfComponents() != 1)
  {
    return {MakeErrorResult<OutputActions>(-57203, fmt::format("Input array must be a scalar array")), {}};
  }

  if(pUseMaskValue)
  {
    const auto* maskPtr = dataStructure.getDataAs<IDataArray>(pMaskArrayPathValue);
    if(maskPtr == nullptr)
    {
      return {MakeErrorResult<OutputActions>(-57207, fmt::format("Could not find mask array at path '{}' ", pMaskArrayPathValue.toString())), {}};
    }
    if(maskPtr->getDataType() != DataType::boolean && maskPtr->getDataType() != DataType::uint8)
    {
      return {MakeErrorResult<OutputActions>(-57207, fmt::format("Mask array must be of type Boolean or UInt8")), {}};
    }
  }

  auto createMatrix = std::make_unique<CreateAttributeMatrixAction>(pDestinationAttributeMatrixValue, tDims);
  resultOutputActions.value().actions.push_back(std::move(createMatrix));

  DataPath path = pDestinationAttributeMatrixValue.createChildPath("Unique Values");
  auto createArray = std::make_unique<CreateArrayAction>(DataType::int32, tDims, compDims, path);
  resultOutputActions.value().actions.push_back(std::move(createArray));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindArrayUniqueValuesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{

  FindArrayUniqueValuesInputValues inputValues;

  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.SelectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.DestinationAttributeMatrix = filterArgs.value<DataPath>(k_DestinationAttributeMatrix_Key);

  return FindArrayUniqueValues(dataStructure, messageHandler, shouldCancel, &inputValues)();

  // findArrayType(dataStructure, selectedArrayPath, maskArrayPath, destinationAttributeMatrix, useMask, "Unique Values", messageHandler);
}
} // namespace complex
