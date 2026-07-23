#include "SimplnxCore/Filters/CreateFeatureArrayFromElementArrayFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/DataTypeUtilities.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const std::string k_CellAMName("Cell Data");
const std::string k_FeatureAMName("Cell Feature Data");
const std::string k_FeatureIdsName("FeatureIds");
const std::string k_ElementArrayName("ElementValues");
const std::string k_CreatedArrayName("FeatureValues");

const DataPath k_CellAMPath({k_CellAMName});
const DataPath k_FeatureAMPath({k_FeatureAMName});
const DataPath k_FeatureIdsPath = k_CellAMPath.createChildPath(k_FeatureIdsName);
const DataPath k_ElementArrayPath = k_CellAMPath.createChildPath(k_ElementArrayName);
const DataPath k_CreatedArrayPath = k_FeatureAMPath.createChildPath(k_CreatedArrayName);

// Analytical (Class 1) fixture builder: a cell AttributeMatrix holding an int32 FeatureIds
// array and a T element array with numComps components per tuple, plus a Feature
// AttributeMatrix whose starting tuple count is featureAMTuples. The filter is expected to
// resize the Feature AttributeMatrix to (max(FeatureIds) + 1) tuples during execution.
// Arrays are created through DataStoreUtilities::CreateDataStore so they honor any active
// out-of-core preferences.
template <typename T>
DataStructure BuildAnalyticalFixture(const std::vector<int32>& featureIds, const std::vector<T>& elementValues, usize numComps, usize featureAMTuples = 1)
{
  REQUIRE(elementValues.size() == featureIds.size() * numComps);

  DataStructure dataStructure;
  const std::vector<usize> cellShape = {featureIds.size()};
  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, k_CellAMName, cellShape);
  AttributeMatrix::Create(dataStructure, k_FeatureAMName, std::vector<usize>{featureAMTuples});

  auto featureIdsStorePtr = DataStoreUtilities::CreateDataStore<int32>(cellShape, {1}, IDataAction::Mode::Execute);
  auto* featureIdsArrayPtr = DataArray<int32>::Create(dataStructure, k_FeatureIdsName, featureIdsStorePtr, cellAMPtr->getId());
  REQUIRE(featureIdsArrayPtr != nullptr);

  auto elementStorePtr = DataStoreUtilities::CreateDataStore<T>(cellShape, {numComps}, IDataAction::Mode::Execute);
  auto* elementArrayPtr = DataArray<T>::Create(dataStructure, k_ElementArrayName, elementStorePtr, cellAMPtr->getId());
  REQUIRE(elementArrayPtr != nullptr);

  auto& featureIdsStoreRef = featureIdsArrayPtr->getDataStoreRef();
  auto& elementStoreRef = elementArrayPtr->getDataStoreRef();
  for(usize i = 0; i < featureIds.size(); i++)
  {
    featureIdsStoreRef[i] = featureIds[i];
  }
  for(usize i = 0; i < elementValues.size(); i++)
  {
    elementStoreRef[i] = elementValues[i];
  }
  return dataStructure;
}

Arguments MakeFilterArgs()
{
  Arguments args;
  args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(k_ElementArrayPath));
  args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
  args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CreatedArrayName_Key, std::make_any<std::string>(k_CreatedArrayName));
  return args;
}

// Preflights and executes the filter, requiring both to succeed, and returns the execute
// result so callers can inspect warnings.
IFilter::ExecuteResult RunFilterExpectSuccess(DataStructure& dataStructure)
{
  CreateFeatureArrayFromElementArrayFilter filter;
  Arguments args = MakeFilterArgs();

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  return executeResult;
}

usize CountWarnings(const IFilter::ExecuteResult& executeResult, int32 warningCode)
{
  const auto& warnings = executeResult.result.warnings();
  return static_cast<usize>(std::count_if(warnings.begin(), warnings.end(), [warningCode](const Warning& warning) { return warning.code == warningCode; }));
}

// Verifies the created feature array holds exactly the expected values and that its parent
// Feature AttributeMatrix was resized to match (Class 4 invariant: tuple count is always
// max(FeatureIds) + 1).
template <typename T>
void CheckCreatedFeatureArray(const DataStructure& dataStructure, const std::vector<T>& expectedValues, usize numComps)
{
  const usize expectedTuples = expectedValues.size() / numComps;

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(k_FeatureAMPath));
  const auto& featureAM = dataStructure.getDataRefAs<AttributeMatrix>(k_FeatureAMPath);
  REQUIRE(featureAM.getNumberOfTuples() == expectedTuples);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(k_CreatedArrayPath));
  const auto& createdStoreRef = dataStructure.getDataRefAs<DataArray<T>>(k_CreatedArrayPath).getDataStoreRef();
  REQUIRE(createdStoreRef.getNumberOfTuples() == expectedTuples);
  REQUIRE(createdStoreRef.getNumberOfComponents() == numComps);
  for(usize i = 0; i < expectedValues.size(); i++)
  {
    INFO(fmt::format("flat index i = {}", i));
    REQUIRE(createdStoreRef[i] == expectedValues[i]);
  }
}

// Runs the type-dispatch check for one primitive type T. FeatureIds = {1, 2, 1}; element
// values = {2, 4, 2}. Hand derivation: feature 1 <- cells 0 and 2 (both 2), feature 2 <- cell 1
// (value 4), feature 0 never appears in FeatureIds so it keeps the created array's fill value 0.
// Expected feature array = {0, 2, 4} (for bool: {false, true, true}).
template <typename T>
void RunTypedDispatchCheck()
{
  const std::vector<int32> featureIds = {1, 2, 1};
  const std::vector<T> elementValues = {static_cast<T>(2), static_cast<T>(4), static_cast<T>(2)};
  DataStructure dataStructure = BuildAnalyticalFixture<T>(featureIds, elementValues, 1);

  IFilter::ExecuteResult executeResult = RunFilterExpectSuccess(dataStructure);
  REQUIRE(CountWarnings(executeResult, -1000) == 0);

  const std::vector<T> expectedValues = {static_cast<T>(0), static_cast<T>(2), static_cast<T>(4)};
  CheckCreatedFeatureArray<T>(dataStructure, expectedValues, 1);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

TEST_CASE("SimplnxCore::CreateFeatureArrayFromElementArrayFilter: Analytical oracle", "[SimplnxCore][CreateFeatureArrayFromElementArrayFilter]")
{
  UnitTest::LoadPlugins();

  // Configure out-of-core settings (automatically restored on scope exit; no-op in an in-core build)
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 100, true);

  SECTION("single component with feature 0 and a gap feature id")
  {
    // Hand derivation (Class 1):
    //   FeatureIds:  {1,  3,  1,  4,  3,  1,  0, 4}
    //   Element:     {10, 30, 10, 40, 30, 10, 5, 40}
    //   feature 0 <- cell 6 (5); feature 1 <- cells 0,2,5 (all 10); feature 2 never appears so it
    //   keeps the fill value 0; feature 3 <- cells 1,4 (30); feature 4 <- cells 3,7 (40).
    //   max(FeatureIds) = 4, so the Feature AttributeMatrix must grow from 1 tuple to 5.
    const std::vector<int32> featureIds = {1, 3, 1, 4, 3, 1, 0, 4};
    const std::vector<int32> elementValues = {10, 30, 10, 40, 30, 10, 5, 40};
    DataStructure dataStructure = BuildAnalyticalFixture<int32>(featureIds, elementValues, 1);

    IFilter::ExecuteResult executeResult = RunFilterExpectSuccess(dataStructure);

    // Class 4 invariant: consistent per-feature values must not produce the -1000 warning
    REQUIRE(CountWarnings(executeResult, -1000) == 0);

    const std::vector<int32> expectedValues = {5, 10, 0, 30, 40};
    CheckCreatedFeatureArray<int32>(dataStructure, expectedValues, 1);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("inconsistent feature values warn exactly once and the last value wins")
  {
    // Hand derivation (Class 1):
    //   FeatureIds:  {1,  1,  2,  2}
    //   Element:     {10, 11, 20, 21}
    //   Cell 1 (value 11) disagrees with the first value stored for feature 1 (10) -> warning
    //   -1000 and the later value overwrites: feature 1 = 11. Feature 2 is likewise
    //   inconsistent (20 vs 21) -> feature 2 = 21, but the algorithm only ever emits ONE
    //   warning, so the second inconsistency must not add another.
    const std::vector<int32> featureIds = {1, 1, 2, 2};
    const std::vector<int32> elementValues = {10, 11, 20, 21};
    DataStructure dataStructure = BuildAnalyticalFixture<int32>(featureIds, elementValues, 1);

    IFilter::ExecuteResult executeResult = RunFilterExpectSuccess(dataStructure);
    REQUIRE(CountWarnings(executeResult, -1000) == 1);

    const std::vector<int32> expectedValues = {0, 11, 21};
    CheckCreatedFeatureArray<int32>(dataStructure, expectedValues, 1);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("multi component tuples copy component-wise with last value winning")
  {
    // Hand derivation (Class 1), 3 components per tuple:
    //   FeatureIds:  {1, 2, 1}
    //   Element:     {10,11,12}, {20,21,22}, {13,14,15}
    //   Cell 2 disagrees with the first tuple stored for feature 1 -> one -1000 warning and the
    //   last tuple wins: feature 1 = {13,14,15}. feature 2 = {20,21,22}. feature 0 never
    //   appears -> keeps fill value {0,0,0}.
    const std::vector<int32> featureIds = {1, 2, 1};
    const std::vector<uint8> elementValues = {10, 11, 12, 20, 21, 22, 13, 14, 15};
    DataStructure dataStructure = BuildAnalyticalFixture<uint8>(featureIds, elementValues, 3);

    IFilter::ExecuteResult executeResult = RunFilterExpectSuccess(dataStructure);
    REQUIRE(CountWarnings(executeResult, -1000) == 1);

    const std::vector<uint8> expectedValues = {0, 0, 0, 13, 14, 15, 20, 21, 22};
    CheckCreatedFeatureArray<uint8>(dataStructure, expectedValues, 3);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("oversized Feature AttributeMatrix is resized down to max(FeatureIds) + 1 with a warning")
  {
    // Class 4 invariant: the output tuple count is always max(FeatureIds) + 1, even when the
    // selected Feature AttributeMatrix starts out LARGER. DREAM3D 6.5.171 errored out in this
    // situation (-5556); SIMPLNX deliberately resizes instead (see the V&V deviation entry) and
    // warns (-5573) that existing Feature arrays are being truncated.
    const std::vector<int32> featureIds = {1, 2};
    const std::vector<int32> elementValues = {10, 20};
    DataStructure dataStructure = BuildAnalyticalFixture<int32>(featureIds, elementValues, 1, 10);

    IFilter::ExecuteResult executeResult = RunFilterExpectSuccess(dataStructure);
    REQUIRE(CountWarnings(executeResult, -5573) == 1);

    const std::vector<int32> expectedValues = {0, 10, 20};
    CheckCreatedFeatureArray<int32>(dataStructure, expectedValues, 1);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("NaN element values do not consume the consistency warning")
  {
    // Feature 1's cells are both NaN — consistent data, so no warning may fire for it (NaN !=
    // NaN would otherwise spuriously consume the single warning). Feature 2 is genuinely
    // inconsistent (5 vs 6), so exactly one -1000 warning fires and the last value (6) wins.
    const float32 k_NaN = std::numeric_limits<float32>::quiet_NaN();
    const std::vector<int32> featureIds = {1, 1, 2, 2};
    const std::vector<float32> elementValues = {k_NaN, k_NaN, 5.0F, 6.0F};
    DataStructure dataStructure = BuildAnalyticalFixture<float32>(featureIds, elementValues, 1);

    IFilter::ExecuteResult executeResult = RunFilterExpectSuccess(dataStructure);
    REQUIRE(CountWarnings(executeResult, -1000) == 1);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_CreatedArrayPath));
    const auto& createdStoreRef = dataStructure.getDataRefAs<Float32Array>(k_CreatedArrayPath).getDataStoreRef();
    REQUIRE(createdStoreRef.getNumberOfTuples() == 3);
    REQUIRE(createdStoreRef[0] == 0.0F);
    REQUIRE(std::isnan(createdStoreRef[1]));
    REQUIRE(createdStoreRef[2] == 6.0F);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("sparse feature ids grow the Feature AttributeMatrix and warn")
  {
    // max(FeatureIds) = 5 exceeds the 3-cell count, which is legal but suspicious (it drives the
    // size of every array in the destination AttributeMatrix), so warning -5574 fires. Feature 5
    // takes the (consistent) cell value 7; ids 0-4 are gaps and keep the fill value 0.
    const std::vector<int32> featureIds = {5, 5, 5};
    const std::vector<int32> elementValues = {7, 7, 7};
    DataStructure dataStructure = BuildAnalyticalFixture<int32>(featureIds, elementValues, 1);

    IFilter::ExecuteResult executeResult = RunFilterExpectSuccess(dataStructure);
    REQUIRE(CountWarnings(executeResult, -5574) == 1);
    REQUIRE(CountWarnings(executeResult, -1000) == 0);

    const std::vector<int32> expectedValues = {0, 0, 0, 0, 0, 7};
    CheckCreatedFeatureArray<int32>(dataStructure, expectedValues, 1);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::CreateFeatureArrayFromElementArrayFilter: All DataTypes dispatch", "[SimplnxCore][CreateFeatureArrayFromElementArrayFilter]")
{
  UnitTest::LoadPlugins();

  for(DataType dataType : GetAllDataTypes())
  {
    DYNAMIC_SECTION(fmt::format("DataType: {}", DataTypeToString(dataType)))
    {
      switch(dataType)
      {
      case DataType::int8:
        RunTypedDispatchCheck<int8>();
        break;
      case DataType::uint8:
        RunTypedDispatchCheck<uint8>();
        break;
      case DataType::int16:
        RunTypedDispatchCheck<int16>();
        break;
      case DataType::uint16:
        RunTypedDispatchCheck<uint16>();
        break;
      case DataType::int32:
        RunTypedDispatchCheck<int32>();
        break;
      case DataType::uint32:
        RunTypedDispatchCheck<uint32>();
        break;
      case DataType::int64:
        RunTypedDispatchCheck<int64>();
        break;
      case DataType::uint64:
        RunTypedDispatchCheck<uint64>();
        break;
      case DataType::float32:
        RunTypedDispatchCheck<float32>();
        break;
      case DataType::float64:
        RunTypedDispatchCheck<float64>();
        break;
      case DataType::boolean:
        RunTypedDispatchCheck<bool>();
        break;
      }
    }
  }
}

TEST_CASE("SimplnxCore::CreateFeatureArrayFromElementArrayFilter: Error conditions", "[SimplnxCore][CreateFeatureArrayFromElementArrayFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("negative feature ids fail execution")
  {
    // A negative Feature Id would index before the start of the created array (this was
    // undefined behavior in DREAM3D 6.5.171). SIMPLNX rejects it at execute time with -5570.
    const std::vector<int32> featureIds = {1, -1, 2};
    const std::vector<int32> elementValues = {10, 20, 30};
    DataStructure dataStructure = BuildAnalyticalFixture<int32>(featureIds, elementValues, 1);

    CreateFeatureArrayFromElementArrayFilter filter;
    Arguments args = MakeFilterArgs();

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
    const auto& errors = executeResult.result.errors();
    REQUIRE(std::any_of(errors.begin(), errors.end(), [](const Error& error) { return error.code == -5570; }));
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("mismatched FeatureIds and element array tuple counts fail preflight")
  {
    // The algorithm reads FeatureIds[i] for every tuple i of the element array, so differing
    // tuple counts would read out of bounds. SIMPLNX rejects this at preflight with -5571.
    DataStructure dataStructure;
    auto* cellAMPtr = AttributeMatrix::Create(dataStructure, k_CellAMName, std::vector<usize>{3});
    auto* otherAMPtr = AttributeMatrix::Create(dataStructure, "Other Cell Data", std::vector<usize>{4});
    AttributeMatrix::Create(dataStructure, k_FeatureAMName, std::vector<usize>{1});

    auto featureIdsStorePtr = DataStoreUtilities::CreateDataStore<int32>({3}, {1}, IDataAction::Mode::Execute);
    DataArray<int32>::Create(dataStructure, k_FeatureIdsName, featureIdsStorePtr, cellAMPtr->getId());

    auto elementStorePtr = DataStoreUtilities::CreateDataStore<int32>({4}, {1}, IDataAction::Mode::Execute);
    DataArray<int32>::Create(dataStructure, k_ElementArrayName, elementStorePtr, otherAMPtr->getId());

    CreateFeatureArrayFromElementArrayFilter filter;
    Arguments args = MakeFilterArgs();
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(DataPath({"Other Cell Data", k_ElementArrayName})));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
    const auto& errors = preflightResult.outputActions.errors();
    REQUIRE(std::any_of(errors.begin(), errors.end(), [](const Error& error) { return error.code == -5571; }));
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("destination Feature AttributeMatrix containing the inputs fails preflight")
  {
    // The execute-time resize of the destination AttributeMatrix would truncate or grow the
    // input arrays out from under the copy loop if they live in that same AttributeMatrix —
    // silent data corruption. SIMPLNX rejects the selection at preflight with -5572.
    const std::vector<int32> featureIds = {1, 2};
    const std::vector<int32> elementValues = {10, 20};
    DataStructure dataStructure = BuildAnalyticalFixture<int32>(featureIds, elementValues, 1);

    CreateFeatureArrayFromElementArrayFilter filter;
    Arguments args = MakeFilterArgs();
    args.insertOrAssign(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellAMPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
    const auto& errors = preflightResult.outputActions.errors();
    REQUIRE(std::any_of(errors.begin(), errors.end(), [](const Error& error) { return error.code == -5572; }));
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::CreateFeatureArrayFromElementArrayFilter: Large analytical", "[SimplnxCore][CreateFeatureArrayFromElementArrayFilter]")
{
  UnitTest::LoadPlugins();

  // Force arrays out-of-core when the SimplnxOoc plugin is loaded; threshold is one 100x100
  // slab of the float32 element array so the arrays are chunked (no-op in an in-core build).
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 100 * 100 * 4, true);

  // 100^3 cells, 5000 features assigned round-robin: FeatureIds[i] = i % 5000, element
  // value = FeatureIds[i] * 0.5 + 0.25 (exact in float32 for this range). Every feature id
  // 0..4999 appears and all its cells agree, so (Class 1) feature f = f * 0.5 + 0.25 with no
  // warnings, and (Class 4) no tuple keeps the fill value.
  constexpr usize k_DimSize = 100;
  constexpr int32 k_NumFeatures = 5000;
  const std::vector<usize> cellShape = {k_DimSize, k_DimSize, k_DimSize};
  const usize numCells = k_DimSize * k_DimSize * k_DimSize;

  DataStructure dataStructure;
  auto* cellAM = AttributeMatrix::Create(dataStructure, k_CellAMName, cellShape);
  AttributeMatrix::Create(dataStructure, k_FeatureAMName, std::vector<usize>{1});

  auto featureIdsStorePtr = DataStoreUtilities::CreateDataStore<int32>(cellShape, {1}, IDataAction::Mode::Execute);
  auto* featureIdsArray = DataArray<int32>::Create(dataStructure, k_FeatureIdsName, featureIdsStorePtr, cellAM->getId());
  REQUIRE(featureIdsArray != nullptr);

  auto elementStorePtr = DataStoreUtilities::CreateDataStore<float32>(cellShape, {1}, IDataAction::Mode::Execute);
  auto* elementArray = DataArray<float32>::Create(dataStructure, k_ElementArrayName, elementStorePtr, cellAM->getId());
  REQUIRE(elementArray != nullptr);

  auto& featureIdsStoreRef = featureIdsArray->getDataStoreRef();
  auto& elementStoreRef = elementArray->getDataStoreRef();
  for(usize i = 0; i < numCells; i++)
  {
    const auto featureId = static_cast<int32>(i % k_NumFeatures);
    featureIdsStoreRef[i] = featureId;
    elementStoreRef[i] = static_cast<float32>(featureId) * 0.5F + 0.25F;
  }

  IFilter::ExecuteResult executeResult = RunFilterExpectSuccess(dataStructure);
  REQUIRE(CountWarnings(executeResult, -1000) == 0);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_CreatedArrayPath));
  const auto& createdStoreRef = dataStructure.getDataRefAs<Float32Array>(k_CreatedArrayPath).getDataStoreRef();
  REQUIRE(createdStoreRef.getNumberOfTuples() == static_cast<usize>(k_NumFeatures));
  for(int32 featureId = 0; featureId < k_NumFeatures; featureId++)
  {
    const auto expected = static_cast<float32>(featureId) * 0.5F + 0.25F;
    INFO(fmt::format("featureId = {}", featureId));
    REQUIRE(createdStoreRef[static_cast<usize>(featureId)] == expected);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CreateFeatureArrayFromElementArrayFilter: SIMPL Backwards Compatibility", "[SimplnxCore][CreateFeatureArrayFromElementArrayFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "CreateFeatureArrayFromElementArrayFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "CreateFeatureArrayFromElementArrayFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<CreateFeatureArrayFromElementArrayFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(CreateFeatureArrayFromElementArrayFilter::k_SelectedCellArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(CreateFeatureArrayFromElementArrayFilter::k_CellFeatureAttributeMatrixPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<std::string>(CreateFeatureArrayFromElementArrayFilter::k_CreatedArrayName_Key) == "TestName");
    }
  }
}
