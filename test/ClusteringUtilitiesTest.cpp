#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IArray.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <vector>

using namespace nx::core;

TEST_CASE("ClusterUtilities::RandomizeFeatureIds preserves feature tuples through long cycles", "[simplnx][ClusteringUtilities]")
{
  bool sawLongCycle = false;
  // Check several sizes so the test does not depend on the random distribution of one standard library.
  for(usize totalFeatures = 4; totalFeatures <= 32; ++totalFeatures)
  {
    CAPTURE(totalFeatures);
    DataStore<int32> ids(totalFeatures, 0);
    for(usize oldId = 0; oldId < totalFeatures; ++oldId)
    {
      ids[oldId] = static_cast<int32>(oldId);
    }

    DataStructure dataStructure;
    auto* valuesPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "Feature Values", {totalFeatures + 2}, {2});
    REQUIRE(valuesPtr != nullptr);
    for(usize index = 0; index < valuesPtr->getSize(); ++index)
    {
      (*valuesPtr)[index] = static_cast<int32>(100 + index);
    }

    std::vector<IArray*> featureArrays{valuesPtr};
    const auto result = cluster_utilities::RandomizeFeatureIds(ids, totalFeatures, featureArrays);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(ids[0] == 0);
    std::vector<bool> seen(totalFeatures, false);
    for(usize oldId = 0; oldId < totalFeatures; ++oldId)
    {
      REQUIRE(ids[oldId] >= 0);
      const auto newId = static_cast<usize>(ids[oldId]);
      REQUIRE(newId < totalFeatures);
      REQUIRE_FALSE(seen[newId]);
      seen[newId] = true;
      sawLongCycle = sawLongCycle || (newId != oldId && ids[newId] != static_cast<int32>(oldId));
      CAPTURE(oldId, newId);
      const auto expectedFirstComponent = static_cast<int32>(100 + (2 * oldId));
      const usize newOffset = 2 * newId;
      CHECK((*valuesPtr)[newOffset] == expectedFirstComponent);
      CHECK((*valuesPtr)[newOffset + 1] == expectedFirstComponent + 1);
    }
    for(usize index = 2 * totalFeatures; index < valuesPtr->getSize(); ++index)
    {
      CHECK((*valuesPtr)[index] == static_cast<int32>(100 + index));
    }
  }
  REQUIRE(sawLongCycle);
}
