#include "ChangeAngleRepresentation.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ChangeAngleRepresentation::name() const
{
  return FilterTraits<ChangeAngleRepresentation>::name.str();
}

//------------------------------------------------------------------------------
std::string ChangeAngleRepresentation::className() const
{
  return FilterTraits<ChangeAngleRepresentation>::className;
}

//------------------------------------------------------------------------------
Uuid ChangeAngleRepresentation::uuid() const
{
  return FilterTraits<ChangeAngleRepresentation>::uuid;
}

//------------------------------------------------------------------------------
std::string ChangeAngleRepresentation::humanName() const
{
  return "Convert Angles to Degrees or Radians";
}

//------------------------------------------------------------------------------
std::vector<std::string> ChangeAngleRepresentation::defaultTags() const
{
  return {"#Processing", "#Conversion"};
}

//------------------------------------------------------------------------------
Parameters ChangeAngleRepresentation::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ChoicesParameter>(k_ConversionType_Key, "Conversion Type", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Angles", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ChangeAngleRepresentation::clone() const
{
  return std::make_unique<ChangeAngleRepresentation>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ChangeAngleRepresentation::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pConversionTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_ConversionType_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ChangeAngleRepresentationAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ChangeAngleRepresentation::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pConversionTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_ConversionType_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
