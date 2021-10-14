#include "ResampleImageGeom.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/PreflightUpdatedValueFilterParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
std::string ResampleImageGeom::name() const
{
  return FilterTraits<ResampleImageGeom>::name.str();
}

Uuid ResampleImageGeom::uuid() const
{
  return FilterTraits<ResampleImageGeom>::uuid;
}

std::string ResampleImageGeom::humanName() const
{
  return "Resample Data (Image Geometry)";
}

Parameters ResampleImageGeom::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<PreflightUpdatedValueFilterParameter>(k_CurrentGeomtryInfo_Key, "Current Image Geometry", "", {}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<PreflightUpdatedValueFilterParameter>(k_NewGeomtryInfo_Key, "Created/Updated Image Geometry", "", {}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RenumberFeatures_Key, "Renumber Features", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SaveAsNewDataContainer_Key, "Save as New Data Container", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellAttributeMatrixPath_Key, "Cell Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Cell Feature Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewDataContainerPath_Key, "Data Container", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_RenumberFeatures_Key, k_CellFeatureAttributeMatrixPath_Key, true);
  params.linkParameters(k_RenumberFeatures_Key, k_FeatureIdsArrayPath_Key, true);
  params.linkParameters(k_SaveAsNewDataContainer_Key, k_NewDataContainerPath_Key, true);

  return params;
}

IFilter::UniquePointer ResampleImageGeom::clone() const
{
  return std::make_unique<ResampleImageGeom>();
}

Result<OutputActions> ResampleImageGeom::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pCurrentGeomtryInfoValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_CurrentGeomtryInfo_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pNewGeomtryInfoValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_NewGeomtryInfo_Key);
  auto pRenumberFeaturesValue = filterArgs.value<bool>(k_RenumberFeatures_Key);
  auto pSaveAsNewDataContainerValue = filterArgs.value<bool>(k_SaveAsNewDataContainer_Key);
  auto pCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  auto pNewDataContainerPathValue = filterArgs.value<DataPath>(k_NewDataContainerPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ResampleImageGeomAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ResampleImageGeom::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pCurrentGeomtryInfoValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_CurrentGeomtryInfo_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pNewGeomtryInfoValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_NewGeomtryInfo_Key);
  auto pRenumberFeaturesValue = filterArgs.value<bool>(k_RenumberFeatures_Key);
  auto pSaveAsNewDataContainerValue = filterArgs.value<bool>(k_SaveAsNewDataContainer_Key);
  auto pCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  auto pNewDataContainerPathValue = filterArgs.value<DataPath>(k_NewDataContainerPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
