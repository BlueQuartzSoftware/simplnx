#pragma once

#include "simplnx/Common/Range.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"

namespace nx::core
{
/**
 * @brief Shared values for the "Bounding Box Skin" ChoicesParameter used by QuickSurfaceMeshFilter,
 * SurfaceNetsFilter, and M3CSurfaceMeshingFilter. Named here (rather than as bare literals) so a
 * future third mode can be added without every `== 1` comparison needing to be rediscovered.
 */
namespace BoundingBoxSkinMode
{
inline constexpr ChoicesParameter::ValueType k_Off = 0;
inline constexpr ChoicesParameter::ValueType k_BackgroundBackedWallsOnly = 1;
} // namespace BoundingBoxSkinMode
} // namespace nx::core

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

/**
 * @brief This function calculates the volume of a supplied triangle
 * @param vertIndices The indices that make up the points of a triangle
 * @param vertices The SharedVertexList of the parent geometry
 * @returns INodeGeometry2D::SharedVertexList::value_type the calculated volume
 */
SIMPLNX_EXPORT INodeGeometry2D::SharedVertexList::value_type FindTriangleVolume(const std::array<usize, 3>& vertIndices, const INodeGeometry2D::SharedVertexList::store_type& vertices);
} // namespace detail

/**
 * @brief This function attempts to make winding as consistent as possible.
 * NOTE: This algorithm requires that there are NO DUPLICATE vertices in the mesh
 * @param triangles The SharedFaceList that may be modified
 * @param neighbors The element neighbors adjacency list
 * @param idsStore This is the face ids or the region ids; num of components < 3 enforced
 * @param shouldCancel
 * @param mesgHandler
 * @returns Result<usize> This result will contain the number of triangles that could not be repaired
 */
SIMPLNX_EXPORT Result<> RepairTriangleWinding(INodeGeometry2D::SharedFaceList::store_type& triangles, const DynamicListArray<uint16, IGeometry::MeshIndexType>& neighbors,
                                              const Int32AbstractDataStore& idsStore, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);

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
 * @brief This function calculates feature volumes without any bound check applied
 * @tparam ContainerT the type of the array to be filled; must have operator[] defined
 * @param triangles The SharedFaceList of the target geometry
 * @param verts The SharedVertexList of the target geometry
 * @param idsStore This is the face ids or the region ids; num of components < 3 enforced
 * @param volumes This the array that will be filled with volumes (no bounds check, size > max feature expected)
 * @param shouldCancel Atomic Bool to check if the algorithm should exit early
 * @returns Result<usize> function result
 */
template <class ContainerT>
Result<> CalculateFeatureVolumes(const INodeGeometry2D::SharedFaceList::store_type& triangles, const INodeGeometry2D::SharedVertexList::store_type& verts, const Int32AbstractDataStore& idsStore,
                                 ContainerT& volumes, const std::atomic_bool& shouldCancel)
{
  usize volumeSize = volumes.size();
  std::array<usize, 3> faceVertexIndices = {0, 0, 0};
  if(idsStore.getNumberOfComponents() == 2)
  {
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

      int32 faceLabel0 = idsStore[2 * i + 0];
      int32 faceLabel1 = idsStore[2 * i + 1];

      bool faceLabel0InRange = faceLabel0 >= 0 && faceLabel0 < static_cast<int64_t>(volumeSize);
      bool faceLabel1InRange = faceLabel1 >= 0 && faceLabel1 < static_cast<int64_t>(volumeSize);

      if(faceLabel0 < 0 && faceLabel1InRange)
      {
        std::swap(faceVertexIndices[2], faceVertexIndices[1]);
        volumes[faceLabel1] += detail::FindTriangleVolume(faceVertexIndices, verts);
      }
      else if(faceLabel1 < 0 && faceLabel0InRange)
      {
        volumes[faceLabel0] += detail::FindTriangleVolume(faceVertexIndices, verts);
      }
      else if(faceLabel0InRange && faceLabel1InRange)
      {
        volumes[faceLabel0] += detail::FindTriangleVolume(faceVertexIndices, verts);
        std::swap(faceVertexIndices[2], faceVertexIndices[1]);
        volumes[faceLabel1] += detail::FindTriangleVolume(faceVertexIndices, verts);
      }
    }
  }
  else if(idsStore.getNumberOfComponents() == 1)
  {
    for(usize i = 0; i < triangles.getNumberOfTuples(); i++)
    {
      if(shouldCancel)
      {
        return {};
      }

      const usize triangleIndex = i * 3;
      faceVertexIndices[0] = triangles[triangleIndex];
      faceVertexIndices[1] = triangles[triangleIndex + 2];
      faceVertexIndices[2] = triangles[triangleIndex + 1];

      int32 featureId = idsStore[i];
      if(featureId < 0)
      {
        continue;
      }
      volumes[featureId] += detail::FindTriangleVolume(faceVertexIndices, verts);
    }
  }
  else
  {
    return MakeErrorResult(-65771, fmt::format("MeshingUtilities::CalculateFeatureVolumes: invalid ID array supplied. The ID array must have 1 or 2 components, supplied array components: {}.",
                                               idsStore.getNumberOfComponents()));
  }

  return {};
}
} // namespace nx::core::MeshingUtilities
