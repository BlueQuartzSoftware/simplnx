#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/BinaryOpeningByReconstructionImageFilter.hpp"

#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/StructuringElement.hpp"

#include <array>
#include <memory>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
// Legacy ITKBinaryOpeningByReconstructionImageFilter, created at runtime by UUID.
const Uuid k_LegacyBinaryOpeningUuid = *Uuid::FromString("02c15392-382c-406d-a174-07ea6fa11b67");

// Live-parity configs: 3D + genuinely-2D (Z=1), each with FullyConnected off and on. The element type (via
// TEMPLATE_TEST_CASE) and the fg/bg pairing (via GENERATE) are iterated by the test.
const std::vector<rt::ReconCase> k_ParityConfigs = {
    {"3D 12x12x12 fc=false", false, 0.0, 12, 12, 12}, {"3D 12x12x12 fc=true", true, 0.0, 12, 12, 12}, {"2D 20x16x1 fc=false", false, 0.0, 20, 16, 1}, {"2D 20x16x1 fc=true", true, 0.0, 20, 16, 1}};

struct FgBg
{
  float64 fg;
  float64 bg;
};
} // namespace

// -----------------------------------------------------------------------------
// (1) Live-ITK EXACT parity grid across uint8 / int16 / int32 x BOTH fg/bg orderings (fg>bg AND fg<bg, exercising
//     the Dilation AND Erosion reduction paths) x FullyConnected {off, on} x 3D + 2D, on a strictly-binary image
//     (foreground blobs on a background field). Binary opening by reconstruction is a binary erode + a binary
//     reconstruction-by-dilation -- all copy/clamp operations -- so the new filter must reproduce the legacy ITK
//     output EXACTLY (no tolerance).
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::BinaryOpeningByReconstructionImageFilter: Live-ITK exact parity grid", "[ImageProcessing][BinaryOpeningByReconstructionImageFilter]", uint8, int16, int32)
{
  using T = TestType;
  const FgBg fgbg = GENERATE(FgBg{1.0, 0.0}, FgBg{0.0, 255.0});
  CAPTURE(fgbg.fg, fgbg.bg);
  // MakeHolePattern paints 'hole'-valued blobs on a 'field' background with exactly two values -> a strictly-binary
  // image when field=bg and hole=fg.
  const auto build = [fgbg](DataStructure& ds, usize dx, usize dy, usize dz) {
    return rt::BuildImageFromPattern<T>(ds, dx, dy, dz, rt::MakeHolePattern<T>(dx, dy, dz, static_cast<T>(fgbg.bg), static_cast<T>(fgbg.fg)));
  };
  const rt::ReconParamSetter setter =
      rt::BinaryReconParamSetter<BinaryOpeningByReconstructionImageFilter>(static_cast<ChoicesParameter::ValueType>(ImageProcessing::KernelType::Ball), {1, 1, 1}, fgbg.fg, fgbg.bg);
  rt::RunReconstructionParityGrid<BinaryOpeningByReconstructionImageFilter, T>(k_LegacyBinaryOpeningUuid, build, k_ParityConfigs, setter);
}

// -----------------------------------------------------------------------------
// (1b) The same live-ITK EXACT parity grid but with a NON-Ball kernel at a NON-unit radius (Box, radius {2,2,1}),
//      exercising the Choices->StructuringElement path for a shape/size other than the default radius-1 Ball (the
//      shape rasterization itself is unit-tested by StructuringElementTest; this pins the filter-level plumbing).
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::BinaryOpeningByReconstructionImageFilter: Live-ITK exact parity grid (Box r{2,2,1})", "[ImageProcessing][BinaryOpeningByReconstructionImageFilter]", uint8, int16,
                   int32)
{
  using T = TestType;
  const FgBg fgbg = GENERATE(FgBg{1.0, 0.0}, FgBg{0.0, 255.0});
  CAPTURE(fgbg.fg, fgbg.bg);
  const auto build = [fgbg](DataStructure& ds, usize dx, usize dy, usize dz) {
    return rt::BuildImageFromPattern<T>(ds, dx, dy, dz, rt::MakeHolePattern<T>(dx, dy, dz, static_cast<T>(fgbg.bg), static_cast<T>(fgbg.fg)));
  };
  const rt::ReconParamSetter setter =
      rt::BinaryReconParamSetter<BinaryOpeningByReconstructionImageFilter>(static_cast<ChoicesParameter::ValueType>(ImageProcessing::KernelType::Box), {2, 2, 1}, fgbg.fg, fgbg.bg);
  rt::RunReconstructionParityGrid<BinaryOpeningByReconstructionImageFilter, T>(k_LegacyBinaryOpeningUuid, build, k_ParityConfigs, setter);
}

// -----------------------------------------------------------------------------
// (2) Hand-computed behavior on a small 2D binary image (fg=1, bg=0): binary opening by reconstruction removes a
//     foreground object smaller than the structuring element (an isolated 1-voxel dot) but fully restores a
//     foreground object larger than it (a 5x5 square). A plain binary opening would round the square's corners;
//     reconstruction regrows the whole connected component.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryOpeningByReconstructionImageFilter: hand-computed remove-dot vs restore-square", "[ImageProcessing][BinaryOpeningByReconstructionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 11;
  constexpr usize DY = 11;
  constexpr usize DZ = 1;
  constexpr int32 bg = 0;
  constexpr int32 fg = 1;

  std::vector<int32> pattern(DX * DY * DZ, bg);
  // A 5x5 foreground square (interior) -- larger than a radius-1 Ball, so it survives erosion and is regrown.
  for(usize y = 3; y <= 7; ++y)
  {
    for(usize x = 3; x <= 7; ++x)
    {
      pattern[rt::FlatIndex(x, y, 0, DX, DY)] = fg;
    }
  }
  // An isolated 1-voxel foreground dot -- smaller than the structuring element, so erosion removes it with no seed.
  pattern[rt::FlatIndex(9, 1, 0, DX, DY)] = fg;

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);

  BinaryOpeningByReconstructionImageFilter filter;
  rt::RunReconstructionFilter<BinaryOpeningByReconstructionImageFilter>(
      filter, ds, inputPath, rt::BinaryReconParamSetter<BinaryOpeningByReconstructionImageFilter>(static_cast<ChoicesParameter::ValueType>(ImageProcessing::KernelType::Ball), {1, 1, 1}, 1.0, 0.0),
      /*fullyConnected=*/false, /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();

  REQUIRE(outStore.getValue(rt::FlatIndex(9, 1, 0, DX, DY)) == bg); // isolated dot removed
  REQUIRE(outStore.getValue(rt::FlatIndex(5, 5, 0, DX, DY)) == fg); // square center preserved
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, DX, DY)) == fg); // square corner regrown (a plain opening would round it off)
  REQUIRE(outStore.getValue(rt::FlatIndex(9, 9, 0, DX, DY)) == bg); // untouched background
}

// -----------------------------------------------------------------------------
// (2b) Both-algorithm-paths correctness on the hand-computed remove-dot/restore-square binary image (fg=1, bg=0).
//      Binary opening by reconstruction routes its binary erode + reconstruction-by-dilation through
//      ApplyMorphologicalReconstruction's DispatchAlgorithm, so AlgorithmTestScope runs the filter under BOTH the
//      in-core and out-of-core algorithm paths on in-memory stores (selected by SIMPLNX_TEST_ALGORITHM_PATH). The
//      hand-derived expectations (isolated dot removed, square fully restored) are asserted directly -- an
//      independent oracle, not the new filter's own in-core run.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryOpeningByReconstructionImageFilter: hand-computed remove-dot vs restore-square (both algorithm paths)", "[ImageProcessing][BinaryOpeningByReconstructionImageFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths on in-memory stores; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  constexpr usize DX = 11, DY = 11, DZ = 1;
  constexpr int32 bg = 0;
  constexpr int32 fg = 1;
  std::vector<int32> pattern(DX * DY * DZ, bg);
  for(usize y = 3; y <= 7; ++y)
  {
    for(usize x = 3; x <= 7; ++x)
    {
      pattern[rt::FlatIndex(x, y, 0, DX, DY)] = fg; // 5x5 foreground square -- larger than the SE, regrown exactly
    }
  }
  pattern[rt::FlatIndex(9, 1, 0, DX, DY)] = fg; // isolated 1-voxel foreground dot -- smaller than the SE, removed

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));

  BinaryOpeningByReconstructionImageFilter filter;
  Arguments args;
  args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
  args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const rt::ReconParamSetter setter =
      rt::BinaryReconParamSetter<BinaryOpeningByReconstructionImageFilter>(static_cast<ChoicesParameter::ValueType>(ImageProcessing::KernelType::Ball), {1, 1, 1}, 1.0, 0.0);
  setter(args, /*fullyConnected=*/false, /*height=*/0.0);

  auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = ds.getDataRefAs<DataArray<int32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  REQUIRE(outStore.getValue(rt::FlatIndex(9, 1, 0, DX, DY)) == bg); // isolated dot removed
  REQUIRE(outStore.getValue(rt::FlatIndex(5, 5, 0, DX, DY)) == fg); // square center preserved
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, DX, DY)) == fg); // square corner regrown
  REQUIRE(outStore.getValue(rt::FlatIndex(9, 9, 0, DX, DY)) == bg); // untouched background
  UnitTest::CheckArraysInheritTupleDims(ds);
}

// -----------------------------------------------------------------------------
// (2c) Annulus (center-EXCLUDING) kernel exercises the clamp-to-mask binary-reconstruction path that Ball/Box never
//      reach. With a center-excluding SE the binary erode can mark a fully-enclosed BACKGROUND hole as foreground (a
//      "stray marker" whose value exceeds the mask there), technically violating reconstruction-by-dilation's
//      marker<=mask precondition; the engine's clampToMask intersects the stray marker back to the mask on the first
//      pass (utilities ~1941-1945). The legacy SimpleITK Annulus kernel is EMPTY (a wrapper bug), so live parity is
//      not the oracle here; instead the two GUARANTEED consequences of a correct clamp are asserted directly:
//      opening-by-reconstruction is anti-extensive (output foreground subset of input foreground), and the enclosed
//      background hole stays background (the stray marker did NOT leak into the output). A 7x7 solid foreground block
//      is large enough that interior voxels far from the hole survive the Annulus erode as seeds, so the block's
//      connected component is regrown -- proving the reconstruction still runs, not that it produced an empty image.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryOpeningByReconstructionImageFilter: Annulus clamp-to-mask (enclosed hole stays background)", "[ImageProcessing][BinaryOpeningByReconstructionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 11, DY = 11, DZ = 1;
  constexpr int32 bg = 0;
  constexpr int32 fg = 1;
  std::vector<int32> pattern(DX * DY * DZ, bg);
  for(usize y = 2; y <= 8; ++y) // 7x7 solid foreground block (strictly binary: only bg/fg)
  {
    for(usize x = 2; x <= 8; ++x)
    {
      pattern[rt::FlatIndex(x, y, 0, DX, DY)] = fg;
    }
  }
  pattern[rt::FlatIndex(5, 5, 0, DX, DY)] = bg; // single fully-enclosed background hole -> Annulus erode marks it fg (stray marker)

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);

  BinaryOpeningByReconstructionImageFilter filter;
  rt::RunReconstructionFilter<BinaryOpeningByReconstructionImageFilter>(
      filter, ds, inputPath, rt::BinaryReconParamSetter<BinaryOpeningByReconstructionImageFilter>(static_cast<ChoicesParameter::ValueType>(ImageProcessing::KernelType::Annulus), {1, 1, 1}, 1.0, 0.0),
      /*fullyConnected=*/false, /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();

  // The stray-marker hole (clamped back to the mask) must stay background -- the exact behavior the clamp-to-mask
  // branch guarantees.
  REQUIRE(outStore.getValue(rt::FlatIndex(5, 5, 0, DX, DY)) == bg);
  // The surrounding block's connected component is restored (interior seeds survived the Annulus erode), so the
  // reconstruction genuinely ran rather than collapsing to all-background.
  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, DX, DY)) == fg);
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, DX, DY)) == fg);

  // Output is strictly binary AND anti-extensive (every output-foreground voxel was input-foreground). A clamp
  // failure would leak a stray marker into background, violating one of these.
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    const int32 outValue = outStore.getValue(i);
    INFO("clamp-to-mask index=" << i << " out=" << outValue << " in=" << pattern[i]);
    REQUIRE((outValue == bg || outValue == fg));
    if(outValue == fg)
    {
      REQUIRE(pattern[i] == fg);
    }
  }
}

// -----------------------------------------------------------------------------
// (2d) Preflight type guard: a float32 input is rejected. NOTE: the -8000 k_UnsupportedDataType branch inside
//      PreflightImageFilter<IntegerOnly> is a DEFENSIVE backstop that filter.preflight() never reaches -- the input
//      ArraySelectionParameter admits only integer scalar types (GetIntegerScalarTypes), so the parameter-validation
//      layer rejects a float32 array first (k_Validate_AllowedType_Error). Either way a non-integer input cannot be
//      processed; this test pins that observable contract at the filter level (mirrors RelabelComponent's float-input
//      reject, which uses the same IntegerOnly policy).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryOpeningByReconstructionImageFilter: preflight rejects float32 input", "[ImageProcessing][BinaryOpeningByReconstructionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, 8, 8, 8, std::vector<float32>(8 * 8 * 8, 1.0f));

  BinaryOpeningByReconstructionImageFilter filter;
  Arguments args;
  args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_ForegroundValue_Key, std::make_any<float64>(1.0));
  args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
  const auto result = filter.preflight(ds, args);
  REQUIRE(result.outputActions.invalid()); // float32 is not an IntegerOnly type
}

TEST_CASE("ImageProcessing::BinaryOpeningByReconstructionImageFilter: FromSIMPLJson", "[ImageProcessing][BinaryOpeningByReconstructionImageFilter]")
{
  const nlohmann::json json = {{"KernelType", 2},
                               {"KernelRadius", {{"x", 2}, {"y", 3}, {"z", 4}}},
                               {"ForegroundValue", 5.0},
                               {"BackgroundValue", 2.0},
                               {"FullyConnected", true},
                               {"SelectedCellArrayPath", {{"Data Container Name", "Image Geometry"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "Input"}}},
                               {"NewCellArrayName", "BinOpen"}};

  Result<Arguments> result = BinaryOpeningByReconstructionImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments& args = result.value();
  REQUIRE(args.value<uint64>(BinaryOpeningByReconstructionImageFilter::k_KernelType_Key) == 2);
  REQUIRE(args.value<std::vector<uint32>>(BinaryOpeningByReconstructionImageFilter::k_KernelRadius_Key) == std::vector<uint32>{2, 3, 4});
  REQUIRE(args.value<float64>(BinaryOpeningByReconstructionImageFilter::k_ForegroundValue_Key) == 5.0);
  REQUIRE(args.value<float64>(BinaryOpeningByReconstructionImageFilter::k_BackgroundValue_Key) == 2.0);
  REQUIRE(args.value<bool>(BinaryOpeningByReconstructionImageFilter::k_FullyConnected_Key) == true);
  REQUIRE(args.value<DataPath>(BinaryOpeningByReconstructionImageFilter::k_InputImageGeomPath_Key) == DataPath({"Image Geometry"}));
  REQUIRE(args.value<DataPath>(BinaryOpeningByReconstructionImageFilter::k_InputImageDataPath_Key) == DataPath({"Image Geometry", "CellData", "Input"}));
  REQUIRE(args.value<std::string>(BinaryOpeningByReconstructionImageFilter::k_OutputImageArrayName_Key) == "BinOpen");
}

// -----------------------------------------------------------------------------
// Preflight fg/bg range guard: BinaryOpening is the only Tier-H filter with a
// ValidateBinaryFgBgInRange guard. For an integer input it must reject a non-finite
// or out-of-range Foreground/Background Value and warn (not error) on a fractional
// value that will be truncated. Error/warning codes kept in sync with
// BinaryOpeningByReconstructionImageFilter.cpp.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryOpeningByReconstructionImageFilter: preflight fg/bg range guard", "[ImageProcessing][BinaryOpeningByReconstructionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr int32 k_NonFiniteBinaryValue = -8570;
  constexpr int32 k_BinaryValueOutOfRange = -8571;
  constexpr int32 k_BinaryValueTruncated = -8572;
  constexpr int32 k_ForegroundEqualsBackground = -8579;

  // uint8 input so the IntegerOnly + scalar preflight passes; the guard then validates fg/bg against [0, 255].
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<uint8>(ds, 8, 8, 8, rt::MakePlateauPattern<uint8>(8, 8, 8));

  auto makeArgs = [&](float64 fg, float64 bg) {
    Arguments args;
    args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_ForegroundValue_Key, std::make_any<float64>(fg));
    args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_BackgroundValue_Key, std::make_any<float64>(bg));
    return args;
  };

  BinaryOpeningByReconstructionImageFilter filter;

  SECTION("valid fg/bg preflights cleanly")
  {
    auto result = filter.preflight(ds, makeArgs(1.0, 0.0));
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }

  SECTION("non-finite fg is rejected")
  {
    auto result = filter.preflight(ds, makeArgs(std::numeric_limits<float64>::infinity(), 0.0));
    REQUIRE(result.outputActions.invalid());
    REQUIRE(result.outputActions.errors().front().code == k_NonFiniteBinaryValue);
  }

  SECTION("out-of-range fg (negative, invalid for uint8) is rejected")
  {
    auto result = filter.preflight(ds, makeArgs(-1.0, 0.0));
    REQUIRE(result.outputActions.invalid());
    REQUIRE(result.outputActions.errors().front().code == k_BinaryValueOutOfRange);
  }

  SECTION("out-of-range bg (above uint8 max) is rejected")
  {
    auto result = filter.preflight(ds, makeArgs(1.0, 300.0));
    REQUIRE(result.outputActions.invalid());
    REQUIRE(result.outputActions.errors().front().code == k_BinaryValueOutOfRange);
  }

  SECTION("fractional fg emits a truncation warning but preflights valid")
  {
    auto result = filter.preflight(ds, makeArgs(0.5, 0.0));
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
    REQUIRE(!result.outputActions.warnings().empty());
    REQUIRE(result.outputActions.warnings().front().code == k_BinaryValueTruncated);
  }

  SECTION("fg == bg is rejected")
  {
    auto result = filter.preflight(ds, makeArgs(1.0, 1.0));
    REQUIRE(result.outputActions.invalid());
    REQUIRE(result.outputActions.errors().front().code == k_ForegroundEqualsBackground);
  }
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- ITKBinaryOpeningByReconstructionImageTest.cpp(BinaryOpeningByReconstruction) runs
// on 2th_cthead1.png with ForegroundValue 200, Ball r{5,1,1}, committed md5 2dff38c9c5d2f516e7435f3e2291d6c1.
//
// CONTRACT DIVERGENCE (documented, NOT a port bug -- the ITK golden is intentionally NOT reproduced here):
// 2th_cthead1.png is a THREE-level thresholding (values {0, 100, 200}); it is not strictly binary. ITK's binary
// morphology binarizes-at-ForegroundValue (every voxel != 200 is silently treated as background, so the 100-valued
// class is folded into background). OUR BinaryOpeningByReconstructionImageFilter enforces a stricter, safer contract:
// it REQUIRES the input to contain only the {foreground, background} pair and errors (-8002) on any third value,
// rather than silently reinterpreting it. So the ITK md5 cannot be reproduced on this input. We instead assert the
// contract explicitly: preflight passes (the third value is only detectable by a data scan at execute) and execute
// rejects the input with the binary-input error. Re-enable an ITK-golden reproduction only if this filter's
// binarize-at-foreground contract is ever changed to match ITK's silent fold-to-background behavior.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryOpeningByReconstructionImageFilter: ITK real-image golden (BinaryOpeningByReconstruction) -- documented contract divergence",
          "[ImageProcessing][ItkGolden][BinaryOpeningByReconstructionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath input = geom.createChildPath("CellData").createChildPath("Input");

  DataStructure ds;
  const Result<> readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath("2th_cthead1.png"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  Arguments args;
  args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_KernelType_Key,
                      std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(ImageProcessing::KernelType::Ball)));
  args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_KernelRadius_Key, std::make_any<std::vector<uint32>>(std::vector<uint32>{5, 1, 1}));
  args.insertOrAssign(BinaryOpeningByReconstructionImageFilter::k_ForegroundValue_Key, std::make_any<float64>(200.0));

  BinaryOpeningByReconstructionImageFilter filter;
  const auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.invalid());                      // non-strictly-binary input (value 100) is rejected
  REQUIRE(executeResult.result.errors().front().code == -8002); // the documented binary-input contract error
}
