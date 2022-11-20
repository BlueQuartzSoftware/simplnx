#include "PartitionGeometry.hpp"

#include "complex/DataStructure/Geometry/EdgeGeom.hpp"
#include "complex/DataStructure/Geometry/HexahedralGeom.hpp"
#include "complex/DataStructure/Geometry/QuadGeom.hpp"
#include "complex/DataStructure/Geometry/RectGridGeom.hpp"
#include "complex/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"

#include "ComplexCore/Filters/PartitionGeometryFilter.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
PartitionGeometry::PartitionGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, PartitionGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
PartitionGeometry::~PartitionGeometry() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& PartitionGeometry::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<PartitionGeometry::PSGeomInfo> PartitionGeometry::GeneratePartitioningSchemeInfo(const PSInfoGenerator& psInfoGen, const DataStructure& ds, const Arguments& filterArgs)
{
  auto pPartitioningModeValue = filterArgs.value<ChoicesParameter::ValueType>(PartitionGeometryFilter::k_PartitioningMode_Key);
  auto pNumberOfPartitionsPerAxisValue = filterArgs.value<VectorInt32Parameter::ValueType>(PartitionGeometryFilter::k_NumberOfPartitionsPerAxis_Key);

  PartitionGeometry::PSGeomInfo psGeomMetadata;

  switch(static_cast<PartitionGeometryFilter::PartitioningMode>(pPartitioningModeValue))
  {
  case PartitionGeometryFilter::PartitioningMode::Basic:
  {
    IntVec3 vec3 = IntVec3(pNumberOfPartitionsPerAxisValue);

    std::optional<FloatVec3> originResult = psInfoGen.getOrigin();
    std::optional<FloatVec3> pLengthResult = psInfoGen.getPartitionLength();
    if(originResult.has_value() && pLengthResult.has_value())
    {
      psGeomMetadata.geometryDims = {static_cast<usize>(pNumberOfPartitionsPerAxisValue[0]), static_cast<usize>(pNumberOfPartitionsPerAxisValue[1]),
                                     static_cast<usize>(pNumberOfPartitionsPerAxisValue[2])};
      psGeomMetadata.geometryOrigin = *originResult;
      psGeomMetadata.geometrySpacing = *pLengthResult;
      psGeomMetadata.geometryUnits = psInfoGen.getUnits();
    }
    break;
  }
  case PartitionGeometryFilter::PartitioningMode::Advanced:
  {
    auto pPartitioningSchemeOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(PartitionGeometryFilter::k_PartitioningSchemeOrigin_Key);
    auto pLengthPerPartitionValue = filterArgs.value<VectorFloat32Parameter::ValueType>(PartitionGeometryFilter::k_LengthPerPartition_Key);

    psGeomMetadata.geometryDims = {static_cast<usize>(pNumberOfPartitionsPerAxisValue[0]), static_cast<usize>(pNumberOfPartitionsPerAxisValue[1]),
                                   static_cast<usize>(pNumberOfPartitionsPerAxisValue[2])};
    psGeomMetadata.geometryOrigin = pPartitioningSchemeOriginValue;
    psGeomMetadata.geometrySpacing = pLengthPerPartitionValue;
    psGeomMetadata.geometryUnits = psInfoGen.getUnits();
    break;
  }
  case PartitionGeometryFilter::PartitioningMode::BoundingBox:
  {
    auto pLowerLeftCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(PartitionGeometryFilter::k_LowerLeftCoord_Key);
    auto pUpperRightCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(PartitionGeometryFilter::k_UpperRightCoord_Key);

    psGeomMetadata.geometryDims = {static_cast<usize>(pNumberOfPartitionsPerAxisValue[0]), static_cast<usize>(pNumberOfPartitionsPerAxisValue[1]),
                                   static_cast<usize>(pNumberOfPartitionsPerAxisValue[2])};
    psGeomMetadata.geometryOrigin = pLowerLeftCoordValue;
    psGeomMetadata.geometrySpacing = psInfoGen.calculatePartitionLengthsUsingBounds(pLowerLeftCoordValue, pUpperRightCoordValue);
    psGeomMetadata.geometryUnits = psInfoGen.getUnits();
    break;
  }
  case PartitionGeometryFilter::PartitioningMode::ExistingPartitioningScheme:
  {
    auto pExistingPartitioningSchemePathValue = filterArgs.value<DataPath>(PartitionGeometryFilter::k_ExistingPartitioningSchemePath_Key);
    const ImageGeom& psGeom = ds.getDataRefAs<ImageGeom>(pExistingPartitioningSchemePathValue);
    psGeomMetadata.geometryDims = psGeom.getDimensions();
    psGeomMetadata.geometryOrigin = psGeom.getOrigin();
    psGeomMetadata.geometrySpacing = psGeom.getSpacing();
    psGeomMetadata.geometryUnits = psGeom.getUnits();
    break;
  }
  default:
  {
    return {MakeErrorResult<PartitionGeometry::PSGeomInfo>(-3011, "Unable to create partitioning scheme geometry - Unknown partitioning mode."), {}};
  }
  }

  return {psGeomMetadata};
}

// -----------------------------------------------------------------------------
std::string PartitionGeometry::GenerateInputGeometryDisplayText(const SizeVec3& dims, const FloatVec3& origin, const FloatVec3& spacing, const IGeometry::LengthUnit& lengthUnits)
{
  std::string lengthUnitStr = IGeometry::LengthUnitToString(lengthUnits);
  if(lengthUnits == IGeometry::LengthUnit::Unspecified)
  {
    lengthUnitStr.append(" Units");
  }
  float32 xRangeMax = origin[0] + (static_cast<float32>(dims[0]) * spacing[0]);
  float32 xDelta = static_cast<float32>(dims[0]) * spacing[0];
  float32 yRangeMax = origin[1] + (static_cast<float32>(dims[1]) * spacing[1]);
  float32 yDelta = static_cast<float32>(dims[1]) * spacing[1];
  float32 zRangeMax = origin[2] + (static_cast<float32>(dims[2]) * spacing[2]);
  float32 zDelta = static_cast<float32>(dims[2]) * spacing[2];

  std::string desc = fmt::format("X Range: {0} to {1} [{4}] (Delta: {2} [{4}]) 0-{3} Voxels\n", origin[0], xRangeMax, xDelta, dims[0] - 1, lengthUnitStr);
  desc.append(fmt::format("Y Range: {0} to {1} [{4}] (Delta: {2} [{4}]) 0-{3} Voxels\n", origin[1], yRangeMax, yDelta, dims[1] - 1, lengthUnitStr));
  desc.append(fmt::format("Z Range: {0} to {1} [{4}] (Delta: {2} [{4}]) 0-{3} Voxels\n", origin[2], zRangeMax, zDelta, dims[2] - 1, lengthUnitStr));
  return desc;
}

// -----------------------------------------------------------------------------
std::string PartitionGeometry::GeneratePartitioningSchemeDisplayText(const SizeVec3& psDims, const FloatVec3& psOrigin, const FloatVec3& psSpacing, const IGeometry::LengthUnit& lengthUnits,
                                                                     const IGeometry& iGeom)
{
  float32 xRangeMax = (psOrigin[0] + (static_cast<float32>(psDims[0]) * psSpacing[0]));
  float32 xDelta = static_cast<float32>(psDims[0]) * psSpacing[0];
  float32 yRangeMax = (psOrigin[1] + (static_cast<float32>(psDims[1]) * psSpacing[1]));
  float32 yDelta = static_cast<float32>(psDims[1]) * psSpacing[1];
  float32 zRangeMax = (psOrigin[2] + (static_cast<float32>(psDims[2]) * psSpacing[2]));
  float32 zDelta = static_cast<float32>(psDims[2]) * psSpacing[2];

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
    const ImageGeom& geometry = dynamic_cast<const ImageGeom&>(iGeom);

    SizeVec3 dims = geometry.getDimensions();
    FloatVec3 origin = geometry.getOrigin();
    FloatVec3 spacing = geometry.getSpacing();

    float32 gxRangeMax = origin[0] + (dims[0] * spacing[0]);
    float32 gyRangeMax = origin[1] + (dims[1] * spacing[1]);
    float32 gzRangeMax = origin[2] + (dims[2] * spacing[2]);

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

// -----------------------------------------------------------------------------
Result<> PartitionGeometry::operator()()
{
  PartitionGeometryFilter::PartitioningMode partitioningMode = static_cast<PartitionGeometryFilter::PartitioningMode>(m_InputValues->PartitioningMode);
  std::optional<int> outOfBoundsValue = {};

  DataPath psGeometryPath;
  if(partitioningMode == PartitionGeometryFilter::PartitioningMode::ExistingPartitioningScheme)
  {
    psGeometryPath = m_InputValues->ExistingPartitioningSchemePath;
  }
  else
  {
    psGeometryPath = m_InputValues->PSGeometryPath;
    DataPath psPartitionIdsPath = m_InputValues->PSGeometryPath.createChildPath(m_InputValues->PSGeometryAMName).createChildPath(m_InputValues->PSGeometryDataArrayName);
    Int32Array& psPartitionIds = m_DataStructure.getDataRefAs<Int32Array>({psPartitionIdsPath});

    for(usize i = 0; i < psPartitionIds.getNumberOfTuples(); i++)
    {
      psPartitionIds[i] = i + m_InputValues->StartingPartitionID;
    }
  }

  const ImageGeom& psImageGeom = m_DataStructure.getDataRefAs<ImageGeom>({psGeometryPath});

  std::optional<BoolArray> vertexMask = {};
  if(m_InputValues->UseVertexMask)
  {
    vertexMask = m_DataStructure.getDataRefAs<BoolArray>({m_InputValues->VertexMaskPath});
  }

  DataPath partitionIdsPath = m_InputValues->AttributeMatrixPath.createChildPath(m_InputValues->PartitionIdsArrayName);
  Int32Array& partitionIds = m_DataStructure.getDataRefAs<Int32Array>({partitionIdsPath});

  const IGeometry& iGeom = m_DataStructure.getDataRefAs<IGeometry>({m_InputValues->GeometryToPartition});
  Result<> result;
  switch(iGeom.getGeomType())
  {
  case IGeometry::Type::Image:
  {
    const ImageGeom& geometry = m_DataStructure.getDataRefAs<ImageGeom>({m_InputValues->GeometryToPartition});
    result = partitionCellBasedGeometry(geometry, partitionIds, psImageGeom, m_InputValues->OutOfBoundsValue);
    break;
  }
  case IGeometry::Type::RectGrid:
  {
    const RectGridGeom& geometry = m_DataStructure.getDataRefAs<RectGridGeom>({m_InputValues->GeometryToPartition});
    result = partitionCellBasedGeometry(geometry, partitionIds, psImageGeom, m_InputValues->OutOfBoundsValue);
    break;
  }
  case IGeometry::Type::Vertex:
  {
    const VertexGeom& geometry = m_DataStructure.getDataRefAs<VertexGeom>({m_InputValues->GeometryToPartition});
    const IGeometry::SharedVertexList* vertexList = geometry.getVertices();
    result = partitionNodeBasedGeometry(geometry.getName(), *vertexList, partitionIds, psImageGeom, m_InputValues->OutOfBoundsValue, vertexMask);
    break;
  }
  case IGeometry::Type::Edge:
  {
    const EdgeGeom& geometry = m_DataStructure.getDataRefAs<EdgeGeom>({m_InputValues->GeometryToPartition});
    const IGeometry::SharedVertexList* vertexList = geometry.getVertices();
    result = partitionNodeBasedGeometry(geometry.getName(), *vertexList, partitionIds, psImageGeom, m_InputValues->OutOfBoundsValue, vertexMask);
    break;
  }
  case IGeometry::Type::Triangle:
  {
    const TriangleGeom& geometry = m_DataStructure.getDataRefAs<TriangleGeom>({m_InputValues->GeometryToPartition});
    const IGeometry::SharedVertexList* vertexList = geometry.getVertices();
    result = partitionNodeBasedGeometry(geometry.getName(), *vertexList, partitionIds, psImageGeom, m_InputValues->OutOfBoundsValue, vertexMask);
    break;
  }
  case IGeometry::Type::Quad:
  {
    const QuadGeom& geometry = m_DataStructure.getDataRefAs<QuadGeom>({m_InputValues->GeometryToPartition});
    const IGeometry::SharedVertexList* vertexList = geometry.getVertices();
    result = partitionNodeBasedGeometry(geometry.getName(), *vertexList, partitionIds, psImageGeom, m_InputValues->OutOfBoundsValue, vertexMask);
    break;
  }
  case IGeometry::Type::Tetrahedral:
  {
    const TetrahedralGeom& geometry = m_DataStructure.getDataRefAs<TetrahedralGeom>({m_InputValues->GeometryToPartition});
    const IGeometry::SharedVertexList* vertexList = geometry.getVertices();
    result = partitionNodeBasedGeometry(geometry.getName(), *vertexList, partitionIds, psImageGeom, m_InputValues->OutOfBoundsValue, vertexMask);
    break;
  }
  case IGeometry::Type::Hexahedral:
  {
    const HexahedralGeom& geometry = m_DataStructure.getDataRefAs<HexahedralGeom>({m_InputValues->GeometryToPartition});
    const IGeometry::SharedVertexList* vertexList = geometry.getVertices();
    result = partitionNodeBasedGeometry(geometry.getName(), *vertexList, partitionIds, psImageGeom, m_InputValues->OutOfBoundsValue, vertexMask);
    break;
  }
  default:
  {
    return {MakeErrorResult(-3012, "Unable to partition geometry - Unknown geometry type detected."), {}};
  }
  }

  if(result.invalid())
  {
    return result;
  }

  return {};
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
Result<> PartitionGeometry::partitionCellBasedGeometry(const IGridGeometry& inputGeometry, Int32Array& partitionIds, const ImageGeom& psImageGeom, int outOfBoundsValue)
{
  SizeVec3 dims = inputGeometry.getDimensions();
  AbstractDataStore<int32>& partitionIdsStore = partitionIds.getDataStoreRef();

  for(size_t z = 0; z < dims[2]; z++)
  {
    for(size_t y = 0; y < dims[1]; y++)
    {
      for(size_t x = 0; x < dims[0]; x++)
      {
        size_t index = (z * dims[1] * dims[0]) + (y * dims[0]) + x;

        Point3D<float64> coord = inputGeometry.getCoords(x, y, z);
        auto partitionIndexResult = psImageGeom.getIndex(coord[0], coord[1], coord[2]);
        if(partitionIndexResult.has_value())
        {
          partitionIdsStore.setValue(index, *partitionIndexResult + m_InputValues->StartingPartitionID);
        }
        else
        {
          partitionIdsStore.setValue(index, outOfBoundsValue);
        }
      }
    }
  }

  return {};
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
Result<> PartitionGeometry::partitionNodeBasedGeometry(const std::string& geomName, const IGeometry::SharedVertexList& vertexList, Int32Array& partitionIds, const ImageGeom& psImageGeom,
                                                       int outOfBoundsValue, const std::optional<const BoolArray>& maskArrayOpt)
{
  AbstractDataStore<int32>& partitionIdsStore = partitionIds.getDataStoreRef();
  const AbstractDataStore<float32>& verticesStore = vertexList.getDataStoreRef();

  size_t numOfVertices = vertexList.getNumberOfTuples();
  for(size_t idx = 0; idx < numOfVertices; idx++)
  {
    float x = verticesStore[idx * 3];
    float y = verticesStore[idx * 3 + 1];
    float z = verticesStore[idx * 3 + 2];

    auto partitionIndexResult = psImageGeom.getIndex(x, y, z);
    if(maskArrayOpt.has_value() && !(*maskArrayOpt)[idx])
    {
      partitionIdsStore.setValue(idx, outOfBoundsValue);
    }
    else if(partitionIndexResult.has_value())
    {
      partitionIdsStore.setValue(idx, *partitionIndexResult + m_InputValues->StartingPartitionID);
    }
    else
    {
      partitionIdsStore.setValue(idx, outOfBoundsValue);
    }
  }

  return {};
}
