#include "ComputeLargestCrossSectionsDirect.hpp"

#include "ComputeLargestCrossSections.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <tbb/combinable.h>

#include <algorithm>
#include <array>
#include <vector>

using namespace nx::core;

namespace
{
/**
 * @brief Counts Feature Id values in one strided plane.
 * @tparam FeatureIdsT Specifies the indexed Feature Id store type.
 * @param featureCounts Receives counts for each Feature Id.
 * @param featureIds Provides the flat Feature Id values.
 * @param planeOffset Identifies the first value in the plane.
 * @param firstDimension Specifies the first plane dimension.
 * @param secondDimension Specifies the second plane dimension.
 * @param firstStride Specifies the flat step along the first dimension.
 * @param secondStride Specifies the flat step along the second dimension.
 *
 * The caller selects strides for the requested image plane. Feature 0 is
 * counted here and excluded when maximum areas are updated.
 */
template <class FeatureIdsT>
void CountPlane(std::vector<float32>& featureCounts, const FeatureIdsT& featureIds, usize planeOffset, usize firstDimension, usize secondDimension, usize firstStride, usize secondStride)
{
  for(usize firstIndex = 0; firstIndex < firstDimension; firstIndex++)
  {
    const usize firstOffset = firstIndex * firstStride;
    for(usize secondIndex = 0; secondIndex < secondDimension; secondIndex++)
    {
      const usize point = planeOffset + firstOffset + (secondIndex * secondStride);
      featureCounts[static_cast<usize>(featureIds[point])]++;
    }
  }
}

/**
 * @brief Updates maximum feature areas from one plane.
 * @param featureCounts Provides counts for each Feature Id.
 * @param numFeatures Specifies the number of feature tuples.
 * @param areaScalar Converts a cell count to physical area.
 * @param largestCrossSections Receives maximum areas.
 *
 * Feature 0 is the background tuple and remains unchanged.
 */
void UpdateLargestCrossSections(const float32* featureCounts, usize numFeatures, float32 areaScalar, float32* largestCrossSections)
{
  for(usize featureId = 1; featureId < numFeatures; featureId++)
  {
    const float32 area = featureCounts[featureId] * areaScalar;
    if(area > largestCrossSections[featureId])
    {
      largestCrossSections[featureId] = area;
    }
  }
}

/**
 * @struct XzCrossSectionScratch
 * @brief Holds one XZ worker's counts and local maxima.
 *
 * Each worker owns one instance. The serial reduction combines maxima after
 * parallel reads finish.
 */
struct XzCrossSectionScratch
{
  /**
   * @brief Creates zeroed counts and copies the starting maxima.
   * @param numFeatures Specifies the number of feature tuples.
   * @param initialLargestCrossSections Provides the output values before work.
   */
  XzCrossSectionScratch(usize numFeatures, const std::vector<float32>& initialLargestCrossSections)
  : featureCounts(numFeatures, 0.0f)
  , largestCrossSections(initialLargestCrossSections)
  {
  }

  std::vector<float32> featureCounts;
  std::vector<float32> largestCrossSections;
};

using XzCrossSectionThreadScratch = tbb::combinable<XzCrossSectionScratch>;

/**
 * @class ComputeXzCrossSectionsImpl
 * @brief Counts independent XZ planes with worker-local scratch.
 *
 * Workers read an immutable raw pointer. The serial reduction updates shared
 * maxima after workers finish and prevents concurrent write races.
 */
class ComputeXzCrossSectionsImpl
{
public:
  /**
   * @brief Creates an XZ-plane worker.
   * @param featureIds Points to the contiguous Feature Id values.
   * @param xCells Specifies the X dimension.
   * @param zCells Specifies the Z dimension.
   * @param sliceSize Specifies the number of cells in one XY slice.
   * @param numFeatures Specifies the number of feature tuples.
   * @param areaScalar Converts a cell count to physical area.
   * @param threadScratch Provides one scratch instance per worker.
   * @param shouldCancel Stops later rows when true.
   */
  ComputeXzCrossSectionsImpl(const int32* featureIds, usize xCells, usize zCells, usize sliceSize, usize numFeatures, float32 areaScalar, XzCrossSectionThreadScratch& threadScratch,
                             const std::atomic_bool& shouldCancel)
  : m_FeatureIds(featureIds)
  , m_XCells(xCells)
  , m_ZCells(zCells)
  , m_SliceSize(sliceSize)
  , m_NumFeatures(numFeatures)
  , m_AreaScalar(areaScalar)
  , m_ThreadScratch(threadScratch)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Counts XZ planes for a range of Y indexes.
   * @param range Specifies the half-open Y-index range.
   *
   * The worker checks cancellation before each Y plane.
   */
  void operator()(const Range& range) const
  {
    XzCrossSectionScratch& scratch = m_ThreadScratch.local();
    for(usize yIndex = range.min(); yIndex < range.max(); yIndex++)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      std::fill(scratch.featureCounts.begin(), scratch.featureCounts.end(), 0.0f);

      for(usize zIndex = 0; zIndex < m_ZCells; zIndex++)
      {
        const int32* rowFeatureIds = m_FeatureIds + (zIndex * m_SliceSize) + (yIndex * m_XCells);
        for(usize xIndex = 0; xIndex < m_XCells; xIndex++)
        {
          scratch.featureCounts[static_cast<usize>(rowFeatureIds[xIndex])]++;
        }
      }
      UpdateLargestCrossSections(scratch.featureCounts.data(), m_NumFeatures, m_AreaScalar, scratch.largestCrossSections.data());
    }
  }

private:
  const int32* m_FeatureIds = nullptr;
  usize m_XCells = 0;
  usize m_ZCells = 0;
  usize m_SliceSize = 0;
  usize m_NumFeatures = 0;
  float32 m_AreaScalar = 0.0f;
  XzCrossSectionThreadScratch& m_ThreadScratch;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace

// -----------------------------------------------------------------------------
ComputeLargestCrossSectionsDirect::ComputeLargestCrossSectionsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                     const ComputeLargestCrossSectionsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeLargestCrossSectionsDirect::~ComputeLargestCrossSectionsDirect() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeLargestCrossSectionsDirect::operator()()
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& featureIdsStore = featureIds.getDataStoreRef();
  auto& largestCrossSectStore = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->LargestCrossSectionsArrayPath).getDataStoreRef();
  const usize numFeatures = largestCrossSectStore.getNumberOfTuples();

  auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, m_InputValues->LargestCrossSectionsArrayPath, featureIds, false, m_MessageHandler);
  if(validateNumFeatResult.invalid())
  {
    return validateNumFeatResult;
  }

  std::vector<float32> featureCounts(numFeatures, 0.0f);

  usize numPlanes = 0;
  usize firstDimension = 0;
  usize secondDimension = 0;
  usize planeStride = 0;
  usize firstStride = 0;
  usize secondStride = 0;
  float32 areaScalar = 0.0f;
  const usize xCells = imageGeom.getNumXCells();
  const usize yCells = imageGeom.getNumYCells();
  const usize zCells = imageGeom.getNumZCells();
  const FloatVec3 spacing = imageGeom.getSpacing();

  if(m_InputValues->Plane == 0)
  {
    numPlanes = zCells;
    firstDimension = xCells;
    secondDimension = yCells;
    planeStride = firstDimension * secondDimension;
    firstStride = 1;
    secondStride = xCells;
    areaScalar = spacing[0] * spacing[1];
  }
  if(m_InputValues->Plane == 1)
  {
    numPlanes = yCells;
    firstDimension = xCells;
    secondDimension = zCells;
    planeStride = xCells;
    firstStride = 1;
    secondStride = xCells * yCells;
    areaScalar = spacing[0] * spacing[2];
  }
  if(m_InputValues->Plane == 2)
  {
    numPlanes = xCells;
    firstDimension = yCells;
    secondDimension = zCells;
    planeStride = 1;
    firstStride = xCells;
    secondStride = xCells * yCells;
    areaScalar = spacing[1] * spacing[2];
  }

  m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Computing Cross Section for {} planes", numPlanes));

  const auto* inMemoryFeatureIds = dynamic_cast<const Int32DataStore*>(&featureIdsStore);
  auto* inMemoryLargestCrossSections = dynamic_cast<Float32DataStore*>(&largestCrossSectStore);
  if(inMemoryFeatureIds != nullptr && inMemoryLargestCrossSections != nullptr)
  {
    const int32* featureIdsData = inMemoryFeatureIds->data();
    float32* largestCrossSectionsData = inMemoryLargestCrossSections->data();
    const usize sliceSize = xCells * yCells;

    if(m_InputValues->Plane == 0)
    {
      for(usize zIndex = 0; zIndex < zCells; zIndex++)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        std::fill(featureCounts.begin(), featureCounts.end(), 0.0f);

        const int32* planeFeatureIds = featureIdsData + (zIndex * sliceSize);
        for(usize planeIndex = 0; planeIndex < sliceSize; planeIndex++)
        {
          featureCounts[static_cast<usize>(planeFeatureIds[planeIndex])]++;
        }
        UpdateLargestCrossSections(featureCounts.data(), numFeatures, areaScalar, largestCrossSectionsData);
      }
      return {};
    }

    if(m_InputValues->Plane == 1)
    {
      // Workers use only the stable in-memory backing pointer and thread-local
      // feature vectors; output remains untouched until the serial reduction.
      const std::vector<float32> initialLargestCrossSections(largestCrossSectionsData, largestCrossSectionsData + numFeatures);
      XzCrossSectionThreadScratch threadScratch([numFeatures, &initialLargestCrossSections] { return XzCrossSectionScratch(numFeatures, initialLargestCrossSections); });
      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, yCells);
      parallelAlgorithm.execute(ComputeXzCrossSectionsImpl(featureIdsData, xCells, zCells, sliceSize, numFeatures, areaScalar, threadScratch, m_ShouldCancel));
      if(m_ShouldCancel)
      {
        return {};
      }
      threadScratch.combine_each([&](const XzCrossSectionScratch& scratch) {
        for(usize featureId = 1; featureId < numFeatures; featureId++)
        {
          if(scratch.largestCrossSections[featureId] > largestCrossSectionsData[featureId])
          {
            largestCrossSectionsData[featureId] = scratch.largestCrossSections[featureId];
          }
        }
      });
      return {};
    }

    if(m_InputValues->Plane == 2)
    {
      // Limit per-block counts to keep YZ scratch bounded for many features.
      constexpr usize k_MaxXPlaneBlockSize = 16;
      constexpr usize k_TargetFeatureCountEntries = 65536;
      const usize xPlaneBlockSize = std::min({xCells, k_MaxXPlaneBlockSize, std::max<usize>(1, k_TargetFeatureCountEntries / std::max<usize>(1, numFeatures))});
      featureCounts.resize(xPlaneBlockSize * numFeatures);

      for(usize xBlockStart = 0; xBlockStart < xCells; xBlockStart += xPlaneBlockSize)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        const usize activeXPlanes = std::min(xPlaneBlockSize, xCells - xBlockStart);
        std::fill_n(featureCounts.begin(), activeXPlanes * numFeatures, 0.0f);
        std::array<float32*, k_MaxXPlaneBlockSize> planeFeatureCounts = {};
        for(usize localX = 0; localX < activeXPlanes; localX++)
        {
          planeFeatureCounts[localX] = featureCounts.data() + (localX * numFeatures);
        }

        for(usize zIndex = 0; zIndex < zCells; zIndex++)
        {
          const int32* rowFeatureIds = featureIdsData + (zIndex * sliceSize) + xBlockStart;
          for(usize yIndex = 0; yIndex < yCells; yIndex++)
          {
            for(usize localX = 0; localX < activeXPlanes; localX++)
            {
              planeFeatureCounts[localX][static_cast<usize>(rowFeatureIds[localX])]++;
            }
            rowFeatureIds += xCells;
          }
        }

        for(usize localX = 0; localX < activeXPlanes; localX++)
        {
          UpdateLargestCrossSections(featureCounts.data() + (localX * numFeatures), numFeatures, areaScalar, largestCrossSectionsData);
        }
      }
      return {};
    }
  }

  for(usize planeIndex = 0; planeIndex < numPlanes; planeIndex++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    std::fill(featureCounts.begin(), featureCounts.end(), 0.0f);

    const usize planeOffset = planeIndex * planeStride;
    CountPlane(featureCounts, featureIdsStore, planeOffset, firstDimension, secondDimension, firstStride, secondStride);

    for(usize featureId = 1; featureId < numFeatures; featureId++)
    {
      const float32 area = featureCounts[featureId] * areaScalar;
      if(area > largestCrossSectStore[featureId])
      {
        largestCrossSectStore[featureId] = area;
      }
    }
  }
  return {};
}
