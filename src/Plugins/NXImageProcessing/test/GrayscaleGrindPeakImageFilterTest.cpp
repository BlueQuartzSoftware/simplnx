#include "ReconstructionFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/GrayscaleGrindPeakImageFilter.hpp"

#include "simplnx/Filter/IParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <array>
#include <limits>
#include <memory>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

// -----------------------------------------------------------------------------
// (2) Hand-computed behavior on a small 2D image: an isolated interior bright peak is ground down to the
//     surrounding dark level, while a bright region that touches the image border (including its non-border voxel,
//     which is connected to the border through the bright region) is left unchanged.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::GrayscaleGrindPeakImageFilter: hand-computed interior grind vs border region", "[NXImageProcessing][GrayscaleGrindPeakImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_Dx = 7;
  constexpr usize k_Dy = 7;
  constexpr usize k_Dz = 1;
  constexpr int32 k_Field = 50;
  constexpr int32 k_Peak = 90;

  std::vector<int32> pattern(k_Dx * k_Dy * k_Dz, k_Field);
  // Isolated interior peak (all four face-neighbors are dark field) -> a regional maximum not connected to the
  // border -> must be GROUND DOWN to the surrounding level (50).
  pattern[rt::FlatIndex(3, 3, 0, k_Dx, k_Dy)] = k_Peak;
  // A bright 2x2 region anchored at the (0,0) corner. Voxels (0,0),(1,0),(0,1) are on the 2D border; (1,1) is a
  // non-border voxel but is connected to the border through the bright region -> the whole region is NOT ground.
  pattern[rt::FlatIndex(0, 0, 0, k_Dx, k_Dy)] = k_Peak;
  pattern[rt::FlatIndex(1, 0, 0, k_Dx, k_Dy)] = k_Peak;
  pattern[rt::FlatIndex(0, 1, 0, k_Dx, k_Dy)] = k_Peak;
  pattern[rt::FlatIndex(1, 1, 0, k_Dx, k_Dy)] = k_Peak;

  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_Dx, k_Dy, k_Dz, pattern);

  GrayscaleGrindPeakImageFilter filter;
  rt::RunReconstructionFilter<GrayscaleGrindPeakImageFilter>(filter, dataStructure, inputPath, rt::FullyConnectedParamSetter<GrayscaleGrindPeakImageFilter>(), /*fullyConnected=*/false,
                                                             /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = dataStructure.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();

  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, k_Dx, k_Dy)) == k_Field); // interior peak ground down
  REQUIRE(outStore.getValue(rt::FlatIndex(0, 0, 0, k_Dx, k_Dy)) == k_Peak);  // border voxel of the border region
  REQUIRE(outStore.getValue(rt::FlatIndex(1, 1, 0, k_Dx, k_Dy)) == k_Peak);  // non-border voxel of the border region, still not ground
  REQUIRE(outStore.getValue(rt::FlatIndex(5, 5, 0, k_Dx, k_Dy)) == k_Field); // untouched field voxel
}

// -----------------------------------------------------------------------------
// (2b) Both-algorithm-paths correctness on the hand-computed interior-grind/border-region image. GrindPeak is a
//      reconstruction-by-dilation routed through ApplyMorphologicalReconstruction's DispatchAlgorithm, so
//      AlgorithmTestScope runs the filter under BOTH the in-core and out-of-core algorithm paths on in-memory stores
//      (selected by SIMPLNX_TEST_ALGORITHM_PATH). The hand-derived expectations (isolated interior peak ground down,
//      border-connected bright region unchanged) are asserted directly -- an independent oracle, not the new filter's
//      own in-core run.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::GrayscaleGrindPeakImageFilter: hand-computed interior grind vs border region (both algorithm paths)", "[NXImageProcessing][GrayscaleGrindPeakImageFilter]")
{
  unit_test::LoadPlugins();
  // Test both algorithm paths on in-memory stores; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  constexpr usize k_Dx = 7;
  constexpr usize k_Dy = 7;
  constexpr usize k_Dz = 1;
  constexpr int32 k_Field = 50;
  constexpr int32 k_Peak = 90;
  std::vector<int32> pattern(k_Dx * k_Dy * k_Dz, k_Field);
  pattern[rt::FlatIndex(3, 3, 0, k_Dx, k_Dy)] = k_Peak; // isolated interior peak -> ground down
  // A bright 2x2 region anchored at the (0,0) corner: connected to the border through the bright region -> NOT ground.
  pattern[rt::FlatIndex(0, 0, 0, k_Dx, k_Dy)] = k_Peak;
  pattern[rt::FlatIndex(1, 0, 0, k_Dx, k_Dy)] = k_Peak;
  pattern[rt::FlatIndex(0, 1, 0, k_Dx, k_Dy)] = k_Peak;
  pattern[rt::FlatIndex(1, 1, 0, k_Dx, k_Dy)] = k_Peak;

  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_Dx, k_Dy, k_Dz, pattern);
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));

  GrayscaleGrindPeakImageFilter filter;
  Arguments args;
  args.insertOrAssign(GrayscaleGrindPeakImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
  args.insertOrAssign(GrayscaleGrindPeakImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(GrayscaleGrindPeakImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  rt::FullyConnectedParamSetter<GrayscaleGrindPeakImageFilter>()(args, /*fullyConnected=*/false, /*height=*/0.0);

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = dataStructure.getDataRefAs<DataArray<int32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, k_Dx, k_Dy)) == k_Field); // interior peak ground down
  REQUIRE(outStore.getValue(rt::FlatIndex(0, 0, 0, k_Dx, k_Dy)) == k_Peak);  // border voxel of the border region
  REQUIRE(outStore.getValue(rt::FlatIndex(1, 1, 0, k_Dx, k_Dy)) == k_Peak);  // non-border voxel of the border region, still not ground
  REQUIRE(outStore.getValue(rt::FlatIndex(5, 5, 0, k_Dx, k_Dy)) == k_Field); // untouched field voxel
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
// (4) Preflight: a multi-component (non-scalar) input is rejected (the reconstruction operator is defined on
//     scalar images), and a valid scalar input preflights cleanly.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::GrayscaleGrindPeakImageFilter: Preflight scalar guard", "[NXImageProcessing][GrayscaleGrindPeakImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  GrayscaleGrindPeakImageFilter filter;

  auto makeArgs = [](const DataPath& inputPath) {
    Arguments args;
    args.insertOrAssign(GrayscaleGrindPeakImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(GrayscaleGrindPeakImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(GrayscaleGrindPeakImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(GrayscaleGrindPeakImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
    return args;
  };

  SECTION("scalar input preflights cleanly")
  {
    DataStructure dataStructure;
    const DataPath inputPath = rt::BuildPeakImage<uint8>(dataStructure, 8, 8, 8, uint8{40}, uint8{220});
    auto result = filter.preflight(dataStructure, makeArgs(inputPath));
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }

  SECTION("multi-component input is rejected")
  {
    DataStructure dataStructure;
    const DataPath scalarPath = rt::BuildPeakImage<uint8>(dataStructure, 8, 8, 8, uint8{40}, uint8{220}); // creates the geometry + CellData
    const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
    auto vecStore = data_store_utilities::CreateDataStore<uint8>(dataStructure, vecPath, {8, 8, 8}, {2});
    const auto& cellAM = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
    DataArray<uint8>::Create(dataStructure, "Vec", vecStore, cellAM.getId());

    auto result = filter.preflight(dataStructure, makeArgs(vecPath));
    REQUIRE(result.output_actions.invalid());
    REQUIRE(result.output_actions.errors().front().code == filter_parameter::constants::k_ValidateTupleShapeValue);
  }
}

// -----------------------------------------------------------------------------
// (5) FromSIMPLJson maps the legacy SIMPL keys (FullyConnected + array paths) to the new parameter keys.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::GrayscaleGrindPeakImageFilter: FromSIMPLJson", "[NXImageProcessing][GrayscaleGrindPeakImageFilter]")
{
  const nlohmann::json json = {{"FullyConnected", true},
                               {"SelectedCellArrayPath", {{"Data Container Name", "Image Geometry"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "Input"}}},
                               {"NewCellArrayName", "Ground"}};

  Result<Arguments> result = GrayscaleGrindPeakImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments& args = result.value();
  REQUIRE(args.value<bool>(GrayscaleGrindPeakImageFilter::k_FullyConnected_Key) == true);
  REQUIRE(args.value<std::string>(GrayscaleGrindPeakImageFilter::k_OutputImageArrayName_Key) == "Ground");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden -- duplicates ITKGrayscaleGrindPeakImageTest.cpp exactly (ITK-default params):
// (B) live-ITK bit-exact + (A) md5-validity-first. GrayscaleGrindPeak1 = RA-Short.nrrd, GrayscaleGrindPeak2 =
// RA-Slice-Short.png.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::GrayscaleGrindPeakImageFilter: ITK real-image golden (GrayscaleGrindPeak1)", "[NXImageProcessing][ItkGolden][GrayscaleGrindPeakImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<GrayscaleGrindPeakImageFilter>("RA-Short.nrrd", "084cdd1d64664ebfab26c2e0ed382e14");
}

TEST_CASE("NXImageProcessing::GrayscaleGrindPeakImageFilter: ITK real-image golden (GrayscaleGrindPeak2)", "[NXImageProcessing][ItkGolden][GrayscaleGrindPeakImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<GrayscaleGrindPeakImageFilter>("RA-Slice-Short.png", "b18d75cccb9361c65b40bb5c0d3c6e0d");
}
