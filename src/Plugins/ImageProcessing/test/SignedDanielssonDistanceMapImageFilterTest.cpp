#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/SignedDanielssonDistanceMapImageFilter.hpp"

#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <string>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
using SignedDanielssonFilter = SignedDanielssonDistanceMapImageFilter;

// Sets the geometry, input, and output keys plus the three parameters on the Arguments. The helper runs preflight
// and execute, and requires that both steps succeed.
void RunSignedDanielsson(IFilter& filter, DataStructure& ds, const DataPath& inputPath, bool insideIsPositive, bool squaredDistance, bool useImageSpacing,
                         UnitTest::AlgorithmTestScope* scope = nullptr, const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(SignedDanielssonFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(SignedDanielssonFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(SignedDanielssonFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(SignedDanielssonFilter::k_InsideIsPositive_Key, std::make_any<bool>(insideIsPositive));
  args.insertOrAssign(SignedDanielssonFilter::k_SquaredDistance_Key, std::make_any<bool>(squaredDistance));
  args.insertOrAssign(SignedDanielssonFilter::k_UseImageSpacing_Key, std::make_any<bool>(useImageSpacing));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope == nullptr ? filter.execute(ds, args) : scope->executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Sign orientation: a filled cube object in a background field. With InsideIsPositive Off, a voxel well outside
//     the object is positive and a voxel deep inside is negative; On flips both. Guards against a flipped subtract.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SignedDanielssonDistanceMapImageFilter: sign orientation", "[ImageProcessing][SignedDanielssonDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize D = 11;
  std::vector<int32> pattern(D * D * D, 0);
  for(usize z = 3; z <= 7; ++z)
  {
    for(usize y = 3; y <= 7; ++y)
    {
      for(usize x = 3; x <= 7; ++x)
      {
        pattern[rt::FlatIndex(x, y, z, D, D)] = 1; // filled 5x5x5 cube object
      }
    }
  }
  const usize cornerIdx = rt::FlatIndex(0, 0, 0, D, D); // well outside the object
  const usize centerIdx = rt::FlatIndex(5, 5, 5, D, D); // deep inside the object
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));

  auto runSign = [&](bool insideIsPositive) {
    UnitTest::AlgorithmTestScope scope(scenario);
    DataStructure ds;
    const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, D, D, D, pattern);
    scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));
    SignedDanielssonFilter filter;
    RunSignedDanielsson(filter, ds, inputPath, insideIsPositive, /*squaredDistance=*/false, /*useImageSpacing=*/false, &scope);
    const auto& out = ds.getDataRefAs<DataArray<float32>>(DataPath({"Image Geometry", "CellData", "Output"})).getDataStoreRef();
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
TEST_CASE("ImageProcessing::SignedDanielssonDistanceMapImageFilter: sign orientation (both algorithm paths)", "[ImageProcessing][SignedDanielssonDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths on in-memory stores; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  constexpr usize D = 11;
  std::vector<int32> pattern(D * D * D, 0);
  for(usize z = 3; z <= 7; ++z)
  {
    for(usize y = 3; y <= 7; ++y)
    {
      for(usize x = 3; x <= 7; ++x)
      {
        pattern[rt::FlatIndex(x, y, z, D, D)] = 1; // filled 5x5x5 cube object
      }
    }
  }
  const usize cornerIdx = rt::FlatIndex(0, 0, 0, D, D); // well outside the object
  const usize centerIdx = rt::FlatIndex(5, 5, 5, D, D); // deep inside the object

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, D, D, D, pattern);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));

  SignedDanielssonFilter filter;
  Arguments args;
  args.insertOrAssign(SignedDanielssonFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
  args.insertOrAssign(SignedDanielssonFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(SignedDanielssonFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(SignedDanielssonFilter::k_InsideIsPositive_Key, std::make_any<bool>(false));
  args.insertOrAssign(SignedDanielssonFilter::k_SquaredDistance_Key, std::make_any<bool>(false));
  args.insertOrAssign(SignedDanielssonFilter::k_UseImageSpacing_Key, std::make_any<bool>(false));

  auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = ds.getDataRefAs<DataArray<float32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& out = outArray.getDataStoreRef();
  REQUIRE(out.getValue(cornerIdx) > 0.0f); // outside is positive when InsideIsPositive is Off
  REQUIRE(out.getValue(centerIdx) < 0.0f); // inside is negative
  UnitTest::CheckArraysInheritTupleDims(ds);
}

// -----------------------------------------------------------------------------
// (2) Preflight: a non-integer (float32) input is rejected by the IntegerOnly policy.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SignedDanielssonDistanceMapImageFilter: preflight rejects non-integer input", "[ImageProcessing][SignedDanielssonDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 6, DY = 6, DZ = 1;
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, DX, DY, DZ, std::vector<float32>(DX * DY * DZ, 1.0f));
  const DataPath geomPath = inputPath.getParent().getParent();

  SignedDanielssonFilter filter;
  Arguments args;
  args.insertOrAssign(SignedDanielssonFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(SignedDanielssonFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(SignedDanielssonFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(SignedDanielssonFilter::k_InsideIsPositive_Key, std::make_any<bool>(false));
  args.insertOrAssign(SignedDanielssonFilter::k_SquaredDistance_Key, std::make_any<bool>(true));
  args.insertOrAssign(SignedDanielssonFilter::k_UseImageSpacing_Key, std::make_any<bool>(false));
  auto result = filter.preflight(ds, args);
  REQUIRE(result.outputActions.invalid()); // float32 is not an IntegerOnly type
}

TEST_CASE("ImageProcessing::SignedDanielssonDistanceMapImageFilter: preflight rejects non-scalar and tuple mismatch", "[ImageProcessing][SignedDanielssonDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure ds;
  const DataPath input = rt::BuildImageFromPattern<int32>(ds, 6, 6, 1, std::vector<int32>(36, 0));
  const DataPath geomPath = input.getParent().getParent();
  SignedDanielssonFilter filter;
  auto argsFor = [&](const DataPath& path) {
    Arguments args;
    args.insertOrAssign(SignedDanielssonFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(SignedDanielssonFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(path));
    args.insertOrAssign(SignedDanielssonFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(SignedDanielssonFilter::k_InsideIsPositive_Key, std::make_any<bool>(false));
    args.insertOrAssign(SignedDanielssonFilter::k_SquaredDistance_Key, std::make_any<bool>(true));
    args.insertOrAssign(SignedDanielssonFilter::k_UseImageSpacing_Key, std::make_any<bool>(false));
    return args;
  };
  SECTION("non-scalar")
  {
    const DataPath path({"Image Geometry", "CellData", "Vec"});
    auto store = DataStoreUtilities::CreateDataStore<int32>(ds, path, {1, 6, 6}, {3});
    DataArray<int32>::Create(ds, "Vec", store, ds.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"})).getId());
    const auto result = filter.preflight(ds, argsFor(path));
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == -8550);
  }
  SECTION("tuple mismatch")
  {
    const auto& geom = ds.getDataRefAs<ImageGeom>(geomPath);
    auto* wrongAM = AttributeMatrix::Create(ds, "WrongCellData", {35}, geom.getId());
    const DataPath path({"Image Geometry", "WrongCellData", "Short"});
    auto store = DataStoreUtilities::CreateDataStore<int32>(ds, path, {35}, {1});
    DataArray<int32>::Create(ds, "Short", store, wrongAM->getId());
    const auto result = filter.preflight(ds, argsFor(path));
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == -8001);
  }
}

// FromSIMPLJson maps every legacy SIMPL key (InsideIsPositive/SquaredDistance/UseImageSpacing + both DataPaths).
TEST_CASE("ImageProcessing::SignedDanielssonDistanceMapImageFilter: FromSIMPLJson", "[ImageProcessing][SignedDanielssonDistanceMapImageFilter]")
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
  const Arguments args = result.value();
  REQUIRE(args.value<bool>(SignedDanielssonFilter::k_InsideIsPositive_Key) == true);
  REQUIRE(args.value<bool>(SignedDanielssonFilter::k_SquaredDistance_Key) == true);
  REQUIRE(args.value<bool>(SignedDanielssonFilter::k_UseImageSpacing_Key) == true);
  REQUIRE(args.value<DataPath>(SignedDanielssonFilter::k_InputImageGeomPath_Key) == DataPath({"ImageDC"}));
  REQUIRE(args.value<DataPath>(SignedDanielssonFilter::k_InputImageDataPath_Key) == DataPath({"ImageDC", "CellAM", "InArray"}));
  REQUIRE(args.value<std::string>(SignedDanielssonFilter::k_OutputImageArrayName_Key) == "SignedDanielssonOut");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image baseline golden -- duplicates ITKSignedDanielssonDistanceMapImageTest.cpp(default):
// 2th_cthead1.png (uint8), ITK-default params, fixed float32 output. The test compares the output with the durable
// ITK baseline at a tolerance of 0.01.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SignedDanielssonDistanceMapImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][SignedDanielssonDistanceMapImageFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  rt::RunDistanceMapItkGoldenBaseline<SignedDanielssonDistanceMapImageFilter>("2th_cthead1.png", "BasicFilters_SignedDanielssonDistanceMapImageFilter_default.nrrd", 0.01, rt::NoExtraParams(), &scope);
}
