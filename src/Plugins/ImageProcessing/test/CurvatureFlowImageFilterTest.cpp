#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/CurvatureFlowImageFilter.hpp"

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
// Legacy ITKCurvatureFlowImageFilter, created at runtime by UUID (so this target does not link ITKImageProcessing).
// It is ENABLED in the ITKImageProcessing build, so it is the live-ITK reference for the finite-difference engine's
// bit-exact parity gate.
const Uuid k_LegacyCurvatureFlowUuid = *Uuid::FromString("fe5b2ed3-54dd-4207-ad88-48a95134684a");

using CurvFilter = CurvatureFlowImageFilter;

// Sets the standard geom/input/output keys + TimeStep/NumberOfIterations on a shared Arguments, runs preflight +
// execute, and requires both succeed. Reuses CurvFilter::k_*_Key for BOTH the new and the legacy ITK filter --
// correct only because the new filter deliberately reuses the legacy key strings ("input_image_geometry_path",
// "input_image_data_path", "output_array_name", "time_step", "number_of_iterations").
void RunCurvatureFlow(IFilter& filter, DataStructure& ds, const DataPath& inputPath, float64 timeStep, uint32 numberOfIterations, const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(CurvFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(CurvFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(CurvFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(CurvFilter::k_TimeStep_Key, std::make_any<float64>(timeStep));
  args.insertOrAssign(CurvFilter::k_NumberOfIterations_Key, std::make_any<uint32>(numberOfIterations));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

// A deterministic non-constant pattern with values in [0, 16], representable in EVERY scalar type (incl. uint8/int8)
// so the full 10-type parity grid can reuse it. Varies along all three axes so the curvature-flow update is non-trivial.
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

// Run new + legacy on the same field/params and require the SameAsInput outputs match. Comparator is
// UnitTest::CompareDataArrays<T> (exact for integer types, EPSILON tolerance for float -- matches the SameAsInput
// output type).
template <class T>
void RequireParity(const std::vector<T>& field, usize dx, usize dy, usize dz, float64 timeStep, uint32 numberOfIterations, FloatVec3 spacing = {1.0f, 1.0f, 1.0f})
{
  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<T>(newDs, dx, dy, dz, field);
  newDs.getDataRefAs<ImageGeom>(newInput.getParent().getParent()).setSpacing(spacing);
  CurvFilter newFilter;
  RunCurvatureFlow(newFilter, newDs, newInput, timeStep, numberOfIterations);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyCurvatureFlowUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<T>(legacyDs, dx, dy, dz, field);
  legacyDs.getDataRefAs<ImageGeom>(legacyInput.getParent().getParent()).setSpacing(spacing);
  RunCurvatureFlow(*legacyFilter, legacyDs, legacyInput, timeStep, numberOfIterations);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& newOut = newDs.getDataRefAs<IDataArray>(outputPath);
  const auto& legacyOut = legacyDs.getDataRefAs<IDataArray>(outputPath);
  REQUIRE(newOut.getDataType() == legacyOut.getDataType());
  UnitTest::CompareDataArrays<T>(newOut, legacyOut); // exact integer / EPSILON float vs live ITK
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Live-ITK EXACT parity, FULL type grid (all 10 scalar types incl. float) x {(TimeStep, NumberOfIterations)} x
//     {3D, 2D}. The output is SameAsInput; the new filter must reproduce legacy ITK EXACTLY for integer types (the
//     per-iteration static_cast<T> truncation order matches ITK's ApplyUpdate) and within EPSILON for float. This is
//     the primary gate on the finite-difference engine + CurvatureFlowFn.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: Live-ITK parity", "[ImageProcessing][CurvatureFlowImageFilter]", uint8, int8, uint16, int16, uint32, int32, uint64, int64, float32,
                   float64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyCurvatureFlowUuid) != nullptr);

  auto config = GENERATE(table<float64, uint32>({{0.05, 5u}, {0.1, 3u}}));
  const float64 timeStep = std::get<0>(config);
  const uint32 numberOfIterations = std::get<1>(config);
  CAPTURE(timeStep, numberOfIterations);

  SECTION("3D")
  {
    RequireParity<T>(MakeRamp<T>(12, 12, 12), 12, 12, 12, timeStep, numberOfIterations);
  }
  SECTION("2D (Z==1)")
  {
    RequireParity<T>(MakeRamp<T>(20, 16, 1), 20, 16, 1, timeStep, numberOfIterations);
  }
}

// -----------------------------------------------------------------------------
// (2) Live-ITK EXACT parity with a non-unit (anisotropic) spacing geometry (the sc[i]=1/spacing[i] scaling path).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: Live-ITK parity with image spacing", "[ImageProcessing][CurvatureFlowImageFilter]")
{
  using T = float32;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyCurvatureFlowUuid) != nullptr);

  constexpr usize DX = 12, DY = 11, DZ = 10;
  const std::vector<T> field = MakeRamp<T>(DX, DY, DZ);
  const FloatVec3 spacing{0.5f, 1.0f, 2.0f};
  RequireParity<T>(field, DX, DY, DZ, /*timeStep=*/0.05, /*numberOfIterations=*/5u, spacing);
}

// -----------------------------------------------------------------------------
// (3) Preflight guards: a multi-component input is rejected (requireScalar); a valid scalar input produces a
//     SameAsInput output array (unlike GradientMagnitude's fixed float32, CurvatureFlow preserves the input type).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: preflight guards", "[ImageProcessing][CurvatureFlowImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  SECTION("multi-component rejected")
  {
    DataStructure ds;
    const DataPath scalarPath = rt::BuildImageFromPattern<int32>(ds, 6, 6, 1, std::vector<int32>(36, 0));
    const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
    auto vecStore = DataStoreUtilities::CreateDataStore<int32>(ds, vecPath, {1, 6, 6}, {3});
    const auto& cellAM = ds.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
    DataArray<int32>::Create(ds, "Vec", vecStore, cellAM.getId());
    const DataPath geomPath = vecPath.getParent().getParent();
    CurvFilter filter;
    Arguments args;
    args.insertOrAssign(CurvFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(CurvFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
    args.insertOrAssign(CurvFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(CurvFilter::k_TimeStep_Key, std::make_any<float64>(0.05));
    args.insertOrAssign(CurvFilter::k_NumberOfIterations_Key, std::make_any<uint32>(5u));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("scalar input creates a SameAsInput output")
  {
    DataStructure ds;
    const std::vector<uint8> field = MakeRamp<uint8>(6, 6, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<uint8>(ds, 6, 6, 1, field);
    CurvFilter filter;
    RunCurvatureFlow(filter, ds, inputPath, 0.05, 5u);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::uint8);
  }
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- duplicates ITKCurvatureFlowImageTest.cpp on OUR ITK-free filter (RA-Float.nrrd ->
// float32, SameAsInput). (A) DURABLE golden = committed baseline @0.0001 via ip_golden::CompareImages; (B) LIVE-ITK
// parity = TOLERANT (EPSILON, CompareDataArrays<float32>) -- CurvatureFlow's established parity class (the synthetic
// parity tests above use UnitTest::CompareDataArrays, NOT rt::RequireExact). Helper pins ForceInCore.
//   defaults: filter defaults (TimeStep 0.05, NumberOfIterations 5 -- identical between our filter and legacy ITK).
//   longer:   TimeStep 0.1, NumberOfIterations 10.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][CurvatureFlowImageFilter]")
{
  rt::RunDistanceMapItkGoldenBaseline<CurvatureFlowImageFilter>("RA-Float.nrrd", "BasicFilters_CurvatureFlowImageFilter_defaults.nrrd", /*tolerance=*/0.0001, /*bitExactB=*/false);
}

TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: ITK real-image golden (longer)", "[ImageProcessing][ItkGolden][CurvatureFlowImageFilter]")
{
  rt::RunDistanceMapItkGoldenBaseline<CurvatureFlowImageFilter>("RA-Float.nrrd", "BasicFilters_CurvatureFlowImageFilter_longer.nrrd", /*tolerance=*/0.0001, /*bitExactB=*/false, [](Arguments& args) {
    args.insertOrAssign(CurvatureFlowImageFilter::k_TimeStep_Key, std::make_any<float64>(0.1));
    args.insertOrAssign(CurvatureFlowImageFilter::k_NumberOfIterations_Key, std::make_any<uint32>(10u));
  });
}

// -----------------------------------------------------------------------------
// (4) NumberOfIterations == 0 is a no-op: the driver's `for(iter=0; iter<numberOfIterations; ...)` loop never runs, and
//     the final output = cast<T>(accum = cast<Real>(input)) is a lossless float32 round-trip -> output == input EXACTLY.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: zero iterations is a no-op", "[ImageProcessing][CurvatureFlowImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 8, DY = 8, DZ = 8;
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, DX, DY, DZ, MakeRamp<float32>(DX, DY, DZ));
  CurvFilter filter;
  RunCurvatureFlow(filter, ds, inputPath, /*timeStep=*/0.05, /*numberOfIterations=*/0u);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  rt::RequireExact<float32>(ds.getDataRefAs<IDataArray>(outputPath), ds.getDataRefAs<IDataArray>(inputPath));
}
