#include "IdentifySampleBatchControlUtilities.hpp"

#include "IdentifySampleBatchTestUtilities.hpp"

#include "SimplnxCore/Filters/IdentifySampleFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <filesystem>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>

using namespace nx::core;
using namespace nx::core::UnitTest;
namespace fs = std::filesystem;

namespace
{
/*
 * The non-square fixture detects an incorrect row stride in each Empty2D dispatch.
 * Its 3 by 4 mask has one four-voxel component and one two-voxel component.
 * The filter must retain only the larger component.
 * Rows 0 and 1 are `T T F`. Rows 2 and 3 are `F F T`.
 */
const DataPath k_NonSquareImagePath = DataPath({"Image"});
const DataPath k_NonSquareMaskPath = k_NonSquareImagePath.createChildPath("CellData").createChildPath("Mask");

/**
 * @brief Builds a non-square two-dimensional mask for row-stride tests.
 * @param dims Image dimensions with exactly one axis of size 1.
 * @param useConfiguredStore True to create the mask with the configured store factory.
 * @return A DataStructure with the 3 by 4 component mask.
 */
DataStructure CreateNonSquare2DMaskDataStructure(const SizeVec3& dims, bool useConfiguredStore = false)
{
  DataStructure dataStructure = {};
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_NonSquareImagePath.getTargetName());
  imageGeom->setSpacing(FloatVec3{std::array<float32, 3>{1.0f, 1.0f, 1.0f}});
  imageGeom->setOrigin(FloatVec3{std::array<float32, 3>{0.0f, 0.0f, 0.0f}});
  imageGeom->setDimensions(dims);

  const ShapeType imageShape{dims[0], dims[1], dims[2]};
  AttributeMatrix* cellData = AttributeMatrix::Create(dataStructure, "CellData", imageShape, imageGeom->getId());
  imageGeom->setCellData(*cellData);

  std::shared_ptr<AbstractDataStore<bool>> maskStore;
  if(useConfiguredStore)
  {
    maskStore = DataStoreUtilities::CreateDataStore<bool>(dataStructure, k_NonSquareMaskPath, cellData->getShape(), ShapeType{1});
  }
  else
  {
    maskStore = std::make_shared<BoolDataStore>(cellData->getShape(), ShapeType{1}, std::optional<bool>{});
  }
  BoolArray* mask = BoolArray::Create(dataStructure, "Mask", maskStore, cellData->getId());

  const std::array<bool, 12> values = {true, true, false, true, true, false, false, false, true, false, false, true};
  REQUIRE(mask->getNumberOfTuples() == values.size());
  for(usize i = 0; i < values.size(); i++)
  {
    mask->setValue(i, values[i]);
  }
  return dataStructure;
}

/**
 * @brief Creates IdentifySample arguments for the non-square mask fixture.
 * @return Configured whole-volume arguments without hole filling.
 */
Arguments CreateNonSquareArguments()
{
  Arguments args;
  args.insert(IdentifySampleFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_NonSquareImagePath));
  args.insert(IdentifySampleFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_NonSquareMaskPath));
  args.insert(IdentifySampleFilter::k_FillHoles_Key, std::make_any<bool>(false));
  args.insert(IdentifySampleFilter::k_SliceBySlice_Key, std::make_any<bool>(false));
  args.insert(IdentifySampleFilter::k_SliceBySlicePlane_Key, std::make_any<ChoicesParameter::ValueType>(0));
  return args;
}

/**
 * @brief Executes IdentifySample and verifies that only the larger component remains.
 * @param dataStructure Contains the non-square mask to update.
 */
void RunIdentifySampleAndCheck(DataStructure& dataStructure)
{
  IdentifySampleFilter filter;
  Arguments args = CreateNonSquareArguments();

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const std::array<bool, 12> expected = {true, true, false, true, true, false, false, false, false, false, false, false};
  const DataPath maskPath = k_NonSquareImagePath.createChildPath("CellData").createChildPath("Mask");
  const auto& mask = dataStructure.getDataRefAs<BoolArray>(maskPath);
  REQUIRE(mask.getNumberOfTuples() == expected.size());
  for(usize i = 0; i < expected.size(); i++)
  {
    INFO("linear index " << i);
    REQUIRE(mask[i] == expected[i]);
  }
}

const DataPath k_ExemplarArrayPath = Constants::k_DataContainerPath.createChildPath(Constants::k_CellData).createChildPath("Mask Exemplar");
} // namespace

TEST_CASE("SimplnxCore::IdentifySampleFilter", "[SimplnxCore][IdentifySampleFilter]")
{
  UnitTest::LoadPlugins();

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "identify_sample_v2.tar.gz", "identify_sample_v2");
  using TestArgType = std::tuple<std::string, std::string, std::string>;
  /* clang-format off */
  std::vector<TestArgType> allTestParams = {
    {"sliced", "xy", "fill"},
    {"sliced", "xy", "nofill"},
    {"sliced", "xz", "fill"},
    {"sliced", "xz", "nofill"},
    {"sliced", "yz", "fill"},
    {"sliced", "yz", "nofill"},

    {"whole", "xy", "fill"},
    {"whole", "xy", "nofill"},
    {"whole", "xz", "fill"},
    {"whole", "xz", "nofill"},
    {"whole", "yz", "fill"},
    {"whole", "yz", "nofill"},
  };
  /* clang-format on */
  for(const auto& testParam : allTestParams)
  {
    std::string slice_by_slice = std::get<0>(testParam);
    bool sliceBySlice = slice_by_slice == "sliced";

    std::string slice_plane = std::get<1>(testParam);

    ChoicesParameter::ValueType sliceBySlicePlane = 0;
    if(slice_plane == "xz")
      sliceBySlicePlane = 1;
    else if(slice_plane == "yz")
      sliceBySlicePlane = 2;

    std::string fill_holes = std::get<2>(testParam);
    bool fillHoles = fill_holes == "fill";

    SECTION(fmt::format("{}_{}_{}", slice_by_slice, slice_plane, fill_holes))
    {
      fs::path inputFilePath = fs::path(fmt::format("{}/identify_sample_v2/{}_{}_{}.dream3d", unit_test::k_TestFilesDir, slice_by_slice, slice_plane, fill_holes));

      DataStructure dataStructure = LoadDataStructure(inputFilePath);
      IdentifySampleFilter filter;
      Arguments args;
      args.insert(IdentifySampleFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
      args.insert(IdentifySampleFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));
      args.insert(IdentifySampleFilter::k_FillHoles_Key, std::make_any<bool>(fillHoles));
      args.insert(IdentifySampleFilter::k_SliceBySlice_Key, std::make_any<bool>(sliceBySlice));
      args.insert(IdentifySampleFilter::k_SliceBySlicePlane_Key, std::make_any<ChoicesParameter::ValueType>(sliceBySlicePlane));

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

      auto executeResult = scope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

      const IDataArray& computedArray = dataStructure.getDataRefAs<IDataArray>(Constants::k_MaskArrayPath);
      const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(k_ExemplarArrayPath);
      CompareDataArrays<uint8>(computedArray, exemplarArray);

      UnitTest::CheckArraysInheritTupleDims(dataStructure);
    }
  }
}

TEST_CASE("SimplnxCore::IdentifySampleFilter: genuine HDF5 equivalence-page and slice oracle", "[SimplnxCore][IdentifySampleFilter][.OocStoreContract]")
{
  UnitTest::LoadPlugins();
  REQUIRE(Application::Instance()->getIOManager("HDF5-OOC") != nullptr);
  const bool fillHoles = GENERATE(false, true);
  CAPTURE(fillHoles);
  const PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);

  constexpr usize k_Width = 129;
  constexpr usize k_Height = 129;
  constexpr usize k_SliceTuples = k_Width * k_Height;
  constexpr usize k_CellTuples = 3 * k_SliceTuples;
  constexpr usize k_Hole = k_SliceTuples + 10 * k_Width + 10;
  DataStructure dataStructure;
  auto* geometry = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(geometry != nullptr);
  geometry->setDimensions({k_Width, k_Height, 3});
  geometry->setSpacing({1.0F, 1.0F, 1.0F});
  geometry->setOrigin({0.0F, 0.0F, 0.0F});
  auto* cellData = AttributeMatrix::Create(dataStructure, "CellData", {3, k_Height, k_Width}, geometry->getId());
  REQUIRE(cellData != nullptr);
  geometry->setCellData(*cellData);
  auto store = DataStoreUtilities::CreateDataStore<uint8>(dataStructure, k_NonSquareMaskPath, {3, k_Height, k_Width}, {1});
  auto* mask = UInt8Array::Create(dataStructure, "Mask", store, cellData->getId());
  REQUIRE(mask != nullptr);

  // Slice zero creates over 8,000 isolated provisional labels. Slice one joins
  // the labels at x<127, across the 4,096-record equivalence-page boundary.
  std::vector<uint8> values(k_CellTuples, 0);
  for(usize y = 0; y < k_Height; y++)
  {
    for(usize x = 0; x < k_Width; x++)
    {
      const usize offset = y * k_Width + x;
      values[offset] = static_cast<uint8>(x != 127 && (x + y) % 2 == 0);
      values[k_SliceTuples + offset] = static_cast<uint8>(x < 127);
      values[2 * k_SliceTuples + offset] = static_cast<uint8>(x < 127);
    }
  }
  values[k_Hole] = 0;
  // Keep the voxel above the hole connected through its right-hand neighbor.
  values[10 * k_Width + 11] = 1;
  auto writeResult = store->copyFromBuffer(0, nonstd::span<const uint8>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  REQUIRE(store->getDataFormat() == "HDF5-OOC");

  IdentifySampleFilter filter;
  auto args = CreateNonSquareArguments();
  args.insertOrAssign(IdentifySampleFilter::k_FillHoles_Key, std::make_any<bool>(fillHoles));
  const auto before = GetAlgorithmPathExecutionCounts();
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const auto after = GetAlgorithmPathExecutionCounts();
  REQUIRE(after.OutOfCoreOnOutOfCoreStore == before.OutOfCoreOnOutOfCoreStore + 1);
  REQUIRE(after.InCore == before.InCore);
  REQUIRE(mask->getDataStoreRef().getDataFormat() == "HDF5-OOC");

  std::vector<uint8> actual(k_CellTuples);
  auto readResult = mask->getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint8>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  for(usize z = 0; z < 3; z++)
  {
    for(usize y = 0; y < k_Height; y++)
    {
      for(usize x = 0; x < k_Width; x++)
      {
        const usize voxelIdx = z * k_SliceTuples + y * k_Width + x;
        const bool isMainComponent = x < 127 && (z != 0 || (x + y) % 2 == 0 || (x == 11 && y == 10));
        const bool expected = isMainComponent && (voxelIdx != k_Hole || fillHoles);
        INFO("voxel " << voxelIdx);
        REQUIRE(actual[voxelIdx] == static_cast<uint8>(expected));
      }
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::IdentifySampleFilter: SIMPL Backwards Compatibility", "[SimplnxCore][IdentifySampleFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "IdentifySampleFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "IdentifySampleFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<IdentifySampleFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<bool>(IdentifySampleFilter::k_FillHoles_Key) == true);
      CHECK(args.value<DataPath>(IdentifySampleFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(IdentifySampleFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}

// These cases rotate the 3 by 4 fixture through each Empty2D dispatch.
// An incorrect flood-fill stride merges the components or accesses outside the mask.
TEST_CASE("SimplnxCore::IdentifySampleFilter: 2D Empty Z Non-Square {3,4,1}", "[SimplnxCore][IdentifySampleFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure = ::CreateNonSquare2DMaskDataStructure(SizeVec3{3, 4, 1});
  scope.execute([&] { ::RunIdentifySampleAndCheck(dataStructure); });
}

TEST_CASE("SimplnxCore::IdentifySampleFilter: 2D Empty Y Non-Square {3,1,4}", "[SimplnxCore][IdentifySampleFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure = ::CreateNonSquare2DMaskDataStructure(SizeVec3{3, 1, 4});
  scope.execute([&] { ::RunIdentifySampleAndCheck(dataStructure); });
}

TEST_CASE("SimplnxCore::IdentifySampleFilter: 2D Empty X Non-Square {1,3,4}", "[SimplnxCore][IdentifySampleFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure = ::CreateNonSquare2DMaskDataStructure(SizeVec3{1, 3, 4});
  scope.execute([&] { ::RunIdentifySampleAndCheck(dataStructure); });
}

namespace
{
/**
 * @class IdentifySampleBudgetGuard
 * @brief Restores the live manager budget without changing the saved preference.
 * @note The caller must release all tokens before this guard is destroyed.
 */
class IdentifySampleBudgetGuard
{
public:
  /** @brief Captures the current manager budget. */
  IdentifySampleBudgetGuard()
  : m_PreviousBudget(CacheMemoryBudgetManager::instance().budgetBytes())
  {
  }

  /** @brief Restores the captured budget after the test releases its tokens. */
  ~IdentifySampleBudgetGuard()
  {
    CacheMemoryBudgetManager::instance().setBudgetBytes(m_PreviousBudget);
  }

  IdentifySampleBudgetGuard(const IdentifySampleBudgetGuard&) = delete;
  IdentifySampleBudgetGuard& operator=(const IdentifySampleBudgetGuard&) = delete;

private:
  uint64 m_PreviousBudget;
};

/**
 * @struct IdentifySampleTransferCounts
 * @brief Records logical bulk calls and live working reservations during target execution.
 */
struct IdentifySampleTransferCounts
{
  usize readCalls = 0;
  usize writeCalls = 0;
  usize successfulReads = 0;
  usize successfulWrites = 0;
  usize requestedReadValues = 0;
  usize requestedWriteValues = 0;
  usize maximumSpan = 0;
  usize activeCalls = 0;
  usize maximumConcurrentCalls = 0;
  uint64 minimumReservedBytes = std::numeric_limits<uint64>::max();
  uint64 maximumReservedBytes = 0;
};

/**
 * @class IdentifySampleReportingStore
 * @brief Observes real resident bulk transfers only while a witnessed filter call is active.
 * @tparam T Bool or UInt8 mask element type.
 * @note Counters measure logical calls. They do not measure disk traffic or backend worker concurrency.
 */
template <class T>
class IdentifySampleReportingStore : public DataStore<T>
{
public:
  /**
   * @brief Creates a real resident store with observation disabled.
   * @param tupleShape Mask tuple dimensions in Z, Y, X order.
   */
  explicit IdentifySampleReportingStore(const ShapeType& tupleShape)
  : DataStore<T>(tupleShape, ShapeType{1}, std::optional<T>{})
  {
  }

  /**
   * @brief Observes one bulk read and delegates the transfer to the resident store exactly once.
   * @param startIndex First flat source value index.
   * @param buffer Receives the source values.
   * @return The resident store transfer result.
   */
  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    if(!m_Armed)
    {
      return DataStore<T>::copyIntoBuffer(startIndex, buffer);
    }
    beginTransfer(true, buffer.size());
    const auto completion = MakeScopeGuard([this]() noexcept { endTransfer(); });
    auto result = DataStore<T>::copyIntoBuffer(startIndex, buffer);
    if(result.valid())
    {
      const std::lock_guard<std::mutex> lock(m_CountMutex);
      m_Counts.successfulReads++;
    }
    return result;
  }

  /**
   * @brief Observes one bulk write and delegates the transfer to the resident store exactly once.
   * @param startIndex First flat destination value index.
   * @param buffer Supplies the new values.
   * @return The resident store transfer result.
   */
  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    if(!m_Armed)
    {
      return DataStore<T>::copyFromBuffer(startIndex, buffer);
    }
    beginTransfer(false, buffer.size());
    const auto completion = MakeScopeGuard([this]() noexcept { endTransfer(); });
    auto result = DataStore<T>::copyFromBuffer(startIndex, buffer);
    if(result.valid())
    {
      const std::lock_guard<std::mutex> lock(m_CountMutex);
      m_Counts.successfulWrites++;
    }
    return result;
  }

  /**
   * @brief Enables or disables observation between transfers.
   * @param armed True to count subsequent calls.
   * @pre No transfer is active.
   */
  void setArmed(bool armed) noexcept
  {
    m_Armed = armed;
  }

  IdentifySampleTransferCounts counts() const
  {
    const std::lock_guard<std::mutex> lock(m_CountMutex);
    return m_Counts;
  }

private:
  void beginTransfer(bool isRead, usize values) const
  {
    const uint64 reserved = CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes();
    const std::lock_guard<std::mutex> lock(m_CountMutex);
    if(isRead)
    {
      m_Counts.readCalls++;
      m_Counts.requestedReadValues += values;
    }
    else
    {
      m_Counts.writeCalls++;
      m_Counts.requestedWriteValues += values;
    }
    m_Counts.maximumSpan = std::max(m_Counts.maximumSpan, values);
    m_Counts.maximumConcurrentCalls = std::max(m_Counts.maximumConcurrentCalls, ++m_Counts.activeCalls);
    m_Counts.minimumReservedBytes = std::min(m_Counts.minimumReservedBytes, reserved);
    m_Counts.maximumReservedBytes = std::max(m_Counts.maximumReservedBytes, reserved);
  }

  void endTransfer() const noexcept
  {
    const std::lock_guard<std::mutex> lock(m_CountMutex);
    m_Counts.activeCalls--;
  }

  bool m_Armed = false;
  mutable std::mutex m_CountMutex;
  mutable IdentifySampleTransferCounts m_Counts;
};

/**
 * @brief Verifies exact output and the CCL one-plane schedule with no available working reservation.
 * @tparam T Bool or UInt8 mask element type.
 * @param scope Selects and witnesses the resident algorithm scenario.
 * @param fillHoles Selects hole filling.
 */
template <class T>
void CheckIdentifySampleWithoutWorkingHeadroom(UnitTest::AlgorithmTestScope& scope, bool fillHoles)
{
  constexpr usize k_X = 19;
  constexpr usize k_Y = 11;
  constexpr usize k_Z = 13;
  auto store = std::make_shared<IdentifySampleReportingStore<T>>(ShapeType{k_Z, k_Y, k_X});
  DataStructure dataStructure = IdentifySampleBatchTest::CreateFixture<T>(2, k_X, store);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(IdentifySampleBatchTest::k_MaskPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<DataArray<T>>(IdentifySampleBatchTest::k_MaskPath));

  auto& budget = CacheMemoryBudgetManager::instance();
  REQUIRE(budget.reservedWorkingMemoryBytes() == 0);
  const uint64 originalBudget = budget.budgetBytes();
  {
    const IdentifySampleBudgetGuard budgetGuard;
    REQUIRE_FALSE(budget.setBudgetBytes(4ULL * 1024 * 1024));
    REQUIRE(budget.budgetBytes() == 4ULL * 1024 * 1024);
    const uint64 capacity = budget.maximumWorkingMemoryBytes();
    REQUIRE(capacity == 1024ULL * 1024);
    const auto occupied = budget.reserveWorkingMemory(capacity);
    REQUIRE(occupied.sizeBytes() == capacity);
    REQUIRE(budget.reservedWorkingMemoryBytes() == capacity);
    const auto refused = budget.reserveWorkingMemory(1);
    REQUIRE(refused.sizeBytes() == 0);

    IdentifySampleFilter filter;
    const auto args = IdentifySampleBatchTest::ArgumentsFor(2, fillHoles);
    const auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    {
      store->setArmed(true);
      const auto disarm = MakeScopeGuard([&store]() noexcept { store->setArmed(false); });
      const auto executeResult = scope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    if(scope.scenario() == UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore)
    {
      // With no headroom, CCL retains the one-plane transfer schedule and full XY spans.
      const auto counts = store->counts();
      REQUIRE(counts.readCalls == 2 * k_X * k_Z);
      REQUIRE(counts.writeCalls == k_X * k_Z);
      REQUIRE(counts.successfulReads == counts.readCalls);
      REQUIRE(counts.successfulWrites == counts.writeCalls);
      REQUIRE(counts.requestedReadValues == counts.readCalls * k_X * k_Y);
      REQUIRE(counts.requestedWriteValues == counts.writeCalls * k_X * k_Y);
      REQUIRE(counts.maximumSpan == k_X * k_Y);
      REQUIRE(counts.maximumConcurrentCalls == 1);
      REQUIRE(counts.activeCalls == 0);
      REQUIRE(counts.minimumReservedBytes == capacity);
      REQUIRE(counts.maximumReservedBytes == capacity);
    }
    REQUIRE(budget.reservedWorkingMemoryBytes() == capacity);
  }
  REQUIRE(budget.reservedWorkingMemoryBytes() == 0);
  REQUIRE(budget.budgetBytes() == originalBudget);
  IdentifySampleBatchTest::RequireOutput<T>(dataStructure, 2, fillHoles, k_X);
}
} // namespace

TEST_CASE("SimplnxCore::IdentifySampleFilter: orthogonal literal masks", "[SimplnxCore][IdentifySampleFilter][.IdentifySampleBatchCorrectness]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const auto plane = GENERATE(ChoicesParameter::ValueType{0}, ChoicesParameter::ValueType{1}, ChoicesParameter::ValueType{2});
  const bool fillHoles = GENERATE(false, true);
  const bool useBool = GENERATE(false, true);
  CAPTURE(scenario, plane, fillHoles, useBool);
  const IdentifySampleBatchTest::ScopedPreferenceRestore preferenceRestore;
  UnitTest::AlgorithmTestScope scope(scenario);
  if(useBool)
  {
    auto dataStructure = IdentifySampleBatchTest::CreateFixture<bool>(plane);
    IdentifySampleBatchTest::ExecuteAndRequire<bool>(scope, dataStructure, plane, fillHoles);
  }
  else
  {
    auto dataStructure = IdentifySampleBatchTest::CreateFixture<uint8>(plane);
    IdentifySampleBatchTest::ExecuteAndRequire<uint8>(scope, dataStructure, plane, fillHoles);
  }
}

TEST_CASE("SimplnxCore::IdentifySampleFilter: YZ literal tail masks", "[SimplnxCore][IdentifySampleFilter][.IdentifySampleBatchCorrectness]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const usize fixedCount = GENERATE(usize{1}, usize{7}, usize{8}, usize{9}, usize{16}, usize{17}, usize{19});
  const bool fillHoles = GENERATE(false, true);
  const bool useBool = GENERATE(false, true);
  CAPTURE(scenario, fixedCount, fillHoles, useBool);
  const IdentifySampleBatchTest::ScopedPreferenceRestore preferenceRestore;
  UnitTest::AlgorithmTestScope scope(scenario);
  if(useBool)
  {
    auto dataStructure = IdentifySampleBatchTest::CreateFixture<bool>(2, fixedCount);
    IdentifySampleBatchControlTest::CheckGrant<bool>(scope, dataStructure, 8, 0, fillHoles, fixedCount, scenario == UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
  }
  else
  {
    auto dataStructure = IdentifySampleBatchTest::CreateFixture<uint8>(2, fixedCount);
    IdentifySampleBatchControlTest::CheckGrant<uint8>(scope, dataStructure, 8, 0, fillHoles, fixedCount, scenario == UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
  }
}

TEST_CASE("SimplnxCore::IdentifySampleFilter: YZ without working headroom", "[SimplnxCore][IdentifySampleFilter][.IdentifySampleBatchCorrectness]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool fillHoles = GENERATE(false, true);
  const bool useBool = GENERATE(false, true);
  CAPTURE(scenario, fillHoles, useBool);
  const IdentifySampleBatchTest::ScopedPreferenceRestore preferenceRestore;
  UnitTest::AlgorithmTestScope scope(scenario);
  if(useBool)
  {
    CheckIdentifySampleWithoutWorkingHeadroom<bool>(scope, fillHoles);
  }
  else
  {
    CheckIdentifySampleWithoutWorkingHeadroom<uint8>(scope, fillHoles);
  }
}

TEST_CASE("SimplnxCore::IdentifySampleFilter: YZ real reservation widths", "[SimplnxCore][IdentifySampleFilter][.IdentifySampleBatchCorrectness]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const usize width = GENERATE(usize{1}, usize{2}, usize{3}, usize{8});
  const usize partial = GENERATE(usize{0}, usize{17});
  const bool fillHoles = GENERATE(false, true);
  const bool useBool = GENERATE(false, true);
  DYNAMIC_SECTION("scenario=" << scenario << " width=" << width << " remainder=" << partial << " Bool=" << useBool << " fill=" << fillHoles)
  {
    const IdentifySampleBatchTest::ScopedPreferenceRestore restorePreferences;
    UnitTest::AlgorithmTestScope scope(scenario);
    if(useBool)
    {
      auto data = IdentifySampleBatchTest::CreateFixture<bool>(2);
      IdentifySampleBatchControlTest::CheckGrant<bool>(scope, data, width, partial, fillHoles, 19, scenario == UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
    }
    else
    {
      auto data = IdentifySampleBatchTest::CreateFixture<uint8>(2);
      IdentifySampleBatchControlTest::CheckGrant<uint8>(scope, data, width, partial, fillHoles, 19, scenario == UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
    }
  }
}
