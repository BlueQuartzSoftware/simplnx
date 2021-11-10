#include "MapPointCloudToRegularGrid.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string MapPointCloudToRegularGrid::name() const
{
  return FilterTraits<MapPointCloudToRegularGrid>::name.str();
}

//------------------------------------------------------------------------------
std::string MapPointCloudToRegularGrid::className() const
{
  return FilterTraits<MapPointCloudToRegularGrid>::className;
}

//------------------------------------------------------------------------------
Uuid MapPointCloudToRegularGrid::uuid() const
{
  return FilterTraits<MapPointCloudToRegularGrid>::uuid;
}

//------------------------------------------------------------------------------
std::string MapPointCloudToRegularGrid::humanName() const
{
  return "Map Point Cloud to Regular Grid";
}

//------------------------------------------------------------------------------
std::vector<std::string> MapPointCloudToRegularGrid::defaultTags() const
{
  return {"#Sampling", "#Mapping"};
}

//------------------------------------------------------------------------------
Parameters MapPointCloudToRegularGrid::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_SamplingGridType_Key, "Sampling Grid Type", "", 0, ChoicesParameter::Choices{"Manual", "Use Exsting Image Geometry"}));
  params.insert(std::make_unique<VectorInt32Parameter>(k_GridDimensions_Key, "Grid Dimensions", "", std::vector<int32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_ImageDataContainerPath_Key, "ImageDataContainerPath", "", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DataContainerName_Key, "Data Container to Map", "", DataPath{}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask", "", false));
  params.insertSeparator(Parameters::Separator{"Vertex Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Vertex Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_VoxelIndicesArrayPath_Key, "Voxel Indices", "", DataPath{}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedImageDataContainerName_Key, "Created Image DataContainer", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_SamplingGridType_Key, k_GridDimensions_Key, 0);
  params.linkParameters(k_SamplingGridType_Key, k_CreatedImageDataContainerName_Key, 0);
  params.linkParameters(k_SamplingGridType_Key, k_ImageDataContainerPath_Key, 1);
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MapPointCloudToRegularGrid::clone() const
{
  return std::make_unique<MapPointCloudToRegularGrid>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult MapPointCloudToRegularGrid::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pSamplingGridTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_SamplingGridType_Key);
  auto pGridDimensionsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_GridDimensions_Key);
  auto pImageDataContainerPathValue = filterArgs.value<DataPath>(k_ImageDataContainerPath_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pVoxelIndicesArrayPathValue = filterArgs.value<DataPath>(k_VoxelIndicesArrayPath_Key);
  auto pCreatedImageDataContainerNameValue = filterArgs.value<DataPath>(k_CreatedImageDataContainerName_Key);

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
  auto action = std::make_unique<MapPointCloudToRegularGridAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> MapPointCloudToRegularGrid::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSamplingGridTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_SamplingGridType_Key);
  auto pGridDimensionsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_GridDimensions_Key);
  auto pImageDataContainerPathValue = filterArgs.value<DataPath>(k_ImageDataContainerPath_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pVoxelIndicesArrayPathValue = filterArgs.value<DataPath>(k_VoxelIndicesArrayPath_Key);
  auto pCreatedImageDataContainerNameValue = filterArgs.value<DataPath>(k_CreatedImageDataContainerName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
