#include <exception>
#include <iostream>
#include <memory>
#include <vector>

#include "DataStructObserver.hpp"

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/DataStore.hpp"

/**
 * @brief Test creation and removal of items in a tree-style structure. No node has more than one parent.
 */
void createDataGroupTreeTest()
{
  std::cout << "createDataGroupTreeTest()..." << std::endl;
  std::cout << "  Creating DataStructure..." << std::endl;

  // Create structuure
  DataStructure dataStr;
  auto group = dataStr.createGroup("Foo");
  auto child = dataStr.createGroup("bar", group->getId());
  auto grandchild = dataStr.createGroup("bazz", child->getId());

  auto groupId = group->getId();
  auto childId = child->getId();
  auto grandchildId = grandchild->getId();

  // Can all data be found by ID?
  if(nullptr == dataStr.getData(groupId))
  {
    throw std::exception();
  }
  if(nullptr == dataStr.getData(childId))
  {
    throw std::exception();
  }
  if(nullptr == dataStr.getData(grandchildId))
  {
    throw std::exception();
  }

  std::cout << "  Removing mid-level DataGroup..." << std::endl;
  // Test removing parent group. Does its child get deleted?
  group->remove(child->getName());
  if(nullptr != dataStr.getData(childId))
  {
    throw std::exception();
  }
  if(nullptr != dataStr.getData(grandchildId))
  {
    throw std::exception();
  }

  std::cout << "  Removing top-level DataGroup..." << std::endl;
  // Test removing top-most group
  dataStr.removeData(group->getId());
  if(nullptr != dataStr.getData(groupId))
  {
    throw std::exception();
  }

  std::cout << "createDataStructure() test complete\n" << std::endl;
}

/**
 * @brief Test creation and removal of DataObjects in a graph-style DataStructure where multiple parents are allowed.
 */
void createDataGroupGraphTest()
{
  std::cout << "createDataGroupGraphTest()..." << std::endl;
  std::cout << "  Creating DataStructure..." << std::endl;

  // Create DataStructure
  DataStructure dataStr;
  auto group = dataStr.createGroup("Foo");
  auto child1 = dataStr.createGroup("Bar1", group->getId());
  auto child2 = dataStr.createGroup("Bar2", group->getId());
  auto grandchild = dataStr.createGroup("Bazz", child1->getId());

  // Get IDs
  auto groupId = group->getId();
  auto child1Id = child1->getId();
  auto child2Id = child2->getId();
  auto grandchildId = grandchild->getId();

  // Test Multi-parent connections
  if(!dataStr.setAdditionalParent(grandchildId, child2Id))
  {
    throw std::exception();
  }

  // Check that all data be found by ID
  if(nullptr == dataStr.getData(groupId))
  {
    throw std::exception();
  }
  if(nullptr == dataStr.getData(child1Id))
  {
    throw std::exception();
  }
  if(nullptr == dataStr.getData(child2Id))
  {
    throw std::exception();
  }
  if(nullptr == dataStr.getData(grandchildId))
  {
    throw std::exception();
  }

  std::cout << "  Removing mid-level DataGroup..." << std::endl;
  // Test removing a single parent
  group->remove(child1->getName());
  if(nullptr != dataStr.getData(child1Id))
  {
    throw std::exception();
  }
  if(nullptr == dataStr.getData(grandchildId))
  {
    throw std::exception();
  }

  std::cout << "  Removing top-level DataGroup..." << std::endl;

  dataStr.removeData(group->getId());
  if(nullptr != dataStr.getData(groupId))
  {
    throw std::exception();
  }
  if(nullptr != dataStr.getData(grandchildId))
  {
    throw std::exception();
  }

  std::cout << "createDataStructure() test complete\n" << std::endl;
}

/**
 * @brief Test DataPath usage
 */
void dataPathTest()
{
  std::cout << "dataPathTest()..." << std::endl;
  std::cout << "  Creating DataStructure..." << std::endl;

  // Create DataStructure
  DataStructure dataStr;
  auto group = dataStr.createGroup("Foo");
  auto child1 = dataStr.createGroup("Bar1", group->getId());
  auto child2 = dataStr.createGroup("Bar2", group->getId());
  auto grandchild = dataStr.createGroup("Bazz", child1->getId());

  auto child2Id = child2->getId();
  auto grandchildId = grandchild->getId();
  if(!dataStr.setAdditionalParent(grandchildId, child2Id))
  {
    throw std::exception();
  }

  // Test DataPath lookup using DataPaths
  DataPath gcPath1({"Foo", "Bar1", "Bazz"});
  DataPath gcPath2({"Foo", "Bar2", "Bazz"});
  if(nullptr == dataStr.getData(gcPath1))
  {
    throw std::exception();
  }
  if(nullptr == dataStr.getData(gcPath2))
  {
    throw std::exception();
  }

  std::cout << "  Removing mid-level DataGroup..." << std::endl;
  // Test removing a parent group using a DataPath
  DataPath c1Path({"Foo", "Bar1"});
  if(!dataStr.removeData(c1Path))
  {
    throw std::exception();
  }
  if(nullptr != dataStr.getData(gcPath1))
  {
    throw std::exception();
  }
  if(nullptr == dataStr.getData(DataPath({"Foo", "Bar2"})))
  {
    throw std::exception();
  }
  auto gc2 = dataStr.getData(gcPath2);
  if(nullptr == dataStr.getData(gcPath2))
  {
    throw std::exception();
  }

  std::cout << "  Removing mid-level DataGroup..." << std::endl;
  // Test removing the remaining parent group via DataPath
  dataStr.removeData(child2Id);
  if(nullptr != dataStr.getData(gcPath2))
  {
    throw std::exception();
  }

  std::cout << "dataPathTest() test complete\n" << std::endl;
}

void linkedPathTest()
{
  std::cout << "linkedPathTest()..." << std::endl;
  std::cout << "  Creating DataStructure..." << std::endl;

  // Create DataStructure
  DataStructure dataStr;
  auto group = dataStr.createGroup("Foo");
  auto child1 = dataStr.createGroup("Bar1", group->getId());
  auto child2 = dataStr.createGroup("Bar2", group->getId());
  auto grandchild = dataStr.createGroup("Bazz", child1->getId());

  auto child2Id = child2->getId();
  auto grandchildId = grandchild->getId();
  if(!dataStr.setAdditionalParent(grandchildId, child2Id))
  {
    throw std::exception();
  }

  // DataPath
  DataPath grandPath({"Foo", "Bar1", "Bazz"});
  std::cout << "  -Data Path: " << grandPath.toString() << std::endl;

  auto linkedPath = dataStr.getLinkedPath(grandPath);
  std::cout << "  -Linked Path: " << linkedPath.toString() << std::endl;
  if(!linkedPath.isValid())
  {
    throw std::exception();
  }

  // Check path result
  if(grandchild != linkedPath.getData())
  {
    throw std::exception();
  }
  if(!linkedPath.isValid())
  {
    throw std::exception();
  }

  // Try rename
  std::cout << "  Renamed group: Bar1 -> Bar1.3" << std::endl;
  if(!child1->rename("Bar1.3"))
  {
    throw std::exception();
  }
  if(nullptr != dataStr.getData(grandPath))
  {
    throw std::exception();
  }
  if(nullptr == linkedPath.getData())
  {
    throw std::exception();
  }
  if(!linkedPath.isValid())
  {
    throw std::exception();
  }
  std::cout << "  -Linked Path: " << linkedPath.toString() << std::endl;

  std::cout << "  Removed group: Bar1.3" << std::endl;
  if(!dataStr.removeData(linkedPath.getIdAt(1)))
  {
    throw std::exception();
  }
  if(linkedPath.isValid())
  {
    throw std::exception();
  }

  std::cout << "  -Linked Path: " << linkedPath.toString() << "\n" << std::endl;

  std::cout << "linkedPathTest() test complete\n" << std::endl;
}

/**
 * @brief Tests IDataStructureListener usage
 */
void dataStructureListenerTest()
{
  std::cout << "dataStructureListenerTest()..." << std::endl;
  std::cout << "  Creating DataStructure...\n" << std::endl;

  // Create DataStructure
  DataStructure dataStr;
  DataStructObserver dsListener(dataStr);

  // Adds items to the DataStructure. Each addition should trigger dsListener.
  auto group = dataStr.createGroup("Foo");
  auto child1 = dataStr.createGroup("Bar1", group->getId());
  auto child2 = dataStr.createGroup("Bar2", group->getId());
  auto grandchild = dataStr.createGroup("Bazz", child1->getId());

  auto groupId = group->getId();
  auto child2Id = child2->getId();
  auto grandchildId = grandchild->getId();
  // Adds a second parent to grandchild group. This should trigger dsListener.
  if(!dataStr.setAdditionalParent(grandchildId, child2Id))
  {
    throw std::exception();
  }

  // Removes a mid-level group. This should trigger dsListener.
  dataStr.removeData(child2Id);
  dataStr.removeData(groupId);

  std::cout << "dataStructureListenerTest() test complete\n" << std::endl;
}

void dataStructureCopyTest()
{
  std::cout << "dataStructureCopyTest()..." << std::endl;
  std::cout << "  Creating DataStructure..." << std::endl;

  // Create DataStructure
  DataStructure dataStr;
  auto group = dataStr.createGroup("Foo");
  auto child1 = dataStr.createGroup("Bar1", group->getId());
  auto child2 = dataStr.createGroup("Bar2", group->getId());
  auto grandchild = dataStr.createGroup("Bazz", child1->getId());

  auto groupId = group->getId();
  auto child1Id = child1->getId();
  auto child2Id = child2->getId();
  auto grandchildId = grandchild->getId();
  if(!dataStr.setAdditionalParent(grandchildId, child2Id))
  {
    throw std::exception();
  }

  // Copy DataStructure
  DataStructure dataStrCopy(dataStr);

  // Check ID exists
  if(!dataStrCopy.getData(groupId))
  {
    throw std::exception();
  }
  if(!dataStrCopy.getData(child1Id))
  {
    throw std::exception();
  }
  if(!dataStrCopy.getData(child2Id))
  {
    throw std::exception();
  }
  if(!dataStrCopy.getData(grandchildId))
  {
    throw std::exception();
  }

  // Check Data Copied
  DataObject* data = dataStr.getData(groupId);
  DataObject* dataCopy = dataStrCopy.getData(groupId);
  if(dataStrCopy.getData(groupId) == dataStr.getData(groupId))
  {
    throw std::exception();
  }
  if(dataStrCopy.getData(child1Id) == dataStr.getData(child1Id))
  {
    throw std::exception();
  }
  if(dataStrCopy.getData(child2Id) == dataStr.getData(child2Id))
  {
    throw std::exception();
  }
  if(dataStrCopy.getData(grandchildId) == dataStr.getData(grandchildId))
  {
    throw std::exception();
  }

  // Create new group
  auto newGroup = dataStr.createGroup("New Group", child2Id);
  auto newId = newGroup->getId();
  if(!dataStr.getData(newId))
  {
    throw std::exception();
  }
  if(dataStrCopy.getData(newId))
  {
    throw std::exception();
  }

  auto newGroup2 = dataStrCopy.createGroup("New Group (2)", child2Id);
  auto newId2 = newGroup2->getId();
  if(dataStr.getData(newId2))
  {
    throw std::exception();
  }
  if(!dataStrCopy.getData(newId2))
  {
    throw std::exception();
  }

  std::cout << "dataStructureCopyTest() test complete\n" << std::endl;
}

void dataStoreTest()
{
  std::cout << "dataStoreTest()..." << std::endl;
  std::cout << "  Creating DataStore..." << std::endl;

  DataStore<int32_t> store(3, 10);

  std::cout << "  Testing store size..." << std::endl;

  if(store.getTupleCount() != 10)
  {
    throw std::exception();
  }
  if(store.getTupleSize() != 3)
  {
    throw std::exception();
  }
  if(store.size() != 30)
  {
    throw std::exception();
  }

  std::cout << "  Testing reading / writing to memory..." << std::endl;
  //  Test subscript operator
  {
    for(size_t i = 0; i < store.size(); i++)
    {
      store[i] = i + 1;
    }
    int32_t x = 1;
    for(size_t i = 0; i < store.size(); i++)
    {
      if(store[i] != x++)
      {
        throw std::exception();
      }
    }
  }

  // Test get/set values
  {
    const size_t index = 5;
    const size_t value = 25;
    store.setValue(index, value);
    if(store.getValue(index) != value)
    {
      throw std::exception();
    }
  }

  // Test iterators
  {
    const int32_t initVal = -30;
    int32_t x = initVal;
    for(auto& value : store)
    {
      value = x++;
    }
    x = initVal;
    for(const auto& value : store)
    {
      if(value != x++)
      {
        throw std::exception();
      }
    }
  }

  std::cout << "dataStoreTest() test complete\n" << std::endl;
}

/**
 * @brief
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char** argv)
{
  createDataGroupTreeTest();
  createDataGroupGraphTest();
  dataPathTest();
  linkedPathTest();
  dataStructureListenerTest();
  dataStructureCopyTest();
  dataStoreTest();
}
