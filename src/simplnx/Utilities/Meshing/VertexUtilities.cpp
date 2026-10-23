#include "VertexUtilities.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/IO/Generic/ITemporaryRecordStore.hpp"

#include <algorithm>
#include <atomic>
#include <limits>
#include <numeric>
#include <span>
#include <utility>

#include <fmt/format.h>
#include <nonstd/span.hpp>

using namespace nx::core;

namespace
{
Result<> readCoordinatePage(const Float32AbstractDataStore& vertices, usize first, nonstd::span<float32> page)
{
  return vertices.copyIntoBuffer(first * 3, page);
}

Result<> readCoordinatePage(const ITemporaryRecordStore& vertices, usize first, nonstd::span<float32> page)
{
  const std::atomic_bool active{false};
  const usize count = page.size() / 3;
  const auto bytes = std::as_writable_bytes(std::span<float32>(page.data(), page.size()));
  auto read = vertices.read(first, count, {bytes.data(), bytes.size()}, active);
  if(read.invalid())
  {
    return ConvertResult(std::move(read));
  }
  if(read.value() != count)
  {
    return MergeResults(ConvertResult(std::move(read)), MakeErrorResult(-65901, "Temporary vertex coordinates returned fewer XYZ records than requested."));
  }
  return ConvertResult(std::move(read));
}

template <typename Store>
Result<> gatherCoordinatePages(const Store& vertices, usize vertexCount, nonstd::span<const IGeometry::MeshIndexType> indexes, nonstd::span<usize> order, nonstd::span<float32> gathered,
                               nonstd::span<float32> page)
{
  constexpr usize maximum = std::numeric_limits<usize>::max();
  if(vertexCount > maximum / 3 || indexes.size() > maximum / 3 || order.size() < indexes.size() || gathered.size() < indexes.size() * 3 || page.size() % 3 != 0 || (!indexes.empty() && page.empty()))
  {
    return MakeErrorResult(-65902, "Vertex-coordinate gather received invalid buffer sizes. Supply one order entry and three coordinates per vertex, and a whole-XYZ read page.");
  }
  for(usize request = 0; request < indexes.size(); ++request)
  {
    if(indexes[request] >= vertexCount)
    {
      return MakeErrorResult(-65903, fmt::format("Vertex-coordinate request {} references vertex {}, but the vertex count is {}.", request, indexes[request], vertexCount));
    }
    order[request] = request;
  }
  if(indexes.empty())
  {
    return {};
  }
  const auto requests = order.first(indexes.size());
  std::ranges::sort(requests, [&indexes](usize left, usize right) { return indexes[left] < indexes[right]; });
  const usize capacity = page.size() / 3;
  usize loadedPage = maximum;
  Result<> result;
  for(usize request = 0; request < indexes.size(); ++request)
  {
    const usize ordinal = order[request];
    const usize vertex = static_cast<usize>(indexes[ordinal]);
    const usize first = (vertex / capacity) * capacity;
    if(first != loadedPage)
    {
      const usize count = std::min(capacity, vertexCount - first);
      result = MergeResults(std::move(result), readCoordinatePage(vertices, first, page.first(count * 3)));
      if(result.invalid())
      {
        return MergeResults(std::move(result), MakeErrorResult(-65904, fmt::format("Error reading vertex-coordinate page at vertex {} ({} vertices).", first, count)));
      }
      loadedPage = first;
    }
    const auto coordinates = page.subspan((vertex - first) * 3, 3);
    const auto destination = gathered.subspan(ordinal * 3, 3);
    std::ranges::copy(coordinates, destination.begin());
  }
  return result;
}

// Uses Hoare's method for speed
IGeometry::MeshIndexType ProcessSection(std::vector<IGeometry::MeshIndexType>& sorted, IGeometry::MeshIndexType begin, IGeometry::MeshIndexType end,
                                        const INodeGeometry0D::SharedVertexList::store_type& vertices, IGeometry::MeshIndexType offset)
{
  const INodeGeometry0D::SharedVertexList::value_type threshold = vertices[(sorted[begin] * 3) + offset];

  IGeometry::MeshIndexType front = begin;
  IGeometry::MeshIndexType back = end;

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

void QuickSortVertices(std::vector<IGeometry::MeshIndexType>& sorted, IGeometry::MeshIndexType begin, IGeometry::MeshIndexType end, const INodeGeometry0D::SharedVertexList::store_type& vertices,
                       IGeometry::MeshIndexType offset, const std::atomic_bool& shouldCancel)
{
  if(begin >= end || shouldCancel)
  {
    return;
  }

  IGeometry::MeshIndexType next = ProcessSection(sorted, begin, end, vertices, offset);

  // Recurse
  QuickSortVertices(sorted, begin, next, vertices, offset, shouldCancel);
  QuickSortVertices(sorted, next + 1, end, vertices, offset, shouldCancel);
}
} // namespace

Result<> MeshingUtilities::detail::GatherVertexCoordinates(const Float32AbstractDataStore& vertices, nonstd::span<const IGeometry::MeshIndexType> indexes, nonstd::span<usize> order,
                                                           nonstd::span<float32> gathered, nonstd::span<float32> page)
{
  if(vertices.getNumberOfComponents() != 3)
  {
    return MakeErrorResult(-65900, "Vertex-coordinate gather requires three-component XYZ tuples.");
  }
  return gatherCoordinatePages(vertices, vertices.getNumberOfTuples(), indexes, order, gathered, page);
}

Result<> MeshingUtilities::detail::GatherVertexCoordinates(const ITemporaryRecordStore& vertices, nonstd::span<const IGeometry::MeshIndexType> indexes, nonstd::span<usize> order,
                                                           nonstd::span<float32> gathered, nonstd::span<float32> page)
{
  if(vertices.recordSize() != 3 * sizeof(float32) || vertices.recordCount() > std::numeric_limits<usize>::max() || page.size() / 3 > vertices.maxRecordsPerBatch())
  {
    return MakeErrorResult(-65905, "Temporary vertex-coordinate gather requires addressable XYZ records and a page no larger than the provider batch limit.");
  }
  return gatherCoordinatePages(vertices, static_cast<usize>(vertices.recordCount()), indexes, order, gathered, page);
}

MeshingUtilities::SortedVerticesList MeshingUtilities::OrderSharedVertices(const nx::core::INodeGeometry0D& geom, const std::atomic_bool& shouldCancel)
{
  const BoundingBox3Df& bounds = geom.getBoundingBox();

  const Point3Df diff = bounds.sideLengths();

  // Inverse of to_underlying
  const AxialAlignment axis = static_cast<AxialAlignment>(std::distance(diff.begin(), std::max_element(diff.begin(), diff.end())));

  // Getting the verts list by ref here for the validation in the ref function
  const INodeGeometry0D::SharedVertexList::store_type& vertexListStore = geom.getVerticesRef().getDataStoreRef();

  return {.axis = axis, .ordering = std::move(OrderSharedVerticesAlongAxis(axis, vertexListStore, shouldCancel))};
}

std::vector<IGeometry::MeshIndexType> MeshingUtilities::OrderSharedVerticesAlongAxis(nx::core::MeshingUtilities::AxialAlignment axis, const INodeGeometry0D::SharedVertexList::store_type& vertexList,
                                                                                     const std::atomic_bool& shouldCancel)
{
  std::vector<IGeometry::MeshIndexType> sorted(vertexList.getNumberOfTuples());
  std::iota(sorted.begin(), sorted.end(), 0);

  QuickSortVertices(sorted, 0, sorted.size() - 1, vertexList, to_underlying(axis), shouldCancel);

  return sorted;
}

bool MeshingUtilities::HasDuplicateVertices(const IGeometry::SharedVertexList::store_type& verts, const nx::core::MeshingUtilities::SortedVerticesList& sortedVertices)
{
  using VertT = INodeGeometry0D::SharedVertexList::value_type;

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
