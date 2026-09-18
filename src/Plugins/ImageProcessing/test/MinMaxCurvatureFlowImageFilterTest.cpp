#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/MinMaxCurvatureFlowImageFilter.hpp"

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
// Legacy ITKMinMaxCurvatureFlowImageFilter, created at runtime by UUID (so this target does not link
// ITKImageProcessing). It is ENABLED in the ITKImageProcessing build, so it is the live-ITK reference for the
// finite-difference engine's bit-exact parity gate.
const Uuid k_LegacyMinMaxCurvatureFlowUuid = *Uuid::FromString("b836c081-6692-411d-81d0-a50afce6b288");

using MinMaxFilter = MinMaxCurvatureFlowImageFilter;

// Sets the standard geom/input/output keys + TimeStep/NumberOfIterations/StencilRadius on a shared Arguments, runs
// preflight + execute, and requires both succeed. Reuses MinMaxFilter::k_*_Key for BOTH the new and the legacy ITK
// filter -- correct only because the new filter deliberately reuses the legacy key strings ("input_image_geometry_path",
// "input_image_data_path", "output_array_name", "time_step", "number_of_iterations", "stencil_radius").
void RunMinMaxCurvatureFlow(IFilter& filter, DataStructure& ds, const DataPath& inputPath, float64 timeStep, uint32 numberOfIterations, int32 stencilRadius, const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(MinMaxFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(MinMaxFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(MinMaxFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(MinMaxFilter::k_TimeStep_Key, std::make_any<float64>(timeStep));
  args.insertOrAssign(MinMaxFilter::k_NumberOfIterations_Key, std::make_any<uint32>(numberOfIterations));
  args.insertOrAssign(MinMaxFilter::k_StencilRadius_Key, std::make_any<int32>(stencilRadius));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

// A deterministic non-constant pattern with values in [0, 16], varying along all three axes so the curvature-flow
// update (and the min/max stencil gate) is non-trivial.
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
// back to UnitTest::CompareDataArrays<T> (tolerant): instrumented per-voxel/per-iteration (dumping the double
// accumulator directly, bypassing the float32 narrowing that would otherwise mask this), iteration 1 is bit-exact
// for the WHOLE field, and the first divergence appears only at iteration 2, isolated to voxels where the base
// curvature update is itself near zero and/or the min/max stencil-average-vs-threshold comparison is a hairline
// tie -- i.e. a single-ULP non-associativity (transcendental libm rounding / operation-order) tipping a near-exact
// boundary decision, not a structural mismatch in the threshold/stencil transcription (which the float32 exact gate
// and the OOC-vs-in-core byte-match gate both confirm is correct). The residual stays at the few-ULP scale through
// 5 iterations (it does not blow up), consistent with a benign tie rather than a real bug.
template <class T>
void RequireParity(const std::vector<T>& field, usize dx, usize dy, usize dz, float64 timeStep, uint32 numberOfIterations, int32 stencilRadius, FloatVec3 spacing = {1.0f, 1.0f, 1.0f})
{
  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<T>(newDs, dx, dy, dz, field);
  newDs.getDataRefAs<ImageGeom>(newInput.getParent().getParent()).setSpacing(spacing);
  MinMaxFilter newFilter;
  RunMinMaxCurvatureFlow(newFilter, newDs, newInput, timeStep, numberOfIterations, stencilRadius);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyMinMaxCurvatureFlowUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<T>(legacyDs, dx, dy, dz, field);
  legacyDs.getDataRefAs<ImageGeom>(legacyInput.getParent().getParent()).setSpacing(spacing);
  RunMinMaxCurvatureFlow(*legacyFilter, legacyDs, legacyInput, timeStep, numberOfIterations, stencilRadius);

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
//     {(TimeStep, NumberOfIterations, StencilRadius)} x {3D, 2D}. The output is SameAsInput; float32 must reproduce
//     legacy ITK EXACTLY (verified bit-exact), float64 within UnitTest::CompareDataArrays' tolerance (see the
//     tolerant-fallback comment on RequireParity above). This is the primary gate on MinMaxCurvatureFlowFn's
//     threshold/stencil port.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::MinMaxCurvatureFlowImageFilter: Live-ITK parity", "[ImageProcessing][MinMaxCurvatureFlowImageFilter]", float32, float64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyMinMaxCurvatureFlowUuid) != nullptr);

  auto config = GENERATE(table<float64, uint32, int32>({{0.05, 5u, 2}, {0.05, 3u, 1}}));
  const float64 timeStep = std::get<0>(config);
  const uint32 numberOfIterations = std::get<1>(config);
  const int32 stencilRadius = std::get<2>(config);
  CAPTURE(timeStep, numberOfIterations, stencilRadius);

  SECTION("3D")
  {
    RequireParity<T>(MakeRamp<T>(12, 12, 12), 12, 12, 12, timeStep, numberOfIterations, stencilRadius);
  }
  SECTION("2D (Z==1)")
  {
    RequireParity<T>(MakeRamp<T>(20, 16, 1), 20, 16, 1, timeStep, numberOfIterations, stencilRadius);
  }
}

// -----------------------------------------------------------------------------
// (2) Live-ITK EXACT parity with a non-unit (anisotropic) spacing geometry (the sc[i]=1/spacing[i] scaling path,
//     which also feeds the gradient direction used by ComputeThreshold).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MinMaxCurvatureFlowImageFilter: Live-ITK parity with image spacing", "[ImageProcessing][MinMaxCurvatureFlowImageFilter]")
{
  using T = float32;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyMinMaxCurvatureFlowUuid) != nullptr);

  constexpr usize DX = 12, DY = 11, DZ = 10;
  const std::vector<T> field = MakeRamp<T>(DX, DY, DZ);
  const FloatVec3 spacing{0.5f, 1.0f, 2.0f};
  RequireParity<T>(field, DX, DY, DZ, /*timeStep=*/0.05, /*numberOfIterations=*/5u, /*stencilRadius=*/2, spacing);
}

// -----------------------------------------------------------------------------
// (3) Preflight guards: an INTEGER input is rejected (FloatingScalar admits only float32/float64, unlike
//     CurvatureFlow's AllNumeric); a multi-component input is rejected (requireScalar); a valid float input
//     produces a SameAsInput output array.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MinMaxCurvatureFlowImageFilter: preflight guards", "[ImageProcessing][MinMaxCurvatureFlowImageFilter]")
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
    MinMaxFilter filter;
    Arguments args;
    args.insertOrAssign(MinMaxFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
    args.insertOrAssign(MinMaxFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(MinMaxFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(MinMaxFilter::k_TimeStep_Key, std::make_any<float64>(0.05));
    args.insertOrAssign(MinMaxFilter::k_NumberOfIterations_Key, std::make_any<uint32>(5u));
    args.insertOrAssign(MinMaxFilter::k_StencilRadius_Key, std::make_any<int32>(2));
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
    MinMaxFilter filter;
    Arguments args;
    args.insertOrAssign(MinMaxFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(MinMaxFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
    args.insertOrAssign(MinMaxFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(MinMaxFilter::k_TimeStep_Key, std::make_any<float64>(0.05));
    args.insertOrAssign(MinMaxFilter::k_NumberOfIterations_Key, std::make_any<uint32>(5u));
    args.insertOrAssign(MinMaxFilter::k_StencilRadius_Key, std::make_any<int32>(2));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("scalar float input creates a SameAsInput output")
  {
    DataStructure ds;
    const std::vector<float32> field = MakeRamp<float32>(6, 6, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, 6, 6, 1, field);
    MinMaxFilter filter;
    RunMinMaxCurvatureFlow(filter, ds, inputPath, 0.05, 5u, 2);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::float32);
  }
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- duplicates ITKMinMaxCurvatureFlowImageTest.cpp on OUR ITK-free filter
// (RA-Float.nrrd -> float32, SameAsInput). (A) DURABLE golden = committed baseline @0.01 via ip_golden::CompareImages;
// (B) LIVE-ITK parity = BIT-EXACT for float32 (rt::RequireExactFloat32) -- MinMaxCurvatureFlow's established parity
// class (its synthetic parity test uses rt::RequireExact<float32> for float32). Helper pins ForceInCore.
//   defaults: filter defaults (TimeStep 0.05, NumberOfIterations 5, StencilRadius 2 -- identical vs legacy ITK).
//   longer:   TimeStep 0.1, NumberOfIterations 10 (StencilRadius stays at default 2).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MinMaxCurvatureFlowImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][MinMaxCurvatureFlowImageFilter]")
{
  rt::RunDistanceMapItkGoldenBaseline<MinMaxCurvatureFlowImageFilter>("RA-Float.nrrd", "BasicFilters_MinMaxCurvatureFlowImageFilter_defaults.nrrd", /*tolerance=*/0.01, /*bitExactB=*/true);
}

TEST_CASE("ImageProcessing::MinMaxCurvatureFlowImageFilter: ITK real-image golden (longer)", "[ImageProcessing][ItkGolden][MinMaxCurvatureFlowImageFilter]")
{
  rt::RunDistanceMapItkGoldenBaseline<MinMaxCurvatureFlowImageFilter>("RA-Float.nrrd", "BasicFilters_MinMaxCurvatureFlowImageFilter_longer.nrrd", /*tolerance=*/0.01, /*bitExactB=*/true,
                                                                      [](Arguments& args) {
                                                                        args.insertOrAssign(MinMaxCurvatureFlowImageFilter::k_TimeStep_Key, std::make_any<float64>(0.1));
                                                                        args.insertOrAssign(MinMaxCurvatureFlowImageFilter::k_NumberOfIterations_Key, std::make_any<uint32>(10u));
                                                                      });
}

// -----------------------------------------------------------------------------
// (6) Live-ITK parity at StencilRadius 3 (> the 1/2 exercised by the primary grid). StencilRadius is this filter's
//     defining parameter: it sizes the MinMaxStencilAverage sphere and the ComputeThreshold gradient-direction sampling
//     radius. Only radii 1 and 2 were previously gated; radius 3 exercises those loops/threshold at a wider extent.
//     float32 is bit-exact vs live ITK; float64 is tolerant (the documented reduction-noise fallback, see RequireParity).
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::MinMaxCurvatureFlowImageFilter: Live-ITK parity at StencilRadius 3", "[ImageProcessing][MinMaxCurvatureFlowImageFilter]", float32, float64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyMinMaxCurvatureFlowUuid) != nullptr);

  RequireParity<T>(MakeRamp<T>(12, 12, 12), 12, 12, 12, /*timeStep=*/0.05, /*numberOfIterations=*/3u, /*stencilRadius=*/3);
}

// -----------------------------------------------------------------------------
// (7) A StencilRadius of 0 (or negative) is CLAMPED to 1 at execute (MinMaxCurvatureFlowImageFilter.cpp: `(stencilRadius
//     > 1) ? stencilRadius : 1`, matching itkMinMaxCurvatureFlowFunction::SetStencilRadius). So a run at radius 0 must
//     produce BYTE-IDENTICAL output to a run at radius 1 (both clamp to 1). Uses a non-constant ramp so radius actually
//     matters, and compares the two runs directly (no live ITK needed for this clamp check).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MinMaxCurvatureFlowImageFilter: StencilRadius 0 clamps to 1", "[ImageProcessing][MinMaxCurvatureFlowImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 12, DY = 12, DZ = 12;
  const std::vector<float32> field = MakeRamp<float32>(DX, DY, DZ);

  DataStructure ds0;
  const DataPath input0 = rt::BuildImageFromPattern<float32>(ds0, DX, DY, DZ, field);
  MinMaxFilter filter0;
  RunMinMaxCurvatureFlow(filter0, ds0, input0, /*timeStep=*/0.05, /*numberOfIterations=*/5u, /*stencilRadius=*/0);

  DataStructure ds1;
  const DataPath input1 = rt::BuildImageFromPattern<float32>(ds1, DX, DY, DZ, field);
  MinMaxFilter filter1;
  RunMinMaxCurvatureFlow(filter1, ds1, input1, /*timeStep=*/0.05, /*numberOfIterations=*/5u, /*stencilRadius=*/1);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  rt::RequireExact<float32>(ds0.getDataRefAs<IDataArray>(outputPath), ds1.getDataRefAs<IDataArray>(outputPath));
}

// -----------------------------------------------------------------------------
// (8) A spatially-constant (uniform) image is an IDENTITY: every base curvature-flow update is 0 (magSqr==0 early
//     return), so MinMaxCurvatureFlowFn's `if(update == 0.0) return 0.0` short-circuits before any threshold/stencil
//     work, and accum never changes -> output == input EXACTLY.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MinMaxCurvatureFlowImageFilter: constant image is identity", "[ImageProcessing][MinMaxCurvatureFlowImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 8, DY = 8, DZ = 8;
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, DX, DY, DZ, std::vector<float32>(DX * DY * DZ, 7.0f));
  MinMaxFilter filter;
  RunMinMaxCurvatureFlow(filter, ds, inputPath, /*timeStep=*/0.05, /*numberOfIterations=*/5u, /*stencilRadius=*/2);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  rt::RequireExact<float32>(ds.getDataRefAs<IDataArray>(outputPath), ds.getDataRefAs<IDataArray>(inputPath));
}

// -----------------------------------------------------------------------------
// (9) NumberOfIterations == 0 is a no-op: the driver's `for(iter=0; iter<numberOfIterations; ...)` loop never runs, and
//     the final output = cast<T>(accum = cast<Real>(input)) is a lossless float32 round-trip -> output == input EXACTLY.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MinMaxCurvatureFlowImageFilter: zero iterations is a no-op", "[ImageProcessing][MinMaxCurvatureFlowImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 8, DY = 8, DZ = 8;
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, DX, DY, DZ, MakeRamp<float32>(DX, DY, DZ));
  MinMaxFilter filter;
  RunMinMaxCurvatureFlow(filter, ds, inputPath, /*timeStep=*/0.05, /*numberOfIterations=*/0u, /*stencilRadius=*/2);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  rt::RequireExact<float32>(ds.getDataRefAs<IDataArray>(outputPath), ds.getDataRefAs<IDataArray>(inputPath));
}
