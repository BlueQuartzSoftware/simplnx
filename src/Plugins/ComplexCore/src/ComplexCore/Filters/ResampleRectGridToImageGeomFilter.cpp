#include "ResampleRectGridToImageGeomFilter.hpp"

#include "ComplexCore/Filters/Algorithms/ResampleRectGridToImageGeom.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ResampleRectGridToImageGeomFilter::name() const
{
  return FilterTraits<ResampleRectGridToImageGeomFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ResampleRectGridToImageGeomFilter::className() const
{
  return FilterTraits<ResampleRectGridToImageGeomFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ResampleRectGridToImageGeomFilter::uuid() const
{
  return FilterTraits<ResampleRectGridToImageGeomFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ResampleRectGridToImageGeomFilter::humanName() const
{
  return "Resample Rectilinear Grid to Image Geom";
}

//------------------------------------------------------------------------------
std::vector<std::string> ResampleRectGridToImageGeomFilter::defaultTags() const
{
  return {"Unsupported", "Sampling"};
}

//------------------------------------------------------------------------------
Parameters ResampleRectGridToImageGeomFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Geometry Parameters"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_RectilinearGridPath_Key, "Input Rectilinear Grid", "Path to the RectGrid Geometry to be re-sampled", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::RectGrid}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(
      k_SelectedDataArrayPaths_Key, "Attribute Arrays to Copy", "Rectilinear Grid Cell Data to possibly copy", MultiArraySelectionParameter::ValueType{},
      MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray, IArray::ArrayType::StringArray, IArray::ArrayType::NeighborListArray}, GetAllDataTypes()));
  params.insertSeparator(Parameters::Separator{"Output Image Geometry Parameters"});
  params.insert(std::make_unique<VectorInt32Parameter>(k_Dimensions_Key, "Dimensions (Voxels)", "The image geometry voxel dimensions in which to re-sample the rectilinear grid geometry",
                                                       std::vector<int32>{128, 128, 128}, std::vector<std::string>{"x", "y", "z"}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_ImageGeometryPath_Key, "Created Image Geometry", "Path to the created Image Geometry", DataPath({"Image Geometry"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_ImageGeomCellAttributeMatrix_Key, "Cell Attribute Matrix", "The name of the cell data Attribute Matrix created with the Image Geometry",
                                                          ImageGeom::k_CellDataName));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ResampleRectGridToImageGeomFilter::clone() const
{
  return std::make_unique<ResampleRectGridToImageGeomFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ResampleRectGridToImageGeomFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                          const std::atomic_bool& shouldCancel) const
{
  auto pRectilinearGridPathValue = filterArgs.value<DataPath>(k_RectilinearGridPath_Key);
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto pDimensionsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_Dimensions_Key);
  auto pImageGeometryPathValue = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto pImageGeomCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_ImageGeomCellAttributeMatrix_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;
  std::string rectGridGeometryDesc;
  std::string createdGeometryDescription;

  //{
  //  auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pImageGeometryPathValue);
  //  resultOutputActions.value().actions.push_back(std::move(createDataGroupAction));
  //}

  preflightUpdatedValues.push_back({"RectGridGeometryDesc", rectGridGeometryDesc});
  preflightUpdatedValues.push_back({"CreatedGeometryDescription", createdGeometryDescription});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ResampleRectGridToImageGeomFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel) const
{
  ResampleRectGridToImageGeomInputValues inputValues;

  inputValues.RectilinearGridPath = filterArgs.value<DataPath>(k_RectilinearGridPath_Key);
  inputValues.SelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  inputValues.Dimensions = filterArgs.value<VectorInt32Parameter::ValueType>(k_Dimensions_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  inputValues.ImageGeomCellAttributeMatrixName = filterArgs.value<std::string>(k_ImageGeomCellAttributeMatrix_Key);

  return ResampleRectGridToImageGeom(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
