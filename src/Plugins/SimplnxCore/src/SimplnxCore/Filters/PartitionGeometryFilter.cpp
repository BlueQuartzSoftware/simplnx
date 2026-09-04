#include "PartitionGeometryFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

using namespace nx::core;
namespace PU = nx::core::PartitionUtilities;
namespace PUP = nx::core::PartitionUtilities::Parameters;

namespace
{

/**
 * @brief Generates the display text that describes the input geometry,
 * shown as a preflight updated value in the user interface.
 * @param dims The dimensions of the input geometry
 * @param origin The origin of the input geometry
 * @param spacing The spacing of the input geometry
 * @param lengthUnits The length units of the input geometry
 * @return The text description of the current input geometry.
 */
std::string GenerateInputGeometryDisplayText(const SizeVec3& dims, const FloatVec3& origin, const FloatVec3& spacing, const IGeometry::LengthUnit& lengthUnits)
{
  std::string lengthUnitStr = IGeometry::LengthUnitToString(lengthUnits);
  if(lengthUnits == IGeometry::LengthUnit::Unspecified)
  {
    lengthUnitStr.append(" Units");
  }
  const float32 xRangeMax = origin[0] + (static_cast<float32>(dims[0]) * spacing[0]);
  const float32 xDelta = static_cast<float32>(dims[0]) * spacing[0];
  const float32 yRangeMax = origin[1] + (static_cast<float32>(dims[1]) * spacing[1]);
  const float32 yDelta = static_cast<float32>(dims[1]) * spacing[1];
  const float32 zRangeMax = origin[2] + (static_cast<float32>(dims[2]) * spacing[2]);
  const float32 zDelta = static_cast<float32>(dims[2]) * spacing[2];

  std::string desc = fmt::format("X Range: {0} to {1} [{4}] (Delta: {2} [{4}]) 0-{3} Voxels\n", origin[0], xRangeMax, xDelta, dims[0] - 1, lengthUnitStr);
  desc.append(fmt::format("Y Range: {0} to {1} [{4}] (Delta: {2} [{4}]) 0-{3} Voxels\n", origin[1], yRangeMax, yDelta, dims[1] - 1, lengthUnitStr));
  desc.append(fmt::format("Z Range: {0} to {1} [{4}] (Delta: {2} [{4}]) 0-{3} Voxels\n", origin[2], zRangeMax, zDelta, dims[2] - 1, lengthUnitStr));
  return desc;
}

/**
 * @brief Generates the display text that describes the partitioning scheme
 * geometry, shown as a preflight updated value in the user interface.
 * @param psDims The dimensions of the partitioning scheme geometry
 * @param psOrigin The origin of the partitioning scheme geometry
 * @param psSpacing The spacing of the partitioning scheme geometry
 * @param lengthUnits The length units of the partitioning scheme geometry
 * @param iGeom The input geometry, used to determine if the
 * partitioning scheme geometry fits the input geometry or not.
 * @return The text description of the partitioning scheme geometry.
 */
std::string GeneratePartitioningSchemeDisplayText(const SizeVec3& psDims, const FloatVec3& psOrigin, const FloatVec3& psSpacing, const IGeometry::LengthUnit& lengthUnits, const IGeometry& iGeom)
{
  const float32 xRangeMax = (psOrigin[0] + (static_cast<float32>(psDims[0]) * psSpacing[0]));
  const float32 xDelta = static_cast<float32>(psDims[0]) * psSpacing[0];
  const float32 yRangeMax = (psOrigin[1] + (static_cast<float32>(psDims[1]) * psSpacing[1]));
  const float32 yDelta = static_cast<float32>(psDims[1]) * psSpacing[1];
  const float32 zRangeMax = (psOrigin[2] + (static_cast<float32>(psDims[2]) * psSpacing[2]));
  const float32 zDelta = static_cast<float32>(psDims[2]) * psSpacing[2];

  std::string lengthUnitStr = IGeometry::LengthUnitToString(lengthUnits);
  if(lengthUnits == IGeometry::LengthUnit::Unspecified)
  {
    lengthUnitStr.append(" Units");
  }

  std::string desc = fmt::format("X Partition Bounds: {0} to {1} [{3}].   Delta: {2} [{3}].\n", psOrigin[0], xRangeMax, xDelta, lengthUnitStr);
  desc.append(fmt::format("Y Partition Bounds: {0} to {1} [{3}].   Delta: {2} [{3}].\n", psOrigin[1], yRangeMax, yDelta, lengthUnitStr));
  desc.append(fmt::format("Z Partition Bounds: {0} to {1} [{3}].   Delta: {2} [{3}].\n", psOrigin[2], zRangeMax, zDelta, lengthUnitStr));

  if(iGeom.getGeomType() == IGeometry::Type::Image)
  {
    const auto& geometry = dynamic_cast<const ImageGeom&>(iGeom);

    SizeVec3 dims = geometry.getDimensions();
    FloatVec3 origin = geometry.getOrigin();
    FloatVec3 spacing = geometry.getSpacing();

    const float32 gxRangeMax = origin[0] + (static_cast<float32>(dims[0]) * spacing[0]);
    const float32 gyRangeMax = origin[1] + (static_cast<float32>(dims[1]) * spacing[1]);
    const float32 gzRangeMax = origin[2] + (static_cast<float32>(dims[2]) * spacing[2]);

    if(origin[0] < psOrigin[0] || origin[1] < psOrigin[1] || origin[2] < psOrigin[2] || gxRangeMax > xRangeMax || gyRangeMax > yRangeMax || gzRangeMax > zRangeMax)
    {
      desc.append("Geometry size DOES NOT fit within the partitioning space!");
    }
    else
    {
      desc.append("Geometry size fits within partitioning space.");
    }
  }

  return desc;
}

} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string PartitionGeometryFilter::name() const
{
  return FilterTraits<PartitionGeometryFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string PartitionGeometryFilter::className() const
{
  return FilterTraits<PartitionGeometryFilter>::className;
}

//------------------------------------------------------------------------------
Uuid PartitionGeometryFilter::uuid() const
{
  return FilterTraits<PartitionGeometryFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string PartitionGeometryFilter::humanName() const
{
  return "Partition Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> PartitionGeometryFilter::defaultTags() const
{
  return {className(), "Processing", "Segmentation"};
}

//------------------------------------------------------------------------------
Parameters PartitionGeometryFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Geometry Parameters"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputGeometryToPartition_Key, "Input Geometry to Partition", "The input geometry that will be partitioned", DataPath{},
                                                             IGeometry::GetAllGeomTypes()));
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_InputGeometryCellAttributeMatrixPath_Key, "Input Geometry Cell Attribute Matrix ",
                                                                    "The attribute matrix that represents the cell data for the geometry.(Vertex=>Node Geometry, Cell=>Image/Rectilinear)",
                                                                    DataPath{}));

  params.insertSeparator(Parameters::Separator{"Output Partition Grid Parameters"});
  PartitionUtilities::AppendPartitioningParameters(params);
  params.insert(std::make_unique<GeometrySelectionParameter>(PUP::k_ExistingPartitionGridPath_Key, "Existing Partition Grid",
                                                             "This is an existing Image Geometry that defines the partition grid that will be used.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<Int32Parameter>(k_StartingFeatureID_Key, "Starting Feature ID", "The value to start the partition grid's feature ids at.", 1));
  params.insert(std::make_unique<Int32Parameter>(k_OutOfBoundsFeatureID_Key, "Out-Of-Bounds Feature ID",
                                                 "The value used as the feature id for voxels/nodes that are outside the bounds of the partition grid.", 0));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UseVertexMask_Key, "Use Vertex Mask (Node Geometries Only)",
                                      "Feature ID values will only be placed on vertices that have a 'true' mask value. All others will have the Out-Of-Bounds Feature ID value used instead", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_VertexMaskPath_Key, "Vertex Mask", "The complete path to the vertex mask array.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Geometry Data Objects"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureAttrMatrixName_Key, "Partition Attribute Matrix",
                                                          "The name of the feature attribute matrix that will be created as a child of the input geometry.", "Partition Data"));
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_PartitionIdsArrayName_Key, "Partition Ids", "The name of the partition ids output array stored in the input cell attribute matrix", "Partition Ids"));

  params.insertSeparator(Parameters::Separator{"Output Partition Grid Data Objects"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_PartitionGridGeometry_Key, "Partition Grid Geometry", "The complete path to the created partition grid geometry",
                                                             DataPath({"Partition Grid Geometry"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_PartitionGridCellAMName_Key, "Cell Attribute Matrix",
                                                          "The name of the cell attribute matrix that will contain the partition grid's cell data arrays.", "Cell Data"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_PartitionGridFeatureIDsName_Key, "Cell Feature Ids",
                                                          "The name of the feature ids array that will contain the feature ids of the generated partition grid.", "Feature Ids"));

  // AppendPartitioningParameters handles linkage for all shared partition grid params.
  // The following links are specific to PartitionGeometry outputs.
  params.linkParameters(PUP::k_PartitioningMode_Key, PUP::k_ExistingPartitionGridPath_Key, std::make_any<ChoicesParameter::ValueType>(PUP::k_ExistingSchemeModeIndex));
  params.linkParameters(PUP::k_PartitioningMode_Key, k_StartingFeatureID_Key, std::make_any<ChoicesParameter::ValueType>(PUP::k_BasicModeIndex));
  params.linkParameters(PUP::k_PartitioningMode_Key, k_StartingFeatureID_Key, std::make_any<ChoicesParameter::ValueType>(PUP::k_AdvancedModeIndex));
  params.linkParameters(PUP::k_PartitioningMode_Key, k_StartingFeatureID_Key, std::make_any<ChoicesParameter::ValueType>(PUP::k_BoundingBoxModeIndex));
  params.linkParameters(PUP::k_PartitioningMode_Key, k_OutOfBoundsFeatureID_Key, std::make_any<ChoicesParameter::ValueType>(PUP::k_AdvancedModeIndex));
  params.linkParameters(PUP::k_PartitioningMode_Key, k_OutOfBoundsFeatureID_Key, std::make_any<ChoicesParameter::ValueType>(PUP::k_BoundingBoxModeIndex));
  params.linkParameters(PUP::k_PartitioningMode_Key, k_OutOfBoundsFeatureID_Key, std::make_any<ChoicesParameter::ValueType>(PUP::k_ExistingSchemeModeIndex));
  params.linkParameters(k_UseVertexMask_Key, k_VertexMaskPath_Key, std::make_any<BoolParameter::ValueType>(true));
  params.linkParameters(PUP::k_PartitioningMode_Key, k_PartitionGridGeometry_Key, std::make_any<ChoicesParameter::ValueType>(PUP::k_BasicModeIndex));
  params.linkParameters(PUP::k_PartitioningMode_Key, k_PartitionGridGeometry_Key, std::make_any<ChoicesParameter::ValueType>(PUP::k_AdvancedModeIndex));
  params.linkParameters(PUP::k_PartitioningMode_Key, k_PartitionGridGeometry_Key, std::make_any<ChoicesParameter::ValueType>(PUP::k_BoundingBoxModeIndex));
  params.linkParameters(PUP::k_PartitioningMode_Key, k_PartitionGridCellAMName_Key, std::make_any<ChoicesParameter::ValueType>(PUP::k_BasicModeIndex));
  params.linkParameters(PUP::k_PartitioningMode_Key, k_PartitionGridCellAMName_Key, std::make_any<ChoicesParameter::ValueType>(PUP::k_AdvancedModeIndex));
  params.linkParameters(PUP::k_PartitioningMode_Key, k_PartitionGridCellAMName_Key, std::make_any<ChoicesParameter::ValueType>(PUP::k_BoundingBoxModeIndex));
  params.linkParameters(PUP::k_PartitioningMode_Key, k_PartitionGridFeatureIDsName_Key, std::make_any<ChoicesParameter::ValueType>(PUP::k_BasicModeIndex));
  params.linkParameters(PUP::k_PartitioningMode_Key, k_PartitionGridFeatureIDsName_Key, std::make_any<ChoicesParameter::ValueType>(PUP::k_AdvancedModeIndex));
  params.linkParameters(PUP::k_PartitioningMode_Key, k_PartitionGridFeatureIDsName_Key, std::make_any<ChoicesParameter::ValueType>(PUP::k_BoundingBoxModeIndex));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType PartitionGeometryFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer PartitionGeometryFilter::clone() const
{
  return std::make_unique<PartitionGeometryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult PartitionGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pPartitioningModeValue = filterArgs.value<ChoicesParameter::ValueType>(PUP::k_PartitioningMode_Key);
  auto pNumberOfCellsPerAxisValue = filterArgs.value<VectorInt32Parameter::ValueType>(PUP::k_NumberOfCellsPerAxis_Key);
  auto pPartitionGridOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(PUP::k_PartitionGridOrigin_Key);
  auto pCellLengthValue = filterArgs.value<VectorFloat32Parameter::ValueType>(PUP::k_CellLength_Key);
  auto pMinGridCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(PUP::k_MinGridCoord_Key);
  auto pMaxGridCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(PUP::k_MaxGridCoord_Key);
  auto pInputGeomCellAMPathValue = filterArgs.value<DataPath>(k_InputGeometryCellAttributeMatrixPath_Key);
  auto pFeatureAttrMatrixNameValue = filterArgs.value<std::string>(k_FeatureAttrMatrixName_Key);
  auto pPartitionGridGeomValue = filterArgs.value<DataPath>(k_PartitionGridGeometry_Key);
  auto pPartitionGridCellAMNameValue = filterArgs.value<std::string>(k_PartitionGridCellAMName_Key);
  auto pPartitionGridFeatureIDsNameValue = filterArgs.value<std::string>(k_PartitionGridFeatureIDsName_Key);
  auto pInputGeometryToPartitionValue = filterArgs.value<DataPath>(k_InputGeometryToPartition_Key);
  auto pPartitionIdsArrayNameValue = filterArgs.value<std::string>(k_PartitionIdsArrayName_Key);
  auto pUseVertexMask = filterArgs.value<bool>(k_UseVertexMask_Key);

  const SizeVec3 numberOfPartitionsPerAxis = {static_cast<usize>(pNumberOfCellsPerAxisValue[0]), static_cast<usize>(pNumberOfCellsPerAxisValue[1]), static_cast<usize>(pNumberOfCellsPerAxisValue[2])};

  const auto& attrMatrix = dataStructure.getDataRefAs<AttributeMatrix>(pInputGeomCellAMPathValue);
  const auto& iGeom = dataStructure.getDataRefAs<IGeometry>({pInputGeometryToPartitionValue});
  std::string inputGeometryInformation;
  Result<PU::PSGeomInfo> psInfo;
  switch(iGeom.getGeomType())
  {
  case IGeometry::Type::Image: {
    const auto& geometry = dataStructure.getDataRefAs<ImageGeom>({pInputGeometryToPartitionValue});
    Result<> result = dataCheckPartitioningMode<ImageGeom>(dataStructure, filterArgs, geometry);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(result), {})};
    }
    psInfo = PU::GeneratePartitioningSchemeInfo(geometry, dataStructure, filterArgs);
    inputGeometryInformation = GenerateInputGeometryDisplayText(geometry.getDimensions(), geometry.getOrigin(), geometry.getSpacing(), geometry.getUnits());
    break;
  }
  case IGeometry::Type::RectGrid: {
    const auto& geometry = dataStructure.getDataRefAs<RectGridGeom>({pInputGeometryToPartitionValue});
    if(attrMatrix.getNumberOfTuples() != geometry.getNumberOfCells())
    {
      return {MakeErrorResult<OutputActions>(-3010, fmt::format("{}: The attribute matrix '{}' does not have the same tuple count ({}) as geometry \"{}\"'s cell count ({}).", humanName(),
                                                                attrMatrix.getName(), attrMatrix.getNumberOfTuples(), geometry.getName(), geometry.getNumberOfCells()))};
    }
    psInfo = PU::GeneratePartitioningSchemeInfo(geometry, dataStructure, filterArgs);
    inputGeometryInformation = "Rectilinear grid geometry space unknown during preflight.";
    break;
  }
  case IGeometry::Type::Vertex: {
    psInfo = generateNodeBasedPSInfo(dataStructure, filterArgs, pInputGeometryToPartitionValue, pInputGeomCellAMPathValue);
    if(psInfo.invalid())
    {
      return {ConvertResultTo<OutputActions>(ConvertResult(std::move(psInfo)), {})};
    }
    inputGeometryInformation = "Vertex geometry space unknown during preflight.";
    break;
  }
  case IGeometry::Type::Edge: {
    psInfo = generateNodeBasedPSInfo(dataStructure, filterArgs, pInputGeometryToPartitionValue, pInputGeomCellAMPathValue);
    if(psInfo.invalid())
    {
      return {ConvertResultTo<OutputActions>(ConvertResult(std::move(psInfo)), {})};
    }
    inputGeometryInformation = "Edge geometry space unknown during preflight.";
    break;
  }
  case IGeometry::Type::Triangle: {
    psInfo = generateNodeBasedPSInfo(dataStructure, filterArgs, pInputGeometryToPartitionValue, pInputGeomCellAMPathValue);
    if(psInfo.invalid())
    {
      return {ConvertResultTo<OutputActions>(ConvertResult(std::move(psInfo)), {})};
    }
    inputGeometryInformation = "Triangle geometry space unknown during preflight.";
    break;
  }
  case IGeometry::Type::Quad: {
    psInfo = generateNodeBasedPSInfo(dataStructure, filterArgs, pInputGeometryToPartitionValue, pInputGeomCellAMPathValue);
    if(psInfo.invalid())
    {
      return {ConvertResultTo<OutputActions>(ConvertResult(std::move(psInfo)), {})};
    }
    inputGeometryInformation = "Quad geometry space unknown during preflight.";
    break;
  }
  case IGeometry::Type::Tetrahedral: {
    psInfo = generateNodeBasedPSInfo(dataStructure, filterArgs, pInputGeometryToPartitionValue, pInputGeomCellAMPathValue);
    if(psInfo.invalid())
    {
      return {ConvertResultTo<OutputActions>(ConvertResult(std::move(psInfo)), {})};
    }
    inputGeometryInformation = "Tetrahedral geometry space unknown during preflight.";
    break;
  }
  case IGeometry::Type::Hexahedral: {
    psInfo = generateNodeBasedPSInfo(dataStructure, filterArgs, pInputGeometryToPartitionValue, pInputGeomCellAMPathValue);
    if(psInfo.invalid())
    {
      return {ConvertResultTo<OutputActions>(ConvertResult(std::move(psInfo)), {})};
    }
    inputGeometryInformation = "Hexahedral geometry space unknown during preflight.";
    break;
  }
  }

  if(psInfo.invalid())
  {
    return {ConvertResultTo<OutputActions>(ConvertResult(std::move(psInfo)), {})};
  }

  nx::core::Result<OutputActions> resultOutputActions;

  DataPath dap = pInputGeomCellAMPathValue.createChildPath(pPartitionIdsArrayNameValue);
  auto action = std::make_unique<CreateArrayAction>(DataType::int32, attrMatrix.getShape(), std::vector<usize>{1}, dap);
  resultOutputActions.value().appendAction(std::move(action));

  dap = pInputGeomCellAMPathValue.getParent();
  dap = dap.createChildPath(pFeatureAttrMatrixNameValue);
  resultOutputActions.value().appendAction(std::make_unique<CreateAttributeMatrixAction>(dap, attrMatrix.getShape()));

  std::string partitioningSchemeInformation;

  auto psMetadata = psInfo.value();
  const std::vector<usize> psDims = {psMetadata.geometryDims[0], psMetadata.geometryDims[1], psMetadata.geometryDims[2]};
  std::vector<float32> psOrigin;
  std::vector<float32> psSpacing;
  if(!psMetadata.geometryOrigin.has_value() || !psMetadata.geometrySpacing.has_value())
  {
    psOrigin = {0.0, 0.0, 0.0};
    psSpacing = {0.0, 0.0, 0.0};
    partitioningSchemeInformation = "Input geometry space unknown during preflight.";
  }
  else
  {
    psOrigin = {(*psMetadata.geometryOrigin)[0], (*psMetadata.geometryOrigin)[1], (*psMetadata.geometryOrigin)[2]};
    psSpacing = {(*psMetadata.geometrySpacing)[0], (*psMetadata.geometrySpacing)[1], (*psMetadata.geometrySpacing)[2]};
    partitioningSchemeInformation = GeneratePartitioningSchemeDisplayText(psDims, psOrigin, psSpacing, psMetadata.geometryUnits, iGeom);
  }

  if(static_cast<PU::PartitioningMode>(pPartitioningModeValue) != PU::PartitioningMode::ExistingPartitionGrid)
  {
    auto createImageGeometryAction = std::make_unique<CreateImageGeometryAction>(pPartitionGridGeomValue, psDims, psOrigin, psSpacing, pPartitionGridCellAMNameValue);
    resultOutputActions.value().appendAction(std::move(createImageGeometryAction));

    dap = pPartitionGridGeomValue;
    dap = dap.createChildPath(pPartitionGridCellAMNameValue).createChildPath(pPartitionGridFeatureIDsNameValue);
    action = std::make_unique<CreateArrayAction>(DataType::int32, std::vector<usize>{psMetadata.geometryDims[2], psMetadata.geometryDims[1], psMetadata.geometryDims[0]}, std::vector<usize>{1}, dap);
    resultOutputActions.value().appendAction(std::move(action));
  }

  std::vector<PreflightValue> preflightUpdatedValues;
  preflightUpdatedValues.push_back({"Partition Grid Information", partitioningSchemeInformation});
  preflightUpdatedValues.push_back({"Input Geometry Information", inputGeometryInformation});

  if(pUseVertexMask && (iGeom.getGeomType() == IGeometry::Type::Image || iGeom.getGeomType() == IGeometry::Type::RectGrid))
  {
    return {MakeErrorResult<OutputActions>(-3019, fmt::format("{}: The input geometry is {}, which is not vertex-based.  Vertex mask cannot be used.", humanName(), iGeom.getTypeName()))};
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

// -----------------------------------------------------------------------------
Result<PU::PSGeomInfo> PartitionGeometryFilter::generateNodeBasedPSInfo(const DataStructure& dataStructure, const Arguments& filterArgs, const DataPath& geometryToPartitionPath,
                                                                        const DataPath& attrMatrixPath) const
{
  const auto& geometry = dataStructure.getDataRefAs<INodeGeometry0D>({geometryToPartitionPath});
  const IGeometry::SharedVertexList& vertexList = geometry.getVerticesRef();
  const auto& attrMatrix = dataStructure.getDataRefAs<AttributeMatrix>(attrMatrixPath);
  if(attrMatrix.getNumberOfTuples() != vertexList.getNumberOfTuples())
  {
    return {MakeErrorResult<PU::PSGeomInfo>(-3014, fmt::format("{}: The attribute matrix '{}' does not have the same tuple count ({}) as geometry \"{}\"'s vertex count ({}).", humanName(),
                                                               attrMatrix.getName(), attrMatrix.getNumberOfTuples(), geometry.getName(), geometry.getNumberOfVertices()))};
  }
  Result<> dimensionalityResult = DataCheckDimensionality(geometry);
  if(dimensionalityResult.invalid())
  {
    return {ConvertResultTo<PU::PSGeomInfo>(std::move(dimensionalityResult), {})};
  }
  return PU::GeneratePartitioningSchemeInfo(geometry, dataStructure, filterArgs);
}

// -----------------------------------------------------------------------------
Result<> PartitionGeometryFilter::DataCheckDimensionality(const INodeGeometry0D& geometry)
{
  Result<bool> yzPlaneResult = geometry.isYZPlane();
  if(yzPlaneResult.valid() && yzPlaneResult.value())
  {
    return MakeErrorResult(-3040, "Unable to create a partitioning scheme with a X dimension size of 0.  Vertices are in an YZ plane.  Use the Advanced or Bounding Box "
                                  "partitioning modes to manually create a partitioning scheme.");
  }

  Result<bool> xzPlaneResult = geometry.isXZPlane();
  if(xzPlaneResult.valid() && xzPlaneResult.value())
  {
    return MakeErrorResult(-3041, "Unable to create a partitioning scheme with a Y dimension size of 0.  Vertices are in an XZ plane.  Use the Advanced or Bounding Box "
                                  "partitioning modes to manually create a partitioning scheme.");
  }

  Result<bool> xyPlaneResult = geometry.isXYPlane();
  if(xyPlaneResult.valid() && xyPlaneResult.value())
  {
    return MakeErrorResult(-3042, "Unable to create a partitioning scheme with a Z dimension size of 0.  Vertices are in an XY plane.  Use the Advanced or Bounding Box "
                                  "partitioning modes to manually create a partitioning scheme.");
  }

  return {};
}

// -----------------------------------------------------------------------------
template <typename GeomType>
Result<> PartitionGeometryFilter::dataCheckPartitioningMode(const DataStructure& dataStructure, const Arguments& filterArgs, const GeomType& geometryToPartition) const
{
  auto pPartitioningModeValue = filterArgs.value<ChoicesParameter::ValueType>(PUP::k_PartitioningMode_Key);
  auto partitioningMode = static_cast<PU::PartitioningMode>(pPartitioningModeValue);

  auto pNumberOfCellsPerAxisValue = filterArgs.value<VectorInt32Parameter::ValueType>(PUP::k_NumberOfCellsPerAxis_Key);
  SizeVec3 numOfPartitionsPerAxis = {static_cast<usize>(pNumberOfCellsPerAxisValue[0]), static_cast<usize>(pNumberOfCellsPerAxisValue[1]), static_cast<usize>(pNumberOfCellsPerAxisValue[2])};

  auto pCellLengthValue = filterArgs.value<VectorFloat32Parameter::ValueType>(PUP::k_CellLength_Key);
  auto pMinGridCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(PUP::k_MinGridCoord_Key);
  auto pMaxGridCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(PUP::k_MaxGridCoord_Key);

  auto pInputGeomCellAMPathValue = filterArgs.value<DataPath>(k_InputGeometryCellAttributeMatrixPath_Key);
  const auto& attrMatrix = dataStructure.getDataRefAs<AttributeMatrix>({pInputGeomCellAMPathValue});

  switch(partitioningMode)
  {
  case PU::PartitioningMode::Basic:
    return dataCheckBasicMode<GeomType>(numOfPartitionsPerAxis, geometryToPartition, attrMatrix);
  case PU::PartitioningMode::Advanced:
    return dataCheckAdvancedMode<GeomType>(numOfPartitionsPerAxis, pCellLengthValue, geometryToPartition, attrMatrix);
  case PU::PartitioningMode::BoundingBox:
    return dataCheckBoundingBoxMode<GeomType>(numOfPartitionsPerAxis, pMinGridCoordValue, pMaxGridCoordValue, geometryToPartition, attrMatrix);
  case PU::PartitioningMode::ExistingPartitionGrid:
    return {};
  }

  return {};
}

// -----------------------------------------------------------------------------
template <typename GeomType>
Result<> PartitionGeometryFilter::dataCheckBasicMode(const SizeVec3& numOfPartitionsPerAxis, const GeomType& geometryToPartition, const AttributeMatrix& attrMatrix) const
{
  Result<> result = PU::DataCheckNumberOfPartitions(numOfPartitionsPerAxis);
  if(result.invalid())
  {
    return result;
  }

  result = dataCheckPartitioningScheme<GeomType>(geometryToPartition, attrMatrix);
  if(result.invalid())
  {
    return result;
  }

  return {};
}

// -----------------------------------------------------------------------------
template <typename GeomType>
Result<> PartitionGeometryFilter::dataCheckAdvancedMode(const SizeVec3& numOfPartitionsPerAxis, const FloatVec3& lengthPerPartition, const GeomType& geometryToPartition,
                                                        const AttributeMatrix& attrMatrix) const
{
  Result<> result = PU::DataCheckNumberOfPartitions(numOfPartitionsPerAxis);
  if(result.invalid())
  {
    return result;
  }

  result = PU::DataCheckCellLength(humanName(), lengthPerPartition);
  if(result.invalid())
  {
    return result;
  }

  result = dataCheckPartitioningScheme<GeomType>(geometryToPartition, attrMatrix);
  if(result.invalid())
  {
    return result;
  }

  return {};
}

// -----------------------------------------------------------------------------
template <typename GeomType>
Result<> PartitionGeometryFilter::dataCheckBoundingBoxMode(const SizeVec3& numOfPartitionsPerAxis, const FloatVec3& llCoord, const FloatVec3& urCoord, const GeomType& geometryToPartition,
                                                           const AttributeMatrix& attrMatrix) const
{
  Result<> result = PU::DataCheckNumberOfPartitions(numOfPartitionsPerAxis);
  if(result.invalid())
  {
    return result;
  }

  result = PU::DataCheckBoundingBoxCoords(humanName(), llCoord, urCoord);
  if(result.invalid())
  {
    return result;
  }

  result = dataCheckPartitioningScheme<GeomType>(geometryToPartition, attrMatrix);
  if(result.invalid())
  {
    return result;
  }

  return {};
}

// -----------------------------------------------------------------------------
template <typename GeomType>
Result<> PartitionGeometryFilter::dataCheckPartitioningScheme(const GeomType& geometryToPartition, const AttributeMatrix& attrMatrix) const
{
  if constexpr(std::is_same_v<GeomType, ImageGeom> || std::is_same_v<GeomType, RectGridGeom>)
  {
    if(attrMatrix.getNumberOfTuples() != geometryToPartition.getNumberOfCells())
    {
      return MakeErrorResult(-3009, fmt::format("{}: The attribute matrix '{}' does not have the same tuple count ({}) as geometry \"{}\"'s cell count ({}).", humanName(), attrMatrix.getName(),
                                                attrMatrix.getNumberOfTuples(), geometryToPartition.getName(), geometryToPartition.getNumberOfCells()));
    }
  }
  else
  {
    const IGeometry::SharedVertexList& vertexList = geometryToPartition.getVertices();
    if(attrMatrix.getNumberOfTuples() != vertexList.getNumberOfTuples())
    {
      return MakeErrorResult(-3010, fmt::format("{}: The attribute matrix '{}' does not have the same tuple count ({}) as geometry \"{}\"'s vertex count ({}).", humanName(), attrMatrix.getName(),
                                                attrMatrix.getNumberOfTuples(), geometryToPartition.getName(), vertexList.getNumberOfTuples()));
    }
  }

  return {};
}

//------------------------------------------------------------------------------
Result<> PartitionGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{

  PartitionGeometryInputValues inputValues;

  inputValues.PartitioningMode = filterArgs.value<ChoicesParameter::ValueType>(PUP::k_PartitioningMode_Key);
  inputValues.StartingFeatureID = filterArgs.value<int32>(k_StartingFeatureID_Key);
  inputValues.OutOfBoundsFeatureID = filterArgs.value<int32>(k_OutOfBoundsFeatureID_Key);
  inputValues.NumberOfCellsPerAxis = filterArgs.value<VectorInt32Parameter::ValueType>(PUP::k_NumberOfCellsPerAxis_Key);
  inputValues.PartitionGridOrigin = filterArgs.value<VectorFloat32Parameter::ValueType>(PUP::k_PartitionGridOrigin_Key);
  inputValues.CellLength = filterArgs.value<VectorFloat32Parameter::ValueType>(PUP::k_CellLength_Key);
  inputValues.MinGridCoord = filterArgs.value<VectorFloat32Parameter::ValueType>(PUP::k_MinGridCoord_Key);
  inputValues.MaxGridCoord = filterArgs.value<VectorFloat32Parameter::ValueType>(PUP::k_MaxGridCoord_Key);
  inputValues.InputGeomCellAMPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_InputGeometryCellAttributeMatrixPath_Key);
  inputValues.PartitionGridGeomPath = filterArgs.value<DataGroupCreationParameter::ValueType>(k_PartitionGridGeometry_Key);
  inputValues.PartitionGridCellAMName = filterArgs.value<DataObjectNameParameter::ValueType>(k_PartitionGridCellAMName_Key);
  inputValues.PartitionGridFeatureIDsArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_PartitionGridFeatureIDsName_Key);
  inputValues.InputGeometryToPartition = filterArgs.value<DataPath>(k_InputGeometryToPartition_Key);
  inputValues.PartitionIdsArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_PartitionIdsArrayName_Key);
  inputValues.ExistingPartitionGridPath = filterArgs.value<DataPath>(PUP::k_ExistingPartitionGridPath_Key);
  inputValues.UseVertexMask = filterArgs.value<bool>(k_UseVertexMask_Key);
  inputValues.VertexMaskPath = filterArgs.value<DataPath>(k_VertexMaskPath_Key);
  inputValues.FeatureAttrMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(k_FeatureAttrMatrixName_Key);

  return PartitionGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
