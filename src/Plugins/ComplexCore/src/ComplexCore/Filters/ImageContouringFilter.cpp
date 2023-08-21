#include "ImageContouringFilter.hpp"

#include "ComplexCore/Filters/Algorithms/ImageContouring.hpp"

#include "complex/Common/Types.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateGeometry2DAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;
namespace
{
const std::string k_VertexNormals = "VertexNormals";
}

namespace complex
{
//------------------------------------------------------------------------------
std::string ImageContouringFilter::name() const
{
  return FilterTraits<ImageContouringFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ImageContouringFilter::className() const
{
  return FilterTraits<ImageContouringFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ImageContouringFilter::uuid() const
{
  return FilterTraits<ImageContouringFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ImageContouringFilter::humanName() const
{
  return "Contour";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImageContouringFilter::defaultTags() const
{
  return {className(), "Contouring", "Image Geometry"};
}

//------------------------------------------------------------------------------
Parameters ImageContouringFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_IsoVal_Key, "Contour Value", "The value to contour on", 1.0));

  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(
      std::make_unique<ArraySelectionParameter>(k_SelectedDataArray_Key, "Data Array to Contour", "This is the data that will be checked for the contouring iso value", DataPath{}, GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Objects"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_NewTriangleGeometryName_Key, "Name of Output Triangle Geometry", "This is where the contouring line will be stored", "Contouring Geometry"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ImageContouringFilter::clone() const
{
  return std::make_unique<ImageContouringFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ImageContouringFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel) const
{
  auto pImageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);
  auto pTriangleGeomName = filterArgs.value<std::string>(k_NewTriangleGeometryName_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Create the Triangle Geometry action and store it
  auto createTriangleGeometryAction =
      std::make_unique<CreateTriangleGeometryAction>(DataPath({pTriangleGeomName}), static_cast<usize>(1), static_cast<usize>(1), INodeGeometry0D::k_VertexDataName, INodeGeometry2D::k_FaceDataName,
                                                     CreateTriangleGeometryAction::k_DefaultVerticesName, CreateTriangleGeometryAction::k_DefaultFacesName);
  auto vertexNormalsPath = createTriangleGeometryAction->getVertexDataPath().createChildPath(k_VertexNormals);
  resultOutputActions.value().appendAction(std::move(createTriangleGeometryAction));

  // Create the face Normals DataArray action and store it
  auto createArrayAction = std::make_unique<CreateArrayAction>(complex::DataType::float32, std::vector<usize>{static_cast<usize>(1)}, std::vector<usize>{static_cast<usize>(3)}, vertexNormalsPath);
  resultOutputActions.value().appendAction(std::move(createArrayAction));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ImageContouringFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel) const
{
  ImageContouringInputValues inputValues;

  inputValues.imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);
  inputValues.contouringArrayPath = filterArgs.value<DataPath>(k_SelectedDataArray_Key);
  inputValues.triangleGeomPath = DataPath({filterArgs.value<std::string>(k_NewTriangleGeometryName_Key)});
  inputValues.isoVal = filterArgs.value<float64>(k_IsoVal_Key);
  inputValues.normalsArrayPath = inputValues.triangleGeomPath.createChildPath(INodeGeometry0D::k_VertexDataName).createChildPath(k_VertexNormals);

  return ImageContouring(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
