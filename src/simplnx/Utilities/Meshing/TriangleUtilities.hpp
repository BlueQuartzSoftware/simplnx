#pragma once

#include "simplnx/Common/Range.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @brief Shared values for the "Bounding Box Skin" ChoicesParameter used by QuickSurfaceMeshFilter,
 * SurfaceNetsFilter, and M3CSurfaceMeshingFilter. Named here (rather than as bare literals) so a
 * future third mode can be added without every `== 1` comparison needing to be rediscovered.
 * Typed as `uint64` (rather than `ChoicesParameter::ValueType`, which is itself just an alias for
 * `uint64`) so this header does not need to pull in ChoicesParameter.hpp; every use site compares or
 * assigns against a `ChoicesParameter::ValueType`, so the two types are interchangeable here.
 */
namespace BoundingBoxSkinMode
{
inline constexpr uint64 k_Off = 0;
inline constexpr uint64 k_BackgroundBackedWallsOnly = 1;
} // namespace BoundingBoxSkinMode
} // namespace nx::core

namespace nx::core::MeshingUtilities
{
/**
 * @brief Warning emitted when the 'Bounding Box Skin' option's 'Background-Backed Walls Only' mode
 * (BoundingBoxSkinMode::k_BackgroundBackedWallsOnly) removes every face of the mesh -- i.e. the input is
 * entirely background (Feature Id 0), so there is no internal interface and no Feature to cap any box
 * wall. Shared verbatim by QuickSurfaceMesh, SurfaceNets, and M3CSurfaceMeshing so the warning text and
 * code are defined exactly once. See MakeEmptyMeshWarning().
 */
inline constexpr int32 k_EmptyMeshAfterSkinRemovalWarning = -56340;

/**
 * @brief Warning emitted when the 'Bounding Box Skin' option's 'Background-Backed Walls Only' mode
 * (BoundingBoxSkinMode::k_BackgroundBackedWallsOnly) is enabled but suppressed zero bounding-box wall
 * faces -- i.e. no wall face is backed by background (Feature Id 0), so the option had nothing to
 * prune and the output is identical to leaving it off. This says nothing about whether the volume
 * contains background elsewhere: a volume whose background is fully enclosed as interior porosity
 * also reaches this warning, because none of that background borders a bounding-box wall. This is
 * the most common dataset shape in practice, so silent no-feedback behavior here is not acceptable.
 * See MakeNoFacesPrunedWarning().
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
 * ~134M reads at 512^3) is too expensive to repeat on every GUI parameter edit. Because that same
 * full-volume scan is otherwise silent and uncancellable, this emits a message before scanning and
 * polls shouldCancel periodically (not on every tuple, to keep the inner loop tight).
 * @param featureIdsStore The Feature Ids to validate.
 * @param featureIdsPath Path to the array, named in the error message so the user can locate it.
 * @param rejectMaxInt32 When true, also reject a Feature Id of exactly INT32_MAX (SurfaceNets and
 * M3CSurfaceMeshing both need this; QuickSurfaceMesh only collides on negative values, so it passes false).
 * @param shouldCancel Checked periodically so the scan can be interrupted.
 * @param mesgHandler Used to report that the scan is running before it starts.
 * @returns An invalid Result<> naming the offending value, its tuple index, and featureIdsPath on the
 * first rejected value found; an empty valid Result<> otherwise (including if cancelled).
 */
SIMPLNX_EXPORT Result<> ValidateFeatureIdsAgainstSentinels(const Int32AbstractDataStore& featureIdsStore, const DataPath& featureIdsPath, bool rejectMaxInt32, const std::atomic_bool& shouldCancel,
                                                           const IFilter::MessageHandler& mesgHandler);

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
 * @brief Calculates one triangle's signed origin-based volume contribution.
 * @param vertIndices Specifies three vertex indexes.
 * @param vertices Provides flat XYZ coordinates.
 * @return Signed volume contribution.
 */
SIMPLNX_EXPORT INodeGeometry2D::SharedVertexList::value_type FindTriangleVolume(const std::array<usize, 3>& vertIndices, const INodeGeometry2D::SharedVertexList::store_type& vertices);
} // namespace detail

/**
 * @brief Makes triangle winding as consistent as mesh topology permits.
 * @param triangles Provides and receives triangle connectivity.
 * @param neighbors Provides adjacent triangles.
 * @param idsStore Provides two face labels or one region ID per triangle.
 * @param shouldCancel Stops before later traversal work when true.
 * @param mesgHandler Receives progress messages.
 * @return Error, warning for unrepaired triangles, or success after cancellation.
 * @pre The mesh has no duplicate vertices.
 */
SIMPLNX_EXPORT Result<> RepairTriangleWinding(INodeGeometry2D::SharedFaceList::store_type& triangles, const DynamicListArray<uint16, IGeometry::MeshIndexType>& neighbors,
                                              const Int32AbstractDataStore& idsStore, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);

/**
 * @brief Attempts to make triangle winding consistent without materializing mesh-sized face, label,
 * connectivity, traversal-state, or queue arrays in memory.
 *
 * This storage-neutral variant reconstructs the same triangle-neighbor order as the legacy
 * connectivity path through bounded external sorts. Mutable traversal state and the FIFO queue are
 * held in temporary record stores, while face and ID DataStores are accessed through bounded page
 * caches. A registered I/O manager that provides external sorting and temporary record storage is
 * required.
 *
 * @param triangles The SharedFaceList that may be modified.
 * @param idsStore Face labels (2 components) or region IDs (1 component).
 * @param shouldCancel Cooperative cancellation flag.
 * @param mesgHandler Progress-message callback.
 * @return Provider, sort, cache, topology, or DataStore error, or success after cancellation.
 */
SIMPLNX_EXPORT Result<> RepairTriangleWindingExternal(INodeGeometry2D::SharedFaceList::store_type& triangles, const Int32AbstractDataStore& idsStore, const std::atomic_bool& shouldCancel,
                                                      const IFilter::MessageHandler& mesgHandler);

/**
 * @class CalculateNormalsImpl
 * @brief Computes triangle normals over scheduler ranges.
 */
class SIMPLNX_EXPORT CalculateNormalsImpl
{
public:
  /**
   * @brief Creates a borrowed normal-calculation worker.
   * @param triangles Provides triangle connectivity.
   * @param verts Provides vertex coordinates.
   * @param normals Receives three values per triangle.
   * @param shouldCancel Stops before later triangles when true.
   */
  CalculateNormalsImpl(const INodeGeometry2D::SharedFaceList::store_type& triangles, const INodeGeometry2D::SharedVertexList::store_type& verts, Float64AbstractDataStore& normals,
                       const std::atomic_bool& shouldCancel);
  /**
   * @brief Destroys the borrowed worker.
   */
  ~CalculateNormalsImpl() = default;

  /**
   * @brief Computes normals for one triangle range.
   * @param start Specifies the first triangle.
   * @param end Specifies the exclusive last triangle.
   */
  void generate(usize start, usize end) const;

  /**
   * @brief Computes normals for one scheduler range.
   * @param range Specifies the triangle range.
   */
  void operator()(const Range& range) const;

private:
  const INodeGeometry2D::SharedFaceList::store_type& m_Triangles;
  const INodeGeometry2D::SharedVertexList::store_type& m_Vertices;
  Float64AbstractDataStore& m_Normals;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @brief Accumulates signed triangle volume contributions by feature ID.
 * @tparam ContainerT Specifies a random-access volume container.
 * @param triangles Provides triangle connectivity.
 * @param verts Provides vertex coordinates.
 * @param idsStore Provides two face labels or one region ID per triangle.
 * @param volumes Receives accumulated volumes.
 * @param shouldCancel Stops before later triangles when true.
 * @return Error for an invalid ID component count, or success after cancellation.
 * @pre volumes contains every nonnegative ID when idsStore has one component.
 *
 * The method uses direct per-value DataStore access and does not report I/O errors.
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
