#include "FindTwinBoundaries.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindTwinBoundaries::name() const
{
  return FilterTraits<FindTwinBoundaries>::name.str();
}

//------------------------------------------------------------------------------
std::string FindTwinBoundaries::className() const
{
  return FilterTraits<FindTwinBoundaries>::className;
}

//------------------------------------------------------------------------------
Uuid FindTwinBoundaries::uuid() const
{
  return FilterTraits<FindTwinBoundaries>::uuid;
}

//------------------------------------------------------------------------------
std::string FindTwinBoundaries::humanName() const
{
  return "Find Twin Boundaries";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindTwinBoundaries::defaultTags() const
{
  return {"#Statistics", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindTwinBoundaries::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float32Parameter>(k_AxisTolerance_Key, "Axis Tolerance (Degrees)", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_AngleTolerance_Key, "Angle Tolerance (Degrees)", "", 1.23345f));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindCoherence_Key, "Compute Coherence", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceNormalsArrayPath_Key, "Face Normals", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceMeshTwinBoundaryArrayName_Key, "Twin Boundary", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceMeshTwinBoundaryIncoherenceArrayName_Key, "Twin Boundary Incoherence", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_FindCoherence_Key, k_SurfaceMeshFaceNormalsArrayPath_Key, true);
  params.linkParameters(k_FindCoherence_Key, k_SurfaceMeshTwinBoundaryIncoherenceArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindTwinBoundaries::clone() const
{
  return std::make_unique<FindTwinBoundaries>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindTwinBoundaries::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pAxisToleranceValue = filterArgs.value<float32>(k_AxisTolerance_Key);
  auto pAngleToleranceValue = filterArgs.value<float32>(k_AngleTolerance_Key);
  auto pFindCoherenceValue = filterArgs.value<bool>(k_FindCoherence_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pSurfaceMeshFaceNormalsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  auto pSurfaceMeshTwinBoundaryArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshTwinBoundaryArrayName_Key);
  auto pSurfaceMeshTwinBoundaryIncoherenceArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshTwinBoundaryIncoherenceArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindTwinBoundariesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindTwinBoundaries::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pAxisToleranceValue = filterArgs.value<float32>(k_AxisTolerance_Key);
  auto pAngleToleranceValue = filterArgs.value<float32>(k_AngleTolerance_Key);
  auto pFindCoherenceValue = filterArgs.value<bool>(k_FindCoherence_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pSurfaceMeshFaceNormalsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  auto pSurfaceMeshTwinBoundaryArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshTwinBoundaryArrayName_Key);
  auto pSurfaceMeshTwinBoundaryIncoherenceArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshTwinBoundaryIncoherenceArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
