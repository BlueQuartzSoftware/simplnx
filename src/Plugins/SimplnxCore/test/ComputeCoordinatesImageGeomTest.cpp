#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeCoordinatesImageGeom.hpp"
#include "SimplnxCore/Filters/ComputeCoordinatesImageGeomFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <array>
#include <filesystem>
#include <memory>
namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
constexpr StringLiteral k_ImageGeomName = "DataContainer";
constexpr StringLiteral k_PhysicalArrayName = "Image Physical Coordinates";
constexpr StringLiteral k_IndexArrayName = "Image Indices";

const DataPath k_ImageGeomPath({k_ImageGeomName});
const DataPath k_PhysicalArrayPath({k_PhysicalArrayName});
const DataPath k_IndexArrayPath({k_IndexArrayName});

const DataPath k_ComputedCoordsPath({"Computed Coords"});
const DataPath k_ComputedIndicesPath({"Computed Indices"});
} // namespace

TEST_CASE("SimplnxCore::ComputeCoordinatesImageGeom: Physical", "[SimplnxCore][ComputeCoordinatesImageGeom]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "image_coords_test.tar.gz", "image_coords_test");

  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/image_coords_test/compute_coord_image_geom_test.dream3d", unit_test::k_TestFilesDir)));
  {
    // Configure the filter arguments.
    ComputeCoordinatesImageGeomFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeCoordinatesImageGeomFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeCoordinatesImageGeom::OutputType::Physical)));
    args.insertOrAssign(ComputeCoordinatesImageGeomFilter::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
    args.insertOrAssign(ComputeCoordinatesImageGeomFilter::k_CoordsArrayPath_Key, std::make_any<DataPath>(k_ComputedCoordsPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  UnitTest::CompareDataArrays<float32>(dataStructure.getDataRefAs<IDataArray>(k_ComputedCoordsPath), dataStructure.getDataRefAs<IDataArray>(k_PhysicalArrayPath));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeCoordinatesImageGeom: Indices", "[SimplnxCore][ComputeCoordinatesImageGeom]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "image_coords_test.tar.gz", "image_coords_test");

  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/image_coords_test/compute_coord_image_geom_test.dream3d", unit_test::k_TestFilesDir)));
  {
    // Configure the filter arguments.
    ComputeCoordinatesImageGeomFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeCoordinatesImageGeomFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeCoordinatesImageGeom::OutputType::Index)));
    args.insertOrAssign(ComputeCoordinatesImageGeomFilter::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
    args.insertOrAssign(ComputeCoordinatesImageGeomFilter::k_IndicesArrayPath_Key, std::make_any<DataPath>(k_ComputedIndicesPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  UnitTest::CompareDataArrays<int32>(dataStructure.getDataRefAs<IDataArray>(k_ComputedIndicesPath), dataStructure.getDataRefAs<IDataArray>(k_IndexArrayPath));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeCoordinatesImageGeom: Both", "[SimplnxCore][ComputeCoordinatesImageGeom]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "image_coords_test.tar.gz", "image_coords_test");

  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/image_coords_test/compute_coord_image_geom_test.dream3d", unit_test::k_TestFilesDir)));
  {
    // Configure the filter arguments.
    ComputeCoordinatesImageGeomFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeCoordinatesImageGeomFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeCoordinatesImageGeom::OutputType::Both)));
    args.insertOrAssign(ComputeCoordinatesImageGeomFilter::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
    args.insertOrAssign(ComputeCoordinatesImageGeomFilter::k_CoordsArrayPath_Key, std::make_any<DataPath>(k_ComputedCoordsPath));
    args.insertOrAssign(ComputeCoordinatesImageGeomFilter::k_IndicesArrayPath_Key, std::make_any<DataPath>(k_ComputedIndicesPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  UnitTest::CompareDataArrays<float32>(dataStructure.getDataRefAs<IDataArray>(k_ComputedCoordsPath), dataStructure.getDataRefAs<IDataArray>(k_PhysicalArrayPath));
  UnitTest::CompareDataArrays<int32>(dataStructure.getDataRefAs<IDataArray>(k_ComputedIndicesPath), dataStructure.getDataRefAs<IDataArray>(k_IndexArrayPath));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
