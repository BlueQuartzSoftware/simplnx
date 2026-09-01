#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @struct SampleSurfaceMeshInputValues
 * @brief Identifies the surface mesh, face labels, and output feature IDs.
 */
struct SIMPLNX_EXPORT SampleSurfaceMeshInputValues
{
  DataPath TriangleGeometryPath;
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath FeatureIdsArrayPath;
};

/**
 * @class SampleSurfaceMesh
 * @brief Assigns each sampling-grid cell to an enclosing surface-mesh feature.
 *
 * Each cell has one sample point that can include a subclass-specific offset.
 * The algorithm tests points against feature polyhedra in increasing feature-ID
 * order. The first enclosing feature wins.
 *
 * Point and output working memory is proportional to one XY slice. Face lists
 * and bounding volumes remain proportional to the triangle mesh. Each completed
 * slice uses one bulk output write. Slice generation is serial and follows
 * increasing Z order so stateful generators keep a stable draw sequence.
 * Mesh preprocessing reads face labels, vertices, and triangles before slice sampling.
 *
 * Parallel point tests write disjoint output slots. They also read triangle
 * geometry concurrently. Generic DataStore access has no concurrent-read guarantee.
 * Safe execution requires concrete geometry stores with a stronger read contract
 * or serialized geometry access.
 */
class SIMPLNX_EXPORT SampleSurfaceMesh
{
public:
  /**
   * @brief Creates a surface-mesh sampler.
   * @param dataStructure Supplies input geometry and output arrays.
   * @param shouldCancel Supplies the cancellation flag.
   * @param mesgHandler Receives progress messages.
   *
   * All three objects must outlive this sampler.
   */
  SampleSurfaceMesh(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  virtual ~SampleSurfaceMesh() noexcept;

  SampleSurfaceMesh(const SampleSurfaceMesh&) = delete;
  SampleSurfaceMesh(SampleSurfaceMesh&&) = delete;
  SampleSurfaceMesh& operator=(const SampleSurfaceMesh&) = delete;
  SampleSurfaceMesh& operator=(SampleSurfaceMesh&&) = delete;

  /**
   * @brief Samples the configured triangle features into the output array.
   * @param inputValues Identifies input and output data objects.
   * @return Valid result, bulk-write error, or feature-ID overflow error.
   * @pre Face labels have an integer type and two components per triangle.
   * @pre The output has an integer type and one tuple for each sampling-grid cell.
   *
   * Cancellation returns a valid result. The current and later slices remain
   * unchanged when cancellation stops a slice before its bulk write.
   */
  Result<> execute(SampleSurfaceMeshInputValues& inputValues);

  /**
   * @brief Gets sampling-grid cell dimensions in X, Y, Z order.
   * @return Sampling-grid dimensions.
   * @pre Dimension products fit usize.
   */
  virtual SizeVec3 getGridDimensions() const = 0;

  /**
   * @brief Generates sample points for one Z slice.
   * @param zSlice Identifies the slice in increasing order.
   * @param slicePoints Receives points in X-fastest row-major order.
   * @pre zSlice is less than the grid Z dimension. slicePoints is pre-sized to X times Y.
   *
   * Fill by index and do not resize the vector. A stateful random generator must
   * continue its sequence across calls. This preserves the full-volume draw order.
   */
  virtual void generateSlicePoints(usize zSlice, std::vector<Point3Df>& slicePoints) = 0;

private:
  DataStructure& m_DataStructure;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  MessageHelper m_MessageHelper;
};
} // namespace nx::core
