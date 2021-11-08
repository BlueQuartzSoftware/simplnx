#include "FindCSLBoundaries.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindCSLBoundaries::name() const
{
  return FilterTraits<FindCSLBoundaries>::name.str();
}

//------------------------------------------------------------------------------
std::string FindCSLBoundaries::className() const
{
  return FilterTraits<FindCSLBoundaries>::className;
}

//------------------------------------------------------------------------------
Uuid FindCSLBoundaries::uuid() const
{
  return FilterTraits<FindCSLBoundaries>::uuid;
}

//------------------------------------------------------------------------------
std::string FindCSLBoundaries::humanName() const
{
  return "Find CSL Boundaries";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindCSLBoundaries::defaultTags() const
{
  return {"#Unsupported", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindCSLBoundaries::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float32Parameter>(k_CSL_Key, "CSL (Sigma)", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_AxisTolerance_Key, "Axis Tolerance (Degrees)", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_AngleTolerance_Key, "Angle Tolerance (Degrees)", "", 1.23345f));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceNormalsArrayPath_Key, "Face Normals", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceMeshCSLBoundaryArrayName_Key, "CSL Boundary", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceMeshCSLBoundaryIncoherenceArrayName_Key, "CSL Boundary Incoherence", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindCSLBoundaries::clone() const
{
  return std::make_unique<FindCSLBoundaries>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindCSLBoundaries::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pCSLValue = filterArgs.value<float32>(k_CSL_Key);
  auto pAxisToleranceValue = filterArgs.value<float32>(k_AxisTolerance_Key);
  auto pAngleToleranceValue = filterArgs.value<float32>(k_AngleTolerance_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pSurfaceMeshFaceNormalsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  auto pSurfaceMeshCSLBoundaryArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshCSLBoundaryArrayName_Key);
  auto pSurfaceMeshCSLBoundaryIncoherenceArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshCSLBoundaryIncoherenceArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindCSLBoundariesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindCSLBoundaries::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pCSLValue = filterArgs.value<float32>(k_CSL_Key);
  auto pAxisToleranceValue = filterArgs.value<float32>(k_AxisTolerance_Key);
  auto pAngleToleranceValue = filterArgs.value<float32>(k_AngleTolerance_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pSurfaceMeshFaceNormalsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  auto pSurfaceMeshCSLBoundaryArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshCSLBoundaryArrayName_Key);
  auto pSurfaceMeshCSLBoundaryIncoherenceArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshCSLBoundaryIncoherenceArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
