#include "ComputeMomentInvariants2D.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"

using namespace complex;

namespace complex
{
std::string ComputeMomentInvariants2D::name() const
{
  return FilterTraits<ComputeMomentInvariants2D>::name.str();
}

std::string ComputeMomentInvariants2D::className() const
{
  return FilterTraits<ComputeMomentInvariants2D>::className;
}

Uuid ComputeMomentInvariants2D::uuid() const
{
  return FilterTraits<ComputeMomentInvariants2D>::uuid;
}

std::string ComputeMomentInvariants2D::humanName() const
{
  return "Compute MomentInvariants (2D)";
}

Parameters ComputeMomentInvariants2D::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureRectArrayPath_Key, "Feature Rect", "", DataPath{}));
  params.insert(std::make_unique<BoolParameter>(k_NormalizeMomentInvariants_Key, "Normalize Moment Invariants", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_Omega1ArrayPath_Key, "Omega 1", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_Omega2ArrayPath_Key, "Omega 2", "", DataPath{}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SaveCentralMoments_Key, "Save Central Moments", "", false));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CentralMomentsArrayPath_Key, "Central Moments", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_SaveCentralMoments_Key, k_CentralMomentsArrayPath_Key, true);

  return params;
}

IFilter::UniquePointer ComputeMomentInvariants2D::clone() const
{
  return std::make_unique<ComputeMomentInvariants2D>();
}

Result<OutputActions> ComputeMomentInvariants2D::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeatureRectArrayPathValue = filterArgs.value<DataPath>(k_FeatureRectArrayPath_Key);
  auto pNormalizeMomentInvariantsValue = filterArgs.value<bool>(k_NormalizeMomentInvariants_Key);
  auto pOmega1ArrayPathValue = filterArgs.value<DataPath>(k_Omega1ArrayPath_Key);
  auto pOmega2ArrayPathValue = filterArgs.value<DataPath>(k_Omega2ArrayPath_Key);
  auto pSaveCentralMomentsValue = filterArgs.value<bool>(k_SaveCentralMoments_Key);
  auto pCentralMomentsArrayPathValue = filterArgs.value<DataPath>(k_CentralMomentsArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ComputeMomentInvariants2DAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ComputeMomentInvariants2D::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeatureRectArrayPathValue = filterArgs.value<DataPath>(k_FeatureRectArrayPath_Key);
  auto pNormalizeMomentInvariantsValue = filterArgs.value<bool>(k_NormalizeMomentInvariants_Key);
  auto pOmega1ArrayPathValue = filterArgs.value<DataPath>(k_Omega1ArrayPath_Key);
  auto pOmega2ArrayPathValue = filterArgs.value<DataPath>(k_Omega2ArrayPath_Key);
  auto pSaveCentralMomentsValue = filterArgs.value<bool>(k_SaveCentralMoments_Key);
  auto pCentralMomentsArrayPathValue = filterArgs.value<DataPath>(k_CentralMomentsArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
