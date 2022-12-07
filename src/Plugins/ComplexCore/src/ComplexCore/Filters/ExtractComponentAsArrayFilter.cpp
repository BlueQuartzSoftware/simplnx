#include "ExtractComponentAsArrayFilter.hpp"

#include "ComplexCore/Filters/Algorithms/ExtractComponentAsArray.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ExtractComponentAsArrayFilter::name() const
{
  return FilterTraits<ExtractComponentAsArrayFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ExtractComponentAsArrayFilter::className() const
{
  return FilterTraits<ExtractComponentAsArrayFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ExtractComponentAsArrayFilter::uuid() const
{
  return FilterTraits<ExtractComponentAsArrayFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ExtractComponentAsArrayFilter::humanName() const
{
  return "Extract Component as Attribute Array";
}

//------------------------------------------------------------------------------
std::vector<std::string> ExtractComponentAsArrayFilter::defaultTags() const
{
  return {"#Core", "#Memory Management"};
}

//------------------------------------------------------------------------------
Parameters ExtractComponentAsArrayFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_MoveToNewArray_Key, "Move Extracted Components to New Array", "If false the extracted components will be deleted", false));
  params.insert(std::make_unique<Int32Parameter>(k_CompNumber_Key, "Component Number to Extract", "The number of components to extract from the array", 0));

  params.insertSeparator(Parameters::Separator{"Required Input DataArray"});
  params.insert(
      std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Multicomponent Attribute Array", "The array to extract componenets from", DataPath{}, complex::GetAllNumericTypes()));

  params.insertSeparator(Parameters::Separator{"Created DataArray"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewArrayPath_Key, "Scalar Attribute Array", "The DataArray to store the extracted components", DataPath({"Extracted Components"})));

  params.linkParameters(k_MoveToNewArray_Key, k_NewArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ExtractComponentAsArrayFilter::clone() const
{
  return std::make_unique<ExtractComponentAsArrayFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ExtractComponentAsArrayFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                      const std::atomic_bool& shouldCancel) const
{
  auto pMoveToNewArrayValue = filterArgs.value<bool>(k_MoveToNewArray_Key);
  auto pCompNumberValue = filterArgs.value<int32>(k_CompNumber_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pNewArrayPathValue = filterArgs.value<DataPath>(k_NewArrayPath_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const auto& selectedArray = dataStructure.getDataRefAs<IDataArray>(pSelectedArrayPathValue);

  if(selectedArray.getNumberOfComponents() < pCompNumberValue)
  {
    return {MakeErrorResult<OutputActions>(-45630, fmt::format("The number to remove [{}] is larger than the array component count: {} ", pCompNumberValue, selectedArray.getNumberOfComponents()))};
  }

  if(pMoveToNewArrayValue)
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(selectedArray.getDataType(), std::vector<usize>{1}, std::vector<usize>{static_cast<usize>(abs(pCompNumberValue))}, pNewArrayPathValue);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ExtractComponentAsArrayFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                    const std::atomic_bool& shouldCancel) const
{
  ExtractComponentAsArrayInputValues inputValues;

  inputValues.MoveToNewArray = filterArgs.value<bool>(k_MoveToNewArray_Key);
  inputValues.CompNumber = filterArgs.value<int32>(k_CompNumber_Key);
  inputValues.SelectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.NewArrayPath = filterArgs.value<DataPath>(k_NewArrayPath_Key);

  return ExtractComponentAsArray(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
