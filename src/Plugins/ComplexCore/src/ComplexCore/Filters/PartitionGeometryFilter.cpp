#include "PartitionGeometryFilter.hpp"

#include "ComplexCore/utils/PSImageGeomInfoGenerator.hpp"
#include "ComplexCore/utils/PSNodeBasedGeomInfoGenerator.hpp"
#include "ComplexCore/utils/PSRectGridGeomInfoGenerator.hpp"
#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/Geometry/EdgeGeom.hpp"
#include "complex/DataStructure/Geometry/HexahedralGeom.hpp"
#include "complex/DataStructure/Geometry/QuadGeom.hpp"
#include "complex/DataStructure/Geometry/RectGridGeom.hpp"
#include "complex/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

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
  return {"#Reconstruction", "#Reconstruction"};
}

//------------------------------------------------------------------------------
Parameters PartitionGeometryFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Geometry Details"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_GeometryToPartition_Key, "Geometry to Partition", "", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex, IGeometry::Type::Edge, IGeometry::Type::Triangle, IGeometry::Type::Quad,
                                                                                                      IGeometry::Type::Tetrahedral, IGeometry::Type::Hexahedral, IGeometry::Type::Image,
                                                                                                      IGeometry::Type::RectGrid}));
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_AttributeMatrixPath_Key, "Attribute Matrix (Vertex=>Node Geometry, Cell=>Image/Rectilinear)", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Partitioning Scheme Details"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_PartitioningMode_Key, "Select the partitioning mode", "", 0,
                                                                    ChoicesParameter::Choices{"Basic", "Advanced", "Bounding Box", "Existing Partitioning Scheme"}));
  params.insert(std::make_unique<Int32Parameter>(k_StartingPartitionID_Key, "Starting Partition ID", "", 1));
  params.insert(std::make_unique<Int32Parameter>(k_OutOfBoundsValue_Key, "Out-Of-Bounds Partition ID", "", 0));
  params.insert(std::make_unique<VectorInt32Parameter>(k_NumberOfPartitionsPerAxis_Key, "Number Of Partitions Per Axis (X, Y, Z)", "", std::vector<int32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_PartitioningSchemeOrigin_Key, "Partitioning Scheme Origin (X, Y, Z)", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_LengthPerPartition_Key, "Length Per Partition (X, Y, Z)", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_LowerLeftCoord_Key, "Lower Left Coordinate (X, Y, Z)", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_UpperRightCoord_Key, "Upper Right Coordinate (X, Y, Z)", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ExistingPartitioningSchemePath_Key, "Existing Partitioning Scheme", "", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseVertexMask_Key, "Use Vertex Mask", "", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_VertexMaskPath_Key, "Vertex Mask", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::boolean},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Created Geometry Objects"});
  params.insert(std::make_unique<StringParameter>(k_FeatureAttrMatrixName_Key, "Feature Attribute Matrix Name", "", "Feature Data"));
  params.insert(std::make_unique<StringParameter>(k_PartitionIdsArrayName_Key, "Partition Ids Array Name", "", "Partition Ids"));

  params.insertSeparator(Parameters::Separator{"Created Partitioning Scheme Objects"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_PSGeometry_Key, "Partitioning Scheme Geometry", "The complete path to the Partitioning Scheme Geometry being created",
                                                             DataPath({"[Partitioning Scheme Geometry]"})));
  params.insert(std::make_unique<StringParameter>(k_PSGeometryAMName_Key, "Partitioning Scheme Attribute Matrix", "", "Cell Data"));
  params.insert(std::make_unique<StringParameter>(k_PSGeometryDataName_Key, "Partitioning Scheme Ids", "", "PartitioningSchemeIds"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_PartitioningMode_Key, k_StartingPartitionID_Key, std::make_any<ChoicesParameter::ValueType>(0));
  params.linkParameters(k_PartitioningMode_Key, k_StartingPartitionID_Key, std::make_any<ChoicesParameter::ValueType>(1));
  params.linkParameters(k_PartitioningMode_Key, k_StartingPartitionID_Key, std::make_any<ChoicesParameter::ValueType>(2));
  params.linkParameters(k_PartitioningMode_Key, k_OutOfBoundsValue_Key, std::make_any<ChoicesParameter::ValueType>(1));
  params.linkParameters(k_PartitioningMode_Key, k_OutOfBoundsValue_Key, std::make_any<ChoicesParameter::ValueType>(2));
  params.linkParameters(k_PartitioningMode_Key, k_OutOfBoundsValue_Key, std::make_any<ChoicesParameter::ValueType>(3));
  params.linkParameters(k_PartitioningMode_Key, k_NumberOfPartitionsPerAxis_Key, std::make_any<ChoicesParameter::ValueType>(0));
  params.linkParameters(k_PartitioningMode_Key, k_NumberOfPartitionsPerAxis_Key, std::make_any<ChoicesParameter::ValueType>(1));
  params.linkParameters(k_PartitioningMode_Key, k_NumberOfPartitionsPerAxis_Key, std::make_any<ChoicesParameter::ValueType>(2));
  params.linkParameters(k_PartitioningMode_Key, k_PartitioningSchemeOrigin_Key, std::make_any<ChoicesParameter::ValueType>(1));
  params.linkParameters(k_PartitioningMode_Key, k_LengthPerPartition_Key, std::make_any<ChoicesParameter::ValueType>(1));
  params.linkParameters(k_PartitioningMode_Key, k_LowerLeftCoord_Key, std::make_any<ChoicesParameter::ValueType>(2));
  params.linkParameters(k_PartitioningMode_Key, k_UpperRightCoord_Key, std::make_any<ChoicesParameter::ValueType>(2));
  params.linkParameters(k_PartitioningMode_Key, k_ExistingPartitioningSchemePath_Key, std::make_any<ChoicesParameter::ValueType>(3));
  params.linkParameters(k_UseVertexMask_Key, k_VertexMaskPath_Key, std::make_any<BoolParameter::ValueType>(true));
  params.linkParameters(k_PartitioningMode_Key, k_PSGeometry_Key, std::make_any<ChoicesParameter::ValueType>(0));
  params.linkParameters(k_PartitioningMode_Key, k_PSGeometry_Key, std::make_any<ChoicesParameter::ValueType>(1));
  params.linkParameters(k_PartitioningMode_Key, k_PSGeometry_Key, std::make_any<ChoicesParameter::ValueType>(2));
  params.linkParameters(k_PartitioningMode_Key, k_PSGeometryAMName_Key, std::make_any<ChoicesParameter::ValueType>(0));
  params.linkParameters(k_PartitioningMode_Key, k_PSGeometryAMName_Key, std::make_any<ChoicesParameter::ValueType>(1));
  params.linkParameters(k_PartitioningMode_Key, k_PSGeometryAMName_Key, std::make_any<ChoicesParameter::ValueType>(2));
  params.linkParameters(k_PartitioningMode_Key, k_PSGeometryDataName_Key, std::make_any<ChoicesParameter::ValueType>(0));
  params.linkParameters(k_PartitioningMode_Key, k_PSGeometryDataName_Key, std::make_any<ChoicesParameter::ValueType>(1));
  params.linkParameters(k_PartitioningMode_Key, k_PSGeometryDataName_Key, std::make_any<ChoicesParameter::ValueType>(2));

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

  SizeVec3 numberOfPartitionsPerAxis = {static_cast<usize>(pNumberOfPartitionsPerAxisValue[0]), static_cast<usize>(pNumberOfPartitionsPerAxisValue[1]),
                                        static_cast<usize>(pNumberOfPartitionsPerAxisValue[2])};

  const AttributeMatrix& attrMatrix = dataStructure.getDataRefAs<AttributeMatrix>(pAttributeMatrixPathValue);
  const IGeometry& iGeom = dataStructure.getDataRefAs<IGeometry>({pGeometryToPartitionValue});
  std::string inputGeometryInformation;
  Result<PartitionGeometry::PSGeomInfo> psInfo;
  switch(iGeom.getGeomType())
  {
  case IGeometry::Type::Image:
  {
    const ImageGeom& geometry = dataStructure.getDataRefAs<ImageGeom>({pGeometryToPartitionValue});
    Result<> result = dataCheckPartitioningMode<ImageGeom>(dataStructure, filterArgs, geometry);
    if(result.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(result), {})};
    }
    PSImageGeomInfoGenerator pGeom(geometry, numberOfPartitionsPerAxis);
    psInfo = PartitionGeometry::GeneratePartitioningSchemeInfo(pGeom, dataStructure, filterArgs);
    inputGeometryInformation = PartitionGeometry::GenerateInputGeometryDisplayText(geometry.getDimensions(), geometry.getOrigin(), geometry.getSpacing(), geometry.getUnits());
    break;
  }
  case IGeometry::Type::RectGrid:
  {
    const RectGridGeom& geometry = dataStructure.getDataRefAs<RectGridGeom>({pGeometryToPartitionValue});
    if(attrMatrix.getNumTuples() != geometry.getNumberOfCells())
    {
      return {MakeErrorResult<OutputActions>(-3010, fmt::format("{}: The attribute matrix '{}' does not have the same tuple count ({}) as geometry \"{}\"'s cell count ({}).", humanName(),
                                                                attrMatrix.getName(), attrMatrix.getNumTuples(), geometry.getName(), geometry.getNumberOfCells())),
              {}};
    }
    PSRectGridGeomInfoGenerator pGeom(geometry, numberOfPartitionsPerAxis);
    psInfo = PartitionGeometry::GeneratePartitioningSchemeInfo(pGeom, dataStructure, filterArgs);
    inputGeometryInformation = "Rectilinear grid geometry space unknown during preflight.";
    break;
  }
  case IGeometry::Type::Vertex:
  {
    psInfo = generateNodeBasedPSInfo(dataStructure, filterArgs, pGeometryToPartitionValue, pAttributeMatrixPathValue, numberOfPartitionsPerAxis);
    if(psInfo.invalid())
    {
      return {ConvertResultTo<OutputActions>(ConvertResult(std::move(psInfo)), {})};
    }
    inputGeometryInformation = "Vertex geometry space unknown during preflight.";
    break;
  }
  case IGeometry::Type::Edge:
  {
    psInfo = generateNodeBasedPSInfo(dataStructure, filterArgs, pGeometryToPartitionValue, pAttributeMatrixPathValue, numberOfPartitionsPerAxis);
    if(psInfo.invalid())
    {
      return {ConvertResultTo<OutputActions>(ConvertResult(std::move(psInfo)), {})};
    }
    inputGeometryInformation = "Edge geometry space unknown during preflight.";
    break;
  }
  case IGeometry::Type::Triangle:
  {
    psInfo = generateNodeBasedPSInfo(dataStructure, filterArgs, pGeometryToPartitionValue, pAttributeMatrixPathValue, numberOfPartitionsPerAxis);
    if(psInfo.invalid())
    {
      return {ConvertResultTo<OutputActions>(ConvertResult(std::move(psInfo)), {})};
    }
    inputGeometryInformation = "Triangle geometry space unknown during preflight.";
    break;
  }
  case IGeometry::Type::Quad:
  {
    psInfo = generateNodeBasedPSInfo(dataStructure, filterArgs, pGeometryToPartitionValue, pAttributeMatrixPathValue, numberOfPartitionsPerAxis);
    if(psInfo.invalid())
    {
      return {ConvertResultTo<OutputActions>(ConvertResult(std::move(psInfo)), {})};
    }
    inputGeometryInformation = "Quad geometry space unknown during preflight.";
    break;
  }
  case IGeometry::Type::Tetrahedral:
  {
    psInfo = generateNodeBasedPSInfo(dataStructure, filterArgs, pGeometryToPartitionValue, pAttributeMatrixPathValue, numberOfPartitionsPerAxis);
    if(psInfo.invalid())
    {
      return {ConvertResultTo<OutputActions>(ConvertResult(std::move(psInfo)), {})};
    }
    inputGeometryInformation = "Tetrahedral geometry space unknown during preflight.";
    break;
  }
  case IGeometry::Type::Hexahedral:
  {
    psInfo = generateNodeBasedPSInfo(dataStructure, filterArgs, pGeometryToPartitionValue, pAttributeMatrixPathValue, numberOfPartitionsPerAxis);
    if(psInfo.invalid())
    {
      return {ConvertResultTo<OutputActions>(ConvertResult(std::move(psInfo)), {})};
    }
    inputGeometryInformation = "Hexahedral geometry space unknown during preflight.";
    break;
  }
  default:
  {
    return {MakeErrorResult<OutputActions>(-3017, fmt::format("{}: Unable to partition geometry - Unknown geometry type detected.", humanName())), {}};
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
  std::vector<usize> psDims = {psMetadata.geometryDims[0], psMetadata.geometryDims[1], psMetadata.geometryDims[2]};
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
    partitioningSchemeInformation = PartitionGeometry::GeneratePartitioningSchemeDisplayText(psDims, psOrigin, psSpacing, psMetadata.geometryUnits, iGeom);
  }

  if(static_cast<PartitionGeometryFilter::PartitioningMode>(pPartitioningModeValue) != PartitionGeometryFilter::PartitioningMode::ExistingPartitioningScheme)
  {
    auto createImageGeometryAction = std::make_unique<CreateImageGeometryAction>(pPSGeometryValue, psDims, psOrigin, psSpacing, pPSGeometryAMNameValue);
    resultOutputActions.value().actions.push_back(std::move(createImageGeometryAction));

    dap = pPSGeometryValue;
    dap = dap.createChildPath(pPSGeometryAMNameValue).createChildPath(pPSGeometryDataArrayNameValue);
    action = std::make_unique<CreateArrayAction>(DataType::int32, psDims, std::vector<usize>{1}, dap);
    resultOutputActions.value().actions.push_back(std::move(action));
  }

  std::vector<PreflightValue> preflightUpdatedValues;
  preflightUpdatedValues.push_back({"Partitioning Scheme Information", partitioningSchemeInformation});
  preflightUpdatedValues.push_back({"Input Geometry Information", inputGeometryInformation});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

// -----------------------------------------------------------------------------
Result<PartitionGeometry::PSGeomInfo> PartitionGeometryFilter::generateNodeBasedPSInfo(const DataStructure& dataStructure, const Arguments& filterArgs, const DataPath& geometryToPartitionPath,
                                                                                       const DataPath& attrMatrixPath, const SizeVec3& numberOfPartitionsPerAxis) const
{
  const INodeGeometry0D& geometry = dataStructure.getDataRefAs<INodeGeometry0D>({geometryToPartitionPath});
  const IGeometry::SharedVertexList& vertexList = geometry.getVerticesRef();
  const AttributeMatrix& attrMatrix = dataStructure.getDataRefAs<AttributeMatrix>(attrMatrixPath);
  if(attrMatrix.getNumTuples() != vertexList.getNumberOfTuples())
  {
    return {MakeErrorResult<PartitionGeometry::PSGeomInfo>(-3014, fmt::format("{}: The attribute matrix '{}' does not have the same tuple count ({}) as geometry \"{}\"'s vertex count ({}).",
                                                                              humanName(), attrMatrix.getName(), attrMatrix.getNumTuples(), geometry.getName(), geometry.getNumberOfVertices())),
            {}};
  }
  PSNodeBasedGeomInfoGenerator pGeom(geometry, numberOfPartitionsPerAxis);
  Result<> dimensionalityResult = pGeom.checkDimensionality();
  if(dimensionalityResult.invalid())
  {
    return {ConvertResultTo<PartitionGeometry::PSGeomInfo>(std::move(dimensionalityResult), {})};
  }
  return PartitionGeometry::GeneratePartitioningSchemeInfo(pGeom, dataStructure, filterArgs);
}

// -----------------------------------------------------------------------------
template <typename GeomType>
Result<> PartitionGeometryFilter::dataCheckPartitioningMode(const DataStructure& dataStructure, const Arguments& filterArgs, const GeomType& geometryToPartition) const
{
  auto pPartitioningModeValue = filterArgs.value<ChoicesParameter::ValueType>(k_PartitioningMode_Key);
  PartitioningMode partitioningMode = static_cast<PartitioningMode>(pPartitioningModeValue);

  auto pNumberOfPartitionsPerAxisValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_NumberOfPartitionsPerAxis_Key);
  SizeVec3 numOfPartitionsPerAxis = {static_cast<usize>(pNumberOfPartitionsPerAxisValue[0]), static_cast<usize>(pNumberOfPartitionsPerAxisValue[1]),
                                     static_cast<usize>(pNumberOfPartitionsPerAxisValue[2])};

  auto pLengthPerPartitionValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_LengthPerPartition_Key);
  auto pLowerLeftCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_LowerLeftCoord_Key);
  auto pUpperRightCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_UpperRightCoord_Key);

  auto pAttributeMatrixPathValue = filterArgs.value<DataPath>(k_AttributeMatrixPath_Key);
  const AttributeMatrix& attrMatrix = dataStructure.getDataRefAs<AttributeMatrix>({pAttributeMatrixPathValue});

  switch(partitioningMode)
  {
  case PartitioningMode::Basic:
    return dataCheckBasicMode<GeomType>(numOfPartitionsPerAxis, geometryToPartition, attrMatrix);
  case PartitioningMode::Advanced:
    return dataCheckAdvancedMode<GeomType>(numOfPartitionsPerAxis, pLengthPerPartitionValue, geometryToPartition, attrMatrix);
  case PartitioningMode::BoundingBox:
    return dataCheckBoundingBoxMode<GeomType>(numOfPartitionsPerAxis, pLowerLeftCoordValue, pUpperRightCoordValue, geometryToPartition, attrMatrix);
  case PartitioningMode::ExistingPartitioningScheme:
    return dataCheckExistingGeometryMode();
    break;
  }

  return {};
}

// -----------------------------------------------------------------------------
template <typename GeomType>
Result<> PartitionGeometryFilter::dataCheckBasicMode(const SizeVec3& numOfPartitionsPerAxis, const GeomType& geometryToPartition, const AttributeMatrix& attrMatrix) const
{
  Result<> result = dataCheckNumberOfPartitions(numOfPartitionsPerAxis);
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
  Result<> result = dataCheckNumberOfPartitions(numOfPartitionsPerAxis);
  if(result.invalid())
  {
    return result;
  }

  if(lengthPerPartition.getX() < 0)
  {
    return {MakeErrorResult(-3003, fmt::format("{}: Length Per Partition - The X value cannot be negative.", humanName())), {}};
  }
  if(lengthPerPartition.getY() < 0)
  {
    return {MakeErrorResult(-3004, fmt::format("{}: Length Per Partition - The Y value cannot be negative.", humanName())), {}};
  }
  if(lengthPerPartition.getZ() < 0)
  {
    return {MakeErrorResult(-3005, fmt::format("{}: Length Per Partition - The Z value cannot be negative.", humanName())), {}};
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
  Result<> result = dataCheckNumberOfPartitions(numOfPartitionsPerAxis);
  if(result.invalid())
  {
    return result;
  }

  if(llCoord.getX() > urCoord.getX())
  {
    return {MakeErrorResult(-3006, fmt::format("{}: Lower Left Coordinate - X value is larger than the upper right coordinate X value.", humanName())), {}};
  }

  if(llCoord.getY() > urCoord.getY())
  {
    return {MakeErrorResult(-3007, fmt::format("{}: Lower Left Coordinate - Y value is larger than the upper right coordinate Y value.", humanName())), {}};
  }

  if(llCoord.getZ() > urCoord.getZ())
  {
    return {MakeErrorResult(-3008, fmt::format("{}: Lower Left Coordinate - Z value is larger than the upper right coordinate Z value.", humanName())), {}};
  }

  result = dataCheckPartitioningScheme<GeomType>(geometryToPartition, attrMatrix);
  if(result.invalid())
  {
    return result;
  }

  return {};
}

// -----------------------------------------------------------------------------
Result<> PartitionGeometryFilter::dataCheckExistingGeometryMode() const
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
                                                 attrMatrix.getNumTuples(), geometryToPartition.getName(), geometryToPartition.getNumberOfCells())),
              {}};
    }
  }
  else
  {
    const IGeometry::SharedVertexList& vertexList = geometryToPartition.getVertices();
    if(attrMatrix.getNumTuples() != vertexList.getNumberOfTuples())
    {
      return {MakeErrorResult(-3010, fmt::format("{}: The attribute matrix '{}' does not have the same tuple count ({}) as geometry \"{}\"'s vertex count ({}).", humanName(), attrMatrix.getName(),
                                                 attrMatrix.getNumTuples(), geometryToPartition.getName(), vertexList.getNumberOfTuples())),
              {}};
    }
  }

  return {};
}

// -----------------------------------------------------------------------------
Result<> PartitionGeometryFilter::dataCheckNumberOfPartitions(const SizeVec3& numberOfPartitionsPerAxis) const
{
  if(numberOfPartitionsPerAxis.getX() <= 0)
  {
    return {MakeErrorResult(-3012, "Number of Partitions Per Axis: The X dimension must be greater than 0."), {}};
  }

  if(numberOfPartitionsPerAxis.getY() <= 0)
  {
    return {MakeErrorResult(-3013, "Number of Partitions Per Axis: The Y dimension must be greater than 0."), {}};
  }

  if(numberOfPartitionsPerAxis.getZ() <= 0)
  {
    return {MakeErrorResult(-3014, "Number of Partitions Per Axis: The Z dimension must be greater than 0."), {}};
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
  inputValues.PSGeometryAMName = filterArgs.value<StringParameter::ValueType>(k_PSGeometryAMName_Key);
  inputValues.PSGeometryDataArrayName = filterArgs.value<StringParameter::ValueType>(k_PSGeometryDataName_Key);
  inputValues.GeometryToPartition = filterArgs.value<DataPath>(k_GeometryToPartition_Key);
  inputValues.PartitionIdsArrayName = filterArgs.value<std::string>(k_PartitionIdsArrayName_Key);
  inputValues.ExistingPartitioningSchemePath = filterArgs.value<DataPath>(k_ExistingPartitioningSchemePath_Key);
  inputValues.UseVertexMask = filterArgs.value<bool>(k_UseVertexMask_Key);
  inputValues.VertexMaskPath = filterArgs.value<DataPath>(k_VertexMaskPath_Key);
  inputValues.FeatureAttrMatrixName = filterArgs.value<std::string>(k_FeatureAttrMatrixName_Key);

  return PartitionGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
