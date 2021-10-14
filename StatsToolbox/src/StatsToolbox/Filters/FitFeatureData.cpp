#include "FitFeatureData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"

using namespace complex;

namespace complex
{
std::string FitFeatureData::name() const
{
  return FilterTraits<FitFeatureData>::name.str();
}

Uuid FitFeatureData::uuid() const
{
  return FilterTraits<FitFeatureData>::uuid;
}

std::string FitFeatureData::humanName() const
{
  return "Fit Distribution to Feature Data";
}

Parameters FitFeatureData::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ChoicesParameter>(k_DistributionType_Key, "Distribution Type", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RemoveBiasedFeatures_Key, "Remove Biased Features", "", false));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedFeatureArrayPath_Key, "Feature Array to Fit", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_BiasedFeaturesArrayPath_Key, "Biased Features", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Ensemble Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewEnsembleArrayArray_Key, "Fit Parameters", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_RemoveBiasedFeatures_Key, k_BiasedFeaturesArrayPath_Key, true);

  return params;
}

IFilter::UniquePointer FitFeatureData::clone() const
{
  return std::make_unique<FitFeatureData>();
}

Result<OutputActions> FitFeatureData::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pDistributionTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_DistributionType_Key);
  auto pRemoveBiasedFeaturesValue = filterArgs.value<bool>(k_RemoveBiasedFeatures_Key);
  auto pSelectedFeatureArrayPathValue = filterArgs.value<DataPath>(k_SelectedFeatureArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pBiasedFeaturesArrayPathValue = filterArgs.value<DataPath>(k_BiasedFeaturesArrayPath_Key);
  auto pNewEnsembleArrayArrayValue = filterArgs.value<DataPath>(k_NewEnsembleArrayArray_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FitFeatureDataAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> FitFeatureData::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pDistributionTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_DistributionType_Key);
  auto pRemoveBiasedFeaturesValue = filterArgs.value<bool>(k_RemoveBiasedFeatures_Key);
  auto pSelectedFeatureArrayPathValue = filterArgs.value<DataPath>(k_SelectedFeatureArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pBiasedFeaturesArrayPathValue = filterArgs.value<DataPath>(k_BiasedFeaturesArrayPath_Key);
  auto pNewEnsembleArrayArrayValue = filterArgs.value<DataPath>(k_NewEnsembleArrayArray_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
