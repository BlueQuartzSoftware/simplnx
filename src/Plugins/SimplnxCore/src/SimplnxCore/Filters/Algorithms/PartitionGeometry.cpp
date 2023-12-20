#include "PartitionGeometry.hpp"

#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/Utilities/ParallelData3DAlgorithm.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include "SimplnxCore/Filters/PartitionGeometryFilter.hpp"

using namespace nx::core;

namespace
{
// -----------------------------------------------------------------------------
class PartitionCellBasedGeometryImpl
{
public:
  PartitionCellBasedGeometryImpl(const IGridGeometry& inputGeometry, Int32Array& partitionIds, const ImageGeom& psImageGeom, int startingPartitionId, int outOfBoundsValue,
                                 const std::atomic_bool& shouldCancel)
  : m_InputGeometry(inputGeometry)
  , m_PartitionIds(partitionIds)
  , m_PSImageGeom(psImageGeom)
  , m_StartingPartitionId(startingPartitionId)
  , m_OutOfBoundsValue(outOfBoundsValue)
  , m_ShouldCancel(shouldCancel)
  {
  }
  ~PartitionCellBasedGeometryImpl() = default;
  PartitionCellBasedGeometryImpl(const PartitionCellBasedGeometryImpl&) = default;           // Copy Constructor default Implemented
  PartitionCellBasedGeometryImpl(PartitionCellBasedGeometryImpl&&) = delete;                 // Move Constructor Not Implemented
  PartitionCellBasedGeometryImpl& operator=(const PartitionCellBasedGeometryImpl&) = delete; // Copy Assignment Not Implemented
  PartitionCellBasedGeometryImpl& operator=(PartitionCellBasedGeometryImpl&&) = delete;      // Move Assignment Not Implemented

  // -----------------------------------------------------------------------------
  void compute(size_t xStart, size_t xEnd, size_t yStart, size_t yEnd, size_t zStart, size_t zEnd) const
  {
    AbstractDataStore<int32>& partitionIdsStore = m_PartitionIds.getDataStoreRef();
    SizeVec3 dims = m_InputGeometry.getDimensions();

    for(usize z = zStart; z < zEnd; z++)
    {
      for(usize y = yStart; y < yEnd; y++)
      {
        for(usize x = xStart; x < xEnd; x++)
        {
          if(m_ShouldCancel)
          {
            return;
          }

          const usize index = (z * dims[1] * dims[0]) + (y * dims[0]) + x;

          Point3D<float64> coord = m_InputGeometry.getCoords(x, y, z);
          auto partitionIndexResult = m_PSImageGeom.getIndex(coord[0], coord[1], coord[2]);
          if(partitionIndexResult.has_value())
          {
            partitionIdsStore.setValue(index, static_cast<int32>(*partitionIndexResult) + m_StartingPartitionId);
            // partitionIdsStore[index] = static_cast<int32>(*partitionIndexResult) + m_StartingPartitionId;
          }
          else
          {
            partitionIdsStore.setValue(index, m_OutOfBoundsValue);
            // partitionIdsStore[index] = m_OutOfBoundsValue;
          }
        }
      }
    }
  }

  void operator()(const Range3D& r) const
  {
    compute(r[0], r[1], r[2], r[3], r[4], r[5]);
  }

private:
  const IGridGeometry& m_InputGeometry;
  Int32Array& m_PartitionIds;
  const ImageGeom& m_PSImageGeom;
  int m_StartingPartitionId;
  int m_OutOfBoundsValue;
  const std::atomic_bool& m_ShouldCancel;
};

// -----------------------------------------------------------------------------
class PartitionNodeBasedGeometryImpl
{
public:
  PartitionNodeBasedGeometryImpl(const IGeometry::SharedVertexList& vertexList, Int32Array& partitionIds, const ImageGeom& psImageGeom, int startingPartitionId, int outOfBoundsValue,
                                 const std::optional<const BoolArray>& maskArrayOpt, const std::atomic_bool& shouldCancel)
  : m_VertexList(vertexList)
  , m_PartitionIds(partitionIds)
  , m_PSImageGeom(psImageGeom)
  , m_StartingPartitionId(startingPartitionId)
  , m_OutOfBoundsValue(outOfBoundsValue)
  , m_MaskArrayOpt(maskArrayOpt)
  , m_ShouldCancel(shouldCancel)
  {
  }
  ~PartitionNodeBasedGeometryImpl() = default;
  PartitionNodeBasedGeometryImpl(const PartitionNodeBasedGeometryImpl&) = default;           // Copy Constructor default Implemented
  PartitionNodeBasedGeometryImpl(PartitionNodeBasedGeometryImpl&&) = delete;                 // Move Constructor Not Implemented
  PartitionNodeBasedGeometryImpl& operator=(const PartitionNodeBasedGeometryImpl&) = delete; // Copy Assignment Not Implemented
  PartitionNodeBasedGeometryImpl& operator=(PartitionNodeBasedGeometryImpl&&) = delete;      // Move Assignment Not Implemented

  // -----------------------------------------------------------------------------
  void compute(size_t start, size_t end) const
  {
    AbstractDataStore<int32>& partitionIdsStore = m_PartitionIds.getDataStoreRef();
    const AbstractDataStore<float32>& verticesStore = m_VertexList.getDataStoreRef();

    for(usize idx = start; idx < end; idx++)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      const float32 x = verticesStore[idx * 3];
      const float32 y = verticesStore[idx * 3 + 1];
      const float32 z = verticesStore[idx * 3 + 2];

      auto partitionIndexResult = m_PSImageGeom.getIndex(x, y, z);
      if((m_MaskArrayOpt.has_value() && !(*m_MaskArrayOpt)[idx]) || !partitionIndexResult.has_value())
      {
        partitionIdsStore.setValue(idx, m_OutOfBoundsValue);
      }
      else
      {
        partitionIdsStore.setValue(idx, static_cast<int32>(*partitionIndexResult) + m_StartingPartitionId);
      }
    }
  }

  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  const IGeometry::SharedVertexList& m_VertexList;
  Int32Array& m_PartitionIds;
  const ImageGeom& m_PSImageGeom;
  int m_StartingPartitionId;
  int m_OutOfBoundsValue;
  const std::optional<const BoolArray>& m_MaskArrayOpt;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace

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

  DataPath partitionGridGeomPath;
  if(partitioningMode == PartitionGeometryFilter::PartitioningMode::ExistingPartitionGrid)
  {
    partitionGridGeomPath = m_InputValues->ExistingPartitionGridPath;
  }
  else
  {
    partitionGridGeomPath = m_InputValues->PartitionGridGeomPath;
    const DataPath partitionGridFeatureIdsPath =
        m_InputValues->PartitionGridGeomPath.createChildPath(m_InputValues->PartitionGridCellAMName).createChildPath(m_InputValues->PartitionGridFeatureIDsArrayName);
    auto& partitionGridFeatureIds = m_DataStructure.getDataRefAs<Int32Array>({partitionGridFeatureIdsPath});

    for(usize i = 0; i < partitionGridFeatureIds.getNumberOfTuples(); i++)
    {
      partitionGridFeatureIds[i] = static_cast<int32>(i) + m_InputValues->StartingFeatureID;
    }
  }

  const ImageGeom& partitionGridGeom = m_DataStructure.getDataRefAs<ImageGeom>({partitionGridGeomPath});

  std::optional<BoolArray> vertexMask = {};
  if(m_InputValues->UseVertexMask)
  {
    vertexMask = m_DataStructure.getDataRefAs<BoolArray>({m_InputValues->VertexMaskPath});
  }

  const DataPath partitionIdsPath = m_InputValues->InputGeomCellAMPath.createChildPath(m_InputValues->PartitionIdsArrayName);
  auto& partitionIds = m_DataStructure.getDataRefAs<Int32Array>({partitionIdsPath});

  const IGeometry& iGeomToPartition = m_DataStructure.getDataRefAs<IGeometry>({m_InputValues->InputGeometryToPartition});
  Result<> result;
  switch(iGeomToPartition.getGeomType())
  {
  case IGeometry::Type::Image: {
    const ImageGeom& inputGeomToPartition = m_DataStructure.getDataRefAs<ImageGeom>({m_InputValues->InputGeometryToPartition});
    result = partitionCellBasedGeometry(inputGeomToPartition, partitionIds, partitionGridGeom, m_InputValues->OutOfBoundsFeatureID);
    break;
  }
  case IGeometry::Type::RectGrid: {
    const RectGridGeom& inputGeomToPartition = m_DataStructure.getDataRefAs<RectGridGeom>({m_InputValues->InputGeometryToPartition});
    result = partitionCellBasedGeometry(inputGeomToPartition, partitionIds, partitionGridGeom, m_InputValues->OutOfBoundsFeatureID);
    break;
  }
  case IGeometry::Type::Vertex:
  case IGeometry::Type::Edge:
  case IGeometry::Type::Triangle:
  case IGeometry::Type::Quad:
  case IGeometry::Type::Tetrahedral:
  case IGeometry::Type::Hexahedral: {
    const INodeGeometry0D& inputGeomToPartition = m_DataStructure.getDataRefAs<INodeGeometry0D>({m_InputValues->InputGeometryToPartition});
    const IGeometry::SharedVertexList* vertexList = inputGeomToPartition.getVertices();
    result = partitionNodeBasedGeometry(*vertexList, partitionIds, partitionGridGeom, m_InputValues->OutOfBoundsFeatureID, vertexMask);
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

  IParallelAlgorithm::AlgorithmArrays algArrays;
  algArrays.push_back(&partitionIds);

  ParallelData3DAlgorithm dataAlg;
  dataAlg.setRange(dims[0], dims[1], dims[2]);
  dataAlg.requireArraysInMemory(algArrays);
  dataAlg.execute(PartitionCellBasedGeometryImpl(inputGeometry, partitionIds, psImageGeom, m_InputValues->StartingFeatureID, outOfBoundsValue, m_ShouldCancel));

  return {};
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
Result<> PartitionGeometry::partitionNodeBasedGeometry(const IGeometry::SharedVertexList& vertexList, Int32Array& partitionIds, const ImageGeom& psImageGeom, int outOfBoundsValue,
                                                       const std::optional<const BoolArray>& maskArrayOpt)
{
  IParallelAlgorithm::AlgorithmArrays algArrays;
  algArrays.push_back(&vertexList);
  algArrays.push_back(&partitionIds);
  if(maskArrayOpt.has_value())
  {
    algArrays.push_back(&(maskArrayOpt.value()));
  }

  // Allow data-based parallelization
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, vertexList.getNumberOfTuples());
  dataAlg.requireArraysInMemory(algArrays);
  dataAlg.execute(PartitionNodeBasedGeometryImpl(vertexList, partitionIds, psImageGeom, m_InputValues->StartingFeatureID, outOfBoundsValue, maskArrayOpt, m_ShouldCancel));

  return {};
}
