#include "ReconstructionFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/DanielssonDistanceMapImageFilter.hpp"

#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <string>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
using DanielssonFilterType = DanielssonDistanceMapImageFilter;

// Sets the geometry, input, and output keys plus the three Danielsson parameters on the Arguments. The helper runs
// preflight and execute, and requires that both steps succeed.
void RunDanielsson(IFilter& filter, DataStructure& dataStructure, const DataPath& inputPath, bool inputIsBinary, bool squaredDistance, bool useImageSpacing,
                   unit_test::AlgorithmTestScope* scope = nullptr, const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(DanielssonFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(DanielssonFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(DanielssonFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(DanielssonFilterType::k_InputIsBinary_Key, std::make_any<bool>(inputIsBinary));
  args.insertOrAssign(DanielssonFilterType::k_SquaredDistance_Key, std::make_any<bool>(squaredDistance));
  args.insertOrAssign(DanielssonFilterType::k_UseImageSpacing_Key, std::make_any<bool>(useImageSpacing));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = scope == nullptr ? filter.execute(dataStructure, args) : scope->executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Hand-computed: a full seed plane at x==0 makes the distance purely 1D along x (= x, or x^2 squared), which is
//     4SED-exact and hand-obvious.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::DanielssonDistanceMapImageFilter: hand-computed seed plane", "[NXImageProcessing][DanielssonDistanceMapImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_Dx = 6;
  constexpr usize k_Dy = 4;
  constexpr usize k_Dz = 1;
  std::vector<int32> pattern(k_Dx * k_Dy * k_Dz, 0);
  for(usize y = 0; y < k_Dy; ++y)
  {
    pattern[rt::FlatIndex(0, y, 0, k_Dx, k_Dy)] = 1;
  }

  DataStructure dataStructure;
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  unit_test::AlgorithmTestScope scope(scenario);
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_Dx, k_Dy, k_Dz, pattern);
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));
  DanielssonFilterType filter;
  RunDanielsson(filter, dataStructure, inputPath, /*inputIsBinary=*/false, /*squaredDistance=*/true, /*useImageSpacing=*/false, &scope);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& out = dataStructure.getDataRefAs<DataArray<float32>>(outputPath).getDataStoreRef();
  for(usize y = 0; y < k_Dy; ++y)
  {
    for(usize x = 0; x < k_Dx; ++x)
    {
      INFO("x=" << x << " y=" << y);
      REQUIRE(out.getValue(rt::FlatIndex(x, y, 0, k_Dx, k_Dy)) == static_cast<float32>(x * x));
    }
  }
}

// -----------------------------------------------------------------------------
// (1b) Both-algorithm-paths correctness on the hand-computed seed plane (a full seed plane at x==0 makes the squared
//      distance purely 1D along x, = x^2). ExecuteDanielssonDistanceMapImageFilter routes through
//      ApplyDanielssonDistanceMap's DispatchAlgorithm, so AlgorithmTestScope runs the filter under BOTH the in-core
//      and out-of-core algorithm paths on in-memory stores (selected by SIMPLNX_TEST_ALGORITHM_PATH) and the closed
//      form x^2 is asserted directly -- an independent oracle, not the new filter's own in-core run.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::DanielssonDistanceMapImageFilter: hand-computed seed plane (both algorithm paths)", "[NXImageProcessing][DanielssonDistanceMapImageFilter]")
{
  unit_test::LoadPlugins();
  // Test both algorithm paths on in-memory stores; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  constexpr usize k_Dx = 6;
  constexpr usize k_Dy = 4;
  constexpr usize k_Dz = 1;
  std::vector<int32> pattern(k_Dx * k_Dy * k_Dz, 0);
  for(usize y = 0; y < k_Dy; ++y)
  {
    pattern[rt::FlatIndex(0, y, 0, k_Dx, k_Dy)] = 1;
  }

  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_Dx, k_Dy, k_Dz, pattern);
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));

  DanielssonFilterType filter;
  Arguments args;
  args.insertOrAssign(DanielssonFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
  args.insertOrAssign(DanielssonFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(DanielssonFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(DanielssonFilterType::k_InputIsBinary_Key, std::make_any<bool>(false));
  args.insertOrAssign(DanielssonFilterType::k_SquaredDistance_Key, std::make_any<bool>(true));
  args.insertOrAssign(DanielssonFilterType::k_UseImageSpacing_Key, std::make_any<bool>(false));

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = dataStructure.getDataRefAs<DataArray<float32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& out = outArray.getDataStoreRef();
  for(usize y = 0; y < k_Dy; ++y)
  {
    for(usize x = 0; x < k_Dx; ++x)
    {
      INFO("x=" << x << " y=" << y);
      REQUIRE(out.getValue(rt::FlatIndex(x, y, 0, k_Dx, k_Dy)) == static_cast<float32>(x * x));
    }
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::DanielssonDistanceMapImageFilter: InputIsBinary is output-neutral", "[NXImageProcessing][DanielssonDistanceMapImageFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  const std::vector<uint8> pattern = rt::MakeHolePattern<uint8>(8, 7, 1, uint8{0}, uint8{1});
  DataStructure falseDs;
  {
    unit_test::AlgorithmTestScope falseScope(scenario);
    const DataPath falseInput = rt::BuildImageFromPattern<uint8>(falseDs, 8, 7, 1, pattern);
    falseScope.requireExpectedStore(falseDs.getDataRefAs<IDataArray>(falseInput));
    DanielssonFilterType falseFilter;
    RunDanielsson(falseFilter, falseDs, falseInput, false, true, false, &falseScope);
  }
  DataStructure trueDs;
  {
    unit_test::AlgorithmTestScope trueScope(scenario);
    const DataPath trueInput = rt::BuildImageFromPattern<uint8>(trueDs, 8, 7, 1, pattern);
    trueScope.requireExpectedStore(trueDs.getDataRefAs<IDataArray>(trueInput));
    DanielssonFilterType trueFilter;
    RunDanielsson(trueFilter, trueDs, trueInput, true, true, false, &trueScope);
  }
  unit_test::CompareDataArrays<float32>(falseDs.getDataRefAs<IDataArray>(DataPath({"Image Geometry", "CellData", "Output"})),
                                        trueDs.getDataRefAs<IDataArray>(DataPath({"Image Geometry", "CellData", "Output"})));
}

// -----------------------------------------------------------------------------
// (2) Preflight: a non-integer (float32) input is rejected by the IntegerOnly policy.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::DanielssonDistanceMapImageFilter: preflight rejects non-integer input", "[NXImageProcessing][DanielssonDistanceMapImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_Dx = 6;
  constexpr usize k_Dy = 6;
  constexpr usize k_Dz = 1;
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(dataStructure, k_Dx, k_Dy, k_Dz, std::vector<float32>(k_Dx * k_Dy * k_Dz, 1.0f));
  const DataPath geomPath = inputPath.getParent().getParent();

  DanielssonFilterType filter;
  Arguments args;
  args.insertOrAssign(DanielssonFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(DanielssonFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(DanielssonFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(DanielssonFilterType::k_InputIsBinary_Key, std::make_any<bool>(false));
  args.insertOrAssign(DanielssonFilterType::k_SquaredDistance_Key, std::make_any<bool>(true));
  args.insertOrAssign(DanielssonFilterType::k_UseImageSpacing_Key, std::make_any<bool>(false));
  auto result = filter.preflight(dataStructure, args);
  REQUIRE(result.output_actions.invalid()); // float32 is not an IntegerOnly type
}

TEST_CASE("NXImageProcessing::DanielssonDistanceMapImageFilter: preflight rejects non-scalar and tuple mismatch", "[NXImageProcessing][DanielssonDistanceMapImageFilter]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure;
  const DataPath input = rt::BuildImageFromPattern<int32>(dataStructure, 6, 6, 1, std::vector<int32>(36, 0));
  const DataPath geomPath = input.getParent().getParent();
  DanielssonFilterType filter;
  auto argsFor = [&](const DataPath& path) {
    Arguments args;
    args.insertOrAssign(DanielssonFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(DanielssonFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(path));
    args.insertOrAssign(DanielssonFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(DanielssonFilterType::k_InputIsBinary_Key, std::make_any<bool>(false));
    args.insertOrAssign(DanielssonFilterType::k_SquaredDistance_Key, std::make_any<bool>(true));
    args.insertOrAssign(DanielssonFilterType::k_UseImageSpacing_Key, std::make_any<bool>(false));
    return args;
  };
  SECTION("non-scalar")
  {
    const DataPath path({"Image Geometry", "CellData", "Vec"});
    auto store = data_store_utilities::CreateDataStore<int32>(dataStructure, path, {1, 6, 6}, {3});
    DataArray<int32>::Create(dataStructure, "Vec", store, dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"})).getId());
    const auto result = filter.preflight(dataStructure, argsFor(path));
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors().front().code == -8550);
  }
  SECTION("tuple mismatch")
  {
    const auto& geom = dataStructure.getDataRefAs<ImageGeom>(geomPath);
    auto* wrongAmPtr = AttributeMatrix::Create(dataStructure, "WrongCellData", {35}, geom.getId());
    const DataPath path({"Image Geometry", "WrongCellData", "Short"});
    auto store = data_store_utilities::CreateDataStore<int32>(dataStructure, path, {35}, {1});
    DataArray<int32>::Create(dataStructure, "Short", store, wrongAmPtr->getId());
    const auto result = filter.preflight(dataStructure, argsFor(path));
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors().front().code == -8001);
  }
}

// FromSIMPLJson maps every legacy SIMPL key (InputIsBinary/SquaredDistance/UseImageSpacing + both DataPaths).
TEST_CASE("NXImageProcessing::DanielssonDistanceMapImageFilter: FromSIMPLJson", "[NXImageProcessing][DanielssonDistanceMapImageFilter]")
{
  const nlohmann::json json = {
      {"InputIsBinary", true},
      {"SquaredDistance", true},
      {"UseImageSpacing", true},
      {"SelectedCellArrayPath", {{"Data Container Name", "ImageDC"}, {"Attribute Matrix Name", "CellAM"}, {"Data Array Name", "InArray"}}},
      {"NewCellArrayName", "DanielssonOut"},
  };
  const Result<Arguments> result = DanielssonDistanceMapImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments& args = result.value();
  REQUIRE(args.value<bool>(DanielssonFilterType::k_InputIsBinary_Key) == true);
  REQUIRE(args.value<bool>(DanielssonFilterType::k_SquaredDistance_Key) == true);
  REQUIRE(args.value<bool>(DanielssonFilterType::k_UseImageSpacing_Key) == true);
  REQUIRE(args.value<DataPath>(DanielssonFilterType::k_InputImageGeomPath_Key) == DataPath({"ImageDC"}));
  REQUIRE(args.value<DataPath>(DanielssonFilterType::k_InputImageDataPath_Key) == DataPath({"ImageDC", "CellAM", "InArray"}));
  REQUIRE(args.value<std::string>(DanielssonFilterType::k_OutputImageArrayName_Key) == "DanielssonOut");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image baseline golden -- duplicates ITKDanielssonDistanceMapImageTest.cpp(default):
// 2th_cthead1.png (uint8), ITK-default params, fixed float32 output. The test compares the output with the durable
// ITK baseline at a tolerance of 0.01.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::DanielssonDistanceMapImageFilter: ITK real-image golden (default)", "[NXImageProcessing][ItkGolden][DanielssonDistanceMapImageFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  unit_test::AlgorithmTestScope scope(scenario);
  rt::RunDistanceMapItkGoldenBaseline<DanielssonDistanceMapImageFilter>("2th_cthead1.png", "BasicFilters_DanielssonDistanceMapImageFilter_default.nrrd", 0.01, rt::NoExtraParams(), &scope);
}
