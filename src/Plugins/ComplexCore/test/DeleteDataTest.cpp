#include <catch2/catch.hpp>

#include "complex/Common/TypeTraits.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/unit_test/complex_test_dirs.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/Filters/DeleteData.hpp"

using namespace complex;
using namespace complex::Constants;

namespace fs = std::filesystem;

namespace CreateImageGeometryUnitTest
{
const fs::path k_DataDir = "test/data";
const fs::path k_TestFile = "CreateImageGeometry_Test.dream3d";
} // namespace CreateImageGeometryUnitTest

TEST_CASE("ComplexCore::Delete Singular Data Array", "[ComplexCore][DeleteData]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object

  std::vector<usize> imageDims = {40, 60, 80};
  std::vector<float32> imageSpacing = {0.10F, 2.0F, 33.0F};
  std::vector<float32> imageOrigin = {0.0F, 22.0F, 77.0F};

  VectorUInt64Parameter::ValueType inputDims = {imageDims[0], imageDims[1], imageDims[2]};

  DataStructure dataGraph = UnitTest::CreateAllPrimitiveTypes(imageDims);

  DataPath selectedDataGroupPath({k_LevelZero, k_LevelOne, k_Int8DataSet});
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(DeleteData::k_DataPath_Key, std::make_any<DataPath>(selectedDataGroupPath));

  DeleteData filter;

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid());

  DataObject* removedDataArray = dataGraph.getData(selectedDataGroupPath);
  REQUIRE(removedDataArray == nullptr);
}

TEST_CASE("ComplexCore::Delete Data Object (Node removal)", "[ComplexCore][DeleteData]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object

  std::vector<usize> imageDims = {40, 60, 80};
  std::vector<float32> imageSpacing = {0.10F, 2.0F, 33.0F};
  std::vector<float32> imageOrigin = {0.0F, 22.0F, 77.0F};

  VectorUInt64Parameter::ValueType inputDims = {imageDims[0], imageDims[1], imageDims[2]};

  DataStructure dataGraph = UnitTest::CreateAllPrimitiveTypes(imageDims);

  DataPath selectedDataGroupPath({k_LevelZero, k_LevelOne});

  DeleteData filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(DeleteData::k_DataPath_Key, std::make_any<DataPath>(selectedDataGroupPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid());

  DataObject* removedDataArray = dataGraph.getData(selectedDataGroupPath);
  REQUIRE(removedDataArray == nullptr);
}

/**These Test Cases Should be Implemented down the line if Advanced Data Graph Management is implemented
 * They require the following:
 * Actual values for selectedDataGroupPath
 * Updated parameters to get desired functionality
 * Requires after filter execution
 */

// TEST_CASE("ComplexCore::Delete DataPath to Object (Edge removal)", "[ComplexCore][DeleteData]")
//{
//   // Create complex DataGraph and unpack it
//   DataStructure dataGraph(std::move(UnitTest::CreateComplexMultiLevelDataGraph()));
//   std::vector<DataPath> paths = dataGraph.getAllDataPaths();
//
//   // Select DataPath to remove
//   DataPath selectedDataGroupPath;
//   bool found = false;
//   for (const auto& path : paths)
//   {
//     if(path == selectedDataGroupPath)
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
//   args.insertOrAssign(DeleteData::k_DataPath_Key, std::make_any<DataPath>(selectedDataGroupPath)); // already verified to exist
//
//   // Preflight the filter and check result
//   auto preflightResult = filter.preflight(dataGraph, args);
//   REQUIRE(preflightResult.outputActions.valid());
//
//   // Execute the filter and check the result
//   auto executeResult = filter.execute(dataGraph, args);
//   REQUIRE(executeResult.result.valid());
// }

// TEST_CASE("ComplexCore::Attempt Delete Shared Node", "[ComplexCore][DeleteData]")
//{
//   // Create complex DataGraph and unpack it
//   DataStructure dataGraph(std::move(UnitTest::CreateComplexMultiLevelDataGraph()));
//   std::vector<DataPath> paths = dataGraph.getAllDataPaths();
//
//   // Select DataPath to remove
//   DataPath selectedDataGroupPath;
//   bool found = false;
//   for(const auto& path : paths)
//   {
//     if(path == selectedDataGroupPath)
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
//   args.insertOrAssign(DeleteData::k_DataPath_Key, std::make_any<DataPath>(selectedDataGroupPath)); // already verified to exist
//
//   // Preflight the filter and check result
//   auto preflightResult = filter.preflight(dataGraph, args);
//   REQUIRE(preflightResult.outputActions.valid());
//
//   // Execute the filter and check the result
//   auto executeResult = filter.execute(dataGraph, args);
//   REQUIRE(executeResult.result.valid());
// }

TEST_CASE("ComplexCore::Delete Shared Node (Node removal)", "[ComplexCore][DeleteData]")
{
  // For this test case the goal will be to completely wipe node (DataObject) C out of the
  // graph completely. Ie clear all edges (parent and child) to node C, call the destructor
  // on the node to ensure it doesn't become freehanging, and verify that the object is no longer
  // in the data-lake (DataStructure) tables by ID grep.

  // Create complex DataGraph and unpack it
  DataStructure dataGraph(std::move(UnitTest::CreateComplexMultiLevelDataGraph()));
  std::vector<DataPath> initialPaths = dataGraph.getAllDataPaths();

  // Select DataPath to remove
  DataPath selectedDataGroupPath({k_GroupBName, k_GroupCName});
  bool found = false;
  for(const auto& path : initialPaths)
  {
    if(path == selectedDataGroupPath)
    {
      found = true;
    }
  }
  REQUIRE(found);

  // Store Data prior to deletion to verify removal
  // The target node here is k_GroupCName (the DataGroup named C)
  std::weak_ptr<DataObject> objectCPtr = dataGraph.getSharedData(selectedDataGroupPath); // convert the shared ptr to a weak ptr
  auto groupCId = dataGraph.getId(selectedDataGroupPath).value();

  DeleteData filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(DeleteData::k_DataPath_Key, std::make_any<DataPath>(selectedDataGroupPath)); // already verified to exist

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid());

  // Verify edges are wiped
  std::vector<DataPath> alteredPaths = dataGraph.getAllDataPaths();
  for(const auto& path : alteredPaths)
  {
    for(const auto& nodeName : path.getPathVector())
    {
      REQUIRE(nodeName != k_GroupCName);
    }
  }

  // Verify node is no longer in data lake
  REQUIRE(dataGraph.getData(groupCId) == nullptr);

  // Verify nodes data has been destructed
  REQUIRE(objectCPtr.expired());
}

TEST_CASE("ComplexCore::Delete Top Level with Multi-Parented Children", "[ComplexCore][DeleteData]")
{
  // For this test case the goal is to pass in a top level object with multi-nested children and set
  // parameters so that the multiparented children (and their subsequent children regardless of number
  // of parents) remain.  

  // Create complex DataGraph and unpack it
  DataStructure dataGraph(std::move(UnitTest::CreateComplexMultiLevelDataGraph()));
  std::vector<DataPath> paths = dataGraph.getAllDataPaths();

  // Select DataPath to remove
  DataPath selectedDataGroupPath;
  bool found = false;
  for(const auto& path : paths)
  {
    if(path == selectedDataGroupPath)
    {
      found = true;
    }
  }
  REQUIRE(found);

  DeleteData filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(DeleteData::k_DataPath_Key, std::make_any<DataPath>(selectedDataGroupPath)); // already verified to exist

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("ComplexCore::Orphaning A Child Node to Top Level", "[ComplexCore][DeleteData]")
//{
//   // Create complex DataGraph and unpack it
//   DataStructure dataGraph(std::move(UnitTest::CreateComplexMultiLevelDataGraph()));
//   std::vector<DataPath> paths = dataGraph.getAllDataPaths();
//
//   // Select DataPath to remove
//   DataPath selectedDataGroupPath;
//   bool found = false;
//   for(const auto& path : paths)
//   {
//     if(path == selectedDataGroupPath)
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
//   args.insertOrAssign(DeleteData::k_DataPath_Key, std::make_any<DataPath>(selectedDataGroupPath)); // already verified to exist
//
//   // Preflight the filter and check result
//   auto preflightResult = filter.preflight(dataGraph, args);
//   REQUIRE(preflightResult.outputActions.valid());
//
//   // Execute the filter and check the result
//   auto executeResult = filter.execute(dataGraph, args);
//   REQUIRE(executeResult.result.valid());
// }

// TEST_CASE("ComplexCore::Delete Level Zero Object and Test Level Three Children", "[ComplexCore][DeleteData]")
//{
//   // Create complex DataGraph and unpack it
//   DataStructure dataGraph(std::move(UnitTest::CreateComplexMultiLevelDataGraph()));
//   std::vector<DataPath> paths = dataGraph.getAllDataPaths();
//
//   // Select DataPath to remove
//   DataPath selectedDataGroupPath;
//   bool found = false;
//   for(const auto& path : paths)
//   {
//     if(path == selectedDataGroupPath)
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
//   args.insertOrAssign(DeleteData::k_DataPath_Key, std::make_any<DataPath>(selectedDataGroupPath)); // already verified to exist
//
//   // Preflight the filter and check result
//   auto preflightResult = filter.preflight(dataGraph, args);
//   REQUIRE(preflightResult.outputActions.valid());
//
//   // Execute the filter and check the result
//   auto executeResult = filter.execute(dataGraph, args);
//   REQUIRE(executeResult.result.valid());
// }
