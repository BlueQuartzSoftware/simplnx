#include "CropEdgeGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

namespace
{
/**
 * @brief
 * @tparam T
 */
template <typename T>
class CropEdgeGeomArray
{
public:
  CropEdgeGeomArray(const IDataArray& oldCellArray, IDataArray& newCellArray, const AttributeMatrix& srcAttrMatrix, const std::vector<bool>& tupleMask, const std::atomic_bool& shouldCancel)
  : m_OldCellStore(oldCellArray.template getIDataStoreRefAs<AbstractDataStore<T>>())
  , m_NewCellStore(newCellArray.template getIDataStoreRefAs<AbstractDataStore<T>>())
  , m_SrcAttrMatrix(srcAttrMatrix)
  , m_TupleMask(tupleMask)
  , m_ShouldCancel(shouldCancel)
  {
  }

  ~CropEdgeGeomArray() = default;

  CropEdgeGeomArray(const CropEdgeGeomArray&) = default;
  CropEdgeGeomArray(CropEdgeGeomArray&&) noexcept = default;
  CropEdgeGeomArray& operator=(const CropEdgeGeomArray&) = delete;
  CropEdgeGeomArray& operator=(CropEdgeGeomArray&&) noexcept = delete;

  void operator()() const
  {
    usize newIndex = 0;
    for(usize i = 0; i < m_SrcAttrMatrix.getNumTuples(); ++i)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      else if(m_TupleMask[i])
      {
        CopyFromArray::CopyData(m_OldCellStore, m_NewCellStore, newIndex, i, 1);
        newIndex++;
      }
    }
  }

private:
  const AbstractDataStore<T>& m_OldCellStore;
  AbstractDataStore<T>& m_NewCellStore;
  const AttributeMatrix& m_SrcAttrMatrix;
  const std::vector<bool> m_TupleMask;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @brief Clips the parameter t_min and t_max based on the provided coordinates and bounds.
 *
 * This function adjusts the t_min and t_max parameters to ensure that the interpolated
 * point lies within the specified minimum and maximum bounds for a single axis.
 *
 * @param t_min Current minimum value of the interpolation parameter t.
 * @param t_max Current maximum value of the interpolation parameter t.
 * @param v1    Coordinate of the inside vertex along a specific axis.
 * @param v2    Coordinate of the outside vertex along the same axis.
 * @param vmin  Minimum allowable value along the axis.
 * @param vmax  Maximum allowable value along the axis.
 * @return std::pair<float32, float32> Updated (t_min, t_max) after clipping.
 */
std::pair<float32, float32> clip(float32 t_min, float32 t_max, float32 v1, float32 v2, float32 vmin, float32 vmax)
{
  if(v1 < v2)
  { // Moving towards increasing coordinate
    if(v2 > vmax)
    {
      t_max = std::min(t_max, (vmax - v1) / (v2 - v1));
    }
    if(v2 < vmin)
    {
      t_min = std::max(t_min, (vmin - v1) / (v2 - v1));
    }
  }
  else
  { // Moving towards decreasing coordinate
    if(v2 < vmin)
    {
      t_max = std::min(t_max, (vmin - v1) / (v2 - v1));
    }
    if(v2 > vmax)
    {
      t_min = std::max(t_min, (vmax - v1) / (v2 - v1));
    }
  }
  return {t_min, t_max};
}

/**
 * @brief Interpolates the position of an outside vertex to lie within the bounding box.
 *
 * This function adjusts the coordinates of an outside vertex so that it lies on the boundary
 * of the specified bounding box along the line connecting it to an inside vertex.
 *
 * @param insideVertex 3D coordinates of the inside vertex as a tuple (x, y, z).
 * @param outsideVertex 3D coordinates of the outside vertex as a tuple (x, y, z).
 * @param boundingBox The bounding box to interpolate the outside vertex to lie within.
 * MUST be in the format (x_min, y_min, z_min, x_max, y_max, z_max).
 * @return std::tuple<float32, float32, float32> Adjusted 3D coordinates of the outside vertex.
 */
std::tuple<float32, float32, float32> interpolate_outside_vertex(const std::tuple<float32, float32, float32>& insideVertex, const std::tuple<float32, float32, float32>& outsideVertex,
                                                                 std::array<float32, 6> boundingBox)
{
  float32 x1 = std::get<0>(insideVertex);
  float32 y1 = std::get<1>(insideVertex);
  float32 z1 = std::get<2>(insideVertex);
  float32 x2 = std::get<0>(outsideVertex);
  float32 y2 = std::get<1>(outsideVertex);
  float32 z2 = std::get<2>(outsideVertex);

  float32 t_min = 0.0f;
  float32 t_max = 1.0f;

  float32 x_min = boundingBox[0];
  float32 y_min = boundingBox[1];
  float32 z_min = boundingBox[2];
  float32 x_max = boundingBox[3];
  float32 y_max = boundingBox[4];
  float32 z_max = boundingBox[5];

  // Apply clipping for each axis: X, Y, Z
  std::tie(t_min, t_max) = clip(t_min, t_max, x1, x2, x_min, x_max);
  std::tie(t_min, t_max) = clip(t_min, t_max, y1, y2, y_min, y_max);
  std::tie(t_min, t_max) = clip(t_min, t_max, z1, z2, z_min, z_max);

  if(t_min > t_max)
  {
    // No intersection with the bounding box; return the original outside vertex
    return {x2, y2, z2};
  }

  // Use t_max to find the intersection point on the bounding box
  float32 t = t_max;

  // Calculate the new coordinates by interpolating with parameter t
  float32 new_x = x1 + t * (x2 - x1);
  float32 new_y = y1 + t * (y2 - y1);
  float32 new_z = z1 + t * (z2 - z1);

  return {new_x, new_y, new_z};
}

bool is_inside(float32 x, float32 y, float32 z, const std::array<float32, 6>& boundingBox)
{
  return (x >= boundingBox[0] && x <= boundingBox[3]) && (y >= boundingBox[1] && y <= boundingBox[4]) && (z >= boundingBox[2] && z <= boundingBox[5]);
}
} // namespace

// -----------------------------------------------------------------------------
CropEdgeGeometry::CropEdgeGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, CropEdgeGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
{
}

// -----------------------------------------------------------------------------
CropEdgeGeometry::~CropEdgeGeometry() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& CropEdgeGeometry::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> CropEdgeGeometry::operator()()
{
  DataPath destEdgeGeomPath = m_InputValues->destEdgeGeomPath;
  if(m_InputValues->removeOriginalGeometry)
  {
    auto tempPathVector = m_InputValues->srcEdgeGeomPath.getPathVector();
    auto tempName = k_TempGeometryName;
    tempPathVector.back() = tempName;
    destEdgeGeomPath = DataPath({tempPathVector});
  }

  float32 xMin = m_InputValues->cropXDim ? m_InputValues->minCoords[0] : std::numeric_limits<float32>::lowest();
  float32 xMax = m_InputValues->cropXDim ? m_InputValues->maxCoords[0] : std::numeric_limits<float32>::max();
  float32 yMin = m_InputValues->cropYDim ? m_InputValues->minCoords[1] : std::numeric_limits<float32>::lowest();
  float32 yMax = m_InputValues->cropYDim ? m_InputValues->maxCoords[1] : std::numeric_limits<float32>::max();
  float32 zMin = m_InputValues->cropZDim ? m_InputValues->minCoords[2] : std::numeric_limits<float32>::lowest();
  float32 zMax = m_InputValues->cropZDim ? m_InputValues->maxCoords[2] : std::numeric_limits<float32>::max();
  std::array<float32, 6> boundingBox = {xMin, yMin, zMin, xMax, yMax, zMax};

  auto& srcEdgeGeom = m_DataStructure.getDataRefAs<EdgeGeom>(m_InputValues->srcEdgeGeomPath);
  auto& srcVertices = srcEdgeGeom.getVerticesRef();
  auto& srcEdges = srcEdgeGeom.getEdgesRef();
  auto& srcVertexAttrMatrix = srcEdgeGeom.getVertexAttributeMatrixRef();
  auto& srcEdgesAttrMatrix = srcEdgeGeom.getEdgeAttributeMatrixRef();

  auto& destEdgeGeom = m_DataStructure.getDataRefAs<EdgeGeom>(destEdgeGeomPath);
  auto& destVertexAttrMatrix = destEdgeGeom.getVertexAttributeMatrixRef();
  auto& destEdgesAttrMatrix = destEdgeGeom.getEdgeAttributeMatrixRef();
  auto& destVertices = destEdgeGeom.getVerticesRef();
  auto& destEdges = destEdgeGeom.getEdgesRef();

  usize numVertices = srcVertices.getNumberOfTuples();
  usize numEdges = srcEdges.getNumberOfTuples();

  auto behavior = static_cast<BoundaryIntersectionBehavior>(m_InputValues->boundaryIntersectionBehavior);

  // Resize vertices, vertex attribute matrix, edges, and edges attribute matrix to the maximum size
  destVertices.resizeTuples({numVertices});
  destVertexAttrMatrix.resizeTuples({numVertices});
  destEdges.resizeTuples({numEdges});
  destEdgesAttrMatrix.resizeTuples({numEdges});

  std::vector<bool> edgesMask(numEdges, false);
  std::vector<bool> vertexReferenced(numVertices, false);
  std::unordered_map<uint64, std::tuple<float32, float32, float32>> interpolatedValuesMap;

  for(usize i = 0; i < numEdges; ++i)
  {
    uint64 v0 = srcEdges[2 * i + 0];
    uint64 v1 = srcEdges[2 * i + 1];

    // Validate vertex indices
    if(v0 >= numVertices)
    {
      return MakeErrorResult(to_underlying(ErrorCodes::InvalidVertexIndex), fmt::format("Edge at index {} with value {} references an invalid vertex index.", 2 * i, v0));
    }

    if(v1 >= numVertices)
    {
      return MakeErrorResult(to_underlying(ErrorCodes::InvalidVertexIndex), fmt::format("Edge at index {} with value {} references an invalid vertex index.", 2 * i + 1, v1));
    }

    // Determine if each vertex is inside or outside
    float32 x1 = srcVertices[3 * v0 + 0];
    float32 y1 = srcVertices[3 * v0 + 1];
    float32 z1 = srcVertices[3 * v0 + 2];
    bool v0_inside = is_inside(x1, y1, z1, boundingBox);

    float32 x2 = srcVertices[3 * v1 + 0];
    float32 y2 = srcVertices[3 * v1 + 1];
    float32 z2 = srcVertices[3 * v1 + 2];
    bool v1_inside = is_inside(x2, y2, z2, boundingBox);

    bool edgeOutsideBoundary = !v0_inside && !v1_inside;
    bool edgeInsideBoundary = v0_inside && v1_inside;
    bool edgeIntersectingBoundary = (v0_inside && !v1_inside) || (!v0_inside && v1_inside);

    if(edgeOutsideBoundary)
    {
      continue;
    }

    if(edgeIntersectingBoundary)
    {
      // Found an edge intersecting the boundary
      // Calculate the interpolated value for the outside vertex and store it in the interpolatedValuesMap
      uint64 insideVertexIdx = v0;
      uint64 outsideVertexIdx = v1;
      if(v1_inside)
      {
        insideVertexIdx = v1;
        outsideVertexIdx = v0;
      }

      float32 insideX = srcVertices[3 * insideVertexIdx + 0];
      float32 insideY = srcVertices[3 * insideVertexIdx + 1];
      float32 insideZ = srcVertices[3 * insideVertexIdx + 2];
      float32 outsideX = srcVertices[3 * outsideVertexIdx + 0];
      float32 outsideY = srcVertices[3 * outsideVertexIdx + 1];
      float32 outsideZ = srcVertices[3 * outsideVertexIdx + 2];

      if(behavior == BoundaryIntersectionBehavior::FilterError)
      {
        // Throw a filter error
        std::string message = fmt::format("Edge {} connects inside vertex ({}, {}, {}) with outside vertex ({}, {}, {}).  This intersects the bounds of ({}, {}, {}) and ({}, {}, {})",
                                          std::to_string(i), std::to_string(insideX), std::to_string(insideY), std::to_string(insideZ), std::to_string(outsideX), std::to_string(outsideY),
                                          std::to_string(outsideZ), boundingBox[0], boundingBox[1], boundingBox[2], boundingBox[3], boundingBox[4], boundingBox[5]);
        return MakeErrorResult(to_underlying(ErrorCodes::OutsideVertexError), message);
      }
      else if(behavior == BoundaryIntersectionBehavior::InterpolateOutsideVertex)
      {
        // Calculate the interpolated value for the outside vertex and store it in the interpolatedValuesMap
        interpolatedValuesMap[outsideVertexIdx] = interpolate_outside_vertex(std::make_tuple(insideX, insideY, insideZ), std::make_tuple(outsideX, outsideY, outsideZ), boundingBox);
      }
    }

    if(edgeInsideBoundary || (edgeIntersectingBoundary && behavior != BoundaryIntersectionBehavior::IgnoreEdge))
    {
      vertexReferenced[v0] = true;
      vertexReferenced[v1] = true;
      edgesMask[i] = true;
    }
  }

  // Tally up the number of vertices referenced and edges kept
  usize totalVerticesReferenced = std::count(vertexReferenced.begin(), vertexReferenced.end(), true);
  usize totalEdgesKept = std::count(edgesMask.begin(), edgesMask.end(), true);

  // Resize to proper sizes
  destVertices.resizeTuples({totalVerticesReferenced});
  destVertexAttrMatrix.resizeTuples({totalVerticesReferenced});
  destEdges.resizeTuples({totalEdgesKept});
  destEdgesAttrMatrix.resizeTuples({totalEdgesKept});

  // Create a mapping from old vertex indices to new indices
  std::vector<int64> vertexMapping(numVertices, -1);

  int64 newIndex = 0;
  for(usize i = 0; i < numVertices; ++i)
  {
    if(vertexReferenced[i])
    {
      vertexMapping[i] = newIndex;
      if(behavior == BoundaryIntersectionBehavior::InterpolateOutsideVertex && interpolatedValuesMap.contains(i))
      {
        destVertices[3 * newIndex + 0] = std::get<0>(interpolatedValuesMap[i]);
        destVertices[3 * newIndex + 1] = std::get<1>(interpolatedValuesMap[i]);
        destVertices[3 * newIndex + 2] = std::get<2>(interpolatedValuesMap[i]);
      }
      else
      {
        destVertices[3 * newIndex + 0] = srcVertices[3 * i + 0];
        destVertices[3 * newIndex + 1] = srcVertices[3 * i + 1];
        destVertices[3 * newIndex + 2] = srcVertices[3 * i + 2];
      }
      newIndex++;
    }
  }

  // Crop each vertex data array in parallel
  {
    ParallelTaskAlgorithm taskRunner;
    for(const auto& [dataId, oldDataObject] : srcVertexAttrMatrix)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      const auto& oldDataArray = dynamic_cast<const IDataArray&>(*oldDataObject);
      const std::string srcName = oldDataArray.getName();

      auto& newDataArray = dynamic_cast<IDataArray&>(destVertexAttrMatrix.at(srcName));

      m_MessageHandler(fmt::format("Cropping Volume || Copying Vertex Array {}", srcName));
      ExecuteParallelFunction<CropEdgeGeomArray>(oldDataArray.getDataType(), taskRunner, oldDataArray, newDataArray, srcVertexAttrMatrix, vertexReferenced, m_ShouldCancel);
    }
    taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.
  }

  // Create final edges with remapped vertex indices
  newIndex = 0;
  for(usize i = 0; i < numEdges; ++i)
  {
    if(edgesMask[i])
    {
      // Get new vertices via indexing into vertex mapping with the old index from srcEdges
      int64 new_v0 = vertexMapping[srcEdges[2 * i + 0]];
      int64 new_v1 = vertexMapping[srcEdges[2 * i + 1]];

      // Validate mapping
      if(new_v0 == -1 || new_v1 == -1)
      {
        return MakeErrorResult(to_underlying(ErrorCodes::InvalidVertexMapping), "Invalid vertex mapping during edge remapping.");
      }

      destEdges[newIndex++] = static_cast<uint64>(new_v0);
      destEdges[newIndex++] = static_cast<uint64>(new_v1);
    }
  }

  // Crop each edge data array in parallel
  {
    ParallelTaskAlgorithm taskRunner;
    for(const auto& [dataId, oldDataObject] : srcEdgesAttrMatrix)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      const auto& oldDataArray = dynamic_cast<const IDataArray&>(*oldDataObject);
      const std::string srcName = oldDataArray.getName();

      auto& newDataArray = dynamic_cast<IDataArray&>(destEdgesAttrMatrix.at(srcName));

      m_MessageHandler(fmt::format("Cropping Volume || Copying Edge Array {}", srcName));
      ExecuteParallelFunction<CropEdgeGeomArray>(oldDataArray.getDataType(), taskRunner, oldDataArray, newDataArray, srcEdgesAttrMatrix, edgesMask, m_ShouldCancel);
    }
    taskRunner.wait();
  }

  return {};
}
