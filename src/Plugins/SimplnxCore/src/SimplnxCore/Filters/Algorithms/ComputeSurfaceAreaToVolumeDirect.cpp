#include <array>

#include "ComputeSurfaceAreaToVolumeDirect.hpp"

#include "ComputeSurfaceAreaToVolume.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <fmt/format.h>

using namespace nx::core;

ComputeSurfaceAreaToVolumeDirect::ComputeSurfaceAreaToVolumeDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                   const ComputeSurfaceAreaToVolumeInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeSurfaceAreaToVolumeDirect::~ComputeSurfaceAreaToVolumeDirect() noexcept = default;

Result<> ComputeSurfaceAreaToVolumeDirect::operator()()
{
  auto featureIdsArrayPtr = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& featureIdsStoreRef = featureIdsArrayPtr->getDataStoreRef();
  const auto& numCells = m_DataStructure.getDataAs<Int32Array>(m_InputValues->NumCellsArrayPath)->getDataStoreRef();
  auto& surfaceAreaVolumeRatio = m_DataStructure.getDataAs<Float32Array>(m_InputValues->SurfaceAreaVolumeRatioArrayName)->getDataStoreRef();

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

  // Multiple cells contribute to one feature, so accumulation stays local until
  // the cell scan is complete.
  std::vector<float32> featureSurfaceArea(static_cast<usize>(numFeatures), 0.0f);

  // Flat offsets avoid repeated coordinate-to-index conversion for in-memory
  // six-neighbor reads.
  std::array<int64, 6> neighborOffset = {0, 0, 0, 0, 0, 0};
  neighborOffset[0] = -xPoints * yPoints; // -Z
  neighborOffset[1] = -xPoints;           // -Y
  neighborOffset[2] = -1;                 // -X
  neighborOffset[3] = 1;                  // +X
  neighborOffset[4] = xPoints;            // +Y
  neighborOffset[5] = xPoints * yPoints;  // +Z

  for(int64 zIdx = 0; zIdx < zPoints; zIdx++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    int64 zStride = zIdx * xPoints * yPoints;
    for(int64 yIdx = 0; yIdx < yPoints; yIdx++)
    {
      int64 yStride = yIdx * xPoints;
      for(int64 xIdx = 0; xIdx < xPoints; xIdx++)
      {
        float32 onSurface = 0.0f;
        int32 currentFeatureId = featureIdsStoreRef[zStride + yStride + xIdx];
        // Feature 0 is the background tuple.
        if(currentFeatureId < 1)
        {
          continue;
        }

        for(int32 neighborOffsetIndex = 0; neighborOffsetIndex < 6; neighborOffsetIndex++)
        {
          if(neighborOffsetIndex == 0 && zIdx == 0)
          {
            continue;
          }
          if(neighborOffsetIndex == 5 && zIdx == (zPoints - 1))
          {
            continue;
          }
          if(neighborOffsetIndex == 1 && yIdx == 0)
          {
            continue;
          }
          if(neighborOffsetIndex == 4 && yIdx == (yPoints - 1))
          {
            continue;
          }
          if(neighborOffsetIndex == 2 && xIdx == 0)
          {
            continue;
          }
          if(neighborOffsetIndex == 3 && xIdx == (xPoints - 1))
          {
            continue;
          }

          int64 neighborIndex = zStride + yStride + xIdx + neighborOffset[neighborOffsetIndex];

          if(featureIdsStoreRef[neighborIndex] != currentFeatureId)
          {
            if(neighborOffsetIndex == 0 || neighborOffsetIndex == 5)
            {
              onSurface = onSurface + spacing[0] * spacing[1];
            }
            if(neighborOffsetIndex == 1 || neighborOffsetIndex == 4)
            {
              onSurface = onSurface + spacing[1] * spacing[2];
            }
            if(neighborOffsetIndex == 2 || neighborOffsetIndex == 3)
            {
              onSurface = onSurface + spacing[2] * spacing[0];
            }
          }
        }
        int32 featureId = featureIdsStoreRef[zStride + yStride + xIdx];
        featureSurfaceArea[featureId] = featureSurfaceArea[featureId] + onSurface;
      }
    }
  }

  const float32 thirdRootPi = std::pow(nx::core::Constants::k_PiF, 0.333333f);
  for(usize i = 1; i < numFeatures; i++)
  {
    float32 featureVolume = voxelVol * numCells[i];
    surfaceAreaVolumeRatio[i] = featureSurfaceArea[i] / featureVolume;
  }

  if(m_InputValues->CalculateSphericity)
  {
    m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Computing Sphericity"));

    auto& sphericity = m_DataStructure.getDataAs<Float32Array>(m_InputValues->SphericityArrayName)->getDataStoreRef();
    // Sphericity is one for a sphere and decreases as the surface becomes less compact.
    for(usize i = 1; i < static_cast<usize>(numFeatures); i++)
    {
      float32 featureVolume = voxelVol * numCells[i];
      sphericity[i] = (thirdRootPi * std::pow((6.0f * featureVolume), 0.66666f)) / featureSurfaceArea[i];
    }
  }

  return {};
}
