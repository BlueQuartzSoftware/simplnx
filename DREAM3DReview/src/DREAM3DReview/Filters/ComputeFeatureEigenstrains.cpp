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
  return {"#DREAM3DReview", "#Registration"};
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
  params.insert(std::make_unique<ArraySelectionParameter>(k_AxisLengthsArrayPath_Key, "Axis Lengths", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AxisEulerAnglesArrayPath_Key, "Axis Euler Angles", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ElasticStrainsArrayPath_Key, "Elastic Strains (Voigt Notation)", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
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
IFilter::PreflightResult ComputeFeatureEigenstrains::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
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
Result<> ComputeFeatureEigenstrains::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
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
