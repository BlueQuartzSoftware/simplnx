#include <catch2/catch.hpp>

#include "complex/Core/Application.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataStructure.hpp"

#include "FileVec/collection/Group.hpp"

namespace complex
{
DataStructure createDataStructure()
{
  DataStructure dataStructure;
  DataGroup::Create(dataStructure, "DataGroup");

  IDataStore::ShapeType tupleShape{1};
  IDataStore::ShapeType componentShape{1};
  Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "IntArray", tupleShape, componentShape);

  return dataStructure;
}

TEST_CASE("Round-trip test", "Out-of-Core")
{
  Application app;
  DataStructure dataStructure = createDataStructure();
  auto group = FileVec::Group::Create();
  auto dataStructureGroup = group->createOrFindGroup(Constants::k_DataStructureTag);
  Zarr::ErrorType err = dataStructure.writeZarr(*group.get());
  REQUIRE(err == 0);

  DataStructure readStructure = DataStructure::readFromZarr(*group.get(), err);
  REQUIRE(dataStructure.getNextId() == readStructure.getNextId());
}
} // namespace complex
