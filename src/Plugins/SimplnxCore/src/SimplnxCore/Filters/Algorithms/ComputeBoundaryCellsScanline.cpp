#include "ComputeBoundaryCellsScanline.hpp"

#include "ComputeBoundaryCells.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include <fmt/format.h>
#include <nonstd/span.hpp>

using namespace nx::core;

// ----------------------------------------------------------------------------
// ComputeBoundaryCellsScanline -- Out-of-Core Algorithm
//
// This file implements the OOC-optimized variant of boundary cell counting.
// The algorithm produces the same output as ComputeBoundaryCellsDirect: for
// each voxel, an Int8 count of how many of its 6 face neighbors belong to a
// different feature.
//
// KEY DESIGN PRINCIPLE: All data access is strictly sequential by Z-slice.
// The FeatureIds input is read one Z-slice at a time using copyIntoBuffer(),
// and the BoundaryCells output is written one Z-slice at a time using
// copyFromBuffer(). This ensures that chunked/OOC storage backends serve
// data in large sequential reads rather than random single-element lookups.
//
// ROLLING WINDOW APPROACH:
// To check the -Z and +Z neighbors of a voxel at slice z, we need data from
// slices z-1 and z+1. Rather than re-reading slices, we maintain 3 buffers:
//
//   prevSlice  = FeatureIds for Z-slice (z-1)
//   curSlice   = FeatureIds for Z-slice (z)    <-- being processed
//   nextSlice  = FeatureIds for Z-slice (z+1)
//
// After processing slice z, we rotate the buffers (via std::swap, which is
// O(1) for vectors -- just swaps internal pointers) and load slice z+2 into
// the now-free buffer. This means each Z-slice is read from disk exactly once.
//
// Within a Z-slice, the X and Y neighbor lookups use simple index arithmetic
// on curSlice: +/-1 for X neighbors, +/-dimX for Y neighbors. These are all
// in the same contiguous buffer, so there is no I/O cost.
//
// MEMORY BUDGET: 3 * (dimX * dimY * 4 bytes) for input buffers, plus
// 1 * (dimX * dimY * 1 byte) for the output buffer. For a 1024x1024 slice,
// this is ~12.6 MB total -- negligible compared to the full volume.
// ----------------------------------------------------------------------------

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
 * @brief Counts boundary faces per voxel using a 3-slice rolling window with
 * sequential bulk I/O for out-of-core storage compatibility.
 *
 * The algorithm has three phases:
 *
 * **Phase 1 -- Initialization**: Allocate three input slice buffers
 * (prevSlice, curSlice, nextSlice) and one output slice buffer. Load the
 * first Z-slice into curSlice and (if the volume has more than one Z-slice)
 * the second Z-slice into nextSlice.
 *
 * **Phase 2 -- Z-slice iteration**: For each Z-slice:
 *   - Iterate all voxels in Y-X order within curSlice.
 *   - For each voxel, check volume-boundary contribution (if enabled).
 *   - Check 6 face neighbors:
 *     - -Z neighbor: read from prevSlice at the same (y, x) position.
 *     - +Z neighbor: read from nextSlice at the same (y, x) position.
 *     - -Y neighbor: read from curSlice at (sliceIndex - dimX).
 *     - +Y neighbor: read from curSlice at (sliceIndex + dimX).
 *     - -X neighbor: read from curSlice at (sliceIndex - 1).
 *     - +X neighbor: read from curSlice at (sliceIndex + 1).
 *   - Write the finished output slice to BoundaryCells via copyFromBuffer().
 *   - Rotate the rolling window: prevSlice <- curSlice <- nextSlice, then
 *     load the next-next Z-slice into the freed buffer.
 *
 * **Phase 3 -- Completion**: After all Z-slices are processed, the output
 * array contains the boundary cell counts for the entire volume.
 *
 * @return Result<> indicating success or cancellation.
 */
Result<> ComputeBoundaryCellsScanline::operator()()
{
  // -- Phase 1: Initialization --

  const auto& imageGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const SizeVec3 udims = imageGeometry.getDimensions();
  const int64 dimX = static_cast<int64>(udims[0]);
  const int64 dimY = static_cast<int64>(udims[1]);
  const int64 dimZ = static_cast<int64>(udims[2]);

  // Access the data stores via references. Even though these may be OOC stores,
  // we never use operator[] on them -- only copyIntoBuffer/copyFromBuffer.
  auto& featureIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();
  auto& boundaryCellsStore = m_DataStructure.getDataAs<Int8Array>(m_InputValues->BoundaryCellsArrayName)->getDataStoreRef();

  // See ComputeBoundaryCellsDirect for explanation of ignoreFeatureZeroVal logic.
  int32 ignoreFeatureZeroVal = 0;
  if(!m_InputValues->IgnoreFeatureZero)
  {
    ignoreFeatureZeroVal = -1;
  }

  // Each Z-slice contains dimY * dimX voxels. This is the unit of bulk I/O.
  const usize sliceSize = static_cast<usize>(dimY) * static_cast<usize>(dimX);

  // Allocate the 3-slice rolling window for FeatureIds input and 1 output buffer.
  // Using std::vector ensures the buffers are contiguous and compatible with
  // nonstd::span for the copyIntoBuffer/copyFromBuffer API.
  std::vector<int32> prevSlice(sliceSize);
  std::vector<int32> curSlice(sliceSize);
  std::vector<int32> nextSlice(sliceSize);
  std::vector<int8> outputSlice(sliceSize);

  // Load the first Z-slice (z=0) into curSlice. The offset is 0 (start of array).
  featureIdsStore.copyIntoBuffer(0, nonstd::span<int32>(curSlice.data(), sliceSize));
  // If there is a second Z-slice, pre-load it into nextSlice. The offset is
  // sliceSize elements (one full Z-slice into the flat array).
  if(dimZ > 1)
  {
    featureIdsStore.copyIntoBuffer(sliceSize, nonstd::span<int32>(nextSlice.data(), sliceSize));
  }

  // -- Phase 2: Z-slice iteration with rolling window --

  for(int64 zIdx = 0; zIdx < dimZ; zIdx++)
  {
    // Check for user cancellation once per Z-slice
    if(m_ShouldCancel)
    {
      return {};
    }

    // Process every voxel in the current Z-slice
    for(int64 yIdx = 0; yIdx < dimY; yIdx++)
    {
      const int64 rowOffset = yIdx * dimX;
      for(int64 xIdx = 0; xIdx < dimX; xIdx++)
      {
        // sliceIndex is the flat index within the current Z-slice buffer.
        // This is NOT the global voxel index -- it ranges from 0 to sliceSize-1.
        const int64 sliceIndex = rowOffset + xIdx;
        int8 onSurf = 0;
        const int32 feature = curSlice[sliceIndex];

        if(feature >= 0)
        {
          // Volume boundary contribution -- same logic as the Direct variant
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

          // -Z neighbor: This voxel's -Z neighbor is the same (y, x) position
          // in the PREVIOUS Z-slice. Because prevSlice is an in-memory buffer,
          // this lookup is a simple array index -- no disk I/O.
          if(zIdx > 0)
          {
            if(prevSlice[sliceIndex] != feature && prevSlice[sliceIndex] > ignoreFeatureZeroVal)
            {
              onSurf++;
            }
          }

          // -Y neighbor: One row back in the current slice. The offset is
          // -dimX because each row has dimX elements.
          if(yIdx > 0)
          {
            const int64 neighborIdx = sliceIndex - dimX;
            if(curSlice[neighborIdx] != feature && curSlice[neighborIdx] > ignoreFeatureZeroVal)
            {
              onSurf++;
            }
          }

          // -X neighbor: One column back in the current row.
          if(xIdx > 0)
          {
            const int64 neighborIdx = sliceIndex - 1;
            if(curSlice[neighborIdx] != feature && curSlice[neighborIdx] > ignoreFeatureZeroVal)
            {
              onSurf++;
            }
          }

          // +X neighbor: One column forward in the current row.
          if(xIdx < dimX - 1)
          {
            const int64 neighborIdx = sliceIndex + 1;
            if(curSlice[neighborIdx] != feature && curSlice[neighborIdx] > ignoreFeatureZeroVal)
            {
              onSurf++;
            }
          }

          // +Y neighbor: One row forward in the current slice.
          if(yIdx < dimY - 1)
          {
            const int64 neighborIdx = sliceIndex + dimX;
            if(curSlice[neighborIdx] != feature && curSlice[neighborIdx] > ignoreFeatureZeroVal)
            {
              onSurf++;
            }
          }

          // +Z neighbor: This voxel's +Z neighbor is the same (y, x) position
          // in the NEXT Z-slice buffer -- again, a simple in-memory lookup.
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

    // Write the completed output Z-slice to the BoundaryCells array.
    // The offset is zIdx * sliceSize elements into the flat output array.
    boundaryCellsStore.copyFromBuffer(static_cast<usize>(zIdx) * sliceSize, nonstd::span<const int8>(outputSlice.data(), sliceSize));

    // Rotate the rolling window for the next iteration:
    //   prevSlice gets the old curSlice data (now z-1 relative to the next iteration)
    //   curSlice  gets the old nextSlice data (now z relative to the next iteration)
    //   nextSlice buffer is now free to receive new data
    // std::swap on vectors is O(1) -- it just swaps internal pointers, not data.
    std::swap(prevSlice, curSlice);
    std::swap(curSlice, nextSlice);

    // Load the next-next Z-slice (z+2) into the freed nextSlice buffer.
    // This is the only disk read per iteration -- one sequential bulk read.
    if(zIdx + 2 < dimZ)
    {
      featureIdsStore.copyIntoBuffer(static_cast<usize>(zIdx + 2) * sliceSize, nonstd::span<int32>(nextSlice.data(), sliceSize));
    }
  }

  // -- Phase 3: Done --
  // All Z-slices have been processed and written. The BoundaryCells array now
  // contains the boundary face count for every voxel in the volume.
  return {};
}
