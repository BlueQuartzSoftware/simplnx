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
 * @brief Checks whether a voxel at (remappedX, remappedY) in a 2D geometry is
 * on the surface.  Operates on a pair of flat buffers that hold the current
 * remapped-Y row and its neighbours in the remapped-Y direction.
 *
 * For the degenerate-Z case (zPoints==1) the entire dataset is one Z-slice,
 * so prevRow/nextRow are unused for Z neighbours -- all four 2D neighbours
 * live in curSlice.
 *
 * For the degenerate-X or degenerate-Y cases, the remapped-Y direction maps
 * to the native Z axis, so prevRow/nextRow come from prevSlice/nextSlice.
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
      // All 4 neighbours are in curSlice (the single Z-slice holds everything)
      // For degenerate Z: remapped coords map directly to native Y*xPoints+X layout
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
      // Remapped Y direction maps to native Z, so +/-Y neighbours come
      // from prevSlice / nextSlice.  Remapped X neighbours are within
      // curSlice at stride ±1 from the native in-slice index (works for
      // both degenerate-X and degenerate-Y because the non-degenerate
      // in-plane dimension is always contiguous in memory).

      // -remappedX neighbour
      if(curSlice[nativeInSlice - 1] == 0)
      {
        return true;
      }
      // +remappedX neighbour
      if(curSlice[nativeInSlice + 1] == 0)
      {
        return true;
      }
      // -remappedY neighbour (previous Z-slice)
      if(hasPrevSlice && prevSlice[nativeInSlice] == 0)
      {
        return true;
      }
      // +remappedY neighbour (next Z-slice)
      if(hasNextSlice && nextSlice[nativeInSlice] == 0)
      {
        return true;
      }
    }
  }

  return false;
}

/**
 * @brief Checks whether a voxel at (x,y) within a Z-slice is on the surface
 * in 3D.  Uses the three-slice rolling window: prevSlice, curSlice, nextSlice.
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

    // -X
    if(curSlice[inSlice - 1] == 0)
    {
      return true;
    }
    // +X
    if(curSlice[inSlice + 1] == 0)
    {
      return true;
    }
    // -Y
    if(curSlice[inSlice - xPoints] == 0)
    {
      return true;
    }
    // +Y
    if(curSlice[inSlice + xPoints] == 0)
    {
      return true;
    }
    // -Z (previous slice)
    if(prevSlice[inSlice] == 0)
    {
      return true;
    }
    // +Z (next slice)
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
 * @brief Identifies surface features using Z-slice bulk I/O with a rolling window.
 * OOC path: reads featureIds one Z-slice at a time via copyIntoBuffer(),
 * keeping 3 slices resident (prev/cur/next) for cross-slice neighbour access.
 * Handles both 3D and 2D geometries with coordinate remapping.
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

  // Cache the small feature-level surfaceFeatures array locally to avoid
  // per-voxel OOC operator[] in the hot Z-Y-X loop.
  const usize numFeatures = surfaceFeatures.getNumberOfTuples();
  std::vector<uint8> localSurfaceFeatures(numFeatures, 0);
  surfaceFeatures.copyIntoBuffer(0, nonstd::span<uint8>(localSurfaceFeatures.data(), numFeatures));

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

  // Z-slice rolling window for bulk I/O
  const usize sliceSize = yPoints * xPoints;
  std::vector<int32> prevSlice(sliceSize, 0);
  std::vector<int32> curSlice(sliceSize, 0);
  std::vector<int32> nextSlice(sliceSize, 0);

  // Load initial slices
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
            // Remap native 3D coordinates to 2D based on degenerate dimension
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

    // Shift the rolling window
    std::swap(prevSlice, curSlice);
    std::swap(curSlice, nextSlice);
    if(z + 2 < zPoints)
    {
      featureIds.copyIntoBuffer((z + 2) * sliceSize, nonstd::span<int32>(nextSlice.data(), sliceSize));
    }
  }

  // Write cached results back to the OOC store
  surfaceFeatures.copyFromBuffer(0, nonstd::span<const uint8>(localSurfaceFeatures.data(), numFeatures));

  return {};
}
