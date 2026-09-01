#include "ComputeSurfaceFeaturesScanline.hpp"

#include "ComputeSurfaceFeatures.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <fmt/format.h>

#include <nonstd/span.hpp>

using namespace nx::core;

namespace
{
/**
 * @brief Tests one voxel in a remapped 2D geometry.
 * @param remappedX Specifies the logical X coordinate.
 * @param remappedY Specifies the logical Y coordinate.
 * @param remappedXPoints Specifies the logical X dimension.
 * @param remappedYPoints Specifies the logical Y dimension.
 * @param markFeature0Neighbors Enables Feature Id 0 neighbor checks.
 * @param curSlice Provides the current native Z slice.
 * @param prevSlice Provides the previous native Z slice.
 * @param nextSlice Provides the next native Z slice.
 * @param nativeInSlice Specifies the flat index in the native Z slice.
 * @param hasPrevSlice Indicates whether a previous Z slice exists.
 * @param hasNextSlice Indicates whether a next Z slice exists.
 * @param degenerateZ Indicates whether the native Z dimension has one cell.
 * @return True when the voxel marks its feature as surface.
 *
 * Degenerate X/Y maps logical Y to native Z. Degenerate Z gets all four
 * neighbors from the current slice.
 */
bool IsPointASurfaceFeature2D(usize remappedX, usize remappedY, usize remappedXPoints, usize remappedYPoints, bool markFeature0Neighbors, const std::vector<int32>& curSlice,
                              const std::vector<int32>& prevSlice, const std::vector<int32>& nextSlice, usize nativeInSlice, bool hasPrevSlice, bool hasNextSlice, bool degenerateZ)
{
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
      const usize yStride = remappedY * remappedXPoints;
      if(curSlice[yStride + remappedX - 1] == 0)
      {
        return true;
      }
      if(curSlice[yStride + remappedX + 1] == 0)
      {
        return true;
      }
      if(curSlice[(remappedY - 1) * remappedXPoints + remappedX] == 0)
      {
        return true;
      }
      if(curSlice[(remappedY + 1) * remappedXPoints + remappedX] == 0)
      {
        return true;
      }
    }
    else
    {
      if(curSlice[nativeInSlice - 1] == 0)
      {
        return true;
      }
      if(curSlice[nativeInSlice + 1] == 0)
      {
        return true;
      }
      if(hasPrevSlice && prevSlice[nativeInSlice] == 0)
      {
        return true;
      }
      if(hasNextSlice && nextSlice[nativeInSlice] == 0)
      {
        return true;
      }
    }
  }

  return false;
}

/**
 * @brief Tests one voxel in a 3D geometry.
 * @param x Specifies the X coordinate.
 * @param y Specifies the Y coordinate.
 * @param z Specifies the Z coordinate.
 * @param xPoints Specifies the X dimension.
 * @param yPoints Specifies the Y dimension.
 * @param zPoints Specifies the Z dimension.
 * @param markFeature0Neighbors Enables Feature Id 0 neighbor checks.
 * @param prevSlice Provides the previous Z slice.
 * @param curSlice Provides the current Z slice.
 * @param nextSlice Provides the next Z slice.
 * @return True when the voxel marks its feature as surface.
 *
 * The buffers replace all six disk-backed neighbor reads.
 */
bool IsPointASurfaceFeature3D(usize x, usize y, usize z, usize xPoints, usize yPoints, usize zPoints, bool markFeature0Neighbors, const std::vector<int32>& prevSlice,
                              const std::vector<int32>& curSlice, const std::vector<int32>& nextSlice)
{
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
    const usize inSlice = y * xPoints + x;

    if(curSlice[inSlice - 1] == 0)
    {
      return true;
    }
    if(curSlice[inSlice + 1] == 0)
    {
      return true;
    }
    if(curSlice[inSlice - xPoints] == 0)
    {
      return true;
    }
    if(curSlice[inSlice + xPoints] == 0)
    {
      return true;
    }
    if(prevSlice[inSlice] == 0)
    {
      return true;
    }
    if(nextSlice[inSlice] == 0)
    {
      return true;
    }
  }

  return false;
}
} // namespace

ComputeSurfaceFeaturesScanline::ComputeSurfaceFeaturesScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                               const ComputeSurfaceFeaturesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeSurfaceFeaturesScanline::~ComputeSurfaceFeaturesScanline() noexcept = default;

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

  // The inner loop indexes labels by Feature Id, so a local cache prevents
  // repeated random output-store access.
  const usize numFeatures = surfaceFeatures.getNumberOfTuples();
  std::vector<uint8> localSurfaceFeatures(numFeatures, 0);
  // Current bulk-I/O Result values are not inspected. A failure can leave input
  // buffers or output labels incomplete while this method returns success.
  surfaceFeatures.copyIntoBuffer(0, nonstd::span<uint8>(localSurfaceFeatures.data(), numFeatures));

  const usize xPoints = featureGeometry.getNumXCells();
  const usize yPoints = featureGeometry.getNumYCells();
  const usize zPoints = featureGeometry.getNumZCells();
  const usize geometryDimensionality = featureGeometry.getDimensionality();

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

  const usize sliceSize = yPoints * xPoints;
  // Native Z slices keep bulk reads sequential when 2D X or Y is degenerate.
  std::vector<int32> prevSlice(sliceSize, 0);
  std::vector<int32> curSlice(sliceSize, 0);
  std::vector<int32> nextSlice(sliceSize, 0);

  featureIds.copyIntoBuffer(0, nonstd::span<int32>(curSlice.data(), sliceSize));
  if(zPoints > 1)
  {
    featureIds.copyIntoBuffer(sliceSize, nonstd::span<int32>(nextSlice.data(), sliceSize));
  }

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
        const usize inSlice = y * xPoints + x;
        const int32 gNum = curSlice[inSlice];

        // Feature 0 is background. Marked features need no further checks.
        if(gNum != 0 && !localSurfaceFeatures[gNum])
        {
          if(geometryDimensionality == 3)
          {
            if(IsPointASurfaceFeature3D(x, y, z, xPoints, yPoints, zPoints, pMarkFeature0NeighborsValue, prevSlice, curSlice, nextSlice))
            {
              localSurfaceFeatures[gNum] = 1;
            }
          }
          else if(geometryDimensionality == 2)
          {
            usize remappedX = 0;
            usize remappedY = 0;
            bool degenerateZ = false;
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

    // Vector swaps rotate the window without copying a slice.
    std::swap(prevSlice, curSlice);
    std::swap(curSlice, nextSlice);
    if(z + 2 < zPoints)
    {
      featureIds.copyIntoBuffer((z + 2) * sliceSize, nonstd::span<int32>(nextSlice.data(), sliceSize));
    }
  }

  surfaceFeatures.copyFromBuffer(0, nonstd::span<const uint8>(localSurfaceFeatures.data(), numFeatures));

  return {};
}
