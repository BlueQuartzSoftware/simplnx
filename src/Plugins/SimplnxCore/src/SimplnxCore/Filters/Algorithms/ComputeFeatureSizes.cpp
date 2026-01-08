#include "ComputeFeatureSizes.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <cmath>

using namespace nx::core;

namespace
{
constexpr nx::core::int32 k_BadFeatureCount = -78231;
constexpr nx::core::float32 k_PI = nx::core::numbers::pi_v<nx::core::float32>;

} // namespace

// -----------------------------------------------------------------------------
ComputeFeatureSizes::ComputeFeatureSizes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureSizesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFeatureSizes::~ComputeFeatureSizes() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeFeatureSizes::operator()()
{

  auto saveElementSizes = m_InputValues->SaveElementSizes;

  auto featureIdsArrayPath = m_InputValues->FeatureIdsPath;
  auto featureIdsArrayPtr = m_DataStructure.getDataAs<Int32Array>(featureIdsArrayPath);
  const auto& featureIdsStoreRef = featureIdsArrayPtr->getDataStoreRef();
  {
    auto featureAttributeMatrixPath = m_InputValues->FeatureAttributeMatrixPath;
    auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, featureAttributeMatrixPath, *featureIdsArrayPtr, false, m_MessageHandler);
    if(validateNumFeatResult.invalid())
    {
      return validateNumFeatResult;
    }
  }
  usize totalPoints = featureIdsStoreRef.getNumberOfTuples();

  auto geomPath = m_InputValues->InputImageGeometryPath;
  auto* geom = m_DataStructure.getDataAs<IGeometry>(geomPath);
  auto featureAttributeMatrixPath = m_InputValues->FeatureAttributeMatrixPath;

  DataPath volumesPath = featureAttributeMatrixPath.createChildPath(m_InputValues->VolumesName);
  DataPath equivDiamPath = featureAttributeMatrixPath.createChildPath(m_InputValues->EquivalentDiametersName);
  DataPath numElementsPath = featureAttributeMatrixPath.createChildPath(m_InputValues->NumElementsName);

  // If the geometry is an ImageGeometry or a RectilinearGeometry
  auto* imageGeom = dynamic_cast<ImageGeom*>(geom);
  if(nullptr != imageGeom)
  {

    auto& volumes = m_DataStructure.getDataAs<Float32Array>(volumesPath)->getDataStoreRef();
    auto& equivalentDiameters = m_DataStructure.getDataAs<Float32Array>(equivDiamPath)->getDataStoreRef();
    auto& numElements = m_DataStructure.getDataAs<Int32Array>(numElementsPath)->getDataStoreRef();

    usize featureIdsMaxIdx = std::distance(featureIdsStoreRef.begin(), std::max_element(featureIdsStoreRef.cbegin(), featureIdsStoreRef.cend()));
    usize maxValue = featureIdsStoreRef[featureIdsMaxIdx];
    usize numFeatures = maxValue + 1;

    std::vector<uint64> featureCounts(numFeatures, 0);

    for(size_t j = 0; j < totalPoints; j++)
    {
      int32_t gnum = featureIdsStoreRef[j];
      auto temp = featureCounts[gnum] + 1;
      featureCounts[gnum] = temp;
    }

    FloatVec3 spacing = imageGeom->getSpacing();

    if(imageGeom->getNumXCells() == 1 || imageGeom->getNumYCells() == 1 || imageGeom->getNumZCells() == 1)
    {
      float res_scalar = 0.0f;
      if(imageGeom->getNumXCells() == 1)
      {
        res_scalar = spacing[1] * spacing[2];
      }
      else if(imageGeom->getNumYCells() == 1)
      {
        res_scalar = spacing[0] * spacing[2];
      }
      else if(imageGeom->getNumZCells() == 1)
      {
        res_scalar = spacing[0] * spacing[1];
      }

      for(size_t i = 1; i < numFeatures; i++)
      {
        numElements[i] = static_cast<int32_t>(featureCounts[i]);
        if(featureCounts[i] > 9007199254740992ULL)
        {
          std::string ss = fmt::format("Number of voxels belonging to feature {} ({}) is greater than 9007199254740992", i, featureCounts[i]);
          return MakeErrorResult(k_BadFeatureCount, ss);
        }
        volumes[i] = static_cast<float32>(featureCounts[i]) * static_cast<float32>(res_scalar);

        float32 rad = volumes[i] / k_PI;
        float32 diameter = (2 * sqrtf(rad));
        equivalentDiameters[i] = diameter;
      }
    }
    else
    {
      float32 res_scalar = spacing[0] * spacing[1] * spacing[2];
      float vol_term = (4.0f / 3.0f) * k_PI;
      for(usize i = 1; i < numFeatures; i++)
      {
        numElements[i] = static_cast<int32>(featureCounts[i]);
        if(featureCounts[i] > 9007199254740992ULL)
        {
          std::string ss = fmt::format("Number of voxels belonging to feature {} ({}) is greater than 9007199254740992", i, featureCounts[i]);
          return MakeErrorResult(k_BadFeatureCount, ss);
        }

        volumes[i] = static_cast<float32>(featureCounts[i]) * static_cast<float32>(res_scalar);

        float32 rad = volumes[i] / vol_term;
        float32 diameter = 2.0f * powf(rad, 0.3333333333f);
        equivalentDiameters[i] = diameter;
      }
    }

    if(saveElementSizes)
    {
      int32 err = imageGeom->findElementSizes(false);
      if(err < 0)
      {
        std::string ss = fmt::format("Error computing Element sizes for Geometry type {}", imageGeom->getTypeName());
        return MakeErrorResult(err, ss);
      }
    }
  }
  else
  {
    auto& volumes = m_DataStructure.getDataAs<Float32Array>(volumesPath)->getDataStoreRef();
    auto& equivalentDiameters = m_DataStructure.getDataAs<Float32Array>(equivDiamPath)->getDataStoreRef();
    auto& numElements = m_DataStructure.getDataAs<Int32Array>(numElementsPath)->getDataStoreRef();

    usize numFeatures = volumes.getNumberOfTuples();

    int32_t err = geom->findElementSizes(false);
    if(err < 0)
    {
      std::string ss = fmt::format("Error computing Element sizes for Geometry type {}", geom->getTypeName());
      return MakeErrorResult(err, ss);
    }

    const Float32Array* elemSizes = geom->getElementSizes();

    std::vector<float> featureCounts(numFeatures, 1);

    for(size_t j = 0; j < totalPoints; j++)
    {
      int32 gnum = featureIdsStoreRef[j];
      auto temp = featureCounts[gnum] + 1;
      featureCounts[gnum] = temp;
      auto temp2 = volumes[gnum];
      volumes[gnum] = temp2 + (*elemSizes)[j];
    }
    float vol_term = (4.0f / 3.0f) * k_PI;
    for(size_t i = 1; i < numFeatures; i++)
    {
      numElements[i] = static_cast<int32>(featureCounts[i]);
      float rad = volumes[i] / vol_term;
      float diameter = 2.0f * powf(rad, 0.3333333333f);
      equivalentDiameters[i] = diameter;
    }

    if(!saveElementSizes)
    {
      geom->deleteElementSizes();
    }
  }

  return {};
}
