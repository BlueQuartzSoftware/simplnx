#include <memory>
#include <vector>

#include <catch2/catch.hpp>

#include "complex/Common/Types.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

using namespace complex;

TEST_CASE("DataArrayCreation")
{
  complex::DataStructure ds;

  using DataStoreType = complex::DataStore<int32_t>;
  DataStoreType data_array = DataStoreType(complex::IDataStore::ShapeType{0}, complex::IDataStore::ShapeType{2}, 0);
  size_t numTuples = data_array.getNumberOfTuples();
  REQUIRE(numTuples == 0);
}

TEST_CASE("complex::DataArray Copy TupleTest", "[complex][DataArray]")
{

  const std::string k_DataArrayName("DataArray");
  const DataPath k_DataPath({k_DataArrayName});
  const usize k_NumTuples = 5;
  const usize k_NumComponents = 3;

  DataStructure dataStructure;
  IDataStore::ShapeType tupleShape{k_NumTuples};
  IDataStore::ShapeType componentShape{k_NumComponents};
  Result<> result = CreateArray<int32>(dataStructure, tupleShape, componentShape, k_DataPath, IDataAction::Mode::Execute);
  REQUIRE(result.valid() == true);

  auto& dataArray = dataStructure.getDataRefAs<DataArray<int32>>(k_DataPath);

  for(usize i = 0; i < k_NumTuples; i++)
  {
    dataArray.initializeTuple(i, static_cast<int32>(i));
  }

  for(usize tupleIndex = 0; tupleIndex < k_NumTuples; tupleIndex++)
  {
    for(usize componentIndex = 0; componentIndex < k_NumComponents; componentIndex++)
    {
      REQUIRE(dataArray[tupleIndex * 3 + componentIndex] == static_cast<int32>(tupleIndex));
    }
  }

  dataArray.copyTuple(4, 0);
  REQUIRE(dataArray[0] == 4);
  REQUIRE(dataArray[1] == 4);
  REQUIRE(dataArray[2] == 4);

  dataArray.copyTuple(1, 4);
  REQUIRE(dataArray[12] == 1);
  REQUIRE(dataArray[13] == 1);
  REQUIRE(dataArray[14] == 1);
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

  dataStore.setComponent(2, 2, 99);
  REQUIRE(dataStore[8] == 99);
  REQUIRE(dataStore.getComponentValue(2, 2) == 99);
}
