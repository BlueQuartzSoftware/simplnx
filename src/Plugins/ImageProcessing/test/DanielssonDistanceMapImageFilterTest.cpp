#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/DanielssonDistanceMapImageFilter.hpp"

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
// Legacy ITKDanielssonDistanceMapImageFilter, created at runtime by UUID (so this target does not link ITKImageProcessing).
const Uuid k_LegacyDanielssonUuid = *Uuid::FromString("f0cd4faf-a676-41ed-9ea5-859035f94836");

using DanielssonFilter = DanielssonDistanceMapImageFilter;

// Set the geom/input/output + 3 Danielsson params on a shared Arguments, run preflight + execute, require both valid.
// The new and legacy filters share identical parameter KEY STRINGS ("input_is_binary", etc.), so DanielssonFilter's
// key constants drive BOTH filters.
void RunDanielsson(IFilter& filter, DataStructure& ds, const DataPath& inputPath, bool inputIsBinary, bool squaredDistance, bool useImageSpacing, UnitTest::AlgorithmTestScope* scope = nullptr,
                   const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(DanielssonFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(DanielssonFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(DanielssonFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(DanielssonFilter::k_InputIsBinary_Key, std::make_any<bool>(inputIsBinary));
  args.insertOrAssign(DanielssonFilter::k_SquaredDistance_Key, std::make_any<bool>(squaredDistance));
  args.insertOrAssign(DanielssonFilter::k_UseImageSpacing_Key, std::make_any<bool>(useImageSpacing));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope == nullptr ? filter.execute(ds, args) : scope->executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

struct DanielssonConfig
{
  std::string label;
  usize dimX;
  usize dimY;
  usize dimZ;
};

const std::vector<DanielssonConfig> k_ParityConfigs = {{"3D 12x12x12", 12, 12, 12}, {"2D 20x16x1", 20, 16, 1}};
} // namespace

// -----------------------------------------------------------------------------
// (1) Live-ITK EXACT parity grid: integer types x {SquaredDistance on/off} x {InputIsBinary on/off} x 3D + 2D on a
//     binary hole-bearing image. The Danielsson distance transform is a fixed float32 output; the new filter must
//     reproduce the legacy ITK output EXACTLY (no tolerance) -- including where 4SED over-estimates the exact EDT.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::DanielssonDistanceMapImageFilter: Live-ITK exact parity grid", "[ImageProcessing][DanielssonDistanceMapImageFilter]", uint8, int16, int32)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyDanielssonUuid) != nullptr);

  const bool inputIsBinary = GENERATE(false, true);
  const bool squaredDistance = GENERATE(false, true);
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(inputIsBinary, squaredDistance, scenario);

  for(const DanielssonConfig& cfg : k_ParityConfigs)
  {
    DYNAMIC_SECTION(cfg.label)
    {
      UnitTest::AlgorithmTestScope scope(scenario);
      // object blobs (value 1) on a background field (value 0); nonzero voxels are the Danielsson features.
      const std::vector<T> pattern = rt::MakeHolePattern<T>(cfg.dimX, cfg.dimY, cfg.dimZ, static_cast<T>(0), static_cast<T>(1));

      DataStructure newDs;
      const DataPath newInput = rt::BuildImageFromPattern<T>(newDs, cfg.dimX, cfg.dimY, cfg.dimZ, pattern);
      scope.requireExpectedStore(newDs.getDataRefAs<IDataArray>(newInput));
      DanielssonFilter newFilter;
      RunDanielsson(newFilter, newDs, newInput, inputIsBinary, squaredDistance, /*useImageSpacing=*/false, &scope);

      IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyDanielssonUuid);
      REQUIRE(legacyFilter != nullptr);
      DataStructure legacyDs;
      const DataPath legacyInput = rt::BuildImageFromPattern<T>(legacyDs, cfg.dimX, cfg.dimY, cfg.dimZ, pattern);
      RunDanielsson(*legacyFilter, legacyDs, legacyInput, inputIsBinary, squaredDistance, /*useImageSpacing=*/false);

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
TEST_CASE("ImageProcessing::DanielssonDistanceMapImageFilter: Live-ITK parity with image spacing", "[ImageProcessing][DanielssonDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyDanielssonUuid) != nullptr);

  constexpr usize DX = 10, DY = 9, DZ = 8;
  const std::vector<uint8> pattern = rt::MakeHolePattern<uint8>(DX, DY, DZ, uint8{0}, uint8{1});
  const FloatVec3 spacing{0.5f, 1.0f, 2.0f};
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<uint8>(newDs, DX, DY, DZ, pattern);
  scope.requireExpectedStore(newDs.getDataRefAs<IDataArray>(newInput));
  newDs.getDataRefAs<ImageGeom>(newInput.getParent().getParent()).setSpacing(spacing);
  DanielssonFilter newFilter;
  RunDanielsson(newFilter, newDs, newInput, /*inputIsBinary=*/false, /*squaredDistance=*/true, /*useImageSpacing=*/true, &scope);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyDanielssonUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<uint8>(legacyDs, DX, DY, DZ, pattern);
  legacyDs.getDataRefAs<ImageGeom>(legacyInput.getParent().getParent()).setSpacing(spacing);
  RunDanielsson(*legacyFilter, legacyDs, legacyInput, /*inputIsBinary=*/false, /*squaredDistance=*/true, /*useImageSpacing=*/true);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  UnitTest::CompareDataArrays<float32>(newDs.getDataRefAs<IDataArray>(outputPath), legacyDs.getDataRefAs<IDataArray>(outputPath));
}

// -----------------------------------------------------------------------------
// (3) Hand-computed: a full seed plane at x==0 makes the distance purely 1D along x (= x, or x^2 squared), which is
//     4SED-exact and hand-obvious.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::DanielssonDistanceMapImageFilter: hand-computed seed plane", "[ImageProcessing][DanielssonDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 6, DY = 4, DZ = 1;
  std::vector<int32> pattern(DX * DY * DZ, 0);
  for(usize y = 0; y < DY; ++y)
  {
    pattern[rt::FlatIndex(0, y, 0, DX, DY)] = 1;
  }

  DataStructure ds;
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));
  DanielssonFilter filter;
  RunDanielsson(filter, ds, inputPath, /*inputIsBinary=*/false, /*squaredDistance=*/true, /*useImageSpacing=*/false, &scope);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& out = ds.getDataRefAs<DataArray<float32>>(outputPath).getDataStoreRef();
  for(usize y = 0; y < DY; ++y)
  {
    for(usize x = 0; x < DX; ++x)
    {
      INFO("x=" << x << " y=" << y);
      REQUIRE(out.getValue(rt::FlatIndex(x, y, 0, DX, DY)) == static_cast<float32>(x * x));
    }
  }
}

// -----------------------------------------------------------------------------
// (3b) Both-algorithm-paths correctness on the hand-computed seed plane (a full seed plane at x==0 makes the squared
//      distance purely 1D along x, = x^2). ExecuteDanielssonDistanceMapImageFilter routes through
//      ApplyDanielssonDistanceMap's DispatchAlgorithm, so AlgorithmTestScope runs the filter under BOTH the in-core
//      and out-of-core algorithm paths on in-memory stores (selected by SIMPLNX_TEST_ALGORITHM_PATH) and the closed
//      form x^2 is asserted directly -- an independent oracle, not the new filter's own in-core run.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::DanielssonDistanceMapImageFilter: hand-computed seed plane (both algorithm paths)", "[ImageProcessing][DanielssonDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths on in-memory stores; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  constexpr usize DX = 6, DY = 4, DZ = 1;
  std::vector<int32> pattern(DX * DY * DZ, 0);
  for(usize y = 0; y < DY; ++y)
  {
    pattern[rt::FlatIndex(0, y, 0, DX, DY)] = 1;
  }

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));

  DanielssonFilter filter;
  Arguments args;
  args.insertOrAssign(DanielssonFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
  args.insertOrAssign(DanielssonFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(DanielssonFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(DanielssonFilter::k_InputIsBinary_Key, std::make_any<bool>(false));
  args.insertOrAssign(DanielssonFilter::k_SquaredDistance_Key, std::make_any<bool>(true));
  args.insertOrAssign(DanielssonFilter::k_UseImageSpacing_Key, std::make_any<bool>(false));

  auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = ds.getDataRefAs<DataArray<float32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& out = outArray.getDataStoreRef();
  for(usize y = 0; y < DY; ++y)
  {
    for(usize x = 0; x < DX; ++x)
    {
      INFO("x=" << x << " y=" << y);
      REQUIRE(out.getValue(rt::FlatIndex(x, y, 0, DX, DY)) == static_cast<float32>(x * x));
    }
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("ImageProcessing::DanielssonDistanceMapImageFilter: InputIsBinary is output-neutral", "[ImageProcessing][DanielssonDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  const std::vector<uint8> pattern = rt::MakeHolePattern<uint8>(8, 7, 1, uint8{0}, uint8{1});
  DataStructure falseDs;
  {
    UnitTest::AlgorithmTestScope falseScope(scenario);
    const DataPath falseInput = rt::BuildImageFromPattern<uint8>(falseDs, 8, 7, 1, pattern);
    falseScope.requireExpectedStore(falseDs.getDataRefAs<IDataArray>(falseInput));
    DanielssonFilter falseFilter;
    RunDanielsson(falseFilter, falseDs, falseInput, false, true, false, &falseScope);
  }
  DataStructure trueDs;
  {
    UnitTest::AlgorithmTestScope trueScope(scenario);
    const DataPath trueInput = rt::BuildImageFromPattern<uint8>(trueDs, 8, 7, 1, pattern);
    trueScope.requireExpectedStore(trueDs.getDataRefAs<IDataArray>(trueInput));
    DanielssonFilter trueFilter;
    RunDanielsson(trueFilter, trueDs, trueInput, true, true, false, &trueScope);
  }
  UnitTest::CompareDataArrays<float32>(falseDs.getDataRefAs<IDataArray>(DataPath({"Image Geometry", "CellData", "Output"})),
                                       trueDs.getDataRefAs<IDataArray>(DataPath({"Image Geometry", "CellData", "Output"})));
}

// -----------------------------------------------------------------------------
// (4) Preflight: a non-integer (float32) input is rejected by the IntegerOnly policy.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::DanielssonDistanceMapImageFilter: preflight rejects non-integer input", "[ImageProcessing][DanielssonDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 6, DY = 6, DZ = 1;
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, DX, DY, DZ, std::vector<float32>(DX * DY * DZ, 1.0f));
  const DataPath geomPath = inputPath.getParent().getParent();

  DanielssonFilter filter;
  Arguments args;
  args.insertOrAssign(DanielssonFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(DanielssonFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(DanielssonFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(DanielssonFilter::k_InputIsBinary_Key, std::make_any<bool>(false));
  args.insertOrAssign(DanielssonFilter::k_SquaredDistance_Key, std::make_any<bool>(true));
  args.insertOrAssign(DanielssonFilter::k_UseImageSpacing_Key, std::make_any<bool>(false));
  auto result = filter.preflight(ds, args);
  REQUIRE(result.outputActions.invalid()); // float32 is not an IntegerOnly type
}

TEST_CASE("ImageProcessing::DanielssonDistanceMapImageFilter: preflight rejects non-scalar and tuple mismatch", "[ImageProcessing][DanielssonDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure ds;
  const DataPath input = rt::BuildImageFromPattern<int32>(ds, 6, 6, 1, std::vector<int32>(36, 0));
  const DataPath geomPath = input.getParent().getParent();
  DanielssonFilter filter;
  auto argsFor = [&](const DataPath& path) {
    Arguments args;
    args.insertOrAssign(DanielssonFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(DanielssonFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(path));
    args.insertOrAssign(DanielssonFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(DanielssonFilter::k_InputIsBinary_Key, std::make_any<bool>(false));
    args.insertOrAssign(DanielssonFilter::k_SquaredDistance_Key, std::make_any<bool>(true));
    args.insertOrAssign(DanielssonFilter::k_UseImageSpacing_Key, std::make_any<bool>(false));
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

// FromSIMPLJson maps every legacy SIMPL key (InputIsBinary/SquaredDistance/UseImageSpacing + both DataPaths).
TEST_CASE("ImageProcessing::DanielssonDistanceMapImageFilter: FromSIMPLJson", "[ImageProcessing][DanielssonDistanceMapImageFilter]")
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
  const Arguments args = result.value();
  REQUIRE(args.value<bool>(DanielssonFilter::k_InputIsBinary_Key) == true);
  REQUIRE(args.value<bool>(DanielssonFilter::k_SquaredDistance_Key) == true);
  REQUIRE(args.value<bool>(DanielssonFilter::k_UseImageSpacing_Key) == true);
  REQUIRE(args.value<DataPath>(DanielssonFilter::k_InputImageGeomPath_Key) == DataPath({"ImageDC"}));
  REQUIRE(args.value<DataPath>(DanielssonFilter::k_InputImageDataPath_Key) == DataPath({"ImageDC", "CellAM", "InArray"}));
  REQUIRE(args.value<std::string>(DanielssonFilter::k_OutputImageArrayName_Key) == "DanielssonOut");
}

// Live-ITK parity on degenerate images: all-zero (no seed features) and all-nonzero (every voxel a feature). Guards
// the no-feature / all-feature paths the hole-pattern grid never hits.
TEST_CASE("ImageProcessing::DanielssonDistanceMapImageFilter: Live-ITK parity edge cases", "[ImageProcessing][DanielssonDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyDanielssonUuid) != nullptr);

  constexpr usize DX = 10, DY = 9, DZ = 8;
  auto parity = [&](const std::vector<uint8>& pattern) {
    IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyDanielssonUuid);
    REQUIRE(legacyFilter != nullptr);
    DataStructure legacyDs;
    const DataPath legacyInput = rt::BuildImageFromPattern<uint8>(legacyDs, DX, DY, DZ, pattern);
    RunDanielsson(*legacyFilter, legacyDs, legacyInput, /*inputIsBinary=*/false, /*squaredDistance=*/true, /*useImageSpacing=*/false);

    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    for(const auto scenario : UnitTest::SelectAlgorithmTestScenariosForInMemoryStores())
    {
      UnitTest::AlgorithmTestScope scope(scenario);
      DataStructure newDs;
      const DataPath newInput = rt::BuildImageFromPattern<uint8>(newDs, DX, DY, DZ, pattern);
      scope.requireExpectedStore(newDs.getDataRefAs<IDataArray>(newInput));
      DanielssonFilter newFilter;
      RunDanielsson(newFilter, newDs, newInput, /*inputIsBinary=*/false, /*squaredDistance=*/true, /*useImageSpacing=*/false, &scope);
      scope.requireExpectedStore(newDs.getDataRefAs<IDataArray>(outputPath));
      UnitTest::CompareDataArrays<float32>(newDs.getDataRefAs<IDataArray>(outputPath), legacyDs.getDataRefAs<IDataArray>(outputPath));
    }
  };

  SECTION("all zero (no features)")
  {
    parity(std::vector<uint8>(DX * DY * DZ, uint8{0}));
  }
  SECTION("all nonzero (every voxel a feature)")
  {
    parity(std::vector<uint8>(DX * DY * DZ, uint8{1}));
  }
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image baseline golden -- duplicates ITKDanielssonDistanceMapImageTest.cpp(default):
// 2th_cthead1.png (uint8), ITK-default params, fixed float32 output. (B) live-ITK at the filter's established
// tolerant (CompareDataArrays<float32>) parity class + (A) durable ITK baseline compare @ 0.01.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::DanielssonDistanceMapImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][DanielssonDistanceMapImageFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  rt::RunDistanceMapItkGoldenBaseline<DanielssonDistanceMapImageFilter>("2th_cthead1.png", "BasicFilters_DanielssonDistanceMapImageFilter_default.nrrd", 0.01, false, rt::NoExtraParams(), &scope);
}
