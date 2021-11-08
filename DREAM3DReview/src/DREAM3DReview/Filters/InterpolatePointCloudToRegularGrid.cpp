#include "InterpolatePointCloudToRegularGrid.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string InterpolatePointCloudToRegularGrid::name() const
{
  return FilterTraits<InterpolatePointCloudToRegularGrid>::name.str();
}

//------------------------------------------------------------------------------
std::string InterpolatePointCloudToRegularGrid::className() const
{
  return FilterTraits<InterpolatePointCloudToRegularGrid>::className;
}

//------------------------------------------------------------------------------
Uuid InterpolatePointCloudToRegularGrid::uuid() const
{
  return FilterTraits<InterpolatePointCloudToRegularGrid>::uuid;
}

//------------------------------------------------------------------------------
std::string InterpolatePointCloudToRegularGrid::humanName() const
{
  return "Interpolate Point Cloud to Regular Grid";
}

//------------------------------------------------------------------------------
std::vector<std::string> InterpolatePointCloudToRegularGrid::defaultTags() const
{
  return {"#Sampling", "#InterpolationFilters"};
}

//------------------------------------------------------------------------------
Parameters InterpolatePointCloudToRegularGrid::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_StoreKernelDistances_Key, "Store Kernel Distances", "", false));
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_InterpolationTechnique_Key, "Interpolation Technique", "", 0, ChoicesParameter::Choices{"Uniform", "Gaussian"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_KernelSize_Key, "Kernel Size", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Sigmas_Key, "Gaussian Sigmas", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DataContainerName_Key, "Data Container to Interpolate", "", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_InterpolatedDataContainerName_Key, "Interpolated Data Container", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Vertex Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_VoxelIndicesArrayPath_Key, "Voxel Indices", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "", DataPath{}));
  params.insert(
      std::make_unique<MultiArraySelectionParameter>(k_ArraysToInterpolate_Key, "Attribute Arrays to Interpolate", "", MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_ArraysToCopy_Key, "Attribute Arrays to Copy", "", MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_InterpolatedAttributeMatrixName_Key, "Interpolated Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_KernelDistancesArrayName_Key, "Kernel Distances", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_InterpolatedSuffix_Key, "Interpolated Array Suffix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_CopySuffix_Key, "Copied Array Suffix", "", "SomeString"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);
  params.linkParameters(k_StoreKernelDistances_Key, k_KernelDistancesArrayName_Key, true);
  params.linkParameters(k_InterpolationTechnique_Key, k_Sigmas_Key, 0);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer InterpolatePointCloudToRegularGrid::clone() const
{
  return std::make_unique<InterpolatePointCloudToRegularGrid>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult InterpolatePointCloudToRegularGrid::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pStoreKernelDistancesValue = filterArgs.value<bool>(k_StoreKernelDistances_Key);
  auto pInterpolationTechniqueValue = filterArgs.value<ChoicesParameter::ValueType>(k_InterpolationTechnique_Key);
  auto pKernelSizeValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_KernelSize_Key);
  auto pSigmasValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Sigmas_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pInterpolatedDataContainerNameValue = filterArgs.value<DataPath>(k_InterpolatedDataContainerName_Key);
  auto pVoxelIndicesArrayPathValue = filterArgs.value<DataPath>(k_VoxelIndicesArrayPath_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pArraysToInterpolateValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_ArraysToInterpolate_Key);
  auto pArraysToCopyValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_ArraysToCopy_Key);
  auto pInterpolatedAttributeMatrixNameValue = filterArgs.value<DataPath>(k_InterpolatedAttributeMatrixName_Key);
  auto pKernelDistancesArrayNameValue = filterArgs.value<DataPath>(k_KernelDistancesArrayName_Key);
  auto pInterpolatedSuffixValue = filterArgs.value<StringParameter::ValueType>(k_InterpolatedSuffix_Key);
  auto pCopySuffixValue = filterArgs.value<StringParameter::ValueType>(k_CopySuffix_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<InterpolatePointCloudToRegularGridAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> InterpolatePointCloudToRegularGrid::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pStoreKernelDistancesValue = filterArgs.value<bool>(k_StoreKernelDistances_Key);
  auto pInterpolationTechniqueValue = filterArgs.value<ChoicesParameter::ValueType>(k_InterpolationTechnique_Key);
  auto pKernelSizeValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_KernelSize_Key);
  auto pSigmasValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Sigmas_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pInterpolatedDataContainerNameValue = filterArgs.value<DataPath>(k_InterpolatedDataContainerName_Key);
  auto pVoxelIndicesArrayPathValue = filterArgs.value<DataPath>(k_VoxelIndicesArrayPath_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pArraysToInterpolateValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_ArraysToInterpolate_Key);
  auto pArraysToCopyValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_ArraysToCopy_Key);
  auto pInterpolatedAttributeMatrixNameValue = filterArgs.value<DataPath>(k_InterpolatedAttributeMatrixName_Key);
  auto pKernelDistancesArrayNameValue = filterArgs.value<DataPath>(k_KernelDistancesArrayName_Key);
  auto pInterpolatedSuffixValue = filterArgs.value<StringParameter::ValueType>(k_InterpolatedSuffix_Key);
  auto pCopySuffixValue = filterArgs.value<StringParameter::ValueType>(k_CopySuffix_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
