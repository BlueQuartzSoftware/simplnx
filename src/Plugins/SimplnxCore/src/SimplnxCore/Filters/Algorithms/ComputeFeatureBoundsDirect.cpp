#include "ComputeFeatureBoundsDirect.hpp"

#include "ComputeFeatureBounds.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry1D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

using namespace nx::core;

namespace
{
/**
 * @concept GeometryType
 * @brief Restricts feature-bound helpers to geometry types.
 * @tparam T Specifies the candidate geometry type.
 */
template <typename T>
concept GeometryType = std::is_base_of_v<IGeometry, T>;

// This sentinel marks a feature with no observed image cell.
constexpr usize k_InvalidIndex = std::numeric_limits<usize>::max();

/**
 * @struct ImageFeatureIndexBounds
 * @brief Stores minimum and maximum cell indices for one feature.
 */
struct ImageFeatureIndexBounds
{
  std::array<usize, 3> minIndices = {k_InvalidIndex, k_InvalidIndex, k_InvalidIndex};
  std::array<usize, 3> maxIndices = {0, 0, 0};
};

/**
 * @struct ImageBoundsResult
 * @brief Stores direct ImageGeom bounds and observed feature count.
 */
struct ImageBoundsResult
{
  std::vector<float32> bounds;
  int32 numFeatures = 0;
};

/**
 * @brief Computes direct ImageGeom bounds from resident Feature IDs.
 * @param imageGeom Supplies dimensions, origin, and spacing.
 * @param featureIds Supplies contiguous Feature IDs.
 * @param featureTupleCount Limits valid Feature IDs.
 * @return Bounds and the observed feature count.
 * @pre featureIds contains one value per ImageGeom cell.
 *
 * X-axis runs update index extrema once per repeated Feature ID. This reduces
 * repeated bounds updates for contiguous image regions.
 */
ImageBoundsResult ComputeImageBounds(const ImageGeom& imageGeom, const int32* featureIds, usize featureTupleCount)
{
  const usize xPoints = imageGeom.getNumXCells();
  const usize yPoints = imageGeom.getNumYCells();
  const usize zPoints = imageGeom.getNumZCells();
  const usize sliceSize = xPoints * yPoints;

  std::vector<ImageFeatureIndexBounds> indexBounds(featureTupleCount);
  int32 maxFeatureId = std::numeric_limits<int32>::lowest();
  for(usize z = 0; z < zPoints; z++)
  {
    const usize sliceOffset = z * sliceSize;
    for(usize y = 0; y < yPoints; y++)
    {
      const int32* rowFeatureIds = featureIds + sliceOffset + (y * xPoints);
      usize runStart = 0;
      while(runStart < xPoints)
      {
        const int32 featureId = rowFeatureIds[runStart];
        usize runEnd = runStart + 1;
        while(runEnd < xPoints && rowFeatureIds[runEnd] == featureId)
        {
          runEnd++;
        }

        maxFeatureId = std::max(maxFeatureId, featureId);
        if(featureId >= 0)
        {
          const usize featureIndex = static_cast<usize>(featureId);
          if(featureIndex < featureTupleCount)
          {
            ImageFeatureIndexBounds& featureBounds = indexBounds[featureIndex];
            featureBounds.minIndices[0] = std::min(featureBounds.minIndices[0], runStart);
            featureBounds.minIndices[1] = std::min(featureBounds.minIndices[1], y);
            featureBounds.minIndices[2] = std::min(featureBounds.minIndices[2], z);
            featureBounds.maxIndices[0] = std::max(featureBounds.maxIndices[0], runEnd - 1);
            featureBounds.maxIndices[1] = std::max(featureBounds.maxIndices[1], y);
            featureBounds.maxIndices[2] = std::max(featureBounds.maxIndices[2], z);
          }
        }
        runStart = runEnd;
      }
    }
  }

  ImageBoundsResult result;
  result.numFeatures = maxFeatureId + 1;
  if(result.numFeatures > featureTupleCount)
  {
    return result;
  }

  const FloatVec3 spacing = imageGeom.getSpacing();
  const FloatVec3 origin = imageGeom.getOrigin();
  result.bounds.resize(static_cast<usize>(result.numFeatures) * 6, std::numeric_limits<float32>::quiet_NaN());
  for(usize featureId = 0; featureId < static_cast<usize>(result.numFeatures); featureId++)
  {
    const ImageFeatureIndexBounds& featureBounds = indexBounds[featureId];
    if(featureBounds.minIndices[0] == k_InvalidIndex)
    {
      continue;
    }

    const usize activeIndex = featureId * 6;
    for(usize component = 0; component < 3; component++)
    {
      const bool isIncreasing = spacing[component] >= 0.0f;
      const usize minIndex = isIncreasing ? featureBounds.minIndices[component] : featureBounds.maxIndices[component];
      const usize maxIndex = isIncreasing ? featureBounds.maxIndices[component] : featureBounds.minIndices[component];
      result.bounds[activeIndex + component] = minIndex * spacing[component] + origin[component];
      result.bounds[activeIndex + 3 + component] = maxIndex * spacing[component] + origin[component] + spacing[component];
    }
  }
  return result;
}

/**
 * @brief Computes direct bounds for one supported geometry type.
 * @tparam GeomT Specifies the geometry type.
 * @param geom Supplies cells and vertex coordinates.
 * @param featureIds Supplies one Feature ID per cell.
 * @param numFeatures Identifies the allocated feature count.
 * @return Six bounds values per feature. Unobserved features retain NaN values.
 *
 * This helper uses direct element access and does not inspect cancellation.
 */
template <GeometryType GeomT>
std::vector<float32> ComputeBounds(const GeomT& geom, const Int32AbstractDataStore& featureIds, usize numFeatures)
{
  static_assert(std::is_same_v<ImageGeom, GeomT> || std::is_base_of_v<INodeGeometry0D, GeomT> || std::is_base_of_v<INodeGeometry1D, GeomT> || std::is_base_of_v<INodeGeometry2D, GeomT>);

  std::vector<float32> bounds(numFeatures * 6, std::numeric_limits<float32>::quiet_NaN());
  if constexpr(std::is_same_v<ImageGeom, GeomT>)
  {
    const usize xPoints = geom.getNumXCells();
    const usize yPoints = geom.getNumYCells();
    const usize zPoints = geom.getNumZCells();
    const FloatVec3 spacing = geom.getSpacing();
    const FloatVec3 origin = geom.getOrigin();

    usize zStride = 0;
    usize yStride = 0;
    for(usize i = 0; i < zPoints; i++)
    {
      zStride = i * xPoints * yPoints;
      for(usize j = 0; j < yPoints; j++)
      {
        yStride = j * xPoints;
        for(usize k = 0; k < xPoints; k++)
        {
          const int32 currentFeatureId = featureIds[zStride + yStride + k];
          if(currentFeatureId < 0)
          {
            continue;
          }

          const float32 xValMin = k * spacing[0] + origin[0];
          const float32 yValMin = j * spacing[1] + origin[1];
          const float32 zValMin = i * spacing[2] + origin[2];

          const float32 xValMax = k * spacing[0] + origin[0] + spacing[0];
          const float32 yValMax = j * spacing[1] + origin[1] + spacing[1];
          const float32 zValMax = i * spacing[2] + origin[2] + spacing[2];

          const usize activeIndex = static_cast<usize>(currentFeatureId) * 6;
          bounds[activeIndex + 0] = std::isnan(bounds[activeIndex + 0]) ? xValMin : std::min(bounds[activeIndex + 0], xValMin);
          bounds[activeIndex + 1] = std::isnan(bounds[activeIndex + 1]) ? yValMin : std::min(bounds[activeIndex + 1], yValMin);
          bounds[activeIndex + 2] = std::isnan(bounds[activeIndex + 2]) ? zValMin : std::min(bounds[activeIndex + 2], zValMin);

          bounds[activeIndex + 3] = std::isnan(bounds[activeIndex + 3]) ? xValMax : std::max(bounds[activeIndex + 3], xValMax);
          bounds[activeIndex + 4] = std::isnan(bounds[activeIndex + 4]) ? yValMax : std::max(bounds[activeIndex + 4], yValMax);
          bounds[activeIndex + 5] = std::isnan(bounds[activeIndex + 5]) ? zValMax : std::max(bounds[activeIndex + 5], zValMax);
        }
      }
    }
  }
  if constexpr(std::is_same_v<VertexGeom, GeomT>)
  {
    const IGeometry::SharedVertexList::store_type& verts = geom.getVerticesRef().getDataStoreRef();

    for(usize i = 0; i < verts.getNumberOfTuples(); i++)
    {
      const int32 currentFeatureId = featureIds[i];
      if(currentFeatureId < 0)
      {
        continue;
      }

      const float32 xVal = verts[(i * 3) + 0];
      const float32 yVal = verts[(i * 3) + 1];
      const float32 zVal = verts[(i * 3) + 2];

      const usize activeIndex = static_cast<usize>(currentFeatureId) * 6;
      bounds[activeIndex + 0] = std::isnan(bounds[activeIndex + 0]) ? xVal : std::min(bounds[activeIndex + 0], xVal);
      bounds[activeIndex + 1] = std::isnan(bounds[activeIndex + 1]) ? yVal : std::min(bounds[activeIndex + 1], yVal);
      bounds[activeIndex + 2] = std::isnan(bounds[activeIndex + 2]) ? zVal : std::min(bounds[activeIndex + 2], zVal);

      bounds[activeIndex + 3] = std::isnan(bounds[activeIndex + 3]) ? xVal : std::max(bounds[activeIndex + 3], xVal);
      bounds[activeIndex + 4] = std::isnan(bounds[activeIndex + 4]) ? yVal : std::max(bounds[activeIndex + 4], yVal);
      bounds[activeIndex + 5] = std::isnan(bounds[activeIndex + 5]) ? zVal : std::max(bounds[activeIndex + 5], zVal);
    }
  }
  if constexpr(std::is_same_v<EdgeGeom, GeomT>)
  {
    const IGeometry::SharedVertexList::store_type& verts = geom.getVerticesRef().getDataStoreRef();
    const IGeometry::SharedEdgeList::store_type& edges = geom.getEdgesRef().getDataStoreRef();

    const usize numComp = edges.getNumberOfComponents();
    for(usize i = 0; i < edges.getNumberOfTuples(); i++)
    {
      const int32 currentFeatureId = featureIds[i];
      if(currentFeatureId < 0)
      {
        continue;
      }

      for(usize comp = 0; comp < numComp; comp++)
      {
        const IGeometry::SharedFaceList::value_type activeVertIndex = edges[(i * numComp) + comp];
        const float32 xVal = verts[(activeVertIndex * 3) + 0];
        const float32 yVal = verts[(activeVertIndex * 3) + 1];
        const float32 zVal = verts[(activeVertIndex * 3) + 2];

        const usize activeIndex = static_cast<usize>(currentFeatureId) * 6;
        bounds[activeIndex + 0] = std::isnan(bounds[activeIndex + 0]) ? xVal : std::min(bounds[activeIndex + 0], xVal);
        bounds[activeIndex + 1] = std::isnan(bounds[activeIndex + 1]) ? yVal : std::min(bounds[activeIndex + 1], yVal);
        bounds[activeIndex + 2] = std::isnan(bounds[activeIndex + 2]) ? zVal : std::min(bounds[activeIndex + 2], zVal);

        bounds[activeIndex + 3] = std::isnan(bounds[activeIndex + 3]) ? xVal : std::max(bounds[activeIndex + 3], xVal);
        bounds[activeIndex + 4] = std::isnan(bounds[activeIndex + 4]) ? yVal : std::max(bounds[activeIndex + 4], yVal);
        bounds[activeIndex + 5] = std::isnan(bounds[activeIndex + 5]) ? zVal : std::max(bounds[activeIndex + 5], zVal);
      }
    }
  }
  if constexpr(std::is_same_v<TriangleGeom, GeomT> || std::is_same_v<QuadGeom, GeomT>)
  {
    const IGeometry::SharedVertexList::store_type& verts = geom.getVerticesRef().getDataStoreRef();
    const IGeometry::SharedFaceList::store_type& faces = geom.getFacesRef().getDataStoreRef();

    const usize numComp = faces.getNumberOfComponents();
    for(usize i = 0; i < faces.getNumberOfTuples(); i++)
    {
      const int32 currentFeatureId = featureIds[i];
      if(currentFeatureId < 0)
      {
        continue;
      }

      for(usize comp = 0; comp < numComp; comp++)
      {
        const IGeometry::SharedFaceList::value_type activeVertIndex = faces[(i * numComp) + comp];
        const float32 xVal = verts[(activeVertIndex * 3) + 0];
        const float32 yVal = verts[(activeVertIndex * 3) + 1];
        const float32 zVal = verts[(activeVertIndex * 3) + 2];

        const usize activeIndex = static_cast<usize>(currentFeatureId) * 6;
        bounds[activeIndex + 0] = std::isnan(bounds[activeIndex + 0]) ? xVal : std::min(bounds[activeIndex + 0], xVal);
        bounds[activeIndex + 1] = std::isnan(bounds[activeIndex + 1]) ? yVal : std::min(bounds[activeIndex + 1], yVal);
        bounds[activeIndex + 2] = std::isnan(bounds[activeIndex + 2]) ? zVal : std::min(bounds[activeIndex + 2], zVal);

        bounds[activeIndex + 3] = std::isnan(bounds[activeIndex + 3]) ? xVal : std::max(bounds[activeIndex + 3], xVal);
        bounds[activeIndex + 4] = std::isnan(bounds[activeIndex + 4]) ? yVal : std::max(bounds[activeIndex + 4], yVal);
        bounds[activeIndex + 5] = std::isnan(bounds[activeIndex + 5]) ? zVal : std::max(bounds[activeIndex + 5], zVal);
      }
    }
  }

  return bounds;
}

/**
 * @brief Selects a direct bounds helper for a geometry type.
 * @tparam ArgsT Specifies forwarded helper argument types.
 * @param geom Supplies the input geometry.
 * @param args Forwards arguments to the selected helper.
 * @return Bounds values for a supported geometry. Returns an empty vector otherwise.
 */
template <class... ArgsT>
std::vector<float32> ExecuteComputeBounds(const IGeometry& geom, ArgsT&&... args)
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
    return {};
  }
  }
}
} // namespace

ComputeFeatureBoundsDirect::ComputeFeatureBoundsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                       const ComputeFeatureBoundsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeFeatureBoundsDirect::~ComputeFeatureBoundsDirect() noexcept = default;

Result<> ComputeFeatureBoundsDirect::operator()()
{
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
  const auto& featureAM = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->FeatureAMPath);

  const auto* inMemoryFeatureIds = dynamic_cast<const Int32DataStore*>(&featureIds);
  const auto& geom = m_DataStructure.getDataRefAs<IGeometry>(m_InputValues->GeometryPath);

  if(m_ShouldCancel)
  {
    return {};
  }

  int32 numFeatures = 0;
  std::vector<float32> bounds;
  if(geom.getGeomType() == IGeometry::Type::Image && inMemoryFeatureIds != nullptr)
  {
    ImageBoundsResult imageBounds = ComputeImageBounds(static_cast<const ImageGeom&>(geom), inMemoryFeatureIds->data(), featureAM.getNumberOfTuples());
    numFeatures = imageBounds.numFeatures;
    bounds = std::move(imageBounds.bounds);
  }
  else
  {
    const int32 maxFeatureId = inMemoryFeatureIds != nullptr ? *std::max_element(inMemoryFeatureIds->data(), inMemoryFeatureIds->data() + inMemoryFeatureIds->getSize()) :
                                                               *std::max_element(featureIds.cbegin(), featureIds.cend());
    numFeatures = maxFeatureId + 1;
  }

  if(numFeatures > featureAM.getNumberOfTuples())
  {
    return MakeErrorResult(-89471, fmt::format("{} Attribute Matrix size ({}) doesn't align with number of features ({}) in {} array", m_InputValues->FeatureAMPath.getTargetName(),
                                               featureAM.getNumberOfTuples(), numFeatures, m_InputValues->FeatureIdsArrayPath.getTargetName()));
  }

  if(bounds.empty())
  {
    bounds = ExecuteComputeBounds(geom, featureIds, numFeatures);
  }
  if(bounds.empty())
  {
    return MakeErrorResult(-89472, fmt::format("Input geometry must be of type(s) [ Image, Triangle, Vertex, Edge, Quad ]. Supplied geometry name {}", geom.getName()));
  }

  switch(static_cast<ComputeFeatureBounds::OutputDataType>(m_InputValues->OutputType))
  {
  case ComputeFeatureBounds::OutputDataType::Split: {
    auto& minBounds = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->MinArrayPath).getDataStoreRef();
    auto& maxBounds = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->MaxArrayPath).getDataStoreRef();
    for(usize i = 0; i < minBounds.getNumberOfTuples(); i++)
    {
      const usize activeIndex = i * 6;
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
      const usize activeIndex = i * 6;
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
    static constexpr std::array<std::pair<int, int>, 12> k_CubeEdges = {{{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}}};
    std::array<usize, 2> vertPair = {0, 0};

    const usize numVerts = numFeatures * 8;
    const usize numEdges = numFeatures * 12;

    auto& edgeGeom = m_DataStructure.getDataRefAs<EdgeGeom>(m_InputValues->EdgeGeometryDataPath);
    edgeGeom.resizeVertexList(numVerts);
    edgeGeom.resizeEdgeList(numEdges);
    edgeGeom.getEdgeAttributeMatrix()->resizeTuples({numEdges});
    edgeGeom.getVertexAttributeMatrix()->resizeTuples({numVerts});

    const DataPath edgeAmPath = m_InputValues->EdgeGeometryDataPath.createChildPath(m_InputValues->EdgeAttributeMatrixName);
    auto& edgeFeatureIds = m_DataStructure.getDataRefAs<Int32Array>(edgeAmPath.createChildPath(m_InputValues->FeatureIdsArrayName)).getDataStoreRef();
    edgeFeatureIds.fill(-1);
    usize currentOffset = 0;
    for(usize idx = 0; idx < numFeatures; ++idx)
    {
      const usize activeIndex = idx * 6;
      bool foundNan = false;
      for(usize i = 0; i < 6; i++)
      {
        if(std::isnan(bounds[activeIndex + i]))
        {
          foundNan = true;
          break;
        }
      }
      if(foundNan)
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

      for(usize edgeIdx = 0; edgeIdx < k_CubeEdges.size(); ++edgeIdx)
      {
        vertPair[0] = currentOffset * 8 + k_CubeEdges[edgeIdx].first;
        vertPair[1] = currentOffset * 8 + k_CubeEdges[edgeIdx].second;

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
