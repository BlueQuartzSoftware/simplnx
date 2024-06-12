#pragma once

#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry3D.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
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
  FindUniqueIdsImpl(nx::core::IGeometry::SharedVertexList& vertex, const std::vector<std::vector<size_t>>& nodesInBin, nx::core::Int64DataStore& uniqueIds);

  void convert(size_t start, size_t end) const;
  void operator()(const Range& range) const;

private:
  const IGeometry::SharedVertexList& m_Vertex;
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

///**
//   * @brief Constructs a new BoundingBox3D defined by the array of position values.
//   * The format is min X, min Y, min Z, max X, max Y, max Z.
//   * @param arr
// */
// template <class PointT = PointType, class = std::enable_if_t<std::is_same<PointT, Point3D<ValueType>>::value>>
// explicit BoundingBox(nonstd::span<const ValueType, 6> arr)
//: m_Lower(Point3D<ValueType>(arr[0], arr[1], arr[2]))
//, m_Upper(Point3D<ValueType>(arr[3], arr[4], arr[5]))
//{
//}

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

  using SharedFaceList = IGeometry::MeshIndexArrayType;
  using SharedVertList = IGeometry::SharedVertexList;

  SharedVertList& vertices = *(geom.getVertices());

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
  INodeGeometry1D::MeshIndexArrayType& cellsRef = *(cells);

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
    if(zBin != 0)
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

  // Move nodes to unique Id and then resize nodes array and apply optional scaling
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
} // namespace nx::core::GeometryUtilities
