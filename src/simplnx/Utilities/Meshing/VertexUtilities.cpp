#include "VertexUtilities.hpp"

#include "simplnx/DataStructure/DataStore.hpp"

#ifdef SIMPLNX_ENABLE_MULTICORE
#include <tbb/parallel_sort.h>
#endif

#include <algorithm>
#include <cmath>
#include <numeric>

using namespace nx::core;

MeshingUtilities::SortedVerticesList MeshingUtilities::OrderSharedVertices(const nx::core::INodeGeometry0D& geom, const std::atomic_bool& shouldCancel)
{
  return {.axis = AxialAlignment::X, .ordering = OrderSharedVerticesAlongAxis(AxialAlignment::X, geom.getVerticesRef().getDataStoreRef(), shouldCancel)};
}

std::vector<IGeometry::MeshIndexType> MeshingUtilities::OrderSharedVerticesAlongAxis(nx::core::MeshingUtilities::AxialAlignment axis, const INodeGeometry0D::SharedVertexList::store_type& vertexList,
                                                                                     const std::atomic_bool& shouldCancel)
{
  std::vector<IGeometry::MeshIndexType> sorted(vertexList.getNumberOfTuples());
  std::iota(sorted.begin(), sorted.end(), 0);
  if(shouldCancel)
  {
    return sorted;
  }

  const auto compare = [axis](const auto& vertices, IGeometry::MeshIndexType lhs, IGeometry::MeshIndexType rhs) {
    for(usize c = 0; c < 3; ++c)
    {
      const usize offset = (static_cast<usize>(axis) + c) % 3;
      const float32 left = vertices[lhs * 3 + offset];
      const float32 right = vertices[rhs * 3 + offset];
      // NaNs follow numbers on each axis to preserve a strict weak order.
      const bool leftNaN = std::isnan(left);
      const bool rightNaN = std::isnan(right);
      if(leftNaN != rightNaN)
      {
        return !leftNaN;
      }
      if(left < right)
      {
        return true;
      }
      if(left > right)
      {
        return false;
      }
    }
    // The first input instance precedes all copies, including signed zeros.
    return lhs < rhs;
  };
  if(const auto* store = dynamic_cast<const DataStore<float32>*>(&vertexList); vertexList.getStoreType() == IDataStore::StoreType::InMemory && store != nullptr)
  {
    // This resident buffer stays alive and unchanged until the synchronous sort joins.
    // Workers read only the buffer, never the store API.
    const float32* data = store->data();
    const auto residentCompare = [data, &compare](IGeometry::MeshIndexType lhs, IGeometry::MeshIndexType rhs) { return compare(data, lhs, rhs); };
#ifdef SIMPLNX_ENABLE_MULTICORE
    tbb::parallel_sort(sorted.begin(), sorted.end(), residentCompare);
#else
    std::sort(sorted.begin(), sorted.end(), residentCompare);
#endif
  }
  else
  {
    // Other stores may mutate internal state even on reads; keep their access serial.
    std::sort(sorted.begin(), sorted.end(), [&vertexList, &compare](IGeometry::MeshIndexType lhs, IGeometry::MeshIndexType rhs) { return compare(vertexList, lhs, rhs); });
  }
  return sorted;
}

bool MeshingUtilities::HasDuplicateVertices(const IGeometry::SharedVertexList::store_type& verts, const nx::core::MeshingUtilities::SortedVerticesList& sortedVertices)
{
  for(usize i = 1; i < sortedVertices.ordering.size(); ++i)
  {
    const IGeometry::MeshIndexType previous = sortedVertices.ordering[i - 1] * 3;
    const IGeometry::MeshIndexType current = sortedVertices.ordering[i] * 3;
    if(verts[previous] == verts[current] && verts[previous + 1] == verts[current + 1] && verts[previous + 2] == verts[current + 2])
    {
      return true;
    }
  }
  return false;
}
