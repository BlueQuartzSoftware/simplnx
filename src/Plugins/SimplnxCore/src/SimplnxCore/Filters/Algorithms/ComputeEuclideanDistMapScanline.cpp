#include "ComputeEuclideanDistMapScanline.hpp"

#include "ComputeEuclideanDistMap.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <type_traits>
#include <vector>

using namespace nx::core;

namespace
{
// The filter supports boundary, triple-line, and quad-point maps.
constexpr usize k_MapCount = 3;

/**
 * @brief Defines output-store pointers indexed by map type.
 * @tparam T Specifies the distance element type.
 */
template <typename T>
using DistanceStoreArray = std::array<AbstractDataStore<T>*, k_MapCount>;

/**
 * @brief Collects enabled distance-map stores.
 * @tparam T Specifies the distance element type.
 * @param dataStructure Contains selected output arrays.
 * @param inputValues Selects map types and output paths.
 * @return Store pointers indexed by ComputeEuclideanDistMap::MapType.
 */
template <typename T>
DistanceStoreArray<T> GetDistanceStores(DataStructure& dataStructure, const ComputeEuclideanDistMapInputValues& inputValues)
{
  DistanceStoreArray<T> stores = {nullptr, nullptr, nullptr};
  if(inputValues.DoBoundaries)
  {
    stores[static_cast<usize>(ComputeEuclideanDistMap::MapType::FeatureBoundary)] = dataStructure.getDataRefAs<DataArray<T>>(inputValues.GBDistancesArrayPath).getDataStore();
  }
  if(inputValues.DoTripleLines)
  {
    stores[static_cast<usize>(ComputeEuclideanDistMap::MapType::TripleJunction)] = dataStructure.getDataRefAs<DataArray<T>>(inputValues.TJDistancesArrayPath).getDataStore();
  }
  if(inputValues.DoQuadPoints)
  {
    stores[static_cast<usize>(ComputeEuclideanDistMap::MapType::QuadPoint)] = dataStructure.getDataRefAs<DataArray<T>>(inputValues.QPDistancesArrayPath).getDataStore();
  }
  return stores;
}

// Seed classification counts each distinct neighboring Feature ID once.
void AddUniqueNeighbor(std::array<int32, 6>& coordination, usize& coordinationCount, int32 feature, int32 neighbor)
{
  if(neighbor < 0 || neighbor == feature)
  {
    return;
  }

  for(usize index = 0; index < coordinationCount; index++)
  {
    if(coordination[index] == neighbor)
    {
      return;
    }
  }
  coordination[coordinationCount++] = neighbor;
}

/**
 * @brief Initializes requested map seeds from Feature IDs.
 * @tparam T Specifies the distance element type.
 * @param featureIds Supplies cell Feature IDs.
 * @param distanceStores Receives selected seed maps.
 * @param dims Supplies image dimensions.
 * @param shouldCancel Signals cancellation between Z slices.
 * @param hasBlockedCells Receives whether a non-positive Feature ID exists.
 * @return The first bulk-I/O error. Cancellation returns success.
 *
 * Three Feature ID slices bound resident source memory. Completed seed slices remain after cancellation.
 */
template <typename T>
Result<> InitializeSeeds(const Int32AbstractDataStore& featureIds, const DistanceStoreArray<T>& distanceStores, const SizeVec3& dims, const std::atomic_bool& shouldCancel, bool& hasBlockedCells)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  const usize sliceSize = dimX * dimY;

  std::vector<int32> previousFeatureIds(sliceSize);
  std::vector<int32> currentFeatureIds(sliceSize);
  std::vector<int32> nextFeatureIds(sliceSize);
  std::array<std::vector<T>, k_MapCount> outputSlices;
  for(usize mapIndex = 0; mapIndex < k_MapCount; mapIndex++)
  {
    if(distanceStores[mapIndex] != nullptr)
    {
      outputSlices[mapIndex].resize(sliceSize);
    }
  }

  Result<> ioResult = featureIds.copyIntoBuffer(0, nonstd::span<int32>(currentFeatureIds.data(), sliceSize));
  if(ioResult.invalid())
  {
    return ioResult;
  }
  if(dimZ > 1)
  {
    ioResult = featureIds.copyIntoBuffer(sliceSize, nonstd::span<int32>(nextFeatureIds.data(), sliceSize));
    if(ioResult.invalid())
    {
      return ioResult;
    }
  }

  hasBlockedCells = false;
  for(usize zIndex = 0; zIndex < dimZ; zIndex++)
  {
    if(shouldCancel)
    {
      return {};
    }

    for(usize mapIndex = 0; mapIndex < k_MapCount; mapIndex++)
    {
      if(distanceStores[mapIndex] != nullptr)
      {
        std::fill(outputSlices[mapIndex].begin(), outputSlices[mapIndex].end(), static_cast<T>(-1));
      }
    }

    for(usize yIndex = 0; yIndex < dimY; yIndex++)
    {
      const usize rowOffset = yIndex * dimX;
      for(usize xIndex = 0; xIndex < dimX; xIndex++)
      {
        const usize sliceIndex = rowOffset + xIndex;
        const int32 feature = currentFeatureIds[sliceIndex];
        if(feature <= 0)
        {
          hasBlockedCells = true;
          continue;
        }

        std::array<int32, 6> coordination = {};
        usize coordinationCount = 0;
        if(zIndex > 0)
        {
          AddUniqueNeighbor(coordination, coordinationCount, feature, previousFeatureIds[sliceIndex]);
        }
        if(yIndex > 0)
        {
          AddUniqueNeighbor(coordination, coordinationCount, feature, currentFeatureIds[sliceIndex - dimX]);
        }
        if(xIndex > 0)
        {
          AddUniqueNeighbor(coordination, coordinationCount, feature, currentFeatureIds[sliceIndex - 1]);
        }
        if(xIndex + 1 < dimX)
        {
          AddUniqueNeighbor(coordination, coordinationCount, feature, currentFeatureIds[sliceIndex + 1]);
        }
        if(yIndex + 1 < dimY)
        {
          AddUniqueNeighbor(coordination, coordinationCount, feature, currentFeatureIds[sliceIndex + dimX]);
        }
        if(zIndex + 1 < dimZ)
        {
          AddUniqueNeighbor(coordination, coordinationCount, feature, nextFeatureIds[sliceIndex]);
        }

        if(coordinationCount > 0 && distanceStores[static_cast<usize>(ComputeEuclideanDistMap::MapType::FeatureBoundary)] != nullptr)
        {
          outputSlices[static_cast<usize>(ComputeEuclideanDistMap::MapType::FeatureBoundary)][sliceIndex] = static_cast<T>(0);
        }
        if(coordinationCount >= 2 && distanceStores[static_cast<usize>(ComputeEuclideanDistMap::MapType::TripleJunction)] != nullptr)
        {
          outputSlices[static_cast<usize>(ComputeEuclideanDistMap::MapType::TripleJunction)][sliceIndex] = static_cast<T>(0);
        }
        if(coordinationCount > 2 && distanceStores[static_cast<usize>(ComputeEuclideanDistMap::MapType::QuadPoint)] != nullptr)
        {
          outputSlices[static_cast<usize>(ComputeEuclideanDistMap::MapType::QuadPoint)][sliceIndex] = static_cast<T>(0);
        }
      }
    }

    const usize sliceOffset = zIndex * sliceSize;
    for(usize mapIndex = 0; mapIndex < k_MapCount; mapIndex++)
    {
      if(distanceStores[mapIndex] != nullptr)
      {
        ioResult = distanceStores[mapIndex]->copyFromBuffer(sliceOffset, nonstd::span<const T>(outputSlices[mapIndex].data(), sliceSize));
        if(ioResult.invalid())
        {
          return ioResult;
        }
      }
    }

    std::swap(previousFeatureIds, currentFeatureIds);
    std::swap(currentFeatureIds, nextFeatureIds);
    if(zIndex + 2 < dimZ)
    {
      ioResult = featureIds.copyIntoBuffer((zIndex + 2) * sliceSize, nonstd::span<int32>(nextFeatureIds.data(), sliceSize));
      if(ioResult.invalid())
      {
        return ioResult;
      }
    }
  }
  return {};
}

/**
 * @brief Propagates one lower distance into a current value.
 * @tparam T Specifies the distance element type.
 * @param neighborDistance Supplies the adjacent distance.
 * @param currentDistance Receives the lower reachable distance.
 */
template <typename T>
void ConsiderDistance(T neighborDistance, T& currentDistance)
{
  if(neighborDistance < static_cast<T>(0))
  {
    return;
  }

  const T candidate = neighborDistance + static_cast<T>(1);
  if(currentDistance < static_cast<T>(0) || candidate < currentDistance)
  {
    currentDistance = candidate;
  }
}

/**
 * @brief Runs forward and backward city-block sweeps without blocked cells.
 * @tparam T Specifies the distance element type.
 * @param distances Stores the map to transform.
 * @param dims Supplies image dimensions.
 * @param shouldCancel Signals cancellation between Z slices.
 * @return The first bulk-I/O error. Cancellation returns success.
 *
 * The sweeps retain two slices.
 */
template <typename T>
Result<> TransformDistanceWithoutObstacles(AbstractDataStore<T>& distances, const SizeVec3& dims, const std::atomic_bool& shouldCancel)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  const usize sliceSize = dimX * dimY;

  std::vector<T> previousSlice(sliceSize);
  std::vector<T> currentSlice(sliceSize);

  for(usize zIndex = 0; zIndex < dimZ; zIndex++)
  {
    if(shouldCancel)
    {
      return {};
    }

    const usize sliceOffset = zIndex * sliceSize;
    Result<> ioResult = distances.copyIntoBuffer(sliceOffset, nonstd::span<T>(currentSlice.data(), sliceSize));
    if(ioResult.invalid())
    {
      return ioResult;
    }
    for(usize yIndex = 0; yIndex < dimY; yIndex++)
    {
      const usize rowOffset = yIndex * dimX;
      for(usize xIndex = 0; xIndex < dimX; xIndex++)
      {
        const usize sliceIndex = rowOffset + xIndex;
        T& distance = currentSlice[sliceIndex];
        if(xIndex > 0)
        {
          ConsiderDistance(currentSlice[sliceIndex - 1], distance);
        }
        if(yIndex > 0)
        {
          ConsiderDistance(currentSlice[sliceIndex - dimX], distance);
        }
        if(zIndex > 0)
        {
          ConsiderDistance(previousSlice[sliceIndex], distance);
        }
      }
    }
    ioResult = distances.copyFromBuffer(sliceOffset, nonstd::span<const T>(currentSlice.data(), sliceSize));
    if(ioResult.invalid())
    {
      return ioResult;
    }
    std::swap(previousSlice, currentSlice);
  }

  std::vector<T> nextSlice(sliceSize);
  for(usize reverseZ = dimZ; reverseZ > 0; reverseZ--)
  {
    if(shouldCancel)
    {
      return {};
    }

    const usize zIndex = reverseZ - 1;
    const usize sliceOffset = zIndex * sliceSize;
    Result<> ioResult = distances.copyIntoBuffer(sliceOffset, nonstd::span<T>(currentSlice.data(), sliceSize));
    if(ioResult.invalid())
    {
      return ioResult;
    }
    for(usize reverseY = dimY; reverseY > 0; reverseY--)
    {
      const usize yIndex = reverseY - 1;
      const usize rowOffset = yIndex * dimX;
      for(usize reverseX = dimX; reverseX > 0; reverseX--)
      {
        const usize xIndex = reverseX - 1;
        const usize sliceIndex = rowOffset + xIndex;
        T& distance = currentSlice[sliceIndex];
        if(xIndex + 1 < dimX)
        {
          ConsiderDistance(currentSlice[sliceIndex + 1], distance);
        }
        if(yIndex + 1 < dimY)
        {
          ConsiderDistance(currentSlice[sliceIndex + dimX], distance);
        }
        if(zIndex + 1 < dimZ)
        {
          ConsiderDistance(nextSlice[sliceIndex], distance);
        }
      }
    }
    ioResult = distances.copyFromBuffer(sliceOffset, nonstd::span<const T>(currentSlice.data(), sliceSize));
    if(ioResult.invalid())
    {
      return ioResult;
    }
    std::swap(nextSlice, currentSlice);
  }
  return {};
}

/**
 * @brief Propagates a labeled distance into a current value.
 * @tparam T Specifies the distance element type.
 * @param neighborDistance Supplies the adjacent distance.
 * @param neighborSeed Identifies the adjacent nearest seed.
 * @param currentDistance Receives the selected distance.
 * @param currentSeed Receives the selected nearest seed.
 *
 * Equal distances select the larger seed index.
 */
template <typename T>
void ConsiderLabeledDistance(T neighborDistance, int64 neighborSeed, T& currentDistance, int64& currentSeed)
{
  if(neighborDistance < static_cast<T>(0) || neighborSeed < 0)
  {
    return;
  }

  const T candidate = neighborDistance + static_cast<T>(1);
  if(currentDistance < static_cast<T>(0) || candidate < currentDistance || (candidate == currentDistance && neighborSeed > currentSeed))
  {
    currentDistance = candidate;
    currentSeed = neighborSeed;
  }
}

/**
 * @brief Creates one nearest-seed entry for each zero-distance value.
 * @tparam T Specifies the distance element type.
 * @param distances Supplies initialized distance values.
 * @param nearestSeeds Receives nearest-seed indices.
 * @param totalVoxels Identifies the number of map values.
 * @param sliceSize Limits resident buffer size.
 * @param shouldCancel Signals cancellation between buffers.
 * @return The first bulk-I/O error. Cancellation returns success.
 *
 */
template <typename T>
Result<> InitializeNearestSeeds(const AbstractDataStore<T>& distances, AbstractDataStore<int64>& nearestSeeds, usize totalVoxels, usize sliceSize, const std::atomic_bool& shouldCancel)
{
  std::vector<T> distanceBuffer(sliceSize);
  std::vector<int64> seedBuffer(sliceSize);
  for(usize offset = 0; offset < totalVoxels; offset += sliceSize)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(sliceSize, totalVoxels - offset);
    Result<> ioResult = distances.copyIntoBuffer(offset, nonstd::span<T>(distanceBuffer.data(), count));
    if(ioResult.invalid())
    {
      return ioResult;
    }
    for(usize index = 0; index < count; index++)
    {
      seedBuffer[index] = distanceBuffer[index] == static_cast<T>(0) ? static_cast<int64>(offset + index) : static_cast<int64>(-1);
    }
    ioResult = nearestSeeds.copyFromBuffer(offset, nonstd::span<const int64>(seedBuffer.data(), count));
    if(ioResult.invalid())
    {
      return ioResult;
    }
  }
  return {};
}

/**
 * @brief Runs labeled city-block sweeps without blocked cells.
 * @tparam T Specifies the distance element type.
 * @param distances Stores the map to transform.
 * @param nearestSeeds Stores nearest-seed indices.
 * @param dims Supplies image dimensions.
 * @param shouldCancel Signals cancellation between Z slices.
 * @return The first bulk-I/O error. Cancellation returns success.
 *
 */
template <typename T>
Result<> TransformLabeledDistanceWithoutObstacles(AbstractDataStore<T>& distances, AbstractDataStore<int64>& nearestSeeds, const SizeVec3& dims, const std::atomic_bool& shouldCancel)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  const usize sliceSize = dimX * dimY;

  std::vector<T> previousDistance(sliceSize);
  std::vector<T> currentDistance(sliceSize);
  std::vector<int64> previousSeed(sliceSize);
  std::vector<int64> currentSeed(sliceSize);

  for(usize zIndex = 0; zIndex < dimZ; zIndex++)
  {
    if(shouldCancel)
    {
      return {};
    }

    const usize sliceOffset = zIndex * sliceSize;
    Result<> ioResult = distances.copyIntoBuffer(sliceOffset, nonstd::span<T>(currentDistance.data(), sliceSize));
    if(ioResult.invalid())
    {
      return ioResult;
    }
    ioResult = nearestSeeds.copyIntoBuffer(sliceOffset, nonstd::span<int64>(currentSeed.data(), sliceSize));
    if(ioResult.invalid())
    {
      return ioResult;
    }
    for(usize yIndex = 0; yIndex < dimY; yIndex++)
    {
      const usize rowOffset = yIndex * dimX;
      for(usize xIndex = 0; xIndex < dimX; xIndex++)
      {
        const usize sliceIndex = rowOffset + xIndex;
        T& distance = currentDistance[sliceIndex];
        int64& seed = currentSeed[sliceIndex];
        if(xIndex > 0)
        {
          ConsiderLabeledDistance(currentDistance[sliceIndex - 1], currentSeed[sliceIndex - 1], distance, seed);
        }
        if(yIndex > 0)
        {
          ConsiderLabeledDistance(currentDistance[sliceIndex - dimX], currentSeed[sliceIndex - dimX], distance, seed);
        }
        if(zIndex > 0)
        {
          ConsiderLabeledDistance(previousDistance[sliceIndex], previousSeed[sliceIndex], distance, seed);
        }
      }
    }
    ioResult = distances.copyFromBuffer(sliceOffset, nonstd::span<const T>(currentDistance.data(), sliceSize));
    if(ioResult.invalid())
    {
      return ioResult;
    }
    ioResult = nearestSeeds.copyFromBuffer(sliceOffset, nonstd::span<const int64>(currentSeed.data(), sliceSize));
    if(ioResult.invalid())
    {
      return ioResult;
    }
    std::swap(previousDistance, currentDistance);
    std::swap(previousSeed, currentSeed);
  }

  std::vector<T> nextDistance(sliceSize);
  std::vector<int64> nextSeed(sliceSize);
  for(usize reverseZ = dimZ; reverseZ > 0; reverseZ--)
  {
    if(shouldCancel)
    {
      return {};
    }

    const usize zIndex = reverseZ - 1;
    const usize sliceOffset = zIndex * sliceSize;
    Result<> ioResult = distances.copyIntoBuffer(sliceOffset, nonstd::span<T>(currentDistance.data(), sliceSize));
    if(ioResult.invalid())
    {
      return ioResult;
    }
    ioResult = nearestSeeds.copyIntoBuffer(sliceOffset, nonstd::span<int64>(currentSeed.data(), sliceSize));
    if(ioResult.invalid())
    {
      return ioResult;
    }
    for(usize reverseY = dimY; reverseY > 0; reverseY--)
    {
      const usize yIndex = reverseY - 1;
      const usize rowOffset = yIndex * dimX;
      for(usize reverseX = dimX; reverseX > 0; reverseX--)
      {
        const usize xIndex = reverseX - 1;
        const usize sliceIndex = rowOffset + xIndex;
        T& distance = currentDistance[sliceIndex];
        int64& seed = currentSeed[sliceIndex];
        if(xIndex + 1 < dimX)
        {
          ConsiderLabeledDistance(currentDistance[sliceIndex + 1], currentSeed[sliceIndex + 1], distance, seed);
        }
        if(yIndex + 1 < dimY)
        {
          ConsiderLabeledDistance(currentDistance[sliceIndex + dimX], currentSeed[sliceIndex + dimX], distance, seed);
        }
        if(zIndex + 1 < dimZ)
        {
          ConsiderLabeledDistance(nextDistance[sliceIndex], nextSeed[sliceIndex], distance, seed);
        }
      }
    }
    ioResult = distances.copyFromBuffer(sliceOffset, nonstd::span<const T>(currentDistance.data(), sliceSize));
    if(ioResult.invalid())
    {
      return ioResult;
    }
    ioResult = nearestSeeds.copyFromBuffer(sliceOffset, nonstd::span<const int64>(currentSeed.data(), sliceSize));
    if(ioResult.invalid())
    {
      return ioResult;
    }
    std::swap(nextDistance, currentDistance);
    std::swap(nextSeed, currentSeed);
  }
  return {};
}

/**
 * @brief Propagates distances around non-positive Feature IDs.
 * @tparam T Specifies the distance element type.
 * @param distances Stores the map to transform.
 * @param nearestSeeds Stores optional nearest-seed indices.
 * @param featureIds Supplies blocked-cell markers.
 * @param dims Supplies image dimensions.
 * @param shouldCancel Signals cancellation between layers or slices.
 * @return The first bulk-I/O error. Cancellation returns success.
 *
 * Layer-synchronous propagation prevents distances from crossing blocked cells.
 */
template <typename T>
Result<> PropagateAroundBlockedCells(AbstractDataStore<T>& distances, AbstractDataStore<int64>* nearestSeeds, const Int32AbstractDataStore& featureIds, const SizeVec3& dims,
                                     const std::atomic_bool& shouldCancel)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  const usize sliceSize = dimX * dimY;

  std::vector<T> previousDistance(sliceSize);
  std::vector<T> currentDistance(sliceSize);
  std::vector<T> nextDistance(sliceSize);
  std::vector<int32> currentFeatureIds(sliceSize);
  std::vector<int64> previousSeed;
  std::vector<int64> currentSeed;
  std::vector<int64> nextSeed;
  if(nearestSeeds != nullptr)
  {
    previousSeed.resize(sliceSize);
    currentSeed.resize(sliceSize);
    nextSeed.resize(sliceSize);
  }

  usize propagationDistance = 1;
  while(true)
  {
    if(shouldCancel)
    {
      return {};
    }

    bool changed = false;
    Result<> ioResult = distances.copyIntoBuffer(0, nonstd::span<T>(currentDistance.data(), sliceSize));
    if(ioResult.invalid())
    {
      return ioResult;
    }
    if(dimZ > 1)
    {
      ioResult = distances.copyIntoBuffer(sliceSize, nonstd::span<T>(nextDistance.data(), sliceSize));
      if(ioResult.invalid())
      {
        return ioResult;
      }
    }
    if(nearestSeeds != nullptr)
    {
      ioResult = nearestSeeds->copyIntoBuffer(0, nonstd::span<int64>(currentSeed.data(), sliceSize));
      if(ioResult.invalid())
      {
        return ioResult;
      }
      if(dimZ > 1)
      {
        ioResult = nearestSeeds->copyIntoBuffer(sliceSize, nonstd::span<int64>(nextSeed.data(), sliceSize));
        if(ioResult.invalid())
        {
          return ioResult;
        }
      }
    }

    const T previousLayer = static_cast<T>(propagationDistance - 1);
    for(usize zIndex = 0; zIndex < dimZ; zIndex++)
    {
      if(shouldCancel)
      {
        return {};
      }

      const usize sliceOffset = zIndex * sliceSize;
      ioResult = featureIds.copyIntoBuffer(sliceOffset, nonstd::span<int32>(currentFeatureIds.data(), sliceSize));
      if(ioResult.invalid())
      {
        return ioResult;
      }
      for(usize yIndex = 0; yIndex < dimY; yIndex++)
      {
        const usize rowOffset = yIndex * dimX;
        for(usize xIndex = 0; xIndex < dimX; xIndex++)
        {
          const usize sliceIndex = rowOffset + xIndex;
          if(currentDistance[sliceIndex] >= static_cast<T>(0) || currentFeatureIds[sliceIndex] <= 0)
          {
            continue;
          }

          bool foundPredecessor = false;
          int64 selectedSeed = -1;
          const auto considerPredecessor = [&](T neighborDistance, int64 neighborSeed) {
            if(neighborDistance == previousLayer)
            {
              foundPredecessor = true;
              selectedSeed = neighborSeed;
            }
          };

          if(zIndex > 0)
          {
            considerPredecessor(previousDistance[sliceIndex], nearestSeeds != nullptr ? previousSeed[sliceIndex] : -1);
          }
          if(yIndex > 0)
          {
            considerPredecessor(currentDistance[sliceIndex - dimX], nearestSeeds != nullptr ? currentSeed[sliceIndex - dimX] : -1);
          }
          if(xIndex > 0)
          {
            considerPredecessor(currentDistance[sliceIndex - 1], nearestSeeds != nullptr ? currentSeed[sliceIndex - 1] : -1);
          }
          if(xIndex + 1 < dimX)
          {
            considerPredecessor(currentDistance[sliceIndex + 1], nearestSeeds != nullptr ? currentSeed[sliceIndex + 1] : -1);
          }
          if(yIndex + 1 < dimY)
          {
            considerPredecessor(currentDistance[sliceIndex + dimX], nearestSeeds != nullptr ? currentSeed[sliceIndex + dimX] : -1);
          }
          if(zIndex + 1 < dimZ)
          {
            considerPredecessor(nextDistance[sliceIndex], nearestSeeds != nullptr ? nextSeed[sliceIndex] : -1);
          }

          if(foundPredecessor)
          {
            currentDistance[sliceIndex] = static_cast<T>(propagationDistance);
            if(nearestSeeds != nullptr)
            {
              currentSeed[sliceIndex] = selectedSeed;
            }
            changed = true;
          }
        }
      }

      ioResult = distances.copyFromBuffer(sliceOffset, nonstd::span<const T>(currentDistance.data(), sliceSize));
      if(ioResult.invalid())
      {
        return ioResult;
      }
      if(nearestSeeds != nullptr)
      {
        ioResult = nearestSeeds->copyFromBuffer(sliceOffset, nonstd::span<const int64>(currentSeed.data(), sliceSize));
        if(ioResult.invalid())
        {
          return ioResult;
        }
      }

      std::swap(previousDistance, currentDistance);
      std::swap(currentDistance, nextDistance);
      if(nearestSeeds != nullptr)
      {
        std::swap(previousSeed, currentSeed);
        std::swap(currentSeed, nextSeed);
      }
      if(zIndex + 2 < dimZ)
      {
        ioResult = distances.copyIntoBuffer((zIndex + 2) * sliceSize, nonstd::span<T>(nextDistance.data(), sliceSize));
        if(ioResult.invalid())
        {
          return ioResult;
        }
        if(nearestSeeds != nullptr)
        {
          ioResult = nearestSeeds->copyIntoBuffer((zIndex + 2) * sliceSize, nonstd::span<int64>(nextSeed.data(), sliceSize));
          if(ioResult.invalid())
          {
            return ioResult;
          }
        }
      }
    }

    if(!changed)
    {
      return {};
    }
    propagationDistance++;
  }
}

/**
 * @brief Converts labeled city-block results to Euclidean distances.
 * @param distances Stores the float32 map to convert.
 * @param nearestSeeds Supplies nearest-seed indices.
 * @param dims Supplies image dimensions.
 * @param spacing Supplies image spacing.
 * @param shouldCancel Signals cancellation between Z slices.
 * @return The first bulk-I/O error. Cancellation returns success.
 *
 */
Result<> ConvertToEuclideanDistances(Float32AbstractDataStore& distances, const AbstractDataStore<int64>& nearestSeeds, const SizeVec3& dims, const FloatVec3& spacing,
                                     const std::atomic_bool& shouldCancel)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  const usize sliceSize = dimX * dimY;
  const float64 oneOverX = 1.0 / static_cast<float64>(dimX);
  const float64 oneOverSlice = 1.0 / static_cast<float64>(sliceSize);

  std::vector<float32> distanceBuffer(sliceSize);
  std::vector<int64> seedBuffer(sliceSize);
  for(usize zIndex = 0; zIndex < dimZ; zIndex++)
  {
    if(shouldCancel)
    {
      return {};
    }

    const usize sliceOffset = zIndex * sliceSize;
    Result<> ioResult = distances.copyIntoBuffer(sliceOffset, nonstd::span<float32>(distanceBuffer.data(), sliceSize));
    if(ioResult.invalid())
    {
      return ioResult;
    }
    ioResult = nearestSeeds.copyIntoBuffer(sliceOffset, nonstd::span<int64>(seedBuffer.data(), sliceSize));
    if(ioResult.invalid())
    {
      return ioResult;
    }
    for(usize yIndex = 0; yIndex < dimY; yIndex++)
    {
      const usize rowOffset = yIndex * dimX;
      for(usize xIndex = 0; xIndex < dimX; xIndex++)
      {
        const usize sliceIndex = rowOffset + xIndex;
        const int64 nearestSeed = seedBuffer[sliceIndex];
        if(nearestSeed < 0)
        {
          continue;
        }

        const float64 x1 = static_cast<float64>(xIndex) * spacing[0];
        const float64 y1 = static_cast<float64>(yIndex) * spacing[1];
        const float64 z1 = static_cast<float64>(zIndex) * spacing[2];
        const float64 x2 = spacing[0] * static_cast<float64>(nearestSeed % static_cast<int64>(dimX));
        const float64 y2 = spacing[1] * static_cast<float64>(static_cast<int64>(static_cast<float64>(nearestSeed) * oneOverX) % static_cast<int64>(dimY));
        const float64 z2 = spacing[2] * std::floor(static_cast<float64>(nearestSeed) * oneOverSlice);
        const float64 xDistance = x1 - x2;
        const float64 yDistance = y1 - y2;
        const float64 zDistance = z1 - z2;
        distanceBuffer[sliceIndex] = static_cast<float32>(std::sqrt(xDistance * xDistance + yDistance * yDistance + zDistance * zDistance));
      }
    }
    ioResult = distances.copyFromBuffer(sliceOffset, nonstd::span<const float32>(distanceBuffer.data(), sliceSize));
    if(ioResult.invalid())
    {
      return ioResult;
    }
  }
  return {};
}

/**
 * @brief Executes the selected scanline distance-map operations.
 * @tparam T Specifies the distance element type.
 * @param dataStructure Contains the ImageGeom, Feature IDs, and output maps.
 * @param inputValues Selects map types and required paths.
 * @param shouldCancel Signals cancellation at phase checkpoints.
 * @return The first bulk-I/O error, or an empty valid Result on completion and on cancellation.
 *
 * Float32 maps use a temporary nearest-seed DataStore selected by the active storage policy.
 * Cancellation returns at the next phase boundary and leaves completed map ranges in place.
 */
template <typename T>
Result<> ExecuteScanline(DataStructure& dataStructure, const ComputeEuclideanDistMapInputValues& inputValues, const std::atomic_bool& shouldCancel)
{
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(inputValues.InputImageGeometry);
  const SizeVec3 dims = imageGeom.getDimensions();
  const usize sliceSize = dims[0] * dims[1];
  const usize totalVoxels = sliceSize * dims[2];
  const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(inputValues.FeatureIdsArrayPath).getDataStoreRef();
  DistanceStoreArray<T> distanceStores = GetDistanceStores<T>(dataStructure, inputValues);

  bool hasBlockedCells = false;
  Result<> operationResult = InitializeSeeds(featureIds, distanceStores, dims, shouldCancel, hasBlockedCells);
  if(operationResult.invalid())
  {
    return operationResult;
  }
  if(shouldCancel)
  {
    return {};
  }

  for(usize mapIndex = 0; mapIndex < k_MapCount; mapIndex++)
  {
    auto* distanceStore = distanceStores[mapIndex];
    if(distanceStore == nullptr)
    {
      continue;
    }
    if(shouldCancel)
    {
      return {};
    }

    // Each phase helper returns an empty valid Result when it stops on cancellation, so the flag is
    // retested after every phase. This keeps a cancelled run from allocating scratch storage or
    // entering a later phase on partially filled data.
    if constexpr(std::is_same_v<T, int32>)
    {
      if(hasBlockedCells)
      {
        operationResult = PropagateAroundBlockedCells(*distanceStore, nullptr, featureIds, dims, shouldCancel);
        if(operationResult.invalid())
        {
          return operationResult;
        }
      }
      else
      {
        operationResult = TransformDistanceWithoutObstacles(*distanceStore, dims, shouldCancel);
        if(operationResult.invalid())
        {
          return operationResult;
        }
      }
      if(shouldCancel)
      {
        return {};
      }
    }
    else
    {
      const DataPath scratchPath = inputValues.FeatureIdsArrayPath.getParent().createChildPath("__ComputeEuclideanDistMapNearestSeedScratch");
      auto nearestSeeds = DataStoreUtilities::CreateDataStore<int64>(dataStructure, scratchPath, featureIds.getTupleShape(), {1});
      operationResult = InitializeNearestSeeds(*distanceStore, *nearestSeeds, totalVoxels, sliceSize, shouldCancel);
      if(operationResult.invalid())
      {
        return operationResult;
      }
      if(shouldCancel)
      {
        return {};
      }

      if(hasBlockedCells)
      {
        operationResult = PropagateAroundBlockedCells(*distanceStore, nearestSeeds.get(), featureIds, dims, shouldCancel);
        if(operationResult.invalid())
        {
          return operationResult;
        }
      }
      else
      {
        operationResult = TransformLabeledDistanceWithoutObstacles(*distanceStore, *nearestSeeds, dims, shouldCancel);
        if(operationResult.invalid())
        {
          return operationResult;
        }
      }
      if(shouldCancel)
      {
        return {};
      }

      operationResult = ConvertToEuclideanDistances(*distanceStore, *nearestSeeds, dims, imageGeom.getSpacing(), shouldCancel);
      if(operationResult.invalid())
      {
        return operationResult;
      }
      if(shouldCancel)
      {
        return {};
      }
    }
  }
  return {};
}
} // namespace

ComputeEuclideanDistMapScanline::ComputeEuclideanDistMapScanline(DataStructure& dataStructure, const IFilter::MessageHandler&, const std::atomic_bool& shouldCancel,
                                                                 const ComputeEuclideanDistMapInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
{
}

ComputeEuclideanDistMapScanline::~ComputeEuclideanDistMapScanline() noexcept = default;

Result<> ComputeEuclideanDistMapScanline::operator()()
{
  if(m_InputValues->CalcManhattanDist)
  {
    return ExecuteScanline<int32>(m_DataStructure, *m_InputValues, m_ShouldCancel);
  }
  return ExecuteScanline<float32>(m_DataStructure, *m_InputValues, m_ShouldCancel);
}
