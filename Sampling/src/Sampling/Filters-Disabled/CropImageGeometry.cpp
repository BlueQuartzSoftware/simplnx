#include "CropImageGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/PreflightUpdatedValueFilterParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string CropImageGeometry::name() const
{
  return FilterTraits<CropImageGeometry>::name.str();
}

//------------------------------------------------------------------------------
std::string CropImageGeometry::className() const
{
  return FilterTraits<CropImageGeometry>::className;
}

//------------------------------------------------------------------------------
Uuid CropImageGeometry::uuid() const
{
  return FilterTraits<CropImageGeometry>::uuid;
}

//------------------------------------------------------------------------------
std::string CropImageGeometry::humanName() const
{
  return "Crop Geometry (Image)";
}

//------------------------------------------------------------------------------
std::vector<std::string> CropImageGeometry::defaultTags() const
{
  return {"#Sampling", "#Croping Cutting"};
}

//------------------------------------------------------------------------------
Parameters CropImageGeometry::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_XMin_Key, "X Min Voxel (Column) ", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_YMin_Key, "Y Min Voxel (Row)", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_ZMin_Key, "Z Min Voxel (Plane)", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_XMax_Key, "X Max Voxel (Column) [Inclusive]", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_YMax_Key, "Y Max Voxel (Row) [Inclusive]", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_ZMax_Key, "Z Max Voxel (Plane) [Inclusive]", "", 1234356));
  /*[x]*/ params.insert(std::make_unique<PreflightUpdatedValueFilterParameter>(k_OldBoxDimensions_Key, "Old Box Size in Length Units", "", {}));
  /*[x]*/ params.insert(std::make_unique<PreflightUpdatedValueFilterParameter>(k_NewBoxDimensions_Key, "New Box Size in Length Units", "", {}));
  params.insert(std::make_unique<BoolParameter>(k_UpdateOrigin_Key, "Update Origin", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SaveAsNewDataContainer_Key, "Save As New Data Container", "", false));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewDataContainerName_Key, "Data Container", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellAttributeMatrixPath_Key, "Cell Attribute Matrix", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Renumber Features Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RenumberFeatures_Key, "Renumber Features", "", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Cell Feature Attribute Matrix", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_SaveAsNewDataContainer_Key, k_NewDataContainerName_Key, true);
  params.linkParameters(k_RenumberFeatures_Key, k_CellFeatureAttributeMatrixPath_Key, true);
  params.linkParameters(k_RenumberFeatures_Key, k_FeatureIdsArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CropImageGeometry::clone() const
{
  return std::make_unique<CropImageGeometry>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CropImageGeometry::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pXMinValue = filterArgs.value<int32>(k_XMin_Key);
  auto pYMinValue = filterArgs.value<int32>(k_YMin_Key);
  auto pZMinValue = filterArgs.value<int32>(k_ZMin_Key);
  auto pXMaxValue = filterArgs.value<int32>(k_XMax_Key);
  auto pYMaxValue = filterArgs.value<int32>(k_YMax_Key);
  auto pZMaxValue = filterArgs.value<int32>(k_ZMax_Key);
  auto pOldBoxDimensionsValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_OldBoxDimensions_Key);
  auto pNewBoxDimensionsValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_NewBoxDimensions_Key);
  auto pUpdateOriginValue = filterArgs.value<bool>(k_UpdateOrigin_Key);
  auto pSaveAsNewDataContainerValue = filterArgs.value<bool>(k_SaveAsNewDataContainer_Key);
  auto pNewDataContainerNameValue = filterArgs.value<DataPath>(k_NewDataContainerName_Key);
  auto pCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);
  auto pRenumberFeaturesValue = filterArgs.value<bool>(k_RenumberFeatures_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<CropImageGeometryAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> CropImageGeometry::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pXMinValue = filterArgs.value<int32>(k_XMin_Key);
  auto pYMinValue = filterArgs.value<int32>(k_YMin_Key);
  auto pZMinValue = filterArgs.value<int32>(k_ZMin_Key);
  auto pXMaxValue = filterArgs.value<int32>(k_XMax_Key);
  auto pYMaxValue = filterArgs.value<int32>(k_YMax_Key);
  auto pZMaxValue = filterArgs.value<int32>(k_ZMax_Key);
  auto pOldBoxDimensionsValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_OldBoxDimensions_Key);
  auto pNewBoxDimensionsValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_NewBoxDimensions_Key);
  auto pUpdateOriginValue = filterArgs.value<bool>(k_UpdateOrigin_Key);
  auto pSaveAsNewDataContainerValue = filterArgs.value<bool>(k_SaveAsNewDataContainer_Key);
  auto pNewDataContainerNameValue = filterArgs.value<DataPath>(k_NewDataContainerName_Key);
  auto pCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);
  auto pRenumberFeaturesValue = filterArgs.value<bool>(k_RenumberFeatures_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
