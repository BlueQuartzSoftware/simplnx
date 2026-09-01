#include "PartitionGeometryDirect.hpp"

#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/Utilities/ParallelData3DAlgorithm.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include "SimplnxCore/Filters/PartitionGeometryFilter.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

using namespace nx::core;

namespace
{
constexpr usize k_InvalidPartitionIndex = std::numeric_limits<usize>::max();

/**
 * @brief Maps input cell centers on one axis to flattened partition offsets.
 * @param inputDimension Specifies input cells on the axis.
 * @param inputOrigin Specifies input origin on the axis.
 * @param inputSpacing Specifies input spacing on the axis.
 * @param partitionDimension Specifies partition cells on the axis.
 * @param partitionOrigin Specifies partition origin on the axis.
 * @param partitionSpacing Specifies partition spacing on the axis.
 * @param partitionIndexMultiplier Converts an axis index to a flat-index contribution.
 * @return Partition offsets, or an invalid sentinel for exterior cell centers.
 *
 * Precomputed axis maps avoid repeated coordinate queries in the ImageGeom fast path.
 */
std::vector<usize> CreateImageAxisPartitionIndices(usize inputDimension, float32 inputOrigin, float32 inputSpacing, usize partitionDimension, float32 partitionOrigin, float32 partitionSpacing,
                                                   usize partitionIndexMultiplier)
{
  std::vector<usize> partitionIndices(inputDimension, k_InvalidPartitionIndex);
  const float64 partitionMax = static_cast<float64>(partitionDimension) * partitionSpacing + partitionOrigin;

  for(usize index = 0; index < inputDimension; index++)
  {
    const float64 coordinate = static_cast<float64>(index) * inputSpacing + inputOrigin + (0.5 * inputSpacing);
    if(coordinate < partitionOrigin || coordinate > partitionMax)
    {
      continue;
    }

    const usize partitionIndex = static_cast<usize>(std::floor((coordinate - partitionOrigin) / partitionSpacing));
    if(partitionIndex < partitionDimension)
    {
      partitionIndices[index] = partitionIndex * partitionIndexMultiplier;
    }
  }

  return partitionIndices;
}

/**
 * @class PartitionImageGeometryImpl
 * @brief Partitions ImageGeom cells from precomputed axis maps.
 *
 * Parallel ranges write disjoint output spans through a resident raw pointer.
 */
class PartitionImageGeometryImpl
{
public:
  /**
   * @brief Creates an ImageGeom partition worker.
   * @param inputDimensions Specifies input cell dimensions.
   * @param xPartitionIndices Provides X-axis flat-index contributions.
   * @param yPartitionIndices Provides Y-axis flat-index contributions.
   * @param zPartitionIndices Provides Z-axis flat-index contributions.
   * @param partitionIds Receives partition IDs.
   * @param startingPartitionId Offsets valid flattened partition indexes.
   * @param outOfBoundsValue Supplies IDs for exterior cells.
   * @param shouldCancel Stops active ranges when true.
   */
  PartitionImageGeometryImpl(const SizeVec3& inputDimensions, const std::vector<usize>& xPartitionIndices, const std::vector<usize>& yPartitionIndices, const std::vector<usize>& zPartitionIndices,
                             int32* partitionIds, int startingPartitionId, int outOfBoundsValue, const std::atomic_bool& shouldCancel)
  : m_InputDimensions(inputDimensions)
  , m_XPartitionIndices(xPartitionIndices)
  , m_YPartitionIndices(yPartitionIndices)
  , m_ZPartitionIndices(zPartitionIndices)
  , m_PartitionIds(partitionIds)
  , m_StartingPartitionId(startingPartitionId)
  , m_OutOfBoundsValue(outOfBoundsValue)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Partitions one rectangular cell range.
   * @param xStart Specifies inclusive X start.
   * @param xEnd Specifies exclusive X end.
   * @param yStart Specifies inclusive Y start.
   * @param yEnd Specifies exclusive Y end.
   * @param zStart Specifies inclusive Z start.
   * @param zEnd Specifies exclusive Z end.
   */
  void compute(size_t xStart, size_t xEnd, size_t yStart, size_t yEnd, size_t zStart, size_t zEnd) const
  {
    const usize inputPlaneSize = m_InputDimensions[0] * m_InputDimensions[1];

    for(usize z = zStart; z < zEnd; z++)
    {
      const usize zPartitionIndex = m_ZPartitionIndices[z];
      for(usize y = yStart; y < yEnd; y++)
      {
        if(m_ShouldCancel)
        {
          return;
        }

        const usize yPartitionIndex = m_YPartitionIndices[y];
        usize outputIndex = (z * inputPlaneSize) + (y * m_InputDimensions[0]) + xStart;
        if(yPartitionIndex == k_InvalidPartitionIndex || zPartitionIndex == k_InvalidPartitionIndex)
        {
          std::fill_n(m_PartitionIds + outputIndex, xEnd - xStart, m_OutOfBoundsValue);
          continue;
        }

        for(usize x = xStart; x < xEnd; x++, outputIndex++)
        {
          const usize xPartitionIndex = m_XPartitionIndices[x];
          if(xPartitionIndex == k_InvalidPartitionIndex)
          {
            m_PartitionIds[outputIndex] = m_OutOfBoundsValue;
          }
          else
          {
            m_PartitionIds[outputIndex] = static_cast<int32>(xPartitionIndex + yPartitionIndex + zPartitionIndex) + m_StartingPartitionId;
          }
        }
      }
    }
  }

  /**
   * @brief Partitions one scheduler range.
   * @param range Specifies exclusive axis bounds.
   */
  void operator()(const Range3D& range) const
  {
    compute(range[0], range[1], range[2], range[3], range[4], range[5]);
  }

private:
  SizeVec3 m_InputDimensions;
  const std::vector<usize>& m_XPartitionIndices;
  const std::vector<usize>& m_YPartitionIndices;
  const std::vector<usize>& m_ZPartitionIndices;
  int32* m_PartitionIds = nullptr;
  int m_StartingPartitionId;
  int m_OutOfBoundsValue;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @class PartitionCellBasedGeometryImpl
 * @brief Partitions grid cells through IGridGeometry coordinate queries.
 *
 * Parallel ranges write disjoint output indexes. All required stores must be resident.
 */
class PartitionCellBasedGeometryImpl
{
public:
  /**
   * @brief Creates a generic grid-geometry partition worker.
   * @param inputGeometry Provides cell coordinates and dimensions.
   * @param partitionIdsStore Receives partition IDs.
   * @param psImageGeom Defines partition-grid cells.
   * @param startingPartitionId Offsets valid flattened partition indexes.
   * @param outOfBoundsValue Supplies IDs for exterior cells.
   * @param shouldCancel Stops active ranges when true.
   */
  PartitionCellBasedGeometryImpl(const IGridGeometry& inputGeometry, Int32AbstractDataStore& partitionIdsStore, const ImageGeom& psImageGeom, int startingPartitionId, int outOfBoundsValue,
                                 const std::atomic_bool& shouldCancel)
  : m_InputGeometry(inputGeometry)
  , m_PartitionIdsStore(partitionIdsStore)
  , m_PSImageGeom(psImageGeom)
  , m_StartingPartitionId(startingPartitionId)
  , m_OutOfBoundsValue(outOfBoundsValue)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Partitions one rectangular cell range.
   * @param xStart Specifies inclusive X start.
   * @param xEnd Specifies exclusive X end.
   * @param yStart Specifies inclusive Y start.
   * @param yEnd Specifies exclusive Y end.
   * @param zStart Specifies inclusive Z start.
   * @param zEnd Specifies exclusive Z end.
   */
  void compute(size_t xStart, size_t xEnd, size_t yStart, size_t yEnd, size_t zStart, size_t zEnd) const
  {
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
            m_PartitionIdsStore[index] = static_cast<int32>(*partitionIndexResult) + m_StartingPartitionId;
          }
          else
          {
            m_PartitionIdsStore[index] = m_OutOfBoundsValue;
          }
        }
      }
    }
  }

  /**
   * @brief Partitions one scheduler range.
   * @param r Specifies exclusive axis bounds.
   */
  void operator()(const Range3D& r) const
  {
    compute(r[0], r[1], r[2], r[3], r[4], r[5]);
  }

private:
  const IGridGeometry& m_InputGeometry;
  Int32AbstractDataStore& m_PartitionIdsStore;
  const ImageGeom& m_PSImageGeom;
  int m_StartingPartitionId;
  int m_OutOfBoundsValue;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @class PartitionNodeBasedGeometryImpl
 * @brief Partitions resident vertices with an optional resident mask.
 *
 * Parallel ranges read shared arrays and write disjoint partition-ID indexes.
 */
class PartitionNodeBasedGeometryImpl
{
public:
  /**
   * @brief Creates a node-geometry partition worker.
   * @param verticesStore Provides flat XYZ vertex coordinates.
   * @param partitionIdsStore Receives partition IDs.
   * @param psImageGeom Defines partition-grid cells.
   * @param startingPartitionId Offsets valid flattened partition indexes.
   * @param outOfBoundsValue Supplies IDs for masked or exterior vertices.
   * @param maskArrayOpt Selects vertices when present.
   * @param shouldCancel Stops active ranges when true.
   */
  PartitionNodeBasedGeometryImpl(const PartitionGeometryDirect::VertexStore& verticesStore, Int32AbstractDataStore& partitionIdsStore, const ImageGeom& psImageGeom, int startingPartitionId,
                                 int outOfBoundsValue, const std::optional<const BoolArray>& maskArrayOpt, const std::atomic_bool& shouldCancel)
  : m_VerticesStore(verticesStore)
  , m_PartitionIdsStore(partitionIdsStore)
  , m_PSImageGeom(psImageGeom)
  , m_StartingPartitionId(startingPartitionId)
  , m_OutOfBoundsValue(outOfBoundsValue)
  , m_MaskArrayOpt(maskArrayOpt)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Partitions one vertex range.
   * @param start Specifies inclusive vertex start.
   * @param end Specifies exclusive vertex end.
   */
  void compute(size_t start, size_t end) const
  {
    for(usize idx = start; idx < end; idx++)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      const float32 x = m_VerticesStore[idx * 3];
      const float32 y = m_VerticesStore[idx * 3 + 1];
      const float32 z = m_VerticesStore[idx * 3 + 2];

      auto partitionIndexResult = m_PSImageGeom.getIndex(x, y, z);
      if((m_MaskArrayOpt.has_value() && !(*m_MaskArrayOpt)[idx]) || !partitionIndexResult.has_value())
      {
        m_PartitionIdsStore[idx] = m_OutOfBoundsValue;
      }
      else
      {
        m_PartitionIdsStore[idx] = static_cast<int32>(*partitionIndexResult) + m_StartingPartitionId;
      }
    }
  }

  /**
   * @brief Partitions one scheduler range.
   * @param range Specifies inclusive and exclusive vertex bounds.
   */
  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  const PartitionGeometryDirect::VertexStore& m_VerticesStore;
  Int32AbstractDataStore& m_PartitionIdsStore;
  const ImageGeom& m_PSImageGeom;
  int m_StartingPartitionId;
  int m_OutOfBoundsValue;
  const std::optional<const BoolArray>& m_MaskArrayOpt;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace

PartitionGeometryDirect::PartitionGeometryDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 PartitionGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

PartitionGeometryDirect::~PartitionGeometryDirect() noexcept = default;

const std::atomic_bool& PartitionGeometryDirect::getCancel()
{
  return m_ShouldCancel;
}

Result<> PartitionGeometryDirect::operator()()
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
    auto& pgFeatureIdsStore = m_DataStructure.getDataAs<Int32Array>(partitionGridFeatureIdsPath)->getDataStoreRef();

    for(usize i = 0; i < pgFeatureIdsStore.getNumberOfTuples(); i++)
    {
      pgFeatureIdsStore[i] = static_cast<int32>(i) + m_InputValues->StartingFeatureID;
    }
  }

  const ImageGeom& partitionGridGeom = m_DataStructure.getDataRefAs<ImageGeom>({partitionGridGeomPath});

  std::optional<BoolArray> vertexMask = {};
  if(m_InputValues->UseVertexMask)
  {
    vertexMask = m_DataStructure.getDataRefAs<BoolArray>(m_InputValues->VertexMaskPath);
  }

  const DataPath partitionIdsPath = m_InputValues->InputGeomCellAMPath.createChildPath(m_InputValues->PartitionIdsArrayName);
  auto& partitionIdsStore = m_DataStructure.getDataAs<Int32Array>(partitionIdsPath)->getDataStoreRef();

  const IGeometry& iGeomToPartition = m_DataStructure.getDataRefAs<IGeometry>(m_InputValues->InputGeometryToPartition);
  Result<> result;
  switch(iGeomToPartition.getGeomType())
  {
  case IGeometry::Type::Image: {
    const ImageGeom& inputGeomToPartition = m_DataStructure.getDataRefAs<ImageGeom>({m_InputValues->InputGeometryToPartition});
    result = partitionCellBasedGeometry(inputGeomToPartition, partitionIdsStore, partitionGridGeom, m_InputValues->OutOfBoundsFeatureID);
    break;
  }
  case IGeometry::Type::RectGrid: {
    const RectGridGeom& inputGeomToPartition = m_DataStructure.getDataRefAs<RectGridGeom>({m_InputValues->InputGeometryToPartition});
    result = partitionCellBasedGeometry(inputGeomToPartition, partitionIdsStore, partitionGridGeom, m_InputValues->OutOfBoundsFeatureID);
    break;
  }
  case IGeometry::Type::Vertex:
  case IGeometry::Type::Edge:
  case IGeometry::Type::Triangle:
  case IGeometry::Type::Quad:
  case IGeometry::Type::Tetrahedral:
  case IGeometry::Type::Hexahedral: {
    const INodeGeometry0D& inputGeomToPartition = m_DataStructure.getDataRefAs<INodeGeometry0D>({m_InputValues->InputGeometryToPartition});
    const AbstractDataStore<IGeometry::SharedVertexList::value_type>& vertexListStore = inputGeomToPartition.getVertices()->getDataStoreRef();
    result = partitionNodeBasedGeometry(vertexListStore, partitionIdsStore, partitionGridGeom, m_InputValues->OutOfBoundsFeatureID, vertexMask);
    break;
  }
  default: {
    return MakeErrorResult(-3012, fmt::format("Unable to partition geometry at path '{}' - Unknown geometry type detected.", m_InputValues->InputGeometryToPartition.toString()));
  }
  }

  if(result.invalid())
  {
    return result;
  }

  return {};
}

Result<> PartitionGeometryDirect::partitionCellBasedGeometry(const IGridGeometry& inputGeometry, Int32AbstractDataStore& partitionIdsStore, const ImageGeom& psImageGeom, int outOfBoundsValue)
{
  const SizeVec3 dims = inputGeometry.getDimensions();

  IParallelAlgorithm::AlgorithmStores algStores;
  algStores.push_back(&partitionIdsStore);

  ParallelData3DAlgorithm dataAlg;
  dataAlg.setRange(dims[0], dims[1], dims[2]);
  dataAlg.requireStoresInMemory(algStores);

  auto* inMemoryPartitionIdsStore = dynamic_cast<Int32DataStore*>(&partitionIdsStore);
  if(inputGeometry.getGeomType() == IGeometry::Type::Image && inMemoryPartitionIdsStore != nullptr)
  {
    const auto& imageGeom = static_cast<const ImageGeom&>(inputGeometry);
    const SizeVec3 partitionDimensions = psImageGeom.getDimensions();
    const FloatVec3 inputOrigin = imageGeom.getOrigin();
    const FloatVec3 inputSpacing = imageGeom.getSpacing();
    const FloatVec3 partitionOrigin = psImageGeom.getOrigin();
    const FloatVec3 partitionSpacing = psImageGeom.getSpacing();

    const std::vector<usize> xPartitionIndices = CreateImageAxisPartitionIndices(dims[0], inputOrigin[0], inputSpacing[0], partitionDimensions[0], partitionOrigin[0], partitionSpacing[0], 1);
    const std::vector<usize> yPartitionIndices =
        CreateImageAxisPartitionIndices(dims[1], inputOrigin[1], inputSpacing[1], partitionDimensions[1], partitionOrigin[1], partitionSpacing[1], partitionDimensions[0]);
    const std::vector<usize> zPartitionIndices =
        CreateImageAxisPartitionIndices(dims[2], inputOrigin[2], inputSpacing[2], partitionDimensions[2], partitionOrigin[2], partitionSpacing[2], partitionDimensions[0] * partitionDimensions[1]);

    dataAlg.execute(PartitionImageGeometryImpl(dims, xPartitionIndices, yPartitionIndices, zPartitionIndices, inMemoryPartitionIdsStore->data(), m_InputValues->StartingFeatureID, outOfBoundsValue,
                                               m_ShouldCancel));
    return {};
  }

  dataAlg.execute(PartitionCellBasedGeometryImpl(inputGeometry, partitionIdsStore, psImageGeom, m_InputValues->StartingFeatureID, outOfBoundsValue, m_ShouldCancel));

  return {};
}

Result<> PartitionGeometryDirect::partitionNodeBasedGeometry(const VertexStore& vertexListStore, Int32AbstractDataStore& partitionIdsStore, const ImageGeom& psImageGeom, int outOfBoundsValue,
                                                             const std::optional<const BoolArray>& maskArrayOpt)
{
  IParallelAlgorithm::AlgorithmStores algStores;
  algStores.push_back(&vertexListStore);
  algStores.push_back(&partitionIdsStore);

  IParallelAlgorithm::AlgorithmArrays algArrays;
  if(maskArrayOpt.has_value())
  {
    algArrays.push_back(&(maskArrayOpt.value()));
  }

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, vertexListStore.getNumberOfTuples());
  dataAlg.requireArraysInMemory(algArrays);
  dataAlg.requireStoresInMemory(algStores);
  dataAlg.execute(PartitionNodeBasedGeometryImpl(vertexListStore, partitionIdsStore, psImageGeom, m_InputValues->StartingFeatureID, outOfBoundsValue, maskArrayOpt, m_ShouldCancel));

  return {};
}
