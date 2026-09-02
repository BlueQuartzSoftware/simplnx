#include "SimplnxCore/utils/FeatureRemovalUtilities.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

#include <algorithm>

using namespace nx::core;

namespace
{
constexpr int32 k_AllFeaturesFlaggedError = -45433;
constexpr int32 k_RemoveInactiveObjectsError = -45434;
constexpr int32 k_FeatureIdOutOfRangeError = -45435;
constexpr int32 k_NoFillProgressError = -45436;

/**
 * @brief Outcome of one pass of IdentifyNeighbors() over the volume.
 */
struct NeighborScan
{
  /// At least one cell still holds a negative (vacated) FeatureId.
  bool unresolvedCellsRemain = false;
  /// At least one vacated cell has a non-negative face neighbor recorded as its fill source.
  bool fillSourceFound = false;
};

/**
 * @brief Chooses a fill source for every vacated cell.
 *
 * A vacated cell is one whose FeatureId is negative. Its six face neighbors are polled in the order
 * -Z, -Y, -X, +X, +Y, +Z. Every non-negative neighbor FeatureId is tallied, background (0) included,
 * and the source becomes the neighbor whose feature first reaches the highest tally. A cell with no
 * non-negative neighbor gets no source this pass and is retried on the next pass, after its own
 * neighbors have been filled.
 *
 * Cells with FeatureId 0 are background. They are never fill targets, so they do not keep the caller
 * iterating. They are legal fill sources, which matches DREAM3D 6.5.171.
 */
NeighborScan IdentifyNeighbors(ImageGeom& imageGeom, Int32AbstractDataStore& featureIds, std::vector<int32>& storageArray, const std::atomic_bool& shouldCancel, MessageHelper& messageHelper)
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

  NeighborScan scan;

  auto progressIncrement = dims[2] / 100;
  usize progressCounter = 0;
  int32 featureName;
  int64 kStride, jStride;
  for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
  {
    if(shouldCancel)
    {
      return scan;
    }

    if(progressCounter > progressIncrement)
    {
      throttledMessenger.sendThrottledMessage([&]() { return fmt::format("Processing Image... {:.2f}%", CalculatePercentComplete(zIdx, dims[2])); });
      progressCounter = 0;
    }
    progressCounter++;

    kStride = dims[0] * dims[1] * zIdx;
    for(int64 yIdx = 0; yIdx < dims[1]; yIdx++)
    {
      jStride = dims[0] * yIdx;
      for(int64 xIdx = 0; xIdx < dims[0]; xIdx++)
      {
        int64 voxelIndex = kStride + jStride + xIdx;
        featureName = featureIds[voxelIndex];
        // Only vacated cells (negative) need a source. Background (0) is not a fill target; treating
        // it as one would keep the caller's loop alive forever because nothing ever overwrites it.
        if(featureName >= 0)
        {
          continue;
        }
        scan.unresolvedCellsRemain = true;
        int32 current;
        int32 most = 0;
        std::vector<int32> numHits(6, 0);
        std::vector<int32> discoveredFeatures = {};
        discoveredFeatures.reserve(6);
        // Loop over the 6 face neighbors of the voxel
        const std::array<bool, k_NumFaceNeighbors> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
        for(const auto& faceIndex : faceNeighborInternalIdx)
        {
          if(!isValidFaceNeighbor[faceIndex])
          {
            continue;
          }

          int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];
          int32 feature = featureIds[neighborPoint];
          if(feature >= 0)
          {
            bool found = false;
            for(usize featIndex = 0; featIndex < discoveredFeatures.size(); featIndex++)
            {
              if(discoveredFeatures[featIndex] == feature)
              {
                found = true;
                numHits[featIndex]++;
                current = numHits[featIndex];
                if(current > most)
                {
                  most = current;
                  storageArray[voxelIndex] = static_cast<int32>(neighborPoint);
                  scan.fillSourceFound = true;
                }
                break;
              }
            }
            if(!found)
            {
              // Count the first sighting as a hit. Without this the tally only records a neighbor on
              // the SECOND sighting of a feature, so a bad cell whose valid neighbors all belong to
              // distinct features never gets a fill source. It stays negative, and the caller's
              // do/while dilation loop never terminates.
              discoveredFeatures.push_back(feature);
              numHits[discoveredFeatures.size() - 1] = 1;
              if(1 > most)
              {
                most = 1;
                storageArray[voxelIndex] = static_cast<int32>(neighborPoint);
                scan.fillSourceFound = true;
              }
            }
          }
        }
      }
    }
  }
  return scan;
}

/**
 * @brief Checks that every FeatureId indexes a tuple of the feature Attribute Matrix.
 *
 * FlagFeatures() indexes a vector of size totalFeatures with each cell's FeatureId, so a negative or
 * too-large value is an out-of-bounds read. Run this before anything is modified.
 */
Result<> ValidateFeatureIds(const Int32AbstractDataStore& featureIds, usize totalFeatures, const DataPath& featureIdsPath, const DataPath& featureAttributeMatrixPath)
{
  const usize totalPoints = featureIds.getNumberOfTuples();
  for(usize i = 0; i < totalPoints; i++)
  {
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

void FindVoxelArrays(const Int32AbstractDataStore& featureIds, const std::vector<int32>& neighbors, std::vector<std::shared_ptr<IDataArray>>& voxelArrays, const std::atomic_bool& shouldCancel)
{
  const usize totalPoints = featureIds.getNumberOfTuples();

  int32 featureName, neighbor;
  for(usize j = 0; j < totalPoints; j++)
  {
    if(shouldCancel)
    {
      return;
    }

    featureName = featureIds[j];
    neighbor = neighbors[j];
    if(neighbor >= 0)
    {
      if(featureName < 0 && featureIds[neighbor] >= 0)
      {
        for(const auto& voxelArray : voxelArrays)
        {
          voxelArray->copyTuple(neighbor, j);
        }
      }
    }
  }
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

  messageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Beginning Feature Removal")});

  Result<> validation = ValidateFeatureIds(featureIds, flaggedFeatures.size(), args.FeatureIdsArrayPath, args.FeatureAttributeMatrixPath);
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
    // One int32 per cell. Declared here rather than at function scope because only the fill path
    // uses it; hoisting it would cost a 4 GB allocation on a 1000^3 volume even with fill disabled,
    // which is the default.
    std::vector<int32> neighbors((featureIds.getNumberOfTuples() * featureIds.getNumberOfComponents()), -1);

    bool shouldLoop;
    usize count = 0;
    do
    {
      count++;
      messageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Entering iteration number {}...", count)});
      std::fill(neighbors.begin(), neighbors.end(), -1);
      const NeighborScan scan = IdentifyNeighbors(imageGeom, featureIds, neighbors, shouldCancel, messageHelper);

      if(shouldCancel)
      {
        return {};
      }

      // Vacated cells remain but none of them touches a cell that belongs to a feature. Another pass
      // would find the same state, so stop instead of looping forever. This only happens when every
      // cell belonged to a flagged feature and the unflagged features own no cells.
      if(scan.unresolvedCellsRemain && !scan.fillSourceFound)
      {
        return MakeErrorResult(k_NoFillProgressError,
                               fmt::format("Fill iteration {} could not fill any of the remaining vacated cells in the Feature Ids array '{}' because none of them has a face neighbor that belongs "
                                           "to a surviving feature. This happens when every cell belongs to a flagged feature and the unflagged feature(s) own no cells. Unflag a feature that "
                                           "owns cells, or disable 'Fill-in Removed Features'. THE FOLLOWING ARRAY HAS BEEN MODIFIED: '{}' (removed cells are set to -1).",
                                           count, args.FeatureIdsArrayPath.toString(), args.FeatureIdsArrayPath.toString()));
      }

      messageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Filling bad voxels...")});
      std::vector<std::shared_ptr<IDataArray>> voxelArrays = GenerateDataArrayList(dataStructure, args.FeatureIdsArrayPath, args.IgnoredDataArrayPaths);
      FindVoxelArrays(featureIds, neighbors, voxelArrays, shouldCancel);
      shouldLoop = scan.unresolvedCellsRemain;
    } while(shouldLoop);
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

  return {};
}
} // namespace nx::core::FeatureRemovalUtilities
