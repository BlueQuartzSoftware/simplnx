#include "ComputeFeatureSizes.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <cmath>

using namespace nx::core;

namespace
{
constexpr int32 k_BadFeatureCount = -78231;
constexpr uint64 k_MaxVoxelCount = std::numeric_limits<int32>::max();
/**
 * Volume of Sphere - `V = 4/3 * pi * r^3`
 * Radius of Sphere - `r = cubed_root(3V / 4pi)`
 * However we can cut a multiplication out of the
 * equation at runtime by isolating the `V`
 * 3V / 4pi == V / (4pi / 3)
 */
constexpr float64 k_ESDVolumeDenominator = (4.0 * nx::core::numbers::pi_v<float64>) / 3.0;
constexpr float64 k_ECDAreaDenominator = nx::core::numbers::pi_v<float64>;

class ParallelImageManager
{
public:
  explicit ParallelImageManager(std::vector<uint64>& featureVoxelCounts)
  : m_FeatureVoxelCounts(featureVoxelCounts)
  {
  }
  ~ParallelImageManager() noexcept = default;

  void execute(const SizeVec3& dims, const Int32AbstractDataStore& featureIds)
  {
    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0, dims[2]);
    dataAlg.execute(ImageSummationImpl(this, m_FeatureVoxelCounts.size(), dims, featureIds));
  }

  void updateStore(const std::vector<uint64>& featureVoxelCounts) const
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    std::transform(featureVoxelCounts.cbegin(), featureVoxelCounts.cend(), m_FeatureVoxelCounts.cbegin(), m_FeatureVoxelCounts.begin(), std::plus<>{});
  }

private:
  mutable std::mutex m_Mutex;
  std::vector<uint64>& m_FeatureVoxelCounts;

  class ImageSummationImpl
  {
  public:
    ImageSummationImpl(ParallelImageManager* manager, const usize numFeatures, const SizeVec3& dims, const Int32AbstractDataStore& featureIds)
    : m_Manager(manager)
    , m_NumFeatures(numFeatures)
    , m_Dims(dims)
    , m_FeatureIds(featureIds)
    {
    }
    ~ImageSummationImpl() noexcept = default;

    void convert(usize start, usize end) const
    {
      std::vector<uint64> featureVoxelCounts(m_NumFeatures);
      for(usize zIndex = start; zIndex < end; zIndex++)
      {
        const int64 zStride = m_Dims[0] * m_Dims[1] * zIndex;
        for(usize yIndex = 0; yIndex < m_Dims[1]; yIndex++)
        {
          const int64 yStride = m_Dims[0] * yIndex;
          for(usize xIndex = 0; xIndex < m_Dims[0]; xIndex++)
          {
            int64 voxelIdx = zStride + yStride + xIndex;
            featureVoxelCounts[m_FeatureIds.getValue(voxelIdx)]++;
          }
        }
      }
      m_Manager->updateStore(featureVoxelCounts);
    }

    void operator()(const Range& range) const
    {
      convert(range.min(), range.max());
    }

  private:
    ParallelImageManager* m_Manager;
    const usize m_NumFeatures;
    const SizeVec3& m_Dims;
    const Int32AbstractDataStore& m_FeatureIds;
  };
};

Result<> ProcessImageGeom(ImageGeom& imageGeom, Float32AbstractDataStore& volumes, Float32AbstractDataStore& equivalentDiameters, Int32AbstractDataStore& numElements,
                          const Int32AbstractDataStore& featureIds, const bool saveElementSizes, MessageHelper& msgHelper, const std::atomic_bool& shouldCancel)
{
  ThrottledMessenger throttledMessenger = msgHelper.createThrottledMessenger();

  const usize numFeatures = volumes.getNumberOfTuples();

  std::vector<uint64> featureVoxelCounts(numFeatures, 0);

  msgHelper.sendMessage("Finding Voxel Counts...");
  // Count and store the number of voxels in each feature
  ParallelImageManager manager(featureVoxelCounts);
  manager.execute(imageGeom.getDimensions(), featureIds);

  if(shouldCancel)
  {
    return {};
  }

  const FloatVec3 spacing = imageGeom.getSpacing();

  const usize xDimSize = imageGeom.getNumXCells();
  const usize yDimSize = imageGeom.getNumYCells();
  const usize zDimSize = imageGeom.getNumZCells();

  // Treat dimensions of 1 as flat for image geom
  if(xDimSize == 1 || yDimSize == 1 || zDimSize == 1)
  {
    msgHelper.sendMessage("Singular image detected. Proceeding with 2D calculations...");
    // One of the dimensions is empty, so we will be calculating area instead

    /**
     * IMPORTANT: Due the nature of ImageGeom the preflight is expected to impose a
     * restriction on the number of empty dimensions (denoted as `1`) in an input
     * ImageGeom. To illustrate why this is consider the following cases:
     *
     * An ImageGeom with 2 "empty" dimensions, such as 5x1x1. In this case the code would
     * calculate the area/volume (ie distance between points) by only using the valid dimension.
     * Functionally flattening the problem to 1D. You may think the solution is to explicitly
     * define the area cases, but there is a caveat of which of the two empty dimensions to
     * select for the area calculation. An image with 1x1x5 (XYZ) illustrates this problem,
     * would you select X or Y for the scaling for area calculation? Clearly it has been rotated,
     * but you lack the orientation information to determine the proper orientation.
     *
     * An ImageGeom with 3 "empty" dimensions, ie 1x1x1. This is a semi-ludicrous case since
     * the value can be derived directly from the spacing, but the issue previously outlined
     * will present itself once again. You cannot determine the orientation for proper area
     * calculation.
     *
     * For these two cases the following code would BREAK, so do not enable.
     **/

    // Calculate the area of a single voxel
    const float64 voxelArea = static_cast<float64>(spacing[0]) * static_cast<float64>(spacing[1]) * static_cast<float64>(spacing[2]);

    msgHelper.sendMessage("Feature Level: Storing Voxel Counts and Calculating Area and ECD...");
    // Process each feature storing feature voxel counts, areas, and equivalent circular diameter
    for(usize featureIdx = 1; featureIdx < numFeatures; featureIdx++)
    {
      if(shouldCancel)
      {
        return {};
      }

      // Check for integer overflow
      if(featureVoxelCounts[featureIdx] > k_MaxVoxelCount)
      {
        return MakeErrorResult(k_BadFeatureCount, fmt::format("Feature {} contains more voxels ({}) than the 32-bit integer limit ({}).", featureIdx, featureVoxelCounts[featureIdx], k_MaxVoxelCount));
      }

      throttledMessenger.sendThrottledMessage([&] { return fmt::format(" - Calculating || {:.2f}% Complete", CalculatePercentComplete(featureIdx, numFeatures)); });

      // Store the number of voxels in feature as int32
      numElements.setValue(featureIdx, static_cast<int32>(featureVoxelCounts[featureIdx]));

      // Calculate and store the area of the feature
      const float64 newArea = static_cast<float64>(featureVoxelCounts[featureIdx]) * voxelArea;
      volumes.setValue(featureIdx, static_cast<float32>(newArea));

      /** Determine diameter from area:
       * Area of Circle - `A = pi * r^2`
       * Radius of Circle - `r = square_root(A / pi)`
       * Diameter of Circle - `d = 2 * r`
       * Thus
       * Equivalent Circular Diameter - `2 * square_root(A / pi)`
       **/
      equivalentDiameters.setValue(featureIdx, static_cast<float32>(2.0 * std::sqrt(newArea / k_ECDAreaDenominator)));
    }
  }
  else
  {
    // If we are here, it is an image stack and thus should be treated as 3D.
    msgHelper.sendMessage("Image Stack detected. Proceeding with 3D calculations...");

    // Calculate the volume of a single voxel
    const float64 voxelVolume = spacing[0] * spacing[1] * spacing[2];

    msgHelper.sendMessage("Feature Level: Storing Voxel Counts and Calculating Volume and ESD...");
    // Process each feature storing feature voxel counts, volumes, and equivalent spherical diameter
    for(usize featureIdx = 1; featureIdx < numFeatures; featureIdx++)
    {
      if(shouldCancel)
      {
        return {};
      }

      // Check for integer overflow
      if(featureVoxelCounts[featureIdx] > k_MaxVoxelCount)
      {
        return MakeErrorResult(k_BadFeatureCount, fmt::format("Feature {} contains more voxels ({}) than the 32-bit integer limit ({}).", featureIdx, featureVoxelCounts[featureIdx], k_MaxVoxelCount));
      }

      throttledMessenger.sendThrottledMessage([&] { return fmt::format(" - Calculating || {:.2f}% Complete", CalculatePercentComplete(featureIdx, numFeatures)); });

      // Store the number of voxels in feature as int32
      numElements.setValue(featureIdx, static_cast<int32>(featureVoxelCounts[featureIdx]));

      // Calculate and store the volume of the feature
      const float64 newVolume = static_cast<float64>(featureVoxelCounts[featureIdx]) * voxelVolume;
      volumes.setValue(featureIdx, static_cast<float32>(newVolume));

      /** Determine diameter from volume:
       * Volume of Sphere - `V = 4/3 * pi * r^3`
       * Radius of Sphere - `r = cubed_root(3V / 4pi)`
       * Diameter of Sphere - `d = 2 * r`
       * Thus
       * Equivalent Spherical Diameter - `2 * cubed_root(V / (4pi / 3))`
       **/
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

class ParallelRectGridManager
{
public:
  explicit ParallelRectGridManager(std::vector<uint64>& featureVoxelCounts, std::vector<float64>& featureVolumes)
  : m_FeatureVoxelCounts(featureVoxelCounts)
  , m_FeatureVolumes(featureVolumes)
  {
  }
  ~ParallelRectGridManager() noexcept = default;

  void execute(const SizeVec3& dims, const Int32AbstractDataStore& featureIds, const Float32AbstractDataStore& elemSizes)
  {
    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0, dims[2]);
    dataAlg.execute(RectGridSummationImpl(this, m_FeatureVoxelCounts.size(), dims, featureIds, elemSizes));
  }

  void updateStore(const std::vector<uint64>& featureVoxelCounts, const std::vector<float64>& featureVolumes) const
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    std::transform(featureVoxelCounts.cbegin(), featureVoxelCounts.cend(), m_FeatureVoxelCounts.cbegin(), m_FeatureVoxelCounts.begin(), std::plus<>{});
    std::transform(featureVolumes.cbegin(), featureVolumes.cend(), m_FeatureVolumes.cbegin(), m_FeatureVolumes.begin(), std::plus<>{});
  }

private:
  mutable std::mutex m_Mutex;
  std::vector<uint64>& m_FeatureVoxelCounts;
  std::vector<float64>& m_FeatureVolumes;

  class RectGridSummationImpl
  {
  public:
    RectGridSummationImpl(ParallelRectGridManager* manager, const usize numFeatures, const SizeVec3& dims, const Int32AbstractDataStore& featureIds, const Float32AbstractDataStore& elemSizes)
    : m_Manager(manager)
    , m_NumFeatures(numFeatures)
    , m_Dims(dims)
    , m_FeatureIds(featureIds)
    , m_ElemSizes(elemSizes)
    {
    }
    ~RectGridSummationImpl() noexcept = default;

    void convert(const usize start, const usize end) const
    {
      std::vector<uint64> featureVoxelCounts(m_NumFeatures, 0);
      std::vector<float64> featureVolumes(m_NumFeatures, 0.0);
      // Needed for Kahan summation of volumes
      std::vector<float64> featureCompensators(m_NumFeatures, 0.0);
      for(usize zIndex = start; zIndex < end; zIndex++)
      {
        const int64 zStride = m_Dims[0] * m_Dims[1] * zIndex;
        for(usize yIndex = 0; yIndex < m_Dims[1]; yIndex++)
        {
          const int64 yStride = m_Dims[0] * yIndex;
          for(usize xIndex = 0; xIndex < m_Dims[0]; xIndex++)
          {
            int64 voxelIdx = zStride + yStride + xIndex;
            const int32 voxelFeatureId = m_FeatureIds.getValue(voxelIdx);
            featureVoxelCounts[voxelFeatureId]++;

            // Use Kahan summation to determine overall volume

            // Attempt to recover low order into the value. The first instance is 0
            const float64 value = static_cast<float64>(m_ElemSizes.getValue(voxelIdx)) - featureCompensators[voxelFeatureId];

            // low order may be lost
            const float64 volSum = featureVolumes[voxelFeatureId] + value;

            // recover and cache low order
            featureCompensators[voxelFeatureId] = (volSum - featureVolumes[voxelFeatureId]) - value;

            // store volumes
            featureVolumes[voxelFeatureId] = volSum;
          }
        }
      }
      m_Manager->updateStore(featureVoxelCounts, featureVolumes);
    }

    void operator()(const Range& range) const
    {
      convert(range.min(), range.max());
    }

  private:
    ParallelRectGridManager* m_Manager;
    const usize m_NumFeatures;
    const SizeVec3& m_Dims;
    const Int32AbstractDataStore& m_FeatureIds;
    const Float32AbstractDataStore& m_ElemSizes;
  };
};

Result<> ProcessRectGridGeom(RectGridGeom& rectGridGeom, Float32AbstractDataStore& volumes, Float32AbstractDataStore& equivalentDiameters, Int32AbstractDataStore& numElements,
                             const Int32AbstractDataStore& featureIds, const bool saveElementSizes, MessageHelper& msgHelper, const std::atomic_bool& shouldCancel)
{
  ThrottledMessenger throttledMessenger = msgHelper.createThrottledMessenger();

  const usize numFeatures = volumes.getNumberOfTuples();

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
  ParallelRectGridManager manager(featureVoxelCounts, featureVolumes);
  manager.execute(rectGridGeom.getDimensions(), featureIds, elemSizes);

  if(shouldCancel)
  {
    return {};
  }

  msgHelper.sendMessage("Feature Level: Storing Voxel Counts and Calculating ESD...");
  // Process each feature storing feature voxel counts and equivalent spherical diameter
  for(usize featureIdx = 1; featureIdx < numFeatures; featureIdx++)
  {
    if(shouldCancel)
    {
      return {};
    }

    throttledMessenger.sendThrottledMessage([&] { return fmt::format(" - Calculating || {:.2f}% Complete", CalculatePercentComplete(featureIdx, numFeatures)); });

    // Check for integer overflow
    if(featureVoxelCounts[featureIdx] > k_MaxVoxelCount)
    {
      return MakeErrorResult(k_BadFeatureCount, fmt::format("Feature {} contains more voxels ({}) than the 32-bit integer limit ({}).", featureIdx, featureVoxelCounts[featureIdx], k_MaxVoxelCount));
    }

    // Store the number of voxels in feature as int32
    numElements.setValue(featureIdx, static_cast<int32>(featureVoxelCounts[featureIdx]));
    // Store the volume of the feature
    volumes.setValue(featureIdx, static_cast<float32>(featureVolumes[featureIdx]));

    /** Determine diameter from volume:
     * Volume of Sphere - `V = 4/3 * pi * r^3`
     * Radius of Sphere - `r = cubed_root(3V / 4pi)`
     * Diameter of Sphere - `d = 2 * r`
     * Thus
     * Equivalent Spherical Diameter - `2 * cubed_root(V / (4pi / 3))`
     **/
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
