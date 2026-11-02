#include "ContourFilterTestUtils.hpp"

#include "ImageProcessing/Filters/LabelContourImageFilter.hpp"

#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include <limits>

using namespace nx::core;

// -----------------------------------------------------------------------------
// (3) Preflight guard: the Float64 background parameter is cast to the (integer) input element type; a
//     non-finite or out-of-range value would be an undefined conversion, so the filter must reject it at
//     preflight. In-range integer values must still preflight cleanly.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::LabelContourImageFilter: Preflight rejects non-finite / out-of-range background for integer input", "[ImageProcessing][LabelContourImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = contour_test::BuildMultiLabelContourImage<uint8>(ds, 8, 8, 8); // uint8 input, range [0, 255]

  const auto makeArgs = [&](float64 bg) {
    Arguments args;
    args.insertOrAssign(LabelContourImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(LabelContourImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(LabelContourImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(LabelContourImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
    args.insertOrAssign(LabelContourImageFilter::k_BackgroundValue_Key, std::make_any<float64>(bg));
    return args;
  };

  LabelContourImageFilter filter;

  // Background out of uint8 range (255 max) -> undefined cast -> preflight error.
  REQUIRE_FALSE(filter.preflight(ds, makeArgs(99999.0)).outputActions.valid());
  // Background out of range on the negative side -> preflight error.
  REQUIRE_FALSE(filter.preflight(ds, makeArgs(-5.0)).outputActions.valid());
  // Non-finite (NaN) background -> preflight error.
  REQUIRE_FALSE(filter.preflight(ds, makeArgs(std::numeric_limits<float64>::quiet_NaN())).outputActions.valid());
  // Non-finite (Inf) background -> preflight error.
  REQUIRE_FALSE(filter.preflight(ds, makeArgs(std::numeric_limits<float64>::infinity())).outputActions.valid());
  // In-range integer value still preflights cleanly.
  auto preflightResult = filter.preflight(ds, makeArgs(0.0));
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden test (plan Task 6) -- duplicates ITKLabelContourImageTest.cpp(default) on OUR
// ITK-free filter: the output md5 must equal ITK's committed hash. Label contour is a deterministic integer
// operation (uint8 in -> uint8 out), so the hash is bit-exact. The ITK case sets no parameters, so FullyConnected
// off / background 0 are the (matching) filter defaults.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::LabelContourImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][LabelContourImageFilter]")
{
  contour_test::RunContourMd5ItkGolden<LabelContourImageFilter>("2th_cthead1.png", "d742c05a8d8aa9b41f58b8d2aad6b5d0",
                                                                contour_test::LabelContourParamSetter<LabelContourImageFilter>(/*fullyConnected=*/false, /*background=*/0.0));
}
