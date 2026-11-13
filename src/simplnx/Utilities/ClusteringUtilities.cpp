#include "ClusteringUtilities.hpp"

#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <nonstd/span.hpp>

#include <random>

using namespace nx::core;

namespace
{
/**
 * @brief Creates a deterministic feature-ID permutation with feature zero fixed.
 * @param totalFeatures Total feature count, including feature zero.
 * @return Mapping from each old feature ID to a permuted ID.
 * @pre totalFeatures is nonzero and totalFeatures - 1 fits int32.
 */
std::vector<int32> CreateRandomizedIdsList(usize totalFeatures)
{
  const usize rangeMin = 1;
  const usize rangeMax = totalFeatures - 1;
  auto gen = std::mt19937_64(std::mt19937_64::default_seed);
  std::uniform_real_distribution<float64> dist(0, 1);

  std::vector<int32> randomIds(totalFeatures);
  std::iota(randomIds.begin(), randomIds.end(), 0);

  // Start at one and reject index zero so the background feature remains fixed.
  for(usize i = 1; i < totalFeatures; i++)
  {
    auto r = static_cast<usize>(std::floor(dist(gen) * static_cast<float64>(rangeMax)));
    if(r < rangeMin)
    {
      continue;
    }

    std::swap(randomIds[i], randomIds[r]);
  }

  return randomIds;
}
} // namespace

namespace nx::core::ClusterUtilities
{
Result<> RandomizeFeatureIds(Int32AbstractDataStore& featureIdsStore, usize totalFeatures)
{
  std::vector<int32> randomIds = CreateRandomizedIdsList(totalFeatures);

  // Fixed-size bulk transfers avoid one disk-backed access for each cell.
  usize totalPoints = featureIdsStore.getSize();
  constexpr usize k_ChunkSize = 65536;
  std::vector<int32> chunkBuf(k_ChunkSize);
  for(usize offset = 0; offset < totalPoints; offset += k_ChunkSize)
  {
    usize count = std::min(k_ChunkSize, totalPoints - offset);
    auto readResult = featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(chunkBuf.data(), count));
    if(readResult.invalid())
    {
      return readResult;
    }
    for(usize i = 0; i < count; i++)
    {
      chunkBuf[i] = randomIds[chunkBuf[i]];
    }
    auto writeResult = featureIdsStore.copyFromBuffer(offset, nonstd::span<const int32>(chunkBuf.data(), count));
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }
  return {};
}

Result<> RandomizeFeatureIds(Int32AbstractDataStore& featureIdsStore, usize totalFeatures, std::vector<IArray*>& featureIArrays)
{
  std::vector<int32> randomIds = CreateRandomizedIdsList(totalFeatures);

  // Fixed-size bulk transfers avoid one disk-backed access for each cell.
  usize totalPoints = featureIdsStore.getSize();
  constexpr usize k_ChunkSize = 65536;
  std::vector<int32> chunkBuf(k_ChunkSize);
  for(usize offset = 0; offset < totalPoints; offset += k_ChunkSize)
  {
    usize count = std::min(k_ChunkSize, totalPoints - offset);
    auto readResult = featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(chunkBuf.data(), count));
    if(readResult.invalid())
    {
      return readResult;
    }
    for(usize i = 0; i < count; i++)
    {
      chunkBuf[i] = randomIds[chunkBuf[i]];
    }
    auto writeResult = featureIdsStore.copyFromBuffer(offset, nonstd::span<const int32>(chunkBuf.data(), count));
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }

  if(!featureIArrays.empty())
  {
    // Complete each cycle so every source tuple reaches its mapped feature ID.
    // Visitation scales with feature count and prevents processing a cycle twice.
    std::vector<bool> visited(randomIds.size(), false);
    for(usize i = 1; i < randomIds.size(); i++)
    {
      if(visited[i])
      {
        continue;
      }

      visited[i] = true;
      usize next = static_cast<usize>(randomIds[i]);
      while(next != i)
      {
        for(auto* iArrayPtr : featureIArrays)
        {
          iArrayPtr->swapTuples(i, next);
        }
        visited[next] = true;
        next = static_cast<usize>(randomIds[next]);
      }
    }
  }
  return {};
}
} // namespace nx::core::ClusterUtilities
