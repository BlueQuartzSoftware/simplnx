#include <catch2/catch.hpp>

#include <cmath>
#include <limits>

#include "SimplnxCore/Filters/Algorithms/IdentifyDuplicateVertices.hpp"
#include "SimplnxCore/Filters/IdentifyDuplicateVerticesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Meshing/VertexUtilities.hpp"

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

TEST_CASE("SimplnxCore::IdentifyDuplicateVerticesFilter: Coordinate ties", "[SimplnxCore][IdentifyDuplicateVerticesFilter][L19]")
{
  UnitTest::LoadPlugins();
  const bool largerCloud = GENERATE(false, true);
  const usize uniqueCount = largerCloud ? 170 : 9;
  const usize tupleCount = largerCloud ? 260 : 18;
  const usize columns = largerCloud ? 17 : 3;
  CAPTURE(largerCloud);
  for(usize permutation = 0; permutation < 4; ++permutation)
  {
    CAPTURE(permutation);
    DataStructure data;
    auto* geom = VertexGeom::Create(data, "Combined");
    REQUIRE(geom != nullptr);
    geom->setUnits(IGeometry::LengthUnit::Micrometer);
    auto* vertices = UnitTest::CreateTestDataArray<float32>(data, "Vertices", {tupleCount}, {3}, geom->getId());
    std::vector<usize> pointIds(tupleCount);
    for(usize i = 0; i < tupleCount; ++i)
    {
      const usize input = permutation < 2 ? (i + permutation) % tupleCount : (tupleCount - 1 - i + permutation) % tupleCount;
      const usize point = input % uniqueCount;
      pointIds[i] = point;
      (*vertices)[3 * i] = 0.5f + static_cast<float32>(point % columns);
      (*vertices)[3 * i + 1] = 0.5f + static_cast<float32>(point / columns);
      (*vertices)[3 * i + 2] = 0.5f;
    }
    geom->setVertices(*vertices);
    const auto sorted = MeshingUtilities::OrderSharedVertices(*geom, std::atomic_bool{false});
    CHECK(MeshingUtilities::HasDuplicateVertices(vertices->getDataStoreRef(), sorted));
    IdentifyDuplicateVerticesFilter filter;
    Arguments args;
    args.insertOrAssign(IdentifyDuplicateVerticesFilter::k_InputGeomPath_Key, std::make_any<DataPath>(DataPath({"Combined"})));
    args.insertOrAssign(IdentifyDuplicateVerticesFilter::k_DuplicateMaskPath_Key, std::make_any<DataPath>(DataPath({"Duplicates"})));
    auto result = filter.execute(data, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    const auto& mask = data.getDataRefAs<UInt8Array>(DataPath({"Duplicates"}));
    // Each grid permutation lists all nine coordinates before their second copies.
    const std::array<uint8, 18> expectedGridMask = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    std::vector<usize> flagsPerPoint(uniqueCount);
    usize flags = 0;
    std::vector<bool> seen(uniqueCount, false);
    for(usize i = 0; i < tupleCount; ++i)
    {
      CAPTURE(i);
      CHECK(mask[i] == (seen[pointIds[i]] ? 1 : 0));
      if(!largerCloud)
      {
        CHECK(mask[i] == expectedGridMask[i]);
      }
      seen[pointIds[i]] = true;
      if(mask[i] != 0)
      {
        ++flags;
        ++flagsPerPoint[pointIds[i]];
      }
    }
    CHECK(flags == tupleCount - uniqueCount);
    for(usize point = 0; point < uniqueCount; ++point)
    {
      CAPTURE(point);
      CHECK(flagsPerPoint[point] == (point < tupleCount - uniqueCount ? 1 : 0));
    }
    UnitTest::CheckArraysInheritTupleDims(data);
  }
}

TEST_CASE("SimplnxCore::IdentifyDuplicateVerticesFilter: Exact equality", "[SimplnxCore][IdentifyDuplicateVerticesFilter][L19]")
{
  UnitTest::LoadPlugins();
  const float32 nan = std::numeric_limits<float32>::quiet_NaN();
  const float32 inf = std::numeric_limits<float32>::infinity();
  const float32 next = std::nextafter(0.5f, 1.0f);
  // Neighbouring floats differ; signed zeros match; NaNs never match.
  const std::array<std::array<float32, 3>, 12> points = {
      {{0.5f, 0, 0}, {next, 0, 0}, {-0.0f, 2, 0}, {0.0f, 2, 0}, {nan, 0, 0}, {nan, 0, 0}, {0.5f, 0, 0}, {inf, 3, 0}, {inf, 3, 0}, {-inf, 3, 0}, {0.5f, 0, 0}, {10, 0, 0}}};
  const std::array<uint8, 12> expected = {0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};
  for(usize axis = 0; axis < 3; ++axis)
  {
    CAPTURE(axis);
    DataStructure data;
    auto* geom = VertexGeom::Create(data, "Combined");
    REQUIRE(geom != nullptr);
    auto* vertices = UnitTest::CreateTestDataArray<float32>(data, "Vertices", {points.size()}, {3}, geom->getId());
    for(usize i = 0; i < points.size(); ++i)
    {
      for(usize c = 0; c < 3; ++c)
      {
        (*vertices)[3 * i + (c + axis) % 3] = points[i][c];
      }
    }
    geom->setVertices(*vertices);
    const auto sorted = MeshingUtilities::OrderSharedVertices(*geom, std::atomic_bool{false});
    CHECK(MeshingUtilities::HasDuplicateVertices(vertices->getDataStoreRef(), sorted));
    // NaNs sort after numbers on their axis; ties retain input index order.
    const std::array<usize, 12> expectedOrderX = {9, 2, 3, 0, 6, 10, 1, 11, 7, 8, 4, 5};
    if(axis == 0)
    {
      CHECK(sorted.ordering == std::vector<IGeometry::MeshIndexType>(expectedOrderX.begin(), expectedOrderX.end()));
    }
    IdentifyDuplicateVerticesFilter filter;
    Arguments args;
    args.insertOrAssign(IdentifyDuplicateVerticesFilter::k_InputGeomPath_Key, std::make_any<DataPath>(DataPath({"Combined"})));
    args.insertOrAssign(IdentifyDuplicateVerticesFilter::k_DuplicateMaskPath_Key, std::make_any<DataPath>(DataPath({"Duplicates"})));
    auto result = filter.execute(data, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    const auto& mask = data.getDataRefAs<UInt8Array>(DataPath({"Duplicates"}));
    for(usize i = 0; i < expected.size(); ++i)
    {
      CAPTURE(i);
      CHECK(mask[i] == expected[i]);
    }
  }
}

TEST_CASE("SimplnxCore::IdentifyDuplicateVerticesFilter: Shared exact cases", "[SimplnxCore][IdentifyDuplicateVerticesFilter][L19]")
{
  UnitTest::LoadPlugins();
  DataStructure data;
  auto* vertices = UnitTest::CreateTestDataArray<float32>(data, "Vertices", {3}, {3});
  for(usize axis = 0; axis < 3; ++axis)
  {
    CAPTURE(axis);
    const auto alignment = static_cast<MeshingUtilities::AxialAlignment>(axis);
    const auto hasDuplicates = [&]() {
      const MeshingUtilities::SortedVerticesList sorted = {alignment, MeshingUtilities::OrderSharedVerticesAlongAxis(alignment, vertices->getDataStoreRef(), std::atomic_bool{false})};
      return MeshingUtilities::HasDuplicateVertices(vertices->getDataStoreRef(), sorted);
    };
    vertices->fill(0);
    (*vertices)[axis] = 0.5f;
    (*vertices)[3 + axis] = 0.5f;
    (*vertices)[3 + (axis + 1) % 3] = 1;
    (*vertices)[6 + axis] = std::nextafter(0.5f, 1.0f);
    CHECK_FALSE(hasDuplicates());
    (*vertices)[6 + axis] = 0.5f;
    CHECK(hasDuplicates());
    (*vertices)[axis] = -0.0f;
    (*vertices)[6 + axis] = +0.0f;
    CHECK(hasDuplicates());
    (*vertices)[axis] = std::numeric_limits<float32>::quiet_NaN();
    (*vertices)[6 + axis] = std::numeric_limits<float32>::quiet_NaN();
    CHECK_FALSE(hasDuplicates());
    (*vertices)[axis] = std::numeric_limits<float32>::infinity();
    (*vertices)[6 + axis] = std::numeric_limits<float32>::infinity();
    CHECK(hasDuplicates());
  }
  const MeshingUtilities::SortedVerticesList empty = {MeshingUtilities::AxialAlignment::X, {}};
  const MeshingUtilities::SortedVerticesList singleton = {MeshingUtilities::AxialAlignment::X, {0}};
  CHECK_FALSE(MeshingUtilities::HasDuplicateVertices(vertices->getDataStoreRef(), empty));
  CHECK_FALSE(MeshingUtilities::HasDuplicateVertices(vertices->getDataStoreRef(), singleton));
  auto resizeResult = vertices->resizeTuples({0});
  SIMPLNX_RESULT_REQUIRE_VALID(resizeResult);
  CHECK(MeshingUtilities::OrderSharedVerticesAlongAxis(MeshingUtilities::AxialAlignment::X, vertices->getDataStoreRef(), std::atomic_bool{false}).empty());
}

TEST_CASE("SimplnxCore::IdentifyDuplicateVerticesFilter: Cancel after sorting", "[SimplnxCore][IdentifyDuplicateVerticesFilter][L19]")
{
  DataStructure data;
  auto* geom = VertexGeom::Create(data, "Mesh");
  REQUIRE(geom != nullptr);
  auto* vertices = UnitTest::CreateTestDataArray<float32>(data, "Vertices", {2}, {3}, geom->getId());
  vertices->fill(0);
  geom->setVertices(*vertices);
  auto* mask = UnitTest::CreateTestDataArray<uint8>(data, "Mask", {2}, {1});
  mask->fill(0);
  IdentifyDuplicateVerticesInputValues inputs;
  inputs.TargetGeometryPath = DataPath({"Mesh"});
  inputs.DuplicatesMaskPath = DataPath({"Mask"});
  const std::atomic_bool cancel = true;
  const IFilter::MessageHandler handler;
  auto result = IdentifyDuplicateVertices(data, handler, cancel, &inputs)();
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK((*mask)[0] == 0);
  CHECK((*mask)[1] == 0);
}
