#include "VertexUtilities.hpp"

using namespace nx::core;

namespace
{
template <bool UseX, bool UseY, bool UseZ>
struct OrderAxis
{
  static constexpr bool UsingX = UseX;
  static constexpr bool UsingY = UseY;
  static constexpr bool UsingZ = UseZ;
};

void QuickSortVertices(std::vector<IGeometry::MeshIndexType>& sorted, IGeometry::MeshIndexType begin, IGeometry::MeshIndexType end, const INodeGeometry2D::SharedVertexList::store_type& vertices,
                       IGeometry::MeshIndexType offset)
{
  if(begin < end)
  {
    using T = INodeGeometry2D::SharedVertexList::value_type;

    const T threshold = vertices[(sorted[end] * 3) + offset];

    IGeometry::MeshIndexType next = begin;

    for(IGeometry::MeshIndexType prev = begin - 1; prev < end; prev++)
    {
      if(vertices[(sorted[prev] * 3) + offset] < threshold)
      {
        std::swap(sorted[next], sorted[prev]);
        next++;
      }
    }
    std::swap(sorted[next + 1], sorted[end]);

    // Recurse
    QuickSortVertices(sorted, begin, next, vertices, offset);
    QuickSortVertices(sorted, next + 1, end, vertices, offset);
  }
}

template <typename OrderAxis>
std::vector<IGeometry::MeshIndexType> SortVertices(const INodeGeometry2D::SharedVertexList::store_type& vertexList)
{
  std::vector<IGeometry::MeshIndexType> sorted(vertexList.getNumberOfTuples());
  std::iota(sorted.begin(), sorted.end(), 1);

  if constexpr(OrderAxis::UsingX)
  {
    QuickSortVertices(sorted, 1, sorted.size() - 1, vertexList, 0);
  }
  if constexpr(OrderAxis::UsingY)
  {
    QuickSortVertices(sorted, 1, sorted.size() - 1, vertexList, 1);
  }
  if constexpr(OrderAxis::UsingZ)
  {
    QuickSortVertices(sorted, 1, sorted.size() - 1, vertexList, 2);
  }

  return sorted;
}

} // namespace

MeshingUtilities::SortedVerticesList MeshingUtilities::OrderSharedVertices(const nx::core::INodeGeometry2D& geom)
{
  const BoundingBox3Df& bounds = geom.getBoundingBox();

  const Point3Df diff = bounds.sideLengths();

  // Inverse of to_underlying
  const AxialAlignment axis = static_cast<AxialAlignment>(std::distance(diff.begin(), std::max_element(diff.begin(), diff.end())));

  return {.axis = axis, .ordering = std::move(OrderSharedVerticesAlongAxis(axis, geom))};
}

std::vector<IGeometry::MeshIndexType> MeshingUtilities::OrderSharedVerticesAlongAxis(nx::core::MeshingUtilities::AxialAlignment axis, const nx::core::INodeGeometry2D& geom)
{
  const INodeGeometry2D::SharedVertexList::store_type& vertexListStore = geom.getVertices()->getDataStoreRef();
  switch(axis)
  {
  case AxialAlignment::X:
    return SortVertices<OrderAxis<true, false, false>>(vertexListStore);
  case AxialAlignment::Y:
    return SortVertices<OrderAxis<false, true, false>>(vertexListStore);
  case AxialAlignment::Z:
    return SortVertices<OrderAxis<false, false, true>>(vertexListStore);
  }

  return {};
}

bool MeshingUtilities::HasDuplicateVertices(const IGeometry::SharedVertexList::store_type& verts, const nx::core::MeshingUtilities::SortedVerticesList& sortedVertices)
{
  using VertT = INodeGeometry2D::SharedVertexList::value_type;

  // Leverage ordering assumptions to speed up duplicate checks
  std::array<IGeometry::MeshIndexType, 3> offset;

  switch(sortedVertices.axis)
  {
  case MeshingUtilities::AxialAlignment::X: {
    offset = {0, 1, 2};
    break;
  }
  case MeshingUtilities::AxialAlignment::Y: {
    offset = {1, 2, 0};
    break;
  }
  case MeshingUtilities::AxialAlignment::Z: {
    offset = {2, 1, 0};
    break;
  }
  }

  for(usize i = 1; i < sortedVertices.ordering.size(); i++)
  {
    const IGeometry::MeshIndexType prevIndex = sortedVertices.ordering[i - 1] * 3;
    const IGeometry::MeshIndexType currentIndex = sortedVertices.ordering[i] * 3;
    if(std::numeric_limits<VertT>::epsilon() < std::fabs(verts[prevIndex + offset[0]] - verts[currentIndex + offset[0]]))
    {
      // value on first axis is different; proceed to next iteration
      continue;
    }
    if(std::numeric_limits<VertT>::epsilon() < std::fabs(verts[prevIndex + offset[1]] - verts[currentIndex + offset[1]]))
    {
      // value on second axis is different; proceed to next iteration
      continue;
    }
    if(std::numeric_limits<VertT>::epsilon() < std::fabs(verts[prevIndex + offset[2]] - verts[currentIndex + offset[2]]))
    {
      // value on third axis is different; proceed to next iteration
      continue;
    }

    // Duplicate found break
    return true;
  }

  return false;
}

Result<> MeshingUtilities::RemoveDuplicateVertices(nx::core::INodeGeometry2D& geom, const nx::core::MeshingUtilities::SortedVerticesList& sortedVertices)
{
  IGeometry::SharedVertexList::store_type& verts = geom.getVertices()->getDataStoreRef();

  using VertT = INodeGeometry2D::SharedVertexList::value_type;

  // Leverage ordering assumptions to speed up duplicate checks
  std::array<IGeometry::MeshIndexType, 3> offset;

  switch(sortedVertices.axis)
  {
  case MeshingUtilities::AxialAlignment::X: {
    offset = {0, 1, 2};
    break;
  }
  case MeshingUtilities::AxialAlignment::Y: {
    offset = {1, 2, 0};
    break;
  }
  case MeshingUtilities::AxialAlignment::Z: {
    offset = {2, 1, 0};
    break;
  }
  }

  // mapping objects
  std::vector<IGeometry::MeshIndexType> oldVertexToNewIndexMapping(sortedVertices.ordering.size(), 0);
  std::vector<VertT> newVertices(verts.size());
  // Base cases
  newVertices[0] = verts[(sortedVertices.ordering[0] * 3) + 0];
  newVertices[1] = verts[(sortedVertices.ordering[0] * 3) + 1];
  newVertices[2] = verts[(sortedVertices.ordering[0] * 3) + 2];

  oldVertexToNewIndexMapping[sortedVertices.ordering[0]] = 0;

  IGeometry::MeshIndexType indexOffset = 0;
  bool activeSet = false;
  IGeometry::MeshIndexType dupIndex = std::numeric_limits<IGeometry::MeshIndexType>::max();
  for(usize i = 1; i < sortedVertices.ordering.size(); i++)
  {
    const IGeometry::MeshIndexType newIndex = i - indexOffset;
    const IGeometry::MeshIndexType prevIndex = sortedVertices.ordering[i - 1] * 3;
    const IGeometry::MeshIndexType currentIndex = sortedVertices.ordering[i] * 3;

    if(std::numeric_limits<VertT>::epsilon() < std::fabs(verts[prevIndex + offset[0]] - verts[currentIndex + offset[0]]))
    {
      // value on first axis is different; proceed to next iteration
      if(activeSet)
      {
        activeSet = false;
      }
      oldVertexToNewIndexMapping[sortedVertices.ordering[i]] = newIndex;
      newVertices[(newIndex * 3) + 0] = verts[currentIndex + 0];
      newVertices[(newIndex * 3) + 1] = verts[currentIndex + 1];
      newVertices[(newIndex * 3) + 2] = verts[currentIndex + 2];
      continue;
    }
    if(std::numeric_limits<VertT>::epsilon() < std::fabs(verts[prevIndex + offset[1]] - verts[currentIndex + offset[1]]))
    {
      // value on second axis is different; proceed to next iteration
      if(activeSet)
      {
        activeSet = false;
      }
      oldVertexToNewIndexMapping[sortedVertices.ordering[i]] = newIndex;
      newVertices[(newIndex * 3) + 0] = verts[currentIndex + 0];
      newVertices[(newIndex * 3) + 1] = verts[currentIndex + 1];
      newVertices[(newIndex * 3) + 2] = verts[currentIndex + 2];
      continue;
    }
    if(std::numeric_limits<VertT>::epsilon() < std::fabs(verts[prevIndex + offset[2]] - verts[currentIndex + offset[2]]))
    {
      // value on third axis is different; proceed to next iteration
      if(activeSet)
      {
        activeSet = false;
      }
      oldVertexToNewIndexMapping[sortedVertices.ordering[i]] = newIndex;
      newVertices[(newIndex * 3) + 0] = verts[currentIndex + 0];
      newVertices[(newIndex * 3) + 1] = verts[currentIndex + 1];
      newVertices[(newIndex * 3) + 2] = verts[currentIndex + 2];
      continue;
    }

    if(!activeSet)
    {
      // starting new set cache index to map to previous
      dupIndex = newIndex - 1;
    }

    // Set as active
    activeSet = true;
    indexOffset++;

    // Duplicate found
    oldVertexToNewIndexMapping[sortedVertices.ordering[i]] = dupIndex;
  }

  // Walk all the triangles and remap
  IGeometry::SharedFaceList::store_type& triangles = geom.getFaces()->getDataStoreRef();
  const usize triangleComp = triangles.getNumberOfComponents();
  for(usize i = 0; i < triangles.size(); i ++)
  {
    const IGeometry::SharedFaceList::value_type old = triangles[i] / triangleComp;
    triangles[i] = oldVertexToNewIndexMapping[old];
  }

  // Update all the vertices
  for(usize i = 0; i < verts.size(); i++)
  {
    verts[i] = newVertices[i];
  }

  // assume truncation
  verts.resizeTuples(IDataStore::ShapeType{newVertices.size() - indexOffset});

  return {};
}

Result<> MeshingUtilities::SortVertices(nx::core::INodeGeometry2D& geom, const nx::core::MeshingUtilities::SortedVerticesList& sortedVertices)
{
  IGeometry::SharedVertexList::store_type& verts = geom.getVertices()->getDataStoreRef();

  using VertT = INodeGeometry2D::SharedVertexList::value_type;
  std::vector<VertT> newVertices(verts.size());
  std::vector<IGeometry::MeshIndexType> inverseSortMapping(sortedVertices.ordering.size(), 0);
  for(usize i = 0; i < sortedVertices.ordering.size(); i++)
  {
    const IGeometry::MeshIndexType currentIndex = sortedVertices.ordering[i] * 3;
    newVertices[(i * 3) + 0] = verts[currentIndex + 0];
    newVertices[(i * 3) + 1] = verts[currentIndex + 1];
    newVertices[(i * 3) + 2] = verts[currentIndex + 2];

    inverseSortMapping[sortedVertices.ordering[i]] = i;
  }

  // Walk all the triangles and remap
  IGeometry::SharedFaceList::store_type& triangles = geom.getFaces()->getDataStoreRef();
  const usize triangleComp = triangles.getNumberOfComponents();
  for(usize i = 0; i < triangles.size(); i ++)
  {
    const IGeometry::SharedFaceList::value_type old = triangles[i] / triangleComp;
    triangles[i] = inverseSortMapping[old];
  }

  // Update all the vertices
  for(usize i = 0; i < verts.size(); i++)
  {
    verts[i] = newVertices[i];
  }

  return {};
}
