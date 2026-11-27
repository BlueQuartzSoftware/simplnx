#include "ReconstructionFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/SignedDanielssonDistanceMapImageFilter.hpp"

#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Filter/IParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <string>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
using SignedDanielssonFilterType = SignedDanielssonDistanceMapImageFilter;

// Sets the geometry, input, and output keys plus the three parameters on the Arguments. The helper runs preflight
// and execute, and requires that both steps succeed.
void RunSignedDanielsson(IFilter& filter, DataStructure& dataStructure, const DataPath& inputPath, bool insideIsPositive, bool squaredDistance, bool useImageSpacing,
                         unit_test::AlgorithmTestScope* scope = nullptr, const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(SignedDanielssonFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(SignedDanielssonFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(SignedDanielssonFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(SignedDanielssonFilterType::k_InsideIsPositive_Key, std::make_any<bool>(insideIsPositive));
  args.insertOrAssign(SignedDanielssonFilterType::k_SquaredDistance_Key, std::make_any<bool>(squaredDistance));
  args.insertOrAssign(SignedDanielssonFilterType::k_UseImageSpacing_Key, std::make_any<bool>(useImageSpacing));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = scope == nullptr ? filter.execute(dataStructure, args) : scope->executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Sign orientation: a filled cube object in a background field. With InsideIsPositive Off, a voxel well outside
//     the object is positive and a voxel deep inside is negative; On flips both. Guards against a flipped subtract.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::SignedDanielssonDistanceMapImageFilter: sign orientation", "[NXImageProcessing][SignedDanielssonDistanceMapImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_D = 11;
  std::vector<int32> pattern(k_D * k_D * k_D, 0);
  for(usize z = 3; z <= 7; ++z)
  {
    for(usize y = 3; y <= 7; ++y)
    {
      for(usize x = 3; x <= 7; ++x)
      {
        pattern[rt::FlatIndex(x, y, z, k_D, k_D)] = 1; // filled 5x5x5 cube object
      }
    }
  }
  const usize cornerIdx = rt::FlatIndex(0, 0, 0, k_D, k_D); // well outside the object
  const usize centerIdx = rt::FlatIndex(5, 5, 5, k_D, k_D); // deep inside the object
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));

  auto runSign = [&](bool insideIsPositive) {
    unit_test::AlgorithmTestScope scope(scenario);
    DataStructure dataStructure;
    const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_D, k_D, k_D, pattern);
    scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));
    SignedDanielssonFilterType filter;
    RunSignedDanielsson(filter, dataStructure, inputPath, insideIsPositive, /*squaredDistance=*/false, /*useImageSpacing=*/false, &scope);
    const auto& out = dataStructure.getDataRefAs<DataArray<float32>>(DataPath({"Image Geometry", "CellData", "Output"})).getDataStoreRef();
    return std::make_pair(out.getValue(cornerIdx), out.getValue(centerIdx));
  };

  const auto [cornerOff, centerOff] = runSign(false);
  REQUIRE(cornerOff > 0.0f); // outside is positive when InsideIsPositive is Off
  REQUIRE(centerOff < 0.0f); // inside is negative

  const auto [cornerOn, centerOn] = runSign(true);
  REQUIRE(cornerOn < 0.0f); // flipped
  REQUIRE(centerOn > 0.0f);
}

// -----------------------------------------------------------------------------
// (1b) Both-algorithm-paths correctness on the sign-orientation image (a filled 5x5x5 cube object in an 11^3 field).
//      ExecuteSignedDanielssonDistanceMapImageFilter routes both Danielsson passes through
//      ApplyDanielssonDistanceMap's DispatchAlgorithm, so AlgorithmTestScope runs the filter under BOTH the in-core
//      and out-of-core algorithm paths on in-memory stores (selected by SIMPLNX_TEST_ALGORITHM_PATH). With
//      InsideIsPositive Off, a voxel well outside the object must be positive and a voxel deep inside negative -- a
//      hand-derived sign oracle independent of the new filter's own in-core run.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::SignedDanielssonDistanceMapImageFilter: sign orientation (both algorithm paths)", "[NXImageProcessing][SignedDanielssonDistanceMapImageFilter]")
{
  unit_test::LoadPlugins();
  // Test both algorithm paths on in-memory stores; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  constexpr usize k_D = 11;
  std::vector<int32> pattern(k_D * k_D * k_D, 0);
  for(usize z = 3; z <= 7; ++z)
  {
    for(usize y = 3; y <= 7; ++y)
    {
      for(usize x = 3; x <= 7; ++x)
      {
        pattern[rt::FlatIndex(x, y, z, k_D, k_D)] = 1; // filled 5x5x5 cube object
      }
    }
  }
  const usize cornerIdx = rt::FlatIndex(0, 0, 0, k_D, k_D); // well outside the object
  const usize centerIdx = rt::FlatIndex(5, 5, 5, k_D, k_D); // deep inside the object

  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_D, k_D, k_D, pattern);
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));

  SignedDanielssonFilterType filter;
  Arguments args;
  args.insertOrAssign(SignedDanielssonFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
  args.insertOrAssign(SignedDanielssonFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(SignedDanielssonFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(SignedDanielssonFilterType::k_InsideIsPositive_Key, std::make_any<bool>(false));
  args.insertOrAssign(SignedDanielssonFilterType::k_SquaredDistance_Key, std::make_any<bool>(false));
  args.insertOrAssign(SignedDanielssonFilterType::k_UseImageSpacing_Key, std::make_any<bool>(false));

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = dataStructure.getDataRefAs<DataArray<float32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& out = outArray.getDataStoreRef();
  REQUIRE(out.getValue(cornerIdx) > 0.0f); // outside is positive when InsideIsPositive is Off
  REQUIRE(out.getValue(centerIdx) < 0.0f); // inside is negative
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
// (2) Preflight: a non-integer (float32) input is rejected by the IntegerOnly policy.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::SignedDanielssonDistanceMapImageFilter: preflight rejects non-integer input", "[NXImageProcessing][SignedDanielssonDistanceMapImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_Dx = 6;
  constexpr usize k_Dy = 6;
  constexpr usize k_Dz = 1;
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(dataStructure, k_Dx, k_Dy, k_Dz, std::vector<float32>(k_Dx * k_Dy * k_Dz, 1.0f));
  const DataPath geomPath = inputPath.getParent().getParent();

  SignedDanielssonFilterType filter;
  Arguments args;
  args.insertOrAssign(SignedDanielssonFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(SignedDanielssonFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(SignedDanielssonFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(SignedDanielssonFilterType::k_InsideIsPositive_Key, std::make_any<bool>(false));
  args.insertOrAssign(SignedDanielssonFilterType::k_SquaredDistance_Key, std::make_any<bool>(true));
  args.insertOrAssign(SignedDanielssonFilterType::k_UseImageSpacing_Key, std::make_any<bool>(false));
  auto result = filter.preflight(dataStructure, args);
  REQUIRE(result.output_actions.invalid()); // float32 is not an IntegerOnly type
}

TEST_CASE("NXImageProcessing::SignedDanielssonDistanceMapImageFilter: preflight rejects non-scalar and tuple mismatch", "[NXImageProcessing][SignedDanielssonDistanceMapImageFilter]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure;
  const DataPath input = rt::BuildImageFromPattern<int32>(dataStructure, 6, 6, 1, std::vector<int32>(36, 0));
  const DataPath geomPath = input.getParent().getParent();
  SignedDanielssonFilterType filter;
  auto argsFor = [&](const DataPath& path) {
    Arguments args;
    args.insertOrAssign(SignedDanielssonFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(SignedDanielssonFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(path));
    args.insertOrAssign(SignedDanielssonFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(SignedDanielssonFilterType::k_InsideIsPositive_Key, std::make_any<bool>(false));
    args.insertOrAssign(SignedDanielssonFilterType::k_SquaredDistance_Key, std::make_any<bool>(true));
    args.insertOrAssign(SignedDanielssonFilterType::k_UseImageSpacing_Key, std::make_any<bool>(false));
    return args;
  };
  SECTION("non-scalar")
  {
    const DataPath path({"Image Geometry", "CellData", "Vec"});
    auto store = data_store_utilities::CreateDataStore<int32>(dataStructure, path, {1, 6, 6}, {3});
    DataArray<int32>::Create(dataStructure, "Vec", store, dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"})).getId());
    const auto result = filter.preflight(dataStructure, argsFor(path));
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors().front().code == filter_parameter::constants::k_ValidateTupleShapeValue);
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

// FromSIMPLJson maps every legacy SIMPL key (InsideIsPositive/SquaredDistance/UseImageSpacing + both DataPaths).
TEST_CASE("NXImageProcessing::SignedDanielssonDistanceMapImageFilter: FromSIMPLJson", "[NXImageProcessing][SignedDanielssonDistanceMapImageFilter]")
{
  const nlohmann::json json = {
      {"InsideIsPositive", true},
      {"SquaredDistance", true},
      {"UseImageSpacing", true},
      {"SelectedCellArrayPath", {{"Data Container Name", "ImageDC"}, {"Attribute Matrix Name", "CellAM"}, {"Data Array Name", "InArray"}}},
      {"NewCellArrayName", "SignedDanielssonOut"},
  };
  const Result<Arguments> result = SignedDanielssonDistanceMapImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments& args = result.value();
  REQUIRE(args.value<bool>(SignedDanielssonFilterType::k_InsideIsPositive_Key) == true);
  REQUIRE(args.value<bool>(SignedDanielssonFilterType::k_SquaredDistance_Key) == true);
  REQUIRE(args.value<bool>(SignedDanielssonFilterType::k_UseImageSpacing_Key) == true);
  REQUIRE(args.value<DataPath>(SignedDanielssonFilterType::k_InputImageGeomPath_Key) == DataPath({"ImageDC"}));
  REQUIRE(args.value<DataPath>(SignedDanielssonFilterType::k_InputImageDataPath_Key) == DataPath({"ImageDC", "CellAM", "InArray"}));
  REQUIRE(args.value<std::string>(SignedDanielssonFilterType::k_OutputImageArrayName_Key) == "SignedDanielssonOut");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image baseline golden -- duplicates ITKSignedDanielssonDistanceMapImageTest.cpp(default):
// 2th_cthead1.png (uint8), ITK-default params, fixed float32 output. The test compares the output with the durable
// ITK baseline at a tolerance of 0.01.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::SignedDanielssonDistanceMapImageFilter: ITK real-image golden (default)", "[NXImageProcessing][ItkGolden][SignedDanielssonDistanceMapImageFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  unit_test::AlgorithmTestScope scope(scenario);
  rt::RunDistanceMapItkGoldenBaseline<SignedDanielssonDistanceMapImageFilter>("2th_cthead1.png", "BasicFilters_SignedDanielssonDistanceMapImageFilter_default.nrrd", 0.01, rt::NoExtraParams(), &scope);
}
