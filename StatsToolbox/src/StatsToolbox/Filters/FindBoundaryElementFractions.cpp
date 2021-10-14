#include "FindBoundaryElementFractions.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
std::string FindBoundaryElementFractions::name() const
{
  return FilterTraits<FindBoundaryElementFractions>::name.str();
}

std::string FindBoundaryElementFractions::className() const
{
  return FilterTraits<FindBoundaryElementFractions>::className;
}

Uuid FindBoundaryElementFractions::uuid() const
{
  return FilterTraits<FindBoundaryElementFractions>::uuid;
}

std::string FindBoundaryElementFractions::humanName() const
{
  return "Find Feature Boundary Element Fractions";
}

Parameters FindBoundaryElementFractions::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_BoundaryCellsArrayPath_Key, "Surface Elements", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_BoundaryCellFractionsArrayPath_Key, "Surface Element Fractions", "", DataPath{}));

  return params;
}

IFilter::UniquePointer FindBoundaryElementFractions::clone() const
{
  return std::make_unique<FindBoundaryElementFractions>();
}

Result<OutputActions> FindBoundaryElementFractions::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pBoundaryCellsArrayPathValue = filterArgs.value<DataPath>(k_BoundaryCellsArrayPath_Key);
  auto pBoundaryCellFractionsArrayPathValue = filterArgs.value<DataPath>(k_BoundaryCellFractionsArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindBoundaryElementFractionsAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> FindBoundaryElementFractions::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pBoundaryCellsArrayPathValue = filterArgs.value<DataPath>(k_BoundaryCellsArrayPath_Key);
  auto pBoundaryCellFractionsArrayPathValue = filterArgs.value<DataPath>(k_BoundaryCellFractionsArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
