#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/ClosingByReconstructionImageFilter.hpp"

#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/StructuringElement.hpp"

#include <array>
#include <memory>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
// Legacy ITKClosingByReconstructionImageFilter, created at runtime by UUID.
const Uuid k_LegacyClosingUuid = *Uuid::FromString("b5ff32a8-e799-4f72-8d13-e2581f748562");

// Live-parity configs: 3D + genuinely-2D (Z=1) gradient/noise images, each with FullyConnected off and on. The
// element type (via TEMPLATE_TEST_CASE) and PreserveIntensities (via GENERATE) are iterated by the test.
const std::vector<rt::ReconCase> k_ParityConfigs = {
    {"3D 12x12x12 fc=false", false, 0.0, 12, 12, 12}, {"3D 12x12x12 fc=true", true, 0.0, 12, 12, 12}, {"2D 20x16x1 fc=false", false, 0.0, 20, 16, 1}, {"2D 20x16x1 fc=true", true, 0.0, 20, 16, 1}};

rt::ReconParamSetter BallSetter(bool preserveIntensities)
{
  return rt::SEReconParamSetter<ClosingByReconstructionImageFilter>(static_cast<ChoicesParameter::ValueType>(ImageProcessing::KernelType::Ball), {1, 1, 1}, preserveIntensities);
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Live-ITK EXACT parity grid across uint8 / int16 / float32 x FullyConnected {off, on} x PreserveIntensities
//     {off, on}, on a gradient/noise image with a radius-1 Ball. Closing by reconstruction is a grayscale dilation
//     followed by a geodesic reconstruction-by-erosion (and, with PreserveIntensities, a compare-select + a second
//     reconstruction) -- all copy/clamp operations -- so the new filter must reproduce the legacy ITK output
//     EXACTLY for integer AND float types (no tolerance).
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::ClosingByReconstructionImageFilter: Live-ITK exact parity grid", "[ImageProcessing][ClosingByReconstructionImageFilter]", uint8, int16, float32)
{
  using T = TestType;
  const bool preserveIntensities = GENERATE(false, true);
  CAPTURE(preserveIntensities);
  const auto build = [](DataStructure& ds, usize dx, usize dy, usize dz) { return rt::BuildImageFromPattern<T>(ds, dx, dy, dz, rt::MakeGradientNoisePattern<T>(dx, dy, dz)); };
  rt::RunReconstructionParityGrid<ClosingByReconstructionImageFilter, T>(k_LegacyClosingUuid, build, k_ParityConfigs, BallSetter(preserveIntensities));
}

// -----------------------------------------------------------------------------
// (1b) The same live-ITK EXACT parity grid but with a NON-Ball kernel at a NON-unit radius (Box, radius {2,2,1}),
//      exercising the Choices->StructuringElement path for a shape/size other than the default radius-1 Ball (the
//      shape rasterization itself is unit-tested by StructuringElementTest; this pins the filter-level plumbing).
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::ClosingByReconstructionImageFilter: Live-ITK exact parity grid (Box r{2,2,1})", "[ImageProcessing][ClosingByReconstructionImageFilter]", uint8, int16, float32)
{
  using T = TestType;
  const bool preserveIntensities = GENERATE(false, true);
  CAPTURE(preserveIntensities);
  const auto build = [](DataStructure& ds, usize dx, usize dy, usize dz) { return rt::BuildImageFromPattern<T>(ds, dx, dy, dz, rt::MakeGradientNoisePattern<T>(dx, dy, dz)); };
  const rt::ReconParamSetter boxSetter =
      rt::SEReconParamSetter<ClosingByReconstructionImageFilter>(static_cast<ChoicesParameter::ValueType>(ImageProcessing::KernelType::Box), {2, 2, 1}, preserveIntensities);
  rt::RunReconstructionParityGrid<ClosingByReconstructionImageFilter, T>(k_LegacyClosingUuid, build, k_ParityConfigs, boxSetter);
}

// -----------------------------------------------------------------------------
// (2) Hand-computed behavior on a small 2D image (dual of opening by reconstruction): closing by reconstruction
//     removes a DARK structure smaller than the structuring element (an isolated 1-voxel dark dot on a bright
//     field) but restores a dark structure larger than it (a 5x5 dark square) to its original value/contours.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ClosingByReconstructionImageFilter: hand-computed remove-dot vs restore-square", "[ImageProcessing][ClosingByReconstructionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 11;
  constexpr usize DY = 11;
  constexpr usize DZ = 1;
  constexpr int32 field = 100; // bright background
  constexpr int32 dark = 0;

  std::vector<int32> pattern(DX * DY * DZ, field);
  // A 5x5 dark square (interior) -- larger than a radius-1 Ball, so its interior survives dilation and is regrown.
  for(usize y = 3; y <= 7; ++y)
  {
    for(usize x = 3; x <= 7; ++x)
    {
      pattern[rt::FlatIndex(x, y, 0, DX, DY)] = dark;
    }
  }
  // An isolated 1-voxel dark dot -- smaller than the structuring element, so dilation fills it with no seed to
  // reconstruct from.
  pattern[rt::FlatIndex(9, 1, 0, DX, DY)] = dark;

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);

  ClosingByReconstructionImageFilter filter;
  rt::RunReconstructionFilter<ClosingByReconstructionImageFilter>(filter, ds, inputPath, BallSetter(/*preserveIntensities=*/false), /*fullyConnected=*/false, /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();

  REQUIRE(outStore.getValue(rt::FlatIndex(9, 1, 0, DX, DY)) == field); // isolated dark dot removed (filled to field)
  REQUIRE(outStore.getValue(rt::FlatIndex(5, 5, 0, DX, DY)) == dark);  // dark square center preserved
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, DX, DY)) == dark);  // dark square corner regrown
  REQUIRE(outStore.getValue(rt::FlatIndex(9, 9, 0, DX, DY)) == field); // untouched bright field
}

// -----------------------------------------------------------------------------
// (2b) Both-algorithm-paths correctness on the hand-computed remove-dot/restore-square image (dual of opening).
//      Closing by reconstruction routes its dilate + geodesic reconstruction-by-erosion through ApplyMorphology's
//      and ApplyMorphologicalReconstruction's DispatchAlgorithm, so AlgorithmTestScope runs the filter under BOTH
//      the in-core and out-of-core algorithm paths on in-memory stores (selected by SIMPLNX_TEST_ALGORITHM_PATH).
//      The hand-derived expectations (isolated dark dot filled, dark square restored) are asserted directly -- an
//      independent oracle, not the new filter's own in-core run.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ClosingByReconstructionImageFilter: hand-computed remove-dot vs restore-square (both algorithm paths)", "[ImageProcessing][ClosingByReconstructionImageFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths on in-memory stores; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  constexpr usize DX = 11, DY = 11, DZ = 1;
  constexpr int32 field = 100; // bright background
  constexpr int32 dark = 0;
  std::vector<int32> pattern(DX * DY * DZ, field);
  for(usize y = 3; y <= 7; ++y)
  {
    for(usize x = 3; x <= 7; ++x)
    {
      pattern[rt::FlatIndex(x, y, 0, DX, DY)] = dark; // 5x5 dark square -- larger than the SE, regrown exactly
    }
  }
  pattern[rt::FlatIndex(9, 1, 0, DX, DY)] = dark; // isolated 1-voxel dark dot -- smaller than the SE, filled

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));

  ClosingByReconstructionImageFilter filter;
  Arguments args;
  args.insertOrAssign(ClosingByReconstructionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
  args.insertOrAssign(ClosingByReconstructionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(ClosingByReconstructionImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  BallSetter(/*preserveIntensities=*/false)(args, /*fullyConnected=*/false, /*height=*/0.0);

  auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = ds.getDataRefAs<DataArray<int32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  REQUIRE(outStore.getValue(rt::FlatIndex(9, 1, 0, DX, DY)) == field); // isolated dark dot filled to field
  REQUIRE(outStore.getValue(rt::FlatIndex(5, 5, 0, DX, DY)) == dark);  // dark square center preserved
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, DX, DY)) == dark);  // dark square corner regrown
  REQUIRE(outStore.getValue(rt::FlatIndex(9, 9, 0, DX, DY)) == field); // untouched bright field
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("ImageProcessing::ClosingByReconstructionImageFilter: FromSIMPLJson", "[ImageProcessing][ClosingByReconstructionImageFilter]")
{
  const nlohmann::json json = {{"KernelType", 2},
                               {"KernelRadius", {{"x", 2}, {"y", 3}, {"z", 4}}},
                               {"FullyConnected", true},
                               {"PreserveIntensities", true},
                               {"SelectedCellArrayPath", {{"Data Container Name", "Image Geometry"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "Input"}}},
                               {"NewCellArrayName", "Closed"}};

  Result<Arguments> result = ClosingByReconstructionImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments& args = result.value();
  REQUIRE(args.value<uint64>(ClosingByReconstructionImageFilter::k_KernelType_Key) == 2);
  REQUIRE(args.value<std::vector<uint32>>(ClosingByReconstructionImageFilter::k_KernelRadius_Key) == std::vector<uint32>{2, 3, 4});
  REQUIRE(args.value<bool>(ClosingByReconstructionImageFilter::k_FullyConnected_Key) == true);
  REQUIRE(args.value<bool>(ClosingByReconstructionImageFilter::k_PreserveIntensities_Key) == true);
  REQUIRE(args.value<DataPath>(ClosingByReconstructionImageFilter::k_InputImageGeomPath_Key) == DataPath({"Image Geometry"}));
  REQUIRE(args.value<DataPath>(ClosingByReconstructionImageFilter::k_InputImageDataPath_Key) == DataPath({"Image Geometry", "CellData", "Input"}));
  REQUIRE(args.value<std::string>(ClosingByReconstructionImageFilter::k_OutputImageArrayName_Key) == "Closed");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden -- duplicates ITKClosingByReconstructionImageTest.cpp(ClosingByReconstruction):
// STAPLE1.png (uint8), Ball radius {1,1,1}. (B) live-ITK bit-exact + (A) md5-validity-first. The committed hash
// 095f00a6... is the identity-output hash (this closing-by-reconstruction leaves STAPLE1 unchanged).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ClosingByReconstructionImageFilter: ITK real-image golden (ClosingByReconstruction)", "[ImageProcessing][ItkGolden][ClosingByReconstructionImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<ClosingByReconstructionImageFilter>("STAPLE1.png", "095f00a68a84df4396914fa758f34dcc", [](Arguments& args) {
    args.insertOrAssign(ClosingByReconstructionImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(ImageProcessing::KernelType::Ball)));
    args.insertOrAssign(ClosingByReconstructionImageFilter::k_KernelRadius_Key, std::make_any<std::vector<uint32>>(std::vector<uint32>{1, 1, 1}));
  });
}
