#include "ReconstructionFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/SignedMaurerDistanceMapImageFilter.hpp"

#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/ImageProcessing/MaurerDistanceMapEngine.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <bit>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
using MaurerFilterType = SignedMaurerDistanceMapImageFilter;

class MaurerSentinelBudgetScope
{
public:
  explicit MaurerSentinelBudgetScope(uint64 budget)
  : m_Manager(CacheMemoryBudgetManager::instance())
  , m_PreviousBudget(m_Manager.budgetBytes())
  {
    m_Manager.clear();
    m_Manager.setBudgetBytes(budget);
  }

  ~MaurerSentinelBudgetScope() noexcept
  {
    m_Manager.clear();
    m_Manager.setBudgetBytes(m_PreviousBudget);
  }

  MaurerSentinelBudgetScope(const MaurerSentinelBudgetScope&) = delete;
  MaurerSentinelBudgetScope(MaurerSentinelBudgetScope&&) = delete;
  MaurerSentinelBudgetScope& operator=(const MaurerSentinelBudgetScope&) = delete;
  MaurerSentinelBudgetScope& operator=(MaurerSentinelBudgetScope&&) = delete;

private:
  CacheMemoryBudgetManager& m_Manager;
  uint64 m_PreviousBudget;
};

// Sets the geometry, input, and output keys plus the four Maurer parameters on the Arguments. The helper runs
// preflight and execute, and requires that both steps succeed.
void RunMaurer(IFilter& filter, DataStructure& dataStructure, const DataPath& inputPath, bool insideIsPositive, bool squaredDistance, bool useImageSpacing, float64 backgroundValue,
               UnitTest::AlgorithmTestScope* scope = nullptr, const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(MaurerFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(MaurerFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(MaurerFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(MaurerFilterType::k_InsideIsPositive_Key, std::make_any<bool>(insideIsPositive));
  args.insertOrAssign(MaurerFilterType::k_SquaredDistance_Key, std::make_any<bool>(squaredDistance));
  args.insertOrAssign(MaurerFilterType::k_UseImageSpacing_Key, std::make_any<bool>(useImageSpacing));
  args.insertOrAssign(MaurerFilterType::k_BackgroundValue_Key, std::make_any<float64>(backgroundValue));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope == nullptr ? filter.execute(dataStructure, args) : scope->executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}
} // namespace

TEST_CASE("NXImageProcessing::SignedMaurerDistanceMapImageFilter: algorithm paths agree on in-memory stores", "[NXImageProcessing][SignedMaurerDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  constexpr usize k_DimX = 5;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 7;
  constexpr usize k_ValueCount = k_DimX * k_DimY * k_DimZ;
  const DataPath outputPath({"Image Geometry", "CellData", "Output"});

  for(const std::string& inputCase : {std::string{"hand pattern"}, std::string{"all background"}, std::string{"all foreground"}})
  {
    const std::vector<uint8> pattern =
        inputCase == "hand pattern" ? rt::MakeHolePattern<uint8>(k_DimX, k_DimY, k_DimZ, uint8{0}, uint8{1}) : std::vector<uint8>(k_ValueCount, inputCase == "all foreground" ? uint8{1} : uint8{0});
    for(const bool insideIsPositive : {false, true})
    {
      for(const bool squaredDistance : {true, false})
      {
        CAPTURE(inputCase, insideIsPositive, squaredDistance);
        std::vector<float32> reference(k_ValueCount);
        {
          const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
          DataStructure referenceDataStructure;
          const DataPath referenceInput = rt::BuildImageFromPattern<uint8>(referenceDataStructure, k_DimX, k_DimY, k_DimZ, pattern);
          MaurerFilterType referenceFilter;
          RunMaurer(referenceFilter, referenceDataStructure, referenceInput, insideIsPositive, squaredDistance, false, 0.0);
          const DataArray<float32>* referenceArrayPtr = nullptr;
          REQUIRE_NOTHROW(referenceArrayPtr = &referenceDataStructure.getDataRefAs<DataArray<float32>>(outputPath));
          const Result<> referenceCopyResult = referenceArrayPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(reference.data(), reference.size()));
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
              CacheMemoryBudgetManager& budget_manager;
              uint64 previous_budget;

              ~BudgetRestorer()
              {
                budget_manager.setBudgetBytes(previous_budget);
              }
            };
            const BudgetRestorer budgetRestorer{.budget_manager = budgetManager, .previous_budget = previousBudget};
            budgetManager.clear();
            // The 1,024-byte budget grants less memory than each worker scratch needs.
            // The planner fails, so the default single-worker slab runs.
            budgetManager.setBudgetBytes(1024);

            DataStructure dataStructure;
            const DataPath inputPath = rt::BuildImageFromPattern<uint8>(dataStructure, k_DimX, k_DimY, k_DimZ, pattern);
            const IDataArray* inputArrayPtr = nullptr;
            REQUIRE_NOTHROW(inputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(inputPath));
            scope.requireExpectedStore(*inputArrayPtr);

            MaurerFilterType filter;
            RunMaurer(filter, dataStructure, inputPath, insideIsPositive, squaredDistance, false, 0.0, &scope);

            const DataArray<float32>* outputArrayPtr = nullptr;
            REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<DataArray<float32>>(outputPath));
            scope.requireExpectedStore(*outputArrayPtr);
            std::vector<float32> actual(k_ValueCount);
            const Result<> actualCopyResult = outputArrayPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(actual.data(), actual.size()));
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
// (1) Hand-computed: a single object pixel at the center of a 5x5 background is a boundary feature, so the squared
//     distance is the squared distance to that pixel; the center itself is 0.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::SignedMaurerDistanceMapImageFilter: hand-computed single seed", "[NXImageProcessing][SignedMaurerDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_Dx = 5;
  constexpr usize k_Dy = 5;
  constexpr usize k_Dz = 1;
  std::vector<int32> pattern(k_Dx * k_Dy * k_Dz, 0);
  pattern[rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)] = 1;

  DataStructure dataStructure;
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_Dx, k_Dy, k_Dz, pattern);
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));
  MaurerFilterType filter;
  RunMaurer(filter, dataStructure, inputPath, /*insideIsPositive=*/false, /*squaredDistance=*/true, /*useImageSpacing=*/false, /*backgroundValue=*/0.0, &scope);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& out = dataStructure.getDataRefAs<DataArray<float32>>(outputPath).getDataStoreRef();
  REQUIRE(out.getValue(rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)) == 0.0f); // the seed pixel
  REQUIRE(out.getValue(rt::FlatIndex(0, 2, 0, k_Dx, k_Dy)) == 4.0f); // dx=2
  REQUIRE(out.getValue(rt::FlatIndex(0, 0, 0, k_Dx, k_Dy)) == 8.0f); // dx=2,dy=2
}

// -----------------------------------------------------------------------------
// (1b) Both-algorithm-paths correctness on the hand-computed single-seed image (a single object pixel at the center
//      of a 5x5 background). ExecuteSignedMaurerDistanceMapImageFilter routes through ApplySignedMaurerDistanceMap's
//      DispatchAlgorithm, so AlgorithmTestScope runs the filter under BOTH the in-core and out-of-core algorithm
//      paths on in-memory stores (selected by SIMPLNX_TEST_ALGORITHM_PATH). The squared distance to the single
//      boundary feature is asserted directly -- an independent oracle, not the new filter's own in-core run.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::SignedMaurerDistanceMapImageFilter: hand-computed single seed (both algorithm paths)", "[NXImageProcessing][SignedMaurerDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths on in-memory stores; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  constexpr usize k_Dx = 5;
  constexpr usize k_Dy = 5;
  constexpr usize k_Dz = 1;
  std::vector<int32> pattern(k_Dx * k_Dy * k_Dz, 0);
  pattern[rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)] = 1;

  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_Dx, k_Dy, k_Dz, pattern);
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));

  MaurerFilterType filter;
  Arguments args;
  args.insertOrAssign(MaurerFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
  args.insertOrAssign(MaurerFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(MaurerFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(MaurerFilterType::k_InsideIsPositive_Key, std::make_any<bool>(false));
  args.insertOrAssign(MaurerFilterType::k_SquaredDistance_Key, std::make_any<bool>(true));
  args.insertOrAssign(MaurerFilterType::k_UseImageSpacing_Key, std::make_any<bool>(false));
  args.insertOrAssign(MaurerFilterType::k_BackgroundValue_Key, std::make_any<float64>(0.0));

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = dataStructure.getDataRefAs<DataArray<float32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& out = outArray.getDataStoreRef();
  REQUIRE(out.getValue(rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)) == 0.0f); // the seed pixel
  REQUIRE(out.getValue(rt::FlatIndex(0, 2, 0, k_Dx, k_Dy)) == 4.0f); // dx=2
  REQUIRE(out.getValue(rt::FlatIndex(0, 0, 0, k_Dx, k_Dy)) == 8.0f); // dx=2,dy=2
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
// The 3-D preflight action carries the bounded chunk plan into OOC store creation. True 2-D keeps the default layout.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::SignedMaurerDistanceMapImageFilter: preflight adds a bounded 3D chunk hint", "[NXImageProcessing][SignedMaurerDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  auto& budgetManager = CacheMemoryBudgetManager::instance();
  const uint64 previousBudget = budgetManager.budgetBytes();
  budgetManager.clear();
  budgetManager.setBudgetBytes(64ULL * 1024ULL * 1024ULL);

  auto check = [&](usize dimZ, bool expectHint) {
    constexpr usize k_DimX = 512;
    constexpr usize k_DimY = 512;
    DataStructure dataStructure;
    const DataPath inputPath = rt::BuildImageFromPattern<uint8>(dataStructure, k_DimX, k_DimY, dimZ, std::vector<uint8>(k_DimX * k_DimY * dimZ, 0));
    MaurerFilterType filter;
    Arguments args;
    args.insertOrAssign(MaurerFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
    args.insertOrAssign(MaurerFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(MaurerFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(MaurerFilterType::k_InsideIsPositive_Key, std::make_any<bool>(false));
    args.insertOrAssign(MaurerFilterType::k_SquaredDistance_Key, std::make_any<bool>(true));
    args.insertOrAssign(MaurerFilterType::k_UseImageSpacing_Key, std::make_any<bool>(false));
    args.insertOrAssign(MaurerFilterType::k_BackgroundValue_Key, std::make_any<float64>(0.0));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
    const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
    REQUIRE(actionPtr != nullptr);
    if(!expectHint)
    {
      REQUIRE_FALSE(actionPtr->chunkShapeHint().has_value());
      return;
    }
    const SizeVec3 dims{k_DimX, k_DimY, dimZ};
    auto planResult = ImageProcessing::detail::CreateMaurer3DSlabMemoryPlan<uint8>(dims, 16ULL * 1024ULL * 1024ULL);
    SIMPLNX_RESULT_REQUIRE_VALID(planResult);
    REQUIRE(actionPtr->chunkShapeHint() == ShapeType{1, planResult.value().maxYRows, k_DimX});
  };

  check(128, true);
  check(1, false);
  budgetManager.setBudgetBytes(previousBudget);
}

// -----------------------------------------------------------------------------
// (2) Preflight: a non-integer (float32) input is rejected by the IntegerOnly policy.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::SignedMaurerDistanceMapImageFilter: preflight rejects non-integer input", "[NXImageProcessing][SignedMaurerDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_Dx = 6;
  constexpr usize k_Dy = 6;
  constexpr usize k_Dz = 1;
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(dataStructure, k_Dx, k_Dy, k_Dz, std::vector<float32>(k_Dx * k_Dy * k_Dz, 1.0f));
  const DataPath geomPath = inputPath.getParent().getParent();

  MaurerFilterType filter;
  Arguments args;
  args.insertOrAssign(MaurerFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(MaurerFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(MaurerFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(MaurerFilterType::k_InsideIsPositive_Key, std::make_any<bool>(false));
  args.insertOrAssign(MaurerFilterType::k_SquaredDistance_Key, std::make_any<bool>(true));
  args.insertOrAssign(MaurerFilterType::k_UseImageSpacing_Key, std::make_any<bool>(false));
  args.insertOrAssign(MaurerFilterType::k_BackgroundValue_Key, std::make_any<float64>(0.0));
  auto result = filter.preflight(dataStructure, args);
  REQUIRE(result.outputActions.invalid()); // float32 is not an IntegerOnly type
}

TEST_CASE("NXImageProcessing::SignedMaurerDistanceMapImageFilter: preflight rejects non-scalar and tuple mismatch", "[NXImageProcessing][SignedMaurerDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, 6, 6, 1, std::vector<int32>(36, 0));
  const DataPath geomPath = inputPath.getParent().getParent();
  MaurerFilterType filter;
  auto makeArgs = [&](const DataPath& path) {
    Arguments args;
    args.insertOrAssign(MaurerFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(MaurerFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(path));
    args.insertOrAssign(MaurerFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(MaurerFilterType::k_InsideIsPositive_Key, std::make_any<bool>(false));
    args.insertOrAssign(MaurerFilterType::k_SquaredDistance_Key, std::make_any<bool>(true));
    args.insertOrAssign(MaurerFilterType::k_UseImageSpacing_Key, std::make_any<bool>(false));
    args.insertOrAssign(MaurerFilterType::k_BackgroundValue_Key, std::make_any<float64>(0.0));
    return args;
  };
  SECTION("non-scalar")
  {
    const DataPath path({"Image Geometry", "CellData", "Vec"});
    auto store = DataStoreUtilities::CreateDataStore<int32>(dataStructure, path, {1, 6, 6}, {3});
    DataArray<int32>::Create(dataStructure, "Vec", store, dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"})).getId());
    const auto result = filter.preflight(dataStructure, makeArgs(path));
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == -8550);
  }
  SECTION("tuple mismatch")
  {
    const DataPath path({"Image Geometry", "WrongCellData", "Short"});
    const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(geomPath);
    auto* wrongCellDataPtr = AttributeMatrix::Create(dataStructure, "WrongCellData", {35}, imageGeom.getId());
    REQUIRE(wrongCellDataPtr != nullptr);
    auto store = DataStoreUtilities::CreateDataStore<int32>(dataStructure, path, {35}, {1});
    DataArray<int32>::Create(dataStructure, "Short", store, wrongCellDataPtr->getId());
    const auto result = filter.preflight(dataStructure, makeArgs(path));
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == -8001);
  }
}

// -----------------------------------------------------------------------------
// (3) Preflight validates the Float64 BackgroundValue against the integer input type: in-range integers are accepted,
//     an out-of-range value is rejected (would be a UB float->integer cast at execute), and a fractional value is
//     accepted with a truncation warning.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::SignedMaurerDistanceMapImageFilter: preflight validates BackgroundValue range", "[NXImageProcessing][SignedMaurerDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_Dx = 6;
  constexpr usize k_Dy = 6;
  constexpr usize k_Dz = 1;
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<uint8>(dataStructure, k_Dx, k_Dy, k_Dz, std::vector<uint8>(k_Dx * k_Dy * k_Dz, uint8{1})); // uint8 input: valid range [0, 255]
  const DataPath geomPath = inputPath.getParent().getParent();
  MaurerFilterType filter;

  auto makeArgs = [&](float64 backgroundValue) {
    Arguments args;
    args.insertOrAssign(MaurerFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(MaurerFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(MaurerFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(MaurerFilterType::k_InsideIsPositive_Key, std::make_any<bool>(false));
    args.insertOrAssign(MaurerFilterType::k_SquaredDistance_Key, std::make_any<bool>(true));
    args.insertOrAssign(MaurerFilterType::k_UseImageSpacing_Key, std::make_any<bool>(false));
    args.insertOrAssign(MaurerFilterType::k_BackgroundValue_Key, std::make_any<float64>(backgroundValue));
    return args;
  };

  SECTION("in-range integer background accepted")
  {
    REQUIRE(filter.preflight(dataStructure, makeArgs(0.0)).outputActions.valid());
    REQUIRE(filter.preflight(dataStructure, makeArgs(255.0)).outputActions.valid());
  }
  SECTION("out-of-range background rejected")
  {
    REQUIRE(filter.preflight(dataStructure, makeArgs(300.0)).outputActions.invalid()); // > uint8 max
    REQUIRE(filter.preflight(dataStructure, makeArgs(-1.0)).outputActions.invalid());  // < uint8 min
  }
  SECTION("fractional background accepted with a truncation warning")
  {
    auto result = filter.preflight(dataStructure, makeArgs(2.5));
    REQUIRE(result.outputActions.valid());
    REQUIRE_FALSE(result.outputActions.warnings().empty());
  }
}

// -----------------------------------------------------------------------------
// (4) FromSIMPLJson maps every legacy SIMPL key (the 4 params + both DataPaths) to the new parameter keys.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::SignedMaurerDistanceMapImageFilter: FromSIMPLJson", "[NXImageProcessing][SignedMaurerDistanceMapImageFilter]")
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
  const Arguments& args = result.value();
  REQUIRE(args.value<bool>(MaurerFilterType::k_InsideIsPositive_Key) == true);
  REQUIRE(args.value<bool>(MaurerFilterType::k_SquaredDistance_Key) == false);
  REQUIRE(args.value<bool>(MaurerFilterType::k_UseImageSpacing_Key) == true);
  REQUIRE(args.value<float64>(MaurerFilterType::k_BackgroundValue_Key) == 3.0);
  REQUIRE(args.value<DataPath>(MaurerFilterType::k_InputImageGeomPath_Key) == DataPath({"ImageDC"}));
  REQUIRE(args.value<DataPath>(MaurerFilterType::k_InputImageDataPath_Key) == DataPath({"ImageDC", "CellAM", "InArray"}));
  REQUIRE(args.value<std::string>(MaurerFilterType::k_OutputImageArrayName_Key) == "MaurerOut");
}

// -----------------------------------------------------------------------------
// (5) Preflight rejects a NON-FINITE BackgroundValue (infinity / NaN). The Float64 BackgroundValue is cast to the
//     integer input element type at execute; a non-finite value is an UNDEFINED float->integer conversion, so
//     ValidateOneValueInRange's !std::isfinite guard rejects it up front with the filter-local k_NonFiniteBackgroundValue
//     (-8560). The existing range test covers only out-of-range/fractional finite values, never the non-finite arm.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::SignedMaurerDistanceMapImageFilter: preflight rejects non-finite BackgroundValue", "[NXImageProcessing][SignedMaurerDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_Dx = 6;
  constexpr usize k_Dy = 6;
  constexpr usize k_Dz = 1;
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<uint8>(dataStructure, k_Dx, k_Dy, k_Dz, std::vector<uint8>(k_Dx * k_Dy * k_Dz, uint8{1})); // uint8 input
  const DataPath geomPath = inputPath.getParent().getParent();
  MaurerFilterType filter;

  auto makeArgs = [&](float64 backgroundValue) {
    Arguments args;
    args.insertOrAssign(MaurerFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(MaurerFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(MaurerFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(MaurerFilterType::k_InsideIsPositive_Key, std::make_any<bool>(false));
    args.insertOrAssign(MaurerFilterType::k_SquaredDistance_Key, std::make_any<bool>(true));
    args.insertOrAssign(MaurerFilterType::k_UseImageSpacing_Key, std::make_any<bool>(false));
    args.insertOrAssign(MaurerFilterType::k_BackgroundValue_Key, std::make_any<float64>(backgroundValue));
    return args;
  };

  // k_NonFiniteBackgroundValue is a filter-local anonymous-namespace constant (not exported); assert the literal -8560.
  SECTION("positive infinity rejected")
  {
    const auto result = filter.preflight(dataStructure, makeArgs(std::numeric_limits<float64>::infinity()));
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == -8560);
  }
  SECTION("negative infinity rejected")
  {
    const auto result = filter.preflight(dataStructure, makeArgs(-std::numeric_limits<float64>::infinity()));
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == -8560);
  }
  SECTION("NaN rejected")
  {
    const auto result = filter.preflight(dataStructure, makeArgs(std::numeric_limits<float64>::quiet_NaN()));
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == -8560);
  }
}

// -----------------------------------------------------------------------------
// (6) Preflight BackgroundValue range at the (u)int64 EXCLUSIVE power-of-two upper bound. ValidateOneValueInRange
//      compares the truncated value against upperExclusive = 2^63 (signed 64-bit) computed via std::ldexp, the special
//      64-bit-width bound the other range test (uint8-only) never reaches. A value just ABOVE 2^63 (~9.3e18) is an
//      out-of-range/undefined int64 cast -> rejected with k_BackgroundValueOutOfRange (-8561); a value just BELOW it
//      (~9.2e18, and within int64's true max 9.223e18) is accepted.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::SignedMaurerDistanceMapImageFilter: preflight BackgroundValue int64 range bound", "[NXImageProcessing][SignedMaurerDistanceMapImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_Dx = 4;
  constexpr usize k_Dy = 4;
  constexpr usize k_Dz = 1;
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int64>(dataStructure, k_Dx, k_Dy, k_Dz, std::vector<int64>(k_Dx * k_Dy * k_Dz, int64{1})); // int64 input
  const DataPath geomPath = inputPath.getParent().getParent();
  MaurerFilterType filter;

  auto makeArgs = [&](float64 backgroundValue) {
    Arguments args;
    args.insertOrAssign(MaurerFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(MaurerFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(MaurerFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(MaurerFilterType::k_InsideIsPositive_Key, std::make_any<bool>(false));
    args.insertOrAssign(MaurerFilterType::k_SquaredDistance_Key, std::make_any<bool>(true));
    args.insertOrAssign(MaurerFilterType::k_UseImageSpacing_Key, std::make_any<bool>(false));
    args.insertOrAssign(MaurerFilterType::k_BackgroundValue_Key, std::make_any<float64>(backgroundValue));
    return args;
  };

  SECTION("value above 2^63 rejected as out-of-range")
  {
    const auto result = filter.preflight(dataStructure, makeArgs(9.3e18)); // > 2^63 == 9.2233...e18
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == -8561); // k_BackgroundValueOutOfRange (filter-local)
  }
  SECTION("value below 2^63 accepted")
  {
    REQUIRE(filter.preflight(dataStructure, makeArgs(9.2e18)).outputActions.valid()); // < 2^63 and within int64 max 9.223e18
  }
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image baseline golden -- duplicates ITKSignedMaurerDistanceMapImageTest.cpp(default):
// 2th_cthead1.png (uint8), ITK-default params, fixed float32 output. The test compares the output with the durable
// ITK baseline at a tolerance of 0.01.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::SignedMaurerDistanceMapImageFilter: ITK real-image golden (default)", "[NXImageProcessing][ItkGolden][SignedMaurerDistanceMapImageFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  rt::RunDistanceMapItkGoldenBaseline<SignedMaurerDistanceMapImageFilter>("2th_cthead1.png", "BasicFilters_SignedMaurerDistanceMapImageFilter_default.nrrd", 0.01, rt::NoExtraParams(), &scope);
}

TEST_CASE("ImageProcessing::SignedMaurerDistanceMapImageFilter: Boundary-free sentinel matches ITK sign rules", "[ImageProcessing][SignedMaurerDistanceMapImageFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  const usize shapeIndex = GENERATE(0, 1, 2, 3);
  const bool foreground = GENERATE(false, true);
  const bool insideIsPositive = GENERATE(false, true);
  const bool squared = GENERATE(false, true);
  const uint64 budget = GENERATE(uint64{1024}, uint64{1048576});
  CAPTURE(scenario, shapeIndex, foreground, insideIsPositive, squared, budget);
  const MaurerSentinelBudgetScope budgetScope(budget);
  const std::array<SizeVec3, 4> shapes = {SizeVec3{1, 1, 1}, SizeVec3{2, 3, 4}, SizeVec3{2, 3, 1}, SizeVec3{5, 6, 7}};
  const SizeVec3& dims = shapes[shapeIndex];
  const usize count = dims[0] * dims[1] * dims[2];
  DataStructure dataStructure;
  const std::vector<uint8> input(count, foreground ? uint8{1} : uint8{0});
  const DataPath inputPath = rt::BuildImageFromPattern<uint8>(dataStructure, dims[0], dims[1], dims[2], input);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(DataPath({"Image Geometry"})));
  auto& geometry = dataStructure.getDataRefAs<ImageGeom>(DataPath({"Image Geometry"}));
  REQUIRE(geometry.setSpacing({2, 3, 5}).valid());
  REQUIRE(geometry.setOrigin({10, 20, 30}).valid());
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(inputPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));
  MaurerFilterType filter;
  RunMaurer(filter, dataStructure, inputPath, insideIsPositive, squared, true, 0.0, &scope);
  const DataPath outputPath = inputPath.replaceName("Output");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(outputPath));
  const auto& output = dataStructure.getDataRefAs<Float32Array>(outputPath);
  scope.requireExpectedStore(output);
  constexpr float32 k_Max = std::numeric_limits<float32>::max();
  const float32 root = static_cast<float32>(std::sqrt(static_cast<float64>(k_Max)));
  // ITK keeps +FLT_MAX for squared images without boundaries.
  // For square roots, rows are background/foreground and columns are InsideIsPositive false/true.
  const std::array<std::array<float32, 2>, 2> signTable = {{{root, -root}, {-root, root}}};
  const float32 expected = squared ? k_Max : signTable[foreground ? 1 : 0][insideIsPositive ? 1 : 0];
  for(usize i = 0; i < count; ++i)
  {
    CAPTURE(i);
    REQUIRE(std::bit_cast<uint32>(output[i]) == std::bit_cast<uint32>(expected));
  }
  REQUIRE(CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() == 0);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
