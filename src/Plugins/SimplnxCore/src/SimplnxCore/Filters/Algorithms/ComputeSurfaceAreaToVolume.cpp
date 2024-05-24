#include "ComputeSurfaceAreaToVolume.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeSurfaceAreaToVolume::ComputeSurfaceAreaToVolume(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                       ComputeSurfaceAreaToVolumeInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeSurfaceAreaToVolume::~ComputeSurfaceAreaToVolume() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeSurfaceAreaToVolume::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeSurfaceAreaToVolume::operator()()
{
  // Input Cell Data
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);

  // Input Feature Data
  const auto& numCells = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->NumCellsArrayPath);

  // Output Feature Data
  auto& surfaceAreaVolumeRatio = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->SurfaceAreaVolumeRatioArrayName);

  // Required Geometry
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);

  // Validate that the selected InArray has tuples equal to the largest
  // Feature Id; the filter would not crash otherwise, but the user should
  // be notified of unanticipated behavior ; this cannot be done in the dataCheck since
  // we don't have access to the data yet
  auto numFeatures = static_cast<int32_t>(numCells.getNumberOfTuples());
  bool mismatchedFeatures = false;
  int32_t largestFeature = 0;
  size_t numTuples = featureIds.getNumberOfTuples();
  std::string errorMessage;
  for(size_t i = 0; i < numTuples; i++)
  {
    if(featureIds[i] > largestFeature)
    {
      largestFeature = featureIds[i];
      if(largestFeature >= numFeatures)
      {
        mismatchedFeatures = true;
        errorMessage = fmt::format("The given FeatureIds Array '{}' has a value that is larger than allowed by the given Feature Attribute Matrix '{}'.\n {} >= {}",
                                   m_InputValues->FeatureIdsArrayPath.toString(), m_InputValues->NumCellsArrayPath.toString(), largestFeature, numFeatures);
      }
    }
  }

  if(mismatchedFeatures)
  {
    return {MakeErrorResult(-5555, errorMessage)};
  }

  SizeVec3 dims = imageGeom.getDimensions();
  FloatVec3 spacing = imageGeom.getSpacing();

  int64_t xPoints = static_cast<int64_t>(dims[0]);
  int64_t yPoints = static_cast<int64_t>(dims[1]);
  int64_t zPoints = static_cast<int64_t>(dims[2]);

  float voxelVol = spacing[0] * spacing[1] * spacing[2];

  std::vector<float> featureSurfaceArea(static_cast<size_t>(numFeatures), 0.0f);

  // This stores an offset to get to a particular index in the array based on
  // a normal orthogonal cube
  int64_t neighborOffset[6] = {0, 0, 0, 0, 0, 0};
  neighborOffset[0] = -xPoints * yPoints; // -Z
  neighborOffset[1] = -xPoints;           // -Y
  neighborOffset[2] = -1;                 // -X
  neighborOffset[3] = 1;                  // +X
  neighborOffset[4] = xPoints;            // +Y
  neighborOffset[5] = xPoints * yPoints;  // +Z

  // Start looping over the regular grid data (This could be either an Image Geometry or a Rectilinear Grid geometry (in theory)
  for(int64_t zIdx = 0; zIdx < zPoints; zIdx++)
  {
    int64_t zStride = zIdx * xPoints * yPoints;
    for(int64_t yIdx = 0; yIdx < yPoints; yIdx++)
    {
      int64_t yStride = yIdx * xPoints;
      for(int64_t xIdx = 0; xIdx < xPoints; xIdx++)
      {
        float onSurface = 0.0f; // Start totalling the surface area
        int32 currentFeatureId = featureIds[zStride + yStride + xIdx];
        // If the current feature ID is not valid (< 1), then just continue;
        if(currentFeatureId < 1)
        {
          continue;
        }

        // Loop over all 6 face neighbors
        for(int32_t neighborOffsetIndex = 0; neighborOffsetIndex < 6; neighborOffsetIndex++)
        {
          if(neighborOffsetIndex == 0 && zIdx == 0) // if we are on the bottom Z Layer, skip
          {
            continue;
          }
          if(neighborOffsetIndex == 5 && zIdx == (zPoints - 1)) // if we are on the top Z Layer, skip
          {
            continue;
          }
          if(neighborOffsetIndex == 1 && yIdx == 0) // If we are on the first Y row, skip
          {
            continue;
          }
          if(neighborOffsetIndex == 4 && yIdx == (yPoints - 1)) // If we are on the last Y row, skip
          {
            continue;
          }
          if(neighborOffsetIndex == 2 && xIdx == 0) // If we are on the first X column, skip
          {
            continue;
          }
          if(neighborOffsetIndex == 3 && xIdx == (xPoints - 1)) // If we are on the last X column, skip
          {
            continue;
          }
          //
          int64_t neighborIndex = zStride + yStride + xIdx + neighborOffset[neighborOffsetIndex];

          if(featureIds[neighborIndex] != currentFeatureId)
          {
            if(neighborOffsetIndex == 0 || neighborOffsetIndex == 5) // XY face shared
            {
              onSurface = onSurface + spacing[0] * spacing[1];
            }
            if(neighborOffsetIndex == 1 || neighborOffsetIndex == 4) // YZ face shared
            {
              onSurface = onSurface + spacing[1] * spacing[2];
            }
            if(neighborOffsetIndex == 2 || neighborOffsetIndex == 3) // XZ face shared
            {
              onSurface = onSurface + spacing[2] * spacing[0];
            }
          }
        }
        int32 featureId = featureIds[zStride + yStride + xIdx];
        featureSurfaceArea[featureId] = featureSurfaceArea[featureId] + onSurface;
      }
    }
  }

  const float thirdRootPi = std::pow(nx::core::Constants::k_PiF, 0.333333f);
  for(size_t i = 1; i < static_cast<size_t>(numFeatures); i++)
  {
    float featureVolume = voxelVol * numCells[i];
    surfaceAreaVolumeRatio[i] = featureSurfaceArea[i] / featureVolume;
  }

  if(m_InputValues->CalculateSphericity) // Calc the sphericity if requested
  {
    auto& sphericity = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->SphericityArrayName);
    for(size_t i = 1; i < static_cast<size_t>(numFeatures); i++)
    {
      float featureVolume = voxelVol * numCells[i];
      sphericity[i] = (thirdRootPi * std::pow((6.0f * featureVolume), 0.66666f)) / featureSurfaceArea[i];
    }
  }

  return {};
}
