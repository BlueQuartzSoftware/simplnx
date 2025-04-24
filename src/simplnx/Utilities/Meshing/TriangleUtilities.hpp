#pragma once

#include "simplnx/Common/Range.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core::MeshingUtilities
{
namespace detail
{
inline static constexpr usize k_00 = 0;
inline static constexpr usize k_01 = 1;
inline static constexpr usize k_02 = 2;
inline static constexpr usize k_10 = 3;
inline static constexpr usize k_11 = 4;
inline static constexpr usize k_12 = 5;
inline static constexpr usize k_20 = 6;
inline static constexpr usize k_21 = 7;
inline static constexpr usize k_22 = 8;

INodeGeometry2D::SharedVertexList::value_type FindTetrahedronVolume(const std::array<usize, 3>& vertIndices, const INodeGeometry2D::SharedVertexList::store_type& vertices);
} // namespace detail

/**
 * @brief This function attempts to make winding as consistent as possible.
 * NOTE: This algorithm requires that there are NO DUPLICATE vertices in the mesh
 * @param triangles The SharedFaceList that may be modified
 * @param neighbors The element neighbors adjacency list
 * @param faceLabelsStore This is the face ids, used to determine expected winding of each triangle
 * @returns Result<usize> This result will contain the number of triangles that could not be repaired
 */
SIMPLNX_EXPORT Result<> RepairTriangleWinding(INodeGeometry2D::SharedFaceList::store_type& triangles, const DynamicListArray<uint16, IGeometry::MeshIndexType>& neighbors,
                               const Int32AbstractDataStore& faceLabelsStore, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);

/**
 * @brief The CalculateAreasImpl class implements a threaded algorithm that computes the normal of each
 * triangle for a set of triangles
 */
class SIMPLNX_EXPORT CalculateNormalsImpl
{
public:
  CalculateNormalsImpl(const INodeGeometry2D::SharedFaceList::store_type& triangles, const INodeGeometry2D::SharedVertexList::store_type& verts, Float64AbstractDataStore& normals,
                       const std::atomic_bool& shouldCancel);
  ~CalculateNormalsImpl() = default;

  void generate(usize start, usize end) const;

  void operator()(const Range& range) const;

private:
  const INodeGeometry2D::SharedFaceList::store_type& m_Triangles;
  const INodeGeometry2D::SharedVertexList::store_type& m_Vertices;
  Float64AbstractDataStore& m_Normals;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @brief The
 */
template <class ContainerT>
Result<> CalculateFeatureVolumes(const INodeGeometry2D::SharedFaceList::store_type& triangles, const INodeGeometry2D::SharedVertexList::store_type& verts, const Int32AbstractDataStore& faceLabels,
                                 ContainerT& volumes, const std::atomic_bool& shouldCancel)
{
  std::array<usize, 3> faceVertexIndices = {0, 0, 0};
  for(usize i = 0; i < triangles.getNumberOfTuples(); i++)
  {
    if(shouldCancel)
    {
      return {};
    }

    const usize triangleIndex = i * 3;
    faceVertexIndices[0] = triangles[triangleIndex];
    faceVertexIndices[1] = triangles[triangleIndex + 1];
    faceVertexIndices[2] = triangles[triangleIndex + 2];

    int32 faceLabel0 = faceLabels[2 * i + 0];
    int32 faceLabel1 = faceLabels[2 * i + 1];

    if(faceLabel0 < 0 && faceLabel1 >= 0)
    {
      std::swap(faceVertexIndices[2], faceVertexIndices[1]);
      volumes[faceLabel1] += detail::FindTetrahedronVolume(faceVertexIndices, verts);
    }
    else if(faceLabel1 < 0 && faceLabel0 >= 0)
    {
      volumes[faceLabel0] += detail::FindTetrahedronVolume(faceVertexIndices, verts);
    }
    else
    {
      volumes[faceLabel0] += detail::FindTetrahedronVolume(faceVertexIndices, verts);
      std::swap(faceVertexIndices[2], faceVertexIndices[1]);
      volumes[faceLabel1] += detail::FindTetrahedronVolume(faceVertexIndices, verts);
    }
  }

  return {};
}
} // namespace nx::core::MeshingUtilities
