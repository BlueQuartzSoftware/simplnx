#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/IdentifyDuplicateVerticesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

using namespace nx::core;

namespace
{
const DataPath k_CompDupPath = DataPath({"computed duplicates mask"});

void GenerateVertexGeom(DataStructure& dataStructure)
{
  // Create a Vertex Geometry grid for the Scan Data
  VertexGeom* vertexGeom = VertexGeom::Create(dataStructure, Constants::k_VertexGeometry);

  Float32Array* vertList = UnitTest::CreateTestDataArray<float32>(dataStructure, "SharedVertexList", std::vector<usize>{25}, {3});

  float32 xVal = 0.0f;
  float32 yVal = 0.0f;
  float32 zVal = 0.0f;
  for(usize i = 0; i < vertList->getNumberOfTuples(); i++)
  {
    vertList->setValue((i * 3) + 0, xVal);
    vertList->setValue((i * 3) + 1, yVal);
    vertList->setValue((i * 3) + 2, zVal);

    xVal += 1.0f;
    yVal += 1.0f;
    zVal += 1.0f;
  }

  vertexGeom->setVertices(*vertList);
}
} // namespace

TEST_CASE("SimplnxCore::IdentifyDuplicateVerticesFilter: Has Duplicates", "[SimplnxCore][IdentifyDuplicateVerticesFilter]")
{
  DataStructure dataStructure = {};
  GenerateVertexGeom(dataStructure);
  dataStructure.exportHierarchyAsText(std::cout);
  const std::array<usize, 4> dupIndices = {5, 6, 13, 24};
  {
    auto& vertexList = dataStructure.getDataRefAs<VertexGeom>(DataPath({Constants::k_VertexGeometry})).getVerticesRef();

    // Make sure that we are in bounds
    REQUIRE(vertexList.getNumberOfTuples() > 24);

    // Create duplicates
    for(usize tupIndex : dupIndices)
    {
      usize currentVert = tupIndex * 3;
      usize prevVert = (tupIndex - 1) * 3;

      vertexList[currentVert] = vertexList[prevVert];
      vertexList[currentVert + 1] = vertexList[prevVert + 1];
      vertexList[currentVert + 2] = vertexList[prevVert + 2];
    }
  }

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    IdentifyDuplicateVerticesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(IdentifyDuplicateVerticesFilter::k_InputGeomPath_Key, std::make_any<DataPath>(DataPath({Constants::k_VertexGeometry})));
    args.insertOrAssign(IdentifyDuplicateVerticesFilter::k_DuplicateMaskPath_Key, std::make_any<DataPath>(k_CompDupPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  auto& duplicates = dataStructure.getDataRefAs<UInt8Array>(k_CompDupPath);

  usize count = 0;
  for(usize i = 0; i < duplicates.getNumberOfTuples(); i++)
  {
    if(duplicates[i] != 0)
    {
      count++;
    }
  }

  REQUIRE(count == 4);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::IdentifyDuplicateVerticesFilter: No Duplicates", "[SimplnxCore][IdentifyDuplicateVerticesFilter]")
{
  DataStructure dataStructure = {};
  GenerateVertexGeom(dataStructure);

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    IdentifyDuplicateVerticesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(IdentifyDuplicateVerticesFilter::k_InputGeomPath_Key, std::make_any<DataPath>(DataPath({Constants::k_VertexGeometry})));
    args.insertOrAssign(IdentifyDuplicateVerticesFilter::k_DuplicateMaskPath_Key, std::make_any<DataPath>(k_CompDupPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  auto& duplicates = dataStructure.getDataRefAs<UInt8Array>(k_CompDupPath);

  usize count = 0;
  for(usize i = 0; i < duplicates.getNumberOfTuples(); i++)
  {
    if(duplicates[i] != 0)
    {
      count++;
    }
  }

  REQUIRE(count == 0);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
