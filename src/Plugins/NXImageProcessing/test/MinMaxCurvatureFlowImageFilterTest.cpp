#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/MinMaxCurvatureFlowImageFilter.hpp"

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
using MinMaxFilter = MinMaxCurvatureFlowImageFilter;

// Sets the geometry, input, and output keys plus TimeStep, NumberOfIterations, and StencilRadius on the Arguments.
// The helper runs preflight and execute, and requires that both steps succeed.
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
} // namespace

// -----------------------------------------------------------------------------
// (1) Preflight guards: an INTEGER input is rejected (FloatingScalar admits only float32/float64, unlike
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
// ITK-sourced real-image golden -- duplicates ITKMinMaxCurvatureFlowImageTest.cpp on our ITK-free filter
// (RA-Float.nrrd -> float32, SameAsInput). The durable golden compares the output with the committed baseline at a
// tolerance of 0.01 via ip_golden::CompareImages. The helper pins ForceInCore.
//   defaults: the filter defaults (TimeStep 0.05, NumberOfIterations 5, StencilRadius 2). They agree with the ITK
//             defaults.
//   longer:   TimeStep 0.1, NumberOfIterations 10 (StencilRadius stays at default 2).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MinMaxCurvatureFlowImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][MinMaxCurvatureFlowImageFilter]")
{
  rt::RunDistanceMapItkGoldenBaseline<MinMaxCurvatureFlowImageFilter>("RA-Float.nrrd", "BasicFilters_MinMaxCurvatureFlowImageFilter_defaults.nrrd", /*tolerance=*/0.01);
}

TEST_CASE("ImageProcessing::MinMaxCurvatureFlowImageFilter: ITK real-image golden (longer)", "[ImageProcessing][ItkGolden][MinMaxCurvatureFlowImageFilter]")
{
  rt::RunDistanceMapItkGoldenBaseline<MinMaxCurvatureFlowImageFilter>("RA-Float.nrrd", "BasicFilters_MinMaxCurvatureFlowImageFilter_longer.nrrd", /*tolerance=*/0.01, [](Arguments& args) {
    args.insertOrAssign(MinMaxCurvatureFlowImageFilter::k_TimeStep_Key, std::make_any<float64>(0.1));
    args.insertOrAssign(MinMaxCurvatureFlowImageFilter::k_NumberOfIterations_Key, std::make_any<uint32>(10u));
  });
}

// -----------------------------------------------------------------------------
// (2) A StencilRadius of 0 (or negative) is CLAMPED to 1 at execute (MinMaxCurvatureFlowImageFilter.cpp: `(stencilRadius
//     > 1) ? stencilRadius : 1`, matching itkMinMaxCurvatureFlowFunction::SetStencilRadius). So a run at radius 0 must
//     produce BYTE-IDENTICAL output to a run at radius 1 (both clamp to 1). The test uses a non-constant ramp, so the
//     radius changes the result, and it compares the two runs directly.
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
// (3) A spatially-constant (uniform) image is an IDENTITY: every base curvature-flow update is 0 (magSqr==0 early
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
// (4) NumberOfIterations == 0 is a no-op: the driver's `for(iter=0; iter<numberOfIterations; ...)` loop never runs, and
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
