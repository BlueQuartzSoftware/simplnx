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
  //
  //  if(largestFeature != (numFeatures - 1))
  //  {
  //    return {MakeErrorResult(-5555, fmt::format("The number of Features in the NumCells array ({}) does not match the largest Feature Id {} in the FeatureIds array", numFeatures, largestFeature))};
  //  }

  SizeVec3 dims = imageGeom.getDimensions();
  FloatVec3 spacing = imageGeom.getSpacing();

  int64_t xPoints = static_cast<int64_t>(dims[0]);
  int64_t yPoints = static_cast<int64_t>(dims[1]);
  int64_t zPoints = static_cast<int64_t>(dims[2]);

  float voxelVol = spacing[0] * spacing[1] * spacing[2];

  std::vector<float> featureSurfaceArea(static_cast<size_t>(numFeatures), 0.0f);

  int64_t neighpoints[6] = {0, 0, 0, 0, 0, 0};
  neighpoints[0] = -xPoints * yPoints;
  neighpoints[1] = -xPoints;
  neighpoints[2] = -1;
  neighpoints[3] = 1;
  neighpoints[4] = xPoints;
  neighpoints[5] = xPoints * yPoints;

  int32_t feature = 0;
  float onsurf = 0.0f;
  bool good = false;
  int64_t neighbor = 0;

  int64_t zStride = 0, yStride = 0;
  for(int64_t i = 0; i < zPoints; i++)
  {
    zStride = i * xPoints * yPoints;
    for(int64_t j = 0; j < yPoints; j++)
    {
      yStride = j * xPoints;
      for(int64_t k = 0; k < xPoints; k++)
      {
        onsurf = 0.0f;
        feature = featureIds[zStride + yStride + k];
        if(feature > 0)
        {
          for(int32_t l = 0; l < 6; l++)
          {
            good = true;
            neighbor = zStride + yStride + k + neighpoints[l];
            if(l == 0 && i == 0)
            {
              good = false;
            }
            if(l == 5 && i == (zPoints - 1))
            {
              good = false;
            }
            if(l == 1 && j == 0)
            {
              good = false;
            }
            if(l == 4 && j == (yPoints - 1))
            {
              good = false;
            }
            if(l == 2 && k == 0)
            {
              good = false;
            }
            if(l == 3 && k == (xPoints - 1))
            {
              good = false;
            }
            if(good && featureIds[neighbor] != feature)
            {
              if(l == 0 || l == 5) // XY face shared
              {
                onsurf = onsurf + spacing[0] * spacing[1];
              }
              if(l == 1 || l == 4) // YZ face shared
              {
                onsurf = onsurf + spacing[1] * spacing[2];
              }
              if(l == 2 || l == 3) // XZ face shared
              {
                onsurf = onsurf + spacing[2] * spacing[0];
              }
            }
          }
        }
        int32 featureId = featureIds[zStride + yStride + k];
        featureSurfaceArea[featureId] = featureSurfaceArea[featureId] + onsurf;
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
