#pragma once

#include "simplnx/Common/Range.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"

namespace nx::core::MeshingUtilities
{
/**
 * @brief This function attempts to make winding as consistent as possible.
 * NOTE: This algorithm requires that there are NO DUPLICATE vertices in the mesh
 * @param triangles The SharedFaceList that may be modified
 * @param faceLabelsStore This is the face ids, used to determine expected winding of each triangle
 * @returns Result<usize> This result will contain the number of triangles that could not be repaired
 */
Result<> RepairTriangleWinding(INodeGeometry2D::SharedFaceList::store_type& triangles, const Int32AbstractDataStore& faceLabelsStore, const std::atomic_bool& shouldCancel);

/**
 * @brief The CalculateAreasImpl class implements a threaded algorithm that computes the normal of each
 * triangle for a set of triangles
 */
class CalculateNormalsImpl
{
public:
  CalculateNormalsImpl(const INodeGeometry2D::SharedFaceList::store_type& triangles, const INodeGeometry2D::SharedVertexList::store_type& verts, Float64AbstractDataStore& normals, const std::atomic_bool& shouldCancel);
  ~CalculateNormalsImpl() = default;

  void generate(usize start, usize end) const;

  void operator()(const Range& range) const;

private:
  const INodeGeometry2D::SharedFaceList::store_type& m_Triangles;
  const INodeGeometry2D::SharedVertexList::store_type& m_Vertices;
  Float64AbstractDataStore& m_Normals;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace nx::core::MeshingUtilities
