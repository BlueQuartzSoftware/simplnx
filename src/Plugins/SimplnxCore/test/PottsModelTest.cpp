#include "SimplnxCore/Filters/PottsModelFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <filesystem>
#include <random>
#include <set>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const DataPath k_ImageGeomPath({"ImageGeometry"});
const DataPath k_CellDataPath = k_ImageGeomPath.createChildPath("CellData");
const DataPath k_FeatureIdsPath = k_CellDataPath.createChildPath("FeatureIds");
const DataPath k_MaskPath = k_CellDataPath.createChildPath("Mask");
constexpr StringLiteral k_SeedArrayName = "PottsModel SeedValue";

struct TestData
{
  DataStructure dataStructure;
  std::vector<int32> inputFeatureIds;
};

std::vector<int32> CreateRandomFeatureIds(usize count, int32 maximumFeatureId, uint32 seed)
{
  std::mt19937 generator(seed);
  std::uniform_int_distribution<int32> distribution(1, maximumFeatureId);
  std::vector<int32> values(count);
  std::generate(values.begin(), values.end(), [&] { return distribution(generator); });
  return values;
}

TestData CreateTestData(const SizeVec3& dimensions, const std::vector<int32>& featureIds)
{
  TestData testData;
  testData.inputFeatureIds = featureIds;

  const ShapeType cellShape = {dimensions[2], dimensions[1], dimensions[0]};
  auto* imageGeom = ImageGeom::Create(testData.dataStructure, k_ImageGeomPath.getTargetName());
  imageGeom->setDimensions(dimensions);
  auto* cellData = AttributeMatrix::Create(testData.dataStructure, k_CellDataPath.getTargetName(), cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellData);

  auto featureIdsStore = DataStoreUtilities::CreateDataStore<int32>(cellShape, {1}, IDataAction::Mode::Execute);
  auto* featureIdsArray = Int32Array::Create(testData.dataStructure, k_FeatureIdsPath.getTargetName(), featureIdsStore, cellData->getId());
  REQUIRE(featureIdsArray != nullptr);
  REQUIRE(featureIds.size() == featureIdsArray->getNumberOfTuples());
  std::copy(featureIds.cbegin(), featureIds.cend(), featureIdsArray->begin());

  return testData;
}

Arguments CreateArguments(int32 iterations = 10, float64 temperature = 273.0, bool periodicBoundaries = false, uint64 seed = 12345)
{
  PottsModelFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(PottsModelFilter::k_Iterations_Key, std::make_any<int32>(iterations));
  args.insertOrAssign(PottsModelFilter::k_Temperature_Key, std::make_any<float64>(temperature));
  args.insertOrAssign(PottsModelFilter::k_PeriodicBoundaries_Key, std::make_any<bool>(periodicBoundaries));
  args.insertOrAssign(PottsModelFilter::k_UseSeed_Key, std::make_any<bool>(true));
  args.insertOrAssign(PottsModelFilter::k_SeedValue_Key, std::make_any<uint64>(seed));
  args.insertOrAssign(PottsModelFilter::k_SeedArrayName_Key, std::make_any<std::string>(k_SeedArrayName));
  args.insertOrAssign(PottsModelFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(PottsModelFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_MaskPath));
  args.insertOrAssign(PottsModelFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  return args;
}

std::vector<int32> CopyFeatureIds(const DataStructure& dataStructure)
{
  const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
  return {featureIds.cbegin(), featureIds.cend()};
}

usize CountDistinctNonzeroIds(const std::vector<int32>& featureIds)
{
  std::set<int32> distinctIds;
  for(const int32 featureId : featureIds)
  {
    if(featureId != 0)
    {
      distinctIds.insert(featureId);
    }
  }
  return distinctIds.size();
}

template <typename T>
void AddMask(DataStructure& dataStructure, const ShapeType& tupleShape, const std::vector<uint8>& maskValues)
{
  auto maskStore = DataStoreUtilities::CreateDataStore<T>(tupleShape, {1}, IDataAction::Mode::Execute);
  auto* mask = DataArray<T>::Create(dataStructure, k_MaskPath.getTargetName(), maskStore, dataStructure.getDataRefAs<AttributeMatrix>(k_CellDataPath).getId());
  REQUIRE(mask != nullptr);
  REQUIRE(maskValues.size() == mask->getNumberOfTuples());
  for(usize index = 0; index < maskValues.size(); index++)
  {
    (*mask)[index] = maskValues[index] != 0;
  }
}

void RequirePreflightError(const DataStructure& dataStructure, const Arguments& args, int32 expectedCode)
{
  PottsModelFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == expectedCode);
}
} // namespace

TEST_CASE("SimplnxCore::PottsModelFilter: Preflight Errors", "[SimplnxCore][PottsModelFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("Iterations must be positive")
  {
    auto testData = CreateTestData({4, 4, 1}, std::vector<int32>(16, 1));
    auto args = CreateArguments();
    args.insertOrAssign(PottsModelFilter::k_Iterations_Key, std::make_any<int32>(0));
    RequirePreflightError(testData.dataStructure, args, -72000);
  }

  SECTION("Temperature must be positive")
  {
    auto testData = CreateTestData({4, 4, 1}, std::vector<int32>(16, 1));
    auto args = CreateArguments();
    args.insertOrAssign(PottsModelFilter::k_Temperature_Key, std::make_any<float64>(0.0));
    RequirePreflightError(testData.dataStructure, args, -72001);
  }

  SECTION("Feature IDs must be in image cell data")
  {
    DataStructure dataStructure;
    auto featureIdsStore = DataStoreUtilities::CreateDataStore<int32>({4, 4, 1}, {1}, IDataAction::Mode::Execute);
    REQUIRE(Int32Array::Create(dataStructure, "FeatureIds", featureIdsStore) != nullptr);
    auto args = CreateArguments();
    args.insertOrAssign(PottsModelFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"FeatureIds"})));
    RequirePreflightError(dataStructure, args, -72002);
  }

  SECTION("Mask must be in image cell data")
  {
    auto testData = CreateTestData({4, 4, 1}, std::vector<int32>(16, 1));
    auto maskStore = DataStoreUtilities::CreateDataStore<uint8>({4, 4, 1}, {1}, IDataAction::Mode::Execute);
    REQUIRE(UInt8Array::Create(testData.dataStructure, "Mask", maskStore) != nullptr);
    auto args = CreateArguments();
    args.insertOrAssign(PottsModelFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(PottsModelFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Mask"})));
    RequirePreflightError(testData.dataStructure, args, -72003);
  }

  SECTION("Mask and Feature IDs tuple counts must match")
  {
    auto testData = CreateTestData({4, 4, 1}, std::vector<int32>(16, 1));
    auto* maskImageGeom = ImageGeom::Create(testData.dataStructure, "MaskGeometry");
    maskImageGeom->setDimensions({2, 2, 1});
    auto* maskCellData = AttributeMatrix::Create(testData.dataStructure, "CellData", {1, 2, 2}, maskImageGeom->getId());
    maskImageGeom->setCellData(*maskCellData);
    auto maskStore = DataStoreUtilities::CreateDataStore<uint8>({1, 2, 2}, {1}, IDataAction::Mode::Execute);
    REQUIRE(UInt8Array::Create(testData.dataStructure, "Mask", maskStore, maskCellData->getId()) != nullptr);
    auto args = CreateArguments();
    args.insertOrAssign(PottsModelFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(PottsModelFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"MaskGeometry", "CellData", "Mask"})));
    RequirePreflightError(testData.dataStructure, args, -72004);
  }

  SECTION("More than one singleton dimension is invalid")
  {
    auto testData = CreateTestData({1, 1, 8}, std::vector<int32>(8, 1));
    RequirePreflightError(testData.dataStructure, CreateArguments(), -72006);
  }
}

TEST_CASE("SimplnxCore::PottsModelFilter: Execute Error With No Eligible Cells", "[SimplnxCore][PottsModelFilter]")
{
  UnitTest::LoadPlugins();
  auto testData = CreateTestData({4, 4, 1}, std::vector<int32>(16, 0));

  PottsModelFilter filter;
  auto executeResult = filter.execute(testData.dataStructure, CreateArguments());
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -72005);
}

TEST_CASE("SimplnxCore::PottsModelFilter: Deterministic Seed", "[SimplnxCore][PottsModelFilter]")
{
  UnitTest::LoadPlugins();
  constexpr usize k_CellCount = 32 * 32;
  const auto inputFeatureIds = CreateRandomFeatureIds(k_CellCount, 200, 89421);
  auto first = CreateTestData({32, 32, 1}, inputFeatureIds);
  auto second = CreateTestData({32, 32, 1}, inputFeatureIds);

  PottsModelFilter filter;
  const auto args = CreateArguments(10, 273.0, false, 12345);
  auto firstResult = filter.execute(first.dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(firstResult.result);
  auto secondResult = filter.execute(second.dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(secondResult.result);

  const auto firstOutput = CopyFeatureIds(first.dataStructure);
  REQUIRE(firstOutput == CopyFeatureIds(second.dataStructure));
  REQUIRE(firstOutput != inputFeatureIds);
  REQUIRE(first.dataStructure.getDataRefAs<UInt64Array>(DataPath({k_SeedArrayName}))[0] == 12345);
}

TEST_CASE("SimplnxCore::PottsModelFilter: Coarsening", "[SimplnxCore][PottsModelFilter]")
{
  UnitTest::LoadPlugins();
  constexpr usize k_CellCount = 32 * 32;
  const auto inputFeatureIds = CreateRandomFeatureIds(k_CellCount, 200, 72631);
  auto testData = CreateTestData({32, 32, 1}, inputFeatureIds);
  const usize beforeCount = CountDistinctNonzeroIds(inputFeatureIds);

  PottsModelFilter filter;
  auto executeResult = filter.execute(testData.dataStructure, CreateArguments(40, 273.0, false, 12345));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const usize afterCount = CountDistinctNonzeroIds(CopyFeatureIds(testData.dataStructure));

  INFO("Distinct nonzero FeatureIds before: " << beforeCount << ", after: " << afterCount);
  REQUIRE(afterCount < beforeCount);
}

TEST_CASE("SimplnxCore::PottsModelFilter: Three-Dimensional Execution", "[SimplnxCore][PottsModelFilter]")
{
  UnitTest::LoadPlugins();
  constexpr usize k_CellCount = 16 * 16 * 16;
  const auto inputFeatureIds = CreateRandomFeatureIds(k_CellCount, 200, 65390);

  SECTION("Non-periodic boundaries")
  {
    auto testData = CreateTestData({16, 16, 16}, inputFeatureIds);
    PottsModelFilter filter;
    auto executeResult = filter.execute(testData.dataStructure, CreateArguments(5, 273.0, false, 12345));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(CopyFeatureIds(testData.dataStructure) != inputFeatureIds);
  }

  SECTION("Periodic boundaries")
  {
    auto testData = CreateTestData({16, 16, 16}, inputFeatureIds);
    PottsModelFilter filter;
    auto executeResult = filter.execute(testData.dataStructure, CreateArguments(5, 273.0, true, 12345));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(CopyFeatureIds(testData.dataStructure) != inputFeatureIds);
  }
}

TEST_CASE("SimplnxCore::PottsModelFilter: Zero Feature IDs Remain Unchanged", "[SimplnxCore][PottsModelFilter]")
{
  UnitTest::LoadPlugins();
  auto inputFeatureIds = CreateRandomFeatureIds(32 * 32, 100, 45190);
  for(usize index = 0; index < inputFeatureIds.size(); index += 17)
  {
    inputFeatureIds[index] = 0;
  }
  auto testData = CreateTestData({32, 32, 1}, inputFeatureIds);

  PottsModelFilter filter;
  auto executeResult = filter.execute(testData.dataStructure, CreateArguments());
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const auto outputFeatureIds = CopyFeatureIds(testData.dataStructure);
  for(usize index = 0; index < inputFeatureIds.size(); index++)
  {
    if(inputFeatureIds[index] == 0)
    {
      REQUIRE(outputFeatureIds[index] == 0);
    }
  }
}

TEST_CASE("SimplnxCore::PottsModelFilter: Masked Cells Remain Unchanged", "[SimplnxCore][PottsModelFilter]")
{
  UnitTest::LoadPlugins();
  constexpr usize k_Dimension = 32;
  const auto inputFeatureIds = CreateRandomFeatureIds(k_Dimension * k_Dimension, 100, 18274);
  std::vector<uint8> maskValues(inputFeatureIds.size(), 1);
  for(usize y = 8; y < 16; y++)
  {
    for(usize x = 8; x < 16; x++)
    {
      maskValues[y * k_Dimension + x] = 0;
    }
  }

  auto runMaskedTest = [&](auto maskType) {
    using MaskType = decltype(maskType);
    auto testData = CreateTestData({k_Dimension, k_Dimension, 1}, inputFeatureIds);
    AddMask<MaskType>(testData.dataStructure, {1, k_Dimension, k_Dimension}, maskValues);
    auto args = CreateArguments();
    args.insertOrAssign(PottsModelFilter::k_UseMask_Key, std::make_any<bool>(true));

    PottsModelFilter filter;
    auto executeResult = filter.execute(testData.dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    const auto outputFeatureIds = CopyFeatureIds(testData.dataStructure);
    for(usize index = 0; index < maskValues.size(); index++)
    {
      if(maskValues[index] == 0)
      {
        REQUIRE(outputFeatureIds[index] == inputFeatureIds[index]);
      }
    }
  };

  SECTION("Boolean mask")
  {
    runMaskedTest(bool{});
  }

  SECTION("UInt8 mask")
  {
    runMaskedTest(uint8{});
  }
}

TEST_CASE("SimplnxCore::PottsModelFilter: FromSIMPLJson", "[SimplnxCore][PottsModelFilter]")
{
  const nlohmann::json legacyJson = {
      {"Iterations", 17},
      {"Temperature", 650.0},
      {"PeriodicBoundaries", true},
      {"UseMask", 1},
      {"MaskArrayPath", {{"Data Container Name", "DataContainer"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "Mask"}}},
      {"FeatureIdsArrayPath", {{"Data Container Name", "DataContainer"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "FeatureIds"}}},
  };

  auto conversionResult = PottsModelFilter::FromSIMPLJson(legacyJson);
  SIMPLNX_RESULT_REQUIRE_VALID(conversionResult);
  const Arguments& args = conversionResult.value();
  REQUIRE(args.value<int32>(PottsModelFilter::k_Iterations_Key) == 17);
  REQUIRE(args.value<float64>(PottsModelFilter::k_Temperature_Key) == 650.0);
  REQUIRE(args.value<bool>(PottsModelFilter::k_PeriodicBoundaries_Key));
  REQUIRE(args.value<bool>(PottsModelFilter::k_UseMask_Key));
  REQUIRE(args.value<DataPath>(PottsModelFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "Mask"}));
  REQUIRE(args.value<DataPath>(PottsModelFilter::k_FeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "FeatureIds"}));
}

TEST_CASE("SimplnxCore::PottsModelFilter: SIMPL Backwards Compatibility", "[SimplnxCore][PottsModelFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path fixturePath = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion" / "6_5" / "PottsModelFilter.json";
  auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
  REQUIRE(pipelineResult.valid());

  auto& pipeline = pipelineResult.value();
  REQUIRE(pipeline.size() == 1);

  auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
  REQUIRE(pipelineFilter != nullptr);

  const IFilter* filter = pipelineFilter->getFilter();
  REQUIRE(filter != nullptr);
  REQUIRE(filter->uuid() == FilterTraits<PottsModelFilter>::uuid);

  REQUIRE(pipelineFilter->getComments().empty());

  const Arguments args = pipelineFilter->getArguments();
  REQUIRE(args.value<int32>(PottsModelFilter::k_Iterations_Key) == 37);
  REQUIRE(args.value<float64>(PottsModelFilter::k_Temperature_Key) == 456.75);
  REQUIRE(args.value<bool>(PottsModelFilter::k_PeriodicBoundaries_Key));
  REQUIRE(args.value<bool>(PottsModelFilter::k_UseMask_Key));
}
