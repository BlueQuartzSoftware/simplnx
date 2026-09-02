#include "RemoveFlaggedFeaturesDirect.hpp"

#include "RemoveFlaggedFeatures.hpp"
#include "RemoveFlaggedFeaturesCommon.hpp"

#include "SimplnxCore/Filters/ComputeFeatureRectFilter.hpp"
#include "SimplnxCore/Filters/CropImageGeometryFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

using namespace nx::core;

namespace
{
constexpr int32 k_FeatureIdOutOfRangeError = -45435;
constexpr int32 k_NoFillProgressError = -45436;
constexpr int32 k_EmptyFeatureSkippedWarning = -53905;

/**
 * @brief Validates every Feature ID before the algorithm modifies data.
 */
Result<> ValidateFeatureIds(const Int32AbstractDataStore& featureIds, usize totalFeatures, const DataPath& featureIdsPath, const DataPath& flaggedFeaturesPath,
                            const std::atomic_bool& shouldCancel)
{
  const usize totalPoints = featureIds.getNumberOfTuples();
  for(usize cellIndex = 0; cellIndex < totalPoints; cellIndex++)
  {
    if(shouldCancel)
    {
      return {};
    }
    const int32 featureId = featureIds[cellIndex];
    if(featureId < 0 || static_cast<usize>(featureId) >= totalFeatures)
    {
      return MakeErrorResult(k_FeatureIdOutOfRangeError,
                             fmt::format("Cell {} of the Feature IDs array '{}' has value {}, but the flagged-features array '{}' has {} tuple(s). Valid Feature IDs are in [0, {}). No data was modified.",
                                         cellIndex, featureIdsPath.toString(), featureId, flaggedFeaturesPath.toString(), totalFeatures, totalFeatures));
    }
  }
  return {};
}

/**
 * @brief Selects one majority face-neighbor source for each negative voxel.
 * @param imageGeom Defines voxel dimensions.
 * @param featureIds Provides current Feature IDs.
 * @param storageArray Receives flat source-voxel indexes.
 * @param replacementCount Receives the number of negative voxels that have a non-negative source.
 * @param unresolvedCount Receives the number of negative voxels without a non-negative source.
 * @param shouldCancel Stops before later Z slices when true.
 * @param messageHelper Creates a throttled progress messenger.
 * @return True if any negative Feature ID remains unresolved; false after cancellation or none.
 * @pre Flat voxel indexes fit in int32.
 */
bool IdentifyNeighbors(ImageGeom& imageGeom, Int32AbstractDataStore& featureIds, std::vector<int32>& storageArray, usize& replacementCount, usize& unresolvedCount,
                       const std::atomic_bool& shouldCancel, MessageHelper& messageHelper)
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

  bool shouldLoop = false;
  replacementCount = 0;
  unresolvedCount = 0;

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
        if(featureName >= 0)
        {
          continue;
        }
        int32 current = 0;
        int32 most = 0;
        std::array<int32, k_NumFaceNeighbors> numHits = {};
        std::array<int32, k_NumFaceNeighbors> discoveredFeatures = {};
        usize discoveredFeatureCount = 0;
        // Check six face neighbors in the shared NeighborUtilities order.
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
            for(usize featIndex = 0; featIndex < discoveredFeatureCount; featIndex++)
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
              discoveredFeatures[discoveredFeatureCount] = feature;
              numHits[discoveredFeatureCount] = 1;
              discoveredFeatureCount++;
              if(most < 1)
              {
                most = 1;
                storageArray[voxelIndex] = static_cast<int32>(neighborPoint);
              }
            }
          }
        }
        if(storageArray[voxelIndex] >= 0)
        {
          replacementCount++;
        }
        else
        {
          shouldLoop = true;
          unresolvedCount++;
        }
      }
    }
  }
  return shouldLoop;
}

/**
 * @brief Marks cells that belong to flagged features.
 * @param featureIds Provides and receives cell Feature IDs.
 * @param flaggedFeatures Identifies features selected for removal.
 * @param fillRemovedFeatures Uses -1 marks for later filling when true.
 * @return Active-feature flags, or an empty vector if removal selects all features.
 * @pre Feature IDs are nonnegative and less than the feature-flag tuple count.
 */
std::vector<bool> FlagFeatures(Int32AbstractDataStore& featureIds, std::unique_ptr<MaskCompareUtilities::MaskCompare>& flaggedFeatures, const bool fillRemovedFeatures)
{
  bool good = false;
  usize totalPoints = featureIds.getNumberOfTuples();
  usize totalFeatures = flaggedFeatures->getNumberOfTuples();
  std::vector<bool> activeObjects(totalFeatures, true);
  for(usize i = 1; i < totalFeatures; i++)
  {
    if(!flaggedFeatures->isTrue(i))
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
 * @brief Copies companion tuples from selected resident source neighbors.
 * @param featureIds Provides current Feature IDs.
 * @param neighbors Provides one flat source index per destination cell.
 * @param voxelArrays Receives source tuples for negative Feature IDs.
 * @param shouldCancel Stops before later destination cells when true.
 */
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

/**
 * @class RunCropImageGeometryImpl
 * @brief Runs one CropImageGeometryFilter task for a flagged feature.
 *
 * The caller executes each task synchronously. This protects DataStructure mutation
 * and keeps borrowed loop-local paths and bounds alive.
 */
} // namespace

RemoveFlaggedFeaturesDirect::RemoveFlaggedFeaturesDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                         const RemoveFlaggedFeaturesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

RemoveFlaggedFeaturesDirect::~RemoveFlaggedFeaturesDirect() noexcept = default;

Result<> RemoveFlaggedFeaturesDirect::operator()()
{
  auto& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();
  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  auto function = static_cast<Functionality>(m_InputValues->ExtractFeatures);

  std::unique_ptr<MaskCompareUtilities::MaskCompare> flaggedFeatures = nullptr;
  try
  {
    flaggedFeatures = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->FlaggedFeaturesArrayPath);
  } catch(const std::out_of_range& exception)
  {
    // Normal filter execution validates this path. Direct algorithm callers can
    // still supply a missing or unsupported mask.
    std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->FlaggedFeaturesArrayPath.toString());
    return MakeErrorResult(-53900, message);
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  MessageHelper messageHelper(m_MessageHandler);
  Result<> result;

  if(function != Functionality::Extract)
  {
    Result<> validationResult = ValidateFeatureIds(featureIds, flaggedFeatures->getNumberOfTuples(), m_InputValues->FeatureIdsArrayPath, m_InputValues->FlaggedFeaturesArrayPath, m_ShouldCancel);
    if(validationResult.invalid())
    {
      return validationResult;
    }
    if(m_ShouldCancel)
    {
      return result;
    }
  }

  // Extract and ExtractThenRemove create one cropped geometry per flagged feature.
  if(function != Functionality::Remove)
  {
    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Beginning Feature Extraction")});

    {
      ComputeFeatureRectFilter filter;
      Arguments args;

      args.insert(ComputeFeatureRectFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(m_InputValues->FeatureIdsArrayPath));
      args.insert(ComputeFeatureRectFilter::k_FeatureDataAttributeMatrixPath_Key, std::make_any<DataPath>(m_InputValues->TempBoundsPath.getParent()));
      args.insert(ComputeFeatureRectFilter::k_FeatureRectArrayName_Key, std::make_any<std::string>(m_InputValues->TempBoundsPath.getTargetName()));

      auto preflightResult = filter.preflight(m_DataStructure, args);
      if(preflightResult.outputActions.invalid())
      {
        // The delegated filter reports the real cause, so its own errors are kept and each one is
        // prefixed with this call's context instead of being buried behind a second error object.
        Result<> delegatedResult = ConvertResult(std::move(preflightResult.outputActions));
        if(delegatedResult.valid() || delegatedResult.errors().empty())
        {
          // Defensive: the delegated preflight reported a failure without any error to explain it.
          return MakeErrorResult(-45442, fmt::format("RemoveFlaggedFeatures: computing the feature bounds at '{}' from Feature Ids '{}' failed: the ComputeFeatureRect preflight reported a "
                                                     "failure without a cause.",
                                                     m_InputValues->TempBoundsPath.toString(), m_InputValues->FeatureIdsArrayPath.toString()));
        }
        for(Error& error : delegatedResult.errors())
        {
          error.message = fmt::format("RemoveFlaggedFeatures: computing the feature bounds at '{}' from Feature Ids '{}' failed: {}", m_InputValues->TempBoundsPath.toString(),
                                      m_InputValues->FeatureIdsArrayPath.toString(), error.message);
        }
        return delegatedResult;
      }

      if(m_ShouldCancel)
      {
        return result;
      }

      auto executeResult = filter.execute(m_DataStructure, args);
      if(executeResult.result.invalid())
      {
        // The delegated filter reports the real cause, so its own errors are kept and each one is
        // prefixed with this call's context instead of being buried behind a second error object.
        Result<> delegatedResult = std::move(executeResult.result);
        if(delegatedResult.errors().empty())
        {
          // Defensive: the delegated execution reported a failure without any error to explain it.
          return MakeErrorResult(-45443, fmt::format("RemoveFlaggedFeatures: computing the feature bounds at '{}' from Feature Ids '{}' failed: the ComputeFeatureRect execution reported a "
                                                     "failure without a cause.",
                                                     m_InputValues->TempBoundsPath.toString(), m_InputValues->FeatureIdsArrayPath.toString()));
        }
        for(Error& error : delegatedResult.errors())
        {
          error.message = fmt::format("RemoveFlaggedFeatures: computing the feature bounds at '{}' from Feature Ids '{}' failed: {}", m_InputValues->TempBoundsPath.toString(),
                                      m_InputValues->FeatureIdsArrayPath.toString(), error.message);
        }
        return delegatedResult;
      }
    }

    auto bounds = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->TempBoundsPath);

    if(m_ShouldCancel)
    {
      return result;
    }

    // Declared before the runner so the runner's destructor joins every task while the
    // holder is still alive.
    CopyFromArray::ParallelTaskResult cropTaskResult;

    ParallelTaskAlgorithm taskRunner;
    // Crop tasks mutate DataStructure and borrow loop-local bounds. Synchronous
    // execution satisfies both thread-safety and lifetime requirements.
    taskRunner.setParallelizationEnabled(false);

    usize maxTuple = flaggedFeatures->getNumberOfTuples();
    std::string paddingWidth = std::to_string(std::to_string(maxTuple).size());
    for(usize i = 1; i < maxTuple; i++)
    {
      if(m_ShouldCancel)
      {
        return result;
      }

      if(!flaggedFeatures->isTrue(i))
      {
        continue;
      }

      usize index = 6 * i;
      std::vector<uint64> minVoxels = {static_cast<uint64>(bounds[index]), static_cast<uint64>(bounds[index + 1]), static_cast<uint64>(bounds[index + 2])};
      std::vector<uint64> maxVoxels = {static_cast<uint64>(bounds[index + 3]), static_cast<uint64>(bounds[index + 4]), static_cast<uint64>(bounds[index + 5])};

      if(minVoxels[0] > maxVoxels[0] || minVoxels[1] > maxVoxels[1] || minVoxels[2] > maxVoxels[2])
      {
        result.warnings().push_back(
            Warning{k_EmptyFeatureSkippedWarning, fmt::format("Feature {} is flagged for extraction but owns no cell in the Feature IDs array '{}'. No geometry was created for it.", i,
                                                              m_InputValues->FeatureIdsArrayPath.toString())});
        continue;
      }

      DataPath createdImgGeomPath({fmt::format(fmt::runtime("{}-{:0" + paddingWidth + "d}"), m_InputValues->CreatedImageGeometryPrefix, i)});

      m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Now Extracting Feature {}", i)});
      taskRunner.execute(RunCropImageGeometryImpl(m_DataStructure, m_ShouldCancel, m_InputValues->ImageGeometryPath, minVoxels, maxVoxels, createdImgGeomPath, cropTaskResult));

      // Stop scheduling crops once one has failed, so the failure is reported instead of
      // being buried behind later work.
      if(cropTaskResult.shouldAbort())
      {
        break;
      }
    }
    taskRunner.wait();

    Result<> cropResult = cropTaskResult.takeResult();
    if(cropResult.invalid())
    {
      return MergeResults(std::move(result), std::move(cropResult));
    }

    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("All Features Successfully Extracted")});
  }

  if(m_ShouldCancel)
  {
    return result;
  }

  // Remove and ExtractThenRemove modify the source feature data.
  if(function != Functionality::Extract)
  {
    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Beginning Feature Removal")});

    std::vector<int32> neighbors((featureIds.getNumberOfTuples() * featureIds.getNumberOfComponents()), -1);
    std::vector<bool> activeObjects = FlagFeatures(featureIds, flaggedFeatures, m_InputValues->FillRemovedFeatures);
    if(activeObjects.empty())
    {
      const usize removableFeatureCount = flaggedFeatures->getNumberOfTuples() > 0 ? flaggedFeatures->getNumberOfTuples() - 1 : 0;
      Result<> allFlaggedResult = MakeErrorResult(-45433, fmt::format("All {} feature(s) in '{}' were flagged and would be removed. At least one feature that owns cells must remain.",
                                                                   removableFeatureCount, m_InputValues->FlaggedFeaturesArrayPath.getParent().toString()));
      return MergeResults(std::move(result), std::move(allFlaggedResult));
    }

    if(m_ShouldCancel)
    {
      return result;
    }

    if(m_InputValues->FillRemovedFeatures)
    {
      bool shouldLoop = false;
      usize count = 0;
      do
      {
        count++;
        m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Entering iteration number {}...", count)});
        std::fill(neighbors.begin(), neighbors.end(), -1);
        usize replacementCount = 0;
        usize unresolvedCount = 0;
        shouldLoop = IdentifyNeighbors(imageGeom, featureIds, neighbors, replacementCount, unresolvedCount, m_ShouldCancel, messageHelper);

        if(m_ShouldCancel)
        {
          return result;
        }

        if(replacementCount == 0 && shouldLoop)
        {
          Result<> noProgressResult = MakeErrorResult(
              k_NoFillProgressError,
              fmt::format("Fill iteration {} could not fill any of the {} remaining vacated cell(s) in the Feature IDs array '{}' because none has a non-negative face neighbor. Unflag a feature "
                          "that owns cells, or disable 'Fill-in Removed Features'. The Feature IDs array was modified: removed cells are set to -1.",
                          count, unresolvedCount, m_InputValues->FeatureIdsArrayPath.toString()));
          return MergeResults(std::move(result), std::move(noProgressResult));
        }

        m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Filling bad voxels...")});
        std::vector<std::shared_ptr<IDataArray>> voxelArrays = GenerateDataArrayList(m_DataStructure, m_InputValues->FeatureIdsArrayPath, m_InputValues->IgnoredDataArrayPaths);
        FindVoxelArrays(featureIds, neighbors, voxelArrays, m_ShouldCancel);
      } while(shouldLoop);
    }

    if(m_ShouldCancel)
    {
      return result;
    }

    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Stripping excess inactive objects from model...")});
    DataPath featureGroupPath = m_InputValues->FlaggedFeaturesArrayPath.getParent();
    Result<> removeResult = RemoveInactiveObjects(m_DataStructure, featureGroupPath, activeObjects, featureIds, flaggedFeatures->getNumberOfTuples(), m_MessageHandler, m_ShouldCancel);
    if(removeResult.invalid())
    {
      return MergeResults(std::move(result), std::move(removeResult));
    }
    // RemoveInactiveObjects reports a cancelled compaction as success. No work follows this
    // call, so a cancelled and a completed run both leave through the empty valid Result below.
  }

  return result;
}
