#include "ComputeLargestCrossSectionsScanline.hpp"

#include "ComputeLargestCrossSections.hpp"

#include "simplnx/Common/Extent.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <memory>
#include <vector>

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeLargestCrossSectionsScanline::ComputeLargestCrossSectionsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                         const ComputeLargestCrossSectionsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeLargestCrossSectionsScanline::~ComputeLargestCrossSectionsScanline() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeLargestCrossSectionsScanline::operator()()
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  auto& featureIdsStore = featureIds.getDataStoreRef();
  auto& largestCrossSectStore = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->LargestCrossSectionsArrayPath).getDataStoreRef();
  const usize numFeatures = largestCrossSectStore.getNumberOfTuples();

  auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, m_InputValues->LargestCrossSectionsArrayPath, featureIds, false, m_MessageHandler);
  if(validateNumFeatResult.invalid())
  {
    return validateNumFeatResult;
  }

  std::vector<float32> featureCounts(numFeatures, 0.0f);
  std::vector<float32> largestCrossSections(numFeatures, 0.0f);
  Result<> outputReadResult = largestCrossSectStore.copyIntoBuffer(0, nonstd::span<float32>(largestCrossSections.data(), numFeatures));
  if(outputReadResult.invalid())
  {
    return outputReadResult;
  }

  const usize xCells = imageGeom.getNumXCells();
  const usize yCells = imageGeom.getNumYCells();
  const usize zCells = imageGeom.getNumZCells();
  const FloatVec3 spacing = imageGeom.getSpacing();

  usize numPlanes = 0;
  usize planeCellCount = 0;
  float32 areaScalar = 0.0f;

  if(m_InputValues->Plane == 0)
  {
    numPlanes = zCells;
    planeCellCount = xCells * yCells;
    areaScalar = spacing[0] * spacing[1];
  }
  if(m_InputValues->Plane == 1)
  {
    numPlanes = yCells;
    planeCellCount = xCells * zCells;
    areaScalar = spacing[0] * spacing[2];
  }
  if(m_InputValues->Plane == 2)
  {
    numPlanes = xCells;
    planeCellCount = yCells * zCells;
    areaScalar = spacing[1] * spacing[2];
  }

  auto contiguousPlane = std::make_unique<int32[]>(m_InputValues->Plane == 0 ? planeCellCount : 0);

  for(usize planeIndex = 0; planeIndex < numPlanes; planeIndex++)
  {
    if(m_ShouldCancel)
    {
      return largestCrossSectStore.copyFromBuffer(0, nonstd::span<const float32>(largestCrossSections.data(), numFeatures));
    }

    std::fill(featureCounts.begin(), featureCounts.end(), 0.0f);

    nonstd::span<const int32> planeFeatureIds;
    std::vector<int32> stridedPlane;
    if(m_InputValues->Plane == 0)
    {
      Result<> inputReadResult = featureIdsStore.copyIntoBuffer(planeIndex * planeCellCount, nonstd::span<int32>(contiguousPlane.get(), planeCellCount));
      if(inputReadResult.invalid())
      {
        return inputReadResult;
      }
      planeFeatureIds = nonstd::span<const int32>(contiguousPlane.get(), planeCellCount);
    }
    else
    {
      // Feature Id tuple axes are [Z, Y, X]. Extent reads collect one
      // noncontiguous cross-section without per-cell OOC access or rescanning.
      const Extent planeExtent = m_InputValues->Plane == 1 ? Extent({0, planeIndex, 0}, {zCells - 1, planeIndex, xCells - 1}) : Extent({0, 0, planeIndex}, {zCells - 1, yCells - 1, planeIndex});
      stridedPlane = featureIdsStore.readExtent(planeExtent);
      planeFeatureIds = nonstd::span<const int32>(stridedPlane.data(), stridedPlane.size());
    }

    for(const int32 featureId : planeFeatureIds)
    {
      featureCounts[static_cast<usize>(featureId)]++;
    }

    for(usize featureId = 1; featureId < numFeatures; featureId++)
    {
      const float32 area = featureCounts[featureId] * areaScalar;
      if(area > largestCrossSections[featureId])
      {
        largestCrossSections[featureId] = area;
      }
    }
  }
  return largestCrossSectStore.copyFromBuffer(0, nonstd::span<const float32>(largestCrossSections.data(), numFeatures));
}
