#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/SignedDanielssonDistanceMapImageFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
// Legacy ITKSignedDanielssonDistanceMapImageFilter, created at runtime by UUID (so this target does not link ITKImageProcessing).
const Uuid k_LegacySignedDanielssonUuid = *Uuid::FromString("2f42e771-1d84-4468-8991-9a1fad7eb740");

using SignedDanielssonFilter = SignedDanielssonDistanceMapImageFilter;

// Set the geom/input/output + 3 params on a shared Arguments, run preflight + execute, require both valid. The new
// and legacy filters share identical parameter KEY STRINGS ("inside_is_positive", etc.), so the key constants drive
// BOTH filters.
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

struct SignedDanielssonConfig
{
  std::string label;
  usize dimX;
  usize dimY;
  usize dimZ;
};

const std::vector<SignedDanielssonConfig> k_ParityConfigs = {{"3D 12x12x12", 12, 12, 12}, {"2D 20x16x1", 20, 16, 1}};
} // namespace

// -----------------------------------------------------------------------------
// (1) Live-ITK EXACT parity grid: integer types x {SquaredDistance on/off} x {InsideIsPositive on/off} x 3D + 2D on a
//     binary hole-bearing image. The signed Danielsson transform is a fixed float32 output; the new filter must
//     reproduce the legacy ITK output EXACTLY (no tolerance) -- the full invert+dilate+2xDanielsson+subtract composite.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::SignedDanielssonDistanceMapImageFilter: Live-ITK exact parity grid", "[ImageProcessing][SignedDanielssonDistanceMapImageFilter]", uint8, int16, int32)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacySignedDanielssonUuid) != nullptr);

  const bool insideIsPositive = GENERATE(false, true);
  const bool squaredDistance = GENERATE(false, true);
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(insideIsPositive, squaredDistance, scenario);

  for(const SignedDanielssonConfig& cfg : k_ParityConfigs)
  {
    DYNAMIC_SECTION(cfg.label)
    {
      UnitTest::AlgorithmTestScope scope(scenario);
      // object blobs (value 1) on a background field (value 0); nonzero voxels are the object.
      const std::vector<T> pattern = rt::MakeHolePattern<T>(cfg.dimX, cfg.dimY, cfg.dimZ, static_cast<T>(0), static_cast<T>(1));

      DataStructure newDs;
      const DataPath newInput = rt::BuildImageFromPattern<T>(newDs, cfg.dimX, cfg.dimY, cfg.dimZ, pattern);
      scope.requireExpectedStore(newDs.getDataRefAs<IDataArray>(newInput));
      SignedDanielssonFilter newFilter;
      RunSignedDanielsson(newFilter, newDs, newInput, insideIsPositive, squaredDistance, /*useImageSpacing=*/false, &scope);

      IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacySignedDanielssonUuid);
      REQUIRE(legacyFilter != nullptr);
      DataStructure legacyDs;
      const DataPath legacyInput = rt::BuildImageFromPattern<T>(legacyDs, cfg.dimX, cfg.dimY, cfg.dimZ, pattern);
      RunSignedDanielsson(*legacyFilter, legacyDs, legacyInput, insideIsPositive, squaredDistance, /*useImageSpacing=*/false);

      const DataPath outputPath({"Image Geometry", "CellData", "Output"});
      const auto& newOut = newDs.getDataRefAs<IDataArray>(outputPath);
      const auto& legacyOut = legacyDs.getDataRefAs<IDataArray>(outputPath);
      REQUIRE(newOut.getDataType() == legacyOut.getDataType());
      UnitTest::CompareDataArrays<float32>(newOut, legacyOut);
    }
  }
}

// -----------------------------------------------------------------------------
// (2) Live-ITK EXACT parity with UseImageSpacing on a non-unit-spacing geometry (the spacing math path).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SignedDanielssonDistanceMapImageFilter: Live-ITK parity with image spacing", "[ImageProcessing][SignedDanielssonDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacySignedDanielssonUuid) != nullptr);

  constexpr usize DX = 10, DY = 9, DZ = 8;
  const std::vector<uint8> pattern = rt::MakeHolePattern<uint8>(DX, DY, DZ, uint8{0}, uint8{1});
  const FloatVec3 spacing{0.5f, 1.0f, 2.0f};
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<uint8>(newDs, DX, DY, DZ, pattern);
  scope.requireExpectedStore(newDs.getDataRefAs<IDataArray>(newInput));
  newDs.getDataRefAs<ImageGeom>(newInput.getParent().getParent()).setSpacing(spacing);
  SignedDanielssonFilter newFilter;
  RunSignedDanielsson(newFilter, newDs, newInput, /*insideIsPositive=*/false, /*squaredDistance=*/true, /*useImageSpacing=*/true, &scope);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacySignedDanielssonUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<uint8>(legacyDs, DX, DY, DZ, pattern);
  legacyDs.getDataRefAs<ImageGeom>(legacyInput.getParent().getParent()).setSpacing(spacing);
  RunSignedDanielsson(*legacyFilter, legacyDs, legacyInput, /*insideIsPositive=*/false, /*squaredDistance=*/true, /*useImageSpacing=*/true);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  UnitTest::CompareDataArrays<float32>(newDs.getDataRefAs<IDataArray>(outputPath), legacyDs.getDataRefAs<IDataArray>(outputPath));
}

// -----------------------------------------------------------------------------
// (3) Sign orientation: a filled cube object in a background field. With InsideIsPositive Off, a voxel well outside
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
// (3b) Both-algorithm-paths correctness on the sign-orientation image (a filled 5x5x5 cube object in an 11^3 field).
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
// (4) Preflight: a non-integer (float32) input is rejected by the IntegerOnly policy.
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
// 2th_cthead1.png (uint8), ITK-default params, fixed float32 output. (B) live-ITK at the filter's established
// tolerant (CompareDataArrays<float32>) parity class + (A) durable ITK baseline compare @ 0.01.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SignedDanielssonDistanceMapImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][SignedDanielssonDistanceMapImageFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  rt::RunDistanceMapItkGoldenBaseline<SignedDanielssonDistanceMapImageFilter>("2th_cthead1.png", "BasicFilters_SignedDanielssonDistanceMapImageFilter_default.nrrd", 0.01, false, rt::NoExtraParams(),
                                                                              &scope);
}
