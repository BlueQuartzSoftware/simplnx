#include "ComputeFeatureSizes.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <nonstd/span.hpp>

#include <cmath>
#include <memory>

using namespace nx::core;

namespace
{
constexpr usize k_ChunkTuples = 65536;
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

Result<> ProcessImageGeom(ImageGeom& imageGeom, Float32AbstractDataStore& volumes, Float32AbstractDataStore& equivalentDiameters, Int32AbstractDataStore& numElements,
                          const Int32AbstractDataStore& featureIds, const bool saveElementSizes, MessageHelper& msgHelper, const std::atomic_bool& shouldCancel)
{
  ThrottledMessenger throttledMessenger = msgHelper.createThrottledMessenger();

  const usize numVoxels = featureIds.getNumberOfTuples();
  const usize numFeatures = volumes.getNumberOfTuples();

  std::vector<uint64> featureVoxelCounts(numFeatures, 0);

  msgHelper.sendMessage("Finding Voxel Counts...");
  // Count voxels per feature using chunked bulk I/O
  auto featureIdBuf = std::make_unique<int32[]>(k_ChunkTuples);
  for(usize offset = 0; offset < numVoxels; offset += k_ChunkTuples)
  {
    if(shouldCancel)
    {
      return {};
    }

    throttledMessenger.sendThrottledMessage([&] { return fmt::format(" - Counting || {:.2f}% Complete", CalculatePercentComplete(offset, numVoxels)); });

    const usize count = std::min(k_ChunkTuples, numVoxels - offset);
    featureIds.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuf.get(), count));
    for(usize i = 0; i < count; i++)
    {
      featureVoxelCounts[featureIdBuf[i]]++;
    }
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

Result<> ProcessRectGridGeom(RectGridGeom& rectGridGeom, Float32AbstractDataStore& volumes, Float32AbstractDataStore& equivalentDiameters, Int32AbstractDataStore& numElements,
                             const Int32AbstractDataStore& featureIds, const bool saveElementSizes, MessageHelper& msgHelper, const std::atomic_bool& shouldCancel)
{
  ThrottledMessenger throttledMessenger = msgHelper.createThrottledMessenger();

  const usize numVoxels = featureIds.getNumberOfTuples();
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
  // Needed for Kahan summation of volumes
  std::vector<float64> featureCompensators(numFeatures, 0.0);

  msgHelper.sendMessage("Cell Level: Finding Voxel Counts and Summing Volumes...");
  // Count voxels and sum volumes using chunked bulk I/O
  auto featureIdBuf = std::make_unique<int32[]>(k_ChunkTuples);
  auto elemSizeBuf = std::make_unique<float32[]>(k_ChunkTuples);
  for(usize offset = 0; offset < numVoxels; offset += k_ChunkTuples)
  {
    if(shouldCancel)
    {
      return {};
    }

    throttledMessenger.sendThrottledMessage([&] { return fmt::format(" - Calculating || {:.2f}% Complete", CalculatePercentComplete(offset, numVoxels)); });

    const usize count = std::min(k_ChunkTuples, numVoxels - offset);
    featureIds.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuf.get(), count));
    elemSizes.copyIntoBuffer(offset, nonstd::span<float32>(elemSizeBuf.get(), count));
    for(usize i = 0; i < count; i++)
    {
      const int32 voxelFeatureId = featureIdBuf[i];
      featureVoxelCounts[voxelFeatureId]++;

      // Use Kahan summation to determine overall volume
      float64 value = static_cast<float64>(elemSizeBuf[i]) - featureCompensators[voxelFeatureId];
      float64 volSum = featureVolumes[voxelFeatureId] + value;
      featureCompensators[voxelFeatureId] = (volSum - featureVolumes[voxelFeatureId]) - value;
      featureVolumes[voxelFeatureId] = volSum;
    }
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
