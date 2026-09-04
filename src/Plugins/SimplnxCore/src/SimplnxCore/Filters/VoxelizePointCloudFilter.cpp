#include "VoxelizePointCloudFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/VoxelizePointCloud.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"

using namespace nx::core;
namespace PU = nx::core::PartitionUtilities;
namespace PUP = nx::core::PartitionUtilities::Parameters;
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
  return {className(), "Core", "Voxelize", "Point Cloud", "Image Geometry", "Conversion", "Mapping"};
}

//------------------------------------------------------------------------------
Parameters VoxelizePointCloudFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputPointCloudGeometryPath_Key, "Target Point Cloud Geometry", "The selected Node-Based geometry that contains the point cloud",
                                                             DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex, IGeometry::Type::Edge, IGeometry::Type::Triangle, IGeometry::Type::Quad,
                                                                                                      IGeometry::Type::Tetrahedral, IGeometry::Type::Hexahedral}));

  params.insertSeparator(Parameters::Separator{"Output Grid Parameters"});
  PU::AppendPartitioningParameters(params);
  params.insert(std::make_unique<GeometrySelectionParameter>(PUP::k_ExistingPartitionGridPath_Key, "Destination Grid Geometry",
                                                             "The destination grid geometry (cell data) that is the location for the voxel mask.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image, IGeometry::Type::RectGrid}));

  params.insertSeparator(Parameters::Separator{"Output Data Object(s)"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_MaskArrayName_Key, "Voxel Mask Name", "Name of the array containing a mask of active voxels in the overlapped Geometries", "Shared Voxels Mask"));
  params.insert(
      std::make_unique<DataGroupCreationParameter>(k_CreatedImageGeometryPath_Key, "New Image Geometry", "The path to the new geometry that will wrap the point cloud", DataPath({"Image Geometry"})));

  params.linkParameters(PUP::k_PartitioningMode_Key, PUP::k_ExistingPartitionGridPath_Key, std::make_any<ChoicesParameter::ValueType>(PUP::k_ExistingSchemeModeIndex));
  params.linkParameters(PUP::k_PartitioningMode_Key, k_CreatedImageGeometryPath_Key, std::make_any<ChoicesParameter::ValueType>(PUP::k_BasicModeIndex));
  params.linkParameters(PUP::k_PartitioningMode_Key, k_CreatedImageGeometryPath_Key, std::make_any<ChoicesParameter::ValueType>(PUP::k_AdvancedModeIndex));
  params.linkParameters(PUP::k_PartitioningMode_Key, k_CreatedImageGeometryPath_Key, std::make_any<ChoicesParameter::ValueType>(PUP::k_BoundingBoxModeIndex));

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
  const auto pPartitioningModeValue = filterArgs.value<ChoicesParameter::ValueType>(PUP::k_PartitioningMode_Key);
  const auto pPointCloudGeometryPathValue = filterArgs.value<GeometrySelectionParameter::ValueType>(k_InputPointCloudGeometryPath_Key);
  const auto pOutputGeometryPathValue = filterArgs.value<GeometrySelectionParameter::ValueType>(PUP::k_ExistingPartitionGridPath_Key);
  const auto pMaskNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_MaskArrayName_Key);
  const auto pNewGeometryPathValue = filterArgs.value<DataGroupCreationParameter::ValueType>(k_CreatedImageGeometryPath_Key);

  Result<OutputActions> resultOutputActions;

  const auto& pointCloudGeom = dataStructure.getDataRefAs<INodeGeometry0D>(pPointCloudGeometryPathValue);
  if(pointCloudGeom.getVertices() == nullptr)
  {
    return MakePreflightErrorResult(-45985, fmt::format("The selected point cloud geometry '{}' does not have a shared vertex list assigned.", pPointCloudGeometryPathValue.toString()));
  }
  if(pointCloudGeom.getVertices()->getNumberOfComponents() != 3)
  {
    return MakePreflightErrorResult(-45989, fmt::format("The vertex list of '{}' has {} component(s) per vertex but exactly 3 (X, Y, Z) are required.", pPointCloudGeometryPathValue.toString(),
                                                        pointCloudGeom.getVertices()->getNumberOfComponents()));
  }

  DataPath maskParent{};
  ShapeType maskDims{1, 1, 1};

  const auto partitioningMode = static_cast<PU::PartitioningMode>(pPartitioningModeValue);

  if(partitioningMode == PU::PartitioningMode::ExistingPartitionGrid)
  {
    const auto& destGeometry = dataStructure.getDataRefAs<IGridGeometry>(pOutputGeometryPathValue);

    if(const auto* imageGeom = dynamic_cast<const ImageGeom*>(&destGeometry); imageGeom != nullptr)
    {
      const FloatVec3 spacing = imageGeom->getSpacing();
      if(!(spacing[0] > 0.0f && spacing[1] > 0.0f && spacing[2] > 0.0f))
      {
        return MakePreflightErrorResult(-45983,
                                        fmt::format("The selected Image Geometry has invalid spacing ({}, {}, {}). All spacing values must be greater than zero.", spacing[0], spacing[1], spacing[2]));
      }
    }

    if(destGeometry.getCellData() == nullptr)
    {
      return MakePreflightErrorResult(-45984, fmt::format("The selected geometry '{}' does not have a cell attribute matrix assigned.", pOutputGeometryPathValue.toString()));
    }

    if(const auto* rectGrid = dynamic_cast<const RectGridGeom*>(&destGeometry); rectGrid != nullptr)
    {
      if(rectGrid->getXBounds() == nullptr || rectGrid->getYBounds() == nullptr || rectGrid->getZBounds() == nullptr)
      {
        return MakePreflightErrorResult(-45986, fmt::format("The selected RectGrid Geometry '{}' is missing one or more bounds arrays (X, Y, or Z).", pOutputGeometryPathValue.toString()));
      }

      const SizeVec3 rectDims = rectGrid->getDimensions();
      if(rectGrid->getXBounds()->getNumberOfTuples() != rectDims[0] + 1 || rectGrid->getYBounds()->getNumberOfTuples() != rectDims[1] + 1 ||
         rectGrid->getZBounds()->getNumberOfTuples() != rectDims[2] + 1)
      {
        return MakePreflightErrorResult(-45987, fmt::format("The selected RectGrid Geometry '{}' has bounds arrays whose sizes do not match its dimensions. "
                                                            "Expected X/Y/Z bounds sizes of {}/{}/{} (dims + 1) but got {}/{}/{}.",
                                                            pOutputGeometryPathValue.toString(), rectDims[0] + 1, rectDims[1] + 1, rectDims[2] + 1, rectGrid->getXBounds()->getNumberOfTuples(),
                                                            rectGrid->getYBounds()->getNumberOfTuples(), rectGrid->getZBounds()->getNumberOfTuples()));
      }
    }

    maskParent = destGeometry.getCellDataPath();
    maskDims = destGeometry.getCellDataRef().getShape();
  }
  else
  {
    const auto pNumberOfCellsPerAxisValue = filterArgs.value<VectorInt32Parameter::ValueType>(PUP::k_NumberOfCellsPerAxis_Key);
    const auto pCellLengthValue = filterArgs.value<VectorFloat32Parameter::ValueType>(PUP::k_CellLength_Key);
    const auto pMinGridCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(PUP::k_MinGridCoord_Key);
    const auto pMaxGridCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(PUP::k_MaxGridCoord_Key);

    const SizeVec3 numberOfPartitionsPerAxis = {static_cast<usize>(pNumberOfCellsPerAxisValue[0]), static_cast<usize>(pNumberOfCellsPerAxisValue[1]),
                                                static_cast<usize>(pNumberOfCellsPerAxisValue[2])};

    Result<> result = PU::DataCheckNumberOfPartitions(numberOfPartitionsPerAxis);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(result), {})};
    }

    switch(partitioningMode)
    {
    case PU::PartitioningMode::Advanced: {
      result = PU::DataCheckCellLength(humanName(), FloatVec3(pCellLengthValue[0], pCellLengthValue[1], pCellLengthValue[2]));
      if(result.invalid())
      {
        return {ConvertResultTo<OutputActions>(std::move(result), {})};
      }
      break;
    }
    case PU::PartitioningMode::BoundingBox: {
      result = PU::DataCheckBoundingBoxCoords(humanName(), FloatVec3(pMinGridCoordValue[0], pMinGridCoordValue[1], pMinGridCoordValue[2]),
                                              FloatVec3(pMaxGridCoordValue[0], pMaxGridCoordValue[1], pMaxGridCoordValue[2]));
      if(result.invalid())
      {
        return {ConvertResultTo<OutputActions>(std::move(result), {})};
      }
      break;
    }
    default:
      break;
    }

    const Result<PU::PSGeomInfo> psInfo = PU::GeneratePartitioningSchemeInfo(pointCloudGeom, dataStructure, filterArgs);

    std::vector<usize> dims{1, 1, 1};
    std::vector<float32> origin{0.0f, 0.0f, 0.0f};
    std::vector<float32> spacing{1.0f, 1.0f, 1.0f};
    if(psInfo.valid())
    {
      const auto& psMetadata = psInfo.value();
      dims = {psMetadata.geometryDims[0], psMetadata.geometryDims[1], psMetadata.geometryDims[2]};
      if(psMetadata.geometryOrigin.has_value())
      {
        origin = {(*psMetadata.geometryOrigin)[0], (*psMetadata.geometryOrigin)[1], (*psMetadata.geometryOrigin)[2]};
      }
      if(psMetadata.geometrySpacing.has_value())
      {
        spacing = {(*psMetadata.geometrySpacing)[0], (*psMetadata.geometrySpacing)[1], (*psMetadata.geometrySpacing)[2]};
      }
      maskDims = {psMetadata.geometryDims[2], psMetadata.geometryDims[1], psMetadata.geometryDims[0]};
    }

    auto createGeomAction = std::make_unique<CreateImageGeometryAction>(pNewGeometryPathValue, dims, origin, spacing, ImageGeom::k_CellAttributeMatrixName);
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

  inputValues.PartitioningMode = filterArgs.value<ChoicesParameter::ValueType>(PUP::k_PartitioningMode_Key);
  inputValues.PointCloudGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(k_InputPointCloudGeometryPath_Key);
  inputValues.NumberOfCellsPerAxis = filterArgs.value<VectorInt32Parameter::ValueType>(PUP::k_NumberOfCellsPerAxis_Key);
  inputValues.PartitionGridOrigin = filterArgs.value<VectorFloat32Parameter::ValueType>(PUP::k_PartitionGridOrigin_Key);
  inputValues.CellLength = filterArgs.value<VectorFloat32Parameter::ValueType>(PUP::k_CellLength_Key);
  inputValues.MinGridCoord = filterArgs.value<VectorFloat32Parameter::ValueType>(PUP::k_MinGridCoord_Key);
  inputValues.MaxGridCoord = filterArgs.value<VectorFloat32Parameter::ValueType>(PUP::k_MaxGridCoord_Key);
  inputValues.OutputGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(PUP::k_ExistingPartitionGridPath_Key);
  inputValues.MaskName = filterArgs.value<DataObjectNameParameter::ValueType>(k_MaskArrayName_Key);
  inputValues.NewGeometryPath = filterArgs.value<DataGroupCreationParameter::ValueType>(k_CreatedImageGeometryPath_Key);

  return VoxelizePointCloud(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
