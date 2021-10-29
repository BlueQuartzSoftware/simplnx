#include "AdaptiveAlignmentMutualInformation.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string AdaptiveAlignmentMutualInformation::name() const
{
  return FilterTraits<AdaptiveAlignmentMutualInformation>::name.str();
}

//------------------------------------------------------------------------------
std::string AdaptiveAlignmentMutualInformation::className() const
{
  return FilterTraits<AdaptiveAlignmentMutualInformation>::className;
}

//------------------------------------------------------------------------------
Uuid AdaptiveAlignmentMutualInformation::uuid() const
{
  return FilterTraits<AdaptiveAlignmentMutualInformation>::uuid;
}

//------------------------------------------------------------------------------
std::string AdaptiveAlignmentMutualInformation::humanName() const
{
  return "Adaptive Alignment (Mutual Information)";
}

//------------------------------------------------------------------------------
std::vector<std::string> AdaptiveAlignmentMutualInformation::defaultTags() const
{
  return {"#Reconstruction", "#Anisotropic Alignment"};
}

//------------------------------------------------------------------------------
Parameters AdaptiveAlignmentMutualInformation::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_GlobalCorrection_Key, "Global Correction", "", 0, ChoicesParameter::Choices{"None", "SEM Images", "Own Shifts"}));
  params.insertSeparator(Parameters::Separator{"Image Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_ImageDataArrayPath_Key, "Image Data", "", DataPath{}));
  params.insert(std::make_unique<Float32Parameter>(k_ShiftX_Key, "Total Shift In X-Direction (Microns)", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_ShiftY_Key, "Total Shift In Y-Direction (Microns)", "", 1.23345f));
  params.insert(
      std::make_unique<MultiArraySelectionParameter>(k_IgnoredDataArrayPaths_Key, "Attribute Arrays to Ignore", "", MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}));
  params.insert(std::make_unique<Float32Parameter>(k_MisorientationTolerance_Key, "Misorientation Tolerance", "", 1.23345f));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseGoodVoxels_Key, "Use Mask Array", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Quaternions", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsArrayPath_Key, "Mask", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_GlobalCorrection_Key, k_ImageDataArrayPath_Key, 0);
  params.linkParameters(k_GlobalCorrection_Key, k_ShiftX_Key, 1);
  params.linkParameters(k_GlobalCorrection_Key, k_ShiftY_Key, 2);
  params.linkParameters(k_UseGoodVoxels_Key, k_GoodVoxelsArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AdaptiveAlignmentMutualInformation::clone() const
{
  return std::make_unique<AdaptiveAlignmentMutualInformation>();
}

//------------------------------------------------------------------------------
Result<OutputActions> AdaptiveAlignmentMutualInformation::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pGlobalCorrectionValue = filterArgs.value<ChoicesParameter::ValueType>(k_GlobalCorrection_Key);
  auto pImageDataArrayPathValue = filterArgs.value<DataPath>(k_ImageDataArrayPath_Key);
  auto pShiftXValue = filterArgs.value<float32>(k_ShiftX_Key);
  auto pShiftYValue = filterArgs.value<float32>(k_ShiftY_Key);
  auto pIgnoredDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);
  auto pMisorientationToleranceValue = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  auto pUseGoodVoxelsValue = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<AdaptiveAlignmentMutualInformationAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> AdaptiveAlignmentMutualInformation::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pGlobalCorrectionValue = filterArgs.value<ChoicesParameter::ValueType>(k_GlobalCorrection_Key);
  auto pImageDataArrayPathValue = filterArgs.value<DataPath>(k_ImageDataArrayPath_Key);
  auto pShiftXValue = filterArgs.value<float32>(k_ShiftX_Key);
  auto pShiftYValue = filterArgs.value<float32>(k_ShiftY_Key);
  auto pIgnoredDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);
  auto pMisorientationToleranceValue = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  auto pUseGoodVoxelsValue = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
