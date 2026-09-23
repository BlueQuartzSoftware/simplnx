#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/GradientAnisotropicDiffusionImageFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <catch2/catch.hpp>

#include <string>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
using GradientAnisoFilter = GradientAnisotropicDiffusionImageFilter;

// Sets the geometry, input, and output keys plus TimeStep, ConductanceParameter, ConductanceScalingUpdateInterval,
// and NumberOfIterations on the Arguments. The helper runs preflight and execute, and requires that both steps
// succeed.
void RunGradientAniso(IFilter& filter, DataStructure& ds, const DataPath& inputPath, float64 timeStep, float64 conductanceParameter, uint32 conductanceScalingUpdateInterval, uint32 numberOfIterations,
                      const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(GradientAnisoFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(GradientAnisoFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(GradientAnisoFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(GradientAnisoFilter::k_TimeStep_Key, std::make_any<float64>(timeStep));
  args.insertOrAssign(GradientAnisoFilter::k_ConductanceParameter_Key, std::make_any<float64>(conductanceParameter));
  args.insertOrAssign(GradientAnisoFilter::k_ConductanceScalingUpdateInterval_Key, std::make_any<uint32>(conductanceScalingUpdateInterval));
  args.insertOrAssign(GradientAnisoFilter::k_NumberOfIterations_Key, std::make_any<uint32>(numberOfIterations));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

// A deterministic non-constant pattern with values in [0, 16], varying along all three axes so the gradient (and
// the conductance term it feeds) is non-trivial.
template <class T>
std::vector<T> MakeRamp(usize dx, usize dy, usize dz)
{
  std::vector<T> v(dx * dy * dz);
  for(usize z = 0; z < dz; ++z)
  {
    for(usize y = 0; y < dy; ++y)
    {
      for(usize x = 0; x < dx; ++x)
      {
        v[rt::FlatIndex(x, y, z, dx, dy)] = static_cast<T>((3 * x + 2 * y + z) % 17);
      }
    }
  }
  return v;
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Preflight guards: an INTEGER input is rejected (FloatingScalar admits only float32/float64, unlike
//     CurvatureFlow's AllNumeric); a multi-component input is rejected (requireScalar); a valid float input
//     produces a SameAsInput output array; and a TimeStep exceeding the CFL stability bound is VALID but carries a
//     warning (matching ITK's own non-fatal InitializeIteration check).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GradientAnisotropicDiffusionImageFilter: preflight guards", "[ImageProcessing][GradientAnisotropicDiffusionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  SECTION("integer input rejected")
  {
    // Rejected by the ArraySelectionParameter's own allowed-type check (GetFloatingScalarTypes() in parameters())
    // BEFORE preflightImpl/PreflightImageFilter ever runs -- so this asserts rejection, not a specific error code
    // (unlike the multi-component case below, which int32 with 1 component sails past that parameter check and is
    // rejected inside PreflightImageFilter itself, where the code IS stable).
    DataStructure ds;
    const std::vector<int32> field = MakeRamp<int32>(6, 6, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, 6, 6, 1, field);
    GradientAnisoFilter filter;
    Arguments args;
    args.insertOrAssign(GradientAnisoFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
    args.insertOrAssign(GradientAnisoFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(GradientAnisoFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(GradientAnisoFilter::k_TimeStep_Key, std::make_any<float64>(0.0625));
    args.insertOrAssign(GradientAnisoFilter::k_ConductanceParameter_Key, std::make_any<float64>(3.0));
    args.insertOrAssign(GradientAnisoFilter::k_ConductanceScalingUpdateInterval_Key, std::make_any<uint32>(1u));
    args.insertOrAssign(GradientAnisoFilter::k_NumberOfIterations_Key, std::make_any<uint32>(5u));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  }
  SECTION("multi-component rejected")
  {
    DataStructure ds;
    const DataPath scalarPath = rt::BuildImageFromPattern<float32>(ds, 6, 6, 1, std::vector<float32>(36, 0.0f));
    const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
    auto vecStore = DataStoreUtilities::CreateDataStore<float32>(ds, vecPath, {1, 6, 6}, {3});
    const auto& cellAM = ds.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
    DataArray<float32>::Create(ds, "Vec", vecStore, cellAM.getId());
    const DataPath geomPath = vecPath.getParent().getParent();
    GradientAnisoFilter filter;
    Arguments args;
    args.insertOrAssign(GradientAnisoFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(GradientAnisoFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
    args.insertOrAssign(GradientAnisoFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(GradientAnisoFilter::k_TimeStep_Key, std::make_any<float64>(0.0625));
    args.insertOrAssign(GradientAnisoFilter::k_ConductanceParameter_Key, std::make_any<float64>(3.0));
    args.insertOrAssign(GradientAnisoFilter::k_ConductanceScalingUpdateInterval_Key, std::make_any<uint32>(1u));
    args.insertOrAssign(GradientAnisoFilter::k_NumberOfIterations_Key, std::make_any<uint32>(5u));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("scalar float input creates a SameAsInput output")
  {
    DataStructure ds;
    const std::vector<float32> field = MakeRamp<float32>(6, 6, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, 6, 6, 1, field);
    GradientAnisoFilter filter;
    RunGradientAniso(filter, ds, inputPath, 0.0625, 3.0, 1u, 5u);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::float32);
  }
  SECTION("over-large TimeStep is VALID but carries a stability warning")
  {
    // effDim==2 (Z==1), unit spacing -> stable bound = 1/2^3 = 0.125. 0.5 is well above that.
    DataStructure ds;
    const std::vector<float32> field = MakeRamp<float32>(6, 6, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, 6, 6, 1, field);
    GradientAnisoFilter filter;
    Arguments args;
    args.insertOrAssign(GradientAnisoFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
    args.insertOrAssign(GradientAnisoFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(GradientAnisoFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(GradientAnisoFilter::k_TimeStep_Key, std::make_any<float64>(0.5));
    args.insertOrAssign(GradientAnisoFilter::k_ConductanceParameter_Key, std::make_any<float64>(3.0));
    args.insertOrAssign(GradientAnisoFilter::k_ConductanceScalingUpdateInterval_Key, std::make_any<uint32>(1u));
    args.insertOrAssign(GradientAnisoFilter::k_NumberOfIterations_Key, std::make_any<uint32>(5u));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
    REQUIRE_FALSE(result.outputActions.warnings().empty());
    REQUIRE(result.outputActions.warnings().front().code == nx::core::ImageProcessing::k_AnisotropicDiffusionUnstableTimeStep);
  }
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- duplicates ITKGradientAnisotropicDiffusionImageTest.cpp on OUR ITK-free filter
// (RA-Float.nrrd -> float32, SameAsInput). The durable golden is a committed baseline at a tolerance of 0.0, which is
// a bit-exact compare via ip_golden::CompareImages. The helper pins ForceInCore.
//   defaults: the ITK "defaults" case sets TimeStep 0.01 (not literally all-default); Conductance/Interval/Iterations
//             stay at the shared filter defaults (3.0 / 1 / 5).
//   longer:   TimeStep 0.01, NumberOfIterations 10.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GradientAnisotropicDiffusionImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][GradientAnisotropicDiffusionImageFilter]")
{
  rt::RunDistanceMapItkGoldenBaseline<GradientAnisotropicDiffusionImageFilter>(
      "RA-Float.nrrd", "BasicFilters_GradientAnisotropicDiffusionImageFilter_defaults.nrrd", /*tolerance=*/0.0,
      [](Arguments& args) { args.insertOrAssign(GradientAnisotropicDiffusionImageFilter::k_TimeStep_Key, std::make_any<float64>(0.01)); });
}

TEST_CASE("ImageProcessing::GradientAnisotropicDiffusionImageFilter: ITK real-image golden (longer)", "[ImageProcessing][ItkGolden][GradientAnisotropicDiffusionImageFilter]")
{
  rt::RunDistanceMapItkGoldenBaseline<GradientAnisotropicDiffusionImageFilter>("RA-Float.nrrd", "BasicFilters_GradientAnisotropicDiffusionImageFilter_longer.nrrd", /*tolerance=*/0.0,
                                                                               [](Arguments& args) {
                                                                                 args.insertOrAssign(GradientAnisotropicDiffusionImageFilter::k_TimeStep_Key, std::make_any<float64>(0.01));
                                                                                 args.insertOrAssign(GradientAnisotropicDiffusionImageFilter::k_NumberOfIterations_Key, std::make_any<uint32>(10u));
                                                                               });
}

// -----------------------------------------------------------------------------
// (2) A spatially-constant (uniform) image is an IDENTITY: its average squared gradient magnitude is 0, so the global
//     reduction sets m_K = 0, and GradientAnisoFn's `if(m_K != 0.0)` conductance branch is skipped (Cx == Cxd == 0);
//     the per-axis delta is therefore 0 at every voxel and accum never changes -> output == input EXACTLY.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GradientAnisotropicDiffusionImageFilter: constant image is identity", "[ImageProcessing][GradientAnisotropicDiffusionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 8, DY = 8, DZ = 8;
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, DX, DY, DZ, std::vector<float32>(DX * DY * DZ, 7.0f));
  GradientAnisoFilter filter;
  RunGradientAniso(filter, ds, inputPath, /*timeStep=*/0.0625, /*conductanceParameter=*/3.0, /*conductanceScalingUpdateInterval=*/1u, /*numberOfIterations=*/5u);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  rt::RequireExact<float32>(ds.getDataRefAs<IDataArray>(outputPath), ds.getDataRefAs<IDataArray>(inputPath));
}

// -----------------------------------------------------------------------------
// (3) The CFL stability WARNING for a genuinely 3D image (effDim == 3): the existing preflight-guards test only trips
//     the effDim==2 (Z==1) arm, so the ImageDimension==3 stable-bound branch of AppendUnstableTimeStepWarning was never
//     asserted. A 6x6x6 image at unit spacing has stable bound 1/2^(3+1) = 0.0625; TimeStep 0.5 is well above it, so
//     preflight stays VALID but carries the shared unstable-time-step warning, whose message names ImageDimension=3.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GradientAnisotropicDiffusionImageFilter: over-large TimeStep warns (3D, ImageDimension=3)", "[ImageProcessing][GradientAnisotropicDiffusionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 6, DY = 6, DZ = 6;
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, DX, DY, DZ, MakeRamp<float32>(DX, DY, DZ));
  GradientAnisoFilter filter;
  Arguments args;
  args.insertOrAssign(GradientAnisoFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
  args.insertOrAssign(GradientAnisoFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(GradientAnisoFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(GradientAnisoFilter::k_TimeStep_Key, std::make_any<float64>(0.5));
  args.insertOrAssign(GradientAnisoFilter::k_ConductanceParameter_Key, std::make_any<float64>(3.0));
  args.insertOrAssign(GradientAnisoFilter::k_ConductanceScalingUpdateInterval_Key, std::make_any<uint32>(1u));
  args.insertOrAssign(GradientAnisoFilter::k_NumberOfIterations_Key, std::make_any<uint32>(5u));
  const auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  REQUIRE_FALSE(result.outputActions.warnings().empty());
  REQUIRE(result.outputActions.warnings().front().code == nx::core::ImageProcessing::k_AnisotropicDiffusionUnstableTimeStep);
  REQUIRE(result.outputActions.warnings().front().message.find("ImageDimension=3") != std::string::npos);
}

// -----------------------------------------------------------------------------
// (4) ConductanceScalingUpdateInterval == 0 is legal and must run cleanly (no modulo-by-zero). UpdateGlobalConductance
//     guards the cadence test with `fn.interval != 0 && (iter % fn.interval) != 0`, so interval 0 is treated as "update
//     every iteration" rather than performing `iter % 0`. RunGradientAniso REQUIREs both preflight and execute VALID.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GradientAnisotropicDiffusionImageFilter: zero conductance update interval runs cleanly", "[ImageProcessing][GradientAnisotropicDiffusionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 8, DY = 8, DZ = 8;
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, DX, DY, DZ, MakeRamp<float32>(DX, DY, DZ));
  GradientAnisoFilter filter;
  RunGradientAniso(filter, ds, inputPath, /*timeStep=*/0.0625, /*conductanceParameter=*/3.0, /*conductanceScalingUpdateInterval=*/0u, /*numberOfIterations=*/5u);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::float32);
}

// -----------------------------------------------------------------------------
// (5) NumberOfIterations == 0 is a no-op: the driver loop never runs, so the output = cast<T>(accum = cast<Real>(input))
//     is a lossless float32 round-trip -> output == input EXACTLY.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GradientAnisotropicDiffusionImageFilter: zero iterations is a no-op", "[ImageProcessing][GradientAnisotropicDiffusionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 8, DY = 8, DZ = 8;
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, DX, DY, DZ, MakeRamp<float32>(DX, DY, DZ));
  GradientAnisoFilter filter;
  RunGradientAniso(filter, ds, inputPath, /*timeStep=*/0.0625, /*conductanceParameter=*/3.0, /*conductanceScalingUpdateInterval=*/1u, /*numberOfIterations=*/0u);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  rt::RequireExact<float32>(ds.getDataRefAs<IDataArray>(outputPath), ds.getDataRefAs<IDataArray>(inputPath));
}
