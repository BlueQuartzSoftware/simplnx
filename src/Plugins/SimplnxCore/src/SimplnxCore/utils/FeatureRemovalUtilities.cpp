#include "SimplnxCore/utils/FeatureRemovalUtilities.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

#include <algorithm>
#include <array>

using namespace nx::core;

namespace
{
constexpr int32 k_AllFeaturesFlaggedError = -45433;
constexpr int32 k_RemoveInactiveObjectsError = -45434;
constexpr int32 k_FeatureIdOutOfRangeError = -45435;
constexpr int32 k_NoFillProgressError = -45436;
constexpr int32 k_TupleCountMismatchError = -45437;
constexpr int32 k_FeatureIdsCannotBeIgnoredWarning = -45438;

/// Number of cells between cancel polls in the validation pass.
constexpr usize k_CancelPollStride = 1ULL << 20;

/**
 * @brief Chooses a fill source for every vacated cell.
 *
 * A vacated cell is one whose FeatureId is negative. Its six face neighbors are polled in the order
 * -Z, -Y, -X, +X, +Y, +Z. Every non-negative neighbor FeatureId is tallied, background (0) included,
 * and the source becomes the neighbor whose feature first reaches the highest tally. The first
 * sighting counts as one hit, so a cell whose valid neighbors all belong to distinct features gets
 * a source in this pass. A cell with no non-negative neighbor gets no source this pass and is
 * retried on the next pass, after its own neighbors have been filled.
 *
 * Cells with FeatureId 0 are background. They are never fill targets, so they are not counted as
 * unresolved. They are legal fill sources, which matches DREAM3D 6.5.171.
 *
 * @param imageGeom Supplies the volume dimensions.
 * @param featureIds Cell FeatureIds. Negative values mark vacated cells. Not modified.
 * @param storageArray Receives, for each vacated cell, the index of the neighbor cell to copy from.
 * Entries for cells that get no source are left unchanged.
 * @param shouldCancel Polled once per Z slice. When set the scan returns early.
 * @param messageHelper Throttled progress messages.
 * @return The number of vacated cells seen in this pass. Zero means the fill is complete.
 */
usize IdentifyNeighbors(const ImageGeom& imageGeom, const Int32AbstractDataStore& featureIds, std::vector<int32>& storageArray, const std::atomic_bool& shouldCancel, MessageHelper& messageHelper)
{
  ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger();

  SizeVec3 uDims = imageGeom.getDimensions();

  std::array<int64, 3> dims = {
      static_cast<int64>(uDims[0]),
      static_cast<int64>(uDims[1]),
      static_cast<int64>(uDims[2]),
  };

  constexpr FaceNeighborType k_NumFaceNeighbors = VoxelNeighbors<Image3D>::k_FaceNeighborCount;
  const std::array<int64, k_NumFaceNeighbors> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  constexpr std::array<FaceNeighborType, k_NumFaceNeighbors> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  usize unresolvedCellCount = 0;

  auto progressIncrement = dims[2] / 100;
  usize progressCounter = 0;
  for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
  {
    if(shouldCancel)
    {
      return unresolvedCellCount;
    }

    if(progressCounter > progressIncrement)
    {
      throttledMessenger.sendThrottledMessage([&]() { return fmt::format("Processing Image... {:.2f}%", CalculatePercentComplete(zIdx, dims[2])); });
      progressCounter = 0;
    }
    progressCounter++;

    const int64 kStride = dims[0] * dims[1] * zIdx;
    for(int64 yIdx = 0; yIdx < dims[1]; yIdx++)
    {
      const int64 jStride = dims[0] * yIdx;
      for(int64 xIdx = 0; xIdx < dims[0]; xIdx++)
      {
        const int64 voxelIndex = kStride + jStride + xIdx;
        const int32 featureName = featureIds[voxelIndex];
        // Only vacated cells (negative) need a source. Background (0) is not a fill target; counting
        // it as unresolved would keep the caller's loop alive forever because nothing overwrites it.
        if(featureName >= 0)
        {
          continue;
        }
        unresolvedCellCount++;

        // At most six distinct features can be seen, so fixed storage avoids a heap allocation per cell.
        int32 most = 0;
        std::array<int32, k_NumFaceNeighbors> numHits{};
        std::array<int32, k_NumFaceNeighbors> discoveredFeatures{};
        usize discoveredCount = 0;

        const std::array<bool, k_NumFaceNeighbors> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
        for(const auto& faceIndex : faceNeighborInternalIdx)
        {
          if(!isValidFaceNeighbor[faceIndex])
          {
            continue;
          }

          const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];
          const int32 feature = featureIds[neighborPoint];
          if(feature < 0)
          {
            continue;
          }

          usize featIndex = 0;
          while(featIndex < discoveredCount && discoveredFeatures[featIndex] != feature)
          {
            featIndex++;
          }
          if(featIndex == discoveredCount)
          {
            discoveredFeatures[discoveredCount] = feature;
            numHits[discoveredCount] = 0;
            discoveredCount++;
          }
          numHits[featIndex]++;
          if(numHits[featIndex] > most)
          {
            most = numHits[featIndex];
            storageArray[voxelIndex] = static_cast<int32>(neighborPoint);
          }
        }
      }
    }
  }
  return unresolvedCellCount;
}

/**
 * @brief Checks the Feature Ids array before anything is modified.
 *
 * FlagFeatures() indexes a vector of size totalFeatures with each cell's FeatureId, and the fill loop
 * indexes the array with cell indices computed from the geometry, so both the value range and the
 * tuple count must be right.
 *
 * @param featureIds Cell array to check.
 * @param totalCells Number of cells in the Image Geometry.
 * @param totalFeatures Tuple count of the feature Attribute Matrix.
 * @param featureIdsPath Named in the error messages.
 * @param featureAttributeMatrixPath Named in the error messages.
 * @param shouldCancel Polled every k_CancelPollStride cells.
 * @return Error -45437 if the tuple count differs from totalCells, error -45435 naming the first
 * cell whose value is outside [0, totalFeatures), otherwise valid.
 */
Result<> ValidateFeatureIds(const Int32AbstractDataStore& featureIds, usize totalCells, usize totalFeatures, const DataPath& featureIdsPath, const DataPath& featureAttributeMatrixPath,
                            const std::atomic_bool& shouldCancel)
{
  const usize totalPoints = featureIds.getNumberOfTuples();
  if(totalPoints != totalCells)
  {
    return MakeErrorResult(k_TupleCountMismatchError, fmt::format("The Feature Ids array '{}' has {} tuple(s), but the selected Image Geometry has {} cell(s). The array must hold exactly one value "
                                                                  "per cell of the selected geometry. No data was modified.",
                                                                  featureIdsPath.toString(), totalPoints, totalCells));
  }

  for(usize i = 0; i < totalPoints; i++)
  {
    if(i % k_CancelPollStride == 0 && shouldCancel)
    {
      return {};
    }
    const int32 featureId = featureIds[i];
    if(featureId < 0 || static_cast<usize>(featureId) >= totalFeatures)
    {
      return MakeErrorResult(k_FeatureIdOutOfRangeError,
                             fmt::format("Cell {} of the Feature Ids array '{}' holds the value {}, but the feature Attribute Matrix '{}' has {} tuple(s), so the valid range is 0 through {}. Every "
                                         "cell must reference a tuple of the feature Attribute Matrix. No data was modified.",
                                         i, featureIdsPath.toString(), featureId, featureAttributeMatrixPath.toString(), totalFeatures, totalFeatures - 1));
    }
  }
  return {};
}

/**
 * @brief Marks the cells of every flagged feature.
 *
 * @param featureIds Cell FeatureIds, already validated. Cells of flagged features are set to -1 when
 * fillRemovedFeatures is true, otherwise to 0.
 * @param flaggedFeatures true means remove. Index 0 is ignored.
 * @param fillRemovedFeatures Selects the marker value.
 * @return One entry per feature, true for features that survive. Empty when every feature is flagged;
 * in that case nothing was modified.
 */
std::vector<bool> FlagFeatures(Int32AbstractDataStore& featureIds, const std::vector<bool>& flaggedFeatures, const bool fillRemovedFeatures)
{
  bool good = false;
  usize totalPoints = featureIds.getNumberOfTuples();
  usize totalFeatures = flaggedFeatures.size();
  std::vector<bool> activeObjects(totalFeatures, true);
  for(usize i = 1; i < totalFeatures; i++)
  {
    if(!flaggedFeatures[i])
    {
      good = true;
    }
    else
    {
      activeObjects[i] = false;
    }
  }
  if(!good)
  {
    return {};
  }
  for(usize i = 0; i < totalPoints; i++)
  {
    if(activeObjects[featureIds[i]])
    {
      continue;
    }

    if(fillRemovedFeatures)
    {
      featureIds[i] = -1;
    }
    else
    {
      featureIds[i] = 0;
    }
  }
  return activeObjects;
}

/**
 * @brief Copies every listed cell array from each vacated cell's chosen source cell.
 *
 * @param featureIds Cell FeatureIds. Read to find vacated cells; it is also one of the voxelArrays and
 * is overwritten through that list.
 * @param neighbors Source cell index per cell, or -1 when the cell has no source this pass.
 * @param voxelArrays Every cell array to copy, including the Feature Ids array itself.
 * @param shouldCancel Polled once per cell.
 * @return The number of cells that received a copy. Zero means the pass made no progress.
 */
usize FindVoxelArrays(const Int32AbstractDataStore& featureIds, const std::vector<int32>& neighbors, std::vector<std::shared_ptr<IDataArray>>& voxelArrays, const std::atomic_bool& shouldCancel)
{
  const usize totalPoints = featureIds.getNumberOfTuples();
  usize filledCellCount = 0;

  for(usize j = 0; j < totalPoints; j++)
  {
    if(shouldCancel)
    {
      return filledCellCount;
    }

    const int32 neighbor = neighbors[j];
    if(neighbor < 0)
    {
      continue;
    }
    if(featureIds[j] < 0 && featureIds[neighbor] >= 0)
    {
      for(const auto& voxelArray : voxelArrays)
      {
        voxelArray->copyTuple(neighbor, j);
      }
      filledCellCount++;
    }
  }
  return filledCellCount;
}
} // namespace

namespace nx::core::FeatureRemovalUtilities
{
// -----------------------------------------------------------------------------
Result<> removeFlaggedFeatures(DataStructure& dataStructure, const std::vector<bool>& flaggedFeatures, const RemovalArgs& args, const IFilter::MessageHandler& messageHandler,
                               const std::atomic_bool& shouldCancel)
{
  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(args.ImageGeometryPath);
  auto& featureIds = dataStructure.getDataAs<Int32Array>(args.FeatureIdsArrayPath)->getDataStoreRef();

  MessageHelper messageHelper(messageHandler);
  Result<> result;

  messageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Beginning Feature Removal")});

  // Tuple 0 is the unused feature, so fewer than two tuples means there is no feature to keep.
  if(flaggedFeatures.size() < 2)
  {
    return MakeErrorResult(k_AllFeaturesFlaggedError, fmt::format("The feature Attribute Matrix '{}' has {} tuple(s). Tuple 0 is the unused feature, so at least 2 tuples are required for a "
                                                                  "feature to survive the removal. No data was modified.",
                                                                  args.FeatureAttributeMatrixPath.toString(), flaggedFeatures.size()));
  }

  Result<> validation = ValidateFeatureIds(featureIds, imageGeom.getNumberOfCells(), flaggedFeatures.size(), args.FeatureIdsArrayPath, args.FeatureAttributeMatrixPath, shouldCancel);
  if(validation.invalid())
  {
    return validation;
  }

  if(shouldCancel)
  {
    return {};
  }

  std::vector<bool> activeObjects = FlagFeatures(featureIds, flaggedFeatures, args.FillRemovedFeatures);
  if(activeObjects.empty())
  {
    return MakeErrorResult(k_AllFeaturesFlaggedError, fmt::format("All {} feature(s) in '{}' were flagged and would all be removed. At least one feature must remain. No data was modified.",
                                                                  flaggedFeatures.size() - 1, args.FeatureAttributeMatrixPath.toString()));
  }

  if(shouldCancel)
  {
    return {};
  }

  if(args.FillRemovedFeatures)
  {
    // The Feature Ids array is the array being filled. Ignoring it would leave every vacated cell
    // negative forever, so it is always copied even when the caller lists it.
    std::vector<DataPath> ignoredPaths;
    ignoredPaths.reserve(args.IgnoredDataArrayPaths.size());
    for(const DataPath& path : args.IgnoredDataArrayPaths)
    {
      if(path == args.FeatureIdsArrayPath)
      {
        result.warnings().push_back(Warning{k_FeatureIdsCannotBeIgnoredWarning, fmt::format("The Feature Ids array '{}' was listed in the arrays to ignore. It is the array being filled and "
                                                                                            "cannot be ignored, so it was removed from the list.",
                                                                                            path.toString())});
        continue;
      }
      ignoredPaths.push_back(path);
    }
    // The set of cell arrays does not change during the fill, so build the list once.
    std::vector<std::shared_ptr<IDataArray>> voxelArrays = GenerateDataArrayList(dataStructure, args.FeatureIdsArrayPath, ignoredPaths);

    // One int32 per cell. Declared here rather than at function scope because only the fill path
    // uses it; hoisting it would cost a 4 GB allocation on a 1000^3 volume even with fill disabled,
    // which is the default.
    std::vector<int32> neighbors((featureIds.getNumberOfTuples() * featureIds.getNumberOfComponents()), -1);

    usize count = 0;
    while(true)
    {
      count++;
      messageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Entering iteration number {}...", count)});
      std::fill(neighbors.begin(), neighbors.end(), -1);
      const usize unresolvedCellCount = IdentifyNeighbors(imageGeom, featureIds, neighbors, shouldCancel, messageHelper);

      if(shouldCancel)
      {
        return {};
      }
      if(unresolvedCellCount == 0)
      {
        break;
      }

      messageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Filling {} bad voxels...", unresolvedCellCount)});
      const usize filledCellCount = FindVoxelArrays(featureIds, neighbors, voxelArrays, shouldCancel);

      if(shouldCancel)
      {
        return {};
      }

      // A pass that fills nothing would be repeated with the same result, so stop instead of looping
      // forever. This happens when every cell belonged to a flagged feature and the unflagged
      // features own no cells, so no vacated cell touches a cell that belongs to a feature.
      if(filledCellCount == 0)
      {
        return MakeErrorResult(k_NoFillProgressError,
                               fmt::format("Fill iteration {} could not fill any of the {} remaining vacated cell(s) in the Feature Ids array '{}' because none of them has a face neighbor that "
                                           "belongs to a surviving feature. This happens when every cell belongs to a flagged feature and the unflagged feature(s) own no cells. Unflag a "
                                           "feature that owns cells, or disable 'Fill-in Removed Features'. THE FOLLOWING ARRAY HAS BEEN MODIFIED: '{}' (removed cells are set to -1).",
                                           count, unresolvedCellCount, args.FeatureIdsArrayPath.toString(), args.FeatureIdsArrayPath.toString()));
      }
    }
  }

  if(shouldCancel)
  {
    return {};
  }

  messageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Stripping excess inactive objects from model...")});
  if(!RemoveInactiveObjects(dataStructure, args.FeatureAttributeMatrixPath, activeObjects, featureIds, flaggedFeatures.size(), messageHandler, shouldCancel))
  {
    return MakeErrorResult(k_RemoveInactiveObjectsError, fmt::format("Failed to remove inactive objects from feature group at path '{}'.", args.FeatureAttributeMatrixPath.toString()));
  }

  return result;
}
} // namespace nx::core::FeatureRemovalUtilities
