#include "SimplnxCore/utils/FeatureRemovalUtilities.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <algorithm>

using namespace nx::core;

namespace
{
bool IdentifyNeighbors(ImageGeom& imageGeom, Int32AbstractDataStore& featureIds, std::vector<int32>& storageArray, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  ThrottledMessageHandler throttledMessenger(messageHandler);

  SizeVec3 uDims = imageGeom.getDimensions();

  std::array<int64, 3> dims = {
      static_cast<int64>(uDims[0]),
      static_cast<int64>(uDims[1]),
      static_cast<int64>(uDims[2]),
  };

  constexpr FaceNeighborType k_NumFaceNeighbors = VoxelNeighbors<Image3D>::k_FaceNeighborCount;
  const std::array<int64, k_NumFaceNeighbors> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  constexpr std::array<FaceNeighborType, k_NumFaceNeighbors> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  bool shouldLoop = false;

  auto progressIncrement = dims[2] / 100;
  usize progressCounter = 0;
  int32 featureName;
  int64 kStride, jStride;
  for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
  {
    if(shouldCancel)
    {
      return false;
    }

    if(progressCounter > progressIncrement)
    {
      throttledMessenger.updatePercent("Processing Image", zIdx, dims[2]);
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
        if(featureName > 0)
        {
          continue;
        }
        shouldLoop = true;
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
                }
                break;
              }
            }
            if(!found)
            {
              // Count the first sighting as a hit. Without this the tally only records a neighbor on
              // the SECOND sighting of a feature, so a bad cell whose valid neighbors all belong to
              // distinct features never gets a fill source. It stays negative, shouldLoop stays true,
              // and the caller's do/while dilation loop never terminates.
              discoveredFeatures.push_back(feature);
              numHits[discoveredFeatures.size() - 1] = 1;
              if(1 > most)
              {
                most = 1;
                storageArray[voxelIndex] = static_cast<int32>(neighborPoint);
              }
            }
          }
        }
      }
    }
  }
  return shouldLoop;
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

  messageHandler.sendInfoMessage(fmt::format("Beginning Feature Removal"));

  std::vector<bool> activeObjects = FlagFeatures(featureIds, flaggedFeatures, args.FillRemovedFeatures);
  if(activeObjects.empty())
  {
    return MakeErrorResult(-45433, "All Features were flagged and would all be removed. The filter has quit.");
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
      messageHandler.sendInfoMessage(fmt::format("Entering iteration number {}...", count));
      std::fill(neighbors.begin(), neighbors.end(), -1);
      shouldLoop = IdentifyNeighbors(imageGeom, featureIds, neighbors, shouldCancel, messageHandler);

      if(shouldCancel)
      {
        return {};
      }

      messageHandler.sendInfoMessage(fmt::format("Filling bad voxels..."));
      std::vector<std::shared_ptr<IDataArray>> voxelArrays = GenerateDataArrayList(dataStructure, args.FeatureIdsArrayPath, args.IgnoredDataArrayPaths);
      FindVoxelArrays(featureIds, neighbors, voxelArrays, shouldCancel);
    } while(shouldLoop);
  }

  if(shouldCancel)
  {
    return {};
  }

  messageHandler.sendInfoMessage(fmt::format("Stripping excess inactive objects from model..."));
  if(!RemoveInactiveObjects(dataStructure, args.FeatureAttributeMatrixPath, activeObjects, featureIds, flaggedFeatures.size(), messageHandler, shouldCancel))
  {
    return MakeErrorResult(-45434, fmt::format("Failed to remove inactive objects from feature group at path '{}'.", args.FeatureAttributeMatrixPath.toString()));
  }

  return {};
}
} // namespace nx::core::FeatureRemovalUtilities
