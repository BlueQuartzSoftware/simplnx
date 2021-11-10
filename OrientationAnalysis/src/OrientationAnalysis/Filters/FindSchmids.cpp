#include "FindSchmids.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindSchmids::name() const
{
  return FilterTraits<FindSchmids>::name.str();
}

//------------------------------------------------------------------------------
std::string FindSchmids::className() const
{
  return FilterTraits<FindSchmids>::className;
}

//------------------------------------------------------------------------------
Uuid FindSchmids::uuid() const
{
  return FilterTraits<FindSchmids>::uuid;
}

//------------------------------------------------------------------------------
std::string FindSchmids::humanName() const
{
  return "Find Schmid Factors";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindSchmids::defaultTags() const
{
  return {"#Statistics", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindSchmids::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<VectorFloat32Parameter>(k_LoadingDirection_Key, "Loading Direction", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_StoreAngleComponents_Key, "Store Angle Components of Schmid Factor", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_OverrideSystem_Key, "Override Default Slip System", "", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_SlipPlane_Key, "Slip Plane", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_SlipDirection_Key, "Slip Direction", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_SchmidsArrayName_Key, "Schmids", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SlipSystemsArrayName_Key, "Slip Systems", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_PolesArrayName_Key, "Poles", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_PhisArrayName_Key, "Phis", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_LambdasArrayName_Key, "Lambdas", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_StoreAngleComponents_Key, k_PhisArrayName_Key, true);
  params.linkParameters(k_StoreAngleComponents_Key, k_LambdasArrayName_Key, true);
  params.linkParameters(k_OverrideSystem_Key, k_SlipPlane_Key, true);
  params.linkParameters(k_OverrideSystem_Key, k_SlipDirection_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindSchmids::clone() const
{
  return std::make_unique<FindSchmids>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindSchmids::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pLoadingDirectionValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_LoadingDirection_Key);
  auto pStoreAngleComponentsValue = filterArgs.value<bool>(k_StoreAngleComponents_Key);
  auto pOverrideSystemValue = filterArgs.value<bool>(k_OverrideSystem_Key);
  auto pSlipPlaneValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_SlipPlane_Key);
  auto pSlipDirectionValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_SlipDirection_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pSchmidsArrayNameValue = filterArgs.value<DataPath>(k_SchmidsArrayName_Key);
  auto pSlipSystemsArrayNameValue = filterArgs.value<DataPath>(k_SlipSystemsArrayName_Key);
  auto pPolesArrayNameValue = filterArgs.value<DataPath>(k_PolesArrayName_Key);
  auto pPhisArrayNameValue = filterArgs.value<DataPath>(k_PhisArrayName_Key);
  auto pLambdasArrayNameValue = filterArgs.value<DataPath>(k_LambdasArrayName_Key);

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
  auto action = std::make_unique<FindSchmidsAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> FindSchmids::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pLoadingDirectionValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_LoadingDirection_Key);
  auto pStoreAngleComponentsValue = filterArgs.value<bool>(k_StoreAngleComponents_Key);
  auto pOverrideSystemValue = filterArgs.value<bool>(k_OverrideSystem_Key);
  auto pSlipPlaneValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_SlipPlane_Key);
  auto pSlipDirectionValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_SlipDirection_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pSchmidsArrayNameValue = filterArgs.value<DataPath>(k_SchmidsArrayName_Key);
  auto pSlipSystemsArrayNameValue = filterArgs.value<DataPath>(k_SlipSystemsArrayName_Key);
  auto pPolesArrayNameValue = filterArgs.value<DataPath>(k_PolesArrayName_Key);
  auto pPhisArrayNameValue = filterArgs.value<DataPath>(k_PhisArrayName_Key);
  auto pLambdasArrayNameValue = filterArgs.value<DataPath>(k_LambdasArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
