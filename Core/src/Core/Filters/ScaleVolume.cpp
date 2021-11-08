#include "ScaleVolume.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ScaleVolume::name() const
{
  return FilterTraits<ScaleVolume>::name.str();
}

//------------------------------------------------------------------------------
std::string ScaleVolume::className() const
{
  return FilterTraits<ScaleVolume>::className;
}

//------------------------------------------------------------------------------
Uuid ScaleVolume::uuid() const
{
  return FilterTraits<ScaleVolume>::uuid;
}

//------------------------------------------------------------------------------
std::string ScaleVolume::humanName() const
{
  return "Change Scaling of Volume";
}

//------------------------------------------------------------------------------
std::vector<std::string> ScaleVolume::defaultTags() const
{
  return {"#Core", "#Spatial"};
}

//------------------------------------------------------------------------------
Parameters ScaleVolume::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<VectorFloat32Parameter>(k_ScaleFactor_Key, "Scaling Factor", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ApplyToVoxelVolume_Key, "Apply to Image Geometry", "", false));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DataContainerName_Key, "Data Container Image Geometry to Scale", "", DataPath{}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ApplyToSurfaceMesh_Key, "Apply to Surface Geometry", "", false));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SurfaceDataContainerName_Key, "Data Container Surface Geometry to Scale", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ApplyToVoxelVolume_Key, k_DataContainerName_Key, true);
  params.linkParameters(k_ApplyToSurfaceMesh_Key, k_SurfaceDataContainerName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ScaleVolume::clone() const
{
  return std::make_unique<ScaleVolume>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ScaleVolume::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pScaleFactorValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ScaleFactor_Key);
  auto pApplyToVoxelVolumeValue = filterArgs.value<bool>(k_ApplyToVoxelVolume_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pApplyToSurfaceMeshValue = filterArgs.value<bool>(k_ApplyToSurfaceMesh_Key);
  auto pSurfaceDataContainerNameValue = filterArgs.value<DataPath>(k_SurfaceDataContainerName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ScaleVolumeAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ScaleVolume::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pScaleFactorValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ScaleFactor_Key);
  auto pApplyToVoxelVolumeValue = filterArgs.value<bool>(k_ApplyToVoxelVolume_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pApplyToSurfaceMeshValue = filterArgs.value<bool>(k_ApplyToSurfaceMesh_Key);
  auto pSurfaceDataContainerNameValue = filterArgs.value<DataPath>(k_SurfaceDataContainerName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
