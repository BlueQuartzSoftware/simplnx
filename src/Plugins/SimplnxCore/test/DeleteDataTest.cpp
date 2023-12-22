#include "SimplnxCore/Filters/DeleteData.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/Parameters/MultiPathSelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;

namespace CreateImageGeometryUnitTest
{
const fs::path k_DataDir = "test/data";
const fs::path k_TestFile = "CreateImageGeometry_Test.dream3d";
} // namespace CreateImageGeometryUnitTest

TEST_CASE("SimplnxCore::Delete Singular Data Array", "[SimplnxCore][DeleteData]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object

  std::vector<usize> imageDims = {40, 60, 80};
  std::vector<float32> imageSpacing = {0.10F, 2.0F, 33.0F};
  std::vector<float32> imageOrigin = {0.0F, 22.0F, 77.0F};

  VectorUInt64Parameter::ValueType inputDims = {imageDims[0], imageDims[1], imageDims[2]};

  DataStructure dataStructure = UnitTest::CreateAllPrimitiveTypes(imageDims);

  DataPath selectedDataPath1({k_LevelZero, k_LevelOne, k_Int8DataSet});
  DataPath selectedDataPath2({k_LevelZero, k_LevelOne, k_Int16DataSet});
  Arguments args;

  // Create default Parameters for the filter.
  // args.insertOrAssign(DeleteData::k_DeletionType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DeleteData::DeletionType::DeleteDataObject)));
  args.insertOrAssign(DeleteData::k_DataPath_Key, std::make_any<MultiPathSelectionParameter::ValueType>({selectedDataPath1, selectedDataPath2}));

  DeleteData filter;

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());

  DataObject* removedDataArray = dataStructure.getData(selectedDataPath1);
  REQUIRE(removedDataArray == nullptr);
  REQUIRE(dataStructure.getData(selectedDataPath2) == nullptr);
}

TEST_CASE("SimplnxCore::Delete Data Object (Node removal)", "[SimplnxCore][DeleteData]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object

  std::vector<usize> imageDims = {40, 60, 80};
  std::vector<float32> imageSpacing = {0.10F, 2.0F, 33.0F};
  std::vector<float32> imageOrigin = {0.0F, 22.0F, 77.0F};

  VectorUInt64Parameter::ValueType inputDims = {imageDims[0], imageDims[1], imageDims[2]};

  DataStructure dataStructure = UnitTest::CreateAllPrimitiveTypes(imageDims);

  DataPath selectedDataGroupPath({k_LevelZero, k_LevelOne});

  DeleteData filter;
  Arguments args;

  // Create default Parameters for the filter.
  // args.insertOrAssign(DeleteData::k_DeletionType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DeleteData::DeletionType::DeleteDataObject)));
  args.insertOrAssign(DeleteData::k_DataPath_Key, std::make_any<MultiPathSelectionParameter::ValueType>({selectedDataGroupPath}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());

  DataObject* removedDataArray = dataStructure.getData(selectedDataGroupPath);
  REQUIRE(removedDataArray == nullptr);
}

/**This code is commented out due to the current state of dataStructure management for the
 * project. These test cases are meant to verify the different fringe case functionality
 * should the underlying graph structure become more exposed. These test cases are mostly
 * functional but will likely need review upon time of implementation.
 */

// TEST_CASE("SimplnxCore::Delete Shared Node (Node removal)", "[SimplnxCore][DeleteData]")
//{
//   // For this test case the goal will be to completely wipe node (DataObject) C out of the
//   // graph completely. Ie clear all edges (parent and child) to node C, call the destructor
//   // on the node to ensure it doesn't become freehanging, and verify that the object is no longer
//   // in the data-lake (DataStructure) tables by ID grep.
//
//   // Create simplnx DataGraph and unpack it
//   DataStructure dataStructure(std::move(UnitTest::CreateComplexMultiLevelDataGraph()));
//   std::vector<DataPath> initialPaths = dataStructure.getAllDataPaths();
//
//   // Select DataPath to remove
//   DataPath selectedDataGroupPath({k_GroupBName, k_GroupCName});
//   bool found = false;
//   for(const auto& path : initialPaths)
//   {
//     if(path.toString() == selectedDataGroupPath.toString())
//     {
//       found = true;
//     }
//   }
//   REQUIRE(found);
//
//   // Store Data prior to deletion to verify removal
//   // The target node here is k_GroupCName (the DataGroup named C)
//   std::weak_ptr<DataObject> objectCPtr = dataStructure.getSharedData(selectedDataGroupPath); // convert the shared ptr to a weak ptr
//   auto groupCId = dataStructure.getId(selectedDataGroupPath).value();
//
//   DeleteData filter;
//   Arguments args;
//
//   // Create default Parameters for the filter.
//   args.insertOrAssign(DeleteData::k_DeletionType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DeleteData::DeletionType::DeleteDataObject)));
//   args.insertOrAssign(DeleteData::k_DataPath_Key, std::make_any<DataPath>(selectedDataGroupPath)); // already verified to exist
//
//   // Preflight the filter and check result
//   auto preflightResult = filter.preflight(dataStructure, args);
//   REQUIRE(preflightResult.outputActions.valid());
//
//   // Execute the filter and check the result
//   auto executeResult = filter.execute(dataStructure, args);
//   REQUIRE(executeResult.result.valid());
//
//   // Verify edges are wiped
//   std::vector<DataPath> alteredPaths = dataStructure.getAllDataPaths();
//   for(const auto& path : alteredPaths)
//   {
//     for(const auto& nodeName : path.getPathVector())
//     {
//       REQUIRE(nodeName.compare(k_GroupCName) != 0);
//     }
//   }
//
//   // Verify node is no longer in data lake
//   REQUIRE(dataStructure.getData(groupCId) == nullptr);
//
//   // Verify nodes data has been destructed
//   REQUIRE(objectCPtr.expired());
// }
//
// TEST_CASE("SimplnxCore::Delete DataPath to Object (Edge removal)", "[SimplnxCore][DeleteData]")
//{
//   // For this test case the goal will be to remove the edge between Top Level Group A and
//   // subgroup C. The nuance to this is that the data graph uses a doubly-linked list structure
//   // therefore the edge must be removed from the parent node's children list and the child node's
//   // parents list. The node's data and other edges to the nodes should be untouched.
//
//   // Create simplnx DataGraph and unpack it
//   DataStructure dataStructure(std::move(UnitTest::CreateComplexMultiLevelDataGraph()));
//   std::vector<DataPath> initialPaths = dataStructure.getAllDataPaths(); // This queries the parent lists implicitly
//
//   // Select DataPath to remove
//   DataPath selectedDataGroupPath({k_GroupBName, k_GroupCName});
//   bool found = false;
//   for(const auto& path : initialPaths)
//   {
//     if(path.toString() == selectedDataGroupPath.toString())
//     {
//       found = true;
//     }
//   }
//   REQUIRE(found);
//
//   DeleteData filter;
//   Arguments args;
//
//   // Create default Parameters for the filter.
//   args.insertOrAssign(DeleteData::k_DeletionType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DeleteData::DeletionType::DeleteDataObject)));
//   args.insertOrAssign(DeleteData::k_DataPath_Key, std::make_any<DataPath>(selectedDataGroupPath)); // already verified to exist
//
//   // Preflight the filter and check result
//   auto preflightResult = filter.preflight(dataStructure, args);
//   REQUIRE(preflightResult.outputActions.valid());
//
//   // Execute the filter and check the result
//   auto executeResult = filter.execute(dataStructure, args);
//   REQUIRE(executeResult.result.valid());
//
//   std::vector<DataPath> alteredPaths = dataStructure.getAllDataPaths(); // This queries the parent lists implicitly
//   for(const auto& path : alteredPaths)
//   {
//     REQUIRE(path.toString() != selectedDataGroupPath.toString());
//   }
// }
//
// TEST_CASE("SimplnxCore::Orphaning A Child Node to Top Level", "[SimplnxCore][DeleteData]")
//{
//   // For this test case the goal will be to remove Group D node and check that the Array I node
//   // has been moved to the top level. This could also occur if the edge between Group D node and
//   // the Array I node is terminated which will be tested in a subsequent section.
//
//   SECTION("Orphan Through Node Deletion")
//   {
//     // Create simplnx DataGraph and unpack it
//     DataStructure dataStructure(std::move(UnitTest::CreateComplexMultiLevelDataGraph()));
//     std::vector<DataPath> paths = dataStructure.getAllDataPaths();
//
//     // Select DataPath to remove
//     DataPath selectedDataGroupPath({k_GroupAName, k_GroupCName, k_GroupDName}); // We are aiming to delete the D group
//     bool found = false;
//     for(const auto& path : paths)
//     {
//       if(path.toString() == selectedDataGroupPath.toString())
//       {
//         found = true;
//       }
//     }
//     REQUIRE(found);
//
//     // Store Data prior to deletion to verify removal
//     // The target node here is k_GroupDName (the DataGroup named D)
//     std::weak_ptr<DataObject> objectDPtr = dataStructure.getSharedData(selectedDataGroupPath); // convert the shared ptr to a weak ptr
//     auto groupDId = dataStructure.getId(selectedDataGroupPath).value();
//
//     DeleteData filter;
//     Arguments args;
//
//     // Create default Parameters for the filter.
//     args.insertOrAssign(DeleteData::k_DeletionType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DeleteData::DeletionType::DeleteDataObject)));
//     args.insertOrAssign(DeleteData::k_DataPath_Key, std::make_any<DataPath>(selectedDataGroupPath)); // already verified to exist
//
//     // Preflight the filter and check result
//     auto preflightResult = filter.preflight(dataStructure, args);
//     REQUIRE(preflightResult.outputActions.valid());
//
//     // Execute the filter and check the result
//     auto executeResult = filter.execute(dataStructure, args);
//     REQUIRE(executeResult.result.valid());
//
//     // Verify edges are wiped
//     std::vector<DataPath> alteredPaths = dataStructure.getAllDataPaths();
//     for(const auto& path : alteredPaths)
//     {
//       for(const auto& nodeName : path.getPathVector())
//       {
//         REQUIRE(nodeName.compare(k_GroupDName) != 0);
//       }
//     }
//
//     // Verify node is no longer in data lake
//     REQUIRE(dataStructure.getData(groupDId) == nullptr);
//
//     // Verify nodes data has been destructed
//     REQUIRE(objectDPtr.expired());
//
//     // Now that Group D is verifiably gone verify array I still exists
//     bool arrayIFound = false;
//     for(const auto& path : alteredPaths)
//     {
//       for(const auto& nodeName : path.getPathVector())
//       {
//         if(nodeName.compare(k_ArrayIName) == 0)
//         {
//           arrayIFound = true;
//           REQUIRE(path.getLength() < 2); // must be top level
//         }
//       }
//     }
//
//     // if array I has been found it has also been verified to be top level
//     REQUIRE(arrayIFound);
//   }
//
//   SECTION("Orphan Through Edge Deletion")
//   {
//     // Create simplnx DataGraph and unpack it
//     DataStructure dataStructure(std::move(UnitTest::CreateComplexMultiLevelDataGraph()));
//     std::vector<DataPath> initialPaths = dataStructure.getAllDataPaths();
//
//     // Select DataPath to remove
//     DataPath selectedDataGroupPath({k_GroupBName, k_GroupCName, k_GroupDName, k_ArrayIName}); // We are aiming to delete the D group
//     bool found = false;
//     for(const auto& path : initialPaths)
//     {
//       if(path.toString() == selectedDataGroupPath.toString())
//       {
//         found = true;
//       }
//     }
//     REQUIRE(found);
//
//     DeleteData filter;
//     Arguments args;
//
//     // Create default Parameters for the filter.
//     args.insertOrAssign(DeleteData::k_DeletionType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DeleteData::DeletionType::DeleteDataPath)));
//     args.insertOrAssign(DeleteData::k_DataPath_Key, std::make_any<DataPath>(selectedDataGroupPath)); // already verified to exist
//
//     // Preflight the filter and check result
//     auto preflightResult = filter.preflight(dataStructure, args);
//     REQUIRE(preflightResult.outputActions.valid());
//
//     // Execute the filter and check the result
//     auto executeResult = filter.execute(dataStructure, args);
//     REQUIRE(executeResult.result.valid());
//
//     // Verify edges are wiped
//     std::vector<DataPath> alteredPaths = dataStructure.getAllDataPaths();
//
//     // Now that Group D is verifiably gone verify array I still exists
//     bool arrayIFound = false;
//     for(const auto& path : alteredPaths)
//     {
//       for(const auto& nodeName : path.getPathVector())
//       {
//         if(nodeName.compare(k_ArrayIName) == 0)
//         {
//           arrayIFound = true;
//           REQUIRE(path.getLength() < 2); // must be top level
//         }
//       }
//     }
//
//     // if array I has been found it has also been verified to be top level
//     REQUIRE(arrayIFound);
//   }
// }
//
// TEST_CASE("SimplnxCore::Attempt Delete Shared Node", "[SimplnxCore][DeleteData]")
//{
//   // The goal of this test is to attempt to delete a shared node using the delete
//   // unshared children. Expected behaviour: the node is untouched (and in actual execution
//   // a warning will be thrown)
//
//   // Create simplnx DataGraph and unpack it
//   DataStructure dataStructure(std::move(UnitTest::CreateComplexMultiLevelDataGraph()));
//   std::vector<DataPath> initialPaths = dataStructure.getAllDataPaths();
//
//   // Select DataPath to remove
//   DataPath selectedDataGroupPath({k_GroupBName, k_GroupFName, k_GroupEName});
//
//   int32 firstGroupECount = 0;
//   bool found = false;
//   for(const auto& path : initialPaths)
//   {
//     if(path.toString() == selectedDataGroupPath.toString())
//     {
//       found = true;
//     }
//     for(const auto& nodeName : path.getPathVector())
//     {
//       if(nodeName.compare(k_GroupEName) == 0)
//       {
//         firstGroupECount++;
//       }
//     }
//   }
//   REQUIRE(found);
//
//   DeleteData filter;
//   Arguments args;
//
//   // Create default Parameters for the filter.
//   args.insertOrAssign(DeleteData::k_DeletionType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DeleteData::DeletionType::DeleteUnsharedChildren)));
//   args.insertOrAssign(DeleteData::k_DataPath_Key, std::make_any<DataPath>(selectedDataGroupPath)); // already verified to exist
//
//   // Preflight the filter and check result
//   auto preflightResult = filter.preflight(dataStructure, args);
//   REQUIRE(preflightResult.outputActions.valid());
//
//   // Execute the filter and check the result
//   auto executeResult = filter.execute(dataStructure, args);
//   REQUIRE(executeResult.result.valid());
//
//   std::vector<DataPath> alteredPaths = dataStructure.getAllDataPaths();
//
//   int32 secondGroupECount = 0;
//   for(const auto& path : alteredPaths)
//   {
//     for(const auto& nodeName : path.getPathVector())
//     {
//       if(nodeName.compare(k_GroupEName) == 0)
//       {
//         secondGroupECount++;
//       }
//     }
//   }
//   REQUIRE(secondGroupECount == firstGroupECount);
// }
//
// TEST_CASE("SimplnxCore::Delete Node with Multi-Parented Children", "[SimplnxCore][DeleteData]")
//{
//   // For this test case the goal is to pass in a top level object [group B] with multi-nested children and verify
//   // the results throughout the four levels according to the respective deletion type.
//
//   // Select DataPaths
//   DataPath groupBPath({k_GroupBName});
//   DataPath groupFPath({k_GroupBName, k_GroupFName});
//   DataPath groupGPath({k_GroupBName, k_GroupFName, k_GroupGName});
//   DataPath groupEPath({k_GroupBName, k_GroupCName, k_GroupEName});
//   DataPath arrayMPath({k_GroupBName, k_GroupFName, k_GroupGName, k_ArrayMName});
//   DataPath arrayLPath({k_GroupBName, k_GroupFName, k_GroupGName, k_ArrayLName});
//   DataPath arrayKPath({k_GroupBName, k_GroupFName, k_GroupGName, k_ArrayKName});
//
//   SECTION("Delete Top Level with delete unshared children (Recursion Check)")
//   {
//     // Create simplnx DataGraph and unpack it
//     DataStructure dataStructure(std::move(UnitTest::CreateComplexMultiLevelDataGraph()));
//     std::vector<DataPath> paths = dataStructure.getAllDataPaths();
//
//     // Store Data prior to deletion to verify removal
//     // The target node here is k_GroupBName (the DataGroup named B)
//     std::map<DataObject::IdType, std::weak_ptr<DataObject>> removedValues;
//     std::vector<DataPath> pathsRemoved = {groupBPath, groupFPath, groupGPath, arrayMPath, arrayLPath};
//
//     for(const auto& path : pathsRemoved)
//     {
//       removedValues.emplace(dataStructure.getId(path).value(), dataStructure.getSharedData(path));
//     }
//
//     std::weak_ptr<DataObject> objectEPtr = dataStructure.getSharedData(groupEPath); // convert the shared ptr to a weak ptr
//     auto groupEId = dataStructure.getId(groupEPath).value();
//     std::weak_ptr<DataObject> objectKPtr = dataStructure.getSharedData(arrayKPath); // convert the shared ptr to a weak ptr
//     auto arrayKId = dataStructure.getId(arrayKPath).value();
//
//     DeleteData filter;
//     Arguments args;
//
//     // Create default Parameters for the filter.
//     args.insertOrAssign(DeleteData::k_DeletionType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DeleteData::DeletionType::DeleteUnsharedChildren)));
//     args.insertOrAssign(DeleteData::k_DataPath_Key, std::make_any<DataPath>(groupBPath));
//
//     // Preflight the filter and check result
//     auto preflightResult = filter.preflight(dataStructure, args);
//     REQUIRE(preflightResult.outputActions.valid());
//
//     // Execute the filter and check the result
//     auto executeResult = filter.execute(dataStructure, args);
//     REQUIRE(executeResult.result.valid());
//
//     // Verify deleted down to level Three
//     std::vector<DataPath> alteredPaths = dataStructure.getAllDataPaths();
//     for(const auto& [identifier, weakPtr] : removedValues)
//     {
//       // Verify node is no longer in data lake
//       REQUIRE(dataStructure.getData(identifier) == nullptr);
//
//       // Verify node's data has been destructed
//       REQUIRE(weakPtr.expired());
//     }
//
//     // Verify unshared children not deleted
//     // Verify E node is in data lake
//     REQUIRE(dataStructure.getData(groupEId) != nullptr);
//     REQUIRE(dataStructure.getData(arrayKId) != nullptr);
//
//     // Verify E node's data has not been destructed
//     REQUIRE(!objectEPtr.expired());
//     REQUIRE(!objectKPtr.expired());
//   }
//
//   SECTION("Delete Top Level without Regard to Childrens' Parent Count (Recursion Check)")
//   {
//     // Create simplnx DataGraph and unpack it
//     DataStructure dataStructure(std::move(UnitTest::CreateComplexMultiLevelDataGraph()));
//     std::vector<DataPath> paths = dataStructure.getAllDataPaths();
//
//     // Store Data prior to deletion to verify removal
//     // The target node here is k_GroupBName (the DataGroup named B)
//     std::map<DataObject::IdType, std::weak_ptr<DataObject>> removedValues;
//     std::vector<DataPath> pathsRemoved = {groupBPath, groupFPath, groupGPath, groupEPath, arrayMPath, arrayLPath, arrayKPath};
//
//     for(const auto& path : pathsRemoved)
//     {
//       removedValues.emplace(dataStructure.getId(path).value(), dataStructure.getSharedData(path));
//     }
//
//     DeleteData filter;
//     Arguments args;
//
//     // Create default Parameters for the filter.
//     args.insertOrAssign(DeleteData::k_DeletionType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DeleteData::DeletionType::DeleteChildren)));
//     args.insertOrAssign(DeleteData::k_DataPath_Key, std::make_any<DataPath>(groupBPath));
//
//     // Preflight the filter and check result
//     auto preflightResult = filter.preflight(dataStructure, args);
//     REQUIRE(preflightResult.outputActions.valid());
//
//     // Execute the filter and check the result
//     auto executeResult = filter.execute(dataStructure, args);
//     REQUIRE(executeResult.result.valid());
//
//     // Verify deleted down to level Three
//     std::vector<DataPath> alteredPaths = dataStructure.getAllDataPaths();
//     for(const auto& [identifier, weakPtr] : removedValues)
//     {
//       // Verify node is no longer in data lake
//       REQUIRE(dataStructure.getData(identifier) == nullptr);
//
//       // Verify node's data has been destructed
//       REQUIRE(weakPtr.expired());
//     }
//   }
// }
