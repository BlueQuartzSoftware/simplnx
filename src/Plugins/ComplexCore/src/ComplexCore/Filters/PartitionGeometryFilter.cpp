#include "PartitionGeometryFilter.hpp"

#include "Algorithms/PartitionGeometry.hpp"

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
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_PartitioningMode_Key, "Select the partitioning scheme", "", 0,
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
  params.linkParameters(k_PartitioningMode_Key, k_OutOfBoundsValue_Key, std::make_any<ChoicesParameter::ValueType>(1));
  params.linkParameters(k_PartitioningMode_Key, k_OutOfBoundsValue_Key, std::make_any<ChoicesParameter::ValueType>(2));
  params.linkParameters(k_PartitioningMode_Key, k_PartitioningSchemeOrigin_Key, std::make_any<ChoicesParameter::ValueType>(1));
  params.linkParameters(k_PartitioningMode_Key, k_LengthPerPartition_Key, std::make_any<ChoicesParameter::ValueType>(1));
  params.linkParameters(k_PartitioningMode_Key, k_LowerLeftCoord_Key, std::make_any<ChoicesParameter::ValueType>(2));
  params.linkParameters(k_PartitioningMode_Key, k_UpperRightCoord_Key, std::make_any<ChoicesParameter::ValueType>(2));
  params.linkParameters(k_PartitioningMode_Key, k_ExistingPartitioningSchemePath_Key, std::make_any<ChoicesParameter::ValueType>(3));
  params.linkParameters(k_UseVertexMask_Key, k_VertexMaskPath_Key, std::make_any<BoolParameter::ValueType>(true));

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
  PartitionGeometry::PartitioningMode pMode = static_cast<PartitionGeometry::PartitioningMode>(pPartitioningModeValue);
  Result<PartitionGeometry::PSGeomInfo> psMetadataResult;
  switch(iGeom.getGeomType())
  {
  case IGeometry::Type::Image:
  {
    Result<> result = PartitionGeometry::DataCheckPartitioningMode<ImageGeom>(pMode);
    if(result.invalid())
    {
      return ConvertResultTo<OutputActions>(result, {});
    }
    const ImageGeom& geometry = dataStructure.getDataRefAs<ImageGeom>({pGeometryToPartitionValue});
    if(attrMatrix.getNumTuples() != geometry.getNumberOfCells())
    {
      return {MakeErrorResult<OutputActions>(-3009, fmt::format("{}: The attribute matrix '{}' does not have the same tuple count ({}) as geometry \"{}\"'s cell count ({}).", humanName(),
                                                                attrMatrix.getName(), attrMatrix.getNumTuples(), geometry.getName(), geometry.getNumberOfCells())),
              {}};
    }
    PSImageGeomInfoGenerator pGeom(geometry, numberOfPartitionsPerAxis);
    psMetadataResult = PartitionGeometry::GeneratePartitioningSchemeInfo(pGeom, filterArgs);
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
    psMetadataResult = PartitionGeometry::GeneratePartitioningSchemeInfo(pGeom, filterArgs);
    inputGeometryInformation = "Rectilinear grid geometry space unknown during preflight.";
    break;
  }
  case IGeometry::Type::Vertex:
  {
    const VertexGeom& geometry = dataStructure.getDataRefAs<VertexGeom>({pGeometryToPartitionValue});
    const IGeometry::SharedVertexList& vertexList = geometry.getVerticesRef();
    if(attrMatrix.getNumTuples() != vertexList.getNumberOfTuples())
    {
      return {MakeErrorResult<OutputActions>(-3011, fmt::format("{}: The attribute matrix '{}' does not have the same tuple count ({}) as geometry \"{}\"'s vertex count ({}).", humanName(),
                                                                attrMatrix.getName(), attrMatrix.getNumTuples(), geometry.getName(), geometry.getNumberOfVertices())),
              {}};
    }
    PSNodeBasedGeomInfoGenerator pGeom(geometry, numberOfPartitionsPerAxis);
    psMetadataResult = PartitionGeometry::GeneratePartitioningSchemeInfo(pGeom, filterArgs);
    inputGeometryInformation = "Vertex geometry space unknown during preflight.";
    break;
  }
  case IGeometry::Type::Edge:
  {
    const EdgeGeom& geometry = dataStructure.getDataRefAs<EdgeGeom>({pGeometryToPartitionValue});
    const IGeometry::SharedVertexList& vertexList = geometry.getVerticesRef();
    if(attrMatrix.getNumTuples() != vertexList.getNumberOfTuples())
    {
      return {MakeErrorResult<OutputActions>(-3012, fmt::format("{}: The attribute matrix '{}' does not have the same tuple count ({}) as geometry \"{}\"'s vertex count ({}).", humanName(),
                                                                attrMatrix.getName(), attrMatrix.getNumTuples(), geometry.getName(), geometry.getNumberOfVertices())),
              {}};
    }
    PSNodeBasedGeomInfoGenerator pGeom(geometry, numberOfPartitionsPerAxis);
    psMetadataResult = PartitionGeometry::GeneratePartitioningSchemeInfo(pGeom, filterArgs);
    inputGeometryInformation = "Edge geometry space unknown during preflight.";
    break;
  }
  case IGeometry::Type::Triangle:
  {
    const TriangleGeom& geometry = dataStructure.getDataRefAs<TriangleGeom>({pGeometryToPartitionValue});
    const IGeometry::SharedVertexList& vertexList = geometry.getVerticesRef();
    if(attrMatrix.getNumTuples() != vertexList.getNumberOfTuples())
    {
      return {MakeErrorResult<OutputActions>(-3013, fmt::format("{}: The attribute matrix '{}' does not have the same tuple count ({}) as geometry \"{}\"'s vertex count ({}).", humanName(),
                                                                attrMatrix.getName(), attrMatrix.getNumTuples(), geometry.getName(), geometry.getNumberOfVertices())),
              {}};
    }
    PSNodeBasedGeomInfoGenerator pGeom(geometry, numberOfPartitionsPerAxis);
    psMetadataResult = PartitionGeometry::GeneratePartitioningSchemeInfo(pGeom, filterArgs);
    inputGeometryInformation = "Triangle geometry space unknown during preflight.";
    break;
  }
  case IGeometry::Type::Quad:
  {
    const QuadGeom& geometry = dataStructure.getDataRefAs<QuadGeom>({pGeometryToPartitionValue});
    const IGeometry::SharedVertexList& vertexList = geometry.getVerticesRef();
    if(attrMatrix.getNumTuples() != vertexList.getNumberOfTuples())
    {
      return {MakeErrorResult<OutputActions>(-3014, fmt::format("{}: The attribute matrix '{}' does not have the same tuple count ({}) as geometry \"{}\"'s vertex count ({}).", humanName(),
                                                                attrMatrix.getName(), attrMatrix.getNumTuples(), geometry.getName(), geometry.getNumberOfVertices())),
              {}};
    }
    PSNodeBasedGeomInfoGenerator pGeom(geometry, numberOfPartitionsPerAxis);
    psMetadataResult = PartitionGeometry::GeneratePartitioningSchemeInfo(pGeom, filterArgs);
    inputGeometryInformation = "Quad geometry space unknown during preflight.";
    break;
  }
  case IGeometry::Type::Tetrahedral:
  {
    const TetrahedralGeom& geometry = dataStructure.getDataRefAs<TetrahedralGeom>({pGeometryToPartitionValue});
    const IGeometry::SharedVertexList& vertexList = geometry.getVerticesRef();
    if(attrMatrix.getNumTuples() != vertexList.getNumberOfTuples())
    {
      return {MakeErrorResult<OutputActions>(-3015, fmt::format("{}: The attribute matrix '{}' does not have the same tuple count ({}) as geometry \"{}\"'s vertex count ({}).", humanName(),
                                                                attrMatrix.getName(), attrMatrix.getNumTuples(), geometry.getName(), geometry.getNumberOfVertices())),
              {}};
    }
    PSNodeBasedGeomInfoGenerator pGeom(geometry, numberOfPartitionsPerAxis);
    psMetadataResult = PartitionGeometry::GeneratePartitioningSchemeInfo(pGeom, filterArgs);
    inputGeometryInformation = "Tetrahedral geometry space unknown during preflight.";
    break;
  }
  case IGeometry::Type::Hexahedral:
  {
    const HexahedralGeom& geometry = dataStructure.getDataRefAs<HexahedralGeom>({pGeometryToPartitionValue});
    const IGeometry::SharedVertexList& vertexList = geometry.getVerticesRef();
    if(attrMatrix.getNumTuples() != vertexList.getNumberOfTuples())
    {
      return {MakeErrorResult<OutputActions>(-3016, fmt::format("{}: The attribute matrix '{}' does not have the same tuple count ({}) as geometry \"{}\"'s vertex count ({}).", humanName(),
                                                                attrMatrix.getName(), attrMatrix.getNumTuples(), geometry.getName(), geometry.getNumberOfVertices())),
              {}};
    }
    PSNodeBasedGeomInfoGenerator pGeom(geometry, numberOfPartitionsPerAxis);
    psMetadataResult = PartitionGeometry::GeneratePartitioningSchemeInfo(pGeom, filterArgs);
    inputGeometryInformation = "Hexahedral geometry space unknown during preflight.";
    break;
  }
  default:
  {
    return {MakeErrorResult<OutputActions>(-3017, fmt::format("{}: Unable to partition geometry - Unknown geometry type detected.", humanName())), {}};
  }
  }

  if(psMetadataResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(ConvertResult(std::move(psMetadataResult))), {})};
  }

  complex::Result<OutputActions> resultOutputActions;

  DataPath dap = pAttributeMatrixPathValue;
  dap = dap.createChildPath(pPartitionIdsArrayNameValue);
  auto action = std::make_unique<CreateArrayAction>(DataType::int32, attrMatrix.getShape(), std::vector<usize>{1}, dap);
  resultOutputActions.value().actions.push_back(std::move(action));

  std::string partitioningSchemeInformation;

  auto psMetadata = psMetadataResult.value();
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

  auto createImageGeometryAction = std::make_unique<CreateImageGeometryAction>(pPSGeometryValue, psDims, psOrigin, psSpacing, pPSGeometryAMNameValue);
  resultOutputActions.value().actions.push_back(std::move(createImageGeometryAction));

  dap = pPSGeometryValue;
  dap = dap.createChildPath(pPSGeometryAMNameValue).createChildPath(pPSGeometryDataArrayNameValue);
  action = std::make_unique<CreateArrayAction>(DataType::int32, psDims, std::vector<usize>{1}, dap);
  resultOutputActions.value().actions.push_back(std::move(action));

  std::vector<PreflightValue> preflightUpdatedValues;
  preflightUpdatedValues.push_back({"Partitioning Scheme Information", partitioningSchemeInformation});
  preflightUpdatedValues.push_back({"Input Geometry Information", inputGeometryInformation});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

// -----------------------------------------------------------------------------
template <typename GeomType>
Result<> PartitionGeometryFilter::dataCheckPartitioningMode(PartitioningMode partitioningMode)
{
  switch(partitioningMode)
  {
  case PartitioningMode::Basic:
    DataCheckBasicMode<GeomType>();
    break;
  case PartitioningMode::Advanced:
    DataCheckAdvancedMode<GeomType>();
    break;
  case PartitioningMode::BoundingBox:
    DataCheckBoundingBoxMode<GeomType>();
    break;
  case PartitioningMode::ExistingPartitioningScheme:
    DataCheckExistingGeometryMode();
    break;
  }
}

// -----------------------------------------------------------------------------
template <typename GeomType>
void PartitionGeometryFilter::dataCheckBasicMode()
{
  DataCheckNumberOfPartitions();
  if(getErrorCode() != 0)
  {
    return;
  }

  dataCheckPartitioningScheme<GeomType>();
  if(getErrorCode() != 0)
  {
    return;
  }
}

// -----------------------------------------------------------------------------
template <typename GeomType>
void PartitionGeometryFilter::dataCheckAdvancedMode()
{
  dataCheckNumberOfPartitions();
  if(getErrorCode() != 0)
  {
    return;
  }

  if(m_LengthPerPartition.getX() < 0)
  {
    QString ss = QObject::tr("Length Per Partition: The X value cannot be negative.");
    setErrorCondition(-3003, ss);
    return;
  }
  if(m_LengthPerPartition.getY() < 0)
  {
    QString ss = QObject::tr("Length Per Partition: The Y value cannot be negative.");
    setErrorCondition(-3004, ss);
    return;
  }
  if(m_LengthPerPartition.getZ() < 0)
  {
    QString ss = QObject::tr("Length Per Partition: The Z value cannot be negative.");
    setErrorCondition(-3005, ss);
    return;
  }

  dataCheckPartitioningScheme<GeomType>();
  if(getErrorCode() != 0)
  {
    return;
  }
}

// -----------------------------------------------------------------------------
template <typename GeomType>
Result<> PartitionGeometryFilter::dataCheckBoundingBoxMode()
{
  Result<> result = dataCheckNumberOfPartitions();
  if(result.invalid())
  {
    return result;
  }

  if(m_LowerLeftCoord.getX() > m_UpperRightCoord.getX())
  {
    QString ss = QObject::tr("Lower Left Coordinate: X value is larger than the upper right coordinate X value.");
    setErrorCondition(-3006, ss);
    return;
  }

  if(m_LowerLeftCoord.getY() > m_UpperRightCoord.getY())
  {
    QString ss = QObject::tr("Lower Left Coordinate: Y value is larger than the upper right coordinate Y value.");
    setErrorCondition(-3007, ss);
    return;
  }

  if(m_LowerLeftCoord.getZ() > m_UpperRightCoord.getZ())
  {
    QString ss = QObject::tr("Lower Left Coordinate: Z value is larger than the upper right coordinate Z value.");
    setErrorCondition(-3008, ss);
    return;
  }

  result = dataCheckPartitioningScheme<GeomType>();
  if(result.invalid())
  {
    return result;
  }

  return {};
}

// -----------------------------------------------------------------------------
void PartitionGeometryFilter::dataCheckExistingGeometryMode()
{
  m_PartitionImageGeometryResult = {existingGeom, {}};
}

// -----------------------------------------------------------------------------
template <typename GeomType>
Result<> PartitionGeometryFilter::dataCheckPartitioningScheme()
{
  DataContainer::Pointer dc = getDataContainerArray()->getDataContainer(m_AttributeMatrixPath.getDataContainerName());
  AttributeMatrix::Pointer am = dc->getAttributeMatrix(m_AttributeMatrixPath.getAttributeMatrixName());

  typename GeomType::Pointer geometry = dc->getGeometryAs<GeomType>();
  if constexpr(std::is_same_v<GeomType, ImageGeom> || std::is_same_v<GeomType, RectGridGeom>)
  {
    if(am->getNumberOfTuples() != geometry->getNumberOfElements())
    {
      QString ss = QObject::tr("The attribute matrix '%1' does not have the same tuple count (%2) as data container \"%3\"'s cell count (%4).")
                       .arg(am->getName(), QString::number(am->getNumberOfTuples()), dc->getName(), QString::number(geometry->getNumberOfElements()));
      setErrorCondition(-3009, ss);
      return;
    }
  }
  else
  {
    SharedVertexList::Pointer vertexList = geometry->getVertices();
    if(am->getNumberOfTuples() != vertexList->getNumberOfTuples())
    {
      QString ss = QObject::tr("The attribute matrix \"%1\" does not have the same tuple count (%2) as data container \"%3\"'s vertex count (%4).")
                       .arg(am->getName(), QString::number(am->getNumberOfTuples()), dc->getName(), QString::number(vertexList->getNumberOfTuples()));
      setErrorCondition(-3010, ss);
      return;
    }
  }

  createPartitioningSchemeGeometry(*geometry);
  if(getErrorCode() != 0)
  {
    return;
  }
}

// -----------------------------------------------------------------------------
Result<> PartitionGeometryFilter::dataCheckNumberOfPartitions(const SizeVec3& numberOfPartitionsPerAxis)
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

  return PartitionGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
