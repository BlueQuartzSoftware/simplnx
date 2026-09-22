#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/GrayscaleFillholeImageFilter.hpp"

#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <array>
#include <limits>
#include <memory>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

// -----------------------------------------------------------------------------
// (2) Hand-computed behavior on a small 2D image: an isolated interior dark hole is filled to the surrounding
//     bright level, while a dark region that touches the image border (including its non-border voxel, which is
//     connected to the border through the dark region) is left unchanged.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleFillholeImageFilter: hand-computed interior fill vs border region", "[ImageProcessing][GrayscaleFillholeImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 7;
  constexpr usize DY = 7;
  constexpr usize DZ = 1;
  constexpr int32 field = 100;
  constexpr int32 hole = 0;

  std::vector<int32> pattern(DX * DY * DZ, field);
  // Isolated interior hole (all four face-neighbors are bright field) -> a regional minimum not connected to the
  // border -> must be FILLED to the surrounding level (100).
  pattern[rt::FlatIndex(3, 3, 0, DX, DY)] = hole;
  // A dark 2x2 region anchored at the (0,0) corner. Voxels (0,0),(1,0),(0,1) are on the 2D border; (1,1) is a
  // non-border voxel but is connected to the border through the dark region -> the whole region is NOT filled.
  pattern[rt::FlatIndex(0, 0, 0, DX, DY)] = hole;
  pattern[rt::FlatIndex(1, 0, 0, DX, DY)] = hole;
  pattern[rt::FlatIndex(0, 1, 0, DX, DY)] = hole;
  pattern[rt::FlatIndex(1, 1, 0, DX, DY)] = hole;

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);

  GrayscaleFillholeImageFilter filter;
  rt::RunReconstructionFilter<GrayscaleFillholeImageFilter>(filter, ds, inputPath, rt::FullyConnectedParamSetter<GrayscaleFillholeImageFilter>(), /*fullyConnected=*/false, /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();

  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, DX, DY)) == field); // interior hole filled
  REQUIRE(outStore.getValue(rt::FlatIndex(0, 0, 0, DX, DY)) == hole);  // border voxel of the border region
  REQUIRE(outStore.getValue(rt::FlatIndex(1, 1, 0, DX, DY)) == hole);  // non-border voxel of the border region, still not filled
  REQUIRE(outStore.getValue(rt::FlatIndex(5, 5, 0, DX, DY)) == field); // untouched field voxel
}

// -----------------------------------------------------------------------------
// (2b) Both-algorithm-paths correctness on the hand-computed interior-fill/border-region image. Fillhole is a
//      reconstruction-by-erosion routed through ApplyMorphologicalReconstruction's DispatchAlgorithm, so
//      AlgorithmTestScope runs the filter under BOTH the in-core and out-of-core algorithm paths on in-memory stores
//      (selected by SIMPLNX_TEST_ALGORITHM_PATH). The hand-derived expectations (isolated interior hole filled,
//      border-connected dark region unchanged) are asserted directly -- an independent oracle, not the new filter's
//      own in-core run.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleFillholeImageFilter: hand-computed interior fill vs border region (both algorithm paths)", "[ImageProcessing][GrayscaleFillholeImageFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths on in-memory stores; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  constexpr usize DX = 7, DY = 7, DZ = 1;
  constexpr int32 field = 100;
  constexpr int32 hole = 0;
  std::vector<int32> pattern(DX * DY * DZ, field);
  pattern[rt::FlatIndex(3, 3, 0, DX, DY)] = hole; // isolated interior hole -> filled
  // A dark 2x2 region anchored at the (0,0) corner: connected to the border through the dark region -> NOT filled.
  pattern[rt::FlatIndex(0, 0, 0, DX, DY)] = hole;
  pattern[rt::FlatIndex(1, 0, 0, DX, DY)] = hole;
  pattern[rt::FlatIndex(0, 1, 0, DX, DY)] = hole;
  pattern[rt::FlatIndex(1, 1, 0, DX, DY)] = hole;

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));

  GrayscaleFillholeImageFilter filter;
  Arguments args;
  args.insertOrAssign(GrayscaleFillholeImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
  args.insertOrAssign(GrayscaleFillholeImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(GrayscaleFillholeImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  rt::FullyConnectedParamSetter<GrayscaleFillholeImageFilter>()(args, /*fullyConnected=*/false, /*height=*/0.0);

  auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = ds.getDataRefAs<DataArray<int32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, DX, DY)) == field); // interior hole filled
  REQUIRE(outStore.getValue(rt::FlatIndex(0, 0, 0, DX, DY)) == hole);  // border voxel of the border region
  REQUIRE(outStore.getValue(rt::FlatIndex(1, 1, 0, DX, DY)) == hole);  // non-border voxel of the border region, still not filled
  REQUIRE(outStore.getValue(rt::FlatIndex(5, 5, 0, DX, DY)) == field); // untouched field voxel
  UnitTest::CheckArraysInheritTupleDims(ds);
}

// -----------------------------------------------------------------------------
// (4) Preflight: a multi-component (non-scalar) input is rejected (the reconstruction operator is defined on
//     scalar images), and a valid scalar input preflights cleanly.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleFillholeImageFilter: Preflight scalar guard", "[ImageProcessing][GrayscaleFillholeImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  GrayscaleFillholeImageFilter filter;

  auto makeArgs = [](const DataPath& inputPath) {
    Arguments args;
    args.insertOrAssign(GrayscaleFillholeImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(GrayscaleFillholeImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(GrayscaleFillholeImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(GrayscaleFillholeImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
    return args;
  };

  SECTION("scalar input preflights cleanly")
  {
    DataStructure ds;
    const DataPath inputPath = rt::BuildHoleImage<uint8>(ds, 8, 8, 8, uint8{200}, uint8{40});
    auto result = filter.preflight(ds, makeArgs(inputPath));
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }

  SECTION("multi-component input is rejected")
  {
    DataStructure ds;
    const DataPath scalarPath = rt::BuildHoleImage<uint8>(ds, 8, 8, 8, uint8{200}, uint8{40}); // creates the geometry + CellData
    const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
    auto vecStore = DataStoreUtilities::CreateDataStore<uint8>(ds, vecPath, {8, 8, 8}, {2});
    const auto& cellAM = ds.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
    DataArray<uint8>::Create(ds, "Vec", vecStore, cellAM.getId());

    auto result = filter.preflight(ds, makeArgs(vecPath));
    REQUIRE(result.outputActions.invalid());
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
}

// -----------------------------------------------------------------------------
// (5) FromSIMPLJson maps the legacy SIMPL keys (FullyConnected + array paths) to the new parameter keys.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleFillholeImageFilter: FromSIMPLJson", "[ImageProcessing][GrayscaleFillholeImageFilter]")
{
  const nlohmann::json json = {{"FullyConnected", true},
                               {"SelectedCellArrayPath", {{"Data Container Name", "Image Geometry"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "Input"}}},
                               {"NewCellArrayName", "Filled"}};

  Result<Arguments> result = GrayscaleFillholeImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments& args = result.value();
  REQUIRE(args.value<bool>(GrayscaleFillholeImageFilter::k_FullyConnected_Key) == true);
  REQUIRE(args.value<std::string>(GrayscaleFillholeImageFilter::k_OutputImageArrayName_Key) == "Filled");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden -- duplicates ITKGrayscaleFillholeImageTest.cpp exactly: read the ITK input
// through OUR ITK-free reader, run OUR filter with ITK-default params (FullyConnected off), then (B) live-ITK
// bit-exact + (A) md5-validity-first vs the committed hash. GrayscaleFillhole1 = RA-Short.nrrd (int16),
// GrayscaleFillhole2 = RA-Slice-Short.png (int16). See ReconstructionFilterTestUtils.hpp for the shared body.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleFillholeImageFilter: ITK real-image golden (GrayscaleFillhole1)", "[ImageProcessing][ItkGolden][GrayscaleFillholeImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<GrayscaleFillholeImageFilter>("RA-Short.nrrd", "e2c49e979bd4c64f0efff67b196b1950");
}

TEST_CASE("ImageProcessing::GrayscaleFillholeImageFilter: ITK real-image golden (GrayscaleFillhole2)", "[ImageProcessing][ItkGolden][GrayscaleFillholeImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<GrayscaleFillholeImageFilter>("RA-Slice-Short.png", "e3cd61348a7824d191e83632bf92baae");
}
