#include "ComputeSurfaceFeatures.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

using namespace nx::core;

namespace
{
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

void findSurfaceFeatures3D(DataStructure& dataStructure, const DataPath& featureGeometryPathValue, const DataPath& featureIdsArrayPathValue, const DataPath& surfaceFeaturesArrayPathValue,
                           bool markFeature0Neighbors, const std::atomic_bool& shouldCancel)
{
  const auto& featureGeometry = dataStructure.getDataRefAs<ImageGeom>(featureGeometryPathValue);
  const auto& featureIds = dataStructure.getDataAs<Int32Array>(featureIdsArrayPathValue)->getDataStoreRef();
  auto& surfaceFeatures = dataStructure.getDataAs<UInt8Array>(surfaceFeaturesArrayPathValue)->getDataStoreRef();

  const usize xPoints = featureGeometry.getNumXCells();
  const usize yPoints = featureGeometry.getNumYCells();
  const usize zPoints = featureGeometry.getNumZCells();

  for(usize z = 0; z < zPoints; z++)
  {
    const usize zStride = z * xPoints * yPoints;
    for(usize y = 0; y < yPoints; y++)
    {
      const usize yStride = y * xPoints;
      for(usize x = 0; x < xPoints; x++)
      {
        if(shouldCancel)
        {
          return;
        }

        const int32 gNum = featureIds[zStride + yStride + x];
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

void findSurfaceFeatures2D(DataStructure& dataStructure, const DataPath& featureGeometryPathValue, const DataPath& featureIdsArrayPathValue, const DataPath& surfaceFeaturesArrayPathValue,
                           bool markFeature0Neighbors, const std::atomic_bool& shouldCancel)
{
  const auto& featureGeometry = dataStructure.getDataRefAs<ImageGeom>(featureGeometryPathValue);
  const auto& featureIds = dataStructure.getDataAs<Int32Array>(featureIdsArrayPathValue)->getDataStoreRef();
  auto& surfaceFeatures = dataStructure.getDataAs<UInt8Array>(surfaceFeaturesArrayPathValue)->getDataStoreRef();

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
    const usize yStride = y * xPoints;

    for(usize x = 0; x < xPoints; x++)
    {
      if(shouldCancel)
      {
        return;
      }

      const int32 gNum = featureIds[yStride + x];
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
ComputeSurfaceFeatures::ComputeSurfaceFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               ComputeSurfaceFeaturesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeSurfaceFeatures::~ComputeSurfaceFeatures() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeSurfaceFeatures::operator()()
{

  const auto pMarkFeature0NeighborsValue = m_InputValues->MarkFeature0Neighbors;
  const auto pFeatureGeometryPathValue = m_InputValues->InputImageGeometryPath;
  const auto pFeatureIdsArrayPathValue = m_InputValues->FeatureIdsPath;
  const auto pFeaturesAttributeMatrixPathValue = m_InputValues->FeatureAttributeMatrixPath;
  const auto pSurfaceFeaturesArrayPathValue = pFeaturesAttributeMatrixPathValue.createChildPath(m_InputValues->SurfaceFeaturesArrayName);

  // Resize the surface features array to the proper size
  const auto& featureIdsArray = m_DataStructure.getDataRefAs<Int32Array>(pFeatureIdsArrayPathValue);

  auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, pFeaturesAttributeMatrixPathValue, featureIdsArray, false, m_MessageHandler);
  if(validateNumFeatResult.invalid())
  {
    return validateNumFeatResult;
  }

  // Find surface features
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
    MakeErrorResult(-1000, fmt::format("Image Geometry at path '{}' must be either 3D or 2D", pFeatureGeometryPathValue.toString()));
  }

  return {};
}
