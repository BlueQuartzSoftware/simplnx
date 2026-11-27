#include "FillBadVoxels.hpp"

#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <memory>

using namespace nx::core;

namespace
{
/**
 * @struct FillScan
 * @brief Counts unresolved and currently fillable cells.
 */
struct FillScan
{
  usize bad_count = 0;
  usize fillable_count = 0;
};

/**
 * @struct NeighborSelection
 * @brief Identifies the selected neighbor direction and Feature ID.
 */
struct NeighborSelection
{
  int8 direction = -1;
  int32 feature_id = -1;
};

template <typename T>
Result<> ReadSlice(const AbstractDataStore<T>& store, usize zIndex, usize sliceValues, T* destination)
{
  return store.copyIntoBuffer(zIndex * sliceValues, nonstd::span<T>(destination, sliceValues));
}

template <typename T>
Result<> WriteSlice(AbstractDataStore<T>& store, usize zIndex, usize sliceValues, const T* source)
{
  return store.copyFromBuffer(zIndex * sliceValues, nonstd::span<const T>(source, sliceValues));
}

// Select the first neighbor whose Feature ID reaches the largest vote count.
inline NeighborSelection SelectNeighbor(const int32* previousSlice, const int32* currentSlice, const int32* nextSlice, usize localIndex, usize xIndex, usize yIndex, usize zIndex,
                                        const std::array<usize, 3>& dimensions)
{
  std::array<int32, 6> votedFeatureIds{};
  std::array<uint8, 6> voteCounts{};
  usize uniqueVoteCount = 0;
  uint8 largestVoteCount = 0;
  NeighborSelection selection;

  const auto considerNeighbor = [&](int32 featureId, int8 direction) {
    if(featureId < 0)
    {
      return;
    }

    usize voteIndex = 0;
    while(voteIndex < uniqueVoteCount && votedFeatureIds[voteIndex] != featureId)
    {
      voteIndex++;
    }
    if(voteIndex == uniqueVoteCount)
    {
      votedFeatureIds[uniqueVoteCount] = featureId;
      uniqueVoteCount++;
    }

    const uint8 currentVoteCount = ++voteCounts[voteIndex];
    if(currentVoteCount > largestVoteCount)
    {
      largestVoteCount = currentVoteCount;
      selection.direction = direction;
      selection.feature_id = featureId;
    }
  };

  const usize xDimension = dimensions[0];
  if(zIndex > 0)
  {
    considerNeighbor(previousSlice[localIndex], 0);
  }
  if(yIndex > 0)
  {
    considerNeighbor(currentSlice[localIndex - xDimension], 1);
  }
  if(xIndex > 0)
  {
    considerNeighbor(currentSlice[localIndex - 1], 2);
  }
  if(xIndex + 1 < xDimension)
  {
    considerNeighbor(currentSlice[localIndex + 1], 3);
  }
  if(yIndex + 1 < dimensions[1])
  {
    considerNeighbor(currentSlice[localIndex + xDimension], 4);
  }
  if(zIndex + 1 < dimensions[2])
  {
    considerNeighbor(nextSlice[localIndex], 5);
  }

  return selection;
}

Result<> ScanFeatureIds(const Int32AbstractDataStoreType& featureIds, const std::array<usize, 3>& dimensions, std::optional<usize> maxFeatureCount, const std::atomic_bool& shouldCancel,
                        FillScan& scan)
{
  const usize sliceSize = dimensions[0] * dimensions[1];
  auto featureBuffer = std::make_unique<int32[]>(3 * sliceSize);
  int32* previousSlicePtr = featureBuffer.get();
  int32* currentSlicePtr = featureBuffer.get() + sliceSize;
  int32* nextSlicePtr = featureBuffer.get() + 2 * sliceSize;

  Result<> readResult = ReadSlice(featureIds, 0, sliceSize, currentSlicePtr);
  if(readResult.invalid())
  {
    return readResult;
  }
  if(dimensions[2] > 1)
  {
    readResult = ReadSlice(featureIds, 1, sliceSize, nextSlicePtr);
    if(readResult.invalid())
    {
      return readResult;
    }
  }

  for(usize zIndex = 0; zIndex < dimensions[2]; zIndex++)
  {
    if(shouldCancel)
    {
      return {};
    }

    for(usize yIndex = 0; yIndex < dimensions[1]; yIndex++)
    {
      const usize rowOffset = yIndex * dimensions[0];
      for(usize xIndex = 0; xIndex < dimensions[0]; xIndex++)
      {
        const usize localIndex = rowOffset + xIndex;
        const int32 featureId = currentSlicePtr[localIndex];
        if(featureId >= 0)
        {
          if(maxFeatureCount.has_value() && static_cast<usize>(featureId) >= *maxFeatureCount)
          {
            return MakeErrorResult(
                -55567, fmt::format("Error: Found a feature Id '{}' that is >= the number of features '{}' at voxel index X={},Y={},Z={}.", featureId, *maxFeatureCount, xIndex, yIndex, zIndex));
          }
          continue;
        }

        scan.bad_count++;
        if(SelectNeighbor(previousSlicePtr, currentSlicePtr, nextSlicePtr, localIndex, xIndex, yIndex, zIndex, dimensions).direction >= 0)
        {
          scan.fillable_count++;
        }
      }
    }

    std::swap(previousSlicePtr, currentSlicePtr);
    std::swap(currentSlicePtr, nextSlicePtr);
    if(zIndex + 2 < dimensions[2])
    {
      readResult = ReadSlice(featureIds, zIndex + 2, sliceSize, nextSlicePtr);
      if(readResult.invalid())
      {
        return readResult;
      }
    }
  }

  return {};
}

/**
 * @struct FillArrayFunctor
 * @brief Copies one sibling array from the current Feature ID snapshot.
 *
 * Three Feature ID slices and three target slices bound working memory. Store
 * errors are returned, and cancellation can leave completed target slices.
 */
struct FillArrayFunctor
{
  template <typename T>
  Result<> operator()(IDataArray& array, const Int32AbstractDataStoreType& featureIds, const std::array<usize, 3>& dimensions, const std::atomic_bool& shouldCancel) const
  {
    auto& targetStore = array.template getIDataStoreRefAs<AbstractDataStore<T>>();
    const usize sliceSize = dimensions[0] * dimensions[1];
    const usize componentCount = targetStore.getNumberOfComponents();
    const usize targetSliceValues = sliceSize * componentCount;

    auto featureBuffer = std::make_unique<int32[]>(3 * sliceSize);
    int32* previousFeaturesPtr = featureBuffer.get();
    int32* currentFeaturesPtr = featureBuffer.get() + sliceSize;
    int32* nextFeaturesPtr = featureBuffer.get() + 2 * sliceSize;

    auto targetBuffer = std::make_unique<T[]>(3 * targetSliceValues);
    T* previousTargetPtr = targetBuffer.get();
    T* currentTargetPtr = targetBuffer.get() + targetSliceValues;
    T* nextTargetPtr = targetBuffer.get() + 2 * targetSliceValues;

    Result<> ioResult = ReadSlice(featureIds, 0, sliceSize, currentFeaturesPtr);
    if(ioResult.invalid())
    {
      return ioResult;
    }
    ioResult = ReadSlice(targetStore, 0, targetSliceValues, currentTargetPtr);
    if(ioResult.invalid())
    {
      return ioResult;
    }
    if(dimensions[2] > 1)
    {
      ioResult = ReadSlice(featureIds, 1, sliceSize, nextFeaturesPtr);
      if(ioResult.invalid())
      {
        return ioResult;
      }
      ioResult = ReadSlice(targetStore, 1, targetSliceValues, nextTargetPtr);
      if(ioResult.invalid())
      {
        return ioResult;
      }
    }

    for(usize zIndex = 0; zIndex < dimensions[2]; zIndex++)
    {
      if(shouldCancel)
      {
        return {};
      }

      bool modified = false;
      for(usize yIndex = 0; yIndex < dimensions[1]; yIndex++)
      {
        const usize rowOffset = yIndex * dimensions[0];
        for(usize xIndex = 0; xIndex < dimensions[0]; xIndex++)
        {
          const usize localIndex = rowOffset + xIndex;
          if(currentFeaturesPtr[localIndex] >= 0)
          {
            continue;
          }

          const NeighborSelection selection = SelectNeighbor(previousFeaturesPtr, currentFeaturesPtr, nextFeaturesPtr, localIndex, xIndex, yIndex, zIndex, dimensions);
          if(selection.direction < 0)
          {
            continue;
          }

          const T* sourceTuplePtr = nullptr;
          switch(selection.direction)
          {
          case 0:
            sourceTuplePtr = previousTargetPtr + localIndex * componentCount;
            break;
          case 1:
            sourceTuplePtr = currentTargetPtr + (localIndex - dimensions[0]) * componentCount;
            break;
          case 2:
            sourceTuplePtr = currentTargetPtr + (localIndex - 1) * componentCount;
            break;
          case 3:
            sourceTuplePtr = currentTargetPtr + (localIndex + 1) * componentCount;
            break;
          case 4:
            sourceTuplePtr = currentTargetPtr + (localIndex + dimensions[0]) * componentCount;
            break;
          case 5:
            sourceTuplePtr = nextTargetPtr + localIndex * componentCount;
            break;
          default:
            break;
          }

          T* destinationTuplePtr = currentTargetPtr + localIndex * componentCount;
          std::copy_n(sourceTuplePtr, componentCount, destinationTuplePtr);
          modified = true;
        }
      }

      if(modified)
      {
        ioResult = WriteSlice(targetStore, zIndex, targetSliceValues, currentTargetPtr);
        if(ioResult.invalid())
        {
          return ioResult;
        }
      }

      std::swap(previousFeaturesPtr, currentFeaturesPtr);
      std::swap(currentFeaturesPtr, nextFeaturesPtr);
      std::swap(previousTargetPtr, currentTargetPtr);
      std::swap(currentTargetPtr, nextTargetPtr);
      if(zIndex + 2 < dimensions[2])
      {
        ioResult = ReadSlice(featureIds, zIndex + 2, sliceSize, nextFeaturesPtr);
        if(ioResult.invalid())
        {
          return ioResult;
        }
        ioResult = ReadSlice(targetStore, zIndex + 2, targetSliceValues, nextTargetPtr);
        if(ioResult.invalid())
        {
          return ioResult;
        }
      }
    }

    return {};
  }
};

Result<> FillFeatureIds(Int32AbstractDataStoreType& featureIds, const std::array<usize, 3>& dimensions, const std::atomic_bool& shouldCancel)
{
  const usize sliceSize = dimensions[0] * dimensions[1];
  auto featureBuffer = std::make_unique<int32[]>(3 * sliceSize);
  auto outputSlice = std::make_unique<int32[]>(sliceSize);
  int32* previousSlicePtr = featureBuffer.get();
  int32* currentSlicePtr = featureBuffer.get() + sliceSize;
  int32* nextSlicePtr = featureBuffer.get() + 2 * sliceSize;

  Result<> ioResult = ReadSlice(featureIds, 0, sliceSize, currentSlicePtr);
  if(ioResult.invalid())
  {
    return ioResult;
  }
  if(dimensions[2] > 1)
  {
    ioResult = ReadSlice(featureIds, 1, sliceSize, nextSlicePtr);
    if(ioResult.invalid())
    {
      return ioResult;
    }
  }

  for(usize zIndex = 0; zIndex < dimensions[2]; zIndex++)
  {
    if(shouldCancel)
    {
      return {};
    }

    std::copy_n(currentSlicePtr, sliceSize, outputSlice.get());
    bool modified = false;
    for(usize yIndex = 0; yIndex < dimensions[1]; yIndex++)
    {
      const usize rowOffset = yIndex * dimensions[0];
      for(usize xIndex = 0; xIndex < dimensions[0]; xIndex++)
      {
        const usize localIndex = rowOffset + xIndex;
        if(currentSlicePtr[localIndex] >= 0)
        {
          continue;
        }

        const NeighborSelection selection = SelectNeighbor(previousSlicePtr, currentSlicePtr, nextSlicePtr, localIndex, xIndex, yIndex, zIndex, dimensions);
        if(selection.direction >= 0)
        {
          outputSlice[localIndex] = selection.feature_id;
          modified = true;
        }
      }
    }

    if(modified)
    {
      ioResult = WriteSlice(featureIds, zIndex, sliceSize, outputSlice.get());
      if(ioResult.invalid())
      {
        return ioResult;
      }
    }

    std::swap(previousSlicePtr, currentSlicePtr);
    std::swap(currentSlicePtr, nextSlicePtr);
    if(zIndex + 2 < dimensions[2])
    {
      ioResult = ReadSlice(featureIds, zIndex + 2, sliceSize, nextSlicePtr);
      if(ioResult.invalid())
      {
        return ioResult;
      }
    }
  }

  return {};
}
} // namespace

Result<> nx::core::FillBadVoxels(DataStructure& dataStructure, const DataPath& featureIdsPath, const SizeVec3Type& dimensions, const std::vector<DataPath>& ignoredArrayPaths,
                                 std::optional<usize> maxFeatureCount, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
{
  if(dimensions[0] == 0 || dimensions[1] == 0 || dimensions[2] == 0)
  {
    return {};
  }

  auto& featureIdsArray = dataStructure.getDataRefAs<Int32ArrayType>(featureIdsPath);
  auto& featureIds = featureIdsArray.getDataStoreRef();
  const std::array<usize, 3> dims = {dimensions[0], dimensions[1], dimensions[2]};

  const std::vector<std::shared_ptr<IDataArray>> cellArrays = GenerateDataArrayList(dataStructure, featureIdsPath, ignoredArrayPaths);
  usize iteration = 0;
  while(!shouldCancel)
  {
    FillScan scan;
    Result<> scanResult = ScanFeatureIds(featureIds, dims, maxFeatureCount, shouldCancel, scan);
    if(scanResult.invalid())
    {
      if(!scanResult.errors().empty())
      {
        messageHandler.sendMessage({.type = IFilter::Message::Type::Info, .message = scanResult.errors().front().message});
      }
      return scanResult;
    }
    if(shouldCancel || scan.bad_count == 0)
    {
      break;
    }
    if(scan.fillable_count == 0)
    {
      const std::string message = fmt::format(
          "Unable to reassign {} cell(s) in Feature Ids array '{}' because none has a non-negative face neighbor. Ensure the array contains at least one cell assigned to a retained feature.",
          scan.bad_count, featureIdsPath.toString());
      messageHandler.sendMessage({.type = IFilter::Message::Type::Info, .message = message});
      return MakeErrorResult(-55572, message);
    }

    iteration++;

    for(const auto& cellArray : cellArrays)
    {
      if(shouldCancel)
      {
        return {};
      }
      if(cellArray.get() == &featureIdsArray)
      {
        continue;
      }

      Result<> fillResult = ExecuteDataFunction(FillArrayFunctor{}, cellArray->getDataType(), *cellArray, featureIds, dims, shouldCancel);
      if(fillResult.invalid())
      {
        return fillResult;
      }
    }

    Result<> featureFillResult = FillFeatureIds(featureIds, dims, shouldCancel);
    if(featureFillResult.invalid())
    {
      return featureFillResult;
    }
  }

  return {};
}
