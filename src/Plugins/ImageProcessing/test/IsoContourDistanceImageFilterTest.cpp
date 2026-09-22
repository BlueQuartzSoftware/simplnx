#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/IsoContourDistanceImageFilter.hpp"

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
using IsoFilter = IsoContourDistanceImageFilter;

void RunIso(IFilter& filter, DataStructure& ds, const DataPath& inputPath, float64 levelSetValue, float64 farValue, UnitTest::AlgorithmTestScope* scope = nullptr,
            const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(IsoFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(IsoFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(IsoFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(IsoFilter::k_LevelSetValue_Key, std::make_any<float64>(levelSetValue));
  args.insertOrAssign(IsoFilter::k_FarValue_Key, std::make_any<float64>(farValue));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope == nullptr ? filter.execute(ds, args) : scope->executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Hand-computed 1D crossing: a 5x1x1 ramp {0,1,2,3,4}, levelSet 1.5 -> the crossing is between x=1 and x=2, whose
//     unit-slope signed distances are -0.5 and +0.5; everything else +-farValue.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::IsoContourDistanceImageFilter: hand-computed 1D crossing", "[ImageProcessing][IsoContourDistanceImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 5, DY = 1, DZ = 1;
  const std::vector<int32> field = {0, 1, 2, 3, 4};
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, field);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));
  IsoFilter filter;
  RunIso(filter, ds, inputPath, 1.5, 10.0, &scope);
  const auto& out = ds.getDataRefAs<DataArray<float32>>(DataPath({"Image Geometry", "CellData", "Output"})).getDataStoreRef();
  REQUIRE(out.getValue(0) == -10.0f);                      // x=0: far below
  REQUIRE(out.getValue(1) == Approx(-0.5f).epsilon(1e-5)); // x=1
  REQUIRE(out.getValue(2) == Approx(0.5f).epsilon(1e-5));  // x=2
  REQUIRE(out.getValue(3) == 10.0f);                       // x=3: far above
  REQUIRE(out.getValue(4) == 10.0f);                       // x=4: far above
}

// -----------------------------------------------------------------------------
// (2) Preflight: multi-component input rejected; float32 AND float64 scalar input ACCEPTED (this filter is NOT
//     integer-only), with the output created float32.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::IsoContourDistanceImageFilter: preflight accepts float, rejects non-scalar", "[ImageProcessing][IsoContourDistanceImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  auto runPreflight = [](DataStructure& ds, const DataPath& inputPath) {
    const DataPath geomPath = inputPath.getParent().getParent();
    IsoFilter filter;
    Arguments args;
    args.insertOrAssign(IsoFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(IsoFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(IsoFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(IsoFilter::k_LevelSetValue_Key, std::make_any<float64>(0.0));
    args.insertOrAssign(IsoFilter::k_FarValue_Key, std::make_any<float64>(10.0));
    return filter.preflight(ds, args);
  };

  SECTION("float32 accepted")
  {
    DataStructure ds;
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, 6, 6, 1, std::vector<float32>(36, 1.0f));
    REQUIRE(runPreflight(ds, inputPath).outputActions.valid());
  }
  SECTION("float64 accepted")
  {
    DataStructure ds;
    const DataPath inputPath = rt::BuildImageFromPattern<float64>(ds, 6, 6, 1, std::vector<float64>(36, 1.0));
    REQUIRE(runPreflight(ds, inputPath).outputActions.valid());
  }
  SECTION("multi-component rejected")
  {
    DataStructure ds;
    const DataPath scalarPath = rt::BuildImageFromPattern<int32>(ds, 6, 6, 1, std::vector<int32>(36, 0)); // creates the geometry + CellData
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
TEST_CASE("ImageProcessing::IsoContourDistanceImageFilter: FromSIMPLJson", "[ImageProcessing][IsoContourDistanceImageFilter]")
{
  const nlohmann::json json = {
      {"LevelSetValue", 3.5},
      {"FarValue", 7.0},
      {"SelectedCellArrayPath", {{"Data Container Name", "ImageDC"}, {"Attribute Matrix Name", "CellAM"}, {"Data Array Name", "InArray"}}},
      {"NewCellArrayName", "IsoOut"},
  };
  const Result<Arguments> result = IsoContourDistanceImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments args = result.value();
  REQUIRE(args.value<float64>(IsoFilter::k_LevelSetValue_Key) == 3.5);
  REQUIRE(args.value<float64>(IsoFilter::k_FarValue_Key) == 7.0);
  REQUIRE(args.value<DataPath>(IsoFilter::k_InputImageGeomPath_Key) == DataPath({"ImageDC"}));
  REQUIRE(args.value<DataPath>(IsoFilter::k_InputImageDataPath_Key) == DataPath({"ImageDC", "CellAM", "InArray"}));
  REQUIRE(args.value<std::string>(IsoFilter::k_OutputImageArrayName_Key) == "IsoOut");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image baseline golden -- duplicates ITKIsoContourDistanceImageTest.cpp(default): 2th_cthead1.png
// (uint8), LevelSetValue 50.0 (FarValue default), fixed float32 output. The test compares the output with the durable
// ITK baseline at a tolerance of 0.0001.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::IsoContourDistanceImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][IsoContourDistanceImageFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  rt::RunDistanceMapItkGoldenBaseline<IsoContourDistanceImageFilter>(
      "2th_cthead1.png", "BasicFilters_IsoContourDistanceImageFilter_default.nrrd", 0.0001,
      [](Arguments& args) { args.insertOrAssign(IsoContourDistanceImageFilter::k_LevelSetValue_Key, std::make_any<float64>(50.0)); }, &scope);
}
