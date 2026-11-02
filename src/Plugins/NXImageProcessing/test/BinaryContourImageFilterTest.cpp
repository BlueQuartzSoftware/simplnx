#include "ContourFilterTestUtils.hpp"

#include "ImageProcessing/Filters/BinaryContourImageFilter.hpp"

#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include <limits>

using namespace nx::core;

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
