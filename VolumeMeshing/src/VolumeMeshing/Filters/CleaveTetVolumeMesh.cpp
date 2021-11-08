#include "CleaveTetVolumeMesh.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string CleaveTetVolumeMesh::name() const
{
  return FilterTraits<CleaveTetVolumeMesh>::name.str();
}

//------------------------------------------------------------------------------
std::string CleaveTetVolumeMesh::className() const
{
  return FilterTraits<CleaveTetVolumeMesh>::className;
}

//------------------------------------------------------------------------------
Uuid CleaveTetVolumeMesh::uuid() const
{
  return FilterTraits<CleaveTetVolumeMesh>::uuid;
}

//------------------------------------------------------------------------------
std::string CleaveTetVolumeMesh::humanName() const
{
  return "Generate Tetrahedral Volume Mesh";
}

//------------------------------------------------------------------------------
std::vector<std::string> CleaveTetVolumeMesh::defaultTags() const
{
  return {"#Volume Meshing", "#Generation"};
}

//------------------------------------------------------------------------------
Parameters CleaveTetVolumeMesh::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(
      std::make_unique<MultiArraySelectionParameter>(k_SignedDistanceArrayPaths_Key, "Attribute Arrays to Combine", "", MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}));
  params.insert(std::make_unique<Float32Parameter>(k_Lipschitz_Key, "Lipschitz", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_Scale_Key, "Scale", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_Multiplier_Key, "Multiplier", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_Alpha_Key, "Alpha", "", 1.23345f));
  params.insert(std::make_unique<Int32Parameter>(k_Padding_Key, "Padding", "", 1234356));
  params.insert(std::make_unique<BoolParameter>(k_AdaptiveSurface_Key, "Adaptive Surface", "", false));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CleaveTetVolumeMesh::clone() const
{
  return std::make_unique<CleaveTetVolumeMesh>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CleaveTetVolumeMesh::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSignedDistanceArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SignedDistanceArrayPaths_Key);
  auto pLipschitzValue = filterArgs.value<float32>(k_Lipschitz_Key);
  auto pScaleValue = filterArgs.value<float32>(k_Scale_Key);
  auto pMultiplierValue = filterArgs.value<float32>(k_Multiplier_Key);
  auto pAlphaValue = filterArgs.value<float32>(k_Alpha_Key);
  auto pPaddingValue = filterArgs.value<int32>(k_Padding_Key);
  auto pAdaptiveSurfaceValue = filterArgs.value<bool>(k_AdaptiveSurface_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<CleaveTetVolumeMeshAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> CleaveTetVolumeMesh::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSignedDistanceArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SignedDistanceArrayPaths_Key);
  auto pLipschitzValue = filterArgs.value<float32>(k_Lipschitz_Key);
  auto pScaleValue = filterArgs.value<float32>(k_Scale_Key);
  auto pMultiplierValue = filterArgs.value<float32>(k_Multiplier_Key);
  auto pAlphaValue = filterArgs.value<float32>(k_Alpha_Key);
  auto pPaddingValue = filterArgs.value<int32>(k_Padding_Key);
  auto pAdaptiveSurfaceValue = filterArgs.value<bool>(k_AdaptiveSurface_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
