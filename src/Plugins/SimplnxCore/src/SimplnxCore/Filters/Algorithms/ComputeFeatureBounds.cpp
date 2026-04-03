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
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

namespace
{
template <typename T>
concept GeometryType = std::is_base_of_v<IGeometry, T>;

template <GeometryType GeomT>
std::vector<float32> ComputeBounds(const GeomT& geom, const Int32AbstractDataStore& featureIds, usize numFeatures)
{
  static_assert(std::is_same_v<ImageGeom, GeomT> || std::is_base_of_v<INodeGeometry0D, GeomT> || std::is_base_of_v<INodeGeometry1D, GeomT> || std::is_base_of_v<INodeGeometry2D, GeomT>);

  std::vector<float32> bounds(numFeatures * 6, std::numeric_limits<float32>::quiet_NaN());
  if constexpr(std::is_same_v<ImageGeom, GeomT>)
  {
    usize xPoints = geom.getNumXCells();
    usize yPoints = geom.getNumYCells();
    usize zPoints = geom.getNumZCells();
    FloatVec3 spacing = geom.getSpacing();
    FloatVec3 origin = geom.getOrigin();

    usize zStride = 0, yStride = 0;
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

          float32 xValMin = k * spacing[0] + origin[0];
          float32 yValMin = j * spacing[1] + origin[1];
          float32 zValMin = i * spacing[2] + origin[2];

          float32 xValMax = k * spacing[0] + origin[0] + spacing[0];
          float32 yValMax = j * spacing[1] + origin[1] + spacing[1];
          float32 zValMax = i * spacing[2] + origin[2] + spacing[2];

          usize activeIndex = currentFeatureId * 6;
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

      float32 xVal = verts[(i * 3) + 0];
      float32 yVal = verts[(i * 3) + 1];
      float32 zVal = verts[(i * 3) + 2];

      usize activeIndex = currentFeatureId * 6;
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

    usize numComp = edges.getNumberOfComponents();
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
        float32 xVal = verts[(activeVertIndex * 3) + 0];
        float32 yVal = verts[(activeVertIndex * 3) + 1];
        float32 zVal = verts[(activeVertIndex * 3) + 2];

        usize activeIndex = currentFeatureId * 6;
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

    usize numComp = faces.getNumberOfComponents();
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
        float32 xVal = verts[(activeVertIndex * 3) + 0];
        float32 yVal = verts[(activeVertIndex * 3) + 1];
        float32 zVal = verts[(activeVertIndex * 3) + 2];

        usize activeIndex = currentFeatureId * 6;
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

// -----------------------------------------------------------------------------
ComputeFeatureBounds::ComputeFeatureBounds(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureBoundsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFeatureBounds::~ComputeFeatureBounds() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeFeatureBounds::operator()()
{
  MessageHelper messageHelper(m_MessageHandler);
  messageHelper.sendMessage("Computing Feature Bounds...");

  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
  const auto& featureAM = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->FeatureAMPath);

  const int32 numFeatures = (*std::max_element(featureIds.cbegin(), featureIds.cend())) + 1;
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

  std::vector<float32> bounds = ExecuteComputeBounds(geom, featureIds, numFeatures);
  if(bounds.empty())
  {
    return MakeErrorResult(-89472, fmt::format("Input geometry must be of type(s) [ Image, Triangle, Vertex, Edge, Quad ]. Supplied geometry name {}", geom.getName()));
  }

  switch(static_cast<OutputDataType>(m_InputValues->OutputType))
  {
  case OutputDataType::Split: {
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
  case OutputDataType::Unified: {
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
    // define all 12 cube edges as pairs of vertex indices
    static constexpr std::array<std::pair<int, int>, 12> cubeEdges = {{// bottom face
                                                                       {0, 1},
                                                                       {1, 2},
                                                                       {2, 3},
                                                                       {3, 0},
                                                                       // top face
                                                                       {4, 5},
                                                                       {5, 6},
                                                                       {6, 7},
                                                                       {7, 4},
                                                                       // vertical sides
                                                                       {0, 4},
                                                                       {1, 5},
                                                                       {2, 6},
                                                                       {3, 7}}};
    std::array<usize, 2> vertPair = {0, 0};

    // Compute the number of features which will tell use the number of vertices and edges
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
      // NaN values mean that there was something wrong with the bounding min/max points.
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

      // Create the 8 Vertices
      edgeGeom.setVertexCoordinate(currentOffset * 8 + 0, {bounds[activeIndex + 0], bounds[activeIndex + 1], bounds[activeIndex + 2]});
      edgeGeom.setVertexCoordinate(currentOffset * 8 + 1, {bounds[activeIndex + 3], bounds[activeIndex + 1], bounds[activeIndex + 2]});
      edgeGeom.setVertexCoordinate(currentOffset * 8 + 2, {bounds[activeIndex + 3], bounds[activeIndex + 4], bounds[activeIndex + 2]});
      edgeGeom.setVertexCoordinate(currentOffset * 8 + 3, {bounds[activeIndex + 0], bounds[activeIndex + 4], bounds[activeIndex + 2]});
      edgeGeom.setVertexCoordinate(currentOffset * 8 + 4, {bounds[activeIndex + 0], bounds[activeIndex + 1], bounds[activeIndex + 5]});
      edgeGeom.setVertexCoordinate(currentOffset * 8 + 5, {bounds[activeIndex + 3], bounds[activeIndex + 1], bounds[activeIndex + 5]});
      edgeGeom.setVertexCoordinate(currentOffset * 8 + 6, {bounds[activeIndex + 3], bounds[activeIndex + 4], bounds[activeIndex + 5]});
      edgeGeom.setVertexCoordinate(currentOffset * 8 + 7, {bounds[activeIndex + 0], bounds[activeIndex + 4], bounds[activeIndex + 5]});

      // Create the 12 Edges
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
    // Update the Edge Geometry sizes
    edgeGeom.resizeVertexList(currentOffset * 8);
    edgeGeom.getVertexAttributeMatrix()->resizeTuples({currentOffset * 8});
    edgeGeom.resizeEdgeList(currentOffset * 12);
    edgeGeom.getEdgeAttributeMatrix()->resizeTuples({currentOffset * 12});
  }

  return {};
}
