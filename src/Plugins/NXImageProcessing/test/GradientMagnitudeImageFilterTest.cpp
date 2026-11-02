#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/GradientMagnitudeImageFilter.hpp"

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
using GradMagFilter = GradientMagnitudeImageFilter;

// Sets the geometry, input, and output keys plus the UseImageSpacing flag on the Arguments. The helper runs preflight
// and execute, and requires that both steps succeed.
void RunGradMag(IFilter& filter, DataStructure& ds, const DataPath& inputPath, bool useImageSpacing, UnitTest::AlgorithmTestScope* algorithmTestScope = nullptr,
                const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(GradMagFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(GradMagFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(GradMagFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(GradMagFilter::k_UseImageSpacing_Key, std::make_any<bool>(useImageSpacing));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeFilter = [&]() { return filter.execute(ds, args); };
  auto executeResult = algorithmTestScope == nullptr ? executeFilter() : algorithmTestScope->execute(executeFilter);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

// A deterministic non-constant pattern with values in [0, 16], representable in EVERY scalar type (incl. uint8/int8) so
// the full 10-type parity grid can reuse it. Varies along all three axes so the gradient magnitude is non-trivial.
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
// (1) Preflight guards: a multi-component input is rejected (requireScalar); a valid scalar input produces a FIXED
//     float32 output array (AlwaysFloat32), regardless of the input element type.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GradientMagnitudeImageFilter: preflight guards", "[ImageProcessing][GradientMagnitudeImageFilter]")
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
    GradMagFilter filter;
    Arguments args;
    args.insertOrAssign(GradMagFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(GradMagFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
    args.insertOrAssign(GradMagFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(GradMagFilter::k_UseImageSpacing_Key, std::make_any<bool>(false));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("scalar input creates a float32 output")
  {
    const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
    CAPTURE(scenario);
    UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
    DataStructure ds;
    const std::vector<uint8> field = MakeRamp<uint8>(6, 6, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<uint8>(ds, 6, 6, 1, field);
    GradMagFilter filter;
    RunGradMag(filter, ds, inputPath, /*useImageSpacing=*/false, &algorithmTestScope);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::float32);
  }
  SECTION("zero spacing rejected when UseImageSpacing is on")
  {
    DataStructure ds;
    const std::vector<uint8> field = MakeRamp<uint8>(6, 6, 6);
    const DataPath inputPath = rt::BuildImageFromPattern<uint8>(ds, 6, 6, 6, field);
    ds.getDataRefAs<ImageGeom>(inputPath.getParent().getParent()).setSpacing(FloatVec3{0.0f, 1.0f, 1.0f});
    GradMagFilter filter;
    const DataPath geomPath = inputPath.getParent().getParent();
    Arguments args;
    args.insertOrAssign(GradMagFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(GradMagFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(GradMagFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(GradMagFilter::k_UseImageSpacing_Key, std::make_any<bool>(true));
    const auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == nx::core::ImageProcessing::k_GradientMagnitudeZeroSpacing);
  }
  SECTION("zero spacing accepted when UseImageSpacing is off (spacing ignored)")
  {
    DataStructure ds;
    const std::vector<uint8> field = MakeRamp<uint8>(6, 6, 6);
    const DataPath inputPath = rt::BuildImageFromPattern<uint8>(ds, 6, 6, 6, field);
    ds.getDataRefAs<ImageGeom>(inputPath.getParent().getParent()).setSpacing(FloatVec3{0.0f, 1.0f, 1.0f});
    GradMagFilter filter;
    RunGradMag(filter, ds, inputPath, /*useImageSpacing=*/false); // spacing ignored -> the degenerate spacing is harmless
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::float32);
  }
}

// -----------------------------------------------------------------------------
// (2) FromSIMPLJson: the UseImageSpacing bool and the geometry/array/name DataPaths.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GradientMagnitudeImageFilter: FromSIMPLJson", "[ImageProcessing][GradientMagnitudeImageFilter]")
{
  const nlohmann::json json = {
      {"UseImageSpacing", true},
      {"SelectedCellArrayPath", {{"Data Container Name", "ImageDC"}, {"Attribute Matrix Name", "CellAM"}, {"Data Array Name", "InArray"}}},
      {"NewCellArrayName", "GradMagOut"},
  };
  const Result<Arguments> result = GradientMagnitudeImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments args = result.value();
  REQUIRE(args.value<bool>(GradMagFilter::k_UseImageSpacing_Key) == true);
  REQUIRE(args.value<DataPath>(GradMagFilter::k_InputImageGeomPath_Key) == DataPath({"ImageDC"}));
  REQUIRE(args.value<DataPath>(GradMagFilter::k_InputImageDataPath_Key) == DataPath({"ImageDC", "CellAM", "InArray"}));
  REQUIRE(args.value<std::string>(GradMagFilter::k_OutputImageArrayName_Key) == "GradMagOut");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- "default" case. Duplicates ITKGradientMagnitudeImageTest.cpp(default) on our
// ITK-free filter: the test reads RA-Float.nrrd (float32) through our reader and runs our filter with the ITK case's
// parameters (all defaults; UseImageSpacing defaults to true). The durable golden compares the output with ITK's
// committed baseline BasicFilters_GradientMagnitudeImageFilter_default.nrrd at the ITK test's tolerance of 1e-05 via
// ip_golden::CompareImages. The output is a fixed float32 (AlwaysFloat32). The helper pins ForceInCore.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GradientMagnitudeImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][GradientMagnitudeImageFilter]")
{
  rt::RunDistanceMapItkGoldenBaseline<GradientMagnitudeImageFilter>("RA-Float.nrrd", "BasicFilters_GradientMagnitudeImageFilter_default.nrrd", /*tolerance=*/1e-05);
}
