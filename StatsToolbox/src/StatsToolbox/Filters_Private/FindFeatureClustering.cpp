#include "FindFeatureClustering.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindFeatureClustering::name() const
{
  return FilterTraits<FindFeatureClustering>::name.str();
}

//------------------------------------------------------------------------------
std::string FindFeatureClustering::className() const
{
  return FilterTraits<FindFeatureClustering>::className;
}

//------------------------------------------------------------------------------
Uuid FindFeatureClustering::uuid() const
{
  return FilterTraits<FindFeatureClustering>::uuid;
}

//------------------------------------------------------------------------------
std::string FindFeatureClustering::humanName() const
{
  return "Find Feature Clustering";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindFeatureClustering::defaultTags() const
{
  return {"#Statistics", "#Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindFeatureClustering::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_NumberOfBins_Key, "Number of Bins for RDF", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_PhaseNumber_Key, "Phase Index", "", 1234356));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RemoveBiasedFeatures_Key, "Remove Biased Features", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_EquivalentDiametersArrayPath_Key, "Equivalent Diameters", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each cell belongs", DataPath({"FeatureData", "Phases"}), ArraySelectionParameter::AllowedTypes{DataType::int32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CentroidsArrayPath_Key, "Centroids", "", DataPath({"FeatureData", "Centroids"}), ArraySelectionParameter::AllowedTypes{DataType::float32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_BiasedFeaturesArrayPath_Key, "Biased Features", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellEnsembleAttributeMatrixName_Key, "Cell Ensemble Attribute Matrix", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_ClusteringListArrayName_Key, "Clustering List", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewEnsembleArrayArrayName_Key, "Radial Distribution Function", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_MaxMinArrayName_Key, "Max and Min Separation Distances", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_RemoveBiasedFeatures_Key, k_BiasedFeaturesArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindFeatureClustering::clone() const
{
  return std::make_unique<FindFeatureClustering>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindFeatureClustering::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pNumberOfBinsValue = filterArgs.value<int32>(k_NumberOfBins_Key);
  auto pPhaseNumberValue = filterArgs.value<int32>(k_PhaseNumber_Key);
  auto pRemoveBiasedFeaturesValue = filterArgs.value<bool>(k_RemoveBiasedFeatures_Key);
  auto pEquivalentDiametersArrayPathValue = filterArgs.value<DataPath>(k_EquivalentDiametersArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCentroidsArrayPathValue = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  auto pBiasedFeaturesArrayPathValue = filterArgs.value<DataPath>(k_BiasedFeaturesArrayPath_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);
  auto pClusteringListArrayNameValue = filterArgs.value<DataPath>(k_ClusteringListArrayName_Key);
  auto pNewEnsembleArrayArrayNameValue = filterArgs.value<DataPath>(k_NewEnsembleArrayArrayName_Key);
  auto pMaxMinArrayNameValue = filterArgs.value<DataPath>(k_MaxMinArrayName_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // None found in this filter based on the filter parameters

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindFeatureClustering::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pNumberOfBinsValue = filterArgs.value<int32>(k_NumberOfBins_Key);
  auto pPhaseNumberValue = filterArgs.value<int32>(k_PhaseNumber_Key);
  auto pRemoveBiasedFeaturesValue = filterArgs.value<bool>(k_RemoveBiasedFeatures_Key);
  auto pEquivalentDiametersArrayPathValue = filterArgs.value<DataPath>(k_EquivalentDiametersArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCentroidsArrayPathValue = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  auto pBiasedFeaturesArrayPathValue = filterArgs.value<DataPath>(k_BiasedFeaturesArrayPath_Key);
  auto pCellEnsembleAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);
  auto pClusteringListArrayNameValue = filterArgs.value<DataPath>(k_ClusteringListArrayName_Key);
  auto pNewEnsembleArrayArrayNameValue = filterArgs.value<DataPath>(k_NewEnsembleArrayArrayName_Key);
  auto pMaxMinArrayNameValue = filterArgs.value<DataPath>(k_MaxMinArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
