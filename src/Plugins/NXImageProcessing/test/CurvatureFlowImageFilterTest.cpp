#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/CurvatureFlowImageFilter.hpp"

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
using CurvFilter = CurvatureFlowImageFilter;

// Sets the geometry, input, and output keys plus TimeStep and NumberOfIterations on the Arguments. The helper runs
// preflight and execute, and requires that both steps succeed.
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
} // namespace

// -----------------------------------------------------------------------------
// (1) Preflight guards: a multi-component input is rejected (requireScalar); a valid scalar input produces a
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
// ITK-sourced real-image golden -- duplicates ITKCurvatureFlowImageTest.cpp on our ITK-free filter (RA-Float.nrrd ->
// float32, SameAsInput). The durable golden is a committed baseline at a tolerance of 0.0001 via
// ip_golden::CompareImages. The helper pins ForceInCore.
//   defaults: the filter defaults (TimeStep 0.05, NumberOfIterations 5). They agree with the ITK defaults.
//   longer:   TimeStep 0.1, NumberOfIterations 10.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][CurvatureFlowImageFilter]")
{
  rt::RunDistanceMapItkGoldenBaseline<CurvatureFlowImageFilter>("RA-Float.nrrd", "BasicFilters_CurvatureFlowImageFilter_defaults.nrrd", /*tolerance=*/0.0001);
}

TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: ITK real-image golden (longer)", "[ImageProcessing][ItkGolden][CurvatureFlowImageFilter]")
{
  rt::RunDistanceMapItkGoldenBaseline<CurvatureFlowImageFilter>("RA-Float.nrrd", "BasicFilters_CurvatureFlowImageFilter_longer.nrrd", /*tolerance=*/0.0001, [](Arguments& args) {
    args.insertOrAssign(CurvatureFlowImageFilter::k_TimeStep_Key, std::make_any<float64>(0.1));
    args.insertOrAssign(CurvatureFlowImageFilter::k_NumberOfIterations_Key, std::make_any<uint32>(10u));
  });
}

// -----------------------------------------------------------------------------
// (2) NumberOfIterations == 0 is a no-op: the driver's `for(iter=0; iter<numberOfIterations; ...)` loop never runs, and
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
