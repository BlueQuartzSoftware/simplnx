#include "PartitionGeometry.hpp"

#include "complex/DataStructure/Geometry/RectGridGeom.hpp"

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
Result<> PartitionGeometry::operator()()
{
  auto partitioningMode = static_cast<PartitionGeometryFilter::PartitioningMode>(m_InputValues->PartitioningMode);

  DataPath psGeometryPath;
  if(partitioningMode == PartitionGeometryFilter::PartitioningMode::ExistingPartitioningScheme)
  {
    psGeometryPath = m_InputValues->ExistingPartitioningSchemePath;
  }
  else
  {
    psGeometryPath = m_InputValues->PSGeometryPath;
    const DataPath psPartitionIdsPath = m_InputValues->PSGeometryPath.createChildPath(m_InputValues->PSGeometryAMName).createChildPath(m_InputValues->PSGeometryDataArrayName);
    auto& psPartitionIds = m_DataStructure.getDataRefAs<Int32Array>({psPartitionIdsPath});

    for(usize i = 0; i < psPartitionIds.getNumberOfTuples(); i++)
    {
      psPartitionIds[i] = static_cast<int32>(i) + m_InputValues->StartingPartitionID;
    }
  }

  const ImageGeom& psImageGeom = m_DataStructure.getDataRefAs<ImageGeom>({psGeometryPath});

  std::optional<BoolArray> vertexMask = {};
  if(m_InputValues->UseVertexMask)
  {
    vertexMask = m_DataStructure.getDataRefAs<BoolArray>({m_InputValues->VertexMaskPath});
  }

  const DataPath partitionIdsPath = m_InputValues->AttributeMatrixPath.createChildPath(m_InputValues->PartitionIdsArrayName);
  auto& partitionIds = m_DataStructure.getDataRefAs<Int32Array>({partitionIdsPath});

  const IGeometry& iGeom = m_DataStructure.getDataRefAs<IGeometry>({m_InputValues->GeometryToPartition});
  Result<> result;
  switch(iGeom.getGeomType())
  {
  case IGeometry::Type::Image: {
    const ImageGeom& geometry = m_DataStructure.getDataRefAs<ImageGeom>({m_InputValues->GeometryToPartition});
    result = partitionCellBasedGeometry(geometry, partitionIds, psImageGeom, m_InputValues->OutOfBoundsValue);
    break;
  }
  case IGeometry::Type::RectGrid: {
    const RectGridGeom& geometry = m_DataStructure.getDataRefAs<RectGridGeom>({m_InputValues->GeometryToPartition});
    result = partitionCellBasedGeometry(geometry, partitionIds, psImageGeom, m_InputValues->OutOfBoundsValue);
    break;
  }
  case IGeometry::Type::Vertex:
  case IGeometry::Type::Edge:
  case IGeometry::Type::Triangle:
  case IGeometry::Type::Quad:
  case IGeometry::Type::Tetrahedral:
  case IGeometry::Type::Hexahedral: {
    const INodeGeometry0D& geometry = m_DataStructure.getDataRefAs<INodeGeometry0D>({m_InputValues->GeometryToPartition});
    const IGeometry::SharedVertexList* vertexList = geometry.getVertices();
    result = partitionNodeBasedGeometry(*vertexList, partitionIds, psImageGeom, m_InputValues->OutOfBoundsValue, vertexMask);
    break;
  }
  default: {
    return {MakeErrorResult(-3012, "Unable to partition geometry - Unknown geometry type detected.")};
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

  for(usize z = 0; z < dims[2]; z++)
  {
    for(usize y = 0; y < dims[1]; y++)
    {
      for(usize x = 0; x < dims[0]; x++)
      {
        const usize index = (z * dims[1] * dims[0]) + (y * dims[0]) + x;

        Point3D<float64> coord = inputGeometry.getCoords(x, y, z);
        auto partitionIndexResult = psImageGeom.getIndex(coord[0], coord[1], coord[2]);
        if(partitionIndexResult.has_value())
        {
          partitionIdsStore.setValue(index, static_cast<int32>(*partitionIndexResult) + m_InputValues->StartingPartitionID);
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
Result<> PartitionGeometry::partitionNodeBasedGeometry(const IGeometry::SharedVertexList& vertexList, Int32Array& partitionIds, const ImageGeom& psImageGeom, int outOfBoundsValue,
                                                       const std::optional<const BoolArray>& maskArrayOpt)
{
  AbstractDataStore<int32>& partitionIdsStore = partitionIds.getDataStoreRef();
  const AbstractDataStore<float32>& verticesStore = vertexList.getDataStoreRef();

  const usize numOfVertices = vertexList.getNumberOfTuples();
  for(usize idx = 0; idx < numOfVertices; idx++)
  {
    const float32 x = verticesStore[idx * 3];
    const float32 y = verticesStore[idx * 3 + 1];
    const float32 z = verticesStore[idx * 3 + 2];

    auto partitionIndexResult = psImageGeom.getIndex(x, y, z);
    if((maskArrayOpt.has_value() && !(*maskArrayOpt)[idx]) || !partitionIndexResult.has_value())
    {
      partitionIdsStore.setValue(idx, outOfBoundsValue);
    }
    else
    {
      partitionIdsStore.setValue(idx, static_cast<int32>(*partitionIndexResult) + m_InputValues->StartingPartitionID);
    }
  }

  return {};
}
