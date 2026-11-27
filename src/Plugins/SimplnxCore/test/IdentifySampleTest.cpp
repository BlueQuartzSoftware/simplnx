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
using namespace nx::core::unit_test;
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
DataStructure CreateNonSquare2DMaskDataStructure(const SizeVec3Type& dims, bool useConfiguredStore = false)
{
  DataStructure dataStructure = {};
  ImageGeom* imageGeomPtr = ImageGeom::Create(dataStructure, k_NonSquareImagePath.getTargetName());
  REQUIRE(imageGeomPtr->setSpacing(FloatVec3Type{std::array<float32, 3>{1.0f, 1.0f, 1.0f}}).valid());
  REQUIRE(imageGeomPtr->setOrigin(FloatVec3Type{std::array<float32, 3>{0.0f, 0.0f, 0.0f}}).valid());
  imageGeomPtr->setDimensions(dims);

  const ShapeType imageShape{dims[0], dims[1], dims[2]};
  AttributeMatrix* cellDataPtr = AttributeMatrix::Create(dataStructure, "CellData", imageShape, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellDataPtr);

  std::shared_ptr<AbstractDataStore<bool>> maskStore;
  if(useConfiguredStore)
  {
    maskStore = data_store_utilities::CreateDataStore<bool>(dataStructure, k_NonSquareMaskPath, cellDataPtr->getShape(), ShapeType{1});
  }
  else
  {
    maskStore = std::make_shared<BoolDataStoreType>(cellDataPtr->getShape(), ShapeType{1}, std::optional<bool>{});
  }
  BoolArrayType* maskPtr = BoolArrayType::Create(dataStructure, "Mask", maskStore, cellDataPtr->getId());

  const std::array<bool, 12> values = {true, true, false, true, true, false, false, false, true, false, false, true};
  REQUIRE(maskPtr->getNumberOfTuples() == values.size());
  for(usize i = 0; i < values.size(); i++)
  {
    maskPtr->setValue(i, values[i]);
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
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const std::array<bool, 12> expected = {true, true, false, true, true, false, false, false, false, false, false, false};
  const DataPath maskPath = k_NonSquareImagePath.createChildPath("CellData").createChildPath("Mask");
  const auto& mask = dataStructure.getDataRefAs<BoolArrayType>(maskPath);
  REQUIRE(mask.getNumberOfTuples() == expected.size());
  for(usize i = 0; i < expected.size(); i++)
  {
    INFO("linear index " << i);
    REQUIRE(mask[i] == expected[i]);
  }
}

const DataPath k_ExemplarArrayPath = constants::k_DataContainerPath.createChildPath(constants::k_CellData).createChildPath("Mask Exemplar");
} // namespace

TEST_CASE("SimplnxCore::IdentifySampleFilter", "[SimplnxCore][IdentifySampleFilter]")
{
  unit_test::LoadPlugins();

  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "identify_sample_v2.tar.gz", "identify_sample_v2");
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

    std::string slicePlane = std::get<1>(testParam);

    ChoicesParameter::ValueType sliceBySlicePlane = 0;
    if(slicePlane == "xz")
    {
      sliceBySlicePlane = 1;
    }
    else if(slicePlane == "yz")
    {
      sliceBySlicePlane = 2;
    }

    std::string fill_holes = std::get<2>(testParam);
    bool fillHoles = fill_holes == "fill";

    SECTION(fmt::format("{}_{}_{}", slice_by_slice, slicePlane, fill_holes))
    {
      fs::path inputFilePath = fs::path(fmt::format("{}/identify_sample_v2/{}_{}_{}.dream3d", unit_test::k_TestFilesDir, slice_by_slice, slicePlane, fill_holes));

      DataStructure dataStructure = LoadDataStructure(inputFilePath);
      IdentifySampleFilter filter;
      Arguments args;
      args.insert(IdentifySampleFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(constants::k_DataContainerPath));
      args.insert(IdentifySampleFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(constants::k_MaskArrayPath));
      args.insert(IdentifySampleFilter::k_FillHoles_Key, std::make_any<bool>(fillHoles));
      args.insert(IdentifySampleFilter::k_SliceBySlice_Key, std::make_any<bool>(sliceBySlice));
      args.insert(IdentifySampleFilter::k_SliceBySlicePlane_Key, std::make_any<ChoicesParameter::ValueType>(sliceBySlicePlane));

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

      auto executeResult = scope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

      const IDataArray& computedArray = dataStructure.getDataRefAs<IDataArray>(constants::k_MaskArrayPath);
      const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(k_ExemplarArrayPath);
      CompareDataArrays<uint8>(computedArray, exemplarArray);

      unit_test::CheckArraysInheritTupleDims(dataStructure);
    }
  }
}

TEST_CASE("SimplnxCore::IdentifySampleFilter: genuine HDF5 equivalence-page and slice oracle", "[SimplnxCore][IdentifySampleFilter][.OocStoreContract]")
{
  unit_test::LoadPlugins();
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
  auto* geometryPtr = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(geometryPtr != nullptr);
  geometryPtr->setDimensions({k_Width, k_Height, 3});
  REQUIRE(geometryPtr->setSpacing({1.0F, 1.0F, 1.0F}).valid());
  REQUIRE(geometryPtr->setOrigin({0.0F, 0.0F, 0.0F}).valid());
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "CellData", {3, k_Height, k_Width}, geometryPtr->getId());
  REQUIRE(cellDataPtr != nullptr);
  geometryPtr->setCellData(*cellDataPtr);
  auto store = data_store_utilities::CreateDataStore<uint8>(dataStructure, k_NonSquareMaskPath, {3, k_Height, k_Width}, {1});
  auto* maskPtr = UInt8ArrayType::Create(dataStructure, "Mask", store, cellDataPtr->getId());
  REQUIRE(maskPtr != nullptr);

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
  REQUIRE(after.out_of_core_on_out_of_core_store == before.out_of_core_on_out_of_core_store + 1);
  REQUIRE(after.in_core == before.in_core);
  REQUIRE(maskPtr->getDataStoreRef().getDataFormat() == "HDF5-OOC");

  std::vector<uint8> actual(k_CellTuples);
  auto readResult = maskPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint8>(actual.data(), actual.size()));
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
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::IdentifySampleFilter: SIMPL Backwards Compatibility", "[SimplnxCore][IdentifySampleFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "IdentifySampleFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "IdentifySampleFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<IdentifySampleFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
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
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure = ::CreateNonSquare2DMaskDataStructure(SizeVec3Type{3, 4, 1});
  scope.execute([&] { ::RunIdentifySampleAndCheck(dataStructure); });
}

TEST_CASE("SimplnxCore::IdentifySampleFilter: 2D Empty Y Non-Square {3,1,4}", "[SimplnxCore][IdentifySampleFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure = ::CreateNonSquare2DMaskDataStructure(SizeVec3Type{3, 1, 4});
  scope.execute([&] { ::RunIdentifySampleAndCheck(dataStructure); });
}

TEST_CASE("SimplnxCore::IdentifySampleFilter: 2D Empty X Non-Square {1,3,4}", "[SimplnxCore][IdentifySampleFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure = ::CreateNonSquare2DMaskDataStructure(SizeVec3Type{1, 3, 4});
  scope.execute([&] { ::RunIdentifySampleAndCheck(dataStructure); });
}

TEST_CASE("SimplnxCore::IdentifySampleFilter: Geometry cell tuple count", "[SimplnxCore][IdentifySampleFilter][TupleCount]")
{
  using FilterType = IdentifySampleFilter;
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 3, 2});
  REQUIRE(imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F}).valid());
  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{12}, imageGeomPtr->getId());
  REQUIRE(cellAMPtr != nullptr);
  imageGeomPtr->setCellData(*cellAMPtr);

  // A matching array is valid outside Cell Data; short and long arrays must fail preflight.
  const usize tuples = GENERATE(usize{0}, usize{11}, usize{12}, usize{13});
  CAPTURE(tuples);
  auto* otherAMPtr = AttributeMatrix::Create(dataStructure, "Other Data", ShapeType{tuples});
  REQUIRE(otherAMPtr != nullptr);
  REQUIRE(DataArray<uint8>::Create(dataStructure, "Values", std::make_shared<DataStore<uint8>>(otherAMPtr->getShape(), ShapeType{1}, 0), otherAMPtr->getId()) != nullptr);
  const DataPath inputPath({"Other Data", "Values"});

  FilterType filter;
  Arguments args;
  args.insertOrAssign(FilterType::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(FilterType::k_MaskArrayPath_Key, std::make_any<DataPath>(inputPath));

  const auto result = filter.preflight(dataStructure, args);
  if(tuples == 12)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors().size() == 1);
    const auto& errorRef = result.output_actions.errors()[0];
    REQUIRE(errorRef.code == -45463);
    const DataPath& expectedPath = inputPath;
    const usize expectedTuples = tuples;
    REQUIRE(errorRef.message.find(expectedPath.toString()) != std::string::npos);
    REQUIRE(errorRef.message.find(fmt::format("has {} tuples", expectedTuples)) != std::string::npos);
    REQUIRE(errorRef.message.find("'Image' has 12 cells") != std::string::npos);
  }
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
  : m_PreviousBudget(CacheMemoryBudgetManager::Instance().budgetBytes())
  {
  }

  /** @brief Restores the captured budget after the test releases its tokens. */
  ~IdentifySampleBudgetGuard()
  {
    CacheMemoryBudgetManager::Instance().setBudgetBytes(m_PreviousBudget);
  }

  IdentifySampleBudgetGuard(const IdentifySampleBudgetGuard&) = delete;
  IdentifySampleBudgetGuard(IdentifySampleBudgetGuard&&) = delete;
  IdentifySampleBudgetGuard& operator=(const IdentifySampleBudgetGuard&) = delete;
  IdentifySampleBudgetGuard& operator=(IdentifySampleBudgetGuard&&) = delete;

private:
  uint64 m_PreviousBudget;
};

/**
 * @struct IdentifySampleTransferCounts
 * @brief Records logical bulk calls and live working reservations during target execution.
 */
struct IdentifySampleTransferCounts
{
  usize read_calls = 0;
  usize write_calls = 0;
  usize successful_reads = 0;
  usize successful_writes = 0;
  usize requested_read_values = 0;
  usize requested_write_values = 0;
  usize maximum_span = 0;
  usize active_calls = 0;
  usize maximum_concurrent_calls = 0;
  uint64 minimum_reserved_bytes = std::numeric_limits<uint64>::max();
  uint64 maximum_reserved_bytes = 0;
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

  ~IdentifySampleReportingStore() override = default;
  IdentifySampleReportingStore(const IdentifySampleReportingStore&) = delete;
  IdentifySampleReportingStore(IdentifySampleReportingStore&&) = delete;
  IdentifySampleReportingStore& operator=(const IdentifySampleReportingStore&) = delete;
  IdentifySampleReportingStore& operator=(IdentifySampleReportingStore&&) = delete;

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
      m_Counts.successful_reads++;
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
      m_Counts.successful_writes++;
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
    const uint64 reserved = CacheMemoryBudgetManager::Instance().reservedWorkingMemoryBytes();
    const std::lock_guard<std::mutex> lock(m_CountMutex);
    if(isRead)
    {
      m_Counts.read_calls++;
      m_Counts.requested_read_values += values;
    }
    else
    {
      m_Counts.write_calls++;
      m_Counts.requested_write_values += values;
    }
    m_Counts.maximum_span = std::max(m_Counts.maximum_span, values);
    m_Counts.maximum_concurrent_calls = std::max(m_Counts.maximum_concurrent_calls, ++m_Counts.active_calls);
    m_Counts.minimum_reserved_bytes = std::min(m_Counts.minimum_reserved_bytes, reserved);
    m_Counts.maximum_reserved_bytes = std::max(m_Counts.maximum_reserved_bytes, reserved);
  }

  void endTransfer() const noexcept
  {
    const std::lock_guard<std::mutex> lock(m_CountMutex);
    m_Counts.active_calls--;
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
void CheckIdentifySampleWithoutWorkingHeadroom(unit_test::AlgorithmTestScope& scope, bool fillHoles)
{
  constexpr usize k_X = 19;
  constexpr usize k_Y = 11;
  constexpr usize k_Z = 13;

  using IdentifySampleReportingStoreType = IdentifySampleReportingStore<T>;
  using DataArrayType = DataArray<T>;

  auto store = std::make_shared<IdentifySampleReportingStoreType>(ShapeType{k_Z, k_Y, k_X});
  DataStructure dataStructure = identify_sample_batch_test::CreateFixture<T>(2, k_X, store);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArrayType>(identify_sample_batch_test::k_MaskPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<DataArrayType>(identify_sample_batch_test::k_MaskPath));

  auto& budget = CacheMemoryBudgetManager::Instance();
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
    const auto args = identify_sample_batch_test::ArgumentsFor(2, fillHoles);
    const auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
    {
      store->setArmed(true);
      const auto disarm = MakeScopeGuard([&store]() noexcept { store->setArmed(false); });
      const auto executeResult = scope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    if(scope.scenario() == unit_test::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore)
    {
      // With no headroom, CCL retains the one-plane transfer schedule and full XY spans.
      const auto counts = store->counts();
      REQUIRE(counts.read_calls == 2 * k_X * k_Z);
      REQUIRE(counts.write_calls == k_X * k_Z);
      REQUIRE(counts.successful_reads == counts.read_calls);
      REQUIRE(counts.successful_writes == counts.write_calls);
      REQUIRE(counts.requested_read_values == counts.read_calls * k_X * k_Y);
      REQUIRE(counts.requested_write_values == counts.write_calls * k_X * k_Y);
      REQUIRE(counts.maximum_span == k_X * k_Y);
      REQUIRE(counts.maximum_concurrent_calls == 1);
      REQUIRE(counts.active_calls == 0);
      REQUIRE(counts.minimum_reserved_bytes == capacity);
      REQUIRE(counts.maximum_reserved_bytes == capacity);
    }
    REQUIRE(budget.reservedWorkingMemoryBytes() == capacity);
  }
  REQUIRE(budget.reservedWorkingMemoryBytes() == 0);
  REQUIRE(budget.budgetBytes() == originalBudget);
  identify_sample_batch_test::RequireOutput<T>(dataStructure, 2, fillHoles, k_X);
}
} // namespace

TEST_CASE("SimplnxCore::IdentifySampleFilter: orthogonal literal masks", "[SimplnxCore][IdentifySampleFilter][.IdentifySampleBatchCorrectness]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  const auto plane = GENERATE(ChoicesParameter::ValueType{0}, ChoicesParameter::ValueType{1}, ChoicesParameter::ValueType{2});
  const bool fillHoles = GENERATE(false, true);
  const bool useBool = GENERATE(false, true);
  CAPTURE(scenario, plane, fillHoles, useBool);
  const identify_sample_batch_test::ScopedPreferenceRestore preferenceRestore;
  unit_test::AlgorithmTestScope scope(scenario);
  if(useBool)
  {
    auto dataStructure = identify_sample_batch_test::CreateFixture<bool>(plane);
    identify_sample_batch_test::ExecuteAndRequire<bool>(scope, dataStructure, plane, fillHoles);
  }
  else
  {
    auto dataStructure = identify_sample_batch_test::CreateFixture<uint8>(plane);
    identify_sample_batch_test::ExecuteAndRequire<uint8>(scope, dataStructure, plane, fillHoles);
  }
}

TEST_CASE("SimplnxCore::IdentifySampleFilter: YZ literal tail masks", "[SimplnxCore][IdentifySampleFilter][.IdentifySampleBatchCorrectness]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  const usize fixedCount = GENERATE(usize{1}, usize{7}, usize{8}, usize{9}, usize{16}, usize{17}, usize{19});
  const bool fillHoles = GENERATE(false, true);
  const bool useBool = GENERATE(false, true);
  CAPTURE(scenario, fixedCount, fillHoles, useBool);
  const identify_sample_batch_test::ScopedPreferenceRestore preferenceRestore;
  unit_test::AlgorithmTestScope scope(scenario);
  if(useBool)
  {
    auto dataStructure = identify_sample_batch_test::CreateFixture<bool>(2, fixedCount);
    identify_sample_batch_control_test::CheckGrant<bool>(scope, dataStructure, 8, 0, fillHoles, fixedCount, scenario == unit_test::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
  }
  else
  {
    auto dataStructure = identify_sample_batch_test::CreateFixture<uint8>(2, fixedCount);
    identify_sample_batch_control_test::CheckGrant<uint8>(scope, dataStructure, 8, 0, fillHoles, fixedCount, scenario == unit_test::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
  }
}

TEST_CASE("SimplnxCore::IdentifySampleFilter: YZ without working headroom", "[SimplnxCore][IdentifySampleFilter][.IdentifySampleBatchCorrectness]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool fillHoles = GENERATE(false, true);
  const bool useBool = GENERATE(false, true);
  CAPTURE(scenario, fillHoles, useBool);
  const identify_sample_batch_test::ScopedPreferenceRestore preferenceRestore;
  unit_test::AlgorithmTestScope scope(scenario);
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
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  const usize width = GENERATE(usize{1}, usize{2}, usize{3}, usize{8});
  const usize partial = GENERATE(usize{0}, usize{17});
  const bool fillHoles = GENERATE(false, true);
  const bool useBool = GENERATE(false, true);
  DYNAMIC_SECTION("scenario=" << scenario << " width=" << width << " remainder=" << partial << " Bool=" << useBool << " fill=" << fillHoles)
  {
    const identify_sample_batch_test::ScopedPreferenceRestore restorePreferences;
    unit_test::AlgorithmTestScope scope(scenario);
    if(useBool)
    {
      auto data = identify_sample_batch_test::CreateFixture<bool>(2);
      identify_sample_batch_control_test::CheckGrant<bool>(scope, data, width, partial, fillHoles, 19, scenario == unit_test::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
    }
    else
    {
      auto data = identify_sample_batch_test::CreateFixture<uint8>(2);
      identify_sample_batch_control_test::CheckGrant<uint8>(scope, data, width, partial, fillHoles, 19, scenario == unit_test::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
    }
  }
}
