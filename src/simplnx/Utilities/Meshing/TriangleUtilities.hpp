#pragma once

#include "simplnx/Common/Range.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
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
/**
 * @brief Warning emitted when 'Omit Bounding Box Skin' (BoundingBoxSkinMode::k_BackgroundBackedWallsOnly)
 * removes every face of the mesh -- i.e. the input is entirely background (Feature Id 0), so there is no
 * internal interface and no Feature to cap any box wall. Shared verbatim by QuickSurfaceMesh, SurfaceNets,
 * and M3CSurfaceMeshing so the warning text and code are defined exactly once. See MakeEmptyMeshWarning().
 */
inline constexpr int32 k_EmptyMeshAfterSkinRemovalWarning = -56340;

/**
 * @brief Warning emitted when 'Omit Bounding Box Skin' (BoundingBoxSkinMode::k_BackgroundBackedWallsOnly)
 * is enabled but suppressed zero faces -- i.e. the input volume contains no background (Feature Id 0)
 * voxels, so the option had nothing to prune and the output is identical to leaving it off. This is the
 * most common dataset shape in practice, so silent no-feedback behavior here is not acceptable. See
 * MakeNoFacesPrunedWarning().
 */
inline constexpr int32 k_NoFacesPrunedWarning = -56342;

/**
 * @brief Error emitted when a Feature Ids array contains a value that collides with a mesher's internal
 * "not a real Feature" sentinel space (see ValidateFeatureIdsAgainstSentinels()). This is a mitigation
 * for the underlying sentinel-collision design, not a fix for it -- the architectural issue is tracked
 * separately as simplnx#1705.
 */
inline constexpr int32 k_InvalidFeatureIdError = -56343;

/**
 * @brief Builds the warning Result for k_EmptyMeshAfterSkinRemovalWarning (see its docs above).
 * The vertex count is reported rather than assumed to be zero: QuickSurfaceMesh and SurfaceNets
 * both reach zero vertices in this case, but M3CSurfaceMeshing's marching-cubes candidate generation
 * can leave a handful of pre-existing candidate nodes that no triangle -- dropped or surviving --
 * ever referenced, so its vertex count here is not necessarily zero.
 * @param triangleGeomPath Path to the (now-empty, or near-empty) Triangle Geometry, named in the message.
 * @param numCells Number of Feature Id cells in the input, named in the message.
 * @param numVertices Number of vertices remaining in the Triangle Geometry after the prune.
 */
SIMPLNX_EXPORT Result<> MakeEmptyMeshWarning(const DataPath& triangleGeomPath, usize numCells, usize numVertices);

/**
 * @brief Builds the warning Result for k_NoFacesPrunedWarning (see its docs above).
 * @param triangleGeomPath Path to the Triangle Geometry, named in the message.
 */
SIMPLNX_EXPORT Result<> MakeNoFacesPrunedWarning(const DataPath& triangleGeomPath);

/**
 * @brief Validates that no value in a Feature Ids array collides with a mesher's internal "not a real
 * Feature" sentinel space (e.g. SurfaceNets' MMSurfaceNet::Padding == INT32_MAX, M3CSurfaceMeshing's
 * maxGrainId+1 overflow and nSpin < 0 ghost convention, QuickSurfaceMesh's hard-coded -1 exterior Face
 * Label). This is a mitigation, not a fix, for that design -- the architectural issue is tracked
 * separately as simplnx#1705.
 *
 * Call this from an algorithm's execute entry point, never from preflight: a full-volume scan (e.g.
 * ~134M reads at 512^3) is too expensive to repeat on every GUI parameter edit.
 * @param featureIdsStore The Feature Ids to validate.
 * @param featureIdsPath Path to the array, named in the error message so the user can locate it.
 * @param rejectMaxInt32 When true, also reject a Feature Id of exactly INT32_MAX (SurfaceNets and
 * M3CSurfaceMeshing both need this; QuickSurfaceMesh only collides on negative values, so it passes false).
 * @returns An invalid Result<> naming the offending value, its tuple index, and featureIdsPath on the
 * first rejected value found; an empty valid Result<> otherwise.
 */
SIMPLNX_EXPORT Result<> ValidateFeatureIdsAgainstSentinels(const Int32AbstractDataStore& featureIdsStore, const DataPath& featureIdsPath, bool rejectMaxInt32);

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
