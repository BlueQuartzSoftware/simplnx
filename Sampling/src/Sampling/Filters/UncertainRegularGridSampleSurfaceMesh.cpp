#include "UncertainRegularGridSampleSurfaceMesh.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string UncertainRegularGridSampleSurfaceMesh::name() const
{
  return FilterTraits<UncertainRegularGridSampleSurfaceMesh>::name.str();
}

//------------------------------------------------------------------------------
std::string UncertainRegularGridSampleSurfaceMesh::className() const
{
  return FilterTraits<UncertainRegularGridSampleSurfaceMesh>::className;
}

//------------------------------------------------------------------------------
Uuid UncertainRegularGridSampleSurfaceMesh::uuid() const
{
  return FilterTraits<UncertainRegularGridSampleSurfaceMesh>::uuid;
}

//------------------------------------------------------------------------------
std::string UncertainRegularGridSampleSurfaceMesh::humanName() const
{
  return "Sample Triangle Geometry on Uncertain Regular Grid";
}

//------------------------------------------------------------------------------
std::vector<std::string> UncertainRegularGridSampleSurfaceMesh::defaultTags() const
{
  return {"#Sampling", "#Spacing"};
}

//------------------------------------------------------------------------------
Parameters UncertainRegularGridSampleSurfaceMesh::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "", DataPath{}));
  params.insert(std::make_unique<Int32Parameter>(k_XPoints_Key, "X Points", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_YPoints_Key, "Y Points", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_ZPoints_Key, "Z Points", "", 1234356));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Uncertainty_Key, "Uncertainty", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataContainerName_Key, "Data Container", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureIdsArrayName_Key, "Feature Ids", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer UncertainRegularGridSampleSurfaceMesh::clone() const
{
  return std::make_unique<UncertainRegularGridSampleSurfaceMesh>();
}

//------------------------------------------------------------------------------
Result<OutputActions> UncertainRegularGridSampleSurfaceMesh::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pXPointsValue = filterArgs.value<int32>(k_XPoints_Key);
  auto pYPointsValue = filterArgs.value<int32>(k_YPoints_Key);
  auto pZPointsValue = filterArgs.value<int32>(k_ZPoints_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pUncertaintyValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Uncertainty_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<UncertainRegularGridSampleSurfaceMeshAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> UncertainRegularGridSampleSurfaceMesh::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pXPointsValue = filterArgs.value<int32>(k_XPoints_Key);
  auto pYPointsValue = filterArgs.value<int32>(k_YPoints_Key);
  auto pZPointsValue = filterArgs.value<int32>(k_ZPoints_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pUncertaintyValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Uncertainty_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
