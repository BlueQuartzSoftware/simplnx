#include "ExtractComponentAsArrayFilter.hpp"

#include "ComplexCore/Filters/Algorithms/ExtractComponentAsArray.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/DeleteDataAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Filter/Actions/RenameDataAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <fstream>

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
  return "Extract/Remove Components";
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
  params.insert(std::make_unique<Int32Parameter>(k_CompNumber_Key, "Component Index to Extract", "The index of the component in each tuple to be removed", 0));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_MoveComponentsToNewArray_Key, "Move Extracted Components to New Array", "If true the extracted components will be placed in a new array", false));
  params.insert(std::make_unique<BoolParameter>(k_RemoveComponentsFromArray_Key, "Remove Extracted Components from Old Array", "If true the extracted components will be deleted", false));

  params.insertSeparator(Parameters::Separator{"Required Input DataArray"});
  params.insert(
      std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Multicomponent Attribute Array", "The array to extract componenets from", DataPath{}, complex::GetAllNumericTypes()));

  params.insertSeparator(Parameters::Separator{"Created DataArray"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewArrayPath_Key, "Scalar Attribute Array", "The DataArray to store the extracted components", DataPath({"Extracted Component"})));

  params.linkParameters(k_MoveComponentsToNewArray_Key, k_NewArrayPath_Key, true);
  params.linkParameters(k_MoveComponentsToNewArray_Key, k_RemoveComponentsFromArray_Key, true);

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
  auto pMoveComponentsToNewArrayValue = filterArgs.value<bool>(k_MoveComponentsToNewArray_Key);
  auto pRemoveComponentsFromArrayValue = filterArgs.value<bool>(k_RemoveComponentsFromArray_Key);
  auto pCompNumberValue = filterArgs.value<int32>(k_CompNumber_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pNewArrayPathValue = filterArgs.value<DataPath>(k_NewArrayPath_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const auto& selectedArray = dataStructure.getDataRefAs<IDataArray>(pSelectedArrayPathValue);

  // Verify Components
  const usize selectedArrayComp = selectedArray.getNumberOfComponents();
  if(selectedArrayComp < 2)
  {
    return {MakeErrorResult<OutputActions>(-45630, fmt::format("The array component count must be more than 1, this component count is: {} ", selectedArrayComp))};
  }
  if(selectedArrayComp < pCompNumberValue)
  {
    return {MakeErrorResult<OutputActions>(
        -45631, fmt::format("The component index '{}' is larger than the total number of components. Valid values are between {} and {} inclusive. ", pCompNumberValue, 0, (selectedArrayComp - 1)))};
  }

  if(pMoveComponentsToNewArrayValue)
  {
    // Create new array to hold extracted components
    auto createNewComponentArrayAction = std::make_unique<CreateArrayAction>(selectedArray.getDataType(), selectedArray.getTupleShape(), std::vector<usize>{1}, pNewArrayPathValue);
    resultOutputActions.value().actions.push_back(std::move(createNewComponentArrayAction));
  }

  // Default to removal if user elects to not extract components because parameter is hidden if move extracted components is false
  if((!pMoveComponentsToNewArrayValue) || pRemoveComponentsFromArrayValue)
  {
    // Set up the array to replace original
    auto tempSelectedArrayName = pSelectedArrayPathValue.getTargetName() + "Temp";
    DataPath baseSelectedArrayPath = pSelectedArrayPathValue;

    // Rename original array to temp
    auto renameSelectedArrayAction = std::make_unique<RenameDataAction>(pSelectedArrayPathValue, tempSelectedArrayName);
    resultOutputActions.value().actions.push_back(std::move(renameSelectedArrayAction));

    // Create new array with old DataPath of new size
    auto createNewSelectedArrayAction =
        std::make_unique<CreateArrayAction>(selectedArray.getDataType(), selectedArray.getTupleShape(), std::vector<usize>{selectedArray.getNumberOfComponents() - 1}, baseSelectedArrayPath);
    resultOutputActions.value().actions.push_back(std::move(createNewSelectedArrayAction));

    // Remove the old array after execution
    auto removeTempArrayAction = std::make_unique<DeleteDataAction>(DataPath::FromString(pSelectedArrayPathValue.toString() + "Temp", '/').value());
    resultOutputActions.value().deferredActions.push_back(std::move(removeTempArrayAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ExtractComponentAsArrayFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                    const std::atomic_bool& shouldCancel) const
{
  ExtractComponentAsArrayInputValues inputValues;

  inputValues.MoveComponentsToNewArray = filterArgs.value<bool>(k_MoveComponentsToNewArray_Key);
  inputValues.RemoveComponentsFromArray = filterArgs.value<bool>(k_RemoveComponentsFromArray_Key);
  inputValues.CompNumber = filterArgs.value<int32>(k_CompNumber_Key);
  // This is the original array if remove components is true OR  move components is false
  inputValues.TempArrayPath = DataPath::FromString(filterArgs.value<DataPath>(k_SelectedArrayPath_Key).toString() + "Temp", '/').value();
  // This is the array on the original array path whether its removed or not
  inputValues.BaseArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  // If move components to new array is true this is a valid path
  inputValues.NewArrayPath = filterArgs.value<DataPath>(k_NewArrayPath_Key);

  return ExtractComponentAsArray(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
