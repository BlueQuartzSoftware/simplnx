#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/SignedMaurerDistanceMapImageFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/ImageProcessing/MaurerDistanceMapEngine.hpp"

#include <catch2/catch.hpp>

#include <bit>
#include <limits>
#include <memory>
#include <string>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
// Legacy ITKSignedMaurerDistanceMapImageFilter, created at runtime by UUID (so this target does not link ITKImageProcessing).
const Uuid k_LegacyMaurerUuid = *Uuid::FromString("e81f72d3-e806-4afe-ab4c-795c6a3f526f");

using MaurerFilter = SignedMaurerDistanceMapImageFilter;

// Set the geom/input/output + 4 Maurer params on a shared Arguments, run preflight + execute, require both valid.
// The new and legacy filters share identical parameter KEY STRINGS ("inside_is_positive", etc.), so MaurerFilter's
// key constants drive BOTH filters.
void RunMaurer(IFilter& filter, DataStructure& ds, const DataPath& inputPath, bool insideIsPositive, bool squaredDistance, bool useImageSpacing, float64 backgroundValue,
               UnitTest::AlgorithmTestScope* scope = nullptr, const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(MaurerFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(MaurerFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(MaurerFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(MaurerFilter::k_InsideIsPositive_Key, std::make_any<bool>(insideIsPositive));
  args.insertOrAssign(MaurerFilter::k_SquaredDistance_Key, std::make_any<bool>(squaredDistance));
  args.insertOrAssign(MaurerFilter::k_UseImageSpacing_Key, std::make_any<bool>(useImageSpacing));
  args.insertOrAssign(MaurerFilter::k_BackgroundValue_Key, std::make_any<float64>(backgroundValue));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope == nullptr ? filter.execute(ds, args) : scope->executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

struct MaurerConfig
{
  std::string label;
  usize dimX;
  usize dimY;
  usize dimZ;
};

const std::vector<MaurerConfig> k_ParityConfigs = {{"3D 12x12x12", 12, 12, 12}, {"2D 20x16x1", 20, 16, 1}};
} // namespace

TEST_CASE("ImageProcessing::SignedMaurerDistanceMapImageFilter: algorithm paths agree on in-memory stores", "[ImageProcessing][SignedMaurerDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  constexpr usize dimX = 5;
  constexpr usize dimY = 6;
  constexpr usize dimZ = 7;
  constexpr usize valueCount = dimX * dimY * dimZ;
  const DataPath outputPath({"Image Geometry", "CellData", "Output"});

  for(const std::string& inputCase : {std::string{"hand pattern"}, std::string{"all background"}, std::string{"all foreground"}})
  {
    const std::vector<uint8> pattern =
        inputCase == "hand pattern" ? rt::MakeHolePattern<uint8>(dimX, dimY, dimZ, uint8{0}, uint8{1}) : std::vector<uint8>(valueCount, inputCase == "all foreground" ? uint8{1} : uint8{0});
    for(const bool insideIsPositive : {false, true})
    {
      for(const bool squaredDistance : {true, false})
      {
        CAPTURE(inputCase, insideIsPositive, squaredDistance);
        std::vector<float32> reference(valueCount);
        {
          const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
          DataStructure referenceDataStructure;
          const DataPath referenceInput = rt::BuildImageFromPattern<uint8>(referenceDataStructure, dimX, dimY, dimZ, pattern);
          MaurerFilter referenceFilter;
          RunMaurer(referenceFilter, referenceDataStructure, referenceInput, insideIsPositive, squaredDistance, false, 0.0);
          const DataArray<float32>* referenceArray = nullptr;
          REQUIRE_NOTHROW(referenceArray = &referenceDataStructure.getDataRefAs<DataArray<float32>>(outputPath));
          const Result<> referenceCopyResult = referenceArray->getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(reference.data(), reference.size()));
          SIMPLNX_RESULT_REQUIRE_VALID(referenceCopyResult);
        }

        for(const auto scenario : UnitTest::SelectAlgorithmTestScenariosForInMemoryStores())
        {
          DYNAMIC_SECTION(inputCase << ", insideIsPositive=" << insideIsPositive << ", squaredDistance=" << squaredDistance << ", scenario=" << scenario)
          {
            UnitTest::AlgorithmTestScope scope(scenario);
            auto& budgetManager = CacheMemoryBudgetManager::instance();
            const uint64 previousBudget = budgetManager.budgetBytes();
            struct BudgetRestorer
            {
              CacheMemoryBudgetManager& budgetManager;
              uint64 previousBudget;

              ~BudgetRestorer()
              {
                budgetManager.setBudgetBytes(previousBudget);
              }
            };
            const BudgetRestorer budgetRestorer{budgetManager, previousBudget};
            budgetManager.clear();
            // The 1,024-byte budget grants less memory than each worker scratch needs.
            // The planner fails, so the default single-worker slab runs.
            budgetManager.setBudgetBytes(1024);

            DataStructure dataStructure;
            const DataPath inputPath = rt::BuildImageFromPattern<uint8>(dataStructure, dimX, dimY, dimZ, pattern);
            const IDataArray* inputArray = nullptr;
            REQUIRE_NOTHROW(inputArray = &dataStructure.getDataRefAs<IDataArray>(inputPath));
            scope.requireExpectedStore(*inputArray);

            MaurerFilter filter;
            RunMaurer(filter, dataStructure, inputPath, insideIsPositive, squaredDistance, false, 0.0, &scope);

            const DataArray<float32>* outputArray = nullptr;
            REQUIRE_NOTHROW(outputArray = &dataStructure.getDataRefAs<DataArray<float32>>(outputPath));
            scope.requireExpectedStore(*outputArray);
            std::vector<float32> actual(valueCount);
            const Result<> actualCopyResult = outputArray->getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(actual.data(), actual.size()));
            SIMPLNX_RESULT_REQUIRE_VALID(actualCopyResult);
            for(usize index = 0; index < actual.size(); ++index)
            {
              CAPTURE(index);
              REQUIRE(std::bit_cast<uint32>(actual[index]) == std::bit_cast<uint32>(reference[index]));
            }
            UnitTest::CheckArraysInheritTupleDims(dataStructure);
          }
        }
      }
    }
  }
}

// -----------------------------------------------------------------------------
// This grid compares integer types, distance modes, sign modes, and dimensions against live ITK.
// The input contains binary holes. The float32 output must match exactly.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::SignedMaurerDistanceMapImageFilter: Live-ITK exact parity grid", "[ImageProcessing][SignedMaurerDistanceMapImageFilter]", uint8, int16, int32)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyMaurerUuid) != nullptr);

  const bool insideIsPositive = GENERATE(false, true);
  const bool squaredDistance = GENERATE(false, true);
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(insideIsPositive, squaredDistance, scenario);

  for(const MaurerConfig& cfg : k_ParityConfigs)
  {
    DYNAMIC_SECTION(cfg.label)
    {
      UnitTest::AlgorithmTestScope scope(scenario);
      // object blobs (value 1) on a background field (value 0); BackgroundValue = 0 defines the object.
      const std::vector<T> pattern = rt::MakeHolePattern<T>(cfg.dimX, cfg.dimY, cfg.dimZ, static_cast<T>(0), static_cast<T>(1));

      DataStructure newDs;
      const DataPath newInput = rt::BuildImageFromPattern<T>(newDs, cfg.dimX, cfg.dimY, cfg.dimZ, pattern);
      scope.requireExpectedStore(newDs.getDataRefAs<IDataArray>(newInput));
      MaurerFilter newFilter;
      RunMaurer(newFilter, newDs, newInput, insideIsPositive, squaredDistance, /*useImageSpacing=*/false, /*backgroundValue=*/0.0, &scope);

      IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyMaurerUuid);
      REQUIRE(legacyFilter != nullptr);
      DataStructure legacyDs;
      const DataPath legacyInput = rt::BuildImageFromPattern<T>(legacyDs, cfg.dimX, cfg.dimY, cfg.dimZ, pattern);
      RunMaurer(*legacyFilter, legacyDs, legacyInput, insideIsPositive, squaredDistance, /*useImageSpacing=*/false, /*backgroundValue=*/0.0);

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
TEST_CASE("ImageProcessing::SignedMaurerDistanceMapImageFilter: Live-ITK parity with image spacing", "[ImageProcessing][SignedMaurerDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyMaurerUuid) != nullptr);

  constexpr usize DX = 10, DY = 9, DZ = 8;
  const std::vector<uint8> pattern = rt::MakeHolePattern<uint8>(DX, DY, DZ, uint8{0}, uint8{1});
  const FloatVec3 spacing{0.5f, 1.0f, 2.0f};

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<uint8>(newDs, DX, DY, DZ, pattern);
  scope.requireExpectedStore(newDs.getDataRefAs<IDataArray>(newInput));
  newDs.getDataRefAs<ImageGeom>(newInput.getParent().getParent()).setSpacing(spacing);
  MaurerFilter newFilter;
  RunMaurer(newFilter, newDs, newInput, /*insideIsPositive=*/false, /*squaredDistance=*/true, /*useImageSpacing=*/true, /*backgroundValue=*/0.0, &scope);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyMaurerUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<uint8>(legacyDs, DX, DY, DZ, pattern);
  legacyDs.getDataRefAs<ImageGeom>(legacyInput.getParent().getParent()).setSpacing(spacing);
  RunMaurer(*legacyFilter, legacyDs, legacyInput, /*insideIsPositive=*/false, /*squaredDistance=*/true, /*useImageSpacing=*/true, /*backgroundValue=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  UnitTest::CompareDataArrays<float32>(newDs.getDataRefAs<IDataArray>(outputPath), legacyDs.getDataRefAs<IDataArray>(outputPath));
}

// -----------------------------------------------------------------------------
// (3) Hand-computed: a single object pixel at the center of a 5x5 background is a boundary feature, so the squared
//     distance is the squared distance to that pixel; the center itself is 0.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SignedMaurerDistanceMapImageFilter: hand-computed single seed", "[ImageProcessing][SignedMaurerDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 5, DY = 5, DZ = 1;
  std::vector<int32> pattern(DX * DY * DZ, 0);
  pattern[rt::FlatIndex(2, 2, 0, DX, DY)] = 1;

  DataStructure ds;
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));
  MaurerFilter filter;
  RunMaurer(filter, ds, inputPath, /*insideIsPositive=*/false, /*squaredDistance=*/true, /*useImageSpacing=*/false, /*backgroundValue=*/0.0, &scope);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& out = ds.getDataRefAs<DataArray<float32>>(outputPath).getDataStoreRef();
  REQUIRE(out.getValue(rt::FlatIndex(2, 2, 0, DX, DY)) == 0.0f); // the seed pixel
  REQUIRE(out.getValue(rt::FlatIndex(0, 2, 0, DX, DY)) == 4.0f); // dx=2
  REQUIRE(out.getValue(rt::FlatIndex(0, 0, 0, DX, DY)) == 8.0f); // dx=2,dy=2
}

// -----------------------------------------------------------------------------
// (3b) Both-algorithm-paths correctness on the hand-computed single-seed image (a single object pixel at the center
//      of a 5x5 background). ExecuteSignedMaurerDistanceMapImageFilter routes through ApplySignedMaurerDistanceMap's
//      DispatchAlgorithm, so AlgorithmTestScope runs the filter under BOTH the in-core and out-of-core algorithm
//      paths on in-memory stores (selected by SIMPLNX_TEST_ALGORITHM_PATH). The squared distance to the single
//      boundary feature is asserted directly -- an independent oracle, not the new filter's own in-core run.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SignedMaurerDistanceMapImageFilter: hand-computed single seed (both algorithm paths)", "[ImageProcessing][SignedMaurerDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths on in-memory stores; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  constexpr usize DX = 5, DY = 5, DZ = 1;
  std::vector<int32> pattern(DX * DY * DZ, 0);
  pattern[rt::FlatIndex(2, 2, 0, DX, DY)] = 1;

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));

  MaurerFilter filter;
  Arguments args;
  args.insertOrAssign(MaurerFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
  args.insertOrAssign(MaurerFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(MaurerFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(MaurerFilter::k_InsideIsPositive_Key, std::make_any<bool>(false));
  args.insertOrAssign(MaurerFilter::k_SquaredDistance_Key, std::make_any<bool>(true));
  args.insertOrAssign(MaurerFilter::k_UseImageSpacing_Key, std::make_any<bool>(false));
  args.insertOrAssign(MaurerFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));

  auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = ds.getDataRefAs<DataArray<float32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& out = outArray.getDataStoreRef();
  REQUIRE(out.getValue(rt::FlatIndex(2, 2, 0, DX, DY)) == 0.0f); // the seed pixel
  REQUIRE(out.getValue(rt::FlatIndex(0, 2, 0, DX, DY)) == 4.0f); // dx=2
  REQUIRE(out.getValue(rt::FlatIndex(0, 0, 0, DX, DY)) == 8.0f); // dx=2,dy=2
  UnitTest::CheckArraysInheritTupleDims(ds);
}

// -----------------------------------------------------------------------------
// The 3-D preflight action carries the bounded chunk plan into OOC store creation. True 2-D keeps the default layout.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SignedMaurerDistanceMapImageFilter: preflight adds a bounded 3D chunk hint", "[ImageProcessing][SignedMaurerDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  auto& budgetManager = CacheMemoryBudgetManager::instance();
  const uint64 previousBudget = budgetManager.budgetBytes();
  budgetManager.clear();
  budgetManager.setBudgetBytes(64ULL * 1024ULL * 1024ULL);

  auto check = [&](usize dimZ, bool expectHint) {
    constexpr usize dimX = 512;
    constexpr usize dimY = 512;
    DataStructure dataStructure;
    const DataPath inputPath = rt::BuildImageFromPattern<uint8>(dataStructure, dimX, dimY, dimZ, std::vector<uint8>(dimX * dimY * dimZ, 0));
    MaurerFilter filter;
    Arguments args;
    args.insertOrAssign(MaurerFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
    args.insertOrAssign(MaurerFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(MaurerFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(MaurerFilter::k_InsideIsPositive_Key, std::make_any<bool>(false));
    args.insertOrAssign(MaurerFilter::k_SquaredDistance_Key, std::make_any<bool>(true));
    args.insertOrAssign(MaurerFilter::k_UseImageSpacing_Key, std::make_any<bool>(false));
    args.insertOrAssign(MaurerFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
    const auto* action = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
    REQUIRE(action != nullptr);
    if(!expectHint)
    {
      REQUIRE_FALSE(action->chunkShapeHint().has_value());
      return;
    }
    const SizeVec3 dims{dimX, dimY, dimZ};
    auto planResult = ImageProcessing::detail::CreateMaurer3DSlabMemoryPlan<uint8>(dims, 16ULL * 1024ULL * 1024ULL);
    SIMPLNX_RESULT_REQUIRE_VALID(planResult);
    REQUIRE(action->chunkShapeHint() == ShapeType{1, planResult.value().maxYRows, dimX});
  };

  check(128, true);
  check(1, false);
  budgetManager.setBudgetBytes(previousBudget);
}

// -----------------------------------------------------------------------------
// (4) Preflight: a non-integer (float32) input is rejected by the IntegerOnly policy.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SignedMaurerDistanceMapImageFilter: preflight rejects non-integer input", "[ImageProcessing][SignedMaurerDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 6, DY = 6, DZ = 1;
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, DX, DY, DZ, std::vector<float32>(DX * DY * DZ, 1.0f));
  const DataPath geomPath = inputPath.getParent().getParent();

  MaurerFilter filter;
  Arguments args;
  args.insertOrAssign(MaurerFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(MaurerFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(MaurerFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(MaurerFilter::k_InsideIsPositive_Key, std::make_any<bool>(false));
  args.insertOrAssign(MaurerFilter::k_SquaredDistance_Key, std::make_any<bool>(true));
  args.insertOrAssign(MaurerFilter::k_UseImageSpacing_Key, std::make_any<bool>(false));
  args.insertOrAssign(MaurerFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
  auto result = filter.preflight(ds, args);
  REQUIRE(result.outputActions.invalid()); // float32 is not an IntegerOnly type
}

TEST_CASE("ImageProcessing::SignedMaurerDistanceMapImageFilter: preflight rejects non-scalar and tuple mismatch", "[ImageProcessing][SignedMaurerDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, 6, 6, 1, std::vector<int32>(36, 0));
  const DataPath geomPath = inputPath.getParent().getParent();
  MaurerFilter filter;
  auto makeArgs = [&](const DataPath& path) {
    Arguments args;
    args.insertOrAssign(MaurerFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(MaurerFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(path));
    args.insertOrAssign(MaurerFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(MaurerFilter::k_InsideIsPositive_Key, std::make_any<bool>(false));
    args.insertOrAssign(MaurerFilter::k_SquaredDistance_Key, std::make_any<bool>(true));
    args.insertOrAssign(MaurerFilter::k_UseImageSpacing_Key, std::make_any<bool>(false));
    args.insertOrAssign(MaurerFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
    return args;
  };
  SECTION("non-scalar")
  {
    const DataPath path({"Image Geometry", "CellData", "Vec"});
    auto store = DataStoreUtilities::CreateDataStore<int32>(ds, path, {1, 6, 6}, {3});
    DataArray<int32>::Create(ds, "Vec", store, ds.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"})).getId());
    const auto result = filter.preflight(ds, makeArgs(path));
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == -8550);
  }
  SECTION("tuple mismatch")
  {
    const DataPath path({"Image Geometry", "WrongCellData", "Short"});
    const auto& imageGeom = ds.getDataRefAs<ImageGeom>(geomPath);
    auto* wrongCellData = AttributeMatrix::Create(ds, "WrongCellData", {35}, imageGeom.getId());
    REQUIRE(wrongCellData != nullptr);
    auto store = DataStoreUtilities::CreateDataStore<int32>(ds, path, {35}, {1});
    DataArray<int32>::Create(ds, "Short", store, wrongCellData->getId());
    const auto result = filter.preflight(ds, makeArgs(path));
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == -8001);
  }
}

// -----------------------------------------------------------------------------
// (6) Preflight validates the Float64 BackgroundValue against the integer input type: in-range integers are accepted,
//     an out-of-range value is rejected (would be a UB float->integer cast at execute), and a fractional value is
//     accepted with a truncation warning.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SignedMaurerDistanceMapImageFilter: preflight validates BackgroundValue range", "[ImageProcessing][SignedMaurerDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 6, DY = 6, DZ = 1;
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<uint8>(ds, DX, DY, DZ, std::vector<uint8>(DX * DY * DZ, uint8{1})); // uint8 input: valid range [0, 255]
  const DataPath geomPath = inputPath.getParent().getParent();
  MaurerFilter filter;

  auto makeArgs = [&](float64 backgroundValue) {
    Arguments args;
    args.insertOrAssign(MaurerFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(MaurerFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(MaurerFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(MaurerFilter::k_InsideIsPositive_Key, std::make_any<bool>(false));
    args.insertOrAssign(MaurerFilter::k_SquaredDistance_Key, std::make_any<bool>(true));
    args.insertOrAssign(MaurerFilter::k_UseImageSpacing_Key, std::make_any<bool>(false));
    args.insertOrAssign(MaurerFilter::k_BackgroundValue_Key, std::make_any<float64>(backgroundValue));
    return args;
  };

  SECTION("in-range integer background accepted")
  {
    REQUIRE(filter.preflight(ds, makeArgs(0.0)).outputActions.valid());
    REQUIRE(filter.preflight(ds, makeArgs(255.0)).outputActions.valid());
  }
  SECTION("out-of-range background rejected")
  {
    REQUIRE(filter.preflight(ds, makeArgs(300.0)).outputActions.invalid()); // > uint8 max
    REQUIRE(filter.preflight(ds, makeArgs(-1.0)).outputActions.invalid());  // < uint8 min
  }
  SECTION("fractional background accepted with a truncation warning")
  {
    auto result = filter.preflight(ds, makeArgs(2.5));
    REQUIRE(result.outputActions.valid());
    REQUIRE_FALSE(result.outputActions.warnings().empty());
  }
}

// -----------------------------------------------------------------------------
// (7) FromSIMPLJson maps every legacy SIMPL key (the 4 params + both DataPaths) to the new parameter keys.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SignedMaurerDistanceMapImageFilter: FromSIMPLJson", "[ImageProcessing][SignedMaurerDistanceMapImageFilter]")
{
  const nlohmann::json json = {
      {"InsideIsPositive", true},
      {"SquaredDistance", false},
      {"UseImageSpacing", true},
      {"BackgroundValue", 3.0},
      {"SelectedCellArrayPath", {{"Data Container Name", "ImageDC"}, {"Attribute Matrix Name", "CellAM"}, {"Data Array Name", "InArray"}}},
      {"NewCellArrayName", "MaurerOut"},
  };
  const Result<Arguments> result = SignedMaurerDistanceMapImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments args = result.value();
  REQUIRE(args.value<bool>(MaurerFilter::k_InsideIsPositive_Key) == true);
  REQUIRE(args.value<bool>(MaurerFilter::k_SquaredDistance_Key) == false);
  REQUIRE(args.value<bool>(MaurerFilter::k_UseImageSpacing_Key) == true);
  REQUIRE(args.value<float64>(MaurerFilter::k_BackgroundValue_Key) == 3.0);
  REQUIRE(args.value<DataPath>(MaurerFilter::k_InputImageGeomPath_Key) == DataPath({"ImageDC"}));
  REQUIRE(args.value<DataPath>(MaurerFilter::k_InputImageDataPath_Key) == DataPath({"ImageDC", "CellAM", "InArray"}));
  REQUIRE(args.value<std::string>(MaurerFilter::k_OutputImageArrayName_Key) == "MaurerOut");
}

// -----------------------------------------------------------------------------
// (8) Live-ITK parity on background/degenerate edge cases the main grid never exercises: a NON-ZERO BackgroundValue
//     (object==1 on a background field of 5), an all-background image (no object -> no boundary features), and an
//     all-foreground image (no background -> no boundary features). Guards the `in != backgroundValue` classification
//     and the no-feature degenerate path.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SignedMaurerDistanceMapImageFilter: Live-ITK parity edge cases", "[ImageProcessing][SignedMaurerDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyMaurerUuid) != nullptr);

  constexpr usize DX = 10, DY = 9, DZ = 8;
  auto parity = [&](const std::vector<uint8>& pattern, float64 backgroundValue) {
    IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyMaurerUuid);
    REQUIRE(legacyFilter != nullptr);
    DataStructure legacyDs;
    const DataPath legacyInput = rt::BuildImageFromPattern<uint8>(legacyDs, DX, DY, DZ, pattern);
    RunMaurer(*legacyFilter, legacyDs, legacyInput, /*insideIsPositive=*/false, /*squaredDistance=*/true, /*useImageSpacing=*/false, backgroundValue);

    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    for(const auto scenario : UnitTest::SelectAlgorithmTestScenariosForInMemoryStores())
    {
      UnitTest::AlgorithmTestScope scope(scenario);
      DataStructure newDs;
      const DataPath newInput = rt::BuildImageFromPattern<uint8>(newDs, DX, DY, DZ, pattern);
      scope.requireExpectedStore(newDs.getDataRefAs<IDataArray>(newInput));
      MaurerFilter newFilter;
      RunMaurer(newFilter, newDs, newInput, /*insideIsPositive=*/false, /*squaredDistance=*/true, /*useImageSpacing=*/false, backgroundValue, &scope);
      scope.requireExpectedStore(newDs.getDataRefAs<IDataArray>(outputPath));
      UnitTest::CompareDataArrays<float32>(newDs.getDataRefAs<IDataArray>(outputPath), legacyDs.getDataRefAs<IDataArray>(outputPath));
    }
  };

  SECTION("non-zero background (object=1 on background=5)")
  {
    parity(rt::MakeHolePattern<uint8>(DX, DY, DZ, uint8{5}, uint8{1}), 5.0);
  }
  SECTION("all background (no object)")
  {
    parity(std::vector<uint8>(DX * DY * DZ, uint8{0}), 0.0);
  }
  SECTION("all foreground (no background)")
  {
    parity(std::vector<uint8>(DX * DY * DZ, uint8{1}), 0.0);
  }
}

// -----------------------------------------------------------------------------
// (9) Preflight rejects a NON-FINITE BackgroundValue (infinity / NaN). The Float64 BackgroundValue is cast to the
//     integer input element type at execute; a non-finite value is an UNDEFINED float->integer conversion, so
//     ValidateOneValueInRange's !std::isfinite guard rejects it up front with the filter-local k_NonFiniteBackgroundValue
//     (-8560). The existing range test covers only out-of-range/fractional finite values, never the non-finite arm.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SignedMaurerDistanceMapImageFilter: preflight rejects non-finite BackgroundValue", "[ImageProcessing][SignedMaurerDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 6, DY = 6, DZ = 1;
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<uint8>(ds, DX, DY, DZ, std::vector<uint8>(DX * DY * DZ, uint8{1})); // uint8 input
  const DataPath geomPath = inputPath.getParent().getParent();
  MaurerFilter filter;

  auto makeArgs = [&](float64 backgroundValue) {
    Arguments args;
    args.insertOrAssign(MaurerFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(MaurerFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(MaurerFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(MaurerFilter::k_InsideIsPositive_Key, std::make_any<bool>(false));
    args.insertOrAssign(MaurerFilter::k_SquaredDistance_Key, std::make_any<bool>(true));
    args.insertOrAssign(MaurerFilter::k_UseImageSpacing_Key, std::make_any<bool>(false));
    args.insertOrAssign(MaurerFilter::k_BackgroundValue_Key, std::make_any<float64>(backgroundValue));
    return args;
  };

  // k_NonFiniteBackgroundValue is a filter-local anonymous-namespace constant (not exported); assert the literal -8560.
  SECTION("positive infinity rejected")
  {
    const auto result = filter.preflight(ds, makeArgs(std::numeric_limits<float64>::infinity()));
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == -8560);
  }
  SECTION("negative infinity rejected")
  {
    const auto result = filter.preflight(ds, makeArgs(-std::numeric_limits<float64>::infinity()));
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == -8560);
  }
  SECTION("NaN rejected")
  {
    const auto result = filter.preflight(ds, makeArgs(std::numeric_limits<float64>::quiet_NaN()));
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == -8560);
  }
}

// -----------------------------------------------------------------------------
// (10) Preflight BackgroundValue range at the (u)int64 EXCLUSIVE power-of-two upper bound. ValidateOneValueInRange
//      compares the truncated value against upperExclusive = 2^63 (signed 64-bit) computed via std::ldexp, the special
//      64-bit-width bound the other range test (uint8-only) never reaches. A value just ABOVE 2^63 (~9.3e18) is an
//      out-of-range/undefined int64 cast -> rejected with k_BackgroundValueOutOfRange (-8561); a value just BELOW it
//      (~9.2e18, and within int64's true max 9.223e18) is accepted.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SignedMaurerDistanceMapImageFilter: preflight BackgroundValue int64 range bound", "[ImageProcessing][SignedMaurerDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 4, DY = 4, DZ = 1;
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int64>(ds, DX, DY, DZ, std::vector<int64>(DX * DY * DZ, int64{1})); // int64 input
  const DataPath geomPath = inputPath.getParent().getParent();
  MaurerFilter filter;

  auto makeArgs = [&](float64 backgroundValue) {
    Arguments args;
    args.insertOrAssign(MaurerFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(MaurerFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(MaurerFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(MaurerFilter::k_InsideIsPositive_Key, std::make_any<bool>(false));
    args.insertOrAssign(MaurerFilter::k_SquaredDistance_Key, std::make_any<bool>(true));
    args.insertOrAssign(MaurerFilter::k_UseImageSpacing_Key, std::make_any<bool>(false));
    args.insertOrAssign(MaurerFilter::k_BackgroundValue_Key, std::make_any<float64>(backgroundValue));
    return args;
  };

  SECTION("value above 2^63 rejected as out-of-range")
  {
    const auto result = filter.preflight(ds, makeArgs(9.3e18)); // > 2^63 == 9.2233...e18
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == -8561); // k_BackgroundValueOutOfRange (filter-local)
  }
  SECTION("value below 2^63 accepted")
  {
    REQUIRE(filter.preflight(ds, makeArgs(9.2e18)).outputActions.valid()); // < 2^63 and within int64 max 9.223e18
  }
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image baseline golden -- duplicates ITKSignedMaurerDistanceMapImageTest.cpp(default):
// 2th_cthead1.png (uint8), ITK-default params, fixed float32 output. (B) live-ITK at the filter's established
// tolerant (CompareDataArrays<float32>) parity class + (A) durable ITK baseline compare @ 0.01.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SignedMaurerDistanceMapImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][SignedMaurerDistanceMapImageFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  rt::RunDistanceMapItkGoldenBaseline<SignedMaurerDistanceMapImageFilter>("2th_cthead1.png", "BasicFilters_SignedMaurerDistanceMapImageFilter_default.nrrd", 0.01, false, rt::NoExtraParams(), &scope);
}
