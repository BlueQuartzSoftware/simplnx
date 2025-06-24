#include "ClusteringUtilities.hpp"

#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <random>

namespace nx::core::ClusterUtilities
{
void RandomizeFeatureIds(Int32AbstractDataStore& featureIdsStore, usize totalFeatures)
{
  const usize rangeMin = 1;
  const usize rangeMax = totalFeatures - 1;
  auto gen = std::mt19937_64(std::mt19937_64::default_seed);
  std::uniform_real_distribution<float64> dist(0, 1);

  IDataStore::ShapeType tupleShape{totalFeatures};
  IDataStore::ShapeType componentShape{1};
  std::shared_ptr<AbstractDataStore<int32>> randomIdsPtr = nx::core::DataStoreUtilities::CreateDataStore<int32>(tupleShape, componentShape, IDataAction::Mode::Execute);
  Int32AbstractDataStore& randomIds = *randomIdsPtr.get();
  std::iota(randomIds.begin(), randomIds.end(), 0);

  //--- Shuffle elements by randomly exchanging each with one other.
  for(usize i = 1; i < totalFeatures; i++)
  {
    auto r = static_cast<usize>(std::floor(dist(gen) * static_cast<float64>(rangeMax))); // Random remaining position.
    if(r < rangeMin)
    {
      continue;
    }

    int32 randId_i = randomIds[i];
    randomIds[i] = randomIds[r];
    randomIds[r] = randId_i;
  }

  // Now adjust all the Grain ID values for each Voxel
  // instead of taking total points as an input just extract the size, so we don't walk off
  usize totalPoints = featureIdsStore.getSize();
  for(int64 i = 0; i < totalPoints; ++i)
  {
    featureIdsStore[i] = randomIds[featureIdsStore[i]];
  }
}
} // namespace nx::core::ClusterUtilities
