#include "ComputeSurfaceAreaToVolumeScanline.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "ComputeSurfaceAreaToVolume.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

using namespace nx::core;

ComputeSurfaceAreaToVolumeScanline::ComputeSurfaceAreaToVolumeScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                       const ComputeSurfaceAreaToVolumeInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeSurfaceAreaToVolumeScanline::~ComputeSurfaceAreaToVolumeScanline() noexcept = default;

Result<> ComputeSurfaceAreaToVolumeScanline::operator()()
{
  auto featureIdsArrayPtr = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& featureIdsStore = featureIdsArrayPtr->getDataStoreRef();
  const auto& numCells = m_DataStructure.getDataAs<Int32Array>(m_InputValues->NumCellsArrayPath)->getDataStoreRef();
  auto& surfaceAreaVolumeRatio = m_DataStructure.getDataAs<Float32Array>(m_InputValues->SurfaceAreaVolumeRatioArrayName)->getDataStoreRef();

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);

  auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, m_InputValues->NumCellsArrayPath.getParent(), *featureIdsArrayPtr, false, m_MessageHandler);
  if(validateNumFeatResult.invalid())
  {
    return validateNumFeatResult;
  }
  const usize numFeatures = numCells.getNumberOfTuples();
  if(numFeatures == 0)
  {
    return {};
  }
  SizeVec3 dims = imageGeom.getDimensions();
  FloatVec3 spacing = imageGeom.getSpacing();

  auto xPoints = static_cast<int64>(dims[0]);
  auto yPoints = static_cast<int64>(dims[1]);
  auto zPoints = static_cast<int64>(dims[2]);

  const float64 voxelVol = static_cast<float64>(spacing[0]) * spacing[1] * spacing[2];

  // Multiple cells contribute to one feature, so accumulation stays local until
  // the cell scan is complete.
  std::vector<float64> featureSurfaceArea(numFeatures, 0.0);

  const float64 xyFaceArea = static_cast<float64>(spacing[0]) * spacing[1];
  const float64 yzFaceArea = static_cast<float64>(spacing[1]) * spacing[2];
  const float64 zxFaceArea = static_cast<float64>(spacing[2]) * spacing[0];

  const usize sliceSize = static_cast<usize>(yPoints) * static_cast<usize>(xPoints);

  {
    // The rolling window turns Z-neighbor reads into sequential slice I/O.
    std::vector<int32> prevSlice(sliceSize, 0);
    std::vector<int32> curSlice(sliceSize, 0);
    std::vector<int32> nextSlice(sliceSize, 0);

    // A bulk-I/O error stops the scan before later slices or output arrays change.
    Result<> ioResult = featureIdsStore.copyIntoBuffer(0, nonstd::span<int32>(curSlice.data(), sliceSize));
    if(ioResult.invalid())
    {
      return ioResult;
    }
    if(zPoints > 1)
    {
      ioResult = featureIdsStore.copyIntoBuffer(sliceSize, nonstd::span<int32>(nextSlice.data(), sliceSize));
      if(ioResult.invalid())
      {
        return ioResult;
      }
    }

    for(int64 zIndex = 0; zIndex < zPoints; zIndex++)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      for(int64 yIndex = 0; yIndex < yPoints; yIndex++)
      {
        for(int64 xIndex = 0; xIndex < xPoints; xIndex++)
        {
          const usize inSlice = (static_cast<usize>(yIndex) * static_cast<usize>(xPoints)) + static_cast<usize>(xIndex);
          const int32 currentFeatureId = curSlice[inSlice];
          if(currentFeatureId < 1)
          {
            continue;
          }

          float64 onSurface = 0.0;

          // Z neighbors use adjacent buffers. X and Y neighbors use the current buffer.
          if(zIndex > 0)
          {
            if(prevSlice[inSlice] != currentFeatureId)
            {
              onSurface += xyFaceArea;
            }
          }

          if(zIndex < zPoints - 1)
          {
            if(nextSlice[inSlice] != currentFeatureId)
            {
              onSurface += xyFaceArea;
            }
          }

          if(yIndex > 0)
          {
            if(curSlice[inSlice - static_cast<usize>(xPoints)] != currentFeatureId)
            {
              onSurface += zxFaceArea;
            }
          }

          if(yIndex < yPoints - 1)
          {
            if(curSlice[inSlice + static_cast<usize>(xPoints)] != currentFeatureId)
            {
              onSurface += zxFaceArea;
            }
          }

          if(xIndex > 0)
          {
            if(curSlice[inSlice - 1] != currentFeatureId)
            {
              onSurface += yzFaceArea;
            }
          }

          if(xIndex < xPoints - 1)
          {
            if(curSlice[inSlice + 1] != currentFeatureId)
            {
              onSurface += yzFaceArea;
            }
          }

          featureSurfaceArea[currentFeatureId] += onSurface;
        }
      }

      // Vector swaps rotate the window without copying a slice.
      std::swap(prevSlice, curSlice);
      std::swap(curSlice, nextSlice);
      if(zIndex + 2 < zPoints)
      {
        ioResult = featureIdsStore.copyIntoBuffer(static_cast<usize>(zIndex + 2) * sliceSize, nonstd::span<int32>(nextSlice.data(), sliceSize));
        if(ioResult.invalid())
        {
          return ioResult;
        }
      }
    }
  }

  // The slice buffers are released before these batches are allocated. The cap
  // gives paired count/result buffers a 512 KiB budget, not a feature-count limit.
  // For F >= 2, B <= F/2 also limits their combined payload to 4*F bytes.
  constexpr usize maxBatchTuples = 65536;
  const usize batchCapacity = std::min(maxBatchTuples, std::max(usize{1}, numFeatures / 2));
  std::vector<int32> localNumCells(batchCapacity);
  std::vector<float32> localMetrics(batchCapacity);

  AbstractDataStore<float32>* sphericityPtr = nullptr;
  if(m_InputValues->CalculateSphericity)
  {
    m_MessageHandler.sendMessage(IFilter::Message::Type::Info, fmt::format("Computing Sphericity"));
    sphericityPtr = &m_DataStructure.getDataAs<Float32Array>(m_InputValues->SphericityArrayName)->getDataStoreRef();
  }
  const float64 thirdRootPi = m_InputValues->CalculateSphericity ? std::cbrt(nx::core::Constants::k_PiD) : 0.0;
  for(usize offset = 0; offset < numFeatures;)
  {
    const usize batchSize = std::min(batchCapacity, numFeatures - offset);
    auto ioResult = numCells.copyIntoBuffer(offset, nonstd::span<int32>(localNumCells.data(), batchSize));
    if(ioResult.invalid())
    {
      return ioResult;
    }

    // Background stays zero. Every other tuple uses the supplied cell count.
    const usize firstFeature = offset == 0 ? 1 : 0;
    localMetrics[0] = 0.0f;
    for(usize batchIndex = firstFeature; batchIndex < batchSize; batchIndex++)
    {
      const float64 featureVolume = voxelVol * localNumCells[batchIndex];
      localMetrics[batchIndex] = static_cast<float32>(featureSurfaceArea[offset + batchIndex] / featureVolume);
    }
    ioResult = surfaceAreaVolumeRatio.copyFromBuffer(offset, nonstd::span<const float32>(localMetrics.data(), batchSize));
    if(ioResult.invalid())
    {
      return ioResult;
    }

    if(sphericityPtr != nullptr)
    {
      // Reuse the ratio buffer after its write; no second output buffer is needed.
      for(usize batchIndex = firstFeature; batchIndex < batchSize; batchIndex++)
      {
        const float64 featureVolume = voxelVol * localNumCells[batchIndex];
        localMetrics[batchIndex] = static_cast<float32>((thirdRootPi * std::pow((6.0 * featureVolume), 2.0 / 3.0)) / featureSurfaceArea[offset + batchIndex]);
      }
      ioResult = sphericityPtr->copyFromBuffer(offset, nonstd::span<const float32>(localMetrics.data(), batchSize));
      if(ioResult.invalid())
      {
        return ioResult;
      }
    }
    offset += batchSize;
  }

  return {};
}
