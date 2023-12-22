#include "SimplnxCore/Filters/AlignGeometries.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
DataStructure createTestData()
{
  DataStructure dataStructure;
  auto* movingGeom = ImageGeom::Create(dataStructure, "Moving Geometry");
  auto* targetGeom = ImageGeom::Create(dataStructure, "Target Geometry");

  SizeVec3 dimensions(5, 10, 15);
  FloatVec3 origin1(0, 0, 0);
  FloatVec3 origin2(50, 100, 60);

  movingGeom->setDimensions(dimensions);
  targetGeom->setDimensions(dimensions);
  movingGeom->setOrigin(origin1);
  targetGeom->setOrigin(origin2);

  return dataStructure;
}
} // namespace

TEST_CASE("SimplnxCore::AlignGeometries: Instantiate Filter", "[AlignGeometries]")
{
  AlignGeometries filter;
  DataStructure dataStructure = createTestData();
  Arguments args;

  DataPath movingGeomPath = DataPath({"Invalid"});
  DataPath targetGeomPath = DataPath({"Invalid"});
  uint64 alignmentType = 0;

  args.insertOrAssign(AlignGeometries::k_MovingGeometry_Key, std::make_any<DataPath>(movingGeomPath));
  args.insertOrAssign(AlignGeometries::k_TargetGeometry_Key, std::make_any<DataPath>(targetGeomPath));
  args.insertOrAssign(AlignGeometries::k_AlignmentType_Key, std::make_any<uint64>(alignmentType));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(!preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(!executeResult.result.valid());
}

TEST_CASE("SimplnxCore::AlignGeometries: Bad Alignment Type", "[AlignGeometries]")
{
  AlignGeometries filter;
  DataStructure dataStructure = createTestData();
  Arguments args;

  DataPath movingGeomPath = DataPath({"Moving Geometry"});
  DataPath targetGeomPath = DataPath({"Target Geometry"});
  uint64 alignmentType = 3;

  args.insertOrAssign(AlignGeometries::k_MovingGeometry_Key, std::make_any<DataPath>(movingGeomPath));
  args.insertOrAssign(AlignGeometries::k_TargetGeometry_Key, std::make_any<DataPath>(targetGeomPath));
  args.insertOrAssign(AlignGeometries::k_AlignmentType_Key, std::make_any<uint64>(alignmentType));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
}

TEST_CASE("SimplnxCore::AlignGeometries: Valid Arguments", "[AlignGeometries]")
{
  AlignGeometries filter;
  DataStructure dataStructure = createTestData();
  Arguments args;

  DataPath movingGeomPath = DataPath({"Moving Geometry"});
  DataPath targetGeomPath = DataPath({"Target Geometry"});
  uint64 alignmentType = 0;

  args.insertOrAssign(AlignGeometries::k_MovingGeometry_Key, std::make_any<DataPath>(movingGeomPath));
  args.insertOrAssign(AlignGeometries::k_TargetGeometry_Key, std::make_any<DataPath>(targetGeomPath));
  args.insertOrAssign(AlignGeometries::k_AlignmentType_Key, std::make_any<uint64>(alignmentType));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  auto& movingGeom = dataStructure.getDataRefAs<ImageGeom>(movingGeomPath);
  auto& targetGeom = dataStructure.getDataRefAs<ImageGeom>(targetGeomPath);

  REQUIRE(movingGeom.getOrigin() == targetGeom.getOrigin());
}
