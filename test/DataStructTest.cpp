#include <memory>
#include <vector>

#include <catch2/catch.hpp>

#include "DataStructObserver.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/ScalarData.hpp"

/**
 * @brief Test creation and removal of items in a tree-style structure. No node has more than one parent.
 */
TEST_CASE("DataGroupTree")
{
  // Create structuure
  DataStructure dataStr;
  auto group = DataGroup::Create(dataStr, "Foo");
  auto child = DataGroup::Create(dataStr, "bar", group->getId());
  auto grandchild = DataGroup::Create(dataStr, "bazz", child->getId());

  auto groupId = group->getId();
  auto childId = child->getId();
  auto grandchildId = grandchild->getId();

  SECTION("find data")
  {
    REQUIRE(dataStr.getData(groupId) != nullptr);
    REQUIRE(dataStr.getData(childId) != nullptr);
    REQUIRE(dataStr.getData(grandchildId) != nullptr);
  }
  SECTION("remove mid-level group")
  {
    group->remove(child->getName());
    REQUIRE(dataStr.getData(childId) == nullptr);
    REQUIRE(dataStr.getData(grandchildId) == nullptr);
  }
  SECTION("remove top-level group")
  {
    dataStr.removeData(group->getId());
    REQUIRE(dataStr.getData(groupId) == nullptr);
  }
}

/**
 * @brief Test creation and removal of DataObjects in a graph-style DataStructure where multiple parents are allowed.
 */
TEST_CASE("DataGroupGraph")
{
  // Create DataStructure
  DataStructure dataStr;
  auto group = DataGroup::Create(dataStr, "Foo");
  auto child1 = DataGroup::Create(dataStr, "Bar1", group->getId());
  auto child2 = DataGroup::Create(dataStr, "Bar2", group->getId());
  auto grandchild = DataGroup::Create(dataStr, "Bazz", child1->getId());

  // Get IDs
  auto groupId = group->getId();
  auto child1Id = child1->getId();
  auto child2Id = child2->getId();
  auto grandchildId = grandchild->getId();

  // Multi-parent connections
  dataStr.setAdditionalParent(grandchildId, child2Id);

  SECTION("find data")
  {
    REQUIRE(dataStr.getData(groupId) != nullptr);
    REQUIRE(dataStr.getData(child1Id) != nullptr);
    REQUIRE(dataStr.getData(child2Id) != nullptr);
    REQUIRE(dataStr.getData(grandchildId) != nullptr);
  }
  SECTION("remove mid-level group")
  {
    group->remove(child1->getName());
    REQUIRE(dataStr.getData(child1Id) == nullptr);
    REQUIRE(dataStr.getData(grandchildId) != nullptr);
  }
  SECTION("remove top-level group")
  {
    dataStr.removeData(group->getId());
    REQUIRE(dataStr.getData(groupId) == nullptr);
    REQUIRE(dataStr.getData(grandchildId) == nullptr);
  }
}

/**
 * @brief Test DataPath usage
 */
TEST_CASE("DataPathTest")
{
  // Create DataStructure
  DataStructure dataStr;
  auto group = DataGroup::Create(dataStr, "Foo");
  auto child1 = DataGroup::Create(dataStr, "Bar1", group->getId());
  auto child2 = DataGroup::Create(dataStr, "Bar2", group->getId());
  auto grandchild = DataGroup::Create(dataStr, "Bazz", child1->getId());

  auto child2Id = child2->getId();
  auto grandchildId = grandchild->getId();
  SECTION("add additional parent")
  {
    REQUIRE(dataStr.setAdditionalParent(grandchildId, child2Id));
  }

  const DataPath gcPath1({"Foo", "Bar1", "Bazz"});
  const DataPath gcPath2({"Foo", "Bar2", "Bazz"});

  REQUIRE(dataStr.getData(gcPath1) != nullptr);
  REQUIRE(dataStr.getData(gcPath2) != nullptr);

  const DataPath c1Path({"Foo", "Bar1"});
  REQUIRE(dataStr.removeData(c1Path));
  REQUIRE(dataStr.getData(gcPath1) == nullptr);
  REQUIRE(dataStr.getData(DataPath({"Foo", "Bar2"})) != nullptr);
  auto gc2 = dataStr.getData(gcPath2);
  REQUIRE(dataStr.getData(gcPath2) != nullptr);

  dataStr.removeData(child2Id);
  REQUIRE(dataStr.getData(gcPath2) == nullptr);

  DataPath fromStringTest = DataPath::FromString("/Group/Data").value();
  REQUIRE(fromStringTest.getLength() == 2);
  fromStringTest = DataPath::FromString("/Group/").value();
  REQUIRE(fromStringTest.getLength() == 1);

  auto empty1 = DataPath::FromString("/");
  REQUIRE(empty1.has_value());
  REQUIRE(empty1.value().empty());
  auto empty2 = DataPath::FromString("");
  REQUIRE(empty2.has_value());
  REQUIRE(empty2.value().empty());
}

TEST_CASE("LinkedPathTest")
{
  DataStructure dataStr;
  auto group = DataGroup::Create(dataStr, "Foo");
  auto child1 = DataGroup::Create(dataStr, "Bar1", group->getId());
  auto child2 = DataGroup::Create(dataStr, "Bar2", group->getId());
  auto grandchild = DataGroup::Create(dataStr, "Bazz", child1->getId());

  auto child2Id = child2->getId();
  auto grandchildId = grandchild->getId();

  REQUIRE(dataStr.setAdditionalParent(grandchildId, child2Id));

  // DataPath
  const DataPath grandPath({"Foo", "Bar1", "Bazz"});
  const LinkedPath linkedPath = dataStr.getLinkedPath(grandPath);

  REQUIRE(linkedPath.isValid());
  REQUIRE(grandchild == linkedPath.getData());

  REQUIRE(child1->rename("Bar1.3"));
  REQUIRE(dataStr.getData(grandPath) == nullptr);
  REQUIRE(linkedPath.getData() != nullptr);
  REQUIRE(linkedPath.isValid());

  REQUIRE(dataStr.removeData(linkedPath.getIdAt(1)));
  REQUIRE(!linkedPath.isValid());
}

/**
 * @brief Tests IDataStructureListener usage
 */
TEST_CASE("DataStructureListenerTest")
{
  DataStructure dataStr;
  DataStructObserver dsListener(dataStr);

  // Adds items to the DataStructure. Each addition should trigger dsListener.
  auto group = DataGroup::Create(dataStr, "Foo");
  auto child1 = DataGroup::Create(dataStr, "Bar1", group->getId());
  auto child2 = DataGroup::Create(dataStr, "Bar2", group->getId());
  auto grandchild = DataGroup::Create(dataStr, "Bazz", child1->getId());

  auto groupId = group->getId();
  auto child2Id = child2->getId();
  auto grandchildId = grandchild->getId();

  REQUIRE(dsListener.getDataAddedCount() == 4);

  REQUIRE(dataStr.setAdditionalParent(grandchildId, child2Id));
  REQUIRE(dsListener.getDataReparentedCount() == 1);

  dataStr.removeData(child2Id);
  REQUIRE(dsListener.getDataRemovedCount() == 1);
  dataStr.removeData(groupId);
  REQUIRE(dsListener.getDataRemovedCount() == 4);
}

TEST_CASE("DataStructureCopyTest")
{
  DataStructure dataStr;
  auto group = DataGroup::Create(dataStr, "Foo");
  auto child1 = DataGroup::Create(dataStr, "Bar1", group->getId());
  auto child2 = DataGroup::Create(dataStr, "Bar2", group->getId());
  auto grandchild = DataGroup::Create(dataStr, "Bazz", child1->getId());

  auto groupId = group->getId();
  auto child1Id = child1->getId();
  auto child2Id = child2->getId();
  auto grandchildId = grandchild->getId();

  REQUIRE(dataStr.setAdditionalParent(grandchildId, child2Id));

  // Copy DataStructure
  DataStructure dataStrCopy(dataStr);

  REQUIRE(dataStrCopy.getData(groupId));
  REQUIRE(dataStrCopy.getData(child1Id));
  REQUIRE(dataStrCopy.getData(child2Id));
  REQUIRE(dataStrCopy.getData(grandchildId));

  DataObject* data = dataStr.getData(groupId);
  DataObject* dataCopy = dataStrCopy.getData(groupId);
  REQUIRE(dataStrCopy.getData(groupId) != dataStr.getData(groupId));
  REQUIRE(dataStrCopy.getData(child1Id) != dataStr.getData(child1Id));
  REQUIRE(dataStrCopy.getData(child2Id) != dataStr.getData(child2Id));
  REQUIRE(dataStrCopy.getData(grandchildId) != dataStr.getData(grandchildId));

  // Create new group
  auto newGroup = DataGroup::Create(dataStr, "New Group", child2Id);
  auto newId = newGroup->getId();
  REQUIRE(dataStr.getData(newId));
  REQUIRE(!dataStrCopy.getData(newId));

  auto newGroup2 = DataGroup::Create(dataStrCopy, "New Group (2)", child2Id);
  auto newId2 = newGroup2->getId();
  REQUIRE(dataStr.getData(newId2) != dataStrCopy.getData(newId2));
  REQUIRE(dataStrCopy.getData(newId2));
}

TEST_CASE("DataStoreTest")
{
  const size_t numComponents = 3;
  const size_t numTuples = 10;
  DataStore<int32_t> store({numTuples}, {numComponents});

  REQUIRE(store.getNumberOfTuples() == numTuples);
  REQUIRE(store.getNumberOfComponents() == numComponents);
  REQUIRE(store.getSize() == (numComponents * numTuples));

  // subscript operator
  {
    for(usize i = 0; i < store.getSize(); i++)
    {
      store[i] = i + 1;
    }
    int32 x = 1;
    for(usize i = 0; i < store.getSize(); i++)
    {
      REQUIRE(store[i] == x++);
    }
  }
  // get / set values
  {
    const usize index = 5;
    const usize value = 25;
    store.setValue(index, value);
    REQUIRE(store.getValue(index) == value);
  }
  // iterators
  {
    const int32 initVal = -30;
    int32 x = initVal;
    for(auto& value : store)
    {
      value = x++;
    }
    x = initVal;
    for(const auto& value : store)
    {
      REQUIRE(value == x++);
    }
  }
}

TEST_CASE("DataArrayTest")
{
  DataStructure dataStr;

  auto store = std::make_shared<DataStore<int32>>(std::vector<usize>{2}, std::vector<usize>{2});
  REQUIRE(store != nullptr);
  auto dataArr = DataArray<int32>::Create(dataStr, "array", store);
  REQUIRE(dataArr != nullptr);

  SECTION("test size")
  {
    REQUIRE(dataArr->getSize() == store->getSize());
    REQUIRE(dataArr->getNumberOfComponents() == store->getNumberOfComponents());
    REQUIRE(dataArr->getNumberOfTuples() == store->getNumberOfTuples());
  }

  SECTION("test reading / writing to memory")
  {
    SECTION("test subscript operators")
    {
      for(usize i = 0; i < dataArr->getSize(); i++)
      {
        (*dataArr)[i] = i + 1;
      }
      int32 x = 1;
      for(usize i = 0; i < dataArr->getSize(); i++)
      {
        REQUIRE((*dataArr)[i] == x++);
      }
    }
    SECTION("test iterators")
    {
      const int32 initVal = -30;
      int32 x = initVal;
      for(auto& value : *dataArr)
      {
        value = x++;
      }
      x = initVal;
      for(const auto& value : *dataArr)
      {
        REQUIRE(value == x++);
      }
    }
  }
}

TEST_CASE("ScalarDataTest")
{
  DataStructure dataStr;
  const int32 value = 6;
  auto scalar = ScalarData<int32>::Create(dataStr, "scalar", value);

  // get value
  REQUIRE(value == scalar->getValue());
  REQUIRE((*scalar) == value);
  REQUIRE((*scalar) != (value + 1));

  // edit values
  const int32 newValue = 11;
  scalar->setValue(newValue);
  REQUIRE(newValue == scalar->getValue());

  const int32 newValue2 = 14;
  (*scalar) = newValue2;
  REQUIRE(scalar->getValue() == newValue2);
}

TEST_CASE("DataStructureDuplicateNames")
{
  static constexpr StringLiteral name = "foo";

  DataStructure ds;

  // Top level test

  DataGroup* group1 = DataGroup::Create(ds, name);
  REQUIRE(group1 != nullptr);

  DataGroup* group2 = DataGroup::Create(ds, name);
  REQUIRE(group2 == nullptr);

  // Nested test

  DataGroup* childGroup1 = DataGroup::Create(ds, name, group1->getId());
  REQUIRE(group1 != nullptr);

  DataGroup* childGroup2 = DataGroup::Create(ds, name, group1->getId());
  REQUIRE(group2 == nullptr);
}
