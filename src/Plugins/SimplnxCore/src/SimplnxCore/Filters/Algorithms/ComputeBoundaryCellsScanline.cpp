#include "ComputeBoundaryCellsScanline.hpp"

#include "ComputeBoundaryCells.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include <fmt/format.h>
#include <nonstd/span.hpp>

using namespace nx::core;

ComputeBoundaryCellsScanline::ComputeBoundaryCellsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                           const ComputeBoundaryCellsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeBoundaryCellsScanline::~ComputeBoundaryCellsScanline() noexcept = default;

Result<> ComputeBoundaryCellsScanline::operator()()
{
  const auto& imageGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const SizeVec3 udims = imageGeometry.getDimensions();
  const int64 dimX = static_cast<int64>(udims[0]);
  const int64 dimY = static_cast<int64>(udims[1]);
  const int64 dimZ = static_cast<int64>(udims[2]);

  auto& featureIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();
  auto& boundaryCellsStore = m_DataStructure.getDataAs<Int8Array>(m_InputValues->BoundaryCellsArrayName)->getDataStoreRef();

  // The lower bound excludes feature zero only when requested.
  int32 ignoreFeatureZeroVal = 0;
  if(!m_InputValues->IgnoreFeatureZero)
  {
    ignoreFeatureZeroVal = -1;
  }

  const usize sliceSize = static_cast<usize>(dimY) * static_cast<usize>(dimX);

  // Keep previous, current, and next inputs plus one output slice.
  std::vector<int32> prevSlice(sliceSize);
  std::vector<int32> curSlice(sliceSize);
  std::vector<int32> nextSlice(sliceSize);
  std::vector<int8> outputSlice(sliceSize);

  // The current API does not inspect these bulk-I/O Result values.
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
          // Count volume faces only for axes with more than two cells.
          // Feature zero receives no volume-boundary contribution.
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

          // Z neighbors use the adjacent slice buffers.
          if(zIdx > 0)
          {
            if(prevSlice[sliceIndex] != feature && prevSlice[sliceIndex] > ignoreFeatureZeroVal)
            {
              onSurf++;
            }
          }

          if(yIdx > 0)
          {
            const int64 neighborIdx = sliceIndex - dimX;
            if(curSlice[neighborIdx] != feature && curSlice[neighborIdx] > ignoreFeatureZeroVal)
            {
              onSurf++;
            }
          }

          if(xIdx > 0)
          {
            const int64 neighborIdx = sliceIndex - 1;
            if(curSlice[neighborIdx] != feature && curSlice[neighborIdx] > ignoreFeatureZeroVal)
            {
              onSurf++;
            }
          }

          if(xIdx < dimX - 1)
          {
            const int64 neighborIdx = sliceIndex + 1;
            if(curSlice[neighborIdx] != feature && curSlice[neighborIdx] > ignoreFeatureZeroVal)
            {
              onSurf++;
            }
          }

          if(yIdx < dimY - 1)
          {
            const int64 neighborIdx = sliceIndex + dimX;
            if(curSlice[neighborIdx] != feature && curSlice[neighborIdx] > ignoreFeatureZeroVal)
            {
              onSurf++;
            }
          }

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

    boundaryCellsStore.copyFromBuffer(static_cast<usize>(zIdx) * sliceSize, nonstd::span<const int8>(outputSlice.data(), sliceSize));

    // Rotate buffer ownership without copying slice values.
    std::swap(prevSlice, curSlice);
    std::swap(curSlice, nextSlice);

    if(zIdx + 2 < dimZ)
    {
      featureIdsStore.copyIntoBuffer(static_cast<usize>(zIdx + 2) * sliceSize, nonstd::span<int32>(nextSlice.data(), sliceSize));
    }
  }

  return {};
}
