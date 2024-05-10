#include "ComputeLargestCrossSections.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeLargestCrossSections::ComputeLargestCrossSections(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                         ComputeLargestCrossSectionsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeLargestCrossSections::~ComputeLargestCrossSections() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeLargestCrossSections::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeLargestCrossSections::operator()()
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  auto& largestCrossSections = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->LargestCrossSectionsArrayPath);
  const usize numFeatures = largestCrossSections.getNumberOfTuples();

  // Validate the largestCrossSections array is the proper size
  auto validateResults = ValidateNumFeaturesInArray(m_DataStructure, m_InputValues->LargestCrossSectionsArrayPath, featureIds);
  if(validateResults.invalid())
  {
    return validateResults;
  }

  std::vector<float32> featureCounts(numFeatures, 0.0f);

  usize outPlane = 0, inPlane1 = 0, inPlane2 = 0;
  float32 resScalar = 0.0f, area = 0.0f;
  usize stride1 = 0, stride2 = 0, stride3 = 0;
  usize iStride = 0, jStride = 0, kStride = 0;
  usize point = 0, gNum = 0;

  FloatVec3 spacing = imageGeom.getSpacing();

  if(m_InputValues->Plane == 0)
  {
    outPlane = imageGeom.getNumZCells();
    inPlane1 = imageGeom.getNumXCells();
    inPlane2 = imageGeom.getNumYCells();

    resScalar = spacing[0] * spacing[1];
    stride1 = inPlane1 * inPlane2;
    stride2 = 1;
    stride3 = inPlane1;
  }
  if(m_InputValues->Plane == 1)
  {
    outPlane = imageGeom.getNumYCells();
    inPlane1 = imageGeom.getNumXCells();
    inPlane2 = imageGeom.getNumZCells();
    resScalar = spacing[0] * spacing[2];
    stride1 = inPlane1;
    stride2 = 1;
    stride3 = inPlane1 * inPlane2;
  }
  if(m_InputValues->Plane == 2)
  {
    outPlane = imageGeom.getNumXCells();
    inPlane1 = imageGeom.getNumYCells();
    inPlane2 = imageGeom.getNumZCells();
    resScalar = spacing[1] * spacing[2];
    stride1 = 1;
    stride2 = inPlane1;
    stride3 = inPlane1 * inPlane2;
  }
  for(size_t i = 0; i < outPlane; i++)
  {
    std::fill(featureCounts.begin(), featureCounts.end(), 0.0f);

    iStride = i * stride1;
    for(size_t j = 0; j < inPlane1; j++)
    {
      jStride = j * stride2;
      for(size_t k = 0; k < inPlane2; k++)
      {
        kStride = k * stride3;
        point = iStride + jStride + kStride;
        gNum = featureIds[point];
        featureCounts[gNum]++;
      }
    }
    for(size_t g = 1; g < numFeatures; g++)
    {
      area = featureCounts[g] * resScalar;
      if(area > largestCrossSections[g])
      {
        largestCrossSections[g] = area;
      }
    }
  }
  return {};
}
