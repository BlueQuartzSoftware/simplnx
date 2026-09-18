#include "ContourFilterTestUtils.hpp"

#include "ImageProcessing/Filters/BinaryContourImageFilter.hpp"

#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include <limits>

using namespace nx::core;

namespace
{
// Legacy ITKBinaryContourImageFilter, created at runtime by UUID (so this target does not link ITKImageProcessing).
const Uuid k_LegacyBinaryContourUuid = *Uuid::FromString("ed214e76-6954-49b4-817b-13f92315e722");
} // namespace

// -----------------------------------------------------------------------------
// (1) Legacy parity grid across FullyConnected in {false, true} on a BINARY input (interior blocks that reduce
//     to background + block borders that become the foreground contour + blocks touching the image edge that
//     exercise the "out-of-bounds neighbors ignored" border rule), plus a 2D (Z=1) shape. On any integer input
//     the new filter must reproduce the legacy ITK Binary Contour output EXACTLY. Storage is forced in-core so
//     the legacy ITK filter can run.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryContourImageFilter: Legacy parity grid (binary input)", "[ImageProcessing][BinaryContourImageFilter]")
{
  const auto build = [](DataStructure& ds, usize dx, usize dy, usize dz) { return contour_test::BuildBinaryContourImage<uint8>(ds, dx, dy, dz, uint8{1}, uint8{0}); };

  SECTION("FullyConnected=false")
  {
    contour_test::RunContourParityGrid<BinaryContourImageFilter, uint8>(k_LegacyBinaryContourUuid, build, contour_test::BinaryContourParamSetter<BinaryContourImageFilter>(false, 1.0, 0.0));
  }
  SECTION("FullyConnected=true")
  {
    contour_test::RunContourParityGrid<BinaryContourImageFilter, uint8>(k_LegacyBinaryContourUuid, build, contour_test::BinaryContourParamSetter<BinaryContourImageFilter>(true, 1.0, 0.0));
  }
}

// -----------------------------------------------------------------------------
// (1b) Legacy parity on a MULTI-LABEL input (labels {0,1,2,3}, foreground = 1). This confirms the pinned
//      passthrough semantics against live ITK: foreground objects are reduced to their contour while every
//      non-foreground label (0, 2, 3) is written to the output UNCHANGED. There is no binary-input safeguard
//      (unlike binary morphology), so a labeled input runs cleanly and matches ITK exactly.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryContourImageFilter: Legacy parity grid (multi-label passthrough)", "[ImageProcessing][BinaryContourImageFilter]")
{
  const auto build = [](DataStructure& ds, usize dx, usize dy, usize dz) { return contour_test::BuildMultiLabelContourImage<int16>(ds, dx, dy, dz); };

  SECTION("FullyConnected=false")
  {
    contour_test::RunContourParityGrid<BinaryContourImageFilter, int16>(k_LegacyBinaryContourUuid, build, contour_test::BinaryContourParamSetter<BinaryContourImageFilter>(false, 1.0, 0.0));
  }
  SECTION("FullyConnected=true")
  {
    contour_test::RunContourParityGrid<BinaryContourImageFilter, int16>(k_LegacyBinaryContourUuid, build, contour_test::BinaryContourParamSetter<BinaryContourImageFilter>(true, 1.0, 0.0));
  }
}

// -----------------------------------------------------------------------------
// (3) Preflight guard: the Float64 foreground/background parameters are cast to the (integer) input element
//     type; a non-finite or out-of-range value would be an undefined conversion, so the filter must reject it
//     at preflight. In-range integer values must still preflight cleanly.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryContourImageFilter: Preflight rejects non-finite / out-of-range fg/bg for integer input", "[ImageProcessing][BinaryContourImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = contour_test::BuildBinaryContourImage<uint8>(ds, 8, 8, 8, uint8{1}, uint8{0}); // uint8 input, range [0, 255]

  const auto makeArgs = [&](float64 fg, float64 bg) {
    Arguments args;
    args.insertOrAssign(BinaryContourImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(BinaryContourImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(BinaryContourImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(BinaryContourImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
    args.insertOrAssign(BinaryContourImageFilter::k_ForegroundValue_Key, std::make_any<float64>(fg));
    args.insertOrAssign(BinaryContourImageFilter::k_BackgroundValue_Key, std::make_any<float64>(bg));
    return args;
  };

  BinaryContourImageFilter filter;

  // Foreground out of uint8 range (255 max) -> undefined cast -> preflight error.
  REQUIRE_FALSE(filter.preflight(ds, makeArgs(99999.0, 0.0)).outputActions.valid());
  // Background out of range on the negative side -> preflight error.
  REQUIRE_FALSE(filter.preflight(ds, makeArgs(1.0, -5.0)).outputActions.valid());
  // Non-finite (NaN) foreground -> preflight error.
  REQUIRE_FALSE(filter.preflight(ds, makeArgs(std::numeric_limits<float64>::quiet_NaN(), 0.0)).outputActions.valid());
  // Non-finite (Inf) background -> preflight error.
  REQUIRE_FALSE(filter.preflight(ds, makeArgs(1.0, std::numeric_limits<float64>::infinity())).outputActions.valid());
  // In-range integer values still preflight cleanly.
  auto preflightResult = filter.preflight(ds, makeArgs(1.0, 0.0));
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden tests (plan Task 6) -- duplicate each ITKBinaryContourImageTest.cpp case on OUR
// ITK-free filter: (A) output md5 == ITK's committed hash + (B) live-ITK bit-exact parity. Binary contour is a
// deterministic integer operation (uint8 in -> uint8 out), so both oracles are bit-exact.
//   - "default": WhiteDots.png, foreground 255 (FullyConnected off, background 0 -- filter defaults).
//   - "custom": 2th_cthead1.png, foreground 100, FullyConnected on.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryContourImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][BinaryContourImageFilter]")
{
  contour_test::RunContourMd5ItkGolden<BinaryContourImageFilter>("WhiteDots.png", "3921141f21fcb41e6d4af197e48ffbb5",
                                                                 contour_test::BinaryContourParamSetter<BinaryContourImageFilter>(/*fullyConnected=*/false, /*foreground=*/255.0, /*background=*/0.0));
}
TEST_CASE("ImageProcessing::BinaryContourImageFilter: ITK real-image golden (custom)", "[ImageProcessing][ItkGolden][BinaryContourImageFilter]")
{
  contour_test::RunContourMd5ItkGolden<BinaryContourImageFilter>("2th_cthead1.png", "09212e4d204a0ed90a445dc832047b22",
                                                                 contour_test::BinaryContourParamSetter<BinaryContourImageFilter>(/*fullyConnected=*/true, /*foreground=*/100.0, /*background=*/0.0));
}
