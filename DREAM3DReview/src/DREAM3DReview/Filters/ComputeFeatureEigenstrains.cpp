#include "ComputeFeatureEigenstrains.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ComputeFeatureEigenstrains::name() const
{
  return FilterTraits<ComputeFeatureEigenstrains>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeFeatureEigenstrains::className() const
{
  return FilterTraits<ComputeFeatureEigenstrains>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeFeatureEigenstrains::uuid() const
{
  return FilterTraits<ComputeFeatureEigenstrains>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeFeatureEigenstrains::humanName() const
{
  return "Compute Eigenstrains by Feature (Grain/Inclusion)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeFeatureEigenstrains::defaultTags() const
{
  return {"#DREAM3D Review", "#Registration"};
}

//------------------------------------------------------------------------------
Parameters ComputeFeatureEigenstrains::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float32Parameter>(k_PoissonRatio_Key, "Poisson's Ratio", "", 1.23345f));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseEllipsoidalGrains_Key, "Use Ellipsoidal Grains (versus spherical assumption)", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseCorrectionalMatrix_Key, "Use Correctional Matrix", "", false));
  params.insert(std::make_unique<Float32Parameter>(k_Beta11_Key, "Beta11", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_Beta22_Key, "Beta22", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_Beta33_Key, "Beta33", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_Beta23_Key, "Beta23", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_Beta13_Key, "Beta13", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_Beta12_Key, "Beta12", "", 1.23345f));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_AxisLengthsArrayPath_Key, "Axis Lengths", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AxisEulerAnglesArrayPath_Key, "Axis Euler Angles", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ElasticStrainsArrayPath_Key, "Elastic Strains (Voigt Notation)", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_EigenstrainsArrayName_Key, "Eigenstrains", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseEllipsoidalGrains_Key, k_AxisLengthsArrayPath_Key, true);
  params.linkParameters(k_UseEllipsoidalGrains_Key, k_AxisEulerAnglesArrayPath_Key, true);
  params.linkParameters(k_UseCorrectionalMatrix_Key, k_Beta11_Key, true);
  params.linkParameters(k_UseCorrectionalMatrix_Key, k_Beta22_Key, true);
  params.linkParameters(k_UseCorrectionalMatrix_Key, k_Beta33_Key, true);
  params.linkParameters(k_UseCorrectionalMatrix_Key, k_Beta23_Key, true);
  params.linkParameters(k_UseCorrectionalMatrix_Key, k_Beta13_Key, true);
  params.linkParameters(k_UseCorrectionalMatrix_Key, k_Beta12_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeFeatureEigenstrains::clone() const
{
  return std::make_unique<ComputeFeatureEigenstrains>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeFeatureEigenstrains::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pPoissonRatioValue = filterArgs.value<float32>(k_PoissonRatio_Key);
  auto pUseEllipsoidalGrainsValue = filterArgs.value<bool>(k_UseEllipsoidalGrains_Key);
  auto pUseCorrectionalMatrixValue = filterArgs.value<bool>(k_UseCorrectionalMatrix_Key);
  auto pBeta11Value = filterArgs.value<float32>(k_Beta11_Key);
  auto pBeta22Value = filterArgs.value<float32>(k_Beta22_Key);
  auto pBeta33Value = filterArgs.value<float32>(k_Beta33_Key);
  auto pBeta23Value = filterArgs.value<float32>(k_Beta23_Key);
  auto pBeta13Value = filterArgs.value<float32>(k_Beta13_Key);
  auto pBeta12Value = filterArgs.value<float32>(k_Beta12_Key);
  auto pAxisLengthsArrayPathValue = filterArgs.value<DataPath>(k_AxisLengthsArrayPath_Key);
  auto pAxisEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_AxisEulerAnglesArrayPath_Key);
  auto pElasticStrainsArrayPathValue = filterArgs.value<DataPath>(k_ElasticStrainsArrayPath_Key);
  auto pEigenstrainsArrayNameValue = filterArgs.value<DataPath>(k_EigenstrainsArrayName_Key);

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
  auto action = std::make_unique<ComputeFeatureEigenstrainsAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ComputeFeatureEigenstrains::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pPoissonRatioValue = filterArgs.value<float32>(k_PoissonRatio_Key);
  auto pUseEllipsoidalGrainsValue = filterArgs.value<bool>(k_UseEllipsoidalGrains_Key);
  auto pUseCorrectionalMatrixValue = filterArgs.value<bool>(k_UseCorrectionalMatrix_Key);
  auto pBeta11Value = filterArgs.value<float32>(k_Beta11_Key);
  auto pBeta22Value = filterArgs.value<float32>(k_Beta22_Key);
  auto pBeta33Value = filterArgs.value<float32>(k_Beta33_Key);
  auto pBeta23Value = filterArgs.value<float32>(k_Beta23_Key);
  auto pBeta13Value = filterArgs.value<float32>(k_Beta13_Key);
  auto pBeta12Value = filterArgs.value<float32>(k_Beta12_Key);
  auto pAxisLengthsArrayPathValue = filterArgs.value<DataPath>(k_AxisLengthsArrayPath_Key);
  auto pAxisEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_AxisEulerAnglesArrayPath_Key);
  auto pElasticStrainsArrayPathValue = filterArgs.value<DataPath>(k_ElasticStrainsArrayPath_Key);
  auto pEigenstrainsArrayNameValue = filterArgs.value<DataPath>(k_EigenstrainsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
