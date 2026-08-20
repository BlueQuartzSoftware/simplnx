#include "VoxelizePointCloudFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/VoxelizePointCloud.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string VoxelizePointCloudFilter::name() const
{
  return FilterTraits<VoxelizePointCloudFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string VoxelizePointCloudFilter::className() const
{
  return FilterTraits<VoxelizePointCloudFilter>::className;
}

//------------------------------------------------------------------------------
Uuid VoxelizePointCloudFilter::uuid() const
{
  return FilterTraits<VoxelizePointCloudFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string VoxelizePointCloudFilter::humanName() const
{
  return "Voxelize Point Cloud";
}

//------------------------------------------------------------------------------
std::vector<std::string> VoxelizePointCloudFilter::defaultTags() const
{
  // TODO:
  //  - Think of more tags
  return {className(), "Core"};
}

//------------------------------------------------------------------------------
Parameters VoxelizePointCloudFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UseExistingGeom_Key, "Use Existing Grid Geometry", "If true use an existing grid geometry, else create a new Image Geometry to wrap the Point Cloud", false));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_PointCloudGeometryPath_Key, "Target Point Cloud Geometry", "The selected Node-Based geometry that contains the point cloud", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex, IGeometry::Type::Edge, IGeometry::Type::Triangle, IGeometry::Type::Quad,
                                                                                                      IGeometry::Type::Tetrahedral, IGeometry::Type::Hexahedral}));

  params.insertSeparator(Parameters::Separator{"Output Parameter(s)"});
  params.insert(std::make_unique<StringParameter>(k_MaskName_Key, "Voxel Mask Name", "Name of the array containing a mask of active voxels in the overlapped Geometries", "Shared Voxels Mask"));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_OutputGeometryPath_Key, "Destination Grid Geometry",
                                                             "The destination grid geometry (cell data) that is the location for the voxel mask.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image, IGeometry::Type::RectGrid}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewGeometryPath_Key, "New Image Geometry", "The path to the new geometry that will wrap the point cloud", DataPath({"Image Geometry"})));

  params.linkParameters(k_UseExistingGeom_Key, k_OutputGeometryPath_Key, true);
  params.linkParameters(k_UseExistingGeom_Key, k_NewGeometryPath_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType VoxelizePointCloudFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer VoxelizePointCloudFilter::clone() const
{
  return std::make_unique<VoxelizePointCloudFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult VoxelizePointCloudFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  const auto pUseExistingGeomValue = filterArgs.value<BoolParameter::ValueType>(k_UseExistingGeom_Key);
  const auto pPointCloudGeometryPathValue = filterArgs.value<GeometrySelectionParameter::ValueType>(k_PointCloudGeometryPath_Key);
  const auto pOutputGeometryPathValue = filterArgs.value<GeometrySelectionParameter::ValueType>(k_OutputGeometryPath_Key);
  const auto pMaskNameValue = filterArgs.value<StringParameter::ValueType>(k_MaskName_Key);
  const auto pNewGeometryPathValue = filterArgs.value<DataGroupCreationParameter::ValueType>(k_NewGeometryPath_Key);

  Result<OutputActions> resultOutputActions;

  DataPath maskParent{};
  ShapeType maskDims{1, 1, 1};

  if(pUseExistingGeomValue)
  {
    const auto& destGeometry = dataStructure.getDataRefAs<IGridGeometry>(pOutputGeometryPathValue);
    maskParent = destGeometry.getCellDataPath();

    const auto& cellData = dataStructure.getDataAs<AttributeMatrix>(maskParent);
    maskDims = cellData->getShape();
  }
  else
  {
    auto createGeomAction =
        std::make_unique<CreateImageGeometryAction>(pNewGeometryPathValue, CreateImageGeometryAction::DimensionType{1, 1, 1}, CreateImageGeometryAction::OriginType{0.0f, 0.0f, 0.0f},
                                                    CreateImageGeometryAction::SpacingType{1.0f, 1.0f, 1.0f}, ImageGeom::k_CellAttributeMatrixName);
    resultOutputActions.value().appendAction(std::move(createGeomAction));

    maskParent = pNewGeometryPathValue.createChildPath(ImageGeom::k_CellAttributeMatrixName);
  }

  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint8, maskDims, ShapeType{1}, maskParent.createChildPath(pMaskNameValue), CreateArrayAction::k_DefaultDataFormat, "0");
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> VoxelizePointCloudFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  VoxelizePointCloudInputValues inputValues;

  inputValues.UseExistingGeom = filterArgs.value<BoolParameter::ValueType>(k_UseExistingGeom_Key);
  inputValues.PointCloudGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(k_PointCloudGeometryPath_Key);
  inputValues.OutputGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(k_OutputGeometryPath_Key);
  inputValues.MaskName = filterArgs.value<StringParameter::ValueType>(k_MaskName_Key);
  inputValues.NewGeometryPath = filterArgs.value<DataGroupCreationParameter::ValueType>(k_NewGeometryPath_Key);

  return VoxelizePointCloud(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

// No SIMPL implementation
Result<Arguments> VoxelizePointCloudFilter::FromSIMPLJson(const nlohmann::json& json)
{
  return {VoxelizePointCloudFilter().getDefaultArguments()};
}
} // namespace nx::core
