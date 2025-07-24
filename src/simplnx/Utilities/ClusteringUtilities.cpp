#include "ClusteringUtilities.hpp"

#include "simplnx/Utilities/DataStoreUtilities.hpp"

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

  // Now adjust all the Grain ID values for each Voxel
  // instead of taking total points as an input just extract the size, so we don't walk off
  usize totalPoints = featureIdsStore.getSize();
  for(int64 i = 0; i < totalPoints; ++i)
  {
    featureIdsStore[i] = randomIds[featureIdsStore[i]];
  }
}

void RandomizeFeatureIds(Int32AbstractDataStore& featureIdsStore, usize totalFeatures, std::vector<IArray*>& featureIArrays)
{
  std::vector<int32> randomIds = CreateRandomizedIdsList(totalFeatures);

  // Now adjust all the Grain ID values for each Voxel
  // instead of taking total points as an input just extract the size, so we don't walk off
  usize totalPoints = featureIdsStore.getSize();
  for(int64 i = 0; i < totalPoints; ++i)
  {
    featureIdsStore[i] = randomIds[featureIdsStore[i]];
  }

  if(!featureIArrays.empty())
  {
    for(usize i = 0; i < totalFeatures; i++)
    {
      for(auto* iArray : featureIArrays)
      {
        iArray->swapTuples(i, randomIds[i]);
      }
    }
  }
}
} // namespace nx::core::ClusterUtilities
