#include "FlyingEdges3DFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/FlyingEdges3D.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry2DAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

using namespace nx::core;
namespace
{
const std::string k_VertexNormals = "VertexNormals";
}

namespace nx::core
{
//------------------------------------------------------------------------------
std::string FlyingEdges3DFilter::name() const
{
  return FilterTraits<FlyingEdges3DFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FlyingEdges3DFilter::className() const
{
  return FilterTraits<FlyingEdges3DFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FlyingEdges3DFilter::uuid() const
{
  return FilterTraits<FlyingEdges3DFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FlyingEdges3DFilter::humanName() const
{
  return "Surface Contour Filter (Flying Edges 3D)";
}

//------------------------------------------------------------------------------
std::vector<std::string> FlyingEdges3DFilter::defaultTags() const
{
  return {className(), "FlyingEdges3D", "Contouring", "Image Geometry", "Flying Edges 3D", "ISO Contour"};
}

//------------------------------------------------------------------------------
Parameters FlyingEdges3DFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_IsoVal_Key, "Contour Value", "The value to contour on", 1.0));

  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedDataArrayPath_Key, "Data Array to Contour", "This is the data that will be checked for the contouring iso value", DataPath{},
                                                          GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Objects"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_NewTriangleGeometryName_Key, "Name of Output Triangle Geometry", "This is where the contouring line will be stored", "Contouring Geometry"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FlyingEdges3DFilter::clone() const
{
  return std::make_unique<FlyingEdges3DFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FlyingEdges3DFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel) const
{
  auto pImageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);
  auto pTriangleGeomName = filterArgs.value<std::string>(k_NewTriangleGeometryName_Key);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Create the Triangle Geometry action and store it
  auto createTriangleGeometryAction =
      std::make_unique<CreateTriangleGeometryAction>(DataPath({pTriangleGeomName}), static_cast<usize>(1), static_cast<usize>(1), INodeGeometry0D::k_VertexDataName, INodeGeometry2D::k_FaceDataName,
                                                     CreateTriangleGeometryAction::k_DefaultVerticesName, CreateTriangleGeometryAction::k_DefaultFacesName);
  auto vertexNormalsPath = createTriangleGeometryAction->getVertexDataPath().createChildPath(k_VertexNormals);
  resultOutputActions.value().appendAction(std::move(createTriangleGeometryAction));

  // Create the face Normals DataArray action and store it
  auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, std::vector<usize>{static_cast<usize>(1)}, std::vector<usize>{static_cast<usize>(3)}, vertexNormalsPath);
  resultOutputActions.value().appendAction(std::move(createArrayAction));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FlyingEdges3DFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                          const std::atomic_bool& shouldCancel) const
{
  FlyingEdges3DInputValues inputValues;

  inputValues.imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);
  inputValues.contouringArrayPath = filterArgs.value<DataPath>(k_SelectedDataArrayPath_Key);
  inputValues.triangleGeomPath = DataPath({filterArgs.value<std::string>(k_NewTriangleGeometryName_Key)});
  inputValues.isoVal = filterArgs.value<float64>(k_IsoVal_Key);
  inputValues.normalsArrayPath = inputValues.triangleGeomPath.createChildPath(INodeGeometry0D::k_VertexDataName).createChildPath(k_VertexNormals);

  return FlyingEdges3D(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
