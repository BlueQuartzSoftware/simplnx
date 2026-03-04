#include "VertexUtilities.hpp"

using namespace nx::core;

namespace
{
// Uses Hoare's method for speed
AbstractGeometry::MeshIndexType ProcessSection(std::vector<AbstractGeometry::MeshIndexType>& sorted, AbstractGeometry::MeshIndexType begin, AbstractGeometry::MeshIndexType end,
                                               const AbstractNodeGeometry0D::SharedVertexList::store_type& vertices, AbstractGeometry::MeshIndexType offset)
{
  const AbstractNodeGeometry0D::SharedVertexList::value_type threshold = vertices[(sorted[begin] * 3) + offset];

  AbstractGeometry::MeshIndexType front = begin;
  AbstractGeometry::MeshIndexType back = end;

  while(true)
  {
    while(vertices[(sorted[front] * 3) + offset] < threshold)
    {
      front++;
    }

    while(vertices[(sorted[back] * 3) + offset] > threshold)
    {
      back--;
    }

    if(front >= back)
    {
      return back;
    }

    std::swap(sorted[front], sorted[back]);
    front++;
    back--;
  }
}

void QuickSortVertices(std::vector<AbstractGeometry::MeshIndexType>& sorted, AbstractGeometry::MeshIndexType begin, AbstractGeometry::MeshIndexType end,
                       const AbstractNodeGeometry0D::SharedVertexList::store_type& vertices, AbstractGeometry::MeshIndexType offset, const std::atomic_bool& shouldCancel)
{
  if(begin >= end || shouldCancel)
  {
    return;
  }

  AbstractGeometry::MeshIndexType next = ProcessSection(sorted, begin, end, vertices, offset);

  // Recurse
  QuickSortVertices(sorted, begin, next, vertices, offset, shouldCancel);
  QuickSortVertices(sorted, next + 1, end, vertices, offset, shouldCancel);
}
} // namespace

MeshingUtilities::SortedVerticesList MeshingUtilities::OrderSharedVertices(const nx::core::AbstractNodeGeometry0D& geom, const std::atomic_bool& shouldCancel)
{
  const BoundingBox3Df& bounds = geom.getBoundingBox();

  const Point3Df diff = bounds.sideLengths();

  // Inverse of to_underlying
  const AxialAlignment axis = static_cast<AxialAlignment>(std::distance(diff.begin(), std::max_element(diff.begin(), diff.end())));

  // Getting the verts list by ref here for the validation in the ref function
  const AbstractNodeGeometry0D::SharedVertexList::store_type& vertexListStore = geom.getVerticesRef().getDataStoreRef();

  return {.axis = axis, .ordering = std::move(OrderSharedVerticesAlongAxis(axis, vertexListStore, shouldCancel))};
}

std::vector<AbstractGeometry::MeshIndexType> MeshingUtilities::OrderSharedVerticesAlongAxis(nx::core::MeshingUtilities::AxialAlignment axis,
                                                                                            const AbstractNodeGeometry0D::SharedVertexList::store_type& vertexList,
                                                                                            const std::atomic_bool& shouldCancel)
{
  std::vector<AbstractGeometry::MeshIndexType> sorted(vertexList.getNumberOfTuples());
  std::iota(sorted.begin(), sorted.end(), 0);

  QuickSortVertices(sorted, 0, sorted.size() - 1, vertexList, to_underlying(axis), shouldCancel);

  return sorted;
}

bool MeshingUtilities::HasDuplicateVertices(const AbstractGeometry::SharedVertexList::store_type& verts, const nx::core::MeshingUtilities::SortedVerticesList& sortedVertices)
{
  using VertT = AbstractNodeGeometry0D::SharedVertexList::value_type;

  // Leverage ordering assumptions to speed up duplicate checks
  std::array<AbstractGeometry::MeshIndexType, 3> offset;

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
    const AbstractGeometry::MeshIndexType prevIndex = sortedVertices.ordering[i - 1] * 3;
    const AbstractGeometry::MeshIndexType currentIndex = sortedVertices.ordering[i] * 3;
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
