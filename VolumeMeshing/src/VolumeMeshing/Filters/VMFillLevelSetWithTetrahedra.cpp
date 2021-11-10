#include "VMFillLevelSetWithTetrahedra.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string VMFillLevelSetWithTetrahedra::name() const
{
  return FilterTraits<VMFillLevelSetWithTetrahedra>::name.str();
}

//------------------------------------------------------------------------------
std::string VMFillLevelSetWithTetrahedra::className() const
{
  return FilterTraits<VMFillLevelSetWithTetrahedra>::className;
}

//------------------------------------------------------------------------------
Uuid VMFillLevelSetWithTetrahedra::uuid() const
{
  return FilterTraits<VMFillLevelSetWithTetrahedra>::uuid;
}

//------------------------------------------------------------------------------
std::string VMFillLevelSetWithTetrahedra::humanName() const
{
  return "Fill Level Set with Tetrahedra (VolumeMeshing)";
}

//------------------------------------------------------------------------------
std::vector<std::string> VMFillLevelSetWithTetrahedra::defaultTags() const
{
  return {"#Volume Meshing", "#Generation"};
}

//------------------------------------------------------------------------------
Parameters VMFillLevelSetWithTetrahedra::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_AutoDetectFeatures_Key, "Auto Detect Features", "", false));
  params.insert(std::make_unique<Float64Parameter>(k_AngleThreshold_Key, "Angle Threshold", "", 2.3456789));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_TriangleDataContainerName_Key, "Triangle Data Container", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_LevelSetArrayPath_Key, "Level Set", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_TetrahedralDataContainerName_Key, "Tetrahedral Data Container", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Vertex Data"});
  params.insert(std::make_unique<StringParameter>(k_VertexAttributeMatrixName_Key, "Vertex Attribute Matrix", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_TetFeatureIdsName_Key, "Feature Ids", "", "SomeString"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_AutoDetectFeatures_Key, k_TriangleDataContainerName_Key, true);
  params.linkParameters(k_AutoDetectFeatures_Key, k_AngleThreshold_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer VMFillLevelSetWithTetrahedra::clone() const
{
  return std::make_unique<VMFillLevelSetWithTetrahedra>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult VMFillLevelSetWithTetrahedra::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pAutoDetectFeaturesValue = filterArgs.value<bool>(k_AutoDetectFeatures_Key);
  auto pAngleThresholdValue = filterArgs.value<float64>(k_AngleThreshold_Key);
  auto pTriangleDataContainerNameValue = filterArgs.value<DataPath>(k_TriangleDataContainerName_Key);
  auto pLevelSetArrayPathValue = filterArgs.value<DataPath>(k_LevelSetArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pTetrahedralDataContainerNameValue = filterArgs.value<DataPath>(k_TetrahedralDataContainerName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  auto pTetFeatureIdsNameValue = filterArgs.value<StringParameter::ValueType>(k_TetFeatureIdsName_Key);

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
  auto action = std::make_unique<VMFillLevelSetWithTetrahedraAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> VMFillLevelSetWithTetrahedra::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pAutoDetectFeaturesValue = filterArgs.value<bool>(k_AutoDetectFeatures_Key);
  auto pAngleThresholdValue = filterArgs.value<float64>(k_AngleThreshold_Key);
  auto pTriangleDataContainerNameValue = filterArgs.value<DataPath>(k_TriangleDataContainerName_Key);
  auto pLevelSetArrayPathValue = filterArgs.value<DataPath>(k_LevelSetArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pTetrahedralDataContainerNameValue = filterArgs.value<DataPath>(k_TetrahedralDataContainerName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  auto pTetFeatureIdsNameValue = filterArgs.value<StringParameter::ValueType>(k_TetFeatureIdsName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
