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
const std::string k_ExistingSchemeMode("Existing Partitioning Scheme (3)");

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
  auto pNumberOfPartitionsPerAxisValue = filterArgs.value<VectorInt32Parameter::ValueType>(PartitionGeometryFilter::k_NumberOfPartitionsPerAxis_Key);

  const SizeVec3 numOfPartitionsPerAxisValue = {static_cast<usize>(pNumberOfPartitionsPerAxisValue[0]), static_cast<usize>(pNumberOfPartitionsPerAxisValue[1]),
                                                static_cast<usize>(pNumberOfPartitionsPerAxisValue[2])};

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
      originResult = CalculateNodeBasedPartitionSchemeOrigin(geometry);
    }

    Result<FloatVec3> pLengthResult = CalculatePartitionLengthsByPartitionCount(geometry, numOfPartitionsPerAxisValue);
    if(originResult.valid() && pLengthResult.valid())
    {
      psGeomMetadata.geometryDims = {static_cast<usize>(pNumberOfPartitionsPerAxisValue[0]), static_cast<usize>(pNumberOfPartitionsPerAxisValue[1]),
                                     static_cast<usize>(pNumberOfPartitionsPerAxisValue[2])};
      psGeomMetadata.geometryOrigin = originResult.value();
      psGeomMetadata.geometrySpacing = pLengthResult.value();
      psGeomMetadata.geometryUnits = geometry.getUnits();
    }
    break;
  }
  case PartitionGeometryFilter::PartitioningMode::Advanced: {
    auto pPartitioningSchemeOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(PartitionGeometryFilter::k_PartitioningSchemeOrigin_Key);
    auto pLengthPerPartitionValue = filterArgs.value<VectorFloat32Parameter::ValueType>(PartitionGeometryFilter::k_LengthPerPartition_Key);

    psGeomMetadata.geometryDims = {static_cast<usize>(pNumberOfPartitionsPerAxisValue[0]), static_cast<usize>(pNumberOfPartitionsPerAxisValue[1]),
                                   static_cast<usize>(pNumberOfPartitionsPerAxisValue[2])};
    psGeomMetadata.geometryOrigin = pPartitioningSchemeOriginValue;
    psGeomMetadata.geometrySpacing = pLengthPerPartitionValue;
    psGeomMetadata.geometryUnits = geometry.getUnits();
    break;
  }
  case PartitionGeometryFilter::PartitioningMode::BoundingBox: {
    auto pLowerLeftCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(PartitionGeometryFilter::k_LowerLeftCoord_Key);
    auto pUpperRightCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(PartitionGeometryFilter::k_UpperRightCoord_Key);

    psGeomMetadata.geometryDims = numOfPartitionsPerAxisValue;
    psGeomMetadata.geometryOrigin = pLowerLeftCoordValue;
    const FloatVec3 llCoord(pLowerLeftCoordValue[0], pLowerLeftCoordValue[1], pLowerLeftCoordValue[2]);
    const FloatVec3 urCoord(pUpperRightCoordValue[0], pUpperRightCoordValue[1], pUpperRightCoordValue[2]);

    Result<FloatVec3> result = CalculatePartitionLengthsOfBoundingBox({llCoord, urCoord}, numOfPartitionsPerAxisValue);
    if(result.valid())
    {
      psGeomMetadata.geometrySpacing = result.value();
    }

    psGeomMetadata.geometryUnits = geometry.getUnits();
    break;
  }
  case PartitionGeometryFilter::PartitioningMode::ExistingPartitioningScheme: {
    auto pExistingPartitioningSchemePathValue = filterArgs.value<DataPath>(PartitionGeometryFilter::k_ExistingPartitioningSchemePath_Key);
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
  return {"Processing", "Segmentation"};
}

//------------------------------------------------------------------------------
Parameters PartitionGeometryFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Geometry Details"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_GeometryToPartition_Key, "Geometry to Partition", "The input geometry that will be partitioned", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex, IGeometry::Type::Edge, IGeometry::Type::Triangle, IGeometry::Type::Quad,
                                                                                                      IGeometry::Type::Tetrahedral, IGeometry::Type::Hexahedral, IGeometry::Type::Image,
                                                                                                      IGeometry::Type::RectGrid}));
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_AttributeMatrixPath_Key, "Input Geometry Cell Attribute Matrix ",
                                                                    "The attribute matrix that represents the cell data for the geometry.(Vertex=>Node Geometry, Cell=>Image/Rectilinear)",
                                                                    DataPath{}));
  params.insertSeparator(Parameters::Separator{"Partitioning Scheme Details"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_PartitioningMode_Key, "Select the partitioning mode",
                                                                    "Mode can be 'Basic (0)', 'Advanced (1)', 'Bounding Box (2)', 'Existing Partitioning Scheme (3)'", 0, ::k_Choices));
  params.insert(std::make_unique<Int32Parameter>(k_StartingPartitionID_Key, "Starting Partition ID", "The value to start the partition ids at.", 1));
  params.insert(std::make_unique<Int32Parameter>(k_OutOfBoundsValue_Key, "Out-Of-Bounds Value",
                                                 "The value used as the partition id for cells/vertices that are outside the bounds of the partitioning scheme.", 0));
  params.insert(std::make_unique<VectorInt32Parameter>(k_NumberOfPartitionsPerAxis_Key, "Number Of Partitions Per Axis", "The number of partitions along each axis", std::vector<int32>({5, 5, 5}),
                                                       std::vector<std::string>({"X", "Y", "Z"})));
  params.insert(
      std::make_unique<VectorFloat32Parameter>(k_PartitioningSchemeOrigin_Key, "Partitioning Scheme Origin", "", std::vector<float32>({0.0F, 0.0F, 0.0F}), std::vector<std::string>({"X", "Y", "Z"})));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_LengthPerPartition_Key, "Length Per Partition",
                                                         "The length in units for each partition. These should be consistent with the units being used in other filters",
                                                         std::vector<float32>({1.0F, 1.0F, 1.0F}), std::vector<std::string>({"X", "Y", "Z"})));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_LowerLeftCoord_Key, "Lower Left Coordinate", "Minimum value coordinate", std::vector<float32>({0.0F, 0.0F, 0.0F}),
                                                         std::vector<std::string>({"X", "Y", "Z"})));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_UpperRightCoord_Key, "Upper Right Coordinate", "The Maximum value coordinate", std::vector<float32>({10.0F, 10.0F, 10.0F}),
                                                         std::vector<std::string>({"X", "Y", "Z"})));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ExistingPartitioningSchemePath_Key, "Existing Partitioning Scheme",
                                                             "This is an existing Image Geometry that defines the partitioning scheme that is to be used.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(
      k_UseVertexMask_Key, "Use Vertex Mask", "Partition ID values will only be placed on vertices that have a 'true' mask value. All others will have the invalid value used instead", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_VertexMaskPath_Key, "Vertex Mask", "The complete path to the vertex mask array.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Created DataStructure Objects"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureAttrMatrixName_Key, "Feature Attribute Matrix Name",
                                                          "The name of the feature attribute matrix that will be created as a child of the input geometry.", "Partition Feature Data"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_PartitionIdsArrayName_Key, "Partition Ids Array Name",
                                                          "The name of the partition ids output array stored in the input cell attribute matrix", "Partition Ids"));

  params.insertSeparator(Parameters::Separator{"Created Partitioning Scheme Objects"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_PSGeometry_Key, "Partitioning Scheme Geometry", "The complete path to the Partitioning Scheme Geometry being created",
                                                             DataPath({"Partitioning Scheme Geometry"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_PSGeometryAMName_Key, "Partitioning Scheme Attribute Matrix",
                                                          "The name of the partitioning scheme cell attribute matrix that will be created as a child of the Partitioning Scheme geometry.",
                                                          "Cell Data"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_PSGeometryDataName_Key, "Partitioning Scheme Ids",
                                                          "The name of the partitioning scheme ids array that will be created as a child of the Partitioning Scheme cell attribute matrix.",
                                                          "PartitioningSchemeIds"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_PartitioningMode_Key, k_StartingPartitionID_Key, std::make_any<ChoicesParameter::ValueType>(k_BasicModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_StartingPartitionID_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_StartingPartitionID_Key, std::make_any<ChoicesParameter::ValueType>(k_BoundingBoxModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_OutOfBoundsValue_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_OutOfBoundsValue_Key, std::make_any<ChoicesParameter::ValueType>(k_BoundingBoxModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_OutOfBoundsValue_Key, std::make_any<ChoicesParameter::ValueType>(k_ExistingSchemeModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_NumberOfPartitionsPerAxis_Key, std::make_any<ChoicesParameter::ValueType>(k_BasicModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_NumberOfPartitionsPerAxis_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_NumberOfPartitionsPerAxis_Key, std::make_any<ChoicesParameter::ValueType>(k_BoundingBoxModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_PartitioningSchemeOrigin_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_LengthPerPartition_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_LowerLeftCoord_Key, std::make_any<ChoicesParameter::ValueType>(k_BoundingBoxModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_UpperRightCoord_Key, std::make_any<ChoicesParameter::ValueType>(k_BoundingBoxModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_ExistingPartitioningSchemePath_Key, std::make_any<ChoicesParameter::ValueType>(k_ExistingSchemeModeIndex));
  params.linkParameters(k_UseVertexMask_Key, k_VertexMaskPath_Key, std::make_any<BoolParameter::ValueType>(true));
  params.linkParameters(k_PartitioningMode_Key, k_PSGeometry_Key, std::make_any<ChoicesParameter::ValueType>(k_BasicModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_PSGeometry_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_PSGeometry_Key, std::make_any<ChoicesParameter::ValueType>(k_BoundingBoxModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_PSGeometryAMName_Key, std::make_any<ChoicesParameter::ValueType>(k_BasicModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_PSGeometryAMName_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_PSGeometryAMName_Key, std::make_any<ChoicesParameter::ValueType>(k_BoundingBoxModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_PSGeometryDataName_Key, std::make_any<ChoicesParameter::ValueType>(k_BasicModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_PSGeometryDataName_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_PSGeometryDataName_Key, std::make_any<ChoicesParameter::ValueType>(k_BoundingBoxModeIndex));

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
  auto pNumberOfPartitionsPerAxisValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_NumberOfPartitionsPerAxis_Key);
  auto pPartitioningSchemeOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_PartitioningSchemeOrigin_Key);
  auto pLengthPerPartitionValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_LengthPerPartition_Key);
  auto pLowerLeftCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_LowerLeftCoord_Key);
  auto pUpperRightCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_UpperRightCoord_Key);
  auto pAttributeMatrixPathValue = filterArgs.value<DataPath>(k_AttributeMatrixPath_Key);
  auto pFeatureAttrMatrixNameValue = filterArgs.value<std::string>(k_FeatureAttrMatrixName_Key);
  auto pPSGeometryValue = filterArgs.value<DataPath>(k_PSGeometry_Key);
  auto pPSGeometryAMNameValue = filterArgs.value<std::string>(k_PSGeometryAMName_Key);
  auto pPSGeometryDataArrayNameValue = filterArgs.value<std::string>(k_PSGeometryDataName_Key);
  auto pGeometryToPartitionValue = filterArgs.value<DataPath>(k_GeometryToPartition_Key);
  auto pPartitionIdsArrayNameValue = filterArgs.value<std::string>(k_PartitionIdsArrayName_Key);

  const SizeVec3 numberOfPartitionsPerAxis = {static_cast<usize>(pNumberOfPartitionsPerAxisValue[0]), static_cast<usize>(pNumberOfPartitionsPerAxisValue[1]),
                                              static_cast<usize>(pNumberOfPartitionsPerAxisValue[2])};

  const auto& attrMatrix = dataStructure.getDataRefAs<AttributeMatrix>(pAttributeMatrixPathValue);
  const auto& iGeom = dataStructure.getDataRefAs<IGeometry>({pGeometryToPartitionValue});
  std::string inputGeometryInformation;
  Result<PartitionGeometry::PSGeomInfo> psInfo;
  switch(iGeom.getGeomType())
  {
  case IGeometry::Type::Image: {
    const auto& geometry = dataStructure.getDataRefAs<ImageGeom>({pGeometryToPartitionValue});
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
    const auto& geometry = dataStructure.getDataRefAs<RectGridGeom>({pGeometryToPartitionValue});
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
    psInfo = generateNodeBasedPSInfo(dataStructure, filterArgs, pGeometryToPartitionValue, pAttributeMatrixPathValue);
    if(psInfo.invalid())
    {
      return {ConvertResultTo<OutputActions>(ConvertResult(std::move(psInfo)), {})};
    }
    inputGeometryInformation = "Vertex geometry space unknown during preflight.";
    break;
  }
  case IGeometry::Type::Edge: {
    psInfo = generateNodeBasedPSInfo(dataStructure, filterArgs, pGeometryToPartitionValue, pAttributeMatrixPathValue);
    if(psInfo.invalid())
    {
      return {ConvertResultTo<OutputActions>(ConvertResult(std::move(psInfo)), {})};
    }
    inputGeometryInformation = "Edge geometry space unknown during preflight.";
    break;
  }
  case IGeometry::Type::Triangle: {
    psInfo = generateNodeBasedPSInfo(dataStructure, filterArgs, pGeometryToPartitionValue, pAttributeMatrixPathValue);
    if(psInfo.invalid())
    {
      return {ConvertResultTo<OutputActions>(ConvertResult(std::move(psInfo)), {})};
    }
    inputGeometryInformation = "Triangle geometry space unknown during preflight.";
    break;
  }
  case IGeometry::Type::Quad: {
    psInfo = generateNodeBasedPSInfo(dataStructure, filterArgs, pGeometryToPartitionValue, pAttributeMatrixPathValue);
    if(psInfo.invalid())
    {
      return {ConvertResultTo<OutputActions>(ConvertResult(std::move(psInfo)), {})};
    }
    inputGeometryInformation = "Quad geometry space unknown during preflight.";
    break;
  }
  case IGeometry::Type::Tetrahedral: {
    psInfo = generateNodeBasedPSInfo(dataStructure, filterArgs, pGeometryToPartitionValue, pAttributeMatrixPathValue);
    if(psInfo.invalid())
    {
      return {ConvertResultTo<OutputActions>(ConvertResult(std::move(psInfo)), {})};
    }
    inputGeometryInformation = "Tetrahedral geometry space unknown during preflight.";
    break;
  }
  case IGeometry::Type::Hexahedral: {
    psInfo = generateNodeBasedPSInfo(dataStructure, filterArgs, pGeometryToPartitionValue, pAttributeMatrixPathValue);
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

  DataPath dap = pAttributeMatrixPathValue.createChildPath(pPartitionIdsArrayNameValue);
  auto action = std::make_unique<CreateArrayAction>(DataType::int32, attrMatrix.getShape(), std::vector<usize>{1}, dap);
  resultOutputActions.value().actions.push_back(std::move(action));

  dap = pAttributeMatrixPathValue.getParent();
  dap = dap.createChildPath(pFeatureAttrMatrixNameValue);
  resultOutputActions.value().actions.push_back(std::make_unique<CreateAttributeMatrixAction>(dap, attrMatrix.getShape()));

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

  if(static_cast<PartitionGeometryFilter::PartitioningMode>(pPartitioningModeValue) != PartitionGeometryFilter::PartitioningMode::ExistingPartitioningScheme)
  {
    auto createImageGeometryAction = std::make_unique<CreateImageGeometryAction>(pPSGeometryValue, psDims, psOrigin, psSpacing, pPSGeometryAMNameValue);
    resultOutputActions.value().actions.push_back(std::move(createImageGeometryAction));

    dap = pPSGeometryValue;
    dap = dap.createChildPath(pPSGeometryAMNameValue).createChildPath(pPSGeometryDataArrayNameValue);
    action = std::make_unique<CreateArrayAction>(DataType::int32, std::vector<usize>{psMetadata.geometryDims[2], psMetadata.geometryDims[1], psMetadata.geometryDims[0]}, std::vector<usize>{1}, dap);
    resultOutputActions.value().actions.push_back(std::move(action));
  }

  std::vector<PreflightValue> preflightUpdatedValues;
  preflightUpdatedValues.push_back({"Partitioning Scheme Information", partitioningSchemeInformation});
  preflightUpdatedValues.push_back({"Input Geometry Information", inputGeometryInformation});

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

  auto pNumberOfPartitionsPerAxisValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_NumberOfPartitionsPerAxis_Key);
  SizeVec3 numOfPartitionsPerAxis = {static_cast<usize>(pNumberOfPartitionsPerAxisValue[0]), static_cast<usize>(pNumberOfPartitionsPerAxisValue[1]),
                                     static_cast<usize>(pNumberOfPartitionsPerAxisValue[2])};

  auto pLengthPerPartitionValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_LengthPerPartition_Key);
  auto pLowerLeftCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_LowerLeftCoord_Key);
  auto pUpperRightCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_UpperRightCoord_Key);

  auto pAttributeMatrixPathValue = filterArgs.value<DataPath>(k_AttributeMatrixPath_Key);
  const auto& attrMatrix = dataStructure.getDataRefAs<AttributeMatrix>({pAttributeMatrixPathValue});

  switch(partitioningMode)
  {
  case PartitioningMode::Basic:
    return dataCheckBasicMode<GeomType>(numOfPartitionsPerAxis, geometryToPartition, attrMatrix);
  case PartitioningMode::Advanced:
    return dataCheckAdvancedMode<GeomType>(numOfPartitionsPerAxis, pLengthPerPartitionValue, geometryToPartition, attrMatrix);
  case PartitioningMode::BoundingBox:
    return dataCheckBoundingBoxMode<GeomType>(numOfPartitionsPerAxis, pLowerLeftCoordValue, pUpperRightCoordValue, geometryToPartition, attrMatrix);
  case PartitioningMode::ExistingPartitioningScheme:
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
  inputValues.StartingPartitionID = filterArgs.value<int32>(k_StartingPartitionID_Key);
  inputValues.OutOfBoundsValue = filterArgs.value<int32>(k_OutOfBoundsValue_Key);
  inputValues.NumberOfPartitionsPerAxis = filterArgs.value<VectorInt32Parameter::ValueType>(k_NumberOfPartitionsPerAxis_Key);
  inputValues.PartitioningSchemeOrigin = filterArgs.value<VectorFloat32Parameter::ValueType>(k_PartitioningSchemeOrigin_Key);
  inputValues.LengthPerPartition = filterArgs.value<VectorFloat32Parameter::ValueType>(k_LengthPerPartition_Key);
  inputValues.LowerLeftCoord = filterArgs.value<VectorFloat32Parameter::ValueType>(k_LowerLeftCoord_Key);
  inputValues.UpperRightCoord = filterArgs.value<VectorFloat32Parameter::ValueType>(k_UpperRightCoord_Key);
  inputValues.AttributeMatrixPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_AttributeMatrixPath_Key);
  inputValues.PSGeometryPath = filterArgs.value<DataGroupCreationParameter::ValueType>(k_PSGeometry_Key);
  inputValues.PSGeometryAMName = filterArgs.value<DataObjectNameParameter::ValueType>(k_PSGeometryAMName_Key);
  inputValues.PSGeometryDataArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_PSGeometryDataName_Key);
  inputValues.GeometryToPartition = filterArgs.value<DataPath>(k_GeometryToPartition_Key);
  inputValues.PartitionIdsArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_PartitionIdsArrayName_Key);
  inputValues.ExistingPartitioningSchemePath = filterArgs.value<DataPath>(k_ExistingPartitioningSchemePath_Key);
  inputValues.UseVertexMask = filterArgs.value<bool>(k_UseVertexMask_Key);
  inputValues.VertexMaskPath = filterArgs.value<DataPath>(k_VertexMaskPath_Key);
  inputValues.FeatureAttrMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(k_FeatureAttrMatrixName_Key);

  return PartitionGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
