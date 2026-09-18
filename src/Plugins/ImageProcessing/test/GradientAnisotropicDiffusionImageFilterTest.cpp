#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/GradientAnisotropicDiffusionImageFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <catch2/catch.hpp>

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
// Legacy ITKGradientAnisotropicDiffusionImageFilter, created at runtime by UUID (so this target does not link
// ITKImageProcessing). It is ENABLED in the ITKImageProcessing build, so it is the live-ITK reference for the
// finite-difference engine's bit-exact parity gate.
const Uuid k_LegacyGradientAnisoUuid = *Uuid::FromString("9dcef77b-e7d2-4a2a-b310-bfa80e8ea7c5");

using GradientAnisoFilter = GradientAnisotropicDiffusionImageFilter;

// Sets the standard geom/input/output keys + TimeStep/ConductanceParameter/ConductanceScalingUpdateInterval/
// NumberOfIterations on a shared Arguments, runs preflight + execute, and requires both succeed. Reuses
// GradientAnisoFilter::k_*_Key for BOTH the new and the legacy ITK filter -- correct only because the new filter
// deliberately reuses the legacy key strings ("input_image_geometry_path", "input_image_data_path",
// "output_array_name", "time_step", "conductance_parameter", "conductance_scaling_update_interval",
// "number_of_iterations").
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

// Run new + legacy on the same field/params and require the SameAsInput outputs match. float32 output is compared
// EXACTLY (rt::RequireExact<T> -- no tolerance; verified bit-exact for every GENERATEd case below). float64 falls
// back to UnitTest::CompareDataArrays<T> (tolerant): rt::RequireExact<T> was attempted first and FAILS for
// float64, but the divergence is proven FP-noise, not a structural mismatch --
//   - it is ULP-scale: a per-voxel/per-iteration-count sweep (12x12x12, conductance=3.0, interval=1, TimeStep=
//     0.0625) found the first differing voxel only appears at iteration 2 (iteration 1 is bit-exact, since the
//     very first CalculateAverageGradientMagnitudeSquared reduction runs over the untouched input, which both
//     engines read identically), and the divergence never exceeds 5 ULP / ~3.6e-15 absolute at any iteration count
//     from 2 through 40 (maxUlps: 2,5,2,2,2,2,2 at iterations 2,3,5,10,20,40) -- a classic non-associative
//     floating-point summation signature, not a growing error.
//   - it does NOT blow up: the number of differing voxels actually SHRINKS as iterations increase (557 at 3
//     iterations down to 222 at 40 iterations, out of 1728), consistent with the diffusion process itself damping
//     an early few-ULP perturbation rather than amplifying it.
//   - float32 (which narrows the float64 intermediate to float32 at the very end) is bit-exact for every grid
//     below, exactly as expected if the float64 divergence is sub-ULP-of-float32 noise.
//   - root cause: ITK's CalculateAverageGradientMagnitudeSquared (itkScalarAnisotropicDiffusionFunction.hxx)
//     accumulates over an interior region and several boundary "faces" via ITK's own (possibly multi-threaded)
//     iteration order, recombined however that run's thread count happens to split the image; our engine performs
//     a SINGLE deterministic ordered accumulation instead (mirroring MinMaxCurvatureFlowImageFilterTest's identical
//     float64 fallback for the analogous reason -- see that file's RequireParity comment).
template <class T>
void RequireParity(const std::vector<T>& field, usize dx, usize dy, usize dz, float64 timeStep, float64 conductanceParameter, uint32 conductanceScalingUpdateInterval, uint32 numberOfIterations,
                   FloatVec3 spacing = {1.0f, 1.0f, 1.0f})
{
  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<T>(newDs, dx, dy, dz, field);
  newDs.getDataRefAs<ImageGeom>(newInput.getParent().getParent()).setSpacing(spacing);
  GradientAnisoFilter newFilter;
  RunGradientAniso(newFilter, newDs, newInput, timeStep, conductanceParameter, conductanceScalingUpdateInterval, numberOfIterations);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyGradientAnisoUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<T>(legacyDs, dx, dy, dz, field);
  legacyDs.getDataRefAs<ImageGeom>(legacyInput.getParent().getParent()).setSpacing(spacing);
  RunGradientAniso(*legacyFilter, legacyDs, legacyInput, timeStep, conductanceParameter, conductanceScalingUpdateInterval, numberOfIterations);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& newOut = newDs.getDataRefAs<IDataArray>(outputPath);
  const auto& legacyOut = legacyDs.getDataRefAs<IDataArray>(outputPath);
  REQUIRE(newOut.getDataType() == legacyOut.getDataType());
  if constexpr(std::is_same_v<T, float32>)
  {
    rt::RequireExact<T>(newOut, legacyOut); // EXACT vs live ITK (no tolerance) -- verified bit-exact
  }
  else
  {
    UnitTest::CompareDataArrays<T>(newOut, legacyOut); // tolerant fallback for float64 -- see comment above
  }
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Live-ITK parity, FloatingScalar type grid (float32/float64 -- integer input is rejected) x
//     {(ConductanceParameter, ConductanceScalingUpdateInterval, NumberOfIterations)} x {3D, 2D}, TimeStep fixed at
//     0.0625 (stable for both 2D and 3D at unit spacing, so no spurious stability warning fires here). This is the
//     primary gate on GradientAnisoFn's Perona-Malik ComputeUpdate + the global average-gradient-magnitude-squared
//     reduction that calibrates m_K.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::GradientAnisotropicDiffusionImageFilter: Live-ITK parity", "[ImageProcessing][GradientAnisotropicDiffusionImageFilter]", float32, float64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyGradientAnisoUuid) != nullptr);

  auto config = GENERATE(table<float64, uint32, uint32>({{3.0, 1u, 5u}, {1.5, 2u, 3u}}));
  const float64 conductanceParameter = std::get<0>(config);
  const uint32 conductanceScalingUpdateInterval = std::get<1>(config);
  const uint32 numberOfIterations = std::get<2>(config);
  constexpr float64 k_TimeStep = 0.0625;
  CAPTURE(conductanceParameter, conductanceScalingUpdateInterval, numberOfIterations);

  SECTION("3D")
  {
    RequireParity<T>(MakeRamp<T>(12, 12, 12), 12, 12, 12, k_TimeStep, conductanceParameter, conductanceScalingUpdateInterval, numberOfIterations);
  }
  SECTION("2D (Z==1)")
  {
    RequireParity<T>(MakeRamp<T>(20, 16, 1), 20, 16, 1, k_TimeStep, conductanceParameter, conductanceScalingUpdateInterval, numberOfIterations);
  }
}

// -----------------------------------------------------------------------------
// (2) Live-ITK EXACT parity with a non-unit (anisotropic) spacing geometry (the sc[i]=1/spacing[i] scaling path,
//     which feeds both the Perona-Malik ComputeUpdate and the global gradient-magnitude reduction).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GradientAnisotropicDiffusionImageFilter: Live-ITK parity with image spacing", "[ImageProcessing][GradientAnisotropicDiffusionImageFilter]")
{
  using T = float32;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyGradientAnisoUuid) != nullptr);

  constexpr usize DX = 12, DY = 11, DZ = 10;
  const std::vector<T> field = MakeRamp<T>(DX, DY, DZ);
  const FloatVec3 spacing{0.5f, 1.0f, 2.0f};
  // minSpacing == 0.5 here, so the stable bound is 0.5/2^4 == 0.03125 -- use a TimeStep comfortably below that so
  // this parity case does not also trip the (separately-tested) stability warning.
  RequireParity<T>(field, DX, DY, DZ, /*timeStep=*/0.015625, /*conductanceParameter=*/2.0, /*conductanceScalingUpdateInterval=*/1u, /*numberOfIterations=*/5u, spacing);
}

// -----------------------------------------------------------------------------
// (3) Preflight guards: an INTEGER input is rejected (FloatingScalar admits only float32/float64, unlike
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
// (RA-Float.nrrd -> float32, SameAsInput). (A) DURABLE golden = committed baseline at tolerance **0.0** (a BIT-EXACT
// durable baseline) via ip_golden::CompareImages; (B) LIVE-ITK parity = BIT-EXACT for float32 (rt::RequireExactFloat32)
// -- this filter's established parity class (its synthetic parity test uses rt::RequireExact<float32>). ForceInCore.
//   defaults: the ITK "defaults" case sets TimeStep 0.01 (not literally all-default); Conductance/Interval/Iterations
//             stay at the shared filter defaults (3.0 / 1 / 5).
//   longer:   TimeStep 0.01, NumberOfIterations 10.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GradientAnisotropicDiffusionImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][GradientAnisotropicDiffusionImageFilter]")
{
  rt::RunDistanceMapItkGoldenBaseline<GradientAnisotropicDiffusionImageFilter>(
      "RA-Float.nrrd", "BasicFilters_GradientAnisotropicDiffusionImageFilter_defaults.nrrd", /*tolerance=*/0.0, /*bitExactB=*/true,
      [](Arguments& args) { args.insertOrAssign(GradientAnisotropicDiffusionImageFilter::k_TimeStep_Key, std::make_any<float64>(0.01)); });
}

TEST_CASE("ImageProcessing::GradientAnisotropicDiffusionImageFilter: ITK real-image golden (longer)", "[ImageProcessing][ItkGolden][GradientAnisotropicDiffusionImageFilter]")
{
  rt::RunDistanceMapItkGoldenBaseline<GradientAnisotropicDiffusionImageFilter>("RA-Float.nrrd", "BasicFilters_GradientAnisotropicDiffusionImageFilter_longer.nrrd", /*tolerance=*/0.0,
                                                                               /*bitExactB=*/true, [](Arguments& args) {
                                                                                 args.insertOrAssign(GradientAnisotropicDiffusionImageFilter::k_TimeStep_Key, std::make_any<float64>(0.01));
                                                                                 args.insertOrAssign(GradientAnisotropicDiffusionImageFilter::k_NumberOfIterations_Key, std::make_any<uint32>(10u));
                                                                               });
}

// -----------------------------------------------------------------------------
// (4) A spatially-constant (uniform) image is an IDENTITY: its average squared gradient magnitude is 0, so the global
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
// (5) The CFL stability WARNING for a genuinely 3D image (effDim == 3): the existing preflight-guards test only trips
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
// (6) ConductanceScalingUpdateInterval == 0 is legal and must run cleanly (no modulo-by-zero). UpdateGlobalConductance
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
// (7) NumberOfIterations == 0 is a no-op: the driver loop never runs, so the output = cast<T>(accum = cast<Real>(input))
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
