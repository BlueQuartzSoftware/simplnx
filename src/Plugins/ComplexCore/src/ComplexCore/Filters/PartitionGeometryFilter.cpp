#include "PartitionGeometryFilter.hpp"

#include "complex/Common/Array.hpp"
#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/Geometry/RectGridGeom.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/GeometryUtilities.hpp"

using namespace complex;

namespace
{

const std::string k_BasicMode("Basic (0)");
const std::string k_AdvancedMode("Advanced (1)");
const std::string k_BoundingBoxMode("Bounding Box (2)");
const std::string k_ExistingSchemeMode("Existing Partition Grid (3)");

const ChoicesParameter::Choices k_Choices = {k_BasicMode, k_AdvancedMode, k_BoundingBoxMode, k_ExistingSchemeMode};

const ChoicesParameter::ValueType k_BasicModeIndex = 0;
const ChoicesParameter::ValueType k_AdvancedModeIndex = 1;
const ChoicesParameter::ValueType k_BoundingBoxModeIndex = 2;
const ChoicesParameter::ValueType k_ExistingSchemeModeIndex = 3;

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

/**
 * @brief Generates the partitioning scheme information (dimensions, origin, spacing, units)
 * that the filter will use to create the partitioning scheme image geometry.
 * @param geometry The input geometry
 * @param filterArgs The arguments from the filter, some filter variables are needed.
 * @return Returns a result that either contains a PSGeomInfo object with the data inside,
 * or an error describing what went wrong during the generation process.
 */
template <typename Geom>
Result<PartitionGeometry::PSGeomInfo> GeneratePartitioningSchemeInfo(const Geom& geometry, const DataStructure& dataStructure, const Arguments& filterArgs)
{
  auto pPartitioningModeValue = filterArgs.value<ChoicesParameter::ValueType>(PartitionGeometryFilter::k_PartitioningMode_Key);
  auto pNumberOfCellsPerAxisValue = filterArgs.value<VectorInt32Parameter::ValueType>(PartitionGeometryFilter::k_NumberOfCellsPerAxis_Key);

  const SizeVec3 numOfPartitionsPerAxisValue = {static_cast<usize>(pNumberOfCellsPerAxisValue[0]), static_cast<usize>(pNumberOfCellsPerAxisValue[1]),
                                                static_cast<usize>(pNumberOfCellsPerAxisValue[2])};

  PartitionGeometry::PSGeomInfo psGeomMetadata;

  switch(static_cast<PartitionGeometryFilter::PartitioningMode>(pPartitioningModeValue))
  {
  case PartitionGeometryFilter::PartitioningMode::Basic: {
    Result<FloatVec3> originResult;
    if constexpr(std::is_same_v<Geom, ImageGeom> || std::is_same_v<Geom, RectGridGeom>)
    {
      originResult = {geometry.getOrigin()};
    }
    else
    {
      originResult = GeometryUtilities::CalculateNodeBasedPartitionSchemeOrigin(geometry);
    }

    Result<FloatVec3> pLengthResult = GeometryUtilities::CalculatePartitionLengthsByPartitionCount(geometry, numOfPartitionsPerAxisValue);
    if(originResult.valid() && pLengthResult.valid())
    {
      psGeomMetadata.geometryDims = {static_cast<usize>(pNumberOfCellsPerAxisValue[0]), static_cast<usize>(pNumberOfCellsPerAxisValue[1]), static_cast<usize>(pNumberOfCellsPerAxisValue[2])};
      psGeomMetadata.geometryOrigin = originResult.value();
      psGeomMetadata.geometrySpacing = pLengthResult.value();
      psGeomMetadata.geometryUnits = geometry.getUnits();
    }
    break;
  }
  case PartitionGeometryFilter::PartitioningMode::Advanced: {
    auto pPartitioningSchemeOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(PartitionGeometryFilter::k_PartitionGridOrigin_Key);
    auto pCellLengthValue = filterArgs.value<VectorFloat32Parameter::ValueType>(PartitionGeometryFilter::k_CellLength_Key);

    psGeomMetadata.geometryDims = {static_cast<usize>(pNumberOfCellsPerAxisValue[0]), static_cast<usize>(pNumberOfCellsPerAxisValue[1]), static_cast<usize>(pNumberOfCellsPerAxisValue[2])};
    psGeomMetadata.geometryOrigin = pPartitioningSchemeOriginValue;
    psGeomMetadata.geometrySpacing = pCellLengthValue;
    psGeomMetadata.geometryUnits = geometry.getUnits();
    break;
  }
  case PartitionGeometryFilter::PartitioningMode::BoundingBox: {
    auto pMinGridCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(PartitionGeometryFilter::k_MinGridCoord_Key);
    auto pMaxGridCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(PartitionGeometryFilter::k_MaxGridCoord_Key);

    psGeomMetadata.geometryDims = numOfPartitionsPerAxisValue;
    psGeomMetadata.geometryOrigin = pMinGridCoordValue;
    const FloatVec3 llCoord(pMinGridCoordValue[0], pMinGridCoordValue[1], pMinGridCoordValue[2]);
    const FloatVec3 urCoord(pMaxGridCoordValue[0], pMaxGridCoordValue[1], pMaxGridCoordValue[2]);

    Result<FloatVec3> result = GeometryUtilities::CalculatePartitionLengthsOfBoundingBox({llCoord, urCoord}, numOfPartitionsPerAxisValue);
    if(result.valid())
    {
      psGeomMetadata.geometrySpacing = result.value();
    }

    psGeomMetadata.geometryUnits = geometry.getUnits();
    break;
  }
  case PartitionGeometryFilter::PartitioningMode::ExistingPartitionGrid: {
    auto pExistingPartitioningSchemePathValue = filterArgs.value<DataPath>(PartitionGeometryFilter::k_ExistingPartitionGridPath_Key);
    const auto& psGeom = dataStructure.getDataRefAs<ImageGeom>(pExistingPartitioningSchemePathValue);
    psGeomMetadata.geometryDims = psGeom.getDimensions();
    psGeomMetadata.geometryOrigin = psGeom.getOrigin();
    psGeomMetadata.geometrySpacing = psGeom.getSpacing();
    psGeomMetadata.geometryUnits = psGeom.getUnits();
    break;
  }
  default: {
    return {MakeErrorResult<PartitionGeometry::PSGeomInfo>(-3011, "Unable to create partitioning scheme geometry - Unknown partitioning mode.")};
  }
  }

  return {psGeomMetadata};
}
} // namespace

namespace complex
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
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex, IGeometry::Type::Edge, IGeometry::Type::Triangle, IGeometry::Type::Quad,
                                                                                                      IGeometry::Type::Tetrahedral, IGeometry::Type::Hexahedral, IGeometry::Type::Image,
                                                                                                      IGeometry::Type::RectGrid}));
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_InputGeometryCellAttributeMatrixPath_Key, "Input Geometry Cell Attribute Matrix ",
                                                                    "The attribute matrix that represents the cell data for the geometry.(Vertex=>Node Geometry, Cell=>Image/Rectilinear)",
                                                                    DataPath{}));
  params.insertSeparator(Parameters::Separator{"Created Partition Grid Parameters"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_PartitioningMode_Key, "Select the partitioning mode",
                                                                    "Mode can be 'Basic (0)', 'Advanced (1)', 'Bounding Box (2)', 'Existing Partition Grid (3)'", 0, ::k_Choices));
  params.insert(std::make_unique<Int32Parameter>(k_StartingFeatureID_Key, "Starting Feature ID", "The value to start the partition grid's feature ids at.", 1));
  params.insert(std::make_unique<Int32Parameter>(k_OutOfBoundsFeatureID_Key, "Out-Of-Bounds Feature ID",
                                                 "The value used as the feature id for voxels/nodes that are outside the bounds of the partition grid.", 0));
  params.insert(std::make_unique<VectorInt32Parameter>(k_NumberOfCellsPerAxis_Key, "Number Of Cells Per Axis", "The number of cells along each axis of the partition grid",
                                                       std::vector<int32>({5, 5, 5}), std::vector<std::string>({"X", "Y", "Z"})));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_PartitionGridOrigin_Key, "Partition Grid Origin", "", std::vector<float32>({0.0F, 0.0F, 0.0F}), std::vector<std::string>({"X", "Y", "Z"})));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_CellLength_Key, "Cell Length (Physical Units)",
                                                         "The length in physical units for each cell in the partition grid. The physical units are automatically set by the input geometry.",
                                                         std::vector<float32>({1.0F, 1.0F, 1.0F}), std::vector<std::string>({"X", "Y", "Z"})));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_MinGridCoord_Key, "Minimum Grid Coordinate", "Minimum grid coordinate used to create the partition grid",
                                                         std::vector<float32>({0.0F, 0.0F, 0.0F}), std::vector<std::string>({"X", "Y", "Z"})));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_MaxGridCoord_Key, "Maximum Grid Coordinate", "Maximum grid coordinate used to create the partition grid",
                                                         std::vector<float32>({10.0F, 10.0F, 10.0F}), std::vector<std::string>({"X", "Y", "Z"})));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ExistingPartitionGridPath_Key, "Existing Partition Grid",
                                                             "This is an existing Image Geometry that defines the partition grid that will be used.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UseVertexMask_Key, "Use Vertex Mask (Node Geometries Only)",
                                      "Feature ID values will only be placed on vertices that have a 'true' mask value. All others will have the Out-Of-Bounds Feature ID value used instead", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_VertexMaskPath_Key, "Vertex Mask", "The complete path to the vertex mask array.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Created Input Geometry Data Objects"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureAttrMatrixName_Key, "Feature Attribute Matrix",
                                                          "The name of the feature attribute matrix that will be created as a child of the input geometry.", "Feature Data"));
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_PartitionIdsArrayName_Key, "Partition Ids", "The name of the partition ids output array stored in the input cell attribute matrix", "Partition Ids"));

  params.insertSeparator(Parameters::Separator{"Created Partition Grid Data Objects"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_PartitionGridGeometry_Key, "Partition Grid Geometry", "The complete path to the created partition grid geometry",
                                                             DataPath({"Partition Grid Geometry"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_PartitionGridCellAMName_Key, "Cell Attribute Matrix",
                                                          "The name of the cell attribute matrix that will contain the partition grid's cell data arrays.", "Cell Data"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_PartitionGridFeatureIDsName_Key, "Feature Ids",
                                                          "The name of the feature ids array that will contain the feature ids of the generated partition grid.", "Feature Ids"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_PartitioningMode_Key, k_StartingFeatureID_Key, std::make_any<ChoicesParameter::ValueType>(k_BasicModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_StartingFeatureID_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_StartingFeatureID_Key, std::make_any<ChoicesParameter::ValueType>(k_BoundingBoxModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_OutOfBoundsFeatureID_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_OutOfBoundsFeatureID_Key, std::make_any<ChoicesParameter::ValueType>(k_BoundingBoxModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_OutOfBoundsFeatureID_Key, std::make_any<ChoicesParameter::ValueType>(k_ExistingSchemeModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_NumberOfCellsPerAxis_Key, std::make_any<ChoicesParameter::ValueType>(k_BasicModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_NumberOfCellsPerAxis_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_NumberOfCellsPerAxis_Key, std::make_any<ChoicesParameter::ValueType>(k_BoundingBoxModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_PartitionGridOrigin_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_CellLength_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_MinGridCoord_Key, std::make_any<ChoicesParameter::ValueType>(k_BoundingBoxModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_MaxGridCoord_Key, std::make_any<ChoicesParameter::ValueType>(k_BoundingBoxModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_ExistingPartitionGridPath_Key, std::make_any<ChoicesParameter::ValueType>(k_ExistingSchemeModeIndex));
  params.linkParameters(k_UseVertexMask_Key, k_VertexMaskPath_Key, std::make_any<BoolParameter::ValueType>(true));
  params.linkParameters(k_PartitioningMode_Key, k_PartitionGridGeometry_Key, std::make_any<ChoicesParameter::ValueType>(k_BasicModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_PartitionGridGeometry_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_PartitionGridGeometry_Key, std::make_any<ChoicesParameter::ValueType>(k_BoundingBoxModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_PartitionGridCellAMName_Key, std::make_any<ChoicesParameter::ValueType>(k_BasicModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_PartitionGridCellAMName_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_PartitionGridCellAMName_Key, std::make_any<ChoicesParameter::ValueType>(k_BoundingBoxModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_PartitionGridFeatureIDsName_Key, std::make_any<ChoicesParameter::ValueType>(k_BasicModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_PartitionGridFeatureIDsName_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_PartitionGridFeatureIDsName_Key, std::make_any<ChoicesParameter::ValueType>(k_BoundingBoxModeIndex));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer PartitionGeometryFilter::clone() const
{
  return std::make_unique<PartitionGeometryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult PartitionGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  auto pPartitioningModeValue = filterArgs.value<ChoicesParameter::ValueType>(k_PartitioningMode_Key);
  auto pNumberOfCellsPerAxisValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_NumberOfCellsPerAxis_Key);
  auto pPartitionGridOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_PartitionGridOrigin_Key);
  auto pCellLengthValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_CellLength_Key);
  auto pMinGridCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MinGridCoord_Key);
  auto pMaxGridCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MaxGridCoord_Key);
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
  Result<PartitionGeometry::PSGeomInfo> psInfo;
  switch(iGeom.getGeomType())
  {
  case IGeometry::Type::Image: {
    const auto& geometry = dataStructure.getDataRefAs<ImageGeom>({pInputGeometryToPartitionValue});
    Result<> result = dataCheckPartitioningMode<ImageGeom>(dataStructure, filterArgs, geometry);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(result), {})};
    }
    psInfo = GeneratePartitioningSchemeInfo(geometry, dataStructure, filterArgs);
    inputGeometryInformation = GenerateInputGeometryDisplayText(geometry.getDimensions(), geometry.getOrigin(), geometry.getSpacing(), geometry.getUnits());
    break;
  }
  case IGeometry::Type::RectGrid: {
    const auto& geometry = dataStructure.getDataRefAs<RectGridGeom>({pInputGeometryToPartitionValue});
    if(attrMatrix.getNumTuples() != geometry.getNumberOfCells())
    {
      return {MakeErrorResult<OutputActions>(-3010, fmt::format("{}: The attribute matrix '{}' does not have the same tuple count ({}) as geometry \"{}\"'s cell count ({}).", humanName(),
                                                                attrMatrix.getName(), attrMatrix.getNumTuples(), geometry.getName(), geometry.getNumberOfCells()))};
    }
    psInfo = GeneratePartitioningSchemeInfo(geometry, dataStructure, filterArgs);
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
  default: {
    return {MakeErrorResult<OutputActions>(-3017, fmt::format("{}: Unable to partition geometry - Unknown geometry type detected.", humanName()))};
  }
  }

  if(psInfo.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(psInfo))), {})};
  }

  complex::Result<OutputActions> resultOutputActions;

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

  if(static_cast<PartitionGeometryFilter::PartitioningMode>(pPartitioningModeValue) != PartitionGeometryFilter::PartitioningMode::ExistingPartitionGrid)
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
Result<PartitionGeometry::PSGeomInfo> PartitionGeometryFilter::generateNodeBasedPSInfo(const DataStructure& dataStructure, const Arguments& filterArgs, const DataPath& geometryToPartitionPath,
                                                                                       const DataPath& attrMatrixPath) const
{
  const auto& geometry = dataStructure.getDataRefAs<INodeGeometry0D>({geometryToPartitionPath});
  const IGeometry::SharedVertexList& vertexList = geometry.getVerticesRef();
  const auto& attrMatrix = dataStructure.getDataRefAs<AttributeMatrix>(attrMatrixPath);
  if(attrMatrix.getNumTuples() != vertexList.getNumberOfTuples())
  {
    return {MakeErrorResult<PartitionGeometry::PSGeomInfo>(-3014, fmt::format("{}: The attribute matrix '{}' does not have the same tuple count ({}) as geometry \"{}\"'s vertex count ({}).",
                                                                              humanName(), attrMatrix.getName(), attrMatrix.getNumTuples(), geometry.getName(), geometry.getNumberOfVertices()))};
  }
  Result<> dimensionalityResult = DataCheckDimensionality(geometry);
  if(dimensionalityResult.invalid())
  {
    return {ConvertResultTo<PartitionGeometry::PSGeomInfo>(std::move(dimensionalityResult), {})};
  }
  return GeneratePartitioningSchemeInfo(geometry, dataStructure, filterArgs);
}

// -----------------------------------------------------------------------------
Result<> PartitionGeometryFilter::DataCheckDimensionality(const INodeGeometry0D& geometry)
{
  Result<bool> yzPlaneResult = geometry.isYZPlane();
  if(yzPlaneResult.valid() && yzPlaneResult.value())
  {
    return {MakeErrorResult(-3040, "Unable to create a partitioning scheme with a X dimension size of 0.  Vertices are in an YZ plane.  Use the Advanced or Bounding Box "
                                   "partitioning modes to manually create a partitioning scheme.")};
  }

  Result<bool> xzPlaneResult = geometry.isXZPlane();
  if(xzPlaneResult.valid() && xzPlaneResult.value())
  {
    return {MakeErrorResult(-3041, "Unable to create a partitioning scheme with a Y dimension size of 0.  Vertices are in an XZ plane.  Use the Advanced or Bounding Box "
                                   "partitioning modes to manually create a partitioning scheme.")};
  }

  Result<bool> xyPlaneResult = geometry.isXYPlane();
  if(xyPlaneResult.valid() && xyPlaneResult.value())
  {
    return {MakeErrorResult(-3042, "Unable to create a partitioning scheme with a Z dimension size of 0.  Vertices are in an XY plane.  Use the Advanced or Bounding Box "
                                   "partitioning modes to manually create a partitioning scheme.")};
  }

  return {};
}

// -----------------------------------------------------------------------------
template <typename GeomType>
Result<> PartitionGeometryFilter::dataCheckPartitioningMode(const DataStructure& dataStructure, const Arguments& filterArgs, const GeomType& geometryToPartition) const
{
  auto pPartitioningModeValue = filterArgs.value<ChoicesParameter::ValueType>(k_PartitioningMode_Key);
  auto partitioningMode = static_cast<PartitioningMode>(pPartitioningModeValue);

  auto pNumberOfCellsPerAxisValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_NumberOfCellsPerAxis_Key);
  SizeVec3 numOfPartitionsPerAxis = {static_cast<usize>(pNumberOfCellsPerAxisValue[0]), static_cast<usize>(pNumberOfCellsPerAxisValue[1]), static_cast<usize>(pNumberOfCellsPerAxisValue[2])};

  auto pCellLengthValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_CellLength_Key);
  auto pMinGridCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MinGridCoord_Key);
  auto pMaxGridCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MaxGridCoord_Key);

  auto pInputGeomCellAMPathValue = filterArgs.value<DataPath>(k_InputGeometryCellAttributeMatrixPath_Key);
  const auto& attrMatrix = dataStructure.getDataRefAs<AttributeMatrix>({pInputGeomCellAMPathValue});

  switch(partitioningMode)
  {
  case PartitioningMode::Basic:
    return dataCheckBasicMode<GeomType>(numOfPartitionsPerAxis, geometryToPartition, attrMatrix);
  case PartitioningMode::Advanced:
    return dataCheckAdvancedMode<GeomType>(numOfPartitionsPerAxis, pCellLengthValue, geometryToPartition, attrMatrix);
  case PartitioningMode::BoundingBox:
    return dataCheckBoundingBoxMode<GeomType>(numOfPartitionsPerAxis, pMinGridCoordValue, pMaxGridCoordValue, geometryToPartition, attrMatrix);
  case PartitioningMode::ExistingPartitionGrid:
    return DataCheckExistingGeometryMode();
  }

  return {};
}

// -----------------------------------------------------------------------------
template <typename GeomType>
Result<> PartitionGeometryFilter::dataCheckBasicMode(const SizeVec3& numOfPartitionsPerAxis, const GeomType& geometryToPartition, const AttributeMatrix& attrMatrix) const
{
  Result<> result = DataCheckNumberOfPartitions(numOfPartitionsPerAxis);
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
  Result<> result = DataCheckNumberOfPartitions(numOfPartitionsPerAxis);
  if(result.invalid())
  {
    return result;
  }

  if(lengthPerPartition.getX() < 0)
  {
    return {MakeErrorResult(-3003, fmt::format("{}: Length Per Partition - The X value cannot be negative.", humanName()))};
  }
  if(lengthPerPartition.getY() < 0)
  {
    return {MakeErrorResult(-3004, fmt::format("{}: Length Per Partition - The Y value cannot be negative.", humanName()))};
  }
  if(lengthPerPartition.getZ() < 0)
  {
    return {MakeErrorResult(-3005, fmt::format("{}: Length Per Partition - The Z value cannot be negative.", humanName()))};
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
  Result<> result = DataCheckNumberOfPartitions(numOfPartitionsPerAxis);
  if(result.invalid())
  {
    return result;
  }

  if(llCoord.getX() > urCoord.getX())
  {
    return {MakeErrorResult(-3006, fmt::format("{}: Lower Left Coordinate - X value is larger than the upper right coordinate X value.", humanName()))};
  }

  if(llCoord.getY() > urCoord.getY())
  {
    return {MakeErrorResult(-3007, fmt::format("{}: Lower Left Coordinate - Y value is larger than the upper right coordinate Y value.", humanName()))};
  }

  if(llCoord.getZ() > urCoord.getZ())
  {
    return {MakeErrorResult(-3008, fmt::format("{}: Lower Left Coordinate - Z value is larger than the upper right coordinate Z value.", humanName()))};
  }

  result = dataCheckPartitioningScheme<GeomType>(geometryToPartition, attrMatrix);
  if(result.invalid())
  {
    return result;
  }

  return {};
}

// -----------------------------------------------------------------------------
Result<> PartitionGeometryFilter::DataCheckExistingGeometryMode()
{
  // Nothing to do!
  return {};
}

// -----------------------------------------------------------------------------
template <typename GeomType>
Result<> PartitionGeometryFilter::dataCheckPartitioningScheme(const GeomType& geometryToPartition, const AttributeMatrix& attrMatrix) const
{
  if constexpr(std::is_same_v<GeomType, ImageGeom> || std::is_same_v<GeomType, RectGridGeom>)
  {
    if(attrMatrix.getNumTuples() != geometryToPartition.getNumberOfCells())
    {
      return {MakeErrorResult(-3009, fmt::format("{}: The attribute matrix '{}' does not have the same tuple count ({}) as geometry \"{}\"'s cell count ({}).", humanName(), attrMatrix.getName(),
                                                 attrMatrix.getNumTuples(), geometryToPartition.getName(), geometryToPartition.getNumberOfCells()))};
    }
  }
  else
  {
    const IGeometry::SharedVertexList& vertexList = geometryToPartition.getVertices();
    if(attrMatrix.getNumTuples() != vertexList.getNumberOfTuples())
    {
      return {MakeErrorResult(-3010, fmt::format("{}: The attribute matrix '{}' does not have the same tuple count ({}) as geometry \"{}\"'s vertex count ({}).", humanName(), attrMatrix.getName(),
                                                 attrMatrix.getNumTuples(), geometryToPartition.getName(), vertexList.getNumberOfTuples()))};
    }
  }

  return {};
}

// -----------------------------------------------------------------------------
Result<> PartitionGeometryFilter::DataCheckNumberOfPartitions(const SizeVec3& numberOfPartitionsPerAxis)
{
  if(numberOfPartitionsPerAxis.getX() <= 0)
  {
    return {MakeErrorResult(-3012, "Number of Partitions Per Axis: The X dimension must be greater than 0.")};
  }

  if(numberOfPartitionsPerAxis.getY() <= 0)
  {
    return {MakeErrorResult(-3013, "Number of Partitions Per Axis: The Y dimension must be greater than 0.")};
  }

  if(numberOfPartitionsPerAxis.getZ() <= 0)
  {
    return {MakeErrorResult(-3014, "Number of Partitions Per Axis: The Z dimension must be greater than 0.")};
  }

  return {};
}

//------------------------------------------------------------------------------
Result<> PartitionGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
{

  PartitionGeometryInputValues inputValues;

  inputValues.PartitioningMode = filterArgs.value<ChoicesParameter::ValueType>(k_PartitioningMode_Key);
  inputValues.StartingFeatureID = filterArgs.value<int32>(k_StartingFeatureID_Key);
  inputValues.OutOfBoundsFeatureID = filterArgs.value<int32>(k_OutOfBoundsFeatureID_Key);
  inputValues.NumberOfCellsPerAxis = filterArgs.value<VectorInt32Parameter::ValueType>(k_NumberOfCellsPerAxis_Key);
  inputValues.PartitionGridOrigin = filterArgs.value<VectorFloat32Parameter::ValueType>(k_PartitionGridOrigin_Key);
  inputValues.CellLength = filterArgs.value<VectorFloat32Parameter::ValueType>(k_CellLength_Key);
  inputValues.MinGridCoord = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MinGridCoord_Key);
  inputValues.MaxGridCoord = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MaxGridCoord_Key);
  inputValues.InputGeomCellAMPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_InputGeometryCellAttributeMatrixPath_Key);
  inputValues.PartitionGridGeomPath = filterArgs.value<DataGroupCreationParameter::ValueType>(k_PartitionGridGeometry_Key);
  inputValues.PartitionGridCellAMName = filterArgs.value<DataObjectNameParameter::ValueType>(k_PartitionGridCellAMName_Key);
  inputValues.PartitionGridFeatureIDsArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_PartitionGridFeatureIDsName_Key);
  inputValues.InputGeometryToPartition = filterArgs.value<DataPath>(k_InputGeometryToPartition_Key);
  inputValues.PartitionIdsArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_PartitionIdsArrayName_Key);
  inputValues.ExistingPartitionGridPath = filterArgs.value<DataPath>(k_ExistingPartitionGridPath_Key);
  inputValues.UseVertexMask = filterArgs.value<bool>(k_UseVertexMask_Key);
  inputValues.VertexMaskPath = filterArgs.value<DataPath>(k_VertexMaskPath_Key);
  inputValues.FeatureAttrMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(k_FeatureAttrMatrixName_Key);

  return PartitionGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
