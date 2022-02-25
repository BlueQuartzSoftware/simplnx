#include <catch2/catch.hpp>

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/AlignGeometries.hpp"

using namespace complex;
using namespace complex::Constants;

namespace
{
DataStructure createTestData()
{
  DataStructure dataGraph;
  auto* movingGeom = ImageGeom::Create(dataGraph, "Moving Geometry");
  auto* targetGeom = ImageGeom::Create(dataGraph, "Target Geometry");

  SizeVec3 dimensions(5, 10, 15);
  FloatVec3 origin1(0, 0, 0);
  FloatVec3 origin2(50, 100, 60);

  movingGeom->setDimensions(dimensions);
  targetGeom->setDimensions(dimensions);
  movingGeom->setOrigin(origin1);
  targetGeom->setOrigin(origin2);

  return dataGraph;
}
} // namespace

TEST_CASE("AlignGeometries: Instantiate Filter", "[AlignGeometries]")
{
  AlignGeometries filter;
  DataStructure dataGraph = createTestData();
  Arguments args;

  DataPath movingGeomPath = DataPath({"Invalid"});
  DataPath targetGeomPath = DataPath({"Invalid"});
  uint64 alignmentType = 0;

  args.insertOrAssign(AlignGeometries::k_MovingGeometry_Key, std::make_any<DataPath>(movingGeomPath));
  args.insertOrAssign(AlignGeometries::k_TargetGeometry_Key, std::make_any<DataPath>(targetGeomPath));
  args.insertOrAssign(AlignGeometries::k_AlignmentType_Key, std::make_any<uint64>(alignmentType));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(!preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(!executeResult.result.valid());
}

TEST_CASE("AlignGeometries: Bad Alignment Type", "[AlignGeometries]")
{
  AlignGeometries filter;
  DataStructure dataGraph = createTestData();
  Arguments args;

  DataPath movingGeomPath = DataPath({"Moving Geometry"});
  DataPath targetGeomPath = DataPath({"Target Geometry"});
  uint64 alignmentType = 3;

  args.insertOrAssign(AlignGeometries::k_MovingGeometry_Key, std::make_any<DataPath>(movingGeomPath));
  args.insertOrAssign(AlignGeometries::k_TargetGeometry_Key, std::make_any<DataPath>(targetGeomPath));
  args.insertOrAssign(AlignGeometries::k_AlignmentType_Key, std::make_any<uint64>(alignmentType));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(!preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(!executeResult.result.valid());
}

TEST_CASE("AlignGeometries: Valid Arguments", "[AlignGeometries]")
{
  AlignGeometries filter;
  DataStructure dataGraph = createTestData();
  Arguments args;

  DataPath movingGeomPath = DataPath({"Moving Geometry"});
  DataPath targetGeomPath = DataPath({"Target Geometry"});
  uint64 alignmentType = 0;

  args.insertOrAssign(AlignGeometries::k_MovingGeometry_Key, std::make_any<DataPath>(movingGeomPath));
  args.insertOrAssign(AlignGeometries::k_TargetGeometry_Key, std::make_any<DataPath>(targetGeomPath));
  args.insertOrAssign(AlignGeometries::k_AlignmentType_Key, std::make_any<uint64>(alignmentType));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid());

  auto& movingGeom = dataGraph.getDataRefAs<ImageGeom>(movingGeomPath);
  auto& targetGeom = dataGraph.getDataRefAs<ImageGeom>(targetGeomPath);

  REQUIRE(movingGeom.getOrigin() == targetGeom.getOrigin());
}
