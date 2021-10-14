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
std::string VMFillLevelSetWithTetrahedra::name() const
{
  return FilterTraits<VMFillLevelSetWithTetrahedra>::name.str();
}

std::string VMFillLevelSetWithTetrahedra::className() const
{
  return FilterTraits<VMFillLevelSetWithTetrahedra>::className;
}

Uuid VMFillLevelSetWithTetrahedra::uuid() const
{
  return FilterTraits<VMFillLevelSetWithTetrahedra>::uuid;
}

std::string VMFillLevelSetWithTetrahedra::humanName() const
{
  return "Fill Level Set with Tetrahedra (VolumeMeshing)";
}

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

IFilter::UniquePointer VMFillLevelSetWithTetrahedra::clone() const
{
  return std::make_unique<VMFillLevelSetWithTetrahedra>();
}

Result<OutputActions> VMFillLevelSetWithTetrahedra::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
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

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<VMFillLevelSetWithTetrahedraAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> VMFillLevelSetWithTetrahedra::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
