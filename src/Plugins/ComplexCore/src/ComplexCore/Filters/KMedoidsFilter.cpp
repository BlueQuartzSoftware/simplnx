#include "KMedoidsFilter.hpp"

#include "ComplexCore/Filters/Algorithms/KMedoids.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string KMedoidsFilter::name() const
{
  return FilterTraits<KMedoidsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string KMedoidsFilter::className() const
{
  return FilterTraits<KMedoidsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid KMedoidsFilter::uuid() const
{
  return FilterTraits<KMedoidsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string KMedoidsFilter::humanName() const
{
  return "K Medoids";
}

//------------------------------------------------------------------------------
std::vector<std::string> KMedoidsFilter::defaultTags() const
{
  return {"DREAM3D Review", "Clustering"};
}

//------------------------------------------------------------------------------
Parameters KMedoidsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Int32Parameter>(k_InitClusters_Key, "Number of Clusters", "", 0 #error Check default values));
  params.insert(
      std::make_unique<ChoicesParameter>(k_DistanceMetric_Key, "Distance Metric", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"} /* Change this to the proper choices */));

  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask", "", false #error Check default values));
  params.insert(
      std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "", DataPath{}, complex::GetAllDataTypes() /* This will allow ANY data type. Adjust as necessary for your filter*/));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array to Cluster", "", DataPath{},
                                                          complex::GetAllDataTypes() /* This will allow ANY data type. Adjust as necessary for your filter*/));

  params.insertSeparator(Parameters::Separator{"Created Data Objects"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureIdsArrayName_Key, "Cluster Ids", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureAttributeMatrixName_Key, "Cluster Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_MedoidsArrayName_Key, "Cluster Medoids", "", DataPath{}));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer KMedoidsFilter::clone() const
{
  return std::make_unique<KMedoidsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult KMedoidsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel) const
{
  auto pInitClustersValue = filterArgs.value<int32>(k_InitClusters_Key);
  auto pDistanceMetricValue = filterArgs.value<ChoicesParameter::ValueType>(k_DistanceMetric_Key);
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);
  auto pFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_FeatureAttributeMatrixName_Key);
  auto pMedoidsArrayNameValue = filterArgs.value<DataPath>(k_MedoidsArrayName_Key);

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
Result<> KMedoidsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                     const std::atomic_bool& shouldCancel) const
{
  KMedoidsInputValues inputValues;

  inputValues.InitClusters = filterArgs.value<int32>(k_InitClusters_Key);
  inputValues.DistanceMetric = filterArgs.value<ChoicesParameter::ValueType>(k_DistanceMetric_Key);
  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.SelectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.FeatureIdsArrayName = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);
  inputValues.FeatureAttributeMatrixName = filterArgs.value<DataPath>(k_FeatureAttributeMatrixName_Key);
  inputValues.MedoidsArrayName = filterArgs.value<DataPath>(k_MedoidsArrayName_Key);

  return KMedoids(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
