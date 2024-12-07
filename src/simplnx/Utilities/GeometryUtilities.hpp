#pragma once

#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry3D.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

namespace nx::core::GeometryUtilities
{
/**
 * @brief The FindUniqueIdsImpl class implements a threaded algorithm that determines the set of
 * unique vertices in a given geometry
 */
class SIMPLNX_EXPORT FindUniqueIdsImpl
{
public:
  using VertexStore = nx::core::AbstractDataStore<nx::core::IGeometry::SharedVertexList::value_type>;
  FindUniqueIdsImpl(VertexStore& vertexStore, const std::vector<std::vector<size_t>>& nodesInBin, nx::core::Int64DataStore& uniqueIds);

  void convert(size_t start, size_t end) const;
  void operator()(const Range& range) const;

private:
  const VertexStore& m_VertexStore;
  const std::vector<std::vector<size_t>>& m_NodesInBin;
  nx::core::Int64DataStore& m_UniqueIds;
};

/**
 * @brief Calculates the X,Y,Z partition length for a given geometry if the geometry were partitioned into equal numberOfPartitionsPerAxis partitions.
 * @param geometry The geometry to be partitioned
 * @param numberOfPartitionsPerAxis The number of partitions in each axis
 */
SIMPLNX_EXPORT Result<FloatVec3> CalculatePartitionLengthsByPartitionCount(const INodeGeometry0D& geometry, const SizeVec3& numberOfPartitionsPerAxis);

/**
 * @brief Calculates the X,Y,Z partition length for a given Image geometry if the geometry were partitioned into equal numberOfPartitionsPerAxis partitions.
 * @param geometry The geometry to be partitioned
 * @param numberOfPartitionsPerAxis The number of partitions in each axis
 */
SIMPLNX_EXPORT Result<FloatVec3> CalculatePartitionLengthsByPartitionCount(const ImageGeom& geometry, const SizeVec3& numberOfPartitionsPerAxis);

/**
 * @brief Calculates the X,Y,Z partition length for a given RectGrid geometry if the geometry were partitioned into equal numberOfPartitionsPerAxis partitions.
 * @param geometry The geometry to be partitioned
 * @param numberOfPartitionsPerAxis The number of partitions in each axis
 */
SIMPLNX_EXPORT Result<FloatVec3> CalculatePartitionLengthsByPartitionCount(const RectGridGeom& geometry, const SizeVec3& numberOfPartitionsPerAxis);

/**
 * @brief Calculates the X,Y,Z partition scheme origin for a given node-based geometry using the geometry's bounding box.
 * @param geometry The geometry whose bounding box origin will be calculated
 */
SIMPLNX_EXPORT Result<FloatVec3> CalculateNodeBasedPartitionSchemeOrigin(const INodeGeometry0D& geometry);

/**
 * @brief Calculates the X,Y,Z partition length if the given bounding box were partitioned into equal numberOfPartitionsPerAxis partitions.
 * @param boundingBox The bounding box
 * @param numberOfPartitionsPerAxis The number of partitions in each axis
 */
SIMPLNX_EXPORT Result<FloatVec3> CalculatePartitionLengthsOfBoundingBox(const BoundingBox3Df& boundingBox, const SizeVec3& numberOfPartitionsPerAxis);

/**
 * @brief This function will find the volume of a tetrahedron
 * @tparam T
 * @param verts
 * @param v4
 * @return
 */
template <typename T>
T ComputeTetrahedronVolume(const std::array<nx::core::Point3Df, 3>& verts, const std::array<T, 3>& v4)
{
  using Point3DType = Point3D<T>;

  // Create the 4 Vertices of the tetrahedron
  const Point3DType& a = verts[0];
  const Point3DType& b = verts[1];
  const Point3DType& c = verts[2];
  const Point3DType& d = v4;

  // The volume of the tetrahedron can be calculated through V= 1/6 |M| (where M is the volume matrix)
  // v1 = b-a, v2 = c-a, v3 = d-a
  //      | v1x     v2x    v3x  |
  //  M = | v1y     v2y    v3y  |
  //      | v1z     v2z    v3z  |
  // clang-format off
    std::array<T, 9> volumeMatrix = {b[0] - a[0], c[0] - a[0], d[0] - a[0],
                                     b[1] - a[1], c[1] - a[1], d[1] - a[1],
                                     b[2] - a[2], c[2] - a[2], d[2] - a[2]};

    // det(M) = v1x(v2y*v3z - v2z*v3y) - v1y(v2x*v3z - v3x*v2z)  +  v1z(v2x*v3y - v3x*v2y)
    T determinant = (volumeMatrix[0] * (volumeMatrix[4] * volumeMatrix[8] - volumeMatrix[5] * volumeMatrix[7])) -
                    (volumeMatrix[3] * (volumeMatrix[1] * volumeMatrix[8] - volumeMatrix[2] * volumeMatrix[7])) +
                    (volumeMatrix[6] * (volumeMatrix[1] * volumeMatrix[5] - volumeMatrix[2] * volumeMatrix[4]));
  // clang-format on

  return std::abs((determinant / 6.0f)); // The final volume of the tetrahedron
}

/**
 * @brief Removes duplicate nodes to ensure the vertex list is unique
 * @param geom The geometry to eliminate the duplicate nodes from.  This MUST be a node-based geometry.
 */
template <class GeometryType = INodeGeometry1D, class = std::enable_if_t<std::is_base_of<INodeGeometry1D, GeometryType>::value>>
Result<> EliminateDuplicateNodes(GeometryType& geom, std::optional<float32> scaleFactor = std::nullopt)
{
  usize numXBins = 100;
  usize numYBins = 100;
  usize numZBins = 100;

  using SharedVertList = AbstractDataStore<IGeometry::SharedVertexList::value_type>;

  SharedVertList& vertices = geom.getVertices()->getDataStoreRef();

  INodeGeometry1D::MeshIndexArrayType* cells = nullptr;
  if constexpr(std::is_base_of<INodeGeometry3D, GeometryType>::value)
  {
    cells = geom.getPolyhedra();
  }
  else if constexpr(std::is_base_of<INodeGeometry2D, GeometryType>::value)
  {
    cells = geom.getFaces();
  }
  else if constexpr(std::is_base_of<INodeGeometry1D, GeometryType>::value)
  {
    cells = geom.getEdges();
  }

  if(nullptr == cells)
  {
    return MakeErrorResult(-56800, "EliminateDuplicateNodes Error: Geometry Type was not 1D, 2D or 3D? Did you pass in a vertex geometry?");
  }
  AbstractDataStore<INodeGeometry1D::MeshIndexArrayType::value_type>& cellsRef = cells->getDataStoreRef();

  IGeometry::MeshIndexType nNodesAll = geom.getNumberOfVertices();
  size_t nNodes = 0;
  if(nNodesAll > 0)
  {
    nNodes = static_cast<size_t>(nNodesAll);
  }

  auto boundingBox = geom.getBoundingBox();
  auto minPoint = boundingBox.getMinPoint();
  auto maxPoint = boundingBox.getMaxPoint();
  float32 stepX = (maxPoint.getX() - minPoint.getX()) / numXBins;
  float32 stepY = (maxPoint.getY() - minPoint.getY()) / numYBins;
  float32 stepZ = (maxPoint.getZ() - minPoint.getZ()) / numZBins;

  std::vector<std::vector<usize>> nodesInBin(numXBins * numYBins * numZBins);

  // determine (xyz) bin each node falls in - used to speed up node comparison
  usize xBin = 0, yBin = 0, zBin = 0;
  for(size_t i = 0; i < nNodes; i++)
  {
    if(stepX != 0.0)
    {
      xBin = static_cast<usize>((vertices[i * 3] - minPoint.getX()) / stepX);
    }
    if(stepY != 0.0)
    {
      yBin = static_cast<usize>((vertices[i * 3 + 1] - minPoint.getY()) / stepY);
    }
    if(stepZ != 0)
    {
      zBin = static_cast<usize>((vertices[i * 3 + 2] - minPoint.getZ()) / stepZ);
    }
    if(xBin == numXBins)
    {
      xBin = numXBins - 1;
    }
    if(yBin == numYBins)
    {
      yBin = numYBins - 1;
    }
    if(zBin == numZBins)
    {
      zBin = numZBins - 1;
    }
    usize bin = (zBin * numYBins * numXBins) + (yBin * numXBins) + xBin;
    nodesInBin[bin].push_back(i);
  }

  // Create array to hold unique node numbers
  Int64DataStore uniqueIds(IDataStore::ShapeType{nNodes}, IDataStore::ShapeType{1}, {});
  for(IGeometry::MeshIndexType i = 0; i < nNodesAll; i++)
  {
    uniqueIds[i] = static_cast<int64>(i);
  }

  // Parallel algorithm to find duplicate nodes
  ParallelDataAlgorithm dataAlg;
  dataAlg.setParallelizationEnabled(true);
  dataAlg.setRange(0ULL, static_cast<usize>(numXBins * numYBins * numZBins));
  dataAlg.execute(GeometryUtilities::FindUniqueIdsImpl(vertices, nodesInBin, uniqueIds));

  // renumber the unique nodes
  int64 uniqueCount = 0;
  for(usize i = 0; i < nNodes; i++)
  {
    if(uniqueIds[i] == static_cast<int64>(i))
    {
      uniqueIds[i] = uniqueCount;
      uniqueCount++;
    }
    else
    {
      uniqueIds[i] = uniqueIds[uniqueIds[i]];
    }
  }

  float32 scaleFactorValue = 1.0F;
  if(scaleFactor.has_value())
  {
    scaleFactorValue = scaleFactor.value();
  }

  // Move nodes to uniqueIds and then resize nodes array and apply optional scaling
  for(size_t i = 0; i < nNodes; i++)
  {
    vertices[uniqueIds[i] * 3] = vertices[i * 3] * scaleFactorValue;
    vertices[uniqueIds[i] * 3 + 1] = vertices[i * 3 + 1] * scaleFactorValue;
    vertices[uniqueIds[i] * 3 + 2] = vertices[i * 3 + 2] * scaleFactorValue;
  }
  geom.resizeVertexList(uniqueCount);

  // Update the triangle nodes to reflect the unique ids
  IGeometry::MeshIndexType nCells;
  usize nVerticesPerCell = 0;
  if constexpr(std::is_base_of<INodeGeometry3D, GeometryType>::value)
  {
    nCells = geom.getNumberOfPolyhedra();
    nVerticesPerCell = geom.getNumberOfVerticesPerCell();
  }
  else if constexpr(std::is_base_of<INodeGeometry2D, GeometryType>::value)
  {
    nCells = geom.getNumberOfFaces();
    nVerticesPerCell = geom.getNumberOfVerticesPerFace();
  }
  else if constexpr(std::is_base_of<INodeGeometry1D, GeometryType>::value)
  {
    nCells = geom.getNumberOfEdges();
    nVerticesPerCell = geom.getNumberOfVerticesPerEdge();
  }

  if(nVerticesPerCell == 0)
  {
    return MakeErrorResult(-56801, "EliminateDuplicateNodes Error: nVerticesPerCell = 0? Did you pass in a vertex geometry?");
  }

  for(size_t i = 0; i < static_cast<size_t>(nCells); i++)
  {
    for(usize j = 0; j < nVerticesPerCell; j++)
    {
      auto node = static_cast<int64>(cellsRef[i * nVerticesPerCell + j]);
      cellsRef[i * nVerticesPerCell + j] = uniqueIds[node];
    }
  }

  if constexpr(std::is_base_of<INodeGeometry3D, GeometryType>::value)
  {
    geom.getPolyhedraAttributeMatrix()->resizeTuples({geom.getNumberOfPolyhedra()});
  }
  else if constexpr(std::is_base_of<INodeGeometry2D, GeometryType>::value)
  {
    geom.getFaceAttributeMatrix()->resizeTuples({geom.getNumberOfFaces()});
  }
  else if constexpr(std::is_base_of<INodeGeometry1D, GeometryType>::value)
  {
    geom.getEdgeAttributeMatrix()->resizeTuples({geom.getNumberOfEdges()});
  }

  geom.getVertexAttributeMatrix()->resizeTuples({geom.getNumberOfVertices()});

  return {};
}

/**
 * @brief This will compute, in parallel, the area of each triangle in a triangle geometry
 * @param triangleGeom
 * @param faceAreas
 * @param shouldCancel
 * @return
 */
SIMPLNX_EXPORT Result<> ComputeTriangleAreas(const nx::core::TriangleGeom* triangleGeom, Float64AbstractDataStore& faceAreas, const std::atomic_bool& shouldCancel);

/**
 * @brief This will compute, in parallel, the normal of each triangle in a triangle geometry
 * @param triangleGeom
 * @param normals
 * @param shouldCancel
 * @return
 */
SIMPLNX_EXPORT Result<> ComputeTriangleNormals(const nx::core::TriangleGeom* triangleGeom, Float64AbstractDataStore& normals, const std::atomic_bool& shouldCancel);

SIMPLNX_EXPORT usize determineBoundsAndNumSlices(float32& minDim, float32& maxDim, usize numTris, AbstractDataStore<INodeGeometry2D::SharedFaceList::value_type>& tris,
                                                 AbstractDataStore<INodeGeometry0D::SharedVertexList::value_type>& triVerts, uint64 sliceRange, float32 zStart, float32 zEnd, float32 sliceResolution);

/**
 * @brief This is the information that is generated by the function and needs to be returned.
 */
struct SliceTriangleReturnType
{
  std::vector<float32> SliceVerts;
  std::vector<int32> SliceIds;
  std::vector<int32> RegionIds;
  usize NumberOfSlices;
};

/**
 * @brief This function will generate the vertices, slice ids and optionally RegionIds when slicing a triangle geometry
 *
 * The function will return the vertices where each pair of vertices represent an edge that
 * can be put into an Edge Geometry. The Vertices are packed into the std::vector<float> as XYZ coordinates
 * so the number of vertices is the size / 3 and the number of edges is size / 6. For
 * each edge there is a "slice id" that represents the integer slice index. This can be
 * used to pull out edges for a specific slice that corresponds to a specific Z
 * height. The total number of slices is also returned from the function.
 *
 * @param triangleGeom
 * @param shouldCancel
 * @param sliceRange This is either '0' or '1' where 0=Slice the entire Z Range of the geometry and 1=Slice a user defined range
 * @param zStart The user defined starting z value to start slicing
 * @param zEnd The user defined ending z value to end slicing
 * @param sliceSpacing The physical distance between slices.
 * @param triRegionIdPtr DataArray that holds the Triangle Region Ids
 * @return
 */
SIMPLNX_EXPORT SliceTriangleReturnType SliceTriangleGeometry(nx::core::TriangleGeom& triangleGeom, const std::atomic_bool& shouldCancel, uint64 sliceRange, float32 zStart, float32 zEnd,
                                                             float32 sliceSpacing, AbstractDataStore<int32>* triRegionIdPtr);

} // namespace nx::core::GeometryUtilities
