#include "ExtractFeatureBoundaries2D.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/GeometryUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <atomic>

using namespace nx::core;

namespace
{
// =============================================================================
// WORKER CLASSES
// =============================================================================
// These classes encapsulate the boundary detection and edge creation logic.
// Each class operates on a range of rows (Y indices) and processes all X
// positions within those rows.
//
// The algorithm uses a two-pass approach:
// 1. Count Pass: Count the total number of boundary edges (CountVerticalEdgesImpl,
//    CountHorizontalEdgesImpl) so we can allocate the exact amount of memory needed.
// 2. Populate Pass: Create the actual vertices and edges (PopulateVerticalEdgesImpl,
//    PopulateHorizontalEdgesImpl).
// =============================================================================
/**
 * @brief Counts vertical boundary edges (edges between horizontally adjacent cells)
 *
 * A vertical edge exists between cell (x, y) and cell (x+1, y) when they have
 * different feature IDs. The edge is placed at the right side of cell (x, y).
 *
 * Grid visualization (4x3 grid):
 *   +---+---+---+---+
 *   | 0 | 1 | 2 | 3 |  y=2
 *   +---+---+---+---+
 *   | 0 | 1 | 2 | 3 |  y=1
 *   +---+---+---+---+
 *   | 0 | 1 | 2 | 3 |  y=0
 *   +---+---+---+---+
 *     ^   ^   ^
 *     Vertical edges checked between adjacent cells in X direction
 */
template <typename T>
class CountVerticalEdgesImpl
{
public:
  CountVerticalEdgesImpl(const AbstractDataStore<T>& featureIds, usize dimX, usize dimY, std::atomic<usize>& edgeCount, const std::atomic_bool& shouldCancel)
  : m_FeatureIds(featureIds)
  , m_DimX(dimX)
  , m_DimY(dimY)
  , m_EdgeCount(edgeCount)
  , m_ShouldCancel(shouldCancel)
  {
  }

  void operator()(const Range& range) const
  {
    usize localCount = 0;
    for(usize y = range.min(); y < range.max(); y++)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      for(usize x = 0; x < m_DimX - 1; x++)
      {
        usize idx1 = y * m_DimX + x;
        usize idx2 = y * m_DimX + (x + 1);
        if(m_FeatureIds[idx1] != m_FeatureIds[idx2])
        {
          localCount++;
        }
      }
    }
    m_EdgeCount.fetch_add(localCount, std::memory_order_relaxed);
  }

private:
  const AbstractDataStore<T>& m_FeatureIds;
  usize m_DimX;
  usize m_DimY;
  std::atomic<usize>& m_EdgeCount;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @brief Counts horizontal boundary edges (edges between vertically adjacent cells)
 *
 * A horizontal edge exists between cell (x, y) and cell (x, y+1) when they have
 * different feature IDs. The edge is placed at the top side of cell (x, y).
 *
 * Grid visualization (4x3 grid):
 *   +---+---+---+---+
 *   | 0 | 1 | 2 | 3 |  y=2
 *   +---+---+---+---+  <- Horizontal edges checked here (y=1 to y=2)
 *   | 0 | 1 | 2 | 3 |  y=1
 *   +---+---+---+---+  <- Horizontal edges checked here (y=0 to y=1)
 *   | 0 | 1 | 2 | 3 |  y=0
 *   +---+---+---+---+
 */
template <typename T>
class CountHorizontalEdgesImpl
{
public:
  CountHorizontalEdgesImpl(const AbstractDataStore<T>& featureIds, usize dimX, usize dimY, std::atomic<usize>& edgeCount, const std::atomic_bool& shouldCancel)
  : m_FeatureIds(featureIds)
  , m_DimX(dimX)
  , m_DimY(dimY)
  , m_EdgeCount(edgeCount)
  , m_ShouldCancel(shouldCancel)
  {
  }

  void operator()(const Range& range) const
  {
    usize localCount = 0;
    for(usize y = range.min(); y < range.max(); y++)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      for(usize x = 0; x < m_DimX; x++)
      {
        usize idx1 = y * m_DimX + x;
        usize idx2 = (y + 1) * m_DimX + x;
        if(m_FeatureIds[idx1] != m_FeatureIds[idx2])
        {
          localCount++;
        }
      }
    }
    m_EdgeCount.fetch_add(localCount, std::memory_order_relaxed);
  }

private:
  const AbstractDataStore<T>& m_FeatureIds;
  usize m_DimX;
  usize m_DimY;
  std::atomic<usize>& m_EdgeCount;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @brief Creates vertices and edges for vertical boundaries
 *
 * For each boundary found between horizontally adjacent cells, this creates:
 * - Two vertices at the top and bottom of the cell interface
 * - One edge connecting those two vertices
 *
 * Uses atomic counter (m_CurrentEdge) to allocate unique edge indices.
 * Each edge gets 2 vertices stored consecutively (v0, v1) in the vertex array.
 */
template <typename T>
class PopulateVerticalEdgesImpl
{
public:
  PopulateVerticalEdgesImpl(const AbstractDataStore<T>& featureIds, usize dimX, usize dimY, float32 originX, float32 originY, float32 originZ, float32 spacingX, float32 spacingY,
                            INodeGeometry0D::SharedVertexList& vertices, INodeGeometry1D::SharedEdgeList& edges, std::atomic<usize>& currentEdge, const std::atomic_bool& shouldCancel)
  : m_FeatureIds(featureIds)
  , m_DimX(dimX)
  , m_DimY(dimY)
  , m_OriginX(originX)
  , m_OriginY(originY)
  , m_OriginZ(originZ)
  , m_SpacingX(spacingX)
  , m_SpacingY(spacingY)
  , m_Vertices(vertices)
  , m_Edges(edges)
  , m_CurrentEdge(currentEdge)
  , m_ShouldCancel(shouldCancel)
  {
  }

  void operator()(const Range& range) const
  {
    for(usize y = range.min(); y < range.max(); y++)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      for(usize x = 0; x < m_DimX - 1; x++)
      {
        usize idx1 = y * m_DimX + x;
        usize idx2 = y * m_DimX + (x + 1);
        if(m_FeatureIds[idx1] != m_FeatureIds[idx2])
        {
          usize edgeIdx = m_CurrentEdge.fetch_add(1, std::memory_order_relaxed);

          // Vertical edge at grid position (x+1, y) to (x+1, y+1)
          float32 edgeX = m_OriginX + static_cast<float32>(x + 1) * m_SpacingX;
          float32 y0 = m_OriginY + static_cast<float32>(y) * m_SpacingY;
          float32 y1 = m_OriginY + static_cast<float32>(y + 1) * m_SpacingY;

          usize v0 = edgeIdx * 2;
          usize v1 = edgeIdx * 2 + 1;

          // Vertex 0: (edgeX, y0, originZ)
          m_Vertices[v0 * 3] = edgeX;
          m_Vertices[v0 * 3 + 1] = y0;
          m_Vertices[v0 * 3 + 2] = m_OriginZ;

          // Vertex 1: (edgeX, y1, originZ)
          m_Vertices[v1 * 3] = edgeX;
          m_Vertices[v1 * 3 + 1] = y1;
          m_Vertices[v1 * 3 + 2] = m_OriginZ;

          // Edge connectivity
          m_Edges[edgeIdx * 2] = v0;
          m_Edges[edgeIdx * 2 + 1] = v1;
        }
      }
    }
  }

private:
  const AbstractDataStore<T>& m_FeatureIds;
  usize m_DimX;
  usize m_DimY;
  float32 m_OriginX;
  float32 m_OriginY;
  float32 m_OriginZ;
  float32 m_SpacingX;
  float32 m_SpacingY;
  INodeGeometry0D::SharedVertexList& m_Vertices;
  INodeGeometry1D::SharedEdgeList& m_Edges;
  std::atomic<usize>& m_CurrentEdge;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @brief Creates vertices and edges for horizontal boundaries
 *
 * For each boundary found between vertically adjacent cells, this creates:
 * - Two vertices at the left and right of the cell interface
 * - One edge connecting those two vertices
 *
 * Uses atomic counter (m_CurrentEdge) to allocate unique edge indices.
 * Each edge gets 2 vertices stored consecutively (v0, v1) in the vertex array.
 */
template <typename T>
class PopulateHorizontalEdgesImpl
{
public:
  PopulateHorizontalEdgesImpl(const AbstractDataStore<T>& featureIds, usize dimX, usize dimY, float32 originX, float32 originY, float32 originZ, float32 spacingX, float32 spacingY,
                              INodeGeometry0D::SharedVertexList& vertices, INodeGeometry1D::SharedEdgeList& edges, std::atomic<usize>& currentEdge, const std::atomic_bool& shouldCancel)
  : m_FeatureIds(featureIds)
  , m_DimX(dimX)
  , m_DimY(dimY)
  , m_OriginX(originX)
  , m_OriginY(originY)
  , m_OriginZ(originZ)
  , m_SpacingX(spacingX)
  , m_SpacingY(spacingY)
  , m_Vertices(vertices)
  , m_Edges(edges)
  , m_CurrentEdge(currentEdge)
  , m_ShouldCancel(shouldCancel)
  {
  }

  void operator()(const Range& range) const
  {
    for(usize y = range.min(); y < range.max(); y++)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      for(usize x = 0; x < m_DimX; x++)
      {
        usize idx1 = y * m_DimX + x;
        usize idx2 = (y + 1) * m_DimX + x;
        if(m_FeatureIds[idx1] != m_FeatureIds[idx2])
        {
          usize edgeIdx = m_CurrentEdge.fetch_add(1, std::memory_order_relaxed);

          // Horizontal edge at grid position (x, y+1) to (x+1, y+1)
          float32 edgeY = m_OriginY + static_cast<float32>(y + 1) * m_SpacingY;
          float32 x0 = m_OriginX + static_cast<float32>(x) * m_SpacingX;
          float32 x1 = m_OriginX + static_cast<float32>(x + 1) * m_SpacingX;

          usize v0 = edgeIdx * 2;
          usize v1 = edgeIdx * 2 + 1;

          // Vertex 0: (x0, edgeY, originZ)
          m_Vertices[v0 * 3] = x0;
          m_Vertices[v0 * 3 + 1] = edgeY;
          m_Vertices[v0 * 3 + 2] = m_OriginZ;

          // Vertex 1: (x1, edgeY, originZ)
          m_Vertices[v1 * 3] = x1;
          m_Vertices[v1 * 3 + 1] = edgeY;
          m_Vertices[v1 * 3 + 2] = m_OriginZ;

          // Edge connectivity
          m_Edges[edgeIdx * 2] = v0;
          m_Edges[edgeIdx * 2 + 1] = v1;
        }
      }
    }
  }

private:
  const AbstractDataStore<T>& m_FeatureIds;
  usize m_DimX;
  usize m_DimY;
  float32 m_OriginX;
  float32 m_OriginY;
  float32 m_OriginZ;
  float32 m_SpacingX;
  float32 m_SpacingY;
  INodeGeometry0D::SharedVertexList& m_Vertices;
  INodeGeometry1D::SharedEdgeList& m_Edges;
  std::atomic<usize>& m_CurrentEdge;
  const std::atomic_bool& m_ShouldCancel;
};

// =============================================================================
// Z VALUE CHOICE CONSTANTS
// =============================================================================
// These constants must match ExtractFeatureBoundaries2DFilter::ZValueChoiceType
constexpr uint64 k_UseMinZValue = 0;
constexpr uint64 k_UseMaxZValue = 1;
constexpr uint64 k_UseCustomZValue = 2;

// =============================================================================
// MAIN ALGORITHM FUNCTOR
// =============================================================================
/**
 * @brief Functor that implements the core boundary extraction algorithm
 *
 * This functor is called via ExecuteDataFunction which handles type dispatching
 * based on the FeatureIds array data type. The algorithm:
 *
 * 1. Extracts geometry parameters (dimensions, origin, spacing)
 * 2. Determines Z value for all vertices based on user preference
 * 3. PASS 1 - COUNT: Counts all boundary edges
 * 4. Allocates vertex and edge arrays based on count
 * 5. PASS 2 - POPULATE: Creates vertices and edges
 * 6. Optionally adds outer boundary edges around the entire grid
 * 7. Eliminates duplicate vertices to create a clean edge network
 */
struct ExtractFeatureBoundariesFunctor
{
  template <typename T>
  Result<> operator()(const DataStructure& dataStructure, const DataPath& featureIdsPath, const ImageGeom& imageGeom, EdgeGeom& edgeGeom, const std::atomic_bool& shouldCancel, uint64 zValueChoice,
                      float32 customZValue, bool extractVirtualSampleEdges)
  {
    // =========================================================================
    // SETUP: Extract geometry parameters and feature IDs
    // =========================================================================
    const auto& featureIds = dataStructure.getDataRefAs<DataArray<T>>(featureIdsPath);
    const auto& featureIdsStore = featureIds.getDataStoreRef();

    SizeVec3 dims = imageGeom.getDimensions();
    FloatVec3 origin = imageGeom.getOrigin();
    FloatVec3 spacing = imageGeom.getSpacing();

    usize dimX = dims.getX();
    usize dimY = dims.getY();
    float32 originX = origin.getX();
    float32 originY = origin.getY();
    float32 spacingX = spacing.getX();
    float32 spacingY = spacing.getY();

    // =========================================================================
    // Z VALUE CALCULATION: Determine the Z coordinate for all generated vertices
    // =========================================================================
    float32 zValue = 0.0f;
    switch(zValueChoice)
    {
    case k_UseMinZValue:
      zValue = origin.getZ();
      break;
    case k_UseMaxZValue:
      // Max Z = origin + spacing * dims (for Z=1, this is origin.z + spacing.z)
      zValue = origin.getZ() + spacing.getZ() * static_cast<float32>(dims.getZ());
      break;
    case k_UseCustomZValue:
    default:
      zValue = customZValue;
      break;
    }

    // =========================================================================
    // PASS 1 - COUNT: Count all boundary edges to determine memory allocation
    // =========================================================================
    // This two-pass approach (count then populate) allows us to allocate the
    // exact amount of memory needed upfront, avoiding dynamic resizing.
    std::atomic<usize> verticalEdgeCount{0};
    std::atomic<usize> horizontalEdgeCount{0};

    // Count vertical edges (between horizontally adjacent cells)
    {
      CountVerticalEdgesImpl<T>(featureIdsStore, dimX, dimY, verticalEdgeCount, shouldCancel).operator()({0, dimY});
    }

    if(shouldCancel)
    {
      return {};
    }

    // Count horizontal edges (between vertically adjacent cells)
    // Note: We only check dimY-1 rows since we're comparing row y with row y+1
    if(dimY > 1)
    {
      CountHorizontalEdgesImpl<T>(featureIdsStore, dimX, dimY, horizontalEdgeCount, shouldCancel).operator()({0, dimY - 1});
    }

    if(shouldCancel)
    {
      return {};
    }

    // Count outer boundary edges if user requested the virtual sample border
    usize outerEdgeCount = 0;
    if(extractVirtualSampleEdges)
    {
      // Left and right boundaries: dimY edges each
      // Top and bottom boundaries: dimX edges each
      outerEdgeCount = 2 * dimX + 2 * dimY;
    }

    usize totalEdgeCount = verticalEdgeCount.load() + horizontalEdgeCount.load() + outerEdgeCount;

    // =========================================================================
    // EARLY EXIT: Handle case where no boundaries exist
    // =========================================================================
    if(totalEdgeCount == 0)
    {
      edgeGeom.resizeVertexList(0);
      edgeGeom.resizeEdgeList(0);
      edgeGeom.getVertexAttributeMatrix()->resizeTuples({0});
      edgeGeom.getEdgeAttributeMatrix()->resizeTuples({0});
      return {};
    }

    // =========================================================================
    // MEMORY ALLOCATION: Resize geometry arrays based on counted edges
    // =========================================================================
    // Initially allocate 2 vertices per edge (duplicates will be removed later).
    usize numVertices = totalEdgeCount * 2;
    edgeGeom.resizeVertexList(numVertices);
    edgeGeom.resizeEdgeList(totalEdgeCount);

    INodeGeometry0D::SharedVertexList& vertices = edgeGeom.getVerticesRef();
    INodeGeometry1D::SharedEdgeList& edges = edgeGeom.getEdgesRef();

    // =========================================================================
    // PASS 2 - POPULATE: Create vertices and edge connectivity
    // =========================================================================
    std::atomic<usize> currentEdge{0};

    // Populate vertical edges
    {
      PopulateVerticalEdgesImpl<T>(featureIdsStore, dimX, dimY, originX, originY, zValue, spacingX, spacingY, vertices, edges, currentEdge, shouldCancel).operator()({0, dimY});
    }

    if(shouldCancel)
    {
      return {};
    }

    // Populate horizontal edges
    if(dimY > 1)
    {
      PopulateHorizontalEdgesImpl<T>(featureIdsStore, dimX, dimY, originX, originY, zValue, spacingX, spacingY, vertices, edges, currentEdge, shouldCancel).operator()({0, dimY - 1});
    }

    if(shouldCancel)
    {
      return {};
    }

    // =========================================================================
    // OUTER BOUNDARY EDGES: Add edges around the perimeter of the grid
    // =========================================================================
    // When extractVirtualSampleEdges is true, we add edges around the entire
    // grid boundary. This creates a complete outline even if all cells have
    // the same feature ID.
    if(extractVirtualSampleEdges)
    {
      // Left boundary (x = 0): vertical edges along the left side
      for(usize y = 0; y < dimY; y++)
      {
        usize edgeIdx = currentEdge.fetch_add(1, std::memory_order_relaxed);
        float32 x = originX;
        float32 y0 = originY + static_cast<float32>(y) * spacingY;
        float32 y1 = originY + static_cast<float32>(y + 1) * spacingY;

        usize v0 = edgeIdx * 2;
        usize v1 = edgeIdx * 2 + 1;

        vertices[v0 * 3] = x;
        vertices[v0 * 3 + 1] = y0;
        vertices[v0 * 3 + 2] = zValue;

        vertices[v1 * 3] = x;
        vertices[v1 * 3 + 1] = y1;
        vertices[v1 * 3 + 2] = zValue;

        edges[edgeIdx * 2] = v0;
        edges[edgeIdx * 2 + 1] = v1;
      }

      // Right boundary (x = dimX): vertical edges from (dimX, y) to (dimX, y+1) for y = 0 to dimY-1
      for(usize y = 0; y < dimY; y++)
      {
        usize edgeIdx = currentEdge.fetch_add(1, std::memory_order_relaxed);
        float32 x = originX + static_cast<float32>(dimX) * spacingX;
        float32 y0 = originY + static_cast<float32>(y) * spacingY;
        float32 y1 = originY + static_cast<float32>(y + 1) * spacingY;

        usize v0 = edgeIdx * 2;
        usize v1 = edgeIdx * 2 + 1;

        vertices[v0 * 3] = x;
        vertices[v0 * 3 + 1] = y0;
        vertices[v0 * 3 + 2] = zValue;

        vertices[v1 * 3] = x;
        vertices[v1 * 3 + 1] = y1;
        vertices[v1 * 3 + 2] = zValue;

        edges[edgeIdx * 2] = v0;
        edges[edgeIdx * 2 + 1] = v1;
      }

      // Bottom boundary (y = 0): horizontal edges from (x, 0) to (x+1, 0) for x = 0 to dimX-1
      for(usize x = 0; x < dimX; x++)
      {
        usize edgeIdx = currentEdge.fetch_add(1, std::memory_order_relaxed);
        float32 y = originY;
        float32 x0 = originX + static_cast<float32>(x) * spacingX;
        float32 x1 = originX + static_cast<float32>(x + 1) * spacingX;

        usize v0 = edgeIdx * 2;
        usize v1 = edgeIdx * 2 + 1;

        vertices[v0 * 3] = x0;
        vertices[v0 * 3 + 1] = y;
        vertices[v0 * 3 + 2] = zValue;

        vertices[v1 * 3] = x1;
        vertices[v1 * 3 + 1] = y;
        vertices[v1 * 3 + 2] = zValue;

        edges[edgeIdx * 2] = v0;
        edges[edgeIdx * 2 + 1] = v1;
      }

      // Top boundary (y = dimY): horizontal edges from (x, dimY) to (x+1, dimY) for x = 0 to dimX-1
      for(usize x = 0; x < dimX; x++)
      {
        usize edgeIdx = currentEdge.fetch_add(1, std::memory_order_relaxed);
        float32 y = originY + static_cast<float32>(dimY) * spacingY;
        float32 x0 = originX + static_cast<float32>(x) * spacingX;
        float32 x1 = originX + static_cast<float32>(x + 1) * spacingX;

        usize v0 = edgeIdx * 2;
        usize v1 = edgeIdx * 2 + 1;

        vertices[v0 * 3] = x0;
        vertices[v0 * 3 + 1] = y;
        vertices[v0 * 3 + 2] = zValue;

        vertices[v1 * 3] = x1;
        vertices[v1 * 3 + 1] = y;
        vertices[v1 * 3 + 2] = zValue;

        edges[edgeIdx * 2] = v0;
        edges[edgeIdx * 2 + 1] = v1;
      }
    }

    // Update attribute matrices to match the actual counts
    edgeGeom.getVertexAttributeMatrix()->resizeTuples({numVertices});
    edgeGeom.getEdgeAttributeMatrix()->resizeTuples({totalEdgeCount});

    // =========================================================================
    // VERTEX DEDUPLICATION: Merge coincident vertices
    // =========================================================================
    // Each edge creates its own pair of vertices, resulting in many duplicate
    // vertices at shared corners. For example, where 4 cells meet, all 4
    // boundary edges would create a vertex at that corner. EliminateDuplicateNodes
    // merges these duplicates and updates the edge connectivity to reference
    // the unique vertices, creating a clean connected edge network suitable
    // for visualization and analysis.
    Result<> result = GeometryUtilities::EliminateDuplicateNodes<EdgeGeom>(edgeGeom);

    return result;
  }
};
} // namespace

// =============================================================================
// ALGORITHM CLASS IMPLEMENTATION
// =============================================================================
// The ExtractFeatureBoundaries2D class is the public interface called by the
// filter's executeImpl() method. It holds references to the DataStructure and
// input values, then delegates to the type-templated functor via ExecuteDataFunction.
// =============================================================================

// -----------------------------------------------------------------------------
ExtractFeatureBoundaries2D::ExtractFeatureBoundaries2D(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                       ExtractFeatureBoundaries2DInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ExtractFeatureBoundaries2D::~ExtractFeatureBoundaries2D() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ExtractFeatureBoundaries2D::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ExtractFeatureBoundaries2D::operator()()
{
  // Get references to the input/output geometries and feature IDs array
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometryPath);
  auto& edgeGeom = m_DataStructure.getDataRefAs<EdgeGeom>(m_InputValues->OutputEdgeGeometryPath);
  const auto& featureIdsArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->FeatureIdsArrayPath);

  // Get the data type to dispatch to the correct template instantiation
  DataType dataType = featureIdsArray.getDataType();

  m_MessageHandler(IFilter::Message::Type::Info, "Extracting feature boundaries...");

  // ExecuteDataFunction dispatches to the templated functor based on dataType.
  // This allows the algorithm to work with any integer type for FeatureIds
  // (int8, int16, int32, int64, uint8, uint16, uint32, uint64).
  return ExecuteDataFunction(ExtractFeatureBoundariesFunctor{}, dataType, m_DataStructure, m_InputValues->FeatureIdsArrayPath, imageGeom, edgeGeom, m_ShouldCancel, m_InputValues->ZValueChoice,
                             m_InputValues->CustomZValue, m_InputValues->ExtractVirtualSampleEdges);
}
