#include "ComputeBoundaryCellsScanline.hpp"

#include "ComputeBoundaryCells.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include <fmt/format.h>
#include <nonstd/span.hpp>

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeBoundaryCellsScanline::ComputeBoundaryCellsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                           const ComputeBoundaryCellsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeBoundaryCellsScanline::~ComputeBoundaryCellsScanline() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief Counts boundary faces per voxel using Z-slice rolling window iteration.
 * OOC path: reads/writes one Z-slice at a time via copyIntoBuffer/copyFromBuffer,
 * keeping a 3-slice rolling window (prevSlice, curSlice, nextSlice) for Z-neighbor access.
 */
Result<> ComputeBoundaryCellsScanline::operator()()
{
  const auto& imageGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const SizeVec3 udims = imageGeometry.getDimensions();
  const int64 dimX = static_cast<int64>(udims[0]);
  const int64 dimY = static_cast<int64>(udims[1]);
  const int64 dimZ = static_cast<int64>(udims[2]);

  auto& featureIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();
  auto& boundaryCellsStore = m_DataStructure.getDataAs<Int8Array>(m_InputValues->BoundaryCellsArrayName)->getDataStoreRef();

  int32 ignoreFeatureZeroVal = 0;
  if(!m_InputValues->IgnoreFeatureZero)
  {
    ignoreFeatureZeroVal = -1;
  }

  const usize sliceSize = static_cast<usize>(dimY) * static_cast<usize>(dimX);

  std::vector<int32> prevSlice(sliceSize);
  std::vector<int32> curSlice(sliceSize);
  std::vector<int32> nextSlice(sliceSize);
  std::vector<int8> outputSlice(sliceSize);

  // Load first Z-slice
  featureIdsStore.copyIntoBuffer(0, nonstd::span<int32>(curSlice.data(), sliceSize));
  if(dimZ > 1)
  {
    featureIdsStore.copyIntoBuffer(sliceSize, nonstd::span<int32>(nextSlice.data(), sliceSize));
  }

  for(int64 zIdx = 0; zIdx < dimZ; zIdx++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    // Process current slice
    for(int64 yIdx = 0; yIdx < dimY; yIdx++)
    {
      const int64 rowOffset = yIdx * dimX;
      for(int64 xIdx = 0; xIdx < dimX; xIdx++)
      {
        const int64 sliceIndex = rowOffset + xIdx;
        int8 onSurf = 0;
        const int32 feature = curSlice[sliceIndex];

        if(feature >= 0)
        {
          if(m_InputValues->IncludeVolumeBoundary)
          {
            if(dimX > 2 && (xIdx == 0 || xIdx == dimX - 1))
            {
              onSurf++;
            }
            if(dimY > 2 && (yIdx == 0 || yIdx == dimY - 1))
            {
              onSurf++;
            }
            if(dimZ > 2 && (zIdx == 0 || zIdx == dimZ - 1))
            {
              onSurf++;
            }

            if(onSurf > 0 && feature == 0)
            {
              onSurf = 0;
            }
          }

          // -Z neighbor
          if(zIdx > 0)
          {
            if(prevSlice[sliceIndex] != feature && prevSlice[sliceIndex] > ignoreFeatureZeroVal)
            {
              onSurf++;
            }
          }

          // -Y neighbor
          if(yIdx > 0)
          {
            const int64 neighborIdx = sliceIndex - dimX;
            if(curSlice[neighborIdx] != feature && curSlice[neighborIdx] > ignoreFeatureZeroVal)
            {
              onSurf++;
            }
          }

          // -X neighbor
          if(xIdx > 0)
          {
            const int64 neighborIdx = sliceIndex - 1;
            if(curSlice[neighborIdx] != feature && curSlice[neighborIdx] > ignoreFeatureZeroVal)
            {
              onSurf++;
            }
          }

          // +X neighbor
          if(xIdx < dimX - 1)
          {
            const int64 neighborIdx = sliceIndex + 1;
            if(curSlice[neighborIdx] != feature && curSlice[neighborIdx] > ignoreFeatureZeroVal)
            {
              onSurf++;
            }
          }

          // +Y neighbor
          if(yIdx < dimY - 1)
          {
            const int64 neighborIdx = sliceIndex + dimX;
            if(curSlice[neighborIdx] != feature && curSlice[neighborIdx] > ignoreFeatureZeroVal)
            {
              onSurf++;
            }
          }

          // +Z neighbor
          if(zIdx < dimZ - 1)
          {
            if(nextSlice[sliceIndex] != feature && nextSlice[sliceIndex] > ignoreFeatureZeroVal)
            {
              onSurf++;
            }
          }
        }

        outputSlice[sliceIndex] = onSurf;
      }
    }

    // Write output slice
    boundaryCellsStore.copyFromBuffer(static_cast<usize>(zIdx) * sliceSize, nonstd::span<const int8>(outputSlice.data(), sliceSize));

    // Shift rolling window
    std::swap(prevSlice, curSlice);
    std::swap(curSlice, nextSlice);
    if(zIdx + 2 < dimZ)
    {
      featureIdsStore.copyIntoBuffer(static_cast<usize>(zIdx + 2) * sliceSize, nonstd::span<int32>(nextSlice.data(), sliceSize));
    }
  }

  return {};
}
