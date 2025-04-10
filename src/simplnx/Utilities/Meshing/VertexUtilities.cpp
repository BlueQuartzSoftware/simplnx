#include "VertexUtilities.hpp"

using namespace nx::core;

namespace
{
// Uses Hoare's method for speed
IGeometry::MeshIndexType ProcessSection(std::vector<IGeometry::MeshIndexType>& sorted, IGeometry::MeshIndexType begin, IGeometry::MeshIndexType end,
                                        const INodeGeometry2D::SharedVertexList::store_type& vertices, IGeometry::MeshIndexType offset)
{
  const INodeGeometry2D::SharedVertexList::value_type threshold = vertices[(sorted[begin] * 3) + offset];

  IGeometry::MeshIndexType front = begin;
  IGeometry::MeshIndexType back = end;

  const IGeometry::MeshIndexType max = sorted.size();
  while(true)
  {
    while(front < max && vertices[(sorted[front] * 3) + offset] < threshold)
    {
      front++;
    }

    while(back > 0 && vertices[(sorted[back] * 3) + offset] > threshold)
    {
      back--;
    }

    if(front >= back)
    {
      return back;
    }

    std::swap(sorted[front], sorted[back]);
  }
}

void QuickSortVertices(std::vector<IGeometry::MeshIndexType>& sorted, IGeometry::MeshIndexType begin, IGeometry::MeshIndexType end, const INodeGeometry2D::SharedVertexList::store_type& vertices,
                       IGeometry::MeshIndexType offset)
{
  if(begin >= end)
  {
    return;
  }

  IGeometry::MeshIndexType next = ProcessSection(sorted, begin, end, vertices, offset);

  // Recurse
  QuickSortVertices(sorted, begin, next, vertices, offset);
  QuickSortVertices(sorted, next + 1, end, vertices, offset);
}
} // namespace

MeshingUtilities::SortedVerticesList MeshingUtilities::OrderSharedVertices(const nx::core::INodeGeometry2D& geom)
{
  const BoundingBox3Df& bounds = geom.getBoundingBox();

  const Point3Df diff = bounds.sideLengths();

  // Inverse of to_underlying
  const AxialAlignment axis = static_cast<AxialAlignment>(std::distance(diff.begin(), std::max_element(diff.begin(), diff.end())));

  // Getting the verts list by ref here for the validation in the ref function
  const INodeGeometry2D::SharedVertexList::store_type& vertexListStore = geom.getVerticesRef().getDataStoreRef();

  return {.axis = axis, .ordering = std::move(OrderSharedVerticesAlongAxis(axis, vertexListStore))};
}

std::vector<IGeometry::MeshIndexType> MeshingUtilities::OrderSharedVerticesAlongAxis(nx::core::MeshingUtilities::AxialAlignment axis, const INodeGeometry2D::SharedVertexList::store_type& vertexList)
{
  std::vector<IGeometry::MeshIndexType> sorted(vertexList.getNumberOfTuples());
  std::iota(sorted.begin(), sorted.end(), 0);

  QuickSortVertices(sorted, 0, sorted.size() - 1, vertexList, to_underlying(axis));

  return sorted;
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
