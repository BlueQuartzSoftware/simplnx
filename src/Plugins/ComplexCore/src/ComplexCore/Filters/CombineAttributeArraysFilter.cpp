#include "CombineAttributeArraysFilter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/DeleteDataAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

#include "ComplexCore/Filters/Algorithms/CombineAttributeArrays.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string CombineAttributeArraysFilter::name() const
{
  return FilterTraits<CombineAttributeArraysFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string CombineAttributeArraysFilter::className() const
{
  return FilterTraits<CombineAttributeArraysFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CombineAttributeArraysFilter::uuid() const
{
  return FilterTraits<CombineAttributeArraysFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CombineAttributeArraysFilter::humanName() const
{
  return "Combine Attribute Arrays";
}

//------------------------------------------------------------------------------
std::vector<std::string> CombineAttributeArraysFilter::defaultTags() const
{
  return {"#Core",
          "#Memory Management"
          "#Combine",
          "#Arrays"};
}

//------------------------------------------------------------------------------
Parameters CombineAttributeArraysFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});

  params.insert(std::make_unique<BoolParameter>(k_NormalizeData_Key, "Normalize Data", "Whether to normalize the combine data on the interval [0, 1]", false));
  params.insert(std::make_unique<BoolParameter>(k_MoveValues_Key, "Move Data", "Whether to remove the original arrays after combining the data", false));

  params.insertSeparator(Parameters::Separator{"Required Input Data Objects"});
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Attribute Arrays to Combine", "The complete path to each of the Attribute Arrays to combine",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray},
                                                               MultiArraySelectionParameter::AllowedDataTypes{}));

  params.insertSeparator(Parameters::Separator{"Created Output Data Objects"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_StackedDataArrayName_Key, "Created Data Array", "This is the DataPath to the created output array of the combined attribute arrays.",
                                                         DataPath({"Combined DataArray"})));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CombineAttributeArraysFilter::clone() const
{
  return std::make_unique<CombineAttributeArraysFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CombineAttributeArraysFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                     const std::atomic_bool& shouldCancel) const
{
  // auto normalizeDataValue = filterArgs.value<bool>(k_NormalizeData_Key);
  auto moveValuesValue = filterArgs.value<bool>(k_MoveValues_Key);
  auto selectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto stackedDataArrayPath = filterArgs.value<ArrayCreationParameter::ValueType>(k_StackedDataArrayName_Key);

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  if(selectedDataArrayPathsValue.empty())
  {
    return {MakeErrorResult<OutputActions>(-66005, "Please select arrays to combine")};
  }

  bool result = complex::CheckArraysAreSameType(dataStructure, selectedDataArrayPathsValue);
  if(!result)
  {
    return {MakeErrorResult<OutputActions>(-66006, "All selected arrays must be of the same type")};
  }
  result = complex::CheckArraysHaveSameTupleCount(dataStructure, selectedDataArrayPathsValue);
  if(!result)
  {
    return {MakeErrorResult<OutputActions>(-66007, "All selected arrays must have the same number of tuples")};
  }

  // Figure out the total number of components
  size_t numComps = 0;
  for(const auto& dataPath : selectedDataArrayPathsValue)
  {
    const auto* dataArray = dataStructure.getDataAs<IDataArray>(dataPath);
    numComps += dataArray->getNumberOfComponents();
  }

  // Create the output array
  {
    const auto* dataArray = dataStructure.getDataAs<IDataArray>(selectedDataArrayPathsValue[0]);
    auto tupleShape = dataArray->getTupleShape();
    auto action = std::make_unique<CreateArrayAction>(dataArray->getDataType(), tupleShape, std::vector<usize>{numComps}, stackedDataArrayPath);
    resultOutputActions.value().actions.push_back(std::move(action));
  }

  // If we are MOVING the data arrays, then we need to delete the data arrays at
  // the end of the execute method.
  if(moveValuesValue)
  {
    for(const auto& dataPath : selectedDataArrayPathsValue)
    {
      auto action = std::make_unique<DeleteDataAction>(dataPath, DeleteDataAction::DeleteType::JustObject);
      resultOutputActions.value().deferredActions.push_back(std::move(action));
    }
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CombineAttributeArraysFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel) const
{
  CombineAttributeArraysInputValues inputValues;

  inputValues.NormalizeData = filterArgs.value<bool>(k_NormalizeData_Key);
  inputValues.SelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);

  inputValues.StackedDataArrayPath = filterArgs.value<ArrayCreationParameter::ValueType>(k_StackedDataArrayName_Key);

  return CombineAttributeArrays(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
