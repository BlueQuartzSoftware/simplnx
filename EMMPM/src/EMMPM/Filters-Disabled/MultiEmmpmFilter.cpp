#include "MultiEmmpmFilter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/EMMPMFilterParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string MultiEmmpmFilter::name() const
{
  return FilterTraits<MultiEmmpmFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string MultiEmmpmFilter::className() const
{
  return FilterTraits<MultiEmmpmFilter>::className;
}

//------------------------------------------------------------------------------
Uuid MultiEmmpmFilter::uuid() const
{
  return FilterTraits<MultiEmmpmFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string MultiEmmpmFilter::humanName() const
{
  return "Segment Features (EM/MPM Multi-Array)";
}

//------------------------------------------------------------------------------
std::vector<std::string> MultiEmmpmFilter::defaultTags() const
{
  return {"#Reconstruction", "#Segmentation"};
}

//------------------------------------------------------------------------------
Parameters MultiEmmpmFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  /*[x]*/ params.insert(std::make_unique<EMMPMFilterParameter>(k_NumClasses_Key, "EMMPM Widget", "", {}));
  params.insert(std::make_unique<BoolParameter>(k_UseOneBasedValues_Key, "Use 1-Based Values", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseGradientPenalty_Key, "Use Gradient Penalty", "", false));
  params.insert(std::make_unique<Float64Parameter>(k_GradientBetaE_Key, "Beta E", "", 2.3456789));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseCurvaturePenalty_Key, "Use Curvature Penalty", "", false));
  params.insert(std::make_unique<Float64Parameter>(k_CurvatureBetaC_Key, "Beta C", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_CurvatureRMax_Key, "R Max", "", 2.3456789));
  params.insert(std::make_unique<Int32Parameter>(k_CurvatureEMLoopDelay_Key, "EM Loop Delay", "", 1234356));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_InputDataArrayVector_Key, "Input Attribute Arrays", "", MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_OutputAttributeMatrixName_Key, "Output Cell Attribute Matrix", "", "SomeString"));
  params.insert(std::make_unique<BoolParameter>(k_UsePreviousMuSigma_Key, "Use Mu/Sigma from Previous Image as Initialization for Current Image", "", false));
  params.insert(std::make_unique<StringParameter>(k_OutputArrayPrefix_Key, "Output Array Prefix", "", "SomeString"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseGradientPenalty_Key, k_GradientBetaE_Key, true);
  params.linkParameters(k_UseCurvaturePenalty_Key, k_CurvatureBetaC_Key, true);
  params.linkParameters(k_UseCurvaturePenalty_Key, k_CurvatureRMax_Key, true);
  params.linkParameters(k_UseCurvaturePenalty_Key, k_CurvatureEMLoopDelay_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MultiEmmpmFilter::clone() const
{
  return std::make_unique<MultiEmmpmFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult MultiEmmpmFilter::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pNumClassesValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_NumClasses_Key);
  auto pUseOneBasedValuesValue = filterArgs.value<bool>(k_UseOneBasedValues_Key);
  auto pUseGradientPenaltyValue = filterArgs.value<bool>(k_UseGradientPenalty_Key);
  auto pGradientBetaEValue = filterArgs.value<float64>(k_GradientBetaE_Key);
  auto pUseCurvaturePenaltyValue = filterArgs.value<bool>(k_UseCurvaturePenalty_Key);
  auto pCurvatureBetaCValue = filterArgs.value<float64>(k_CurvatureBetaC_Key);
  auto pCurvatureRMaxValue = filterArgs.value<float64>(k_CurvatureRMax_Key);
  auto pCurvatureEMLoopDelayValue = filterArgs.value<int32>(k_CurvatureEMLoopDelay_Key);
  auto pInputDataArrayVectorValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_InputDataArrayVector_Key);
  auto pOutputAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_OutputAttributeMatrixName_Key);
  auto pUsePreviousMuSigmaValue = filterArgs.value<bool>(k_UsePreviousMuSigma_Key);
  auto pOutputArrayPrefixValue = filterArgs.value<StringParameter::ValueType>(k_OutputArrayPrefix_Key);

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
  auto action = std::make_unique<MultiEmmpmFilterAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> MultiEmmpmFilter::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pNumClassesValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_NumClasses_Key);
  auto pUseOneBasedValuesValue = filterArgs.value<bool>(k_UseOneBasedValues_Key);
  auto pUseGradientPenaltyValue = filterArgs.value<bool>(k_UseGradientPenalty_Key);
  auto pGradientBetaEValue = filterArgs.value<float64>(k_GradientBetaE_Key);
  auto pUseCurvaturePenaltyValue = filterArgs.value<bool>(k_UseCurvaturePenalty_Key);
  auto pCurvatureBetaCValue = filterArgs.value<float64>(k_CurvatureBetaC_Key);
  auto pCurvatureRMaxValue = filterArgs.value<float64>(k_CurvatureRMax_Key);
  auto pCurvatureEMLoopDelayValue = filterArgs.value<int32>(k_CurvatureEMLoopDelay_Key);
  auto pInputDataArrayVectorValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_InputDataArrayVector_Key);
  auto pOutputAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_OutputAttributeMatrixName_Key);
  auto pUsePreviousMuSigmaValue = filterArgs.value<bool>(k_UsePreviousMuSigma_Key);
  auto pOutputArrayPrefixValue = filterArgs.value<StringParameter::ValueType>(k_OutputArrayPrefix_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
