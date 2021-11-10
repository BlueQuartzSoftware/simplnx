#include "ComputeMomentInvariants2D.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ComputeMomentInvariants2D::name() const
{
  return FilterTraits<ComputeMomentInvariants2D>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeMomentInvariants2D::className() const
{
  return FilterTraits<ComputeMomentInvariants2D>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeMomentInvariants2D::uuid() const
{
  return FilterTraits<ComputeMomentInvariants2D>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeMomentInvariants2D::humanName() const
{
  return "Compute MomentInvariants (2D)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeMomentInvariants2D::defaultTags() const
{
  return {"#Unsupported", "#Statistics"};
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeMomentInvariants2D::clone() const
{
  return std::make_unique<ComputeMomentInvariants2D>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeMomentInvariants2D::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeatureRectArrayPathValue = filterArgs.value<DataPath>(k_FeatureRectArrayPath_Key);
  auto pNormalizeMomentInvariantsValue = filterArgs.value<bool>(k_NormalizeMomentInvariants_Key);
  auto pOmega1ArrayPathValue = filterArgs.value<DataPath>(k_Omega1ArrayPath_Key);
  auto pOmega2ArrayPathValue = filterArgs.value<DataPath>(k_Omega2ArrayPath_Key);
  auto pSaveCentralMomentsValue = filterArgs.value<bool>(k_SaveCentralMoments_Key);
  auto pCentralMomentsArrayPathValue = filterArgs.value<DataPath>(k_CentralMomentsArrayPath_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

#if 0
  // Define the OutputActions Object that will hold the actions that would take
  // place if the filter were to execute. This is mainly what would happen to the
  // DataStructure during this filter, i.e., what modificationst to the DataStructure
  // would take place.
  OutputActions actions;
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ComputeMomentInvariants2DAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ComputeMomentInvariants2D::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
