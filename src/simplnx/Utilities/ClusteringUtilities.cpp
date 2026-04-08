#include "ClusteringUtilities.hpp"

#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <nonstd/span.hpp>

#include <random>

using namespace nx::core;

namespace
{
std::vector<int32> CreateRandomizedIdsList(usize totalFeatures)
{
  const usize rangeMin = 1;
  const usize rangeMax = totalFeatures - 1;
  auto gen = std::mt19937_64(std::mt19937_64::default_seed);
  std::uniform_real_distribution<float64> dist(0, 1);

  std::vector<int32> randomIds(totalFeatures);
  std::iota(randomIds.begin(), randomIds.end(), 0);

  //--- Shuffle elements by randomly exchanging each with one other.
  for(usize i = 1; i < totalFeatures; i++)
  {
    auto r = static_cast<usize>(std::floor(dist(gen) * static_cast<float64>(rangeMax))); // Random remaining position.
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
void RandomizeFeatureIds(Int32AbstractDataStore& featureIdsStore, usize totalFeatures)
{
  std::vector<int32> randomIds = CreateRandomizedIdsList(totalFeatures);

  // Chunked bulk I/O for OOC efficiency
  usize totalPoints = featureIdsStore.getSize();
  constexpr usize k_ChunkSize = 65536;
  std::vector<int32> chunkBuf(k_ChunkSize);
  for(usize offset = 0; offset < totalPoints; offset += k_ChunkSize)
  {
    usize count = std::min(k_ChunkSize, totalPoints - offset);
    featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(chunkBuf.data(), count));
    for(usize i = 0; i < count; i++)
    {
      chunkBuf[i] = randomIds[chunkBuf[i]];
    }
    featureIdsStore.copyFromBuffer(offset, nonstd::span<const int32>(chunkBuf.data(), count));
  }
}

void RandomizeFeatureIds(Int32AbstractDataStore& featureIdsStore, usize totalFeatures, std::vector<IArray*>& featureIArrays)
{
  std::vector<int32> randomIds = CreateRandomizedIdsList(totalFeatures);

  // Chunked bulk I/O for OOC efficiency
  usize totalPoints = featureIdsStore.getSize();
  constexpr usize k_ChunkSize = 65536;
  std::vector<int32> chunkBuf(k_ChunkSize);
  for(usize offset = 0; offset < totalPoints; offset += k_ChunkSize)
  {
    usize count = std::min(k_ChunkSize, totalPoints - offset);
    featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(chunkBuf.data(), count));
    for(usize i = 0; i < count; i++)
    {
      chunkBuf[i] = randomIds[chunkBuf[i]];
    }
    featureIdsStore.copyFromBuffer(offset, nonstd::span<const int32>(chunkBuf.data(), count));
  }

  if(!featureIArrays.empty())
  {
    // Visitation pattern for feature-level tuple swaps (small, no OOC concern)
    std::vector<bool> visited(randomIds.size(), false);
    for(usize i = 0; i < randomIds.size(); i++)
    {
      if(visited[i])
      {
        continue;
      }

      visited[i] = true;
      visited[randomIds[i]] = true;

      for(auto* iArray : featureIArrays)
      {
        iArray->swapTuples(i, randomIds[i]);
      }
    }
  }
}
} // namespace nx::core::ClusterUtilities
