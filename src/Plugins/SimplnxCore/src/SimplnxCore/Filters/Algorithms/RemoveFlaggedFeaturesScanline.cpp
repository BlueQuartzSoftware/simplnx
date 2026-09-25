#include "RemoveFlaggedFeaturesScanline.hpp"

#include "RemoveFlaggedFeatures.hpp"
#include "RemoveFlaggedFeaturesCommon.hpp"

#include "SimplnxCore/Filters/ComputeFeatureRectFilter.hpp"
#include "SimplnxCore/Filters/CropImageGeometryFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <stdexcept>

using namespace nx::core;

// The scanline path uses bulk I/O and a write-behind Z window. This design
// keeps fill scratch proportional to slice area instead of total voxel count.

namespace
{
constexpr int32 k_FeatureIdOutOfRangeError = -45435;
constexpr int32 k_NoFillProgressError = -45436;
constexpr int32 k_TupleCountMismatchError = -45437;
constexpr int32 k_FeatureIdsCannotBeIgnoredWarning = -45438;
constexpr int32 k_EmptyFeatureSkippedWarning = -53905;
constexpr usize k_MaxListedEmptyFeatures = 10;

/**
 * @brief Validates every Feature ID through bounded reads before the algorithm modifies data.
 */
Result<> ValidateFeatureIdsScanline(const Int32AbstractDataStore& featureIds, usize totalCells, usize totalFeatures, const DataPath& featureIdsPath, const DataPath& flaggedFeaturesPath,
                                    const std::atomic_bool& shouldCancel)
{
  constexpr usize k_ChunkSize = 65536;
  const usize totalPoints = featureIds.getNumberOfTuples();
  if(totalPoints != totalCells)
  {
    return MakeErrorResult(
        k_TupleCountMismatchError,
        fmt::format("The Feature IDs array '{}' has {} tuple(s), but the selected Image Geometry has {} cell(s). The array must hold exactly one value per cell. No data was modified.",
                    featureIdsPath.toString(), totalPoints, totalCells));
  }
  auto values = std::make_unique<int32[]>(std::min(k_ChunkSize, totalPoints));
  for(usize offset = 0; offset < totalPoints; offset += k_ChunkSize)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_ChunkSize, totalPoints - offset);
    Result<> readResult = featureIds.copyIntoBuffer(offset, nonstd::span<int32>(values.get(), count));
    if(readResult.invalid())
    {
      return readResult;
    }
    for(usize chunkIndex = 0; chunkIndex < count; chunkIndex++)
    {
      const int32 featureId = values[chunkIndex];
      if(featureId < 0 || static_cast<usize>(featureId) >= totalFeatures)
      {
        const usize cellIndex = offset + chunkIndex;
        return MakeErrorResult(
            k_FeatureIdOutOfRangeError,
            fmt::format("Cell {} of the Feature IDs array '{}' has value {}, but the flagged-features array '{}' has {} tuple(s). Valid Feature IDs are in [0, {}). No data was modified.", cellIndex,
                        featureIdsPath.toString(), featureId, flaggedFeaturesPath.toString(), totalFeatures, totalFeatures));
      }
    }
  }
  return {};
}

/**
 * @struct TransferMarkedSlice
 * @brief Applies one slice of neighbor-source marks to a typed cell array.
 *
 * The transfer loads one destination slice and only the referenced adjacent
 * source slices. It writes the destination only when a mark changes a tuple.
 */
struct TransferMarkedSlice
{
  /**
   * @brief Transfers marked tuples for one destination slice.
   * @tparam T Specifies the cell-array scalar type.
   * @param dataArray Provides source tuples and receives destination tuples.
   * @param marks Provides one flat source index or -1 per destination tuple.
   * @param sliceSize Specifies tuples per Z slice.
   * @param destinationZ Specifies the destination Z index.
   * @param dimZ Specifies total Z slices.
   * @param shouldCancel Stops before read or write work when true.
   * @return First bulk-I/O or source-range error, or success after cancellation.
   */
  template <typename T>
  Result<> operator()(IDataArray& dataArray, const std::vector<int64>& marks, usize sliceSize, usize destinationZ, usize dimZ, const std::atomic_bool& shouldCancel) const
  {
    if(shouldCancel)
    {
      return {};
    }
    auto& store = dynamic_cast<DataArray<T>&>(dataArray).getDataStoreRef();
    const usize components = store.getNumberOfComponents();
    if(components == 0 || sliceSize > std::numeric_limits<usize>::max() / components)
    {
      return MakeErrorResult(-45435, "RemoveFlaggedFeatures slice transfer has an invalid component shape.");
    }
    const usize valuesPerSlice = sliceSize * components;
    auto destination = std::make_unique<T[]>(valuesPerSlice);
    auto readResult = store.copyIntoBuffer(destinationZ * valuesPerSlice, nonstd::span<T>(destination.get(), valuesPerSlice));
    if(readResult.invalid())
    {
      return readResult;
    }
    std::array<std::unique_ptr<T[]>, 3> sources;
    std::array<bool, 3> loaded = {false, false, false};
    const auto loadSource = [&](usize slot, usize sourceZ) -> Result<> {
      if(loaded[slot])
      {
        return {};
      }
      if(sourceZ >= dimZ)
      {
        return MakeErrorResult(-45436, "RemoveFlaggedFeatures source slice is outside the image geometry.");
      }
      sources[slot] = std::make_unique<T[]>(valuesPerSlice);
      auto result = store.copyIntoBuffer(sourceZ * valuesPerSlice, nonstd::span<T>(sources[slot].get(), valuesPerSlice));
      if(result.valid())
      {
        loaded[slot] = true;
      }
      return result;
    };

    bool modified = false;
    for(usize tuple = 0; tuple < sliceSize; tuple++)
    {
      const int64 sourceIndex = marks[tuple];
      if(sourceIndex < 0)
      {
        continue;
      }
      const usize sourceTuple = static_cast<usize>(sourceIndex);
      const usize sourceZ = sourceTuple / sliceSize;
      const usize sourceInSlice = sourceTuple % sliceSize;
      const usize sourceSlot = sourceZ < destinationZ ? 0 : (sourceZ > destinationZ ? 2 : 1);
      auto sourceResult = loadSource(sourceSlot, sourceZ);
      if(sourceResult.invalid())
      {
        return sourceResult;
      }
      std::copy_n(sources[sourceSlot].get() + sourceInSlice * components, components, destination.get() + tuple * components);
      modified = true;
    }
    if(shouldCancel || !modified)
    {
      return {};
    }
    return store.copyFromBuffer(destinationZ * valuesPerSlice, nonstd::span<const T>(destination.get(), valuesPerSlice));
  }
};

/**
 * @brief Dispatches one marked-slice transfer from the runtime array type.
 * @param dataArray Provides source tuples and receives destination tuples.
 * @param marks Provides one flat source index or -1 per destination tuple.
 * @param sliceSize Specifies tuples per Z slice.
 * @param destinationZ Specifies the destination Z index.
 * @param dimZ Specifies total Z slices.
 * @param shouldCancel Stops before read or write work when true.
 * @return First bulk-I/O or source-range error, or success after cancellation.
 */
Result<> TransferMarkedSliceForArray(IDataArray& dataArray, const std::vector<int64>& marks, usize sliceSize, usize destinationZ, usize dimZ, const std::atomic_bool& shouldCancel)
{
  return ExecuteDataFunction(TransferMarkedSlice{}, dataArray.getDataType(), dataArray, marks, sliceSize, destinationZ, dimZ, shouldCancel);
}

/**
 * @brief Selects and applies majority face-neighbor fills with a three-slice Feature-ID window.
 * @param imageGeom Defines voxel dimensions.
 * @param featureIdsStore Provides Feature IDs for neighbor votes.
 * @param voxelArrays Provides and receives each retained cell array.
 * @param replacementCount Receives the number of negative voxels that have a non-negative source.
 * @param unresolvedCount Receives the number of negative voxels without a non-negative source.
 * @param shouldCancel Stops before later Z slices when true.
 * @param messageHandler Creates a throttled progress messenger.
 * @return First bulk-I/O error, or whether any negative Feature ID remains unresolved.
 *
 * Three input slices preserve the Feature-ID state at iteration start. Two mark
 * slices keep writes behind the vote frontier. This order prevents earlier writes
 * from changing later votes.
 *
 * Face neighbors use the direct algorithm's order. The first feature to exceed
 * the current vote count wins. A later tie does not replace that feature.
 */
Result<bool> IdentifyAndFillNeighborsScanline(const ImageGeom& imageGeom, Int32AbstractDataStore& featureIdsStore, const std::vector<std::shared_ptr<IDataArray>>& voxelArrays, usize& replacementCount,
                                              usize& unresolvedCount, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  ThrottledMessageHandler progressThrottle(messageHandler);
  replacementCount = 0;
  unresolvedCount = 0;

  const SizeVec3 uDims = imageGeom.getDimensions();
  const int64 dimX = static_cast<int64>(uDims[0]);
  const int64 dimY = static_cast<int64>(uDims[1]);
  const int64 dimZ = static_cast<int64>(uDims[2]);
  const usize sliceSize = static_cast<usize>(dimX) * static_cast<usize>(dimY);
  const usize dimZUnsigned = static_cast<usize>(dimZ);

  // The rolling window keeps previous, current, and next Feature-ID slices.
  // Each advance needs at most one new disk read.
  std::vector<int32> prevSlice(sliceSize);
  std::vector<int32> curSlice(sliceSize);
  std::vector<int32> nextSlice(sliceSize);

  if(dimZ == 0)
  {
    return {false};
  }
  auto initialRead = featureIdsStore.copyIntoBuffer(0, nonstd::span<int32>(curSlice.data(), sliceSize));
  if(initialRead.invalid())
  {
    return ConvertInvalidResult<bool>(std::move(initialRead));
  }
  if(dimZ > 1)
  {
    auto nextRead = featureIdsStore.copyIntoBuffer(sliceSize, nonstd::span<int32>(nextSlice.data(), sliceSize));
    if(nextRead.invalid())
    {
      return ConvertInvalidResult<bool>(std::move(nextRead));
    }
  }

  // A mark stores the winning flat source index, or -1. Current and previous
  // mark slices keep scratch proportional to slice area.
  std::vector<int64> curMarks(sliceSize, -1);
  std::vector<int64> prevMarks(sliceSize, -1);

  bool shouldLoop = false;

  auto progressIncrement = dimZ / 100;
  usize progressCounter = 0;

  // Commit one resolved mark slice across all retained cell arrays.
  auto commitSlice = [&](usize z, const std::vector<int64>& marks) -> Result<> {
    for(const auto& voxelArray : voxelArrays)
    {
      auto result = TransferMarkedSliceForArray(*voxelArray, marks, sliceSize, z, dimZUnsigned, shouldCancel);
      if(result.invalid())
      {
        return result;
      }
    }
    return {};
  };

  for(int64 zIdx = 0; zIdx < dimZ; zIdx++)
  {
    if(shouldCancel)
    {
      return {false};
    }

    if(progressCounter > progressIncrement)
    {
      progressThrottle.updatePercent("Processing Image", zIdx, dimZ);
      progressCounter = 0;
    }
    progressCounter++;

    std::fill(curMarks.begin(), curMarks.end(), -1);

    const int64 kStride = dimX * dimY * zIdx;
    for(int64 yIdx = 0; yIdx < dimY; yIdx++)
    {
      const int64 rowOffset = yIdx * dimX;
      for(int64 xIdx = 0; xIdx < dimX; xIdx++)
      {
        const int64 sliceIndex = rowOffset + xIdx;
        const int64 voxelIndex = kStride + sliceIndex;
        const int32 featureName = curSlice[sliceIndex];
        if(featureName >= 0)
        {
          continue;
        }

        int32 current = 0;
        int32 most = 0;
        std::array<int32, 6> numHits = {0, 0, 0, 0, 0, 0};
        std::array<int32, 6> discoveredFeatures = {0, 0, 0, 0, 0, 0};
        usize discoveredFeatureCount = 0;

        // Preserve the direct algorithm's vote rule for negative destinations.
        auto considerNeighbor = [&](int32 feature, int64 neighborGlobalIndex) {
          if(feature < 0)
          {
            return;
          }
          for(usize featIndex = 0; featIndex < discoveredFeatureCount; featIndex++)
          {
            if(discoveredFeatures[featIndex] == feature)
            {
              numHits[featIndex]++;
              current = numHits[featIndex];
              if(current > most)
              {
                most = current;
                if(featureName < 0)
                {
                  curMarks[sliceIndex] = neighborGlobalIndex;
                }
              }
              return;
            }
          }
          discoveredFeatures[discoveredFeatureCount] = feature;
          numHits[discoveredFeatureCount] = 1;
          discoveredFeatureCount++;
          if(most < 1)
          {
            most = 1;
            curMarks[sliceIndex] = neighborGlobalIndex;
          }
        };

        // Check face neighbors in direct-path order: -Z, -Y, -X, +X, +Y, +Z.
        if(zIdx > 0)
        {
          considerNeighbor(prevSlice[sliceIndex], voxelIndex - dimX * dimY);
        }
        if(yIdx > 0)
        {
          considerNeighbor(curSlice[sliceIndex - dimX], voxelIndex - dimX);
        }
        if(xIdx > 0)
        {
          considerNeighbor(curSlice[sliceIndex - 1], voxelIndex - 1);
        }
        if(xIdx < dimX - 1)
        {
          considerNeighbor(curSlice[sliceIndex + 1], voxelIndex + 1);
        }
        if(yIdx < dimY - 1)
        {
          considerNeighbor(curSlice[sliceIndex + dimX], voxelIndex + dimX);
        }
        if(zIdx < dimZ - 1)
        {
          considerNeighbor(nextSlice[sliceIndex], voxelIndex + dimX * dimY);
        }
        if(curMarks[sliceIndex] >= 0)
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

    // Commit the previous slice after all votes that need its original IDs are complete.
    if(zIdx > 0)
    {
      auto commitResult = commitSlice(static_cast<usize>(zIdx - 1), prevMarks);
      if(commitResult.invalid())
      {
        return ConvertInvalidResult<bool>(std::move(commitResult));
      }
    }
    std::swap(prevMarks, curMarks);

    // Rotate the window and read the next required slice into the free buffer.
    std::swap(prevSlice, curSlice);
    std::swap(curSlice, nextSlice);
    if(zIdx + 2 < dimZ)
    {
      auto readResult = featureIdsStore.copyIntoBuffer(static_cast<usize>(zIdx + 2) * sliceSize, nonstd::span<int32>(nextSlice.data(), sliceSize));
      if(readResult.invalid())
      {
        return ConvertInvalidResult<bool>(std::move(readResult));
      }
    }
  }

  // Commit the final resolved mark slice after the read frontier exits the volume.
  if(dimZ > 0)
  {
    auto commitResult = commitSlice(static_cast<usize>(dimZ - 1), prevMarks);
    if(commitResult.invalid())
    {
      return ConvertInvalidResult<bool>(std::move(commitResult));
    }
  }

  return {shouldLoop};
}

/**
 * @brief Marks cells that belong to flagged features through bounded I/O.
 * @param featureIdsStore Provides and receives cell Feature IDs.
 * @param flaggedFeatures Identifies features selected for removal.
 * @param fillRemovedFeatures Uses -1 marks for later filling when true.
 * @param shouldCancel Stops before later chunks when true.
 * @return First bulk-I/O error, active-feature flags, or an empty vector when all are flagged.
 *
 * Feature-level flags remain resident. Cell Feature IDs use 65,536-tuple chunks.
 * IDs outside the flag range remain unchanged.
 */
Result<std::vector<bool>> FlagFeaturesScanline(Int32AbstractDataStore& featureIdsStore, std::unique_ptr<MaskCompareUtilities::MaskCompare>& flaggedFeatures, const bool fillRemovedFeatures,
                                               const std::atomic_bool& shouldCancel)
{
  bool good = false;
  const usize totalPoints = featureIdsStore.getNumberOfTuples();
  const usize totalFeatures = flaggedFeatures->getNumberOfTuples();
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
    return {std::vector<bool>{}};
  }

  constexpr usize k_ChunkSize = 65536;
  const int32 replacementValue = fillRemovedFeatures ? -1 : 0;
  auto chunkBuf = std::make_unique<int32[]>(k_ChunkSize);
  for(usize offset = 0; offset < totalPoints; offset += k_ChunkSize)
  {
    if(shouldCancel)
    {
      return {activeObjects};
    }
    const usize count = std::min(k_ChunkSize, totalPoints - offset);
    auto readResult = featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(chunkBuf.get(), count));
    if(readResult.invalid())
    {
      return ConvertInvalidResult<std::vector<bool>>(std::move(readResult));
    }
    bool chunkModified = false;
    for(usize i = 0; i < count; i++)
    {
      const int32 featureId = chunkBuf[i];
      if(featureId >= 0 && static_cast<usize>(featureId) < activeObjects.size() && !activeObjects[featureId])
      {
        chunkBuf[i] = replacementValue;
        chunkModified = true;
      }
    }
    if(chunkModified)
    {
      auto writeResult = featureIdsStore.copyFromBuffer(offset, nonstd::span<const int32>(chunkBuf.get(), count));
      if(writeResult.invalid())
      {
        return ConvertInvalidResult<std::vector<bool>>(std::move(writeResult));
      }
    }
  }
  return {activeObjects};
}

/**
 * @brief Applies active-feature renumbering through bounded I/O.
 * @param featureIdsStore Provides and receives cell Feature IDs.
 * @param activeObjects Selects feature tuples retained after removal.
 * @param shouldCancel Stops before later chunks when true.
 * @return First bulk-I/O error, or success after completion or cancellation.
 *
 * Negative and out-of-range IDs remain unchanged.
 */
Result<> RenumberFeatureIdsScanline(Int32AbstractDataStore& featureIdsStore, const std::vector<bool>& activeObjects, const std::atomic_bool& shouldCancel)
{
  const FeatureRenumbering renumbering = ComputeFeatureRenumbering(activeObjects);
  if(!renumbering.anyRemoved)
  {
    return {};
  }

  constexpr usize k_ChunkSize = 65536;
  auto values = std::make_unique<int32[]>(k_ChunkSize);
  const usize tupleCount = featureIdsStore.getNumberOfTuples();
  for(usize offset = 0; offset < tupleCount; offset += k_ChunkSize)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_ChunkSize, tupleCount - offset);
    auto readResult = featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(values.get(), count));
    if(readResult.invalid())
    {
      return readResult;
    }
    bool modified = false;
    for(usize i = 0; i < count; i++)
    {
      const int32 featureId = values[i];
      if(featureId >= 0 && static_cast<usize>(featureId) < renumbering.newNames.size())
      {
        const int32 remapped = static_cast<int32>(renumbering.newNames[static_cast<usize>(featureId)]);
        modified |= values[i] != remapped;
        values[i] = remapped;
      }
    }
    if(modified)
    {
      auto writeResult = featureIdsStore.copyFromBuffer(offset, nonstd::span<const int32>(values.get(), count));
      if(writeResult.invalid())
      {
        return writeResult;
      }
    }
  }
  return {};
}

/**
 * @class RunCropImageGeometryImpl
 * @brief Runs one CropImageGeometryFilter task for a flagged feature.
 *
 * The caller executes each task synchronously. This protects DataStructure mutation
 * and keeps borrowed loop-local paths and bounds alive.
 */
} // namespace

RemoveFlaggedFeaturesScanline::RemoveFlaggedFeaturesScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                             const RemoveFlaggedFeaturesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

RemoveFlaggedFeaturesScanline::~RemoveFlaggedFeaturesScanline() noexcept = default;

Result<> RemoveFlaggedFeaturesScanline::operator()()
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

  const IFilter::MessageHandler& messageHandler = m_MessageHandler;
  Result<> result;

  if(function != Functionality::Extract)
  {
    const usize totalFeatures = flaggedFeatures->getNumberOfTuples();
    if(totalFeatures < 2)
    {
      return MakeErrorResult(-45433, fmt::format("The feature Attribute Matrix '{}' has {} tuple(s). Tuple 0 is unused, so at least 2 tuples are required for a feature to survive removal. No data "
                                                 "was modified.",
                                                 m_InputValues->FlaggedFeaturesArrayPath.getParent().toString(), totalFeatures));
    }
    Result<> validationResult =
        ValidateFeatureIdsScanline(featureIds, imageGeom.getNumberOfCells(), totalFeatures, m_InputValues->FeatureIdsArrayPath, m_InputValues->FlaggedFeaturesArrayPath, m_ShouldCancel);
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
    m_MessageHandler.sendMessage(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Beginning Feature Extraction")});

    {
      ComputeFeatureRectFilter filter;
      Arguments args;

      args.insert(ComputeFeatureRectFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(m_InputValues->FeatureIdsArrayPath));
      args.insert(ComputeFeatureRectFilter::k_FeatureDataAttributeMatrixPath_Key, std::make_any<DataPath>(m_InputValues->TempBoundsPath.getParent()));
      args.insert(ComputeFeatureRectFilter::k_FeatureRectArrayName_Key, std::make_any<std::string>(m_InputValues->TempBoundsPath.getTargetName()));

      auto preflightResult = filter.preflight(m_DataStructure, args);
      if(preflightResult.outputActions.invalid())
      {
        return MakeErrorResult(-53901, fmt::format("Preflight of the feature bounding-box computation at '{}' for Feature IDs array '{}' failed: {}", m_InputValues->TempBoundsPath.toString(),
                                                   m_InputValues->FeatureIdsArrayPath.toString(), FirstRemoveFlaggedFeaturesErrorMessage(preflightResult.outputActions.errors())));
      }

      if(m_ShouldCancel)
      {
        return result;
      }

      auto executeResult = filter.execute(m_DataStructure, args);
      if(executeResult.result.invalid())
      {
        m_DataStructure.removeData(m_InputValues->TempBoundsPath);
        return MakeErrorResult(-53902, fmt::format("The feature bounding-box computation at '{}' for Feature IDs array '{}' failed: {}", m_InputValues->TempBoundsPath.toString(),
                                                   m_InputValues->FeatureIdsArrayPath.toString(), FirstRemoveFlaggedFeaturesErrorMessage(executeResult.result.errors())));
      }
    }

    const auto& boundsStore = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->TempBoundsPath).getDataStoreRef();
    std::vector<uint32> bounds(boundsStore.getSize());
    Result<> boundsReadResult = boundsStore.copyIntoBuffer(0, nonstd::span<uint32>(bounds.data(), bounds.size()));
    m_DataStructure.removeData(m_InputValues->TempBoundsPath);
    if(boundsReadResult.invalid())
    {
      return MergeResults(std::move(result), std::move(boundsReadResult));
    }

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
    std::vector<usize> emptyFeatures;
    for(usize i = 1; i < maxTuple && 6 * i + 5 < bounds.size(); i++)
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
        emptyFeatures.push_back(i);
        continue;
      }

      DataPath createdImgGeomPath({fmt::format(fmt::runtime("{}-{:0" + paddingWidth + "d}"), m_InputValues->CreatedImageGeometryPrefix, i)});

      m_MessageHandler.sendMessage(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Now Extracting Feature {}", i)});
      taskRunner.execute(RunCropImageGeometryImpl(m_DataStructure, m_ShouldCancel, m_InputValues->ImageGeometryPath, minVoxels, maxVoxels, createdImgGeomPath, cropTaskResult));

      // Stop scheduling crops once one has failed, so the failure is reported instead of
      // being buried behind later work.
      if(cropTaskResult.shouldAbort())
      {
        break;
      }
    }
    taskRunner.wait();

    if(!emptyFeatures.empty())
    {
      std::string listed;
      for(usize index = 0; index < std::min(emptyFeatures.size(), k_MaxListedEmptyFeatures); index++)
      {
        listed += fmt::format("{}{}", index == 0 ? "" : ", ", emptyFeatures[index]);
      }
      if(emptyFeatures.size() > k_MaxListedEmptyFeatures)
      {
        listed += ", ...";
      }
      result.warnings().push_back(Warning{k_EmptyFeatureSkippedWarning, fmt::format("{} flagged feature(s) own no cell in the Feature IDs array '{}' and were skipped; no geometry was created for "
                                                                                    "them. Feature ID(s): {}",
                                                                                    emptyFeatures.size(), m_InputValues->FeatureIdsArrayPath.toString(), listed)});
    }

    Result<> cropResult = cropTaskResult.takeResult();
    if(cropResult.invalid())
    {
      return MergeResults(std::move(result), std::move(cropResult));
    }

    m_MessageHandler.sendMessage(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("All Features Successfully Extracted")});
  }

  if(m_ShouldCancel)
  {
    return result;
  }

  // Remove and ExtractThenRemove modify the source feature data.
  if(function != Functionality::Extract)
  {
    m_MessageHandler.sendMessage(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Beginning Feature Removal")});

    auto activeObjectsResult = FlagFeaturesScanline(featureIds, flaggedFeatures, m_InputValues->FillRemovedFeatures, m_ShouldCancel);
    if(activeObjectsResult.invalid())
    {
      return ConvertResult(std::move(activeObjectsResult));
    }
    std::vector<bool> activeObjects = std::move(activeObjectsResult.value());
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
      std::vector<DataPath> ignoredPaths;
      ignoredPaths.reserve(m_InputValues->IgnoredDataArrayPaths.size());
      for(const DataPath& path : m_InputValues->IgnoredDataArrayPaths)
      {
        if(path == m_InputValues->FeatureIdsArrayPath)
        {
          result.warnings().push_back(Warning{k_FeatureIdsCannotBeIgnoredWarning, fmt::format("The Feature IDs array '{}' was listed among the arrays to ignore. It is the array being filled and "
                                                                                              "cannot be ignored, so it was removed from the ignore list.",
                                                                                              path.toString())});
          continue;
        }
        ignoredPaths.push_back(path);
      }
      std::vector<std::shared_ptr<IDataArray>> voxelArrays = GenerateDataArrayList(m_DataStructure, m_InputValues->FeatureIdsArrayPath, ignoredPaths);

      bool shouldLoop = false;
      usize count = 0;
      do
      {
        count++;
        m_MessageHandler.sendMessage(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Entering iteration number {}...", count)});

        usize replacementCount = 0;
        usize unresolvedCount = 0;
        auto fillResult = IdentifyAndFillNeighborsScanline(imageGeom, featureIds, voxelArrays, replacementCount, unresolvedCount, m_ShouldCancel, messageHandler);
        if(fillResult.invalid())
        {
          return ConvertResult(std::move(fillResult));
        }
        shouldLoop = fillResult.value();

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
      } while(shouldLoop);
    }

    if(m_ShouldCancel)
    {
      return result;
    }

    m_MessageHandler.sendMessage(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Stripping excess inactive objects from model...")});
    DataPath featureGroupPath = m_InputValues->FlaggedFeaturesArrayPath.getParent();
    auto renumberResult = RenumberFeatureIdsScanline(featureIds, activeObjects, m_ShouldCancel);
    if(renumberResult.invalid())
    {
      return MergeResults(std::move(result), std::move(renumberResult));
    }
    if(m_ShouldCancel)
    {
      return result;
    }
    Result<> removeResult = RemoveInactiveObjects(m_DataStructure, featureGroupPath, activeObjects, featureIds, flaggedFeatures->getNumberOfTuples(), m_MessageHandler, m_ShouldCancel,
                                                  /*cellFeatureIdsRenumbered=*/true);
    if(removeResult.invalid())
    {
      return MergeResults(std::move(result), std::move(removeResult));
    }
    // RemoveInactiveObjects reports a cancelled compaction as success. No work follows this
    // call, so a cancelled and a completed run both leave through the empty valid Result below.
  }

  return result;
}
