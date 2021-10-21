#include "GenerateColorTable.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GenerateColorTableFilterParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string GenerateColorTable::name() const
{
  return FilterTraits<GenerateColorTable>::name.str();
}

//------------------------------------------------------------------------------
std::string GenerateColorTable::className() const
{
  return FilterTraits<GenerateColorTable>::className;
}

//------------------------------------------------------------------------------
Uuid GenerateColorTable::uuid() const
{
  return FilterTraits<GenerateColorTable>::uuid;
}

//------------------------------------------------------------------------------
std::string GenerateColorTable::humanName() const
{
  return "Generate Color Table";
}

//------------------------------------------------------------------------------
std::vector<std::string> GenerateColorTable::defaultTags() const
{
  return {"#Core", "#Image"};
}

//------------------------------------------------------------------------------
Parameters GenerateColorTable::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<GenerateColorTableFilterParameter>(k_SelectedPresetName_Key, "Select Preset...", "", {}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedDataArrayPath_Key, "Data Array", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_RgbArrayName_Key, "RGB Array Name", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer GenerateColorTable::clone() const
{
  return std::make_unique<GenerateColorTable>();
}

//------------------------------------------------------------------------------
Result<OutputActions> GenerateColorTable::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSelectedPresetNameValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SelectedPresetName_Key);
  auto pSelectedDataArrayPathValue = filterArgs.value<DataPath>(k_SelectedDataArrayPath_Key);
  auto pRgbArrayNameValue = filterArgs.value<DataPath>(k_RgbArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<GenerateColorTableAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> GenerateColorTable::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedPresetNameValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SelectedPresetName_Key);
  auto pSelectedDataArrayPathValue = filterArgs.value<DataPath>(k_SelectedDataArrayPath_Key);
  auto pRgbArrayNameValue = filterArgs.value<DataPath>(k_RgbArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
