#include "EMMPMFilter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/EMMPMFilterParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string EMMPMFilter::name() const
{
  return FilterTraits<EMMPMFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string EMMPMFilter::className() const
{
  return FilterTraits<EMMPMFilter>::className;
}

//------------------------------------------------------------------------------
Uuid EMMPMFilter::uuid() const
{
  return FilterTraits<EMMPMFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string EMMPMFilter::humanName() const
{
  return "Segment Features (EM/MPM)";
}

//------------------------------------------------------------------------------
std::vector<std::string> EMMPMFilter::defaultTags() const
{
  return {"#Reconstruction", "#Segmentation"};
}

//------------------------------------------------------------------------------
Parameters EMMPMFilter::parameters() const
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
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputDataArrayPath_Key, "Input Attribute Array", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputDataArrayPath_Key, "Output Attribute Array", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseGradientPenalty_Key, k_GradientBetaE_Key, true);
  params.linkParameters(k_UseCurvaturePenalty_Key, k_CurvatureBetaC_Key, true);
  params.linkParameters(k_UseCurvaturePenalty_Key, k_CurvatureRMax_Key, true);
  params.linkParameters(k_UseCurvaturePenalty_Key, k_CurvatureEMLoopDelay_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer EMMPMFilter::clone() const
{
  return std::make_unique<EMMPMFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult EMMPMFilter::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
  auto pInputDataArrayPathValue = filterArgs.value<DataPath>(k_InputDataArrayPath_Key);
  auto pOutputDataArrayPathValue = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);

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
  auto action = std::make_unique<EMMPMFilterAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> EMMPMFilter::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
  auto pInputDataArrayPathValue = filterArgs.value<DataPath>(k_InputDataArrayPath_Key);
  auto pOutputDataArrayPathValue = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
