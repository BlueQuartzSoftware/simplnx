#include "ComputeSurfaceAreaToVolumeScanline.hpp"

#include "ComputeSurfaceAreaToVolume.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

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
 * @brief Computes surface-area-to-volume ratio using chunk-sequential iteration.
 * OOC path: iterates chunks in order via loadChunk/getChunkLowerBounds/getChunkUpperBounds.
 * Same logic as ComputeSurfaceAreaToVolumeDirect.
 */
Result<> ComputeSurfaceAreaToVolumeScanline::operator()()
{
  // Input Cell Data
  auto featureIdsArrayPtr = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  auto& featureIdsStoreRef = featureIdsArrayPtr->getDataStoreRef();

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

  auto xPoints = static_cast<int64_t>(dims[0]);
  auto yPoints = static_cast<int64_t>(dims[1]);
  auto zPoints = static_cast<int64_t>(dims[2]);

  float32 voxelVol = spacing[0] * spacing[1] * spacing[2];

  std::vector<float> featureSurfaceArea(static_cast<size_t>(numFeatures), 0.0f);

  int64 neighborOffset[6] = {0, 0, 0, 0, 0, 0};
  neighborOffset[0] = -xPoints * yPoints; // -Z
  neighborOffset[1] = -xPoints;           // -Y
  neighborOffset[2] = -1;                 // -X
  neighborOffset[3] = 1;                  // +X
  neighborOffset[4] = xPoints;            // +Y
  neighborOffset[5] = xPoints * yPoints;  // +Z

  // Chunk-sequential iteration
  const uint64 numChunks = featureIdsStoreRef.getNumberOfChunks();

  for(uint64 chunkIdx = 0; chunkIdx < numChunks; chunkIdx++)
  {
    featureIdsStoreRef.loadChunk(chunkIdx);

    const auto chunkLowerBounds = featureIdsStoreRef.getChunkLowerBounds(chunkIdx);
    const auto chunkUpperBounds = featureIdsStoreRef.getChunkUpperBounds(chunkIdx);

    m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Processing Chunk {}/{}", chunkIdx + 1, numChunks));

    for(usize zIdx = chunkLowerBounds[0]; zIdx <= chunkUpperBounds[0]; zIdx++)
    {
      const int64 zStride = static_cast<int64>(zIdx) * xPoints * yPoints;
      for(usize yIdx = chunkLowerBounds[1]; yIdx <= chunkUpperBounds[1]; yIdx++)
      {
        const int64 yStride = static_cast<int64>(yIdx) * xPoints;
        for(usize xIdx = chunkLowerBounds[2]; xIdx <= chunkUpperBounds[2]; xIdx++)
        {
          const int64 ix = static_cast<int64>(xIdx);
          float onSurface = 0.0f;
          int32 currentFeatureId = featureIdsStoreRef[zStride + yStride + ix];
          if(currentFeatureId < 1)
          {
            continue;
          }

          for(int32 neighborOffsetIndex = 0; neighborOffsetIndex < 6; neighborOffsetIndex++)
          {
            if(neighborOffsetIndex == 0 && static_cast<int64>(zIdx) == 0)
            {
              continue;
            }
            if(neighborOffsetIndex == 5 && static_cast<int64>(zIdx) == (zPoints - 1))
            {
              continue;
            }
            if(neighborOffsetIndex == 1 && static_cast<int64>(yIdx) == 0)
            {
              continue;
            }
            if(neighborOffsetIndex == 4 && static_cast<int64>(yIdx) == (yPoints - 1))
            {
              continue;
            }
            if(neighborOffsetIndex == 2 && ix == 0)
            {
              continue;
            }
            if(neighborOffsetIndex == 3 && ix == (xPoints - 1))
            {
              continue;
            }

            int64 neighborIndex = zStride + yStride + ix + neighborOffset[neighborOffsetIndex];

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
          int32 featureId = featureIdsStoreRef[zStride + yStride + ix];
          featureSurfaceArea[featureId] = featureSurfaceArea[featureId] + onSurface;
        }
      }
    }
  }

  const float32 thirdRootPi = std::pow(nx::core::Constants::k_PiF, 0.333333f);
  for(usize i = 1; i < numFeatures; i++)
  {
    float featureVolume = voxelVol * numCells[i];
    surfaceAreaVolumeRatio[i] = featureSurfaceArea[i] / featureVolume;
  }

  if(m_InputValues->CalculateSphericity)
  {
    m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Computing Sphericity"));

    auto& sphericity = m_DataStructure.getDataAs<Float32Array>(m_InputValues->SphericityArrayName)->getDataStoreRef();
    for(usize i = 1; i < static_cast<usize>(numFeatures); i++)
    {
      float featureVolume = voxelVol * numCells[i];
      sphericity[i] = (thirdRootPi * std::pow((6.0f * featureVolume), 0.66666f)) / featureSurfaceArea[i];
    }
  }

  return {};
}
