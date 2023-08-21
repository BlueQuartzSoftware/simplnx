#include "GenerateColorTableFilter.hpp"

#include "Algorithms/GenerateColorTable.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GenerateColorTableParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string GenerateColorTableFilter::name() const
{
  return FilterTraits<GenerateColorTableFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string GenerateColorTableFilter::className() const
{
  return FilterTraits<GenerateColorTableFilter>::className;
}

//------------------------------------------------------------------------------
Uuid GenerateColorTableFilter::uuid() const
{
  return FilterTraits<GenerateColorTableFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string GenerateColorTableFilter::humanName() const
{
  return "Generate Color Table";
}

//------------------------------------------------------------------------------
std::vector<std::string> GenerateColorTableFilter::defaultTags() const
{
  return {className(), "Core", "Image"};
}

//------------------------------------------------------------------------------
Parameters GenerateColorTableFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator({"Input Parameters"});
  params.insert(std::make_unique<GenerateColorTableParameter>(k_SelectedPreset_Key, "Select Preset...", "Select a preset color scheme to apply to the created array", nlohmann::json{}));
  params.insertSeparator({"Required Data Objects"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedDataArrayPath_Key, "Data Array",
                                                          "The complete path to the data array from which to create the rgb array by applying the selected preset color scheme", DataPath{},
                                                          complex::GetAllDataTypes(), ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator({"Created Data Objects"});
  params.insert(std::make_unique<DataObjectNameParameter>(
      k_RgbArrayPath_Key, "Output RGB Array", "The rgb array created by normalizing each element of the input array and converting to a color based on the selected preset color scheme", ""));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer GenerateColorTableFilter::clone() const
{
  return std::make_unique<GenerateColorTableFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult GenerateColorTableFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel) const
{
  auto pSelectedPresetValue = filterArgs.value<nlohmann::json>(k_SelectedPreset_Key);
  auto pSelectedDataArrayPathValue = filterArgs.value<DataPath>(k_SelectedDataArrayPath_Key);
  auto pRgbArrayPathValue = pSelectedDataArrayPathValue.getParent().createChildPath(filterArgs.value<std::string>(k_RgbArrayPath_Key));

  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const auto& dataArray = dataStructure.getDataRefAs<IDataArray>(pSelectedDataArrayPathValue);
  auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint8, dataArray.getTupleShape(), std::vector<usize>{3}, pRgbArrayPathValue);
  resultOutputActions.value().appendAction(std::move(createArrayAction));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> GenerateColorTableFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel) const
{
  GenerateColorTableInputValues inputValues;

  inputValues.SelectedPreset = filterArgs.value<nlohmann::json>(k_SelectedPreset_Key);
  inputValues.SelectedDataArrayPath = filterArgs.value<DataPath>(k_SelectedDataArrayPath_Key);
  inputValues.RgbArrayPath = inputValues.SelectedDataArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_RgbArrayPath_Key));

  return GenerateColorTable(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
