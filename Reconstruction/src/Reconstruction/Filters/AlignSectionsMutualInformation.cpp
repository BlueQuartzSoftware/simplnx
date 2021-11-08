#include "AlignSectionsMutualInformation.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string AlignSectionsMutualInformation::name() const
{
  return FilterTraits<AlignSectionsMutualInformation>::name.str();
}

//------------------------------------------------------------------------------
std::string AlignSectionsMutualInformation::className() const
{
  return FilterTraits<AlignSectionsMutualInformation>::className;
}

//------------------------------------------------------------------------------
Uuid AlignSectionsMutualInformation::uuid() const
{
  return FilterTraits<AlignSectionsMutualInformation>::uuid;
}

//------------------------------------------------------------------------------
std::string AlignSectionsMutualInformation::humanName() const
{
  return "Align Sections (Mutual Information)";
}

//------------------------------------------------------------------------------
std::vector<std::string> AlignSectionsMutualInformation::defaultTags() const
{
  return {"#Reconstruction", "#Alignment"};
}

//------------------------------------------------------------------------------
Parameters AlignSectionsMutualInformation::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_WriteAlignmentShifts_Key, "Write Alignment Shift File", "", false));
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_AlignmentShiftFileName_Key, "Alignment File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<Float32Parameter>(k_MisorientationTolerance_Key, "Misorientation Tolerance", "", 1.23345f));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseGoodVoxels_Key, "Use Mask Array", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Quaternions", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsArrayPath_Key, "Mask", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_WriteAlignmentShifts_Key, k_AlignmentShiftFileName_Key, true);
  params.linkParameters(k_UseGoodVoxels_Key, k_GoodVoxelsArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AlignSectionsMutualInformation::clone() const
{
  return std::make_unique<AlignSectionsMutualInformation>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AlignSectionsMutualInformation::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pWriteAlignmentShiftsValue = filterArgs.value<bool>(k_WriteAlignmentShifts_Key);
  auto pAlignmentShiftFileNameValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_AlignmentShiftFileName_Key);
  auto pMisorientationToleranceValue = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  auto pUseGoodVoxelsValue = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<AlignSectionsMutualInformationAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> AlignSectionsMutualInformation::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pWriteAlignmentShiftsValue = filterArgs.value<bool>(k_WriteAlignmentShifts_Key);
  auto pAlignmentShiftFileNameValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_AlignmentShiftFileName_Key);
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
