#include "ComputeSurfaceFeaturesScanline.hpp"

#include "ComputeSurfaceFeatures.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <fmt/format.h>

#include <nonstd/span.hpp>

using namespace nx::core;

// ----------------------------------------------------------------------------
// ComputeSurfaceFeaturesScanline -- Out-of-Core Algorithm
//
// Identifies which features touch the outer surface of the image geometry or
// border FeatureId==0 voxels. Produces the same output as the Direct variant:
// a feature-level UInt8 array where 0 = interior, 1 = surface.
//
// KEY DESIGN PRINCIPLE: All access to the large cell-level FeatureIds array is
// strictly sequential by Z-slice, using copyIntoBuffer() for bulk reads. The
// small feature-level SurfaceFeatures array is cached locally in a std::vector
// and written back in a single copyFromBuffer() call at the end.
//
// ROLLING WINDOW: Three std::vector<int32> buffers (prevSlice, curSlice,
// nextSlice) hold adjacent Z-slices so that all neighbor lookups are simple
// in-memory array accesses with no disk I/O.
//
// 2D GEOMETRY HANDLING: Unlike the Direct variant which has separate 2D/3D
// code paths, the Scanline variant always iterates the native Z-Y-X grid and
// remaps coordinates to the 2D plane as needed. This unified approach
// maintains sequential Z-slice I/O even for 2D geometries where the
// degenerate dimension is X or Y (not Z).
//
// For the degenerate-Z case (zPoints==1), all data fits in a single Z-slice,
// so prevSlice/nextSlice are unused and all 4 neighbors come from curSlice.
// For degenerate-X or degenerate-Y cases, the remapped-Y direction maps to
// the native Z axis, so the +/-Y neighbors come from prevSlice/nextSlice.
// ----------------------------------------------------------------------------

namespace
{
/**
 * @brief Checks whether a voxel in a 2D geometry qualifies its feature as a
 * surface feature, using the rolling-window slice buffers.
 *
 * This function handles coordinate remapping from the native 3D Z-Y-X grid
 * to the logical 2D plane. The remapping depends on which dimension is
 * degenerate (size == 1):
 *
 * - **Degenerate Z (zPoints==1)**: The entire dataset is a single Z-slice.
 *   remappedX = native X, remappedY = native Y. All 4 neighbors live in
 *   curSlice using standard Y*xPoints+X indexing.
 *
 * - **Degenerate X or Y**: The non-degenerate in-plane dimension maps to
 *   remappedX (contiguous in memory), and the native Z dimension maps to
 *   remappedY. The +/-remappedX neighbors are at nativeInSlice +/- 1 in
 *   curSlice. The +/-remappedY neighbors come from prevSlice/nextSlice
 *   (the adjacent native Z-slices).
 *
 * @param remappedX X coordinate in the logical 2D plane.
 * @param remappedY Y coordinate in the logical 2D plane.
 * @param remappedXPoints Number of cells in the remapped X dimension.
 * @param remappedYPoints Number of cells in the remapped Y dimension.
 * @param markFeature0Neighbors Whether to check for FeatureId==0 neighbors.
 * @param curSlice Buffer holding the current native Z-slice's FeatureIds.
 * @param prevSlice Buffer holding the previous native Z-slice's FeatureIds.
 * @param nextSlice Buffer holding the next native Z-slice's FeatureIds.
 * @param nativeInSlice Flat index of this voxel within the native Z-slice
 *   (y * nativeXPoints + x). Used for non-degenerate-Z neighbor lookups.
 * @param hasPrevSlice True if a previous Z-slice exists (z > 0).
 * @param hasNextSlice True if a next Z-slice exists (z + 1 < zPoints).
 * @param degenerateZ True if the Z dimension has size 1.
 * @return true if the voxel makes its feature a surface feature.
 */
bool IsPointASurfaceFeature2D(usize remappedX, usize remappedY, usize remappedXPoints, usize remappedYPoints, bool markFeature0Neighbors, const std::vector<int32>& curSlice,
                              const std::vector<int32>& prevSlice, const std::vector<int32>& nextSlice, usize nativeInSlice, bool hasPrevSlice, bool hasNextSlice, bool degenerateZ)
{
  // Boundary check: voxels on the outer edges of the 2D plane are surface voxels
  if(remappedX <= 0 || remappedX >= remappedXPoints - 1)
  {
    return true;
  }
  if(remappedY <= 0 || remappedY >= remappedYPoints - 1)
  {
    return true;
  }

  if(markFeature0Neighbors)
  {
    if(degenerateZ)
    {
      // DEGENERATE Z: All data is in one Z-slice. The 4 neighbors in the
      // 2D plane are all within curSlice using the native Y*xPoints+X layout
      // (which matches the remapped layout since remappedX=X, remappedY=Y).
      const usize yStride = remappedY * remappedXPoints;
      // -X neighbor
      if(curSlice[yStride + remappedX - 1] == 0)
      {
        return true;
      }
      // +X neighbor
      if(curSlice[yStride + remappedX + 1] == 0)
      {
        return true;
      }
      // -Y neighbor (previous row in the same slice)
      if(curSlice[(remappedY - 1) * remappedXPoints + remappedX] == 0)
      {
        return true;
      }
      // +Y neighbor (next row in the same slice)
      if(curSlice[(remappedY + 1) * remappedXPoints + remappedX] == 0)
      {
        return true;
      }
    }
    else
    {
      // DEGENERATE X or Y: The remapped Y direction maps to the native Z
      // axis, so +/-remappedY neighbors come from the adjacent Z-slices
      // (prevSlice/nextSlice). The remapped X neighbors are within curSlice
      // at +/-1 from the native in-slice index. This works for both
      // degenerate-X and degenerate-Y because the non-degenerate in-plane
      // dimension is always contiguous in the native Z-slice layout.

      // -remappedX neighbor (adjacent element in the current Z-slice)
      if(curSlice[nativeInSlice - 1] == 0)
      {
        return true;
      }
      // +remappedX neighbor
      if(curSlice[nativeInSlice + 1] == 0)
      {
        return true;
      }
      // -remappedY neighbor (same position in the previous native Z-slice)
      if(hasPrevSlice && prevSlice[nativeInSlice] == 0)
      {
        return true;
      }
      // +remappedY neighbor (same position in the next native Z-slice)
      if(hasNextSlice && nextSlice[nativeInSlice] == 0)
      {
        return true;
      }
    }
  }

  return false;
}

/**
 * @brief Checks whether a voxel in a 3D geometry qualifies its feature as a
 * surface feature, using the rolling-window slice buffers.
 *
 * All 6 face-neighbor lookups use the in-memory buffers:
 * - +/-X: curSlice at inSlice +/- 1
 * - +/-Y: curSlice at inSlice +/- xPoints (one row offset)
 * - -Z:   prevSlice at the same inSlice position
 * - +Z:   nextSlice at the same inSlice position
 *
 * This avoids any direct access to the OOC DataStore, which is the entire
 * point of the Scanline approach.
 *
 * @param x X coordinate of the voxel.
 * @param y Y coordinate of the voxel.
 * @param z Z coordinate of the voxel.
 * @param xPoints Number of cells in X.
 * @param yPoints Number of cells in Y.
 * @param zPoints Number of cells in Z.
 * @param markFeature0Neighbors Whether to check for FeatureId==0 neighbors.
 * @param prevSlice Buffer holding Z-slice (z-1).
 * @param curSlice Buffer holding Z-slice (z).
 * @param nextSlice Buffer holding Z-slice (z+1).
 * @return true if the voxel makes its feature a surface feature.
 */
bool IsPointASurfaceFeature3D(usize x, usize y, usize z, usize xPoints, usize yPoints, usize zPoints, bool markFeature0Neighbors, const std::vector<int32>& prevSlice,
                              const std::vector<int32>& curSlice, const std::vector<int32>& nextSlice)
{
  // Boundary check: voxels on the outer faces of the volume are surface voxels
  if(x <= 0 || x >= xPoints - 1)
  {
    return true;
  }
  if(y <= 0 || y >= yPoints - 1)
  {
    return true;
  }
  if(z <= 0 || z >= zPoints - 1)
  {
    return true;
  }

  // Neighbor-zero check: test all 6 face neighbors for FeatureId == 0
  if(markFeature0Neighbors)
  {
    // Compute the flat index within the Z-slice buffer
    const usize inSlice = y * xPoints + x;

    // -X neighbor (one element back in the current row)
    if(curSlice[inSlice - 1] == 0)
    {
      return true;
    }
    // +X neighbor (one element forward in the current row)
    if(curSlice[inSlice + 1] == 0)
    {
      return true;
    }
    // -Y neighbor (one row back = -xPoints elements)
    if(curSlice[inSlice - xPoints] == 0)
    {
      return true;
    }
    // +Y neighbor (one row forward = +xPoints elements)
    if(curSlice[inSlice + xPoints] == 0)
    {
      return true;
    }
    // -Z neighbor (same position in the previous Z-slice buffer)
    if(prevSlice[inSlice] == 0)
    {
      return true;
    }
    // +Z neighbor (same position in the next Z-slice buffer)
    if(nextSlice[inSlice] == 0)
    {
      return true;
    }
  }

  return false;
}
} // namespace

// -----------------------------------------------------------------------------
ComputeSurfaceFeaturesScanline::ComputeSurfaceFeaturesScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                               const ComputeSurfaceFeaturesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeSurfaceFeaturesScanline::~ComputeSurfaceFeaturesScanline() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief Identifies surface features using a 3-slice rolling window with sequential
 * bulk I/O for out-of-core storage compatibility.
 *
 * The algorithm has four phases:
 *
 * **Phase 1 -- Validation and setup**: Validate FeatureId-to-AttributeMatrix
 * indexing, cache the small feature-level SurfaceFeatures array locally, and
 * compute remapped dimensions for 2D geometries.
 *
 * **Phase 2 -- Rolling window initialization**: Allocate three Z-slice buffers
 * and load the first one or two slices from the FeatureIds OOC store.
 *
 * **Phase 3 -- Z-slice iteration**: For each native Z-slice:
 *   - Iterate all voxels in Y-X order within curSlice.
 *   - For 3D geometries: call IsPointASurfaceFeature3D with the three buffers.
 *   - For 2D geometries: remap native (x, y, z) to the logical 2D plane and
 *     call IsPointASurfaceFeature2D.
 *   - Rotate the rolling window and load the next Z-slice.
 *
 * **Phase 4 -- Write-back**: Bulk-write the local SurfaceFeatures vector back
 * to the OOC store in a single copyFromBuffer() call.
 *
 * @return Result<> indicating success, validation errors, or unsupported dimensionality.
 */
Result<> ComputeSurfaceFeaturesScanline::operator()()
{
  // -- Phase 1: Validation and setup --

  // Extract input values into local variables for readability
  const auto pMarkFeature0NeighborsValue = m_InputValues->MarkFeature0Neighbors;
  const auto pFeatureGeometryPathValue = m_InputValues->InputImageGeometryPath;
  const auto pFeatureIdsArrayPathValue = m_InputValues->FeatureIdsPath;
  const auto pFeaturesAttributeMatrixPathValue = m_InputValues->FeatureAttributeMatrixPath;
  const auto pSurfaceFeaturesArrayPathValue = pFeaturesAttributeMatrixPathValue.createChildPath(m_InputValues->SurfaceFeaturesArrayName);

  // Validate that the max FeatureId does not exceed the AttributeMatrix size
  const auto& featureIdsArray = m_DataStructure.getDataRefAs<Int32Array>(pFeatureIdsArrayPathValue);
  auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, pFeaturesAttributeMatrixPathValue, featureIdsArray, false, m_MessageHandler);
  if(validateNumFeatResult.invalid())
  {
    return validateNumFeatResult;
  }

  const auto& featureGeometry = m_DataStructure.getDataRefAs<ImageGeom>(pFeatureGeometryPathValue);
  auto& featureIds = m_DataStructure.getDataAs<Int32Array>(pFeatureIdsArrayPathValue)->getDataStoreRef();
  auto& surfaceFeatures = m_DataStructure.getDataAs<UInt8Array>(pSurfaceFeaturesArrayPathValue)->getDataStoreRef();

  // Cache the small feature-level SurfaceFeatures array in a local vector.
  // This is critical because the inner Z-Y-X loop indexes into this array
  // by FeatureId (e.g., localSurfaceFeatures[gNum]). If the SurfaceFeatures
  // array were OOC, each of these lookups would be a random-access read/write
  // to a chunked store. By caching locally, we keep all feature-level access
  // in fast contiguous memory.
  const usize numFeatures = surfaceFeatures.getNumberOfTuples();
  std::vector<uint8> localSurfaceFeatures(numFeatures, 0);
  surfaceFeatures.copyIntoBuffer(0, nonstd::span<uint8>(localSurfaceFeatures.data(), numFeatures));

  const usize xPoints = featureGeometry.getNumXCells();
  const usize yPoints = featureGeometry.getNumYCells();
  const usize zPoints = featureGeometry.getNumZCells();
  const usize geometryDimensionality = featureGeometry.getDimensionality();

  // For 2D geometries, determine the remapped dimensions.
  // The degenerate dimension (size == 1) is collapsed, and the remaining two
  // dimensions become remappedXPoints and remappedYPoints. The remapping
  // determines how native (x, y, z) coordinates map to the 2D plane:
  //   - Degenerate X (xPoints==1): remappedX = native Y, remappedY = native Z
  //   - Degenerate Y (yPoints==1): remappedX = native X, remappedY = native Z
  //   - Degenerate Z (zPoints==1): remappedX = native X, remappedY = native Y
  usize remappedXPoints = 0;
  usize remappedYPoints = 0;
  if(geometryDimensionality == 2)
  {
    if(xPoints == 1)
    {
      remappedXPoints = yPoints;
      remappedYPoints = zPoints;
    }
    else if(yPoints == 1)
    {
      remappedXPoints = xPoints;
      remappedYPoints = zPoints;
    }
    else // zPoints == 1
    {
      remappedXPoints = xPoints;
      remappedYPoints = yPoints;
    }
  }

  // -- Phase 2: Rolling window initialization --

  // Each native Z-slice has yPoints * xPoints voxels. This is the granularity
  // of bulk I/O -- one copyIntoBuffer() call per Z-slice.
  const usize sliceSize = yPoints * xPoints;
  std::vector<int32> prevSlice(sliceSize, 0);
  std::vector<int32> curSlice(sliceSize, 0);
  std::vector<int32> nextSlice(sliceSize, 0);

  // Load the first native Z-slice into curSlice
  featureIds.copyIntoBuffer(0, nonstd::span<int32>(curSlice.data(), sliceSize));
  // Pre-load the second Z-slice if available
  if(zPoints > 1)
  {
    featureIds.copyIntoBuffer(sliceSize, nonstd::span<int32>(nextSlice.data(), sliceSize));
  }

  // -- Phase 3: Z-slice iteration with rolling window --

  for(usize z = 0; z < zPoints; z++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    for(usize y = 0; y < yPoints; y++)
    {
      for(usize x = 0; x < xPoints; x++)
      {
        // Compute the flat index within the current Z-slice buffer
        const usize inSlice = y * xPoints + x;
        const int32 gNum = curSlice[inSlice];

        // Skip feature 0 (background) and features already marked as surface.
        // The short-circuit on localSurfaceFeatures[gNum] avoids redundant
        // neighbor checks for features that have already been identified.
        if(gNum != 0 && !localSurfaceFeatures[gNum])
        {
          if(geometryDimensionality == 3)
          {
            // 3D: Check boundary position and 6 face neighbors via the
            // three rolling-window buffers
            if(IsPointASurfaceFeature3D(x, y, z, xPoints, yPoints, zPoints, pMarkFeature0NeighborsValue, prevSlice, curSlice, nextSlice))
            {
              localSurfaceFeatures[gNum] = 1;
            }
          }
          else if(geometryDimensionality == 2)
          {
            // 2D: Remap native 3D coordinates (x, y, z) to the logical 2D
            // plane based on which dimension is degenerate.
            usize remappedX = 0;
            usize remappedY = 0;
            bool degenerateZ = false;
            if(xPoints == 1)
            {
              // Degenerate X: the YZ plane is the 2D plane
              remappedX = y;
              remappedY = z;
            }
            else if(yPoints == 1)
            {
              // Degenerate Y: the XZ plane is the 2D plane
              remappedX = x;
              remappedY = z;
            }
            else // zPoints == 1
            {
              // Degenerate Z: the XY plane is the 2D plane (most common case)
              remappedX = x;
              remappedY = y;
              degenerateZ = true;
            }

            if(IsPointASurfaceFeature2D(remappedX, remappedY, remappedXPoints, remappedYPoints, pMarkFeature0NeighborsValue, curSlice, prevSlice, nextSlice, inSlice, z > 0, z + 1 < zPoints,
                                        degenerateZ))
            {
              localSurfaceFeatures[gNum] = 1;
            }
          }
          else
          {
            return MakeErrorResult(-1000, fmt::format("Image Geometry at path '{}' must be either 3D or 2D", pFeatureGeometryPathValue.toString()));
          }
        }
      }
    }

    // Rotate the rolling window: prevSlice <- curSlice <- nextSlice.
    // std::swap is O(1) for vectors (pointer swap only, no data copy).
    std::swap(prevSlice, curSlice);
    std::swap(curSlice, nextSlice);
    // Load the next-next Z-slice into the freed buffer
    if(z + 2 < zPoints)
    {
      featureIds.copyIntoBuffer((z + 2) * sliceSize, nonstd::span<int32>(nextSlice.data(), sliceSize));
    }
  }

  // -- Phase 4: Write-back --
  // Bulk-write the locally cached SurfaceFeatures results back to the OOC
  // store. This is a single sequential write of the entire feature-level array.
  surfaceFeatures.copyFromBuffer(0, nonstd::span<const uint8>(localSurfaceFeatures.data(), numFeatures));

  return {};
}
