#include "ComputeSurfaceAreaToVolumeScanline.hpp"

#include "ComputeSurfaceAreaToVolume.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeSurfaceAreaToVolumeScanline::ComputeSurfaceAreaToVolumeScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                       const ComputeSurfaceAreaToVolumeInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeSurfaceAreaToVolumeScanline::~ComputeSurfaceAreaToVolumeScanline() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief Computes surface-area-to-volume ratio using Z-slice bulk I/O with a rolling window.
 * OOC path: reads featureIds one Z-slice at a time via copyIntoBuffer(),
 * keeping 3 slices resident (prev/cur/next) for cross-slice neighbour access.
 * Same logic as ComputeSurfaceAreaToVolumeDirect.
 */
Result<> ComputeSurfaceAreaToVolumeScanline::operator()()
{
  // Input Cell Data
  auto featureIdsArrayPtr = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  auto& featureIdsStore = featureIdsArrayPtr->getDataStoreRef();

  // Input Feature Data
  const auto& numCells = m_DataStructure.getDataAs<Int32Array>(m_InputValues->NumCellsArrayPath)->getDataStoreRef();

  // Output Feature Data
  auto& surfaceAreaVolumeRatio = m_DataStructure.getDataAs<Float32Array>(m_InputValues->SurfaceAreaVolumeRatioArrayName)->getDataStoreRef();

  // Required Geometry
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);

  auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, m_InputValues->NumCellsArrayPath.getParent(), *featureIdsArrayPtr, false, m_MessageHandler);
  if(validateNumFeatResult.invalid())
  {
    return validateNumFeatResult;
  }
  auto numFeatures = static_cast<int32>(numCells.getNumberOfTuples());
  SizeVec3 dims = imageGeom.getDimensions();
  FloatVec3 spacing = imageGeom.getSpacing();

  auto xPoints = static_cast<int64>(dims[0]);
  auto yPoints = static_cast<int64>(dims[1]);
  auto zPoints = static_cast<int64>(dims[2]);

  float32 voxelVol = spacing[0] * spacing[1] * spacing[2];

  std::vector<float32> featureSurfaceArea(static_cast<usize>(numFeatures), 0.0f);

  // Face areas for each neighbor direction
  const float32 xyFaceArea = spacing[0] * spacing[1]; // XY face shared (Z-normal)
  const float32 yzFaceArea = spacing[1] * spacing[2]; // YZ face shared (X-normal)
  const float32 zxFaceArea = spacing[2] * spacing[0]; // XZ face shared (Y-normal)

  // Z-slice rolling window for bulk I/O
  const usize sliceSize = static_cast<usize>(yPoints) * static_cast<usize>(xPoints);
  std::vector<int32> prevSlice(sliceSize, 0);
  std::vector<int32> curSlice(sliceSize, 0);
  std::vector<int32> nextSlice(sliceSize, 0);

  // Load initial slices
  featureIdsStore.copyIntoBuffer(0, nonstd::span<int32>(curSlice.data(), sliceSize));
  if(zPoints > 1)
  {
    featureIdsStore.copyIntoBuffer(sliceSize, nonstd::span<int32>(nextSlice.data(), sliceSize));
  }

  for(int64 z = 0; z < zPoints; z++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    for(int64 y = 0; y < yPoints; y++)
    {
      for(int64 x = 0; x < xPoints; x++)
      {
        const usize inSlice = static_cast<usize>(y) * static_cast<usize>(xPoints) + static_cast<usize>(x);
        int32 currentFeatureId = curSlice[inSlice];
        if(currentFeatureId < 1)
        {
          continue;
        }

        float32 onSurface = 0.0f;

        // -Z neighbor (index 0): check prevSlice at same (y,x)
        if(z > 0)
        {
          if(prevSlice[inSlice] != currentFeatureId)
          {
            onSurface += xyFaceArea;
          }
        }

        // +Z neighbor (index 5): check nextSlice at same (y,x)
        if(z < zPoints - 1)
        {
          if(nextSlice[inSlice] != currentFeatureId)
          {
            onSurface += xyFaceArea;
          }
        }

        // -Y neighbor (index 1): check curSlice at (y-1, x)
        if(y > 0)
        {
          if(curSlice[inSlice - static_cast<usize>(xPoints)] != currentFeatureId)
          {
            onSurface += yzFaceArea;
          }
        }

        // +Y neighbor (index 4): check curSlice at (y+1, x)
        if(y < yPoints - 1)
        {
          if(curSlice[inSlice + static_cast<usize>(xPoints)] != currentFeatureId)
          {
            onSurface += yzFaceArea;
          }
        }

        // -X neighbor (index 2): check curSlice at (y, x-1)
        if(x > 0)
        {
          if(curSlice[inSlice - 1] != currentFeatureId)
          {
            onSurface += zxFaceArea;
          }
        }

        // +X neighbor (index 3): check curSlice at (y, x+1)
        if(x < xPoints - 1)
        {
          if(curSlice[inSlice + 1] != currentFeatureId)
          {
            onSurface += zxFaceArea;
          }
        }

        featureSurfaceArea[currentFeatureId] += onSurface;
      }
    }

    // Shift the rolling window
    std::swap(prevSlice, curSlice);
    std::swap(curSlice, nextSlice);
    if(z + 2 < zPoints)
    {
      featureIdsStore.copyIntoBuffer(static_cast<usize>(z + 2) * sliceSize, nonstd::span<int32>(nextSlice.data(), sliceSize));
    }
  }

  // Cache the feature-level numCells array locally to avoid per-element OOC lookups
  const usize numFeaturesUSize = static_cast<usize>(numFeatures);
  std::vector<int32> localNumCells(numFeaturesUSize);
  numCells.copyIntoBuffer(0, nonstd::span<int32>(localNumCells.data(), numFeaturesUSize));

  // Compute results into a local buffer, then bulk-write to OOC store
  std::vector<float32> localSurfaceAreaVolumeRatio(numFeaturesUSize, 0.0f);

  const float32 thirdRootPi = std::pow(nx::core::Constants::k_PiF, 0.333333f);
  for(usize i = 1; i < numFeaturesUSize; i++)
  {
    float32 featureVolume = voxelVol * localNumCells[i];
    localSurfaceAreaVolumeRatio[i] = featureSurfaceArea[i] / featureVolume;
  }
  surfaceAreaVolumeRatio.copyFromBuffer(0, nonstd::span<const float32>(localSurfaceAreaVolumeRatio.data(), numFeaturesUSize));

  if(m_InputValues->CalculateSphericity)
  {
    m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Computing Sphericity"));

    auto& sphericity = m_DataStructure.getDataAs<Float32Array>(m_InputValues->SphericityArrayName)->getDataStoreRef();
    std::vector<float32> localSphericity(numFeaturesUSize, 0.0f);
    for(usize i = 1; i < numFeaturesUSize; i++)
    {
      float32 featureVolume = voxelVol * localNumCells[i];
      localSphericity[i] = (thirdRootPi * std::pow((6.0f * featureVolume), 0.66666f)) / featureSurfaceArea[i];
    }
    sphericity.copyFromBuffer(0, nonstd::span<const float32>(localSphericity.data(), numFeaturesUSize));
  }

  return {};
}
