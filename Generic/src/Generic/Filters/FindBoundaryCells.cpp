#include "FindBoundaryCells.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindBoundaryCells::name() const
{
  return FilterTraits<FindBoundaryCells>::name.str();
}

//------------------------------------------------------------------------------
std::string FindBoundaryCells::className() const
{
  return FilterTraits<FindBoundaryCells>::className;
}

//------------------------------------------------------------------------------
Uuid FindBoundaryCells::uuid() const
{
  return FilterTraits<FindBoundaryCells>::uuid;
}

//------------------------------------------------------------------------------
std::string FindBoundaryCells::humanName() const
{
  return "Find Boundary Cells (Image)";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindBoundaryCells::defaultTags() const
{
  return {"#Generic", "#Spatial"};
}

//------------------------------------------------------------------------------
Parameters FindBoundaryCells::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_IgnoreFeatureZero_Key, "Ignore Feature 0", "", false));
  params.insert(std::make_unique<BoolParameter>(k_IncludeVolumeBoundary_Key, "Include Volume Boundary", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_BoundaryCellsArrayName_Key, "Boundary Cells", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindBoundaryCells::clone() const
{
  return std::make_unique<FindBoundaryCells>();
}

//------------------------------------------------------------------------------
Result<OutputActions> FindBoundaryCells::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pIgnoreFeatureZeroValue = filterArgs.value<bool>(k_IgnoreFeatureZero_Key);
  auto pIncludeVolumeBoundaryValue = filterArgs.value<bool>(k_IncludeVolumeBoundary_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pBoundaryCellsArrayNameValue = filterArgs.value<DataPath>(k_BoundaryCellsArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindBoundaryCellsAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindBoundaryCells::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pIgnoreFeatureZeroValue = filterArgs.value<bool>(k_IgnoreFeatureZero_Key);
  auto pIncludeVolumeBoundaryValue = filterArgs.value<bool>(k_IncludeVolumeBoundary_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pBoundaryCellsArrayNameValue = filterArgs.value<DataPath>(k_BoundaryCellsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
