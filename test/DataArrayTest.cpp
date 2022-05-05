#include <memory>
#include <vector>

#include <catch2/catch.hpp>

#include "complex/Common/Types.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"

using namespace complex;

TEST_CASE("DataArrayCreation")
{
  complex::DataStructure ds;

  using DataStoreType = complex::DataStore<int32_t>;
  DataStoreType data_array = DataStoreType(complex::IDataStore::ShapeType{0}, complex::IDataStore::ShapeType{2}, 0);
  size_t numTuples = data_array.getNumberOfTuples();
  REQUIRE(numTuples == 0);
}

TEST_CASE("DataStore Test")
{
  IDataStore::ShapeType tupleShape{5};
  IDataStore::ShapeType componentShape{3};
  DataStore<int32> dataStore(tupleShape, componentShape, 5);

  REQUIRE(dataStore.getSize() == 15);

  for(uint64_t i = 0; i < dataStore.getSize(); i++)
  {
    REQUIRE(dataStore[i] == 5);
  }

  int32 newArrayValues[] = {6, 7, 8};
  dataStore.setTuple(0, newArrayValues);
  for(uint64 i = 0; i < 3; i++)
  {
    REQUIRE(dataStore[i] == newArrayValues[i]);
    REQUIRE(dataStore.getComponentValue(0, i) == newArrayValues[i]);
  }

  std::vector<int32> newValues{1, 2, 3};
  dataStore.setTuple(1, newValues);
  int64 offset = dataStore.getNumberOfComponents();
  for(uint64 i = 0; i < newValues.size(); i++)
  {
    REQUIRE(dataStore[offset + i] == newValues[i]);
    REQUIRE(dataStore.getComponentValue(1, i) == newValues[i]);
  }
  
  dataStore.setComponent(2, 3, 99);
  REQUIRE(dataStore[9] == 99);
  REQUIRE(dataStore.getComponentValue(2, 3) == 99);
}
