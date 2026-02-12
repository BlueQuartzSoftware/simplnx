#include "ComputeFeatureSizes.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <cmath>

using namespace nx::core;

namespace
{
constexpr int32 k_BadFeatureCount = -78231;
constexpr uint64 k_MaxVoxelCount = std::numeric_limits<int32>::max();
constexpr float32 k_ESDVolumeDenominator = (4.0f / 3.0f) * nx::core::numbers::pi_v<float32>;
constexpr float32 k_ECDAreaDenominator = nx::core::numbers::pi_v<float32>;

Result<> ProcessImageGeom(ImageGeom& imageGeom, Float32AbstractDataStore& volumes, Float32AbstractDataStore& equivalentDiameters, Int32AbstractDataStore& numElements,
                          const Int32AbstractDataStore& featureIds, const bool saveElementSizes, MessageHelper& msgHelper, const std::atomic_bool& shouldCancel)
{
  ThrottledMessenger throttledMessenger = msgHelper.createThrottledMessenger();

  const usize numVoxels = featureIds.getNumberOfTuples();

  msgHelper.sendMessage("Finding Max Feature Id...");
  const usize maxFeatureIdx = *std::max_element(featureIds.cbegin(), featureIds.cend());
  const usize numFeatures = maxFeatureIdx + 1;

  std::vector<uint64> featureVoxelCounts(numFeatures, 0);

  msgHelper.sendMessage("Finding Max Feature Id...");
  // Count and store the number of voxels in each feature
  for(usize voxelIdx = 0; voxelIdx < numVoxels; voxelIdx++)
  {
    if(shouldCancel)
    {
      return {};
    }

    throttledMessenger.sendThrottledMessage([&] { return fmt::format(" - Counting || {:.2f}% Complete", CalculatePercentComplete(voxelIdx, numVoxels)); });

    featureVoxelCounts[featureIds.getValue(voxelIdx)]++;
  }

  const FloatVec3 spacing = imageGeom.getSpacing();

  const usize xDimSize = imageGeom.getNumXCells();
  const usize yDimSize = imageGeom.getNumYCells();
  const usize zDimSize = imageGeom.getNumZCells();

  // Treat dimensions of 1 as flat for image geom
  if(xDimSize == 1 || yDimSize == 1 || zDimSize == 1)
  {
    msgHelper.sendMessage("Singular image detected. Proceeding with 2D calculations...");
    // One of the dimensions is empty so we will be calculating area instead

    /**
     * IMPORTANT: Due the nature of ImageGeom the preflight is expected to impose a
     * restriction on the number of empty dimensions (dentoted as `1`) in an input
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
     * the value can be derived directly from the spcaing, but the issue previously outlined
     * will present itself once again. You cannot determine the orientation for proper area
     * calcualtion.
     *
     * For these two cases the following code would BREAK, so do not enable.
     **/

    // if x dimension has a size of 1 then xSpacing = 1; else xSpacing = spacing[0]
    const float32 xSpacing = (spacing[0] * static_cast<float32>(xDimSize > 1ULL)) + (1.0f * static_cast<float32>(xDimSize < 2ULL));
    // if y dimension has a size of 1 then ySpacing = 1; else ySpacing = spacing[1]
    const float32 ySpacing = (spacing[1] * static_cast<float32>(yDimSize > 1ULL)) + (1.0f * static_cast<float32>(yDimSize < 2ULL));
    // if z dimension has a size of 1 then zSpacing = 1; else zSpacing = spacing[2]
    const float32 zSpacing = (spacing[2] * static_cast<float32>(zDimSize > 1ULL)) + (1.0f * static_cast<float32>(zDimSize < 2ULL));

    // Calculate the area of a single voxel
    const float32 voxelArea = xSpacing * ySpacing * zSpacing;

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
      const float32 newArea = static_cast<float32>(featureVoxelCounts[featureIdx]) * voxelArea;
      volumes.setValue(featureIdx, newArea);

      /** Determine diameter from area:
       * Area of Circle - `A = pi * r^2`
       * Radius of Circle - `r = square_root(A / pi)`
       * Diameter of Circle - `d = 2 * r`
       * Thus
       * Equivalent Circular Diameter - `2 * square_root(A / pi)`
       **/
      equivalentDiameters.setValue(featureIdx, 2 * std::sqrt(newArea / k_ECDAreaDenominator));
    }
  }
  else
  {
    // If we are here it is an image stack and thus should be treated as 3D
    msgHelper.sendMessage("Image Stack detected. Proceeding with 3D calculations...");

    // Calculate the volume of a single voxel
    const float32 voxelVolume = spacing[0] * spacing[1] * spacing[2];

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
      const float32 newVolume = static_cast<float32>(featureVoxelCounts[featureIdx]) * voxelVolume;
      volumes.setValue(featureIdx, newVolume);

      /** Determine diameter from volume:
       * Volume of Sphere - `V = 4/3 * pi * r^3`
       * Radius of Sphere - `r = cubed_root(V / (4/3 * pi))`
       * Diameter of Sphere - `d = 2 * r`
       * Thus
       * Equivalent Spherical Diameter - `2 * cubed_root(V / (4/3 * pi))`
       **/
      equivalentDiameters.setValue(featureIdx, 2.0f * std::cbrt(newVolume / k_ESDVolumeDenominator));
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

  msgHelper.sendMessage("Cell Level: Finding Voxel Counts and Summing Volumes...");
  // Count and store the number of voxels in each feature
  for(usize voxelIdx = 0; voxelIdx < numVoxels; voxelIdx++)
  {
    if(shouldCancel)
    {
      return {};
    }

    throttledMessenger.sendThrottledMessage([&] { return fmt::format(" - Calculating || {:.2f}% Complete", CalculatePercentComplete(voxelIdx, numVoxels)); });

    const int32 voxelFeatureId = featureIds.getValue(voxelIdx);
    featureVoxelCounts[featureIds.getValue(voxelIdx)] += 1.0f;

    // Use summation to determine overall volume
    const float32 temp2 = volumes.getValue(voxelFeatureId);
    volumes.setValue(voxelFeatureId, temp2 + elemSizes.getValue(voxelIdx));
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

    /** Determine diameter from volume:
     * Volume of Sphere - `V = 4/3 * pi * r^3`
     * Radius of Sphere - `r = cubed_root(V / (4/3 * pi))`
     * Diameter of Sphere - `d = 2 * r`
     * Thus
     * Equivalent Spherical Diameter - `2 * cubed_root(V / (4/3 * pi))`
     **/
    equivalentDiameters.setValue(featureIdx, 2.0f * std::cbrt(volumes.getValue(featureIdx) / k_ESDVolumeDenominator));
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
