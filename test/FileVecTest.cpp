#include <catch2/catch.hpp>

#include "complex/Core/Application.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataStructure.hpp"

#include "FileVec/Zarr/Group.hpp"

namespace complex
{
DataStructure createDataStructure()
{
  DataStructure dataStructure;
  DataGroup::Create(dataStructure, "DataGroup");

  IDataStore::ShapeType tupleShape{1};
  IDataStore::ShapeType componentShape{1};
  auto* intArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "IntArray", tupleShape, componentShape);
  intArray->getDataStoreRef().setValue(0, 5);

  return dataStructure;
}

TEST_CASE("Round-trip test", "Out-of-Core")
{
  Application app;
  DataStructure dataStructure = createDataStructure();

  auto group = FileVec::Zarr::Group::Create();
  REQUIRE(group != nullptr);

  auto dataStructureGroup = group->createOrFindGroup(Constants::k_DataStructureTag);
  Zarr::ErrorType err = dataStructure.writeZarr(*group.get());
  REQUIRE(err == 0);

  DataStructure readStructure = DataStructure::readFromZarr(*group.get(), err);
  REQUIRE(dataStructure.getNextId() == readStructure.getNextId());

  {
    auto* readGroup = readStructure.getDataAs<DataGroup>(DataPath({"DataGroup"}));
    REQUIRE(readGroup != nullptr);
  }

  {
    Int32Array* readIntArray = readStructure.getDataAs<Int32Array>(DataPath({"IntArray"}));
    REQUIRE(readIntArray != nullptr);
    REQUIRE(readIntArray->getSize() == 1);
    REQUIRE(readIntArray->getDataStoreRef().getValue(0) == 5);
  }
}
} // namespace complex
