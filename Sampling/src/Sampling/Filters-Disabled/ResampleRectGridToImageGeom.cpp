#include "ResampleRectGridToImageGeom.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/PreflightUpdatedValueFilterParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ResampleRectGridToImageGeom::name() const
{
  return FilterTraits<ResampleRectGridToImageGeom>::name.str();
}

//------------------------------------------------------------------------------
std::string ResampleRectGridToImageGeom::className() const
{
  return FilterTraits<ResampleRectGridToImageGeom>::className;
}

//------------------------------------------------------------------------------
Uuid ResampleRectGridToImageGeom::uuid() const
{
  return FilterTraits<ResampleRectGridToImageGeom>::uuid;
}

//------------------------------------------------------------------------------
std::string ResampleRectGridToImageGeom::humanName() const
{
  return "Resample Rectilinear Grid to Image Geom";
}

//------------------------------------------------------------------------------
std::vector<std::string> ResampleRectGridToImageGeom::defaultTags() const
{
  return {"#Unsupported", "#Sampling"};
}

//------------------------------------------------------------------------------
Parameters ResampleRectGridToImageGeom::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_RectilinearGridPath_Key, "Input Rectilinear Grid", "", DataPath{}));
  params.insert(
      std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Attribute Arrays to Copy", "", MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}));
  params.insert(std::make_unique<PreflightUpdatedValueFilterParameter>(k_RectGridGeometryDesc_Key, "Rect Grid Geom Info.", "", {}));
  params.insertSeparator(Parameters::Separator{"Output Image Geometry Parameters"});
  params.insert(std::make_unique<VectorInt32Parameter>(k_Dimensions_Key, "Dimensions (Voxels)", "", std::vector<int32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<PreflightUpdatedValueFilterParameter>(k_CreatedGeometryDescription_Key, "Created Volume Info.", "", {}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_ImageGeometryPath_Key, "Data Container", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_ImageGeomCellAttributeMatrix_Key, "Cell Attribute Matrix", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ResampleRectGridToImageGeom::clone() const
{
  return std::make_unique<ResampleRectGridToImageGeom>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ResampleRectGridToImageGeom::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pRectilinearGridPathValue = filterArgs.value<DataPath>(k_RectilinearGridPath_Key);
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto pRectGridGeometryDescValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_RectGridGeometryDesc_Key);
  auto pDimensionsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_Dimensions_Key);
  auto pCreatedGeometryDescriptionValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_CreatedGeometryDescription_Key);
  auto pImageGeometryPathValue = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto pImageGeomCellAttributeMatrixValue = filterArgs.value<DataPath>(k_ImageGeomCellAttributeMatrix_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ResampleRectGridToImageGeomAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ResampleRectGridToImageGeom::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pRectilinearGridPathValue = filterArgs.value<DataPath>(k_RectilinearGridPath_Key);
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto pRectGridGeometryDescValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_RectGridGeometryDesc_Key);
  auto pDimensionsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_Dimensions_Key);
  auto pCreatedGeometryDescriptionValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_CreatedGeometryDescription_Key);
  auto pImageGeometryPathValue = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto pImageGeomCellAttributeMatrixValue = filterArgs.value<DataPath>(k_ImageGeomCellAttributeMatrix_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
