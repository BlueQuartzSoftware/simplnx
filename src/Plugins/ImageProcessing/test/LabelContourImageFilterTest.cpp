#include "ContourFilterTestUtils.hpp"

#include "ImageProcessing/Filters/LabelContourImageFilter.hpp"

#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include <limits>

using namespace nx::core;

namespace
{
// Legacy ITKLabelContourImageFilter, created at runtime by UUID (so this target does not link ITKImageProcessing).
const Uuid k_LegacyLabelContourUuid = *Uuid::FromString("b64ff45d-3661-4926-9e87-5dd7b379b261");
} // namespace

// -----------------------------------------------------------------------------
// (1) Legacy parity grid across FullyConnected in {false, true} on a MULTI-LABEL image (labels {0,1,2,3},
//     background = 0): several distinct regions share faces (adjacent different labels), labeled regions touch
//     the image edge (exercising the "out-of-bounds neighbors ignored" border rule), and the background region
//     is present (a background voxel is never a contour). On any integer input the new filter must reproduce the
//     legacy ITK Label Contour output EXACTLY. The parity grid also covers a 2D (Z=1) shape. Storage is forced
//     in-core so the legacy ITK filter can run.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::LabelContourImageFilter: Legacy parity grid (multi-label)", "[ImageProcessing][LabelContourImageFilter]")
{
  const auto build = [](DataStructure& ds, usize dx, usize dy, usize dz) { return contour_test::BuildMultiLabelContourImage<int32>(ds, dx, dy, dz); };

  SECTION("FullyConnected=false")
  {
    contour_test::RunContourParityGrid<LabelContourImageFilter, int32>(k_LegacyLabelContourUuid, build, contour_test::LabelContourParamSetter<LabelContourImageFilter>(false, 0.0));
  }
  SECTION("FullyConnected=true")
  {
    contour_test::RunContourParityGrid<LabelContourImageFilter, int32>(k_LegacyLabelContourUuid, build, contour_test::LabelContourParamSetter<LabelContourImageFilter>(true, 0.0));
  }
}

// -----------------------------------------------------------------------------
// (1b) Legacy parity with a NON-ZERO background value (background = 2): confirms that the chosen background
//      label is never a contour while every OTHER value (0, 1, 3) is a labeled region whose borders survive.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::LabelContourImageFilter: Legacy parity (non-zero background)", "[ImageProcessing][LabelContourImageFilter]")
{
  const auto build = [](DataStructure& ds, usize dx, usize dy, usize dz) { return contour_test::BuildMultiLabelContourImage<int32>(ds, dx, dy, dz); };
  contour_test::RunContourParityGrid<LabelContourImageFilter, int32>(k_LegacyLabelContourUuid, build, contour_test::LabelContourParamSetter<LabelContourImageFilter>(false, 2.0));
}

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
// ITK-free filter: (A) output md5 == ITK's committed hash + (B) live-ITK bit-exact parity. Label contour is a
// deterministic integer operation (uint8 in -> uint8 out), so both oracles are bit-exact. The ITK case sets no
// parameters, so FullyConnected off / background 0 are the (matching) filter defaults.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::LabelContourImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][LabelContourImageFilter]")
{
  contour_test::RunContourMd5ItkGolden<LabelContourImageFilter>("2th_cthead1.png", "d742c05a8d8aa9b41f58b8d2aad6b5d0",
                                                                contour_test::LabelContourParamSetter<LabelContourImageFilter>(/*fullyConnected=*/false, /*background=*/0.0));
}
