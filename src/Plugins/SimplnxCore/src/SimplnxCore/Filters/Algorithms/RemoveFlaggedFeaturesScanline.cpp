#include "RemoveFlaggedFeaturesScanline.hpp"

#include "RemoveFlaggedFeatures.hpp"

#include "SimplnxCore/Filters/ComputeFeatureRectFilter.hpp"
#include "SimplnxCore/Filters/CropImageGeometryFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

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
 * @param shouldCancel Stops before later Z slices when true.
 * @param messageHelper Creates a throttled progress messenger.
 * @return First bulk-I/O error, or whether any nonpositive Feature ID exists.
 *
 * Three input slices preserve the Feature-ID state at iteration start. Two mark
 * slices keep writes behind the vote frontier. This order prevents earlier writes
 * from changing later votes.
 *
 * Face neighbors use the direct algorithm's order. The first feature to exceed
 * the current vote count wins. A later tie does not replace that feature.
 */
Result<bool> IdentifyAndFillNeighborsScanline(const ImageGeom& imageGeom, Int32AbstractDataStore& featureIdsStore, const std::vector<std::shared_ptr<IDataArray>>& voxelArrays,
                                              const std::atomic_bool& shouldCancel, MessageHelper& messageHelper)
{
  ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger();

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
      throttledMessenger.sendThrottledMessage([&]() { return fmt::format("Processing Image... {:.2f}%", CalculatePercentComplete(zIdx, dimZ)); });
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
        if(featureName > 0)
        {
          continue;
        }
        shouldLoop = true;

        int32 current = 0;
        int32 most = 0;
        std::array<int32, 6> numHits = {0, 0, 0, 0, 0, 0};
        std::array<int32, 6> discoveredFeatures = {0, 0, 0, 0, 0, 0};
        usize discoveredFeatureCount = 0;

        // Preserve the direct algorithm's vote rule. Only negative destinations
        // receive a source mark, although zero IDs still request another iteration.
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
          discoveredFeatureCount++;
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
class RunCropImageGeometryImpl
{
public:
  /**
   * @brief Creates one borrowed crop task.
   * @param dataStructure Receives the cropped geometry.
   * @param shouldCancel Stops before delegated execution when true.
   * @param imageGeometryPath Identifies the source ImageGeom.
   * @param minVoxelVector Specifies inclusive minimum voxel indexes.
   * @param maxVoxelVector Specifies inclusive maximum voxel indexes.
   * @param createdImgGeomPath Identifies the cropped ImageGeom.
   */
  RunCropImageGeometryImpl(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const DataPath& imageGeometryPath, const std::vector<uint64>& minVoxelVector,
                           const std::vector<uint64>& maxVoxelVector, const DataPath& createdImgGeomPath)
  : m_DataStructure(dataStructure)
  , m_ShouldCancel(shouldCancel)
  , m_ImageGeometryPath(imageGeometryPath)
  , m_MinVoxelVector(minVoxelVector)
  , m_MaxVoxelVector(maxVoxelVector)
  , m_CreatedImgGeomPath(createdImgGeomPath)
  {
  }

  /**
   * @brief Destroys the borrowed crop task.
   */
  ~RunCropImageGeometryImpl() = default;

  /**
   * @brief Preflights and executes the delegated crop.
   *
   * The task throws after a preflight failure. The implementation does not inspect
   * the execute result and can therefore ignore an execution failure.
   */
  void operator()() const
  {
    CropImageGeometryFilter filter;

    Arguments args;

    args.insertOrAssign(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
    args.insertOrAssign(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(m_ImageGeometryPath));
    args.insertOrAssign(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
    args.insertOrAssign(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(false));

    args.insertOrAssign(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(m_MinVoxelVector));
    args.insertOrAssign(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(m_MaxVoxelVector));
    args.insertOrAssign(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(m_CreatedImgGeomPath));

    auto preflightResult = filter.preflight(m_DataStructure, args);
    if(preflightResult.outputActions.invalid())
    {
      throw std::runtime_error("Preflight failed when cropping the geometry in extract flagged features!");
    }

    if(m_ShouldCancel)
    {
      return;
    }

    auto executeResult = filter.execute(m_DataStructure, args);
    if(preflightResult.outputActions.invalid())
    {
      throw std::runtime_error("Execute failed when cropping the geometry in extract flagged features!");
    }
  }

private:
  DataStructure& m_DataStructure;
  const std::atomic_bool& m_ShouldCancel;
  const DataPath& m_ImageGeometryPath;
  const std::vector<uint64>& m_MinVoxelVector;
  const std::vector<uint64>& m_MaxVoxelVector;
  const DataPath& m_CreatedImgGeomPath;
};
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

  MessageHelper messageHelper(m_MessageHandler);

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
        throw std::runtime_error("Preflight failed when cropping the geometry in extract flagged features!");
      }

      if(m_ShouldCancel)
      {
        return {};
      }

      auto executeResult = filter.execute(m_DataStructure, args);
      // Only preflight status controls this path. The delegated execute result is not inspected.
      if(preflightResult.outputActions.invalid())
      {
        throw std::runtime_error("Execute failed when cropping the geometry in extract flagged features!");
      }
    }

    auto bounds = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->TempBoundsPath);

    if(m_ShouldCancel)
    {
      return {};
    }

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
        return {};
      }

      if(!flaggedFeatures->isTrue(i))
      {
        continue;
      }

      usize index = 6 * i;
      std::vector<uint64> minVoxels = {static_cast<uint64>(bounds[index]), static_cast<uint64>(bounds[index + 1]), static_cast<uint64>(bounds[index + 2])};
      std::vector<uint64> maxVoxels = {static_cast<uint64>(bounds[index + 3]), static_cast<uint64>(bounds[index + 4]), static_cast<uint64>(bounds[index + 5])};

      DataPath createdImgGeomPath({fmt::format(fmt::runtime("{}-{:0" + paddingWidth + "d}"), m_InputValues->CreatedImageGeometryPrefix, i)});

      m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Now Extracting Feature {}", i)});
      taskRunner.execute(RunCropImageGeometryImpl(m_DataStructure, m_ShouldCancel, m_InputValues->ImageGeometryPath, minVoxels, maxVoxels, createdImgGeomPath));
    }
    taskRunner.wait();

    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("All Features Successfully Extracted")});
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  // Remove and ExtractThenRemove modify the source feature data.
  if(function != Functionality::Extract)
  {
    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Beginning Feature Removal")});

    auto activeObjectsResult = FlagFeaturesScanline(featureIds, flaggedFeatures, m_InputValues->FillRemovedFeatures, m_ShouldCancel);
    if(activeObjectsResult.invalid())
    {
      return ConvertResult(std::move(activeObjectsResult));
    }
    std::vector<bool> activeObjects = std::move(activeObjectsResult.value());
    if(activeObjects.empty())
    {
      return MakeErrorResult(-45433, "All Features were flagged and would all be removed. The filter has quit.");
    }

    if(m_ShouldCancel)
    {
      return {};
    }

    if(m_InputValues->FillRemovedFeatures)
    {
      bool shouldLoop;
      usize count = 0;
      do
      {
        count++;
        m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Entering iteration number {}...", count)});

        // Rebuild the retained cell-array list for each convergence iteration.
        // Feature IDs must remain in this list for negative IDs to be replaced.
        std::vector<std::shared_ptr<IDataArray>> voxelArrays = GenerateDataArrayList(m_DataStructure, m_InputValues->FeatureIdsArrayPath, m_InputValues->IgnoredDataArrayPaths);
        auto fillResult = IdentifyAndFillNeighborsScanline(imageGeom, featureIds, voxelArrays, m_ShouldCancel, messageHelper);
        if(fillResult.invalid())
        {
          return ConvertResult(std::move(fillResult));
        }
        shouldLoop = fillResult.value();

        if(m_ShouldCancel)
        {
          return {};
        }
      } while(shouldLoop);
    }

    if(m_ShouldCancel)
    {
      return {};
    }

    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Stripping excess inactive objects from model...")});
    DataPath featureGroupPath = m_InputValues->FlaggedFeaturesArrayPath.getParent();
    auto renumberResult = RenumberFeatureIdsScanline(featureIds, activeObjects, m_ShouldCancel);
    if(renumberResult.invalid())
    {
      return renumberResult;
    }
    if(m_ShouldCancel)
    {
      return {};
    }
    if(!RemoveInactiveObjects(m_DataStructure, featureGroupPath, activeObjects, featureIds, flaggedFeatures->getNumberOfTuples(), m_MessageHandler, m_ShouldCancel,
                              /*cellFeatureIdsRenumbered=*/true))
    {
      return MakeErrorResult(-45434, fmt::format("Failed to remove inactive objects from feature group at path '{}'.", featureGroupPath.toString()));
    }
  }

  return {};
}
