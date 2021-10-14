#include "FindRelativeMotionBetweenSlices.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
std::string FindRelativeMotionBetweenSlices::name() const
{
  return FilterTraits<FindRelativeMotionBetweenSlices>::name.str();
}

std::string FindRelativeMotionBetweenSlices::className() const
{
  return FilterTraits<FindRelativeMotionBetweenSlices>::className;
}

Uuid FindRelativeMotionBetweenSlices::uuid() const
{
  return FilterTraits<FindRelativeMotionBetweenSlices>::uuid;
}

std::string FindRelativeMotionBetweenSlices::humanName() const
{
  return "Find Relative Motion Between Slices";
}

Parameters FindRelativeMotionBetweenSlices::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ChoicesParameter>(k_Plane_Key, "Plane of Interest", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(std::make_unique<Int32Parameter>(k_PSize1_Key, "Patch Size 1", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_PSize2_Key, "Patch Size 2", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_SSize1_Key, "Search Distance 1", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_SSize2_Key, "Search Distance 2", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_SliceStep_Key, "Slice Step", "", 1234356));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array to Track Motion", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_MotionDirectionArrayName_Key, "Motion Direction", "", DataPath{}));

  return params;
}

IFilter::UniquePointer FindRelativeMotionBetweenSlices::clone() const
{
  return std::make_unique<FindRelativeMotionBetweenSlices>();
}

Result<OutputActions> FindRelativeMotionBetweenSlices::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pPlaneValue = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  auto pPSize1Value = filterArgs.value<int32>(k_PSize1_Key);
  auto pPSize2Value = filterArgs.value<int32>(k_PSize2_Key);
  auto pSSize1Value = filterArgs.value<int32>(k_SSize1_Key);
  auto pSSize2Value = filterArgs.value<int32>(k_SSize2_Key);
  auto pSliceStepValue = filterArgs.value<int32>(k_SliceStep_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pMotionDirectionArrayNameValue = filterArgs.value<DataPath>(k_MotionDirectionArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindRelativeMotionBetweenSlicesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> FindRelativeMotionBetweenSlices::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pPlaneValue = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  auto pPSize1Value = filterArgs.value<int32>(k_PSize1_Key);
  auto pPSize2Value = filterArgs.value<int32>(k_PSize2_Key);
  auto pSSize1Value = filterArgs.value<int32>(k_SSize1_Key);
  auto pSSize2Value = filterArgs.value<int32>(k_SSize2_Key);
  auto pSliceStepValue = filterArgs.value<int32>(k_SliceStep_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pMotionDirectionArrayNameValue = filterArgs.value<DataPath>(k_MotionDirectionArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
