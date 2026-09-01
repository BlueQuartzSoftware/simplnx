#include "ComputeFeatureBoundsScanline.hpp"

#include "ComputeFeatureBounds.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry1D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

using namespace nx::core;

namespace
{
// Each bulk read contains 65,536 Feature IDs. This caps cell-level storage at 256 KiB.
constexpr usize k_ChunkTuples = 65536;

/**
 * @concept GeometryType
 * @brief Restricts feature-bound helpers to geometry types.
 * @tparam T Specifies the candidate geometry type.
 */
template <typename T>
concept GeometryType = std::is_base_of_v<IGeometry, T>;

/**
 * @brief Expands one feature's six bounds with one cell or geometry element.
 * @param bounds Stores six values for each feature.
 * @param featureId Identifies the feature to update.
 * @param xMin Supplies the x lower bound.
 * @param yMin Supplies the y lower bound.
 * @param zMin Supplies the z lower bound.
 * @param xMax Supplies the x upper bound.
 * @param yMax Supplies the y upper bound.
 * @param zMax Supplies the z upper bound.
 * @pre bounds has six values for featureId.
 *
 * NaN values identify a feature with no earlier geometry element.
 */
void UpdateBounds(std::vector<float32>& bounds, int32 featureId, float32 xMin, float32 yMin, float32 zMin, float32 xMax, float32 yMax, float32 zMax)
{
  const usize activeIndex = static_cast<usize>(featureId) * 6;
  bounds[activeIndex + 0] = std::isnan(bounds[activeIndex + 0]) ? xMin : std::min(bounds[activeIndex + 0], xMin);
  bounds[activeIndex + 1] = std::isnan(bounds[activeIndex + 1]) ? yMin : std::min(bounds[activeIndex + 1], yMin);
  bounds[activeIndex + 2] = std::isnan(bounds[activeIndex + 2]) ? zMin : std::min(bounds[activeIndex + 2], zMin);

  bounds[activeIndex + 3] = std::isnan(bounds[activeIndex + 3]) ? xMax : std::max(bounds[activeIndex + 3], xMax);
  bounds[activeIndex + 4] = std::isnan(bounds[activeIndex + 4]) ? yMax : std::max(bounds[activeIndex + 4], yMax);
  bounds[activeIndex + 5] = std::isnan(bounds[activeIndex + 5]) ? zMax : std::max(bounds[activeIndex + 5], zMax);
}

/**
 * @brief Computes bounds with bounded Feature ID reads.
 * @tparam GeomT Specifies the geometry type.
 * @param geom Supplies geometry cells and coordinates.
 * @param featureIds Supplies cell Feature IDs.
 * @param bounds Receives six values per feature.
 * @param featureIdBuffer Supplies the fixed Feature ID staging buffer.
 * @param shouldCancel Signals cancellation between read batches.
 * @return Success, or a Feature ID bulk-I/O error.
 *
 * Cancellation returns success before bounds are published to output arrays. Vertex and
 * connectivity reads remain direct element access.
 */
template <GeometryType GeomT>
Result<> ComputeBounds(const GeomT& geom, const Int32AbstractDataStore& featureIds, std::vector<float32>& bounds, int32* featureIdBuffer, const std::atomic_bool& shouldCancel)
{
  static_assert(std::is_same_v<ImageGeom, GeomT> || std::is_base_of_v<INodeGeometry0D, GeomT> || std::is_base_of_v<INodeGeometry1D, GeomT> || std::is_base_of_v<INodeGeometry2D, GeomT>);

  const usize numElements = featureIds.getNumberOfTuples();
  if constexpr(std::is_same_v<ImageGeom, GeomT>)
  {
    const usize xPoints = geom.getNumXCells();
    const usize yPoints = geom.getNumYCells();
    const usize sliceSize = xPoints * yPoints;
    const FloatVec3 spacing = geom.getSpacing();
    const FloatVec3 origin = geom.getOrigin();

    for(usize offset = 0; offset < numElements; offset += k_ChunkTuples)
    {
      if(shouldCancel)
      {
        return {};
      }

      const usize count = std::min(k_ChunkTuples, numElements - offset);
      Result<> readResult = featureIds.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuffer, count));
      if(readResult.invalid())
      {
        return readResult;
      }

      for(usize chunkIndex = 0; chunkIndex < count; chunkIndex++)
      {
        const int32 currentFeatureId = featureIdBuffer[chunkIndex];
        if(currentFeatureId < 0)
        {
          continue;
        }

        const usize cellIndex = offset + chunkIndex;
        const usize i = cellIndex / sliceSize;
        const usize j = (cellIndex / xPoints) % yPoints;
        const usize k = cellIndex % xPoints;

        const float32 xValMin = k * spacing[0] + origin[0];
        const float32 yValMin = j * spacing[1] + origin[1];
        const float32 zValMin = i * spacing[2] + origin[2];

        const float32 xValMax = k * spacing[0] + origin[0] + spacing[0];
        const float32 yValMax = j * spacing[1] + origin[1] + spacing[1];
        const float32 zValMax = i * spacing[2] + origin[2] + spacing[2];
        UpdateBounds(bounds, currentFeatureId, xValMin, yValMin, zValMin, xValMax, yValMax, zValMax);
      }
    }
  }
  if constexpr(std::is_same_v<VertexGeom, GeomT>)
  {
    const IGeometry::SharedVertexList::store_type& verts = geom.getVerticesRef().getDataStoreRef();

    for(usize offset = 0; offset < numElements; offset += k_ChunkTuples)
    {
      if(shouldCancel)
      {
        return {};
      }

      const usize count = std::min(k_ChunkTuples, numElements - offset);
      Result<> readResult = featureIds.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuffer, count));
      if(readResult.invalid())
      {
        return readResult;
      }

      for(usize chunkIndex = 0; chunkIndex < count; chunkIndex++)
      {
        const int32 currentFeatureId = featureIdBuffer[chunkIndex];
        if(currentFeatureId < 0)
        {
          continue;
        }

        const usize i = offset + chunkIndex;
        const float32 xVal = verts[(i * 3) + 0];
        const float32 yVal = verts[(i * 3) + 1];
        const float32 zVal = verts[(i * 3) + 2];
        UpdateBounds(bounds, currentFeatureId, xVal, yVal, zVal, xVal, yVal, zVal);
      }
    }
  }
  if constexpr(std::is_same_v<EdgeGeom, GeomT>)
  {
    const IGeometry::SharedVertexList::store_type& verts = geom.getVerticesRef().getDataStoreRef();
    const IGeometry::SharedEdgeList::store_type& edges = geom.getEdgesRef().getDataStoreRef();

    const usize numComp = edges.getNumberOfComponents();
    for(usize offset = 0; offset < numElements; offset += k_ChunkTuples)
    {
      if(shouldCancel)
      {
        return {};
      }

      const usize count = std::min(k_ChunkTuples, numElements - offset);
      Result<> readResult = featureIds.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuffer, count));
      if(readResult.invalid())
      {
        return readResult;
      }

      for(usize chunkIndex = 0; chunkIndex < count; chunkIndex++)
      {
        const int32 currentFeatureId = featureIdBuffer[chunkIndex];
        if(currentFeatureId < 0)
        {
          continue;
        }

        const usize i = offset + chunkIndex;
        for(usize comp = 0; comp < numComp; comp++)
        {
          const IGeometry::SharedFaceList::value_type activeVertIndex = edges[(i * numComp) + comp];
          const float32 xVal = verts[(activeVertIndex * 3) + 0];
          const float32 yVal = verts[(activeVertIndex * 3) + 1];
          const float32 zVal = verts[(activeVertIndex * 3) + 2];
          UpdateBounds(bounds, currentFeatureId, xVal, yVal, zVal, xVal, yVal, zVal);
        }
      }
    }
  }
  if constexpr(std::is_same_v<TriangleGeom, GeomT> || std::is_same_v<QuadGeom, GeomT>)
  {
    const IGeometry::SharedVertexList::store_type& verts = geom.getVerticesRef().getDataStoreRef();
    const IGeometry::SharedFaceList::store_type& faces = geom.getFacesRef().getDataStoreRef();

    const usize numComp = faces.getNumberOfComponents();
    for(usize offset = 0; offset < numElements; offset += k_ChunkTuples)
    {
      if(shouldCancel)
      {
        return {};
      }

      const usize count = std::min(k_ChunkTuples, numElements - offset);
      Result<> readResult = featureIds.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuffer, count));
      if(readResult.invalid())
      {
        return readResult;
      }

      for(usize chunkIndex = 0; chunkIndex < count; chunkIndex++)
      {
        const int32 currentFeatureId = featureIdBuffer[chunkIndex];
        if(currentFeatureId < 0)
        {
          continue;
        }

        const usize i = offset + chunkIndex;
        for(usize comp = 0; comp < numComp; comp++)
        {
          const IGeometry::SharedFaceList::value_type activeVertIndex = faces[(i * numComp) + comp];
          const float32 xVal = verts[(activeVertIndex * 3) + 0];
          const float32 yVal = verts[(activeVertIndex * 3) + 1];
          const float32 zVal = verts[(activeVertIndex * 3) + 2];
          UpdateBounds(bounds, currentFeatureId, xVal, yVal, zVal, xVal, yVal, zVal);
        }
      }
    }
  }

  return {};
}

/**
 * @brief Selects a bounded Feature ID reader for a geometry type.
 * @tparam ArgsT Specifies forwarded helper argument types.
 * @param geom Supplies the input geometry.
 * @param args Forwards arguments to the selected helper.
 * @return Success, or a Feature ID bulk-I/O or unsupported-geometry error.
 */
template <class... ArgsT>
Result<> ExecuteComputeBounds(const IGeometry& geom, ArgsT&&... args)
{
  switch(geom.getGeomType())
  {
  case IGeometry::Type::Image: {
    return ComputeBounds(dynamic_cast<const ImageGeom&>(geom), std::forward<ArgsT>(args)...);
  }
  case IGeometry::Type::Triangle: {
    return ComputeBounds(dynamic_cast<const TriangleGeom&>(geom), std::forward<ArgsT>(args)...);
  }
  case IGeometry::Type::Vertex: {
    return ComputeBounds(dynamic_cast<const VertexGeom&>(geom), std::forward<ArgsT>(args)...);
  }
  case IGeometry::Type::Edge: {
    return ComputeBounds(dynamic_cast<const EdgeGeom&>(geom), std::forward<ArgsT>(args)...);
  }
  case IGeometry::Type::Quad: {
    return ComputeBounds(dynamic_cast<const QuadGeom&>(geom), std::forward<ArgsT>(args)...);
  }
  default: {
    return MakeErrorResult(-89472, fmt::format("Input geometry must be of type(s) [ Image, Triangle, Vertex, Edge, Quad ]. Supplied geometry name {}", geom.getName()));
  }
  }
}
} // namespace

ComputeFeatureBoundsScanline::ComputeFeatureBoundsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                           const ComputeFeatureBoundsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeFeatureBoundsScanline::~ComputeFeatureBoundsScanline() noexcept = default;

Result<> ComputeFeatureBoundsScanline::operator()()
{
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
  const auto& featureAM = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->FeatureAMPath);

  const usize numFeatureIdValues = featureIds.getSize();
  auto featureIdBuffer = std::make_unique<int32[]>(k_ChunkTuples);
  int32 maxFeatureId = std::numeric_limits<int32>::lowest();
  for(usize offset = 0; offset < numFeatureIdValues; offset += k_ChunkTuples)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const usize count = std::min(k_ChunkTuples, numFeatureIdValues - offset);
    Result<> readResult = featureIds.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuffer.get(), count));
    if(readResult.invalid())
    {
      return readResult;
    }
    maxFeatureId = std::max(maxFeatureId, *std::max_element(featureIdBuffer.get(), featureIdBuffer.get() + count));
  }

  const int32 numFeatures = maxFeatureId + 1;
  if(numFeatures > featureAM.getNumberOfTuples())
  {
    return MakeErrorResult(-89471, fmt::format("{} Attribute Matrix size ({}) doesn't align with number of features ({}) in {} array", m_InputValues->FeatureAMPath.getTargetName(),
                                               featureAM.getNumberOfTuples(), numFeatures, m_InputValues->FeatureIdsArrayPath.getTargetName()));
  }

  const auto& geom = m_DataStructure.getDataRefAs<IGeometry>(m_InputValues->GeometryPath);

  if(m_ShouldCancel)
  {
    return {};
  }

  std::vector<float32> bounds(static_cast<usize>(numFeatures) * 6, std::numeric_limits<float32>::quiet_NaN());
  Result<> computeResult = ExecuteComputeBounds(geom, featureIds, bounds, featureIdBuffer.get(), m_ShouldCancel);
  if(computeResult.invalid())
  {
    return computeResult;
  }
  if(bounds.empty())
  {
    return MakeErrorResult(-89472, fmt::format("Input geometry must be of type(s) [ Image, Triangle, Vertex, Edge, Quad ]. Supplied geometry name {}", geom.getName()));
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  switch(static_cast<ComputeFeatureBounds::OutputDataType>(m_InputValues->OutputType))
  {
  case ComputeFeatureBounds::OutputDataType::Split: {
    auto& minBounds = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->MinArrayPath).getDataStoreRef();
    auto& maxBounds = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->MaxArrayPath).getDataStoreRef();
    for(usize i = 0; i < minBounds.getNumberOfTuples(); i++)
    {
      usize activeIndex = i * 6;
      minBounds.setValue((i * 3) + 0, bounds[activeIndex + 0]);
      minBounds.setValue((i * 3) + 1, bounds[activeIndex + 1]);
      minBounds.setValue((i * 3) + 2, bounds[activeIndex + 2]);

      maxBounds.setValue((i * 3) + 0, bounds[activeIndex + 3]);
      maxBounds.setValue((i * 3) + 1, bounds[activeIndex + 4]);
      maxBounds.setValue((i * 3) + 2, bounds[activeIndex + 5]);
    }
    break;
  }
  case ComputeFeatureBounds::OutputDataType::Unified: {
    auto& unifiedBounds = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->UnifiedArrayPath).getDataStoreRef();
    for(usize i = 0; i < unifiedBounds.getNumberOfTuples(); i++)
    {
      usize activeIndex = i * 6;
      unifiedBounds.setValue(activeIndex + 0, bounds[activeIndex + 0]);
      unifiedBounds.setValue(activeIndex + 1, bounds[activeIndex + 1]);
      unifiedBounds.setValue(activeIndex + 2, bounds[activeIndex + 2]);

      unifiedBounds.setValue(activeIndex + 3, bounds[activeIndex + 3]);
      unifiedBounds.setValue(activeIndex + 4, bounds[activeIndex + 4]);
      unifiedBounds.setValue(activeIndex + 5, bounds[activeIndex + 5]);
    }
    break;
  }
  }

  if(m_InputValues->CreateEdgeGeometry)
  {
    /**
     * @brief Connects the eight bounding-box vertices into twelve edges.
     */
    static constexpr std::array<std::pair<int, int>, 12> cubeEdges = {{{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}}};
    std::array<usize, 2> vertPair = {0, 0};

    // Allocate maximum edge storage before omitting features without valid bounds.
    usize numVerts = numFeatures * 8;
    usize numEdges = numFeatures * 12;

    auto& edgeGeom = m_DataStructure.getDataRefAs<EdgeGeom>(m_InputValues->EdgeGeometryDataPath);
    edgeGeom.resizeVertexList(numVerts);
    edgeGeom.resizeEdgeList(numEdges);
    edgeGeom.getEdgeAttributeMatrix()->resizeTuples({numEdges});
    edgeGeom.getVertexAttributeMatrix()->resizeTuples({numVerts});

    DataPath edgeAmPath = m_InputValues->EdgeGeometryDataPath.createChildPath(m_InputValues->EdgeAttributeMatrixName);
    auto& edgeFeatureIds = m_DataStructure.getDataRefAs<Int32Array>(edgeAmPath.createChildPath(m_InputValues->FeatureIdsArrayName)).getDataStoreRef();
    edgeFeatureIds.fill(-1);
    usize currentOffset = 0;
    for(usize idx = 0; idx < numFeatures; ++idx)
    {
      usize activeIndex = idx * 6;
      // NaN bounds identify a feature with no supported geometry element.
      bool foundNAN = false;
      for(usize i = 0; i < 6; i++)
      {
        if(std::isnan(bounds[activeIndex + i]))
        {
          foundNAN = true;
          break;
        }
      }
      if(foundNAN)
      {
        continue;
      }

      edgeGeom.setVertexCoordinate(currentOffset * 8 + 0, {bounds[activeIndex + 0], bounds[activeIndex + 1], bounds[activeIndex + 2]});
      edgeGeom.setVertexCoordinate(currentOffset * 8 + 1, {bounds[activeIndex + 3], bounds[activeIndex + 1], bounds[activeIndex + 2]});
      edgeGeom.setVertexCoordinate(currentOffset * 8 + 2, {bounds[activeIndex + 3], bounds[activeIndex + 4], bounds[activeIndex + 2]});
      edgeGeom.setVertexCoordinate(currentOffset * 8 + 3, {bounds[activeIndex + 0], bounds[activeIndex + 4], bounds[activeIndex + 2]});
      edgeGeom.setVertexCoordinate(currentOffset * 8 + 4, {bounds[activeIndex + 0], bounds[activeIndex + 1], bounds[activeIndex + 5]});
      edgeGeom.setVertexCoordinate(currentOffset * 8 + 5, {bounds[activeIndex + 3], bounds[activeIndex + 1], bounds[activeIndex + 5]});
      edgeGeom.setVertexCoordinate(currentOffset * 8 + 6, {bounds[activeIndex + 3], bounds[activeIndex + 4], bounds[activeIndex + 5]});
      edgeGeom.setVertexCoordinate(currentOffset * 8 + 7, {bounds[activeIndex + 0], bounds[activeIndex + 4], bounds[activeIndex + 5]});

      for(usize edgeIdx = 0; edgeIdx < cubeEdges.size(); ++edgeIdx)
      {
        vertPair[0] = currentOffset * 8 + (cubeEdges[edgeIdx].first);
        vertPair[1] = currentOffset * 8 + (cubeEdges[edgeIdx].second);

        edgeGeom.setEdgePointIds(currentOffset * 12 + edgeIdx, vertPair);
        edgeFeatureIds[currentOffset * 12 + edgeIdx] = currentOffset;
      }
      currentOffset++;
    }

    currentOffset--;
    edgeGeom.resizeVertexList(currentOffset * 8);
    edgeGeom.getVertexAttributeMatrix()->resizeTuples({currentOffset * 8});
    edgeGeom.resizeEdgeList(currentOffset * 12);
    edgeGeom.getEdgeAttributeMatrix()->resizeTuples({currentOffset * 12});
  }

  return {};
}
