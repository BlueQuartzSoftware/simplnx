#include <memory>
#include <vector>

#include <catch2/catch.hpp>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"

TEST_CASE("DataArrayCreation")
{
  complex::DataStructure ds;

  using DataStoreType = complex::DataStore<int32_t>;
  DataStoreType data_array = DataStoreType({0}, {2});
  size_t numTuples = data_array.getNumberOfTuples();
  REQUIRE(numTuples == 0);
}
