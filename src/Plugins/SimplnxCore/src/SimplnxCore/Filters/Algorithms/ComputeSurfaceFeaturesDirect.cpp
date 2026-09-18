#include "ComputeSurfaceFeaturesDirect.hpp"

#include "ComputeSurfaceFeatures.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

using namespace nx::core;

namespace
{
/**
 * @brief Tests one voxel in a remapped 2D geometry.
 * @param point Specifies the remapped voxel coordinates.
 * @param xPoints Specifies the remapped X dimension.
 * @param yPoints Specifies the remapped Y dimension.
 * @param markFeature0Neighbors Enables Feature Id 0 neighbor checks.
 * @param featureIds Provides in-memory Feature Id values.
 * @return True when the voxel marks its feature as surface.
 */
bool IsPointASurfaceFeature(const Point2D<usize>& point, usize xPoints, usize yPoints, bool markFeature0Neighbors, const Int32AbstractDataStore& featureIds)
{
  const usize yStride = point.getY() * xPoints;

  if(point.getX() <= 0 || point.getX() >= xPoints - 1)
  {
    return true;
  }
  if(point.getY() <= 0 || point.getY() >= yPoints - 1)
  {
    return true;
  }

  if(markFeature0Neighbors)
  {
    if(featureIds[yStride + point.getX() - 1] == 0)
    {
      return true;
    }
    if(featureIds[yStride + point.getX() + 1] == 0)
    {
      return true;
    }
    if(featureIds[yStride + point.getX() - xPoints] == 0)
    {
      return true;
    }
    if(featureIds[yStride + point.getX() + xPoints] == 0)
    {
      return true;
    }
  }

  return false;
}

/**
 * @brief Tests one voxel in a 3D geometry.
 * @param point Specifies the voxel coordinates.
 * @param xPoints Specifies the X dimension.
 * @param yPoints Specifies the Y dimension.
 * @param zPoints Specifies the Z dimension.
 * @param markFeature0Neighbors Enables Feature Id 0 neighbor checks.
 * @param featureIds Provides in-memory Feature Id values.
 * @return True when the voxel marks its feature as surface.
 */
bool IsPointASurfaceFeature(const Point3D<usize>& point, usize xPoints, usize yPoints, usize zPoints, bool markFeature0Neighbors, const Int32AbstractDataStore& featureIds)
{
  usize yStride = point.getY() * xPoints;
  usize zStride = point.getZ() * xPoints * yPoints;

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

  if(markFeature0Neighbors)
  {
    if(featureIds[zStride + yStride + point.getX() - 1] == 0)
    {
      return true;
    }
    if(featureIds[zStride + yStride + point.getX() + 1] == 0)
    {
      return true;
    }
    if(featureIds[zStride + yStride + point.getX() - xPoints] == 0)
    {
      return true;
    }
    if(featureIds[zStride + yStride + point.getX() + xPoints] == 0)
    {
      return true;
    }
    if(featureIds[zStride + yStride + point.getX() - (xPoints * yPoints)] == 0)
    {
      return true;
    }
    if(featureIds[zStride + yStride + point.getX() + (xPoints * yPoints)] == 0)
    {
      return true;
    }
  }

  return false;
}

/**
 * @brief Labels surface features in a 3D image geometry.
 * @param dataStructure Provides the selected arrays and geometry.
 * @param featureGeometryPathValue Identifies the image geometry.
 * @param featureIdsArrayPathValue Identifies the cell Feature Id array.
 * @param surfaceFeaturesArrayPathValue Identifies the feature output array.
 * @param markFeature0Neighbors Enables Feature Id 0 neighbor checks.
 * @param shouldCancel Stops later Z slices when true.
 *
 * A label short-circuits later voxels of the same feature.
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
        // Feature 0 is background. A marked feature needs no further checks.
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
 * @brief Labels surface features in a remapped 2D image geometry.
 * @param dataStructure Provides the selected arrays and geometry.
 * @param featureGeometryPathValue Identifies the image geometry.
 * @param featureIdsArrayPathValue Identifies the cell Feature Id array.
 * @param surfaceFeaturesArrayPathValue Identifies the feature output array.
 * @param markFeature0Neighbors Enables Feature Id 0 neighbor checks.
 * @param shouldCancel Stops later rows when true.
 *
 * The dimension with one cell is removed before the 2D neighbor test.
 */
void findSurfaceFeatures2D(DataStructure& dataStructure, const DataPath& featureGeometryPathValue, const DataPath& featureIdsArrayPathValue, const DataPath& surfaceFeaturesArrayPathValue,
                           bool markFeature0Neighbors, const std::atomic_bool& shouldCancel)
{
  const auto& featureGeometry = dataStructure.getDataRefAs<ImageGeom>(featureGeometryPathValue);
  const auto& featureIds = dataStructure.getDataAs<Int32Array>(featureIdsArrayPathValue)->getDataStoreRef();
  auto& surfaceFeatures = dataStructure.getDataAs<UInt8Array>(surfaceFeaturesArrayPathValue)->getDataStoreRef();

  // Remove the dimension with one cell before using 2D flat indexes.
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
      // Feature 0 is background. A marked feature needs no further checks.
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

ComputeSurfaceFeaturesDirect::ComputeSurfaceFeaturesDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                           const ComputeSurfaceFeaturesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeSurfaceFeaturesDirect::~ComputeSurfaceFeaturesDirect() noexcept = default;

Result<> ComputeSurfaceFeaturesDirect::operator()()
{
  const auto pMarkFeature0NeighborsValue = m_InputValues->MarkFeature0Neighbors;
  const auto pFeatureGeometryPathValue = m_InputValues->InputImageGeometryPath;
  const auto pFeatureIdsArrayPathValue = m_InputValues->FeatureIdsPath;
  const auto pFeaturesAttributeMatrixPathValue = m_InputValues->FeatureAttributeMatrixPath;
  const auto pSurfaceFeaturesArrayPathValue = pFeaturesAttributeMatrixPathValue.createChildPath(m_InputValues->SurfaceFeaturesArrayName);

  const auto& featureIdsArray = m_DataStructure.getDataRefAs<Int32Array>(pFeatureIdsArrayPathValue);
  auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, pFeaturesAttributeMatrixPathValue, featureIdsArray, false, m_MessageHandler);
  if(validateNumFeatResult.invalid())
  {
    return validateNumFeatResult;
  }

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
