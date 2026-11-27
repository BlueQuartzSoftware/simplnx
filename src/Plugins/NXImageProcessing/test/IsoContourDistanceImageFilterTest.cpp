#include "ReconstructionFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/IsoContourDistanceImageFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Filter/IParameter.hpp"
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
using IsoFilterType = IsoContourDistanceImageFilter;

void RunIso(IFilter& filter, DataStructure& dataStructure, const DataPath& inputPath, float64 levelSetValue, float64 farValue, unit_test::AlgorithmTestScope* scope = nullptr,
            const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(IsoFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(IsoFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(IsoFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(IsoFilterType::k_LevelSetValue_Key, std::make_any<float64>(levelSetValue));
  args.insertOrAssign(IsoFilterType::k_FarValue_Key, std::make_any<float64>(farValue));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = scope == nullptr ? filter.execute(dataStructure, args) : scope->executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Hand-computed 1D crossing: a 5x1x1 ramp {0,1,2,3,4}, levelSet 1.5 -> the crossing is between x=1 and x=2, whose
//     unit-slope signed distances are -0.5 and +0.5; everything else +-farValue.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::IsoContourDistanceImageFilter: hand-computed 1D crossing", "[NXImageProcessing][IsoContourDistanceImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_Dx = 5;
  constexpr usize k_Dy = 1;
  constexpr usize k_Dz = 1;
  const std::vector<int32> field = {0, 1, 2, 3, 4};
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  unit_test::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_Dx, k_Dy, k_Dz, field);
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));
  IsoFilterType filter;
  RunIso(filter, dataStructure, inputPath, 1.5, 10.0, &scope);
  const auto& out = dataStructure.getDataRefAs<DataArray<float32>>(DataPath({"Image Geometry", "CellData", "Output"})).getDataStoreRef();
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
TEST_CASE("NXImageProcessing::IsoContourDistanceImageFilter: preflight accepts float, rejects non-scalar", "[NXImageProcessing][IsoContourDistanceImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  auto runPreflight = [](DataStructure& dataStructure, const DataPath& inputPath) {
    const DataPath geomPath = inputPath.getParent().getParent();
    IsoFilterType filter;
    Arguments args;
    args.insertOrAssign(IsoFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(IsoFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(IsoFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(IsoFilterType::k_LevelSetValue_Key, std::make_any<float64>(0.0));
    args.insertOrAssign(IsoFilterType::k_FarValue_Key, std::make_any<float64>(10.0));
    return filter.preflight(dataStructure, args);
  };

  SECTION("float32 accepted")
  {
    DataStructure dataStructure;
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(dataStructure, 6, 6, 1, std::vector<float32>(36, 1.0f));
    REQUIRE(runPreflight(dataStructure, inputPath).output_actions.valid());
  }
  SECTION("float64 accepted")
  {
    DataStructure dataStructure;
    const DataPath inputPath = rt::BuildImageFromPattern<float64>(dataStructure, 6, 6, 1, std::vector<float64>(36, 1.0));
    REQUIRE(runPreflight(dataStructure, inputPath).output_actions.valid());
  }
  SECTION("multi-component rejected")
  {
    DataStructure dataStructure;
    const DataPath scalarPath = rt::BuildImageFromPattern<int32>(dataStructure, 6, 6, 1, std::vector<int32>(36, 0)); // creates the geometry + CellData
    const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
    auto vecStore = data_store_utilities::CreateDataStore<int32>(dataStructure, vecPath, {1, 6, 6}, {3});
    const auto& cellAM = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
    DataArray<int32>::Create(dataStructure, "Vec", vecStore, cellAM.getId());

    auto result = runPreflight(dataStructure, vecPath);
    REQUIRE(result.output_actions.invalid());
    REQUIRE(result.output_actions.errors().front().code == filter_parameter::constants::k_ValidateTupleShapeValue);
  }
  SECTION("tuple mismatch rejected")
  {
    DataStructure dataStructure;
    const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, 6, 6, 1, std::vector<int32>(36, 0));
    const auto& geom = dataStructure.getDataRefAs<ImageGeom>(inputPath.getParent().getParent());
    auto* wrongAmPtr = AttributeMatrix::Create(dataStructure, "WrongCellData", {35}, geom.getId());
    const DataPath wrongPath({"Image Geometry", "WrongCellData", "Short"});
    auto store = data_store_utilities::CreateDataStore<int32>(dataStructure, wrongPath, {35}, {1});
    DataArray<int32>::Create(dataStructure, "Short", store, wrongAmPtr->getId());
    const auto result = runPreflight(dataStructure, wrongPath);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors().front().code == -8001);
  }
}

// -----------------------------------------------------------------------------
// (3) FromSIMPLJson: every mapped parameter (both doubles + both DataPaths) at non-default values.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::IsoContourDistanceImageFilter: FromSIMPLJson", "[NXImageProcessing][IsoContourDistanceImageFilter]")
{
  const nlohmann::json json = {
      {"LevelSetValue", 3.5},
      {"FarValue", 7.0},
      {"SelectedCellArrayPath", {{"Data Container Name", "ImageDC"}, {"Attribute Matrix Name", "CellAM"}, {"Data Array Name", "InArray"}}},
      {"NewCellArrayName", "IsoOut"},
  };
  const Result<Arguments> result = IsoContourDistanceImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments& args = result.value();
  REQUIRE(args.value<float64>(IsoFilterType::k_LevelSetValue_Key) == 3.5);
  REQUIRE(args.value<float64>(IsoFilterType::k_FarValue_Key) == 7.0);
  REQUIRE(args.value<DataPath>(IsoFilterType::k_InputImageGeomPath_Key) == DataPath({"ImageDC"}));
  REQUIRE(args.value<DataPath>(IsoFilterType::k_InputImageDataPath_Key) == DataPath({"ImageDC", "CellAM", "InArray"}));
  REQUIRE(args.value<std::string>(IsoFilterType::k_OutputImageArrayName_Key) == "IsoOut");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image baseline golden -- duplicates ITKIsoContourDistanceImageTest.cpp(default): 2th_cthead1.png
// (uint8), LevelSetValue 50.0 (FarValue default), fixed float32 output. The test compares the output with the durable
// ITK baseline at a tolerance of 0.0001.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::IsoContourDistanceImageFilter: ITK real-image golden (default)", "[NXImageProcessing][ItkGolden][IsoContourDistanceImageFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  unit_test::AlgorithmTestScope scope(scenario);
  rt::RunDistanceMapItkGoldenBaseline<IsoContourDistanceImageFilter>(
      "2th_cthead1.png", "BasicFilters_IsoContourDistanceImageFilter_default.nrrd", 0.0001,
      [](Arguments& args) { args.insertOrAssign(IsoContourDistanceImageFilter::k_LevelSetValue_Key, std::make_any<float64>(50.0)); }, &scope);
}
