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
 * @brief Tries to map an image axis to flat partition-index contributions.
 * @param inputGrid Supplies the input cell centers.
 * @param partitionGrid Defines the cells that receive the input centers.
 * @param axis Selects X, Y or Z.
 * @param stride Gives the partition index increment for one cell along the axis.
 * @return Offsets or exterior sentinels. An unsafe conversion requires checking actual input tuples.
 */
std::optional<std::vector<usize>> CreateImageAxisPartitionIndices(const ImageGeom& inputGrid, const ImageGeom& partitionGrid, usize axis, usize stride)
{
  const auto inputDimension = inputGrid.getDimensions()[axis];
  const auto inputOrigin = inputGrid.getOrigin()[axis];
  const auto inputSpacing = inputGrid.getSpacing()[axis];
  const auto partitionDimension = partitionGrid.getDimensions()[axis];
  const auto partitionOrigin = partitionGrid.getOrigin()[axis];
  const auto partitionSpacing = partitionGrid.getSpacing()[axis];
  std::vector<usize> partitionIndices(inputDimension, k_InvalidPartitionIndex);
  for(usize index = 0; index < inputDimension; ++index)
  {
    const float64 coordinate = static_cast<float64>(index) * inputSpacing + inputOrigin + (0.5 * inputSpacing);
    const auto mapped = detail::TryMapPartitionAxis(coordinate, partitionDimension, partitionOrigin, partitionSpacing);
    if(!mapped.has_value())
    {
      return {};
    }
    if(*mapped < partitionDimension)
    {
      partitionIndices[index] = *mapped * stride;
    }
  }
  return {std::move(partitionIndices)};
}

/**
 * @brief Checks whether every rectilinear axis can use the Direct worker.
 * @param grid Supplies the input dimensions.
 * @param partitionGrid Defines the cells that receive the input centers.
 * @param centers Reads the input centers through contiguous buffers.
 * @param shouldCancel Stops the proof when true.
 * @return True for safe axes, false if actual tuples need checking, or a store read error.
 */
Result<bool> CanMapRectGridAxes(const RectGridGeom& grid, const ImageGeom& partitionGrid, detail::RectGridPartitionCenters& centers, const std::atomic_bool& shouldCancel)
{
  const auto inputDimensions = grid.getDimensions();
  const auto dimensions = partitionGrid.getDimensions();
  const auto origin = partitionGrid.getOrigin();
  const auto spacing = partitionGrid.getSpacing();
  for(usize axis = 0; axis < 3; ++axis)
  {
    for(usize index = 0; index < inputDimensions[axis]; ++index)
    {
      if(shouldCancel)
      {
        return {false};
      }
      auto center = centers.readAxis(axis, index);
      if(center.invalid())
      {
        return ConvertInvalidResult<bool>(std::move(center));
      }
      if(!detail::TryMapPartitionAxis(center.value(), dimensions[axis], origin[axis], spacing[axis]).has_value())
      {
        return {false};
      }
    }
  }
  return {true};
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

      if(m_MaskArrayOpt.has_value() && !(*m_MaskArrayOpt)[idx])
      {
        m_PartitionIdsStore[idx] = m_OutOfBoundsValue;
        continue;
      }

      const float32 x = m_VerticesStore[idx * 3];
      const float32 y = m_VerticesStore[idx * 3 + 1];
      const float32 z = m_VerticesStore[idx * 3 + 2];

      auto partitionIndexResult = m_PSImageGeom.getIndex(x, y, z);
      if(!partitionIndexResult.has_value())
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
    return MakeErrorResult(detail::k_UnsupportedPartitionGeometry,
                           fmt::format("Input geometry '{}' has a type that Partition Geometry cannot use.", m_InputValues->InputGeometryToPartition.toString()));
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

  ParallelData3DAlgorithm dataAlg;
  dataAlg.setRange(dims[0], dims[1], dims[2]);
  IParallelAlgorithm::AlgorithmStores algStores = {&partitionIdsStore};
  const auto* rectGrid = dynamic_cast<const RectGridGeom*>(&inputGeometry);
  if(rectGrid != nullptr)
  {
    algStores.push_back(&rectGrid->getXBoundsRef().getDataStoreRef());
    algStores.push_back(&rectGrid->getYBoundsRef().getDataStoreRef());
    algStores.push_back(&rectGrid->getZBoundsRef().getDataStoreRef());
  }
  dataAlg.requireStoresInMemory(algStores);
  const DataPath& inputPath = m_InputValues->InputGeometryToPartition;

  auto* inMemoryPartitionIdsStore = dynamic_cast<Int32DataStore*>(&partitionIdsStore);
  if(inputGeometry.getGeomType() == IGeometry::Type::Image && inMemoryPartitionIdsStore != nullptr)
  {
    const auto& imageGeom = static_cast<const ImageGeom&>(inputGeometry);
    const auto partitionDims = psImageGeom.getDimensions();
    const auto xIndices = CreateImageAxisPartitionIndices(imageGeom, psImageGeom, 0, 1);
    const auto yIndices = CreateImageAxisPartitionIndices(imageGeom, psImageGeom, 1, partitionDims[0]);
    const auto zIndices = CreateImageAxisPartitionIndices(imageGeom, psImageGeom, 2, partitionDims[0] * partitionDims[1]);
    if(xIndices.has_value() && yIndices.has_value() && zIndices.has_value())
    {
      dataAlg.execute(PartitionImageGeometryImpl(dims, *xIndices, *yIndices, *zIndices, inMemoryPartitionIdsStore->data(), m_InputValues->StartingFeatureID, outOfBoundsValue, m_ShouldCancel));
      return {};
    }
  }

  // The fast axis proof can fail for an axis that no actual cell needs to convert.
  std::optional<detail::RectGridPartitionCenters> rectCenters;
  bool axesAreSafe = false;
  if(rectGrid != nullptr)
  {
    rectCenters.emplace(*rectGrid);
    auto checked = CanMapRectGridAxes(*rectGrid, psImageGeom, *rectCenters, m_ShouldCancel);
    if(checked.invalid())
    {
      return ConvertResult(std::move(checked));
    }
    axesAreSafe = checked.value();
  }
  if(!axesAreSafe)
  {
    for(usize index = 0; index < partitionIdsStore.getNumberOfTuples(); ++index)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      auto coordinate = rectCenters.has_value() ? rectCenters->readTuple(index) : Result<Point3D<float64>>{inputGeometry.getCoords(index)};
      if(coordinate.invalid())
      {
        return ConvertResult(std::move(coordinate));
      }
      auto checked = ComputeCheckedPartitionIndex(psImageGeom, coordinate.value(), inputPath, index);
      if(checked.invalid())
      {
        return ConvertResult(std::move(checked));
      }
    }
  }
  dataAlg.execute(PartitionCellBasedGeometryImpl(inputGeometry, partitionIdsStore, psImageGeom, m_InputValues->StartingFeatureID, outOfBoundsValue, m_ShouldCancel));

  return {};
}

Result<> PartitionGeometryDirect::partitionNodeBasedGeometry(const VertexStore& vertexListStore, Int32AbstractDataStore& partitionIdsStore, const ImageGeom& psImageGeom, int outOfBoundsValue,
                                                             const std::optional<const BoolArray>& maskArrayOpt)
{
  IParallelAlgorithm::AlgorithmStores algStores = {&vertexListStore, &partitionIdsStore};
  if(maskArrayOpt.has_value())
  {
    algStores.push_back(&maskArrayOpt->getDataStoreRef());
  }

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, vertexListStore.getNumberOfTuples());
  dataAlg.requireStoresInMemory(algStores);
  // The validation pass propagates errors without adding shared mutable worker state.
  for(usize index = 0; index < vertexListStore.getNumberOfTuples(); ++index)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    if(maskArrayOpt.has_value() && !(*maskArrayOpt)[index])
    {
      continue;
    }
    const Point3Df coordinate = {vertexListStore[3 * index], vertexListStore[3 * index + 1], vertexListStore[3 * index + 2]};
    auto checked = ComputeCheckedPartitionIndex(psImageGeom, coordinate, m_InputValues->InputGeometryToPartition, index);
    if(checked.invalid())
    {
      return ConvertResult(std::move(checked));
    }
  }
  dataAlg.execute(PartitionNodeBasedGeometryImpl(vertexListStore, partitionIdsStore, psImageGeom, m_InputValues->StartingFeatureID, outOfBoundsValue, maskArrayOpt, m_ShouldCancel));

  return {};
}
