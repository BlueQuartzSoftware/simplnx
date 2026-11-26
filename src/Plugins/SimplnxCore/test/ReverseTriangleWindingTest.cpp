#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/ReverseTriangleWindingFilter.hpp"

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
const DataPath k_TriangleGeomPath = DataPath({Constants::k_DataContainer});
}

TEST_CASE("SimplnxCore::ReverseTriangleWindingFilter: Valid Filter Execution", "[SimplnxCore][ReverseTriangleWindingFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "reverse_triangle_winding.tar.gz", "reverse_triangle_winding");

  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/reverse_triangle_winding/6_6_exemplar_triangle_winding.dream3d", unit_test::k_TestFilesDir)));

  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/reverse_triangle_winding/6_6_reverse_triangle_winding.dream3d", unit_test::k_TestFilesDir)));
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ReverseTriangleWindingFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ReverseTriangleWindingFilter::k_TriGeomPath_Key, std::make_any<DataPath>(k_TriangleGeomPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  auto& exemplarTriGeom = exemplarDataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);
  auto& generatedTriGeom = exemplarDataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);

  UnitTest::CompareDataArrays<TriangleGeom::MeshIndexType>(exemplarTriGeom.getFacesRef(), generatedTriGeom.getFacesRef());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ReverseTriangleWindingFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ReverseTriangleWindingFilter][BackwardsCompatibility]")
{
  const auto version = GENERATE("6_4", "6_5");
  CAPTURE(version);
  const fs::path fixturePath = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion" / version / "ReverseTriangleWindingFilter.json";
  const auto args = UnitTest::LoadSIMPLConversionArguments(fixturePath, FilterTraits<ReverseTriangleWindingFilter>::uuid);
  CHECK(args.value<DataPath>(ReverseTriangleWindingFilter::k_TriGeomPath_Key) == DataPath({"DataContainer"}));
}
