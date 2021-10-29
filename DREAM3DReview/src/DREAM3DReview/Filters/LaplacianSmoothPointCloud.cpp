#include "LaplacianSmoothPointCloud.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string LaplacianSmoothPointCloud::name() const
{
  return FilterTraits<LaplacianSmoothPointCloud>::name.str();
}

//------------------------------------------------------------------------------
std::string LaplacianSmoothPointCloud::className() const
{
  return FilterTraits<LaplacianSmoothPointCloud>::className;
}

//------------------------------------------------------------------------------
Uuid LaplacianSmoothPointCloud::uuid() const
{
  return FilterTraits<LaplacianSmoothPointCloud>::uuid;
}

//------------------------------------------------------------------------------
std::string LaplacianSmoothPointCloud::humanName() const
{
  return "Smooth Point Cloud (Laplacian)";
}

//------------------------------------------------------------------------------
std::vector<std::string> LaplacianSmoothPointCloud::defaultTags() const
{
  return {"#PointCloudFilters", "#Smoothing"};
}

//------------------------------------------------------------------------------
Parameters LaplacianSmoothPointCloud::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask", "", false));
  params.insert(std::make_unique<Int32Parameter>(k_NumIterations_Key, "Number of Iterations", "", 1234356));
  params.insert(std::make_unique<Float32Parameter>(k_Lambda_Key, "Lambda", "", 1.23345f));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DataContainerName_Key, "Data Container to Smooth", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer LaplacianSmoothPointCloud::clone() const
{
  return std::make_unique<LaplacianSmoothPointCloud>();
}

//------------------------------------------------------------------------------
Result<OutputActions> LaplacianSmoothPointCloud::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pNumIterationsValue = filterArgs.value<int32>(k_NumIterations_Key);
  auto pLambdaValue = filterArgs.value<float32>(k_Lambda_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<LaplacianSmoothPointCloudAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> LaplacianSmoothPointCloud::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pNumIterationsValue = filterArgs.value<int32>(k_NumIterations_Key);
  auto pLambdaValue = filterArgs.value<float32>(k_Lambda_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
