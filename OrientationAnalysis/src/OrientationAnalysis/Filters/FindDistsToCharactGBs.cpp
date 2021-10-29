#include "FindDistsToCharactGBs.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindDistsToCharactGBs::name() const
{
  return FilterTraits<FindDistsToCharactGBs>::name.str();
}

//------------------------------------------------------------------------------
std::string FindDistsToCharactGBs::className() const
{
  return FilterTraits<FindDistsToCharactGBs>::className;
}

//------------------------------------------------------------------------------
Uuid FindDistsToCharactGBs::uuid() const
{
  return FilterTraits<FindDistsToCharactGBs>::uuid;
}

//------------------------------------------------------------------------------
std::string FindDistsToCharactGBs::humanName() const
{
  return "Find Distances to Characteristic Grain Boundaries";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindDistsToCharactGBs::defaultTags() const
{
  return {"#Statistics", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindDistsToCharactGBs::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceNormalsArrayPath_Key, "Face Normals", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureEulerAnglesArrayPath_Key, "Average Euler Angles", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_DistToTiltArrayPath_Key, "Distance to Nearest Tilt Boundary", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DistToTwistArrayPath_Key, "Distance to Nearest Twist Boundary", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DistToSymmetricArrayPath_Key, "Distance to Nearest Symmetric Boundary", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DistTo180TiltArrayPath_Key, "Distance to Nearest 180°-tilt  Boundary", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindDistsToCharactGBs::clone() const
{
  return std::make_unique<FindDistsToCharactGBs>();
}

//------------------------------------------------------------------------------
Result<OutputActions> FindDistsToCharactGBs::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pSurfaceMeshFaceNormalsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  auto pFeatureEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pDistToTiltArrayPathValue = filterArgs.value<DataPath>(k_DistToTiltArrayPath_Key);
  auto pDistToTwistArrayPathValue = filterArgs.value<DataPath>(k_DistToTwistArrayPath_Key);
  auto pDistToSymmetricArrayPathValue = filterArgs.value<DataPath>(k_DistToSymmetricArrayPath_Key);
  auto pDistTo180TiltArrayPathValue = filterArgs.value<DataPath>(k_DistTo180TiltArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindDistsToCharactGBsAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindDistsToCharactGBs::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pSurfaceMeshFaceNormalsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  auto pFeatureEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pDistToTiltArrayPathValue = filterArgs.value<DataPath>(k_DistToTiltArrayPath_Key);
  auto pDistToTwistArrayPathValue = filterArgs.value<DataPath>(k_DistToTwistArrayPath_Key);
  auto pDistToSymmetricArrayPathValue = filterArgs.value<DataPath>(k_DistToSymmetricArrayPath_Key);
  auto pDistTo180TiltArrayPathValue = filterArgs.value<DataPath>(k_DistTo180TiltArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
