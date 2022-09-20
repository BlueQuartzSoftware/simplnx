#include <catch2/catch.hpp>

#include "complex/Core/Application.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataStructure.hpp"

#include "FileVec/Zarr/Group.hpp"

#if 0
namespace complex
{
DataStructure createDataStructure()
{
  DataStructure dataStructure;
  auto* group = DataGroup::Create(dataStructure, k_GroupName);

  IDataStore::ShapeType tupleShape{1};
  IDataStore::ShapeType componentShape{1};
  auto* intArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_IntArrayName, tupleShape, componentShape, group->getId());
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

  group->flush();

  DataStructure readStructure = DataStructure::readFromZarr(*group.get(), err);
  REQUIRE(dataStructure.getNextId() == readStructure.getNextId());

  {
    auto* readGroup = readStructure.getDataAs<DataGroup>(DataPath({k_GroupName}));
    REQUIRE(readGroup != nullptr);
  }

  {
    Int32Array* readIntArray = readStructure.getDataAs<Int32Array>(DataPath({k_GroupName, k_IntArrayName}));
    REQUIRE(readIntArray != nullptr);
    REQUIRE(readIntArray->getSize() == 1);
    REQUIRE(readIntArray->getDataStoreRef().getValue(0) == 5);
  }
}
} // namespace complex
#endif
