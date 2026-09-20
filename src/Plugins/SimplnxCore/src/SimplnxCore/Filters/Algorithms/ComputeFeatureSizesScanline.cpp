#include "ComputeFeatureSizesScanline.hpp"

#include "ComputeFeatureSizes.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <nonstd/span.hpp>

#include <cmath>
#include <memory>
#include <vector>

using namespace nx::core;

namespace
{
// Each Feature ID chunk has 262,144 tuples and uses one MiB. RectGrid adds one MiB for element sizes.
constexpr usize k_ChunkTuples = 262144;
// This error reports a feature count that cannot fit in the int32 output array.
constexpr int32 k_BadFeatureCount = -78231;
// NumElements stores int32 values.
constexpr uint64 k_MaxVoxelCount = std::numeric_limits<int32>::max();
// Equivalent spherical diameter uses d = 2*cbrt(V/(4*pi/3)).
constexpr float64 k_ESDVolumeDenominator = (4.0 * nx::core::numbers::pi_v<float64>) / 3.0;
// Equivalent circular diameter uses d = 2*sqrt(A/pi).
constexpr float64 k_ECDAreaDenominator = nx::core::numbers::pi_v<float64>;

/**
 * @brief Validates Feature ID bounds observed during a scanline pass.
 * @param featureIdsName Identifies the Feature ID array for errors.
 * @param featureAttributeMatrixPath Identifies the feature output parent.
 * @param numFeatures Identifies the feature output count.
 * @param minFeatureId Supplies the observed minimum Feature ID.
 * @param maxFeatureId Supplies the observed maximum Feature ID.
 * @return Success, or a negative or out-of-range Feature ID error.
 */
Result<> ValidateFeatureIdsInScanlinePass(const std::string& featureIdsName, const DataPath& featureAttributeMatrixPath, usize numFeatures, int32 minFeatureId, int32 maxFeatureId)
{
  if(minFeatureId < 0)
  {
    return MakeErrorResult(
        -5355, fmt::format("Feature Ids array with name '{}' has negative values within the array. The most negative value encountered was '{}'. All values must be positive within the array",
                           featureIdsName, minFeatureId));
  }

  if(maxFeatureId >= 0 && static_cast<usize>(maxFeatureId) >= numFeatures)
  {
    return MakeErrorResult(-5351, fmt::format("Feature Ids array with name '{}' has a value '{}' that would exceed the number of tuples {} in the selected Data Path: '{}'", featureIdsName,
                                              maxFeatureId, numFeatures, featureAttributeMatrixPath.toString()));
  }

  return {};
}

/**
 * @brief Writes one contiguous feature-output block.
 * @param featureStart Identifies the first feature in the block.
 * @param featureCount Number of features in the block.
 * @param[in]
 * numElementsBuffer Feature counts.
 * @param volumesBuffer Supplies feature areas or volumes.
 * @param equivalentDiametersBuffer Supplies feature equivalent diameters.
 * @param[out] numElements
 * Output counts.
 * @param volumes Receives feature areas or volumes.
 * @param equivalentDiameters Receives feature equivalent diameters.
 * @return Success, or the first output-store write error.

 */
Result<> WriteFeatureOutputBlock(usize featureStart, usize featureCount, const std::vector<int32>& numElementsBuffer, const std::vector<float32>& volumesBuffer,
                                 const std::vector<float32>& equivalentDiametersBuffer, Int32AbstractDataStore& numElements, Float32AbstractDataStore& volumes,
                                 Float32AbstractDataStore& equivalentDiameters)
{
  if(featureCount == 0)
  {
    return {};
  }

  Result<> writeResult = numElements.copyFromBuffer(featureStart, nonstd::span<const int32>(numElementsBuffer.data(), featureCount));
  if(writeResult.invalid())
  {
    return writeResult;
  }

  writeResult = volumes.copyFromBuffer(featureStart, nonstd::span<const float32>(volumesBuffer.data(), featureCount));
  if(writeResult.invalid())
  {
    return writeResult;
  }

  return equivalentDiameters.copyFromBuffer(featureStart, nonstd::span<const float32>(equivalentDiametersBuffer.data(), featureCount));
}

/**
 * @brief Calculates and writes feature outputs in bounded blocks.
 * @tparam FeatureSizeFunctorT Calculates one feature area or volume.
 * @tparam EquivalentDiameterFunctorT Calculates one
 * equivalent diameter.
 * @param numFeatures Number of output feature tuples.
 * @param featureVoxelCounts Supplies voxel counts for all features.
 * @param featureSizeFunctor Calculates an area or
 * volume for one feature.
 * @param equivalentDiameterFunctor Calculates a diameter from an area or volume.
 * @param numElements Receives feature voxel counts.
 * @param volumes Receives feature
 * areas or volumes.
 * @param equivalentDiameters Receives feature equivalent diameters.
 * @param throttledMessenger Reports feature progress.
 * @param shouldCancel Signals cancellation between
 * features or output blocks.
 * @return Success, or the first feature-count or output-store write error.
 *
 * The function flushes completed features before it returns for cancellation or an invalid
 * count.
 */
template <typename FeatureSizeFunctorT, typename EquivalentDiameterFunctorT>
Result<> StoreFeatureOutputs(usize numFeatures, const std::vector<uint64>& featureVoxelCounts, FeatureSizeFunctorT&& featureSizeFunctor, EquivalentDiameterFunctorT&& equivalentDiameterFunctor,
                             Int32AbstractDataStore& numElements, Float32AbstractDataStore& volumes, Float32AbstractDataStore& equivalentDiameters, ThrottledMessenger& throttledMessenger,
                             const std::atomic_bool& shouldCancel)
{
  std::vector<int32> numElementsBuffer(k_ChunkTuples);
  std::vector<float32> volumesBuffer(k_ChunkTuples);
  std::vector<float32> equivalentDiametersBuffer(k_ChunkTuples);
  usize featureStart = 1;
  usize featureCount = 0;

  auto flushBlock = [&]() -> Result<> {
    Result<> writeResult = WriteFeatureOutputBlock(featureStart, featureCount, numElementsBuffer, volumesBuffer, equivalentDiametersBuffer, numElements, volumes, equivalentDiameters);
    if(writeResult.valid())
    {
      featureStart += featureCount;
      featureCount = 0;
    }
    return writeResult;
  };

  for(usize featureIdx = 1; featureIdx < numFeatures; featureIdx++)
  {
    if(shouldCancel)
    {
      return flushBlock();
    }

    if(featureVoxelCounts[featureIdx] > k_MaxVoxelCount)
    {
      Result<> writeResult = flushBlock();
      if(writeResult.invalid())
      {
        return writeResult;
      }
      return MakeErrorResult(k_BadFeatureCount, fmt::format("Feature {} contains more voxels ({}) than the 32-bit integer limit ({}).", featureIdx, featureVoxelCounts[featureIdx], k_MaxVoxelCount));
    }

    throttledMessenger.sendThrottledMessage([&] { return fmt::format(" - Calculating || {:.2f}% Complete", CalculatePercentComplete(featureIdx, numFeatures)); });

    const float64 featureSize = featureSizeFunctor(featureIdx);
    numElementsBuffer[featureCount] = static_cast<int32>(featureVoxelCounts[featureIdx]);
    volumesBuffer[featureCount] = static_cast<float32>(featureSize);
    equivalentDiametersBuffer[featureCount] = static_cast<float32>(equivalentDiameterFunctor(featureSize));
    featureCount++;

    if(featureCount == k_ChunkTuples)
    {
      Result<> writeResult = flushBlock();
      if(writeResult.invalid())
      {
        return writeResult;
      }
    }
  }

  return flushBlock();
}

/**
 * @brief Computes ImageGeom feature sizes with sequential bulk reads.
 * @param imageGeom Supplies dimensions, spacing, and optional element sizes.
 * @param volumes Receives feature area or volume.
 * @param equivalentDiameters Receives equivalent circular or spherical diameter.
 * @param numElements Receives voxel counts.
 * @param featureIds Supplies Feature IDs.
 * @param featureIdsName Identifies the Feature ID array for errors.
 * @param featureAttributeMatrixPath Identifies the feature output parent.
 * @param saveElementSizes True to retain generated element sizes.
 * @param msgHelper Supplies progress messages.
 * @param shouldCancel Signals cancellation between chunks or features.
 * @return Success, or an element-size, Feature ID, or feature-count error.
 *
 * The scan traverses global raster order and returns the first Feature ID bulk-I/O error.
 */
Result<> ProcessImageGeom(ImageGeom& imageGeom, Float32AbstractDataStore& volumes, Float32AbstractDataStore& equivalentDiameters, Int32AbstractDataStore& numElements,
                          const Int32AbstractDataStore& featureIds, const std::string& featureIdsName, const DataPath& featureAttributeMatrixPath, const bool saveElementSizes,
                          MessageHelper& msgHelper, const std::atomic_bool& shouldCancel)
{
  ThrottledMessenger throttledMessenger = msgHelper.createThrottledMessenger();

  const usize numVoxels = featureIds.getNumberOfTuples();
  const usize numFeatures = volumes.getNumberOfTuples();

  std::vector<uint64> featureVoxelCounts(numFeatures, 0);
  int32 minFeatureId = std::numeric_limits<int32>::max();
  int32 maxFeatureId = std::numeric_limits<int32>::lowest();

  msgHelper.sendMessage("Finding Voxel Counts...");
  // The count pass validates Feature IDs without a second full-volume read.
  auto featureIdBuf = std::make_unique<int32[]>(k_ChunkTuples);
  for(usize offset = 0; offset < numVoxels; offset += k_ChunkTuples)
  {
    if(shouldCancel)
    {
      return {};
    }

    const usize count = std::min(k_ChunkTuples, numVoxels - offset);
    Result<> readResult = featureIds.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuf.get(), count));
    if(readResult.invalid())
    {
      return readResult;
    }
    for(usize i = 0; i < count; i++)
    {
      const int32 featureId = featureIdBuf[i];
      minFeatureId = std::min(minFeatureId, featureId);
      maxFeatureId = std::max(maxFeatureId, featureId);
      if(featureId >= 0 && static_cast<usize>(featureId) < numFeatures)
      {
        featureVoxelCounts[featureId]++;
      }
    }
  }

  Result<> validateResult = ValidateFeatureIdsInScanlinePass(featureIdsName, featureAttributeMatrixPath, numFeatures, minFeatureId, maxFeatureId);
  if(validateResult.invalid())
  {
    return validateResult;
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
    Result<> outputResult = StoreFeatureOutputs(
        numFeatures, featureVoxelCounts, [&featureVoxelCounts, voxelArea](usize featureIdx) { return static_cast<float64>(featureVoxelCounts[featureIdx]) * voxelArea; },
        [](float64 featureArea) { return 2.0 * std::sqrt(featureArea / k_ECDAreaDenominator); }, numElements, volumes, equivalentDiameters, throttledMessenger, shouldCancel);
    if(outputResult.invalid() || shouldCancel)
    {
      return outputResult;
    }
  }
  else
  {
    msgHelper.sendMessage("Image Stack detected. Proceeding with 3D calculations...");

    const float64 voxelVolume = spacing[0] * spacing[1] * spacing[2];

    msgHelper.sendMessage("Feature Level: Storing Voxel Counts and Calculating Volume and ESD...");
    Result<> outputResult = StoreFeatureOutputs(
        numFeatures, featureVoxelCounts, [&featureVoxelCounts, voxelVolume](usize featureIdx) { return static_cast<float64>(featureVoxelCounts[featureIdx]) * voxelVolume; },
        [](float64 featureVolume) { return 2.0 * std::cbrt(featureVolume / k_ESDVolumeDenominator); }, numElements, volumes, equivalentDiameters, throttledMessenger, shouldCancel);
    if(outputResult.invalid() || shouldCancel)
    {
      return outputResult;
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
 * @brief Computes RectGrid feature sizes with sequential paired reads.
 * @param rectGridGeom Supplies dimensions and generated element sizes.
 * @param volumes Receives feature volumes.
 * @param equivalentDiameters Receives equivalent spherical diameters.
 * @param numElements Receives voxel counts.
 * @param featureIds Supplies Feature IDs.
 * @param featureIdsName Identifies the Feature ID array for errors.
 * @param featureAttributeMatrixPath Identifies the feature output parent.
 * @param saveElementSizes True to retain generated element sizes.
 * @param msgHelper Supplies progress messages.
 * @param shouldCancel Signals cancellation between chunks or features.
 * @return Success, or an element-size, Feature ID, or feature-count error.
 *
 * Feature IDs and element sizes use matching chunks. The function returns the first bulk-I/O error.
 */
Result<> ProcessRectGridGeom(RectGridGeom& rectGridGeom, Float32AbstractDataStore& volumes, Float32AbstractDataStore& equivalentDiameters, Int32AbstractDataStore& numElements,
                             const Int32AbstractDataStore& featureIds, const std::string& featureIdsName, const DataPath& featureAttributeMatrixPath, const bool saveElementSizes,
                             MessageHelper& msgHelper, const std::atomic_bool& shouldCancel)
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
  // Kahan compensation retains low-order feature-volume contributions.
  std::vector<float64> featureCompensators(numFeatures, 0.0);
  int32 minFeatureId = std::numeric_limits<int32>::max();
  int32 maxFeatureId = std::numeric_limits<int32>::lowest();

  msgHelper.sendMessage("Cell Level: Finding Voxel Counts and Summing Volumes...");
  // Matching chunks validate Feature IDs and keep Kahan accumulation local.
  auto featureIdBuf = std::make_unique<int32[]>(k_ChunkTuples);
  auto elemSizeBuf = std::make_unique<float32[]>(k_ChunkTuples);
  for(usize offset = 0; offset < numVoxels; offset += k_ChunkTuples)
  {
    if(shouldCancel)
    {
      return {};
    }

    const usize count = std::min(k_ChunkTuples, numVoxels - offset);
    Result<> readResult = featureIds.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuf.get(), count));
    if(readResult.invalid())
    {
      return readResult;
    }
    readResult = elemSizes.copyIntoBuffer(offset, nonstd::span<float32>(elemSizeBuf.get(), count));
    if(readResult.invalid())
    {
      return readResult;
    }
    for(usize i = 0; i < count; i++)
    {
      const int32 voxelFeatureId = featureIdBuf[i];
      minFeatureId = std::min(minFeatureId, voxelFeatureId);
      maxFeatureId = std::max(maxFeatureId, voxelFeatureId);
      if(voxelFeatureId >= 0 && static_cast<usize>(voxelFeatureId) < numFeatures)
      {
        featureVoxelCounts[voxelFeatureId]++;

        float64 value = static_cast<float64>(elemSizeBuf[i]) - featureCompensators[voxelFeatureId];
        float64 volSum = featureVolumes[voxelFeatureId] + value;
        featureCompensators[voxelFeatureId] = (volSum - featureVolumes[voxelFeatureId]) - value;
        featureVolumes[voxelFeatureId] = volSum;
      }
    }
  }

  Result<> validateResult = ValidateFeatureIdsInScanlinePass(featureIdsName, featureAttributeMatrixPath, numFeatures, minFeatureId, maxFeatureId);
  if(validateResult.invalid())
  {
    return validateResult;
  }

  msgHelper.sendMessage("Feature Level: Storing Voxel Counts and Calculating ESD...");
  Result<> outputResult = StoreFeatureOutputs(
      numFeatures, featureVoxelCounts, [&featureVolumes](usize featureIdx) { return featureVolumes[featureIdx]; },
      [](float64 featureVolume) { return 2.0 * std::cbrt(featureVolume / k_ESDVolumeDenominator); }, numElements, volumes, equivalentDiameters, throttledMessenger, shouldCancel);
  if(outputResult.invalid() || shouldCancel)
  {
    return outputResult;
  }

  if(!saveElementSizes)
  {
    msgHelper.sendMessage("Cleaning Up Element Sizes...");
    rectGridGeom.deleteElementSizes();
  }

  return {};
}
} // namespace

ComputeFeatureSizesScanline::ComputeFeatureSizesScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                         const ComputeFeatureSizesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeFeatureSizesScanline::~ComputeFeatureSizesScanline() noexcept = default;

Result<> ComputeFeatureSizesScanline::operator()()
{
  MessageHelper messageHelper(m_MessageHandler);

  const bool saveElementSizes = m_InputValues->SaveElementSizes;

  messageHelper.sendMessage("Validating Feature Ids and Feature Attribute Matrix...");
  const DataPath featureIdsArrayPath = m_InputValues->FeatureIdsPath;
  const auto* featureIdsArrayPtr = m_DataStructure.getDataAs<Int32Array>(featureIdsArrayPath);

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
    return ProcessImageGeom(imageGeom, volumes, equivalentDiameters, numElements, featureIds, featureIdsArrayPtr->getName(), featureAttributeMatrixPath, saveElementSizes, messageHelper,
                            m_ShouldCancel);
  }
  if(geomType == IGeometry::Type::RectGrid)
  {
    messageHelper.sendMessage("Beginning Processing Features in Rectilinear Grid Geometry...");
    auto& rectGridGeom = dynamic_cast<RectGridGeom&>(geom);
    return ProcessRectGridGeom(rectGridGeom, volumes, equivalentDiameters, numElements, featureIds, featureIdsArrayPtr->getName(), featureAttributeMatrixPath, saveElementSizes, messageHelper,
                               m_ShouldCancel);
  }

  return {};
}
