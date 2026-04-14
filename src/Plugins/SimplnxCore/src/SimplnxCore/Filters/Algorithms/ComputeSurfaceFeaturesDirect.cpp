#include "ComputeSurfaceFeaturesDirect.hpp"

#include "ComputeSurfaceFeatures.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

using namespace nx::core;

// ----------------------------------------------------------------------------
// ComputeSurfaceFeaturesDirect -- In-Core Algorithm
//
// Identifies which features touch the outer surface of the image geometry or
// border FeatureId==0 voxels. The output is a feature-level boolean (UInt8)
// array: 0 = interior feature, 1 = surface feature.
//
// Data access pattern: Uses operator[] on the FeatureIds DataStore for both
// the voxel under test and its face neighbors. This is efficient when the
// DataStore is contiguous in-memory, but would cause chunk thrashing on OOC
// storage because neighbor lookups span large index offsets (+/-dimX*dimY
// for Z neighbors).
//
// Two helper functions handle the dimensionality-specific logic:
//   - findSurfaceFeatures3D: Full 6-neighbor check for 3D geometries.
//   - findSurfaceFeatures2D: 4-neighbor check on the non-degenerate plane
//     for geometries where one dimension has size 1.
//
// Within each helper, two private IsPointASurfaceFeature overloads (2D/3D)
// encapsulate the boundary and neighbor-zero checks for a single voxel.
// ----------------------------------------------------------------------------

namespace
{
/**
 * @brief Checks whether a single voxel in a 2D geometry qualifies its owning
 * feature as a surface feature.
 *
 * A voxel is on the surface if:
 * - It sits on the outer boundary of the 2D plane (x or y == 0 or max).
 * - Any of its 4 face neighbors has FeatureId == 0 (when markFeature0Neighbors
 *   is true).
 *
 * @param point The (x, y) coordinates of the voxel in the 2D plane.
 * @param xPoints Number of cells in the remapped X dimension.
 * @param yPoints Number of cells in the remapped Y dimension.
 * @param markFeature0Neighbors Whether to check neighbors for FeatureId == 0.
 * @param featureIds The cell-level FeatureIds DataStore (accessed via operator[]).
 * @return true if the voxel makes its feature a surface feature.
 */
bool IsPointASurfaceFeature(const Point2D<usize>& point, usize xPoints, usize yPoints, bool markFeature0Neighbors, const Int32AbstractDataStore& featureIds)
{
  const usize yStride = point.getY() * xPoints;

  // Boundary check: voxels on the outer edge of the plane are always surface voxels
  if(point.getX() <= 0 || point.getX() >= xPoints - 1)
  {
    return true;
  }
  if(point.getY() <= 0 || point.getY() >= yPoints - 1)
  {
    return true;
  }

  // Neighbor-zero check: if any face neighbor has FeatureId == 0, the feature
  // is considered to touch the surface (feature 0 typically represents empty space)
  if(markFeature0Neighbors)
  {
    // -X neighbor
    if(featureIds[yStride + point.getX() - 1] == 0)
    {
      return true;
    }
    // +X neighbor
    if(featureIds[yStride + point.getX() + 1] == 0)
    {
      return true;
    }
    // -Y neighbor
    if(featureIds[yStride + point.getX() - xPoints] == 0)
    {
      return true;
    }
    // +Y neighbor
    if(featureIds[yStride + point.getX() + xPoints] == 0)
    {
      return true;
    }
  }

  return false;
}

/**
 * @brief Checks whether a single voxel in a 3D geometry qualifies its owning
 * feature as a surface feature.
 *
 * A voxel is on the surface if:
 * - It sits on the outer boundary of the volume (x, y, or z == 0 or max).
 * - Any of its 6 face neighbors has FeatureId == 0 (when markFeature0Neighbors
 *   is true).
 *
 * @param point The (x, y, z) coordinates of the voxel.
 * @param xPoints Number of cells in X.
 * @param yPoints Number of cells in Y.
 * @param zPoints Number of cells in Z.
 * @param markFeature0Neighbors Whether to check neighbors for FeatureId == 0.
 * @param featureIds The cell-level FeatureIds DataStore (accessed via operator[]).
 * @return true if the voxel makes its feature a surface feature.
 */
bool IsPointASurfaceFeature(const Point3D<usize>& point, usize xPoints, usize yPoints, usize zPoints, bool markFeature0Neighbors, const Int32AbstractDataStore& featureIds)
{
  usize yStride = point.getY() * xPoints;
  usize zStride = point.getZ() * xPoints * yPoints;

  // Boundary check: voxels on the outer faces of the volume are always surface voxels
  if(point.getX() <= 0 || point.getX() >= xPoints - 1)
  {
    return true;
  }
  if(point.getY() <= 0 || point.getY() >= yPoints - 1)
  {
    return true;
  }
  if(point.getZ() <= 0 || point.getZ() >= zPoints - 1)
  {
    return true;
  }

  // Neighbor-zero check: test all 6 face neighbors for FeatureId == 0
  if(markFeature0Neighbors)
  {
    // -X neighbor
    if(featureIds[zStride + yStride + point.getX() - 1] == 0)
    {
      return true;
    }
    // +X neighbor
    if(featureIds[zStride + yStride + point.getX() + 1] == 0)
    {
      return true;
    }
    // -Y neighbor (one row back = -xPoints in flat index)
    if(featureIds[zStride + yStride + point.getX() - xPoints] == 0)
    {
      return true;
    }
    // +Y neighbor (one row forward = +xPoints in flat index)
    if(featureIds[zStride + yStride + point.getX() + xPoints] == 0)
    {
      return true;
    }
    // -Z neighbor (one slice back = -(xPoints*yPoints) in flat index)
    if(featureIds[zStride + yStride + point.getX() - (xPoints * yPoints)] == 0)
    {
      return true;
    }
    // +Z neighbor (one slice forward = +(xPoints*yPoints) in flat index)
    if(featureIds[zStride + yStride + point.getX() + (xPoints * yPoints)] == 0)
    {
      return true;
    }
  }

  return false;
}

/**
 * @brief Identifies surface features in a 3D image geometry using direct array access.
 *
 * Iterates all voxels in Z-Y-X order. For each voxel with a non-zero FeatureId
 * whose feature has not already been marked, calls IsPointASurfaceFeature to
 * check boundary and neighbor-zero conditions. Once a feature is marked as
 * surface (surfaceFeatures[gNum] = 1), subsequent voxels of that feature are
 * skipped (short-circuit optimization).
 *
 * @param dataStructure The DataStructure containing all arrays.
 * @param featureGeometryPathValue Path to the ImageGeom.
 * @param featureIdsArrayPathValue Path to the FeatureIds cell array.
 * @param surfaceFeaturesArrayPathValue Path to the output SurfaceFeatures feature array.
 * @param markFeature0Neighbors Whether to treat FeatureId==0 neighbors as surface indicators.
 * @param shouldCancel Cancellation flag checked once per Z-slice.
 */
void findSurfaceFeatures3D(DataStructure& dataStructure, const DataPath& featureGeometryPathValue, const DataPath& featureIdsArrayPathValue, const DataPath& surfaceFeaturesArrayPathValue,
                           bool markFeature0Neighbors, const std::atomic_bool& shouldCancel)
{
  const auto& featureGeometry = dataStructure.getDataRefAs<ImageGeom>(featureGeometryPathValue);
  const auto& featureIds = dataStructure.getDataAs<Int32Array>(featureIdsArrayPathValue)->getDataStoreRef();
  auto& surfaceFeatures = dataStructure.getDataAs<UInt8Array>(surfaceFeaturesArrayPathValue)->getDataStoreRef();

  const usize xPoints = featureGeometry.getNumXCells();
  const usize yPoints = featureGeometry.getNumYCells();
  const usize zPoints = featureGeometry.getNumZCells();
  const usize totalSlices = zPoints;

  for(usize z = 0; z < zPoints; z++)
  {
    if(shouldCancel)
    {
      return;
    }
    const usize zStride = z * xPoints * yPoints;
    for(usize y = 0; y < yPoints; y++)
    {
      const usize yStride = y * xPoints;
      for(usize x = 0; x < xPoints; x++)
      {
        const int32 gNum = featureIds[zStride + yStride + x];
        // Skip feature 0 (background) and features already marked as surface
        if(gNum != 0 && !surfaceFeatures[gNum])
        {
          if(IsPointASurfaceFeature(Point3D<usize>{x, y, z}, xPoints, yPoints, zPoints, markFeature0Neighbors, featureIds))
          {
            surfaceFeatures[gNum] = 1;
          }
        }
      }
    }
  }
}

/**
 * @brief Identifies surface features in a 2D image geometry using direct array access.
 *
 * Determines which dimension is degenerate (size == 1) and remaps the remaining
 * two dimensions to a 2D (xPoints, yPoints) coordinate system. Then iterates
 * all voxels in the 2D plane, checking boundary and neighbor-zero conditions
 * with IsPointASurfaceFeature (2D overload).
 *
 * @param dataStructure The DataStructure containing all arrays.
 * @param featureGeometryPathValue Path to the ImageGeom.
 * @param featureIdsArrayPathValue Path to the FeatureIds cell array.
 * @param surfaceFeaturesArrayPathValue Path to the output SurfaceFeatures feature array.
 * @param markFeature0Neighbors Whether to treat FeatureId==0 neighbors as surface indicators.
 * @param shouldCancel Cancellation flag checked once per Y-row.
 */
void findSurfaceFeatures2D(DataStructure& dataStructure, const DataPath& featureGeometryPathValue, const DataPath& featureIdsArrayPathValue, const DataPath& surfaceFeaturesArrayPathValue,
                           bool markFeature0Neighbors, const std::atomic_bool& shouldCancel)
{
  const auto& featureGeometry = dataStructure.getDataRefAs<ImageGeom>(featureGeometryPathValue);
  const auto& featureIds = dataStructure.getDataAs<Int32Array>(featureIdsArrayPathValue)->getDataStoreRef();
  auto& surfaceFeatures = dataStructure.getDataAs<UInt8Array>(surfaceFeaturesArrayPathValue)->getDataStoreRef();

  // Determine which two dimensions form the non-degenerate plane.
  // The degenerate dimension (size == 1) is collapsed, and the remaining
  // two dimensions are remapped to xPoints and yPoints for the 2D algorithm.
  usize xPoints = 0;
  usize yPoints = 0;

  if(featureGeometry.getNumXCells() == 1)
  {
    xPoints = featureGeometry.getNumYCells();
    yPoints = featureGeometry.getNumZCells();
  }
  if(featureGeometry.getNumYCells() == 1)
  {
    xPoints = featureGeometry.getNumXCells();
    yPoints = featureGeometry.getNumZCells();
  }
  if(featureGeometry.getNumZCells() == 1)
  {
    xPoints = featureGeometry.getNumXCells();
    yPoints = featureGeometry.getNumYCells();
  }

  for(usize y = 0; y < yPoints; y++)
  {
    if(shouldCancel)
    {
      return;
    }
    const usize yStride = y * xPoints;

    for(usize x = 0; x < xPoints; x++)
    {
      const int32 gNum = featureIds[yStride + x];
      // Skip feature 0 (background) and features already marked
      if(gNum != 0 && surfaceFeatures[gNum] == 0)
      {
        if(IsPointASurfaceFeature(Point2D<usize>{x, y}, xPoints, yPoints, markFeature0Neighbors, featureIds))
        {
          surfaceFeatures[gNum] = 1;
        }
      }
    }
  }
}
} // namespace

// -----------------------------------------------------------------------------
ComputeSurfaceFeaturesDirect::ComputeSurfaceFeaturesDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                           const ComputeSurfaceFeaturesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeSurfaceFeaturesDirect::~ComputeSurfaceFeaturesDirect() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief Identifies surface features using direct in-memory array indexing.
 *
 * The algorithm proceeds as follows:
 * 1. Build the output array path from the FeatureAttributeMatrix and output name.
 * 2. Validate that the FeatureIds values are consistent with the AttributeMatrix
 *    tuple count (catches mismatches that would cause out-of-bounds writes).
 * 3. Query the image geometry dimensionality:
 *    - 3D: delegate to findSurfaceFeatures3D (6-neighbor check).
 *    - 2D: delegate to findSurfaceFeatures2D (4-neighbor check on the
 *      non-degenerate plane).
 *    - Other: return an error.
 *
 * @return Result<> indicating success, validation errors, or unsupported geometry.
 */
Result<> ComputeSurfaceFeaturesDirect::operator()()
{
  // Extract input values into local variables for readability
  const auto pMarkFeature0NeighborsValue = m_InputValues->MarkFeature0Neighbors;
  const auto pFeatureGeometryPathValue = m_InputValues->InputImageGeometryPath;
  const auto pFeatureIdsArrayPathValue = m_InputValues->FeatureIdsPath;
  const auto pFeaturesAttributeMatrixPathValue = m_InputValues->FeatureAttributeMatrixPath;
  // The output SurfaceFeatures array lives under the Feature AttributeMatrix
  const auto pSurfaceFeaturesArrayPathValue = pFeaturesAttributeMatrixPathValue.createChildPath(m_InputValues->SurfaceFeaturesArrayName);

  // Validate that the max FeatureId does not exceed the AttributeMatrix size
  const auto& featureIdsArray = m_DataStructure.getDataRefAs<Int32Array>(pFeatureIdsArrayPathValue);
  auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, pFeaturesAttributeMatrixPathValue, featureIdsArray, false, m_MessageHandler);
  if(validateNumFeatResult.invalid())
  {
    return validateNumFeatResult;
  }

  // Branch on geometry dimensionality
  const auto& featureGeometry = m_DataStructure.getDataRefAs<ImageGeom>(pFeatureGeometryPathValue);
  if(const usize geometryDimensionality = featureGeometry.getDimensionality(); geometryDimensionality == 3)
  {
    findSurfaceFeatures3D(m_DataStructure, pFeatureGeometryPathValue, pFeatureIdsArrayPathValue, pSurfaceFeaturesArrayPathValue, pMarkFeature0NeighborsValue, m_ShouldCancel);
  }
  else if(geometryDimensionality == 2)
  {
    findSurfaceFeatures2D(m_DataStructure, pFeatureGeometryPathValue, pFeatureIdsArrayPathValue, pSurfaceFeaturesArrayPathValue, pMarkFeature0NeighborsValue, m_ShouldCancel);
  }
  else
  {
    return MakeErrorResult(-1000, fmt::format("Image Geometry at path '{}' must be either 3D or 2D", pFeatureGeometryPathValue.toString()));
  }

  return {};
}
