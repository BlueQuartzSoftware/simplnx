#include "FindGBCDMetricBased.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindGBCDMetricBased::name() const
{
  return FilterTraits<FindGBCDMetricBased>::name.str();
}

//------------------------------------------------------------------------------
std::string FindGBCDMetricBased::className() const
{
  return FilterTraits<FindGBCDMetricBased>::className;
}

//------------------------------------------------------------------------------
Uuid FindGBCDMetricBased::uuid() const
{
  return FilterTraits<FindGBCDMetricBased>::uuid;
}

//------------------------------------------------------------------------------
std::string FindGBCDMetricBased::humanName() const
{
  return "Find GBCD (Metric-Based Approach)";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindGBCDMetricBased::defaultTags() const
{
  return {"#Statistics", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindGBCDMetricBased::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_PhaseOfInterest_Key, "Phase of Interest", "", 1234356));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_MisorientationRotation_Key, "Fixed Misorientation", "", std::vector<float32>(4), std::vector<std::string>(4)));
  params.insert(std::make_unique<ChoicesParameter>(k_ChosenLimitDists_Key, "Limiting Distances", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(std::make_unique<Int32Parameter>(k_NumSamplPts_Key, "Number of Sampling Points (on a Hemisphere)", "", 1234356));
  params.insert(std::make_unique<BoolParameter>(k_ExcludeTripleLines_Key, "Exclude Triangles Directly Neighboring Triple Lines", "", false));
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_DistOutputFile_Key, "Output Distribution File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<FileSystemPathParameter>(k_ErrOutputFile_Key, "Output Distribution Errors File", "", fs::path("<default file to read goes here>"),
                                                          FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<BoolParameter>(k_SaveRelativeErr_Key, "Save Relative Errors Instead of Their Absolute Values", "", false));
  params.insertSeparator(Parameters::Separator{"Vertex Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_NodeTypesArrayPath_Key, "Node Types", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceNormalsArrayPath_Key, "Face Normals", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceAreasArrayPath_Key, "Face Areas", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Face Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFeatureFaceLabelsArrayPath_Key, "Feature Face Labels", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureEulerAnglesArrayPath_Key, "Average Euler Angles", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindGBCDMetricBased::clone() const
{
  return std::make_unique<FindGBCDMetricBased>();
}

//------------------------------------------------------------------------------
Result<OutputActions> FindGBCDMetricBased::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pPhaseOfInterestValue = filterArgs.value<int32>(k_PhaseOfInterest_Key);
  auto pMisorientationRotationValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MisorientationRotation_Key);
  auto pChosenLimitDistsValue = filterArgs.value<ChoicesParameter::ValueType>(k_ChosenLimitDists_Key);
  auto pNumSamplPtsValue = filterArgs.value<int32>(k_NumSamplPts_Key);
  auto pExcludeTripleLinesValue = filterArgs.value<bool>(k_ExcludeTripleLines_Key);
  auto pDistOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_DistOutputFile_Key);
  auto pErrOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_ErrOutputFile_Key);
  auto pSaveRelativeErrValue = filterArgs.value<bool>(k_SaveRelativeErr_Key);
  auto pNodeTypesArrayPathValue = filterArgs.value<DataPath>(k_NodeTypesArrayPath_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pSurfaceMeshFaceNormalsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  auto pSurfaceMeshFaceAreasArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceAreasArrayPath_Key);
  auto pSurfaceMeshFeatureFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFeatureFaceLabelsArrayPath_Key);
  auto pFeatureEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindGBCDMetricBasedAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindGBCDMetricBased::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pPhaseOfInterestValue = filterArgs.value<int32>(k_PhaseOfInterest_Key);
  auto pMisorientationRotationValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MisorientationRotation_Key);
  auto pChosenLimitDistsValue = filterArgs.value<ChoicesParameter::ValueType>(k_ChosenLimitDists_Key);
  auto pNumSamplPtsValue = filterArgs.value<int32>(k_NumSamplPts_Key);
  auto pExcludeTripleLinesValue = filterArgs.value<bool>(k_ExcludeTripleLines_Key);
  auto pDistOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_DistOutputFile_Key);
  auto pErrOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_ErrOutputFile_Key);
  auto pSaveRelativeErrValue = filterArgs.value<bool>(k_SaveRelativeErr_Key);
  auto pNodeTypesArrayPathValue = filterArgs.value<DataPath>(k_NodeTypesArrayPath_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pSurfaceMeshFaceNormalsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  auto pSurfaceMeshFaceAreasArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceAreasArrayPath_Key);
  auto pSurfaceMeshFeatureFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFeatureFaceLabelsArrayPath_Key);
  auto pFeatureEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
