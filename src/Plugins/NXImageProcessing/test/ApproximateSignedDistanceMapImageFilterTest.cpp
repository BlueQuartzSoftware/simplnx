#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/ApproximateSignedDistanceMapImageFilter.hpp"

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
using AsdFilter = ApproximateSignedDistanceMapImageFilter;

void RunAsd(IFilter& filter, DataStructure& ds, const DataPath& inputPath, float64 insideValue, float64 outsideValue, UnitTest::AlgorithmTestScope* scope = nullptr,
            const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(AsdFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(AsdFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(AsdFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(AsdFilter::k_InsideValue_Key, std::make_any<float64>(insideValue));
  args.insertOrAssign(AsdFilter::k_OutsideValue_Key, std::make_any<float64>(outsideValue));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope == nullptr ? filter.execute(ds, args) : scope->executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Sign orientation on a filled square: the interior is negative and the exterior is positive.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ApproximateSignedDistanceMapImageFilter: sign orientation", "[ImageProcessing][ApproximateSignedDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize D = 9;
  std::vector<int32> field(D * D, 0);
  for(usize y = 3; y <= 5; ++y)
  {
    for(usize x = 3; x <= 5; ++x)
    {
      field[rt::FlatIndex(x, y, 0, D, D)] = 1; // filled 3x3 object
    }
  }
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, D, D, 1, field);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));
  AsdFilter filter;
  RunAsd(filter, ds, inputPath, 1.0, 0.0, &scope);
  const auto& out = ds.getDataRefAs<DataArray<float32>>(DataPath({"Image Geometry", "CellData", "Output"})).getDataStoreRef();
  REQUIRE(out.getValue(rt::FlatIndex(4, 4, 0, D, D)) < 0.0f); // deep interior
  REQUIRE(out.getValue(rt::FlatIndex(0, 0, 0, D, D)) > 0.0f); // far exterior corner
}

// -----------------------------------------------------------------------------
// (2) Preflight: a float32 input is rejected (IntegerOnly); a multi-component input is rejected; integer input creates
//     a float32 output.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ApproximateSignedDistanceMapImageFilter: preflight guards", "[ImageProcessing][ApproximateSignedDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  auto runPreflight = [](DataStructure& ds, const DataPath& inputPath) {
    const DataPath geomPath = inputPath.getParent().getParent();
    AsdFilter filter;
    Arguments args;
    args.insertOrAssign(AsdFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(AsdFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(AsdFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(AsdFilter::k_InsideValue_Key, std::make_any<float64>(1.0));
    args.insertOrAssign(AsdFilter::k_OutsideValue_Key, std::make_any<float64>(0.0));
    return filter.preflight(ds, args);
  };

  SECTION("float32 rejected")
  {
    DataStructure ds;
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, 6, 6, 1, std::vector<float32>(36, 1.0f));
    REQUIRE(runPreflight(ds, inputPath).outputActions.invalid());
  }
  SECTION("integer accepted, output float32")
  {
    DataStructure ds;
    const DataPath inputPath = rt::BuildImageFromPattern<uint8>(ds, 6, 6, 1, std::vector<uint8>(36, uint8{1}));
    REQUIRE(runPreflight(ds, inputPath).outputActions.valid());
  }
  SECTION("multi-component rejected")
  {
    DataStructure ds;
    const DataPath scalarPath = rt::BuildImageFromPattern<int32>(ds, 6, 6, 1, std::vector<int32>(36, 0));
    const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
    auto vecStore = DataStoreUtilities::CreateDataStore<int32>(ds, vecPath, {1, 6, 6}, {3});
    const auto& cellAM = ds.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
    DataArray<int32>::Create(ds, "Vec", vecStore, cellAM.getId());
    auto result = runPreflight(ds, vecPath);
    REQUIRE(result.outputActions.invalid());
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("tuple mismatch rejected")
  {
    DataStructure ds;
    const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, 6, 6, 1, std::vector<int32>(36, 0));
    const auto& geom = ds.getDataRefAs<ImageGeom>(inputPath.getParent().getParent());
    auto* wrongAM = AttributeMatrix::Create(ds, "WrongCellData", {35}, geom.getId());
    const DataPath wrongPath({"Image Geometry", "WrongCellData", "Short"});
    auto store = DataStoreUtilities::CreateDataStore<int32>(ds, wrongPath, {35}, {1});
    DataArray<int32>::Create(ds, "Short", store, wrongAM->getId());
    const auto result = runPreflight(ds, wrongPath);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == -8001);
  }
}

// -----------------------------------------------------------------------------
// (3) FromSIMPLJson: every mapped parameter (both doubles + both DataPaths) at non-default values.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ApproximateSignedDistanceMapImageFilter: FromSIMPLJson", "[ImageProcessing][ApproximateSignedDistanceMapImageFilter]")
{
  const nlohmann::json json = {
      {"InsideValue", 3.0},
      {"OutsideValue", 7.0},
      {"SelectedCellArrayPath", {{"Data Container Name", "ImageDC"}, {"Attribute Matrix Name", "CellAM"}, {"Data Array Name", "InArray"}}},
      {"NewCellArrayName", "AsdOut"},
  };
  const Result<Arguments> result = ApproximateSignedDistanceMapImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments args = result.value();
  REQUIRE(args.value<float64>(AsdFilter::k_InsideValue_Key) == 3.0);
  REQUIRE(args.value<float64>(AsdFilter::k_OutsideValue_Key) == 7.0);
  REQUIRE(args.value<DataPath>(AsdFilter::k_InputImageGeomPath_Key) == DataPath({"ImageDC"}));
  REQUIRE(args.value<DataPath>(AsdFilter::k_InputImageDataPath_Key) == DataPath({"ImageDC", "CellAM", "InArray"}));
  REQUIRE(args.value<std::string>(AsdFilter::k_OutputImageArrayName_Key) == "AsdOut");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image baseline golden -- duplicates ITKApproximateSignedDistanceMapImageTest.cpp:
//   default:        2th_cthead1.png (uint8), ITK-default params.
//   modified_parms: 2th_cthead1.png, InsideValue 100, OutsideValue 0.
// The filter writes a float32 output. The test compares the output with the ITK baseline at a tolerance of 0.01.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ApproximateSignedDistanceMapImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][ApproximateSignedDistanceMapImageFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  rt::RunDistanceMapItkGoldenBaseline<ApproximateSignedDistanceMapImageFilter>("2th_cthead1.png", "BasicFilters_ApproximateSignedDistanceMapImageFilter_default.nrrd", 0.01, rt::NoExtraParams(),
                                                                               &scope);
}

TEST_CASE("ImageProcessing::ApproximateSignedDistanceMapImageFilter: ITK real-image golden (modified_parms)", "[ImageProcessing][ItkGolden][ApproximateSignedDistanceMapImageFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  rt::RunDistanceMapItkGoldenBaseline<ApproximateSignedDistanceMapImageFilter>(
      "2th_cthead1.png", "BasicFilters_ApproximateSignedDistanceMapImageFilter_modified_parms.nrrd", 0.01,
      [](Arguments& args) {
        args.insertOrAssign(ApproximateSignedDistanceMapImageFilter::k_InsideValue_Key, std::make_any<float64>(100.0));
        args.insertOrAssign(ApproximateSignedDistanceMapImageFilter::k_OutsideValue_Key, std::make_any<float64>(0.0));
      },
      &scope);
}
