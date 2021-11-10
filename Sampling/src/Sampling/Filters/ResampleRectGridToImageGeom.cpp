#include "ResampleRectGridToImageGeom.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
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
  params.insertSeparator(Parameters::Separator{"Output Image Geometry Parameters"});
  params.insert(std::make_unique<VectorInt32Parameter>(k_Dimensions_Key, "Dimensions (Voxels)", "", std::vector<int32>(3), std::vector<std::string>(3)));
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
IFilter::PreflightResult ResampleRectGridToImageGeom::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pRectilinearGridPathValue = filterArgs.value<DataPath>(k_RectilinearGridPath_Key);
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto pDimensionsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_Dimensions_Key);
  auto pImageGeometryPathValue = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto pImageGeomCellAttributeMatrixValue = filterArgs.value<DataPath>(k_ImageGeomCellAttributeMatrix_Key);

  // These variables should be updated with the latest data generated for each variable during preflight.
  // These will be returned through the preflightResult variable to the
  // user interface. You could make these member variables instead if needed.
  std::string rectGridGeometryDesc;
  std::string createdGeometryDescription;

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
  auto action = std::make_unique<ResampleRectGridToImageGeomAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  // These values should have been updated during the preflightImpl(...) method
  preflightResult.outputValues.push_back({"RectGridGeometryDesc", rectGridGeometryDesc});
  preflightResult.outputValues.push_back({"CreatedGeometryDescription", createdGeometryDescription});

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ResampleRectGridToImageGeom::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pRectilinearGridPathValue = filterArgs.value<DataPath>(k_RectilinearGridPath_Key);
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto pDimensionsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_Dimensions_Key);
  auto pImageGeometryPathValue = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto pImageGeomCellAttributeMatrixValue = filterArgs.value<DataPath>(k_ImageGeomCellAttributeMatrix_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
