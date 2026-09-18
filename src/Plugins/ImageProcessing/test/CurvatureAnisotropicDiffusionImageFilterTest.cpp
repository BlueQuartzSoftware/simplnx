#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/CurvatureAnisotropicDiffusionImageFilter.hpp"

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
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
// Legacy ITKCurvatureAnisotropicDiffusionImageFilter, created at runtime by UUID (so this target does not link
// ITKImageProcessing). It is ENABLED in the ITKImageProcessing build, so it is the live-ITK reference for the
// finite-difference engine's bit-exact parity gate.
const Uuid k_LegacyCurvatureAnisoUuid = *Uuid::FromString("ada68f29-b1f2-44a2-86dc-f0cd28f54633");

using CurvatureAnisoFilter = CurvatureAnisotropicDiffusionImageFilter;

// Sets the standard geom/input/output keys + TimeStep/ConductanceParameter/ConductanceScalingUpdateInterval/
// NumberOfIterations on a shared Arguments, runs preflight + execute, and requires both succeed. Reuses
// CurvatureAnisoFilter::k_*_Key for BOTH the new and the legacy ITK filter -- correct only because the new filter
// deliberately reuses the legacy key strings ("input_image_geometry_path", "input_image_data_path",
// "output_array_name", "time_step", "conductance_parameter", "conductance_scaling_update_interval",
// "number_of_iterations").
void RunCurvatureAniso(IFilter& filter, DataStructure& ds, const DataPath& inputPath, float64 timeStep, float64 conductanceParameter, uint32 conductanceScalingUpdateInterval,
                       uint32 numberOfIterations, const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(CurvatureAnisoFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(CurvatureAnisoFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(CurvatureAnisoFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(CurvatureAnisoFilter::k_TimeStep_Key, std::make_any<float64>(timeStep));
  args.insertOrAssign(CurvatureAnisoFilter::k_ConductanceParameter_Key, std::make_any<float64>(conductanceParameter));
  args.insertOrAssign(CurvatureAnisoFilter::k_ConductanceScalingUpdateInterval_Key, std::make_any<uint32>(conductanceScalingUpdateInterval));
  args.insertOrAssign(CurvatureAnisoFilter::k_NumberOfIterations_Key, std::make_any<uint32>(numberOfIterations));
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
// EXACTLY (rt::RequireExact<T> -- no tolerance); float64 falls back to UnitTest::CompareDataArrays<T> (tolerant).
//
// float32 is bit-exact vs live ITK because this functor runs NATIVELY in the input type (k_NativePrecision, matching
// ITK's IntermediateType-less bridge -- see CurvatureAnisoFn). The one thing that had to be modeled to reach exact
// was m_K: ITK's InitializeIteration recomputes m_K as `static_cast<PixelType>(avg * c * c * -2.0f)` EVERY iteration,
// so for a float32 image m_K is a float32 -- the double-precision product is ROUNDED to float32 every iteration. An
// earlier port kept m_K a full double, which made the `grad_mag_sq / m_K` conductance divisor ~6e-8 (relative) too
// precise; that missing narrowing -- NOT reduction-order FP noise -- was the ENTIRE float32 gap. Measured directly
// against live ITK on a 12x12x12 ramp at conductance=3.0/interval=1, the m_K-as-double port's float32 output
// diffCount/maxUlp (out of 1728 voxels) was 0/0, 18/1, 14/1, 21/1, 53/2, 137/7, 271/163 at iterations
// 1,2,3,5,10,20,40 -- a divergence that GROWS with iteration count, the opposite of a bounded reduction-noise
// plateau. detail::CurvatureAnisoFn::setGlobalK now narrows m_K through the native precision (a no-op for float64
// input), and grad_mag_sq stays a double in computeUpdate exactly as in ITK, so the double-precision `grad_mag_sq /
// m_K` division is byte-identical. With that fix float32 is bit-exact at EVERY one of those iteration counts (0/0
// through 40 iterations) -- hence rt::RequireExact<T> below.
//
// float64 stays tolerant: for a float64 image ITK's PixelType is already double, so m_K is double in BOTH engines
// and the m_K narrowing is a pure no-op (the float64 diffCount/maxUlp is byte-identical with and without the fix --
// e.g. 264/2 at 3 iters and 309/4 at 5 iters, this grid's actual settings). That residual is genuine reduction-order
// FP noise, the SAME root cause documented for GradientAnisoFn's float64 fallback: ITK's
// CalculateAverageGradientMagnitudeSquared accumulates the average-gradient-magnitude-squared reduction over an
// interior region plus boundary "faces" in ITK's own (possibly multi-threaded) order, whereas this engine performs a
// single deterministic ordered accumulation (see detail::UpdateGlobalConductance) that differs only by summation
// order. It is bit-exact at iteration 1 for float64 too (the first reduction runs over the untouched input, read
// identically), diverging only from iteration 2 -- the hallmark of reduction-order noise, not a formula bug.
template <class T>
void RequireParity(const std::vector<T>& field, usize dx, usize dy, usize dz, float64 timeStep, float64 conductanceParameter, uint32 conductanceScalingUpdateInterval, uint32 numberOfIterations,
                   FloatVec3 spacing = {1.0f, 1.0f, 1.0f})
{
  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<T>(newDs, dx, dy, dz, field);
  newDs.getDataRefAs<ImageGeom>(newInput.getParent().getParent()).setSpacing(spacing);
  CurvatureAnisoFilter newFilter;
  RunCurvatureAniso(newFilter, newDs, newInput, timeStep, conductanceParameter, conductanceScalingUpdateInterval, numberOfIterations);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyCurvatureAnisoUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<T>(legacyDs, dx, dy, dz, field);
  legacyDs.getDataRefAs<ImageGeom>(legacyInput.getParent().getParent()).setSpacing(spacing);
  RunCurvatureAniso(*legacyFilter, legacyDs, legacyInput, timeStep, conductanceParameter, conductanceScalingUpdateInterval, numberOfIterations);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& newOut = newDs.getDataRefAs<IDataArray>(outputPath);
  const auto& legacyOut = legacyDs.getDataRefAs<IDataArray>(outputPath);
  REQUIRE(newOut.getDataType() == legacyOut.getDataType());
  if constexpr(std::is_same_v<T, float32>)
  {
    rt::RequireExact<T>(newOut, legacyOut); // EXACT vs live ITK (no tolerance) -- verified bit-exact, see comment above
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
//     primary gate on CurvatureAnisoFn's modified-curvature-diffusion-equation ComputeUpdate + the global
//     average-gradient-magnitude-squared reduction that calibrates m_K (shared verbatim with GradientAnisoFn).
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::CurvatureAnisotropicDiffusionImageFilter: Live-ITK parity", "[ImageProcessing][CurvatureAnisotropicDiffusionImageFilter]", float32, float64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyCurvatureAnisoUuid) != nullptr);

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
// (2) Live-ITK parity with a non-unit (anisotropic) spacing geometry (the sc[i]=1/spacing[i] scaling path, which
//     feeds both the curvature ComputeUpdate and the global gradient-magnitude reduction). float32 input, so
//     RequireParity compares BIT-EXACT here (rt::RequireExact) -- see its comment above.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::CurvatureAnisotropicDiffusionImageFilter: Live-ITK parity with image spacing", "[ImageProcessing][CurvatureAnisotropicDiffusionImageFilter]")
{
  using T = float32;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyCurvatureAnisoUuid) != nullptr);

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
TEST_CASE("ImageProcessing::CurvatureAnisotropicDiffusionImageFilter: preflight guards", "[ImageProcessing][CurvatureAnisotropicDiffusionImageFilter]")
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
    CurvatureAnisoFilter filter;
    Arguments args;
    args.insertOrAssign(CurvatureAnisoFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
    args.insertOrAssign(CurvatureAnisoFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(CurvatureAnisoFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(CurvatureAnisoFilter::k_TimeStep_Key, std::make_any<float64>(0.0625));
    args.insertOrAssign(CurvatureAnisoFilter::k_ConductanceParameter_Key, std::make_any<float64>(3.0));
    args.insertOrAssign(CurvatureAnisoFilter::k_ConductanceScalingUpdateInterval_Key, std::make_any<uint32>(1u));
    args.insertOrAssign(CurvatureAnisoFilter::k_NumberOfIterations_Key, std::make_any<uint32>(5u));
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
    CurvatureAnisoFilter filter;
    Arguments args;
    args.insertOrAssign(CurvatureAnisoFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(CurvatureAnisoFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
    args.insertOrAssign(CurvatureAnisoFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(CurvatureAnisoFilter::k_TimeStep_Key, std::make_any<float64>(0.0625));
    args.insertOrAssign(CurvatureAnisoFilter::k_ConductanceParameter_Key, std::make_any<float64>(3.0));
    args.insertOrAssign(CurvatureAnisoFilter::k_ConductanceScalingUpdateInterval_Key, std::make_any<uint32>(1u));
    args.insertOrAssign(CurvatureAnisoFilter::k_NumberOfIterations_Key, std::make_any<uint32>(5u));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("scalar float input creates a SameAsInput output")
  {
    DataStructure ds;
    const std::vector<float32> field = MakeRamp<float32>(6, 6, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, 6, 6, 1, field);
    CurvatureAnisoFilter filter;
    RunCurvatureAniso(filter, ds, inputPath, 0.0625, 3.0, 1u, 5u);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::float32);
  }
  SECTION("over-large TimeStep is VALID but carries a stability warning")
  {
    // effDim==2 (Z==1), unit spacing -> stable bound = 1/2^3 = 0.125. 0.5 is well above that.
    DataStructure ds;
    const std::vector<float32> field = MakeRamp<float32>(6, 6, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, 6, 6, 1, field);
    CurvatureAnisoFilter filter;
    Arguments args;
    args.insertOrAssign(CurvatureAnisoFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
    args.insertOrAssign(CurvatureAnisoFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(CurvatureAnisoFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(CurvatureAnisoFilter::k_TimeStep_Key, std::make_any<float64>(0.5));
    args.insertOrAssign(CurvatureAnisoFilter::k_ConductanceParameter_Key, std::make_any<float64>(3.0));
    args.insertOrAssign(CurvatureAnisoFilter::k_ConductanceScalingUpdateInterval_Key, std::make_any<uint32>(1u));
    args.insertOrAssign(CurvatureAnisoFilter::k_NumberOfIterations_Key, std::make_any<uint32>(5u));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
    REQUIRE_FALSE(result.outputActions.warnings().empty());
    REQUIRE(result.outputActions.warnings().front().code == nx::core::ImageProcessing::k_AnisotropicDiffusionUnstableTimeStep);
  }
}

namespace
{
// -----------------------------------------------------------------------------
// Shared body for the two ITK-sourced real-image golden cases below. Mirrors
// ITKImageProcessing/test/ITKCurvatureAnisotropicDiffusionImageTest.cpp exactly: read RA-Float.nrrd (float32) through
// OUR ITK-free reader, run OUR filter with the ITK case's params (defaults, or defaults + NumberOfIterations=10 for
// the "longer" case), then apply the plan's two golden oracles:
//   (A) DURABLE golden -- compare our output to ITK's committed baseline .nrrd (read through the SAME reader) at the
//       ITK test's tolerance 0.1 via ip_golden::CompareImages (permanent, survives ITK removal).
//   (B) LIVE-ITK parity -- run the legacy ITK filter (resolved from the loaded plugin's replacement map) on the SAME
//       input and require BIT-EXACT parity. float32 input is bit-exact vs live ITK per the m_K fix documented at the
//       top of this file, so rt::RequireExact<float32> is the correct (B) comparator (no tolerance).
// The whole case is pinned ForceInCore: the legacy ITK filter bad_casts an OOC store (reading via OUR reader is
// OOC-safe, but (B) is not). All unset params (ConductanceParameter, ConductanceScalingUpdateInterval, and -- for the
// defaults case -- NumberOfIterations) resolve to the filter's defaults, which are identical between our filter and
// the legacy ITK filter, so one Arguments drives both.
// -----------------------------------------------------------------------------
void RunCurvatureAnisoItkGolden(float64 timeStep, std::optional<uint32> numberOfIterations, const std::string& baselineFile)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath cellData = geom.createChildPath("CellData");
  const DataPath input = cellData.createChildPath("Input");
  const DataPath output = cellData.createChildPath("Output");

  // --- read input through OUR ITK-free reader ---
  DataStructure ds;
  const auto readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath("RA-Float.nrrd"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  // --- run OUR filter with the ITK case's params ---
  Arguments args;
  args.insertOrAssign(CurvatureAnisotropicDiffusionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(CurvatureAnisotropicDiffusionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(CurvatureAnisotropicDiffusionImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(CurvatureAnisotropicDiffusionImageFilter::k_TimeStep_Key, std::make_any<float64>(timeStep));
  if(numberOfIterations.has_value())
  {
    args.insertOrAssign(CurvatureAnisotropicDiffusionImageFilter::k_NumberOfIterations_Key, std::make_any<uint32>(*numberOfIterations));
  }
  CurvatureAnisotropicDiffusionImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // --- (A) DURABLE golden: compare to ITK's committed baseline at tol 0.1 (SameAsInput output is owned by 'geom') ---
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath(baselineFile), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.1);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  // --- (B) LIVE-ITK parity (coexistence only): legacy ITK filter on the SAME input, require bit-exact (float32) ---
  const auto legacyUuid = ip_golden::LegacyUuidFor(FilterTraits<CurvatureAnisotropicDiffusionImageFilter>::uuid);
  REQUIRE(legacyUuid.has_value());
  DataStructure itkDs;
  const auto readItkInput = ip_golden::ReadInputImage(itkDs, ip_golden::InputPath("RA-Float.nrrd"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readItkInput);
  const auto itkRun = ip_golden::RunLegacyItkFilter(*legacyUuid, itkDs, args); // identical key strings drive both filters
  SIMPLNX_RESULT_REQUIRE_VALID(itkRun);
  rt::RequireExact<float32>(ds.getDataRefAs<IDataArray>(output), itkDs.getDataRefAs<IDataArray>(output));

  UnitTest::CheckArraysInheritTupleDims(ds);
}
} // namespace

// -----------------------------------------------------------------------------
// (5) ITK-sourced real-image golden -- "defaults" case: RA-Float.nrrd, TimeStep 0.01, all else default. Duplicates
//     ITKCurvatureAnisotropicDiffusionImageTest.cpp(defaults) on OUR ITK-free filter: (A) baseline @0.1 + (B) live-ITK
//     bit-exact.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::CurvatureAnisotropicDiffusionImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][CurvatureAnisotropicDiffusionImageFilter]")
{
  RunCurvatureAnisoItkGolden(/*timeStep=*/0.01, /*numberOfIterations=*/std::nullopt, "BasicFilters_CurvatureAnisotropicDiffusionImageFilter_defaults.nrrd");
}

// -----------------------------------------------------------------------------
// (6) ITK-sourced real-image golden -- "longer" case: RA-Float.nrrd, TimeStep 0.01, NumberOfIterations 10. Duplicates
//     ITKCurvatureAnisotropicDiffusionImageTest.cpp(longer).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::CurvatureAnisotropicDiffusionImageFilter: ITK real-image golden (longer)", "[ImageProcessing][ItkGolden][CurvatureAnisotropicDiffusionImageFilter]")
{
  RunCurvatureAnisoItkGolden(/*timeStep=*/0.01, /*numberOfIterations=*/10u, "BasicFilters_CurvatureAnisotropicDiffusionImageFilter_longer.nrrd");
}

// -----------------------------------------------------------------------------
// (7) A spatially-constant (uniform) image is an IDENTITY: its average squared gradient magnitude is 0, so the global
//     reduction sets m_K = 0, and CurvatureAnisoFn's `if(m_K != 0.0)` conductance branch is skipped (Cx == Cxd == 0);
//     speed is 0 at every voxel, propagationGradient is 0, the returned update is 0, and accum never changes ->
//     output == input EXACTLY.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::CurvatureAnisotropicDiffusionImageFilter: constant image is identity", "[ImageProcessing][CurvatureAnisotropicDiffusionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 8, DY = 8, DZ = 8;
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, DX, DY, DZ, std::vector<float32>(DX * DY * DZ, 7.0f));
  CurvatureAnisoFilter filter;
  RunCurvatureAniso(filter, ds, inputPath, /*timeStep=*/0.0625, /*conductanceParameter=*/3.0, /*conductanceScalingUpdateInterval=*/1u, /*numberOfIterations=*/5u);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  rt::RequireExact<float32>(ds.getDataRefAs<IDataArray>(outputPath), ds.getDataRefAs<IDataArray>(inputPath));
}

// -----------------------------------------------------------------------------
// (8) The CFL stability WARNING for a genuinely 3D image (effDim == 3): the existing preflight-guards test only trips
//     the effDim==2 (Z==1) arm, so the ImageDimension==3 stable-bound branch of AppendUnstableTimeStepWarning was never
//     asserted. A 6x6x6 image at unit spacing has stable bound 1/2^(3+1) = 0.0625; TimeStep 0.5 is well above it, so
//     preflight stays VALID but carries the shared unstable-time-step warning, whose message names ImageDimension=3.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::CurvatureAnisotropicDiffusionImageFilter: over-large TimeStep warns (3D, ImageDimension=3)", "[ImageProcessing][CurvatureAnisotropicDiffusionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 6, DY = 6, DZ = 6;
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, DX, DY, DZ, MakeRamp<float32>(DX, DY, DZ));
  CurvatureAnisoFilter filter;
  Arguments args;
  args.insertOrAssign(CurvatureAnisoFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
  args.insertOrAssign(CurvatureAnisoFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(CurvatureAnisoFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(CurvatureAnisoFilter::k_TimeStep_Key, std::make_any<float64>(0.5));
  args.insertOrAssign(CurvatureAnisoFilter::k_ConductanceParameter_Key, std::make_any<float64>(3.0));
  args.insertOrAssign(CurvatureAnisoFilter::k_ConductanceScalingUpdateInterval_Key, std::make_any<uint32>(1u));
  args.insertOrAssign(CurvatureAnisoFilter::k_NumberOfIterations_Key, std::make_any<uint32>(5u));
  const auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  REQUIRE_FALSE(result.outputActions.warnings().empty());
  REQUIRE(result.outputActions.warnings().front().code == nx::core::ImageProcessing::k_AnisotropicDiffusionUnstableTimeStep);
  REQUIRE(result.outputActions.warnings().front().message.find("ImageDimension=3") != std::string::npos);
}

// -----------------------------------------------------------------------------
// (9) ConductanceScalingUpdateInterval == 0 is legal and must run cleanly (no modulo-by-zero). UpdateGlobalConductance
//     guards the cadence test with `fn.interval != 0 && (iter % fn.interval) != 0`, so interval 0 is treated as "update
//     every iteration" rather than performing `iter % 0`. RunCurvatureAniso REQUIREs both preflight and execute VALID.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::CurvatureAnisotropicDiffusionImageFilter: zero conductance update interval runs cleanly", "[ImageProcessing][CurvatureAnisotropicDiffusionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 8, DY = 8, DZ = 8;
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, DX, DY, DZ, MakeRamp<float32>(DX, DY, DZ));
  CurvatureAnisoFilter filter;
  RunCurvatureAniso(filter, ds, inputPath, /*timeStep=*/0.0625, /*conductanceParameter=*/3.0, /*conductanceScalingUpdateInterval=*/0u, /*numberOfIterations=*/5u);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::float32);
}

// -----------------------------------------------------------------------------
// (10) NumberOfIterations == 0 is a no-op: the driver loop never runs, so the output = cast<T>(accum = cast<Real>(input))
//      is a lossless float32 round-trip -> output == input EXACTLY.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::CurvatureAnisotropicDiffusionImageFilter: zero iterations is a no-op", "[ImageProcessing][CurvatureAnisotropicDiffusionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 8, DY = 8, DZ = 8;
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, DX, DY, DZ, MakeRamp<float32>(DX, DY, DZ));
  CurvatureAnisoFilter filter;
  RunCurvatureAniso(filter, ds, inputPath, /*timeStep=*/0.0625, /*conductanceParameter=*/3.0, /*conductanceScalingUpdateInterval=*/1u, /*numberOfIterations=*/0u);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  rt::RequireExact<float32>(ds.getDataRefAs<IDataArray>(outputPath), ds.getDataRefAs<IDataArray>(inputPath));
}
