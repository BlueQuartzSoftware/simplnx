#include "ComputeSurfaceFeaturesScanline.hpp"

#include "ComputeSurfaceFeatures.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

using namespace nx::core;

namespace
{
bool IsPointASurfaceFeature2D(usize remappedX, usize remappedY, usize xPoints, usize yPoints, bool markFeature0Neighbors, const Int32AbstractDataStore& featureIds)
{
  const usize yStride = remappedY * xPoints;

  if(remappedX <= 0 || remappedX >= xPoints - 1)
  {
    return true;
  }
  if(remappedY <= 0 || remappedY >= yPoints - 1)
  {
    return true;
  }

  if(markFeature0Neighbors)
  {
    if(featureIds[yStride + remappedX - 1] == 0)
    {
      return true;
    }
    if(featureIds[yStride + remappedX + 1] == 0)
    {
      return true;
    }
    if(featureIds[yStride + remappedX - xPoints] == 0)
    {
      return true;
    }
    if(featureIds[yStride + remappedX + xPoints] == 0)
    {
      return true;
    }
  }

  return false;
}

bool IsPointASurfaceFeature3D(usize x, usize y, usize z, usize xPoints, usize yPoints, usize zPoints, bool markFeature0Neighbors, const Int32AbstractDataStore& featureIds)
{
  const usize yStride = y * xPoints;
  const usize zStride = z * xPoints * yPoints;

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

  if(markFeature0Neighbors)
  {
    if(featureIds[zStride + yStride + x - 1] == 0)
    {
      return true;
    }
    if(featureIds[zStride + yStride + x + 1] == 0)
    {
      return true;
    }
    if(featureIds[zStride + yStride + x - xPoints] == 0)
    {
      return true;
    }
    if(featureIds[zStride + yStride + x + xPoints] == 0)
    {
      return true;
    }
    if(featureIds[zStride + yStride + x - (xPoints * yPoints)] == 0)
    {
      return true;
    }
    if(featureIds[zStride + yStride + x + (xPoints * yPoints)] == 0)
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
 * @brief Identifies surface features using chunk-sequential iteration.
 * OOC path: iterates chunks in order, handling both 3D and 2D geometries
 * with coordinate remapping. Same logic as ComputeSurfaceFeaturesDirect.
 */
Result<> ComputeSurfaceFeaturesScanline::operator()()
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
  auto& featureIds = m_DataStructure.getDataAs<Int32Array>(pFeatureIdsArrayPathValue)->getDataStoreRef();
  auto& surfaceFeatures = m_DataStructure.getDataAs<UInt8Array>(pSurfaceFeaturesArrayPathValue)->getDataStoreRef();

  const usize xPoints = featureGeometry.getNumXCells();
  const usize yPoints = featureGeometry.getNumYCells();
  const usize zPoints = featureGeometry.getNumZCells();
  const usize geometryDimensionality = featureGeometry.getDimensionality();

  // For 2D geometries, compute the remapped dimensions
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

  // Chunk-sequential iteration always uses 3D chunk bounds
  const uint64 numChunks = featureIds.getNumberOfChunks();

  for(uint64 chunkIdx = 0; chunkIdx < numChunks; chunkIdx++)
  {
    featureIds.loadChunk(chunkIdx);

    const auto chunkLowerBounds = featureIds.getChunkLowerBounds(chunkIdx);
    const auto chunkUpperBounds = featureIds.getChunkUpperBounds(chunkIdx);

    for(usize z = chunkLowerBounds[0]; z <= chunkUpperBounds[0]; z++)
    {
      const usize zStride = z * xPoints * yPoints;
      for(usize y = chunkLowerBounds[1]; y <= chunkUpperBounds[1]; y++)
      {
        const usize yStride = y * xPoints;
        for(usize x = chunkLowerBounds[2]; x <= chunkUpperBounds[2]; x++)
        {
          if(m_ShouldCancel)
          {
            return {};
          }

          const int32 gNum = featureIds[zStride + yStride + x];
          if(gNum != 0 && !surfaceFeatures[gNum])
          {
            if(geometryDimensionality == 3)
            {
              if(IsPointASurfaceFeature3D(x, y, z, xPoints, yPoints, zPoints, pMarkFeature0NeighborsValue, featureIds))
              {
                surfaceFeatures[gNum] = 1;
              }
            }
            else if(geometryDimensionality == 2)
            {
              // Remap native 3D coordinates to 2D based on degenerate dimension
              usize remappedX = 0;
              usize remappedY = 0;
              if(xPoints == 1)
              {
                remappedX = y;
                remappedY = z;
              }
              else if(yPoints == 1)
              {
                remappedX = x;
                remappedY = z;
              }
              else // zPoints == 1
              {
                remappedX = x;
                remappedY = y;
              }

              if(IsPointASurfaceFeature2D(remappedX, remappedY, remappedXPoints, remappedYPoints, pMarkFeature0NeighborsValue, featureIds))
              {
                surfaceFeatures[gNum] = 1;
              }
            }
            else
            {
              return MakeErrorResult(-1000, fmt::format("Image Geometry at path '{}' must be either 3D or 2D", pFeatureGeometryPathValue.toString()));
            }
          }
        }
      }
    }
  }

  return {};
}
