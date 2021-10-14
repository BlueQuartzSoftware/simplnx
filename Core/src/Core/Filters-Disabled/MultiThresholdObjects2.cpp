#include "MultiThresholdObjects2.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ComparisonSelectionAdvancedFilterParameter.hpp"

using namespace complex;

namespace complex
{
std::string MultiThresholdObjects2::name() const
{
  return FilterTraits<MultiThresholdObjects2>::name.str();
}

std::string MultiThresholdObjects2::className() const
{
  return FilterTraits<MultiThresholdObjects2>::className;
}

Uuid MultiThresholdObjects2::uuid() const
{
  return FilterTraits<MultiThresholdObjects2>::uuid;
}

std::string MultiThresholdObjects2::humanName() const
{
  return "Threshold Objects (Advanced)";
}

Parameters MultiThresholdObjects2::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ComparisonSelectionAdvancedFilterParameter>(k_SelectedThresholds_Key, "Select Arrays to Threshold", "", {}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DestinationArrayName_Key, "Output Attribute Array", "", DataPath{}));

  return params;
}

IFilter::UniquePointer MultiThresholdObjects2::clone() const
{
  return std::make_unique<MultiThresholdObjects2>();
}

Result<OutputActions> MultiThresholdObjects2::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSelectedThresholdsValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SelectedThresholds_Key);
  auto pDestinationArrayNameValue = filterArgs.value<DataPath>(k_DestinationArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<MultiThresholdObjects2Action>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> MultiThresholdObjects2::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedThresholdsValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SelectedThresholds_Key);
  auto pDestinationArrayNameValue = filterArgs.value<DataPath>(k_DestinationArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
