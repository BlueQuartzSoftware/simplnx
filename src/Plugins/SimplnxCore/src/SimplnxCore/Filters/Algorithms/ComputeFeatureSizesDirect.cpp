#include "ComputeFeatureSizesDirect.hpp"

#include "ComputeFeatureSizes.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <tbb/combinable.h>

#include <cmath>

using namespace nx::core;

namespace
{
// This error reports a feature count that cannot fit in the int32 output array.
constexpr int32 k_BadFeatureCount = -78231;
// NumElements stores int32 values.
constexpr uint64 k_MaxVoxelCount = std::numeric_limits<int32>::max();
// Equivalent spherical diameter uses d = 2*cbrt(V/(4*pi/3)).
constexpr float64 k_ESDVolumeDenominator = (4.0 * nx::core::numbers::pi_v<float64>) / 3.0;
// Equivalent circular diameter uses d = 2*sqrt(A/pi).
constexpr float64 k_ECDAreaDenominator = nx::core::numbers::pi_v<float64>;

/**
 * @brief Defines thread-local feature voxel counts.
 */
using FeatureVoxelCountsT = tbb::combinable<std::vector<uint64>>;
/**
 * @brief Defines thread-local feature volume sums.
 */
using FeatureVolumesT = tbb::combinable<std::vector<float64>>;

/**
 * @class ImageSummationImpl
 * @brief Counts ImageGeom voxels for each feature.
 *
 * Workers use private count vectors. The caller controls parallel scheduling.
 */
class ImageSummationImpl
{
public:
  /**
   * @brief Initializes an ImageGeom count worker.
   * @param voxelCounts Receives thread-local feature counts.
   * @param dims Supplies ImageGeom dimensions.
   * @param featureIds Supplies Feature IDs.
   * @param shouldCancel Signals cancellation between Z slices.
   * @pre All arguments outlive the worker execution.
   */
  ImageSummationImpl(FeatureVoxelCountsT& voxelCounts, const SizeVec3& dims, const Int32AbstractDataStore& featureIds, const std::atomic_bool& shouldCancel)
  : m_VoxelCounts(voxelCounts)
  , m_Dims(dims)
  , m_FeatureIds(featureIds)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Counts features in one Z-slice interval.
   * @param start Identifies the first Z slice.
   * @param end Identifies one past the last Z slice.
   */
  void convert(const usize start, const usize end) const
  {
    std::vector<uint64>& threadLocalVoxelCounts = m_VoxelCounts.local();
    for(usize zIndex = start; zIndex < end; zIndex++)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      const int64 zStride = m_Dims[0] * m_Dims[1] * zIndex;
      for(usize yIndex = 0; yIndex < m_Dims[1]; yIndex++)
      {
        const int64 yStride = m_Dims[0] * yIndex;
        for(usize xIndex = 0; xIndex < m_Dims[0]; xIndex++)
        {
          const int64 voxelIdx = zStride + yStride + xIndex;
          threadLocalVoxelCounts[m_FeatureIds.getValue(voxelIdx)]++;
        }
      }
    }
  }

  /**
   * @brief Counts features in an assigned Z range.
   * @param range Identifies the Z-slice interval.
   */
  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }

private:
  FeatureVoxelCountsT& m_VoxelCounts;
  const SizeVec3& m_Dims;
  const Int32AbstractDataStore& m_FeatureIds;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @brief Computes ImageGeom feature sizes with direct feature access.
 * @param imageGeom Supplies dimensions, spacing, and optional element sizes.
 * @param volumes Receives feature area or volume.
 * @param equivalentDiameters Receives equivalent circular or spherical diameter.
 * @param numElements Receives voxel counts.
 * @param featureIds Supplies Feature IDs.
 * @param saveElementSizes True to retain generated element sizes.
 * @param msgHelper Supplies progress messages.
 * @param shouldCancel Signals cancellation between Z slices or features.
 * @return Success, or an element-size or feature-count error.
 *
 * requireStoresInMemory() only disables parallel scheduling for a nonresident Feature ID store.
 * It does not pin or synchronize store access.
 */
Result<> ProcessImageGeom(ImageGeom& imageGeom, Float32AbstractDataStore& volumes, Float32AbstractDataStore& equivalentDiameters, Int32AbstractDataStore& numElements,
                          const Int32AbstractDataStore& featureIds, const bool saveElementSizes, MessageHelper& msgHelper, const std::atomic_bool& shouldCancel)
{
  ThrottledMessenger throttledMessenger = msgHelper.createThrottledMessenger();

  const usize numFeatures = volumes.getNumberOfTuples();
  SizeVec3 dims = imageGeom.getDimensions();

  std::vector<uint64> featureVoxelCounts(numFeatures, 0);

  msgHelper.sendMessage("Finding Voxel Counts...");
  // Count each feature with thread-local Z-slice accumulators.
  FeatureVoxelCountsT threadLocalVoxelCounts([numFeatures] { return std::vector<uint64>(numFeatures, 0); });
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, dims[2]);
  // A nonresident Feature ID store disables parallel scheduling. This does not pin or synchronize access.
  IParallelAlgorithm::AlgorithmStores algStores;
  algStores.push_back(&featureIds);
  dataAlg.requireStoresInMemory(algStores);
  dataAlg.execute(ImageSummationImpl(threadLocalVoxelCounts, dims, featureIds, shouldCancel));

  if(shouldCancel)
  {
    return {};
  }

  // Reduce thread-local counts after all Z ranges complete.
  threadLocalVoxelCounts.combine_each(
      [&](const std::vector<uint64>& localCounts) { std::transform(localCounts.cbegin(), localCounts.cend(), featureVoxelCounts.cbegin(), featureVoxelCounts.begin(), std::plus{}); });

  if(shouldCancel)
  {
    return {};
  }

  const FloatVec3 spacing = imageGeom.getSpacing();

  const usize xDimSize = imageGeom.getNumXCells();
  const usize yDimSize = imageGeom.getNumYCells();
  const usize zDimSize = imageGeom.getNumZCells();

  // A unit dimension selects flat ImageGeom area calculation.
  if(xDimSize == 1 || yDimSize == 1 || zDimSize == 1)
  {
    msgHelper.sendMessage("Singular image detected. Proceeding with 2D calculations...");
    // Preflight permits one unit dimension. More unit dimensions do not identify a unique area plane.
    // The slab convention matches ImageGeom::findElementSizes.
    // It differs from DREAM3D 6.5.171 when flat spacing is not one.
    // The [2DFlatSpacing] V&V case records this compatibility difference.
    const float64 voxelArea = static_cast<float64>(spacing[0]) * static_cast<float64>(spacing[1]) * static_cast<float64>(spacing[2]);

    msgHelper.sendMessage("Feature Level: Storing Voxel Counts and Calculating Area and ECD...");
    for(usize featureIdx = 1; featureIdx < numFeatures; featureIdx++)
    {
      if(shouldCancel)
      {
        return {};
      }

      if(featureVoxelCounts[featureIdx] > k_MaxVoxelCount)
      {
        return MakeErrorResult(k_BadFeatureCount, fmt::format("Feature {} contains more voxels ({}) than the 32-bit integer limit ({}).", featureIdx, featureVoxelCounts[featureIdx], k_MaxVoxelCount));
      }

      throttledMessenger.sendThrottledMessage([&] { return fmt::format(" - Calculating || {:.2f}% Complete", CalculatePercentComplete(featureIdx, numFeatures)); });

      numElements.setValue(featureIdx, static_cast<int32>(featureVoxelCounts[featureIdx]));

      const float64 newArea = static_cast<float64>(featureVoxelCounts[featureIdx]) * voxelArea;
      volumes.setValue(featureIdx, static_cast<float32>(newArea));

      equivalentDiameters.setValue(featureIdx, static_cast<float32>(2.0 * std::sqrt(newArea / k_ECDAreaDenominator)));
    }
  }
  else
  {
    msgHelper.sendMessage("Image Stack detected. Proceeding with 3D calculations...");

    const float64 voxelVolume = spacing[0] * spacing[1] * spacing[2];

    msgHelper.sendMessage("Feature Level: Storing Voxel Counts and Calculating Volume and ESD...");
    for(usize featureIdx = 1; featureIdx < numFeatures; featureIdx++)
    {
      if(shouldCancel)
      {
        return {};
      }

      if(featureVoxelCounts[featureIdx] > k_MaxVoxelCount)
      {
        return MakeErrorResult(k_BadFeatureCount, fmt::format("Feature {} contains more voxels ({}) than the 32-bit integer limit ({}).", featureIdx, featureVoxelCounts[featureIdx], k_MaxVoxelCount));
      }

      throttledMessenger.sendThrottledMessage([&] { return fmt::format(" - Calculating || {:.2f}% Complete", CalculatePercentComplete(featureIdx, numFeatures)); });

      numElements.setValue(featureIdx, static_cast<int32>(featureVoxelCounts[featureIdx]));

      const float64 newVolume = static_cast<float64>(featureVoxelCounts[featureIdx]) * voxelVolume;
      volumes.setValue(featureIdx, static_cast<float32>(newVolume));

      equivalentDiameters.setValue(featureIdx, static_cast<float32>(2.0 * std::cbrt(newVolume / k_ESDVolumeDenominator)));
    }
  }

  if(saveElementSizes)
  {
    msgHelper.sendMessage("Calculating Element Sizes...");
    return imageGeom.findElementSizes(false);
  }

  return {};
}

/**
 * @class RectGridSummationImpl
 * @brief Counts RectGrid cells and sums their element volumes.
 *
 * Workers use thread-local counts, sums, and Kahan compensators.
 */
class RectGridSummationImpl
{
public:
  /**
   * @brief Initializes a RectGrid summation worker.
   * @param voxelCounts Receives thread-local feature counts.
   * @param volumes Receives thread-local feature volume sums.
   * @param dims Supplies RectGrid dimensions.
   * @param featureIds Supplies Feature IDs.
   * @param elemSizes Supplies generated element volumes.
   * @param shouldCancel Signals cancellation between Z slices.
   * @pre All arguments outlive the worker execution.
   */
  RectGridSummationImpl(FeatureVoxelCountsT& voxelCounts, FeatureVolumesT& volumes, const SizeVec3& dims, const Int32AbstractDataStore& featureIds, const Float32AbstractDataStore& elemSizes,
                        const std::atomic_bool& shouldCancel)
  : m_VoxelCounts(voxelCounts)
  , m_Volumes(volumes)
  , m_Dims(dims)
  , m_FeatureIds(featureIds)
  , m_ElemSizes(elemSizes)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Sums one Z-slice interval.
   * @param start Identifies the first Z slice.
   * @param end Identifies one past the last Z slice.
   */
  void convert(const usize start, const usize end) const
  {
    std::vector<uint64>& threadLocalVoxelCounts = m_VoxelCounts.local();
    std::vector<float64>& threadLocalVolumes = m_Volumes.local();

    // Kahan compensation retains low-order volume contributions.
    std::vector<float64> featureCompensators(threadLocalVolumes.size(), 0.0);
    for(usize zIndex = start; zIndex < end; zIndex++)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      const int64 zStride = m_Dims[0] * m_Dims[1] * zIndex;
      for(usize yIndex = 0; yIndex < m_Dims[1]; yIndex++)
      {
        const int64 yStride = m_Dims[0] * yIndex;
        for(usize xIndex = 0; xIndex < m_Dims[0]; xIndex++)
        {
          const int64 voxelIdx = zStride + yStride + xIndex;
          const int32 voxelFeatureId = m_FeatureIds.getValue(voxelIdx);
          threadLocalVoxelCounts[voxelFeatureId]++;

          const float64 value = static_cast<float64>(m_ElemSizes.getValue(voxelIdx)) - featureCompensators[voxelFeatureId];
          const float64 volSum = threadLocalVolumes[voxelFeatureId] + value;
          featureCompensators[voxelFeatureId] = (volSum - threadLocalVolumes[voxelFeatureId]) - value;
          threadLocalVolumes[voxelFeatureId] = volSum;
        }
      }
    }
  }

  /**
   * @brief Sums an assigned Z range.
   * @param range Identifies the Z-slice interval.
   */
  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }

private:
  FeatureVoxelCountsT& m_VoxelCounts;
  FeatureVolumesT& m_Volumes;
  const SizeVec3& m_Dims;
  const Int32AbstractDataStore& m_FeatureIds;
  const Float32AbstractDataStore& m_ElemSizes;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @brief Computes RectGrid feature sizes with direct feature access.
 * @param rectGridGeom Supplies dimensions and generated element sizes.
 * @param volumes Receives feature volumes.
 * @param equivalentDiameters Receives equivalent spherical diameters.
 * @param numElements Receives voxel counts.
 * @param featureIds Supplies Feature IDs.
 * @param saveElementSizes True to retain generated element sizes.
 * @param msgHelper Supplies progress messages.
 * @param shouldCancel Signals cancellation between Z slices or features.
 * @return Success, or an element-size or feature-count error.
 *
 * requireStoresInMemory() considers Feature IDs only. It does not pin or synchronize Feature IDs
 * or element sizes.
 */
Result<> ProcessRectGridGeom(RectGridGeom& rectGridGeom, Float32AbstractDataStore& volumes, Float32AbstractDataStore& equivalentDiameters, Int32AbstractDataStore& numElements,
                             const Int32AbstractDataStore& featureIds, const bool saveElementSizes, MessageHelper& msgHelper, const std::atomic_bool& shouldCancel)
{
  ThrottledMessenger throttledMessenger = msgHelper.createThrottledMessenger();

  const usize numFeatures = volumes.getNumberOfTuples();
  SizeVec3 dims = rectGridGeom.getDimensions();

  msgHelper.sendMessage("Finding Element Sizes...");
  Result<> result = rectGridGeom.findElementSizes(false);
  if(result.invalid())
  {
    return result;
  }

  const Float32AbstractDataStore& elemSizes = rectGridGeom.getElementSizes()->getDataStoreRef();

  std::vector<uint64> featureVoxelCounts(numFeatures, 0);
  std::vector<float64> featureVolumes(numFeatures, 0.0);

  msgHelper.sendMessage("Cell Level: Finding Voxel Counts and Summing Volumes...");
  // Count each feature and sum element volumes with thread-local Z ranges.
  FeatureVoxelCountsT threadLocalVoxelCounts([numFeatures] { return std::vector<uint64>(numFeatures, 0); });
  FeatureVolumesT threadLocalVolumes([numFeatures] { return std::vector<float64>(numFeatures, 0); });
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, dims[2]);
  // The scheduling guard includes Feature IDs only. It does not pin or synchronize element sizes.
  IParallelAlgorithm::AlgorithmStores algStores;
  algStores.push_back(&featureIds);
  dataAlg.requireStoresInMemory(algStores);
  dataAlg.execute(RectGridSummationImpl(threadLocalVoxelCounts, threadLocalVolumes, dims, featureIds, elemSizes, shouldCancel));

  if(shouldCancel)
  {
    return {};
  }

  // Reduce thread-local voxel counts after all Z ranges complete.
  threadLocalVoxelCounts.combine_each(
      [&](const std::vector<uint64>& localCounts) { std::transform(localCounts.cbegin(), localCounts.cend(), featureVoxelCounts.cbegin(), featureVoxelCounts.begin(), std::plus{}); });
  // Combine thread-local volume sums with Kahan compensation.
  std::vector<float64> featureCompensators(numFeatures, 0.0);
  threadLocalVolumes.combine_each([&](const std::vector<float64>& localVolumes) {
    for(usize featureIdx = 0; featureIdx < localVolumes.size(); featureIdx++)
    {
      const float64 value = localVolumes[featureIdx] - featureCompensators[featureIdx];
      const float64 volSum = featureVolumes[featureIdx] + value;
      featureCompensators[featureIdx] = (volSum - featureVolumes[featureIdx]) - value;
      featureVolumes[featureIdx] = volSum;
    }
  });

  if(shouldCancel)
  {
    return {};
  }

  msgHelper.sendMessage("Feature Level: Storing Voxel Counts and Calculating ESD...");
  for(usize featureIdx = 1; featureIdx < numFeatures; featureIdx++)
  {
    if(shouldCancel)
    {
      return {};
    }

    throttledMessenger.sendThrottledMessage([&] { return fmt::format(" - Calculating || {:.2f}% Complete", CalculatePercentComplete(featureIdx, numFeatures)); });

    if(featureVoxelCounts[featureIdx] > k_MaxVoxelCount)
    {
      return MakeErrorResult(k_BadFeatureCount, fmt::format("Feature {} contains more voxels ({}) than the 32-bit integer limit ({}).", featureIdx, featureVoxelCounts[featureIdx], k_MaxVoxelCount));
    }

    numElements.setValue(featureIdx, static_cast<int32>(featureVoxelCounts[featureIdx]));
    volumes.setValue(featureIdx, static_cast<float32>(featureVolumes[featureIdx]));
    equivalentDiameters.setValue(featureIdx, static_cast<float32>(2.0 * std::cbrt(featureVolumes[featureIdx] / k_ESDVolumeDenominator)));
  }

  if(!saveElementSizes)
  {
    msgHelper.sendMessage("Cleaning Up Element Sizes...");
    rectGridGeom.deleteElementSizes();
  }

  return {};
}
} // namespace

ComputeFeatureSizesDirect::ComputeFeatureSizesDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                     const ComputeFeatureSizesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeFeatureSizesDirect::~ComputeFeatureSizesDirect() noexcept = default;

Result<> ComputeFeatureSizesDirect::operator()()
{
  MessageHelper messageHelper(m_MessageHandler);

  const bool saveElementSizes = m_InputValues->SaveElementSizes;

  messageHelper.sendMessage("Validating Feature Ids and Feature Attribute Matrix...");
  const DataPath featureIdsArrayPath = m_InputValues->FeatureIdsPath;
  const auto* featureIdsArrayPtr = m_DataStructure.getDataAs<Int32Array>(featureIdsArrayPath);
  {
    const DataPath featureAttributeMatrixPath = m_InputValues->FeatureAttributeMatrixPath;
    Result<> validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, featureAttributeMatrixPath, *featureIdsArrayPtr, false, m_MessageHandler);
    if(validateNumFeatResult.invalid())
    {
      return validateNumFeatResult;
    }
  }

  const DataPath geomPath = m_InputValues->InputImageGeometryPath;
  auto& geom = m_DataStructure.getDataRefAs<IGeometry>(geomPath);
  const auto& featureIds = featureIdsArrayPtr->getDataStoreRef();

  const DataPath featureAttributeMatrixPath = m_InputValues->FeatureAttributeMatrixPath;
  const DataPath volumesPath = featureAttributeMatrixPath.createChildPath(m_InputValues->VolumesName);
  const DataPath equivDiamPath = featureAttributeMatrixPath.createChildPath(m_InputValues->EquivalentDiametersName);
  const DataPath numElementsPath = featureAttributeMatrixPath.createChildPath(m_InputValues->NumElementsName);

  auto& volumes = m_DataStructure.getDataAs<Float32Array>(volumesPath)->getDataStoreRef();
  auto& equivalentDiameters = m_DataStructure.getDataAs<Float32Array>(equivDiamPath)->getDataStoreRef();
  auto& numElements = m_DataStructure.getDataAs<Int32Array>(numElementsPath)->getDataStoreRef();

  const IGeometry::Type geomType = geom.getGeomType();
  if(geomType == IGeometry::Type::Image)
  {
    messageHelper.sendMessage("Beginning Processing Features in Image Geometry...");
    auto& imageGeom = dynamic_cast<ImageGeom&>(geom);
    return ProcessImageGeom(imageGeom, volumes, equivalentDiameters, numElements, featureIds, saveElementSizes, messageHelper, m_ShouldCancel);
  }
  if(geomType == IGeometry::Type::RectGrid)
  {
    messageHelper.sendMessage("Beginning Processing Features in Rectilinear Grid Geometry...");
    auto& rectGridGeom = dynamic_cast<RectGridGeom&>(geom);
    return ProcessRectGridGeom(rectGridGeom, volumes, equivalentDiameters, numElements, featureIds, saveElementSizes, messageHelper, m_ShouldCancel);
  }

  return {};
}
