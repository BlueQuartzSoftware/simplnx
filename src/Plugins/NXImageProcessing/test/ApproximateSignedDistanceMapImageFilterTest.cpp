#include "ReconstructionFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/ApproximateSignedDistanceMapImageFilter.hpp"

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
using AsdFilterType = ApproximateSignedDistanceMapImageFilter;

void RunAsd(IFilter& filter, DataStructure& dataStructure, const DataPath& inputPath, float64 insideValue, float64 outsideValue, unit_test::AlgorithmTestScope* scope = nullptr,
            const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(AsdFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(AsdFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(AsdFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(AsdFilterType::k_InsideValue_Key, std::make_any<float64>(insideValue));
  args.insertOrAssign(AsdFilterType::k_OutsideValue_Key, std::make_any<float64>(outsideValue));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = scope == nullptr ? filter.execute(dataStructure, args) : scope->executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Sign orientation on a filled square: the interior is negative and the exterior is positive.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ApproximateSignedDistanceMapImageFilter: sign orientation", "[NXImageProcessing][ApproximateSignedDistanceMapImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_D = 9;
  std::vector<int32> field(k_D * k_D, 0);
  for(usize y = 3; y <= 5; ++y)
  {
    for(usize x = 3; x <= 5; ++x)
    {
      field[rt::FlatIndex(x, y, 0, k_D, k_D)] = 1; // filled 3x3 object
    }
  }
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  unit_test::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_D, k_D, 1, field);
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));
  AsdFilterType filter;
  RunAsd(filter, dataStructure, inputPath, 1.0, 0.0, &scope);
  const auto& out = dataStructure.getDataRefAs<DataArray<float32>>(DataPath({"Image Geometry", "CellData", "Output"})).getDataStoreRef();
  REQUIRE(out.getValue(rt::FlatIndex(4, 4, 0, k_D, k_D)) < 0.0f); // deep interior
  REQUIRE(out.getValue(rt::FlatIndex(0, 0, 0, k_D, k_D)) > 0.0f); // far exterior corner
}

// -----------------------------------------------------------------------------
// (2) Preflight: a float32 input is rejected (IntegerOnly); a multi-component input is rejected; integer input creates
//     a float32 output.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ApproximateSignedDistanceMapImageFilter: preflight guards", "[NXImageProcessing][ApproximateSignedDistanceMapImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  auto runPreflight = [](DataStructure& dataStructure, const DataPath& inputPath) {
    const DataPath geomPath = inputPath.getParent().getParent();
    AsdFilterType filter;
    Arguments args;
    args.insertOrAssign(AsdFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(AsdFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(AsdFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(AsdFilterType::k_InsideValue_Key, std::make_any<float64>(1.0));
    args.insertOrAssign(AsdFilterType::k_OutsideValue_Key, std::make_any<float64>(0.0));
    return filter.preflight(dataStructure, args);
  };

  SECTION("float32 rejected")
  {
    DataStructure dataStructure;
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(dataStructure, 6, 6, 1, std::vector<float32>(36, 1.0f));
    REQUIRE(runPreflight(dataStructure, inputPath).output_actions.invalid());
  }
  SECTION("integer accepted, output float32")
  {
    DataStructure dataStructure;
    const DataPath inputPath = rt::BuildImageFromPattern<uint8>(dataStructure, 6, 6, 1, std::vector<uint8>(36, uint8{1}));
    REQUIRE(runPreflight(dataStructure, inputPath).output_actions.valid());
  }
  SECTION("multi-component rejected")
  {
    DataStructure dataStructure;
    const DataPath scalarPath = rt::BuildImageFromPattern<int32>(dataStructure, 6, 6, 1, std::vector<int32>(36, 0));
    const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
    auto vecStore = data_store_utilities::CreateDataStore<int32>(dataStructure, vecPath, {1, 6, 6}, {3});
    const auto& cellAM = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
    DataArray<int32>::Create(dataStructure, "Vec", vecStore, cellAM.getId());
    auto result = runPreflight(dataStructure, vecPath);
    REQUIRE(result.output_actions.invalid());
    REQUIRE(result.output_actions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
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
TEST_CASE("NXImageProcessing::ApproximateSignedDistanceMapImageFilter: FromSIMPLJson", "[NXImageProcessing][ApproximateSignedDistanceMapImageFilter]")
{
  const nlohmann::json json = {
      {"InsideValue", 3.0},
      {"OutsideValue", 7.0},
      {"SelectedCellArrayPath", {{"Data Container Name", "ImageDC"}, {"Attribute Matrix Name", "CellAM"}, {"Data Array Name", "InArray"}}},
      {"NewCellArrayName", "AsdOut"},
  };
  const Result<Arguments> result = ApproximateSignedDistanceMapImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments& args = result.value();
  REQUIRE(args.value<float64>(AsdFilterType::k_InsideValue_Key) == 3.0);
  REQUIRE(args.value<float64>(AsdFilterType::k_OutsideValue_Key) == 7.0);
  REQUIRE(args.value<DataPath>(AsdFilterType::k_InputImageGeomPath_Key) == DataPath({"ImageDC"}));
  REQUIRE(args.value<DataPath>(AsdFilterType::k_InputImageDataPath_Key) == DataPath({"ImageDC", "CellAM", "InArray"}));
  REQUIRE(args.value<std::string>(AsdFilterType::k_OutputImageArrayName_Key) == "AsdOut");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image baseline golden -- duplicates ITKApproximateSignedDistanceMapImageTest.cpp:
//   default:        2th_cthead1.png (uint8), ITK-default params.
//   modified_parms: 2th_cthead1.png, InsideValue 100, OutsideValue 0.
// The filter writes a float32 output. The test compares the output with the ITK baseline at a tolerance of 0.01.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ApproximateSignedDistanceMapImageFilter: ITK real-image golden (default)", "[NXImageProcessing][ItkGolden][ApproximateSignedDistanceMapImageFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  unit_test::AlgorithmTestScope scope(scenario);
  rt::RunDistanceMapItkGoldenBaseline<ApproximateSignedDistanceMapImageFilter>("2th_cthead1.png", "BasicFilters_ApproximateSignedDistanceMapImageFilter_default.nrrd", 0.01, rt::NoExtraParams(),
                                                                               &scope);
}

TEST_CASE("NXImageProcessing::ApproximateSignedDistanceMapImageFilter: ITK real-image golden (modified_parms)", "[NXImageProcessing][ItkGolden][ApproximateSignedDistanceMapImageFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  unit_test::AlgorithmTestScope scope(scenario);
  rt::RunDistanceMapItkGoldenBaseline<ApproximateSignedDistanceMapImageFilter>(
      "2th_cthead1.png", "BasicFilters_ApproximateSignedDistanceMapImageFilter_modified_parms.nrrd", 0.01,
      [](Arguments& args) {
        args.insertOrAssign(ApproximateSignedDistanceMapImageFilter::k_InsideValue_Key, std::make_any<float64>(100.0));
        args.insertOrAssign(ApproximateSignedDistanceMapImageFilter::k_OutsideValue_Key, std::make_any<float64>(0.0));
      },
      &scope);
}
