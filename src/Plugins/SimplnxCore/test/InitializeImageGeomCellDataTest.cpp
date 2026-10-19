#include "SimplnxCore/Filters/InitializeImageGeomCellDataFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <random>
#include <stdexcept>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
const DataPath k_ImageGeomPath{{"ImageGeom"}};
const DataPath k_Int32ArrayPath = k_ImageGeomPath.createChildPath("Int32Array");
const DataPath k_Float32ArrayPath = k_ImageGeomPath.createChildPath("Float32Array");

const std::vector<usize> k_ImageDims = {23, 24, 25};
const std::vector<usize> k_ComponentDims = {3};

Arguments CreateArgs(std::vector<DataPath> cellArrayPaths, DataPath imageGeomPath, uint64 xMin, uint64 yMin, uint64 zMin, uint64 xMax, uint64 yMax, uint64 zMax,
                     InitializeImageGeomCellDataFilter::InitType initType, float64 initValue, std::pair<float64, float64> initRange, uint64 seed = 5489)
{
  Arguments args;

  args.insert(InitializeImageGeomCellDataFilter::k_CellArrayPaths_Key, std::make_any<std::vector<DataPath>>(std::move(cellArrayPaths)));
  args.insert(InitializeImageGeomCellDataFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(std::move(imageGeomPath)));
  args.insert(InitializeImageGeomCellDataFilter::k_MinPoint_Key, std::make_any<std::vector<uint64>>({xMin, yMin, zMin}));
  args.insert(InitializeImageGeomCellDataFilter::k_MaxPoint_Key, std::make_any<std::vector<uint64>>({xMax, yMax, zMax}));
  args.insert(InitializeImageGeomCellDataFilter::k_InitType_Key, std::make_any<uint64>(to_underlying(initType)));
  args.insert(InitializeImageGeomCellDataFilter::k_InitValue_Key, std::make_any<float64>(initValue));
  args.insert(InitializeImageGeomCellDataFilter::k_InitRange_Key, std::make_any<std::vector<float64>>({initRange.first, initRange.second}));
  args.insert(InitializeImageGeomCellDataFilter::k_UseSeed_Key, std::make_any<bool>(true));
  args.insert(InitializeImageGeomCellDataFilter::k_SeedValue_Key, std::make_any<uint64>(seed));
  args.insert(InitializeImageGeomCellDataFilter::k_SeedArrayName_Key, std::make_any<std::string>("InitializeImageGeomCellData Seed"));

  return args;
}

DataStructure CreateDataStructure(const std::vector<usize>& imageDims = k_ImageDims, bool useConfiguredStores = false)
{
  DataStructure dataStructure;

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomPath.getTargetName());
  REQUIRE(imageGeom != nullptr);

  imageGeom->setDimensions(imageDims);
  const std::vector<usize> arrayDims(imageDims.crbegin(), imageDims.crend());

  auto int32Store = useConfiguredStores ? DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_Int32ArrayPath, arrayDims, k_ComponentDims) :
                                          std::make_shared<Int32DataStore>(arrayDims, k_ComponentDims, std::nullopt);
  Int32Array* int32Array = Int32Array::Create(dataStructure, k_Int32ArrayPath.getTargetName(), int32Store, imageGeom->getId());
  REQUIRE(int32Array != nullptr);

  int32Array->fill(0);

  auto float32Store = useConfiguredStores ? DataStoreUtilities::CreateDataStore<float32>(dataStructure, k_Float32ArrayPath, arrayDims, k_ComponentDims) :
                                            std::make_shared<Float32DataStore>(arrayDims, k_ComponentDims, std::nullopt);
  Float32Array* float32Array = Float32Array::Create(dataStructure, k_Float32ArrayPath.getTargetName(), float32Store, imageGeom->getId());
  REQUIRE(float32Array != nullptr);

  float32Array->fill(0.0f);

  return dataStructure;
}

template <class T>
std::vector<T> ReadValues(const DataStructure& dataStructure, const DataPath& path)
{
  const DataArray<T>* array = nullptr;
  REQUIRE_NOTHROW(array = &dataStructure.getDataRefAs<DataArray<T>>(path));
  std::vector<T> values(array->getNumberOfTuples() * array->getNumberOfComponents());
  auto arrayReadResult = array->getDataStoreRef().copyIntoBuffer(0, nonstd::span<T>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(arrayReadResult);
  return values;
}

template <class T>
std::vector<T> ExpectedValues(const std::vector<usize>& imageDims, uint64 xMin, uint64 yMin, uint64 zMin, uint64 xMax, uint64 yMax, uint64 zMax, InitializeImageGeomCellDataFilter::InitType initType,
                              float64 initValue, std::pair<float64, float64> initRange, uint64 seed)
{
  constexpr usize k_NumComponents = 3;
  const usize tupleCount = imageDims[0] * imageDims[1] * imageDims[2];
  std::vector<T> values(tupleCount * k_NumComponents, 0);
  std::mt19937_64 generator(seed);

  if constexpr(std::is_integral_v<T>)
  {
    std::uniform_int_distribution<> distribution(initType == InitializeImageGeomCellDataFilter::InitType::RandomWithRange ? static_cast<T>(initRange.first) : std::numeric_limits<T>::min(),
                                                 initType == InitializeImageGeomCellDataFilter::InitType::RandomWithRange ? static_cast<T>(initRange.second) : std::numeric_limits<T>::max());
    for(uint64 z = zMin; z <= zMax; z++)
    {
      for(uint64 y = yMin; y <= yMax; y++)
      {
        for(uint64 x = xMin; x <= xMax; x++)
        {
          const T value = initType == InitializeImageGeomCellDataFilter::InitType::Manual ? static_cast<T>(initValue) : distribution(generator);
          const usize tupleIndex = (z * imageDims[0] * imageDims[1]) + (y * imageDims[0]) + x;
          std::fill_n(values.data() + (tupleIndex * k_NumComponents), k_NumComponents, value);
        }
      }
    }
  }
  else
  {
    std::uniform_real_distribution<T> distribution(initType == InitializeImageGeomCellDataFilter::InitType::RandomWithRange ? static_cast<T>(initRange.first) : std::numeric_limits<T>::min(),
                                                   initType == InitializeImageGeomCellDataFilter::InitType::RandomWithRange ? static_cast<T>(initRange.second) : std::numeric_limits<T>::max());
    for(uint64 z = zMin; z <= zMax; z++)
    {
      for(uint64 y = yMin; y <= yMax; y++)
      {
        for(uint64 x = xMin; x <= xMax; x++)
        {
          const T value = initType == InitializeImageGeomCellDataFilter::InitType::Manual ? static_cast<T>(initValue) : distribution(generator);
          const usize tupleIndex = (z * imageDims[0] * imageDims[1]) + (y * imageDims[0]) + x;
          std::fill_n(values.data() + (tupleIndex * k_NumComponents), k_NumComponents, value);
        }
      }
    }
  }

  return values;
}

struct InitializationOutputs
{
  std::vector<int32> int32Values;
  std::vector<float32> float32Values;
};

InitializationOutputs ExecuteInitialization(UnitTest::AlgorithmTestScenario scenario, const std::vector<usize>& imageDims, InitializeImageGeomCellDataFilter::InitType initType, float64 initValue,
                                            std::pair<float64, float64> initRange, uint64 seed, bool useConfiguredStores)
{
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure = CreateDataStructure(imageDims, useConfiguredStores);
  const auto cellArrayPaths = std::vector<DataPath>{k_Int32ArrayPath, k_Float32ArrayPath};
  const IDataArray* int32Array = nullptr;
  const IDataArray* float32Array = nullptr;
  REQUIRE_NOTHROW(int32Array = &dataStructure.getDataRefAs<IDataArray>(k_Int32ArrayPath));
  REQUIRE_NOTHROW(float32Array = &dataStructure.getDataRefAs<IDataArray>(k_Float32ArrayPath));
  scope.requireExpectedStore(*int32Array);
  scope.requireExpectedStore(*float32Array);

  InitializeImageGeomCellDataFilter filter;
  constexpr uint64 k_XMin = 1;
  constexpr uint64 k_YMin = 0;
  constexpr uint64 k_ZMin = 0;
  const uint64 xMax = imageDims[0] - 2;
  const uint64 yMax = imageDims[1] - 1;
  const uint64 zMax = imageDims[2] - 1;
  Arguments args = CreateArgs(cellArrayPaths, k_ImageGeomPath, k_XMin, k_YMin, k_ZMin, xMax, yMax, zMax, initType, initValue, initRange, seed);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const UInt64Array* storedSeed = nullptr;
  REQUIRE_NOTHROW(storedSeed = &dataStructure.getDataRefAs<UInt64Array>(DataPath({"InitializeImageGeomCellData Seed"})));
  REQUIRE((*storedSeed)[0] == seed);
  return {ReadValues<int32>(dataStructure, k_Int32ArrayPath), ReadValues<float32>(dataStructure, k_Float32ArrayPath)};
}

template <class T, class PredicateT>
bool DoesRangeSatisfyCondition(const IDataStore& dataStore, uint64 xMin, uint64 yMin, uint64 zMin, uint64 xMax, uint64 yMax, uint64 zMax, PredicateT&& predicate)
{
  auto& dataStoreTyped = dynamic_cast<const AbstractDataStore<T>&>(dataStore);
  // Tuple dimensions use {Z, Y, X} order.
  auto dims = dataStoreTyped.getTupleShape();
  usize numComps = dataStoreTyped.getNumberOfComponents();

  for(uint64 k = zMin; k < zMax + 1; k++)
  {
    for(uint64 j = yMin; j < yMax + 1; j++)
    {
      for(uint64 i = xMin; i < xMax + 1; i++)
      {
        usize tuple = (k * dims[1] * dims[2]) + (j * dims[2]) + i;
        for(usize c = 0; c < numComps; c++)
        {
          T value = dataStoreTyped.getComponentValue(tuple, c);
          if(!predicate(value))
          {
            return false;
          }
        }
      }
    }
  }

  return true;
}

struct DoesRangeEqualValueFunctor
{
  template <class T>
  bool operator()(const IDataStore& dataStore, uint64 xMin, uint64 xMax, uint64 yMin, uint64 yMax, uint64 zMin, uint64 zMax, float64 expectedValue)
  {
    return DoesRangeSatisfyCondition<T>(dataStore, xMin, yMin, zMin, xMax, yMax, zMax, [expectedValue](T value) { return value == expectedValue; });
  }
};

struct IsDataWithinInclusiveRangeFunctor
{
  template <class T>
  bool operator()(const IDataStore& dataStore, uint64 xMin, uint64 xMax, uint64 yMin, uint64 yMax, uint64 zMin, uint64 zMax, std::pair<float64, float64> range)
  {
    return DoesRangeSatisfyCondition<T>(dataStore, xMin, yMin, zMin, xMax, yMax, zMax, [range](T value) { return value >= range.first && value <= range.second; });
  }
};
} // namespace

TEST_CASE("SimplnxCore::InitializeImageGeomCellDataFilter: Bounded Bulk Write Semantics", "[SimplnxCore][InitializeImageGeomCellDataFilter]")
{
  UnitTest::LoadPlugins();

  constexpr uint64 k_Seed = 8421;
  const std::vector<usize> imageDims = {22000, 3, 2};
  constexpr uint64 k_XMin = 1;
  constexpr uint64 k_YMin = 0;
  constexpr uint64 k_ZMin = 0;
  const uint64 xMax = imageDims[0] - 2;
  const uint64 yMax = imageDims[1] - 1;
  const uint64 zMax = imageDims[2] - 1;

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  SECTION("Manual")
  {
    const auto actual = ExecuteInitialization(scenario, imageDims, InitializeImageGeomCellDataFilter::InitType::Manual, 42.0, {0.0, 0.0}, k_Seed, true);
    REQUIRE(actual.int32Values == ExpectedValues<int32>(imageDims, k_XMin, k_YMin, k_ZMin, xMax, yMax, zMax, InitializeImageGeomCellDataFilter::InitType::Manual, 42.0, {0.0, 0.0}, k_Seed));
    REQUIRE(actual.float32Values == ExpectedValues<float32>(imageDims, k_XMin, k_YMin, k_ZMin, xMax, yMax, zMax, InitializeImageGeomCellDataFilter::InitType::Manual, 42.0, {0.0, 0.0}, k_Seed + 1));
  }

  SECTION("Random")
  {
    const auto actual = ExecuteInitialization(scenario, imageDims, InitializeImageGeomCellDataFilter::InitType::Random, 0.0, {0.0, 0.0}, k_Seed, true);
    REQUIRE(actual.int32Values == ExpectedValues<int32>(imageDims, k_XMin, k_YMin, k_ZMin, xMax, yMax, zMax, InitializeImageGeomCellDataFilter::InitType::Random, 0.0, {0.0, 0.0}, k_Seed));
    REQUIRE(actual.float32Values == ExpectedValues<float32>(imageDims, k_XMin, k_YMin, k_ZMin, xMax, yMax, zMax, InitializeImageGeomCellDataFilter::InitType::Random, 0.0, {0.0, 0.0}, k_Seed + 1));
  }
}

#if SIMPLNX_TEST_ALGORITHM_PATH == 0
TEST_CASE("SimplnxCore::InitializeImageGeomCellDataFilter: Direct and Forced OOC Parity", "[SimplnxCore][InitializeImageGeomCellDataFilter]")
{
  UnitTest::LoadPlugins();

  constexpr uint64 k_Seed = 8421;
  const std::vector<usize> imageDims = {22000, 3, 2};
  const auto scenarios = UnitTest::SelectAlgorithmTestScenariosForInMemoryStores();
  const auto directScenario = std::find(scenarios.cbegin(), scenarios.cend(), UnitTest::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore);
  const auto forcedScenario = std::find(scenarios.cbegin(), scenarios.cend(), UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
  REQUIRE(directScenario != scenarios.cend());
  REQUIRE(forcedScenario != scenarios.cend());

  SECTION("Manual")
  {
    const auto direct = ExecuteInitialization(*directScenario, imageDims, InitializeImageGeomCellDataFilter::InitType::Manual, 42.0, {0.0, 0.0}, k_Seed, true);
    const auto forced = ExecuteInitialization(*forcedScenario, imageDims, InitializeImageGeomCellDataFilter::InitType::Manual, 42.0, {0.0, 0.0}, k_Seed, true);
    REQUIRE(forced.int32Values == direct.int32Values);
    REQUIRE(forced.float32Values == direct.float32Values);
  }

  SECTION("Random")
  {
    const auto direct = ExecuteInitialization(*directScenario, imageDims, InitializeImageGeomCellDataFilter::InitType::Random, 0.0, {0.0, 0.0}, k_Seed, true);
    const auto forced = ExecuteInitialization(*forcedScenario, imageDims, InitializeImageGeomCellDataFilter::InitType::Random, 0.0, {0.0, 0.0}, k_Seed, true);
    REQUIRE(forced.int32Values == direct.int32Values);
    REQUIRE(forced.float32Values == direct.float32Values);
  }
}
#endif

TEST_CASE("SimplnxCore::InitializeImageGeomCellDataFilter(Manual)", "[SimplnxCore][InitializeImageGeomCellDataFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  InitializeImageGeomCellDataFilter filter;
  DataStructure dataStructure = CreateDataStructure();

  constexpr uint64 xMin = 3;
  constexpr uint64 yMin = 4;
  constexpr uint64 zMin = 0;
  constexpr uint64 xMax = 13;
  constexpr uint64 yMax = 14;
  constexpr uint64 zMax = 24;
  constexpr float64 initValue = 42.0;
  const std::vector<DataPath> cellArrayPaths = {k_Int32ArrayPath, k_Float32ArrayPath};
  Arguments args = CreateArgs(cellArrayPaths, k_ImageGeomPath, xMin, yMin, zMin, xMax, yMax, zMax, InitializeImageGeomCellDataFilter::InitType::Manual, initValue, {0.0, 0.0});

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  for(const auto& path : cellArrayPaths)
  {
    const auto& dataArray = dataStructure.getDataRefAs<IDataArray>(path);
    const auto& dataStore = dataArray.getIDataStoreRef();

    DataType type = dataStore.getDataType();

    // Values inside the range must change. Values outside it must remain unchanged.
    REQUIRE(ExecuteNeighborFunction(DoesRangeEqualValueFunctor{}, type, dataStore, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));                                                    // No bool
    REQUIRE(ExecuteNeighborFunction(DoesRangeEqualValueFunctor{}, type, dataStore, xMin, xMax, yMin, yMax, zMin, zMax, initValue));                                                 // No bool
    REQUIRE(ExecuteNeighborFunction(DoesRangeEqualValueFunctor{}, type, dataStore, xMax + 1, k_ImageDims[0] - 1, yMax + 1, k_ImageDims[1] - 1, zMax + 1, k_ImageDims[2] - 1, 0.0)); // No bool
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::InitializeImageGeomCellDataFilter(Random)", "[SimplnxCore][InitializeImageGeomCellDataFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  InitializeImageGeomCellDataFilter filter;
  DataStructure dataStructure = CreateDataStructure();

  constexpr uint64 xMin = 3;
  constexpr uint64 yMin = 4;
  constexpr uint64 zMin = 0;
  constexpr uint64 xMax = 13;
  constexpr uint64 yMax = 14;
  constexpr uint64 zMax = 24;
  const std::vector<DataPath> cellArrayPaths = {k_Int32ArrayPath, k_Float32ArrayPath};
  Arguments args = CreateArgs(cellArrayPaths, k_ImageGeomPath, xMin, yMin, zMin, xMax, yMax, zMax, InitializeImageGeomCellDataFilter::InitType::Random, 0.0, {0.0, 0.0});

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  for(const auto& path : cellArrayPaths)
  {
    const auto& dataArray = dataStructure.getDataRefAs<IDataArray>(path);
    const auto& dataStore = dataArray.getIDataStoreRef();

    DataType type = dataStore.getDataType();

    // Values outside the range must remain unchanged. The range values are random.

    REQUIRE(ExecuteNeighborFunction(DoesRangeEqualValueFunctor{}, type, dataStore, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));                                                    // No bool
    REQUIRE(ExecuteNeighborFunction(DoesRangeEqualValueFunctor{}, type, dataStore, xMax + 1, k_ImageDims[0] - 1, yMax + 1, k_ImageDims[1] - 1, zMax + 1, k_ImageDims[2] - 1, 0.0)); // No bool
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::InitializeImageGeomCellDataFilter(RandomWithRange)", "[SimplnxCore][InitializeImageGeomCellDataFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  InitializeImageGeomCellDataFilter filter;
  DataStructure dataStructure = CreateDataStructure();

  constexpr uint64 xMin = 3;
  constexpr uint64 yMin = 4;
  constexpr uint64 zMin = 0;
  constexpr uint64 xMax = 13;
  constexpr uint64 yMax = 14;
  constexpr uint64 zMax = 24;
  constexpr std::pair<float64, float64> initRange = {1.0, 25.0};
  const std::vector<DataPath> cellArrayPaths = {k_Int32ArrayPath, k_Float32ArrayPath};
  Arguments args = CreateArgs(cellArrayPaths, k_ImageGeomPath, xMin, yMin, zMin, xMax, yMax, zMax, InitializeImageGeomCellDataFilter::InitType::RandomWithRange, 0.0, initRange);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  for(const auto& path : cellArrayPaths)
  {
    const auto& dataArray = dataStructure.getDataRefAs<IDataArray>(path);
    const auto& dataStore = dataArray.getIDataStoreRef();

    DataType type = dataStore.getDataType();

    // Values inside the range must use the requested range. Values outside it must remain unchanged.
    REQUIRE(ExecuteNeighborFunction(DoesRangeEqualValueFunctor{}, type, dataStore, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));                                                    // No bool
    REQUIRE(ExecuteNeighborFunction(IsDataWithinInclusiveRangeFunctor{}, type, dataStore, xMin, xMax, yMin, yMax, zMin, zMax, initRange));                                          // No bool
    REQUIRE(ExecuteNeighborFunction(DoesRangeEqualValueFunctor{}, type, dataStore, xMax + 1, k_ImageDims[0] - 1, yMax + 1, k_ImageDims[1] - 1, zMax + 1, k_ImageDims[2] - 1, 0.0)); // No bool
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::InitializeImageGeomCellDataFilter: SIMPL Backwards Compatibility", "[SimplnxCore][InitializeImageGeomCellDataFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "InitializeImageGeomCellDataFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "InitializeImageGeomCellDataFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<InitializeImageGeomCellDataFilter>::uuid);

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<ChoicesParameter::ValueType>(InitializeImageGeomCellDataFilter::k_InitType_Key) == 0);
      CHECK(args.value<float64>(InitializeImageGeomCellDataFilter::k_InitValue_Key) == 2.5);
      CHECK(args.value<std::vector<float64>>(InitializeImageGeomCellDataFilter::k_InitRange_Key) == std::vector<float64>{0.0, 1.0});
      CHECK(args.value<std::vector<uint64>>(InitializeImageGeomCellDataFilter::k_MinPoint_Key) == std::vector<uint64>{0, 0, 0});
      CHECK(args.value<std::vector<uint64>>(InitializeImageGeomCellDataFilter::k_MaxPoint_Key) == std::vector<uint64>{5, 5, 5});
    }
  }
}

TEST_CASE("SimplnxCore::InitializeImageGeomCellDataFilter: Geometry cell tuple count", "[SimplnxCore][InitializeImageGeomCellDataFilter][TupleCount]")
{
  using Filter = InitializeImageGeomCellDataFilter;
  DataStructure dataStructure;
  auto* geometry = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(geometry != nullptr);
  geometry->setDimensions({2, 3, 2});
  geometry->setSpacing({1.0F, 1.0F, 1.0F});
  auto* cellAM = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{12}, geometry->getId());
  REQUIRE(cellAM != nullptr);
  geometry->setCellData(*cellAM);

  // A matching array is valid outside Cell Data; short and long arrays must fail preflight.
  const usize tuples = GENERATE(usize{0}, usize{11}, usize{12}, usize{13});
  CAPTURE(tuples);
  auto* otherAM = AttributeMatrix::Create(dataStructure, "Other Data", ShapeType{1, 1, tuples});
  REQUIRE(otherAM != nullptr);
  REQUIRE(DataArray<int32>::Create(dataStructure, "Values", std::make_shared<DataStore<int32>>(otherAM->getShape(), ShapeType{1}, 0), otherAM->getId()) != nullptr);
  const DataPath inputPath({"Other Data", "Values"});
  const bool shortSecondInput = GENERATE(false, true);
  CAPTURE(shortSecondInput);
  auto* secondAM = AttributeMatrix::Create(dataStructure, "Second Data", ShapeType{1, 1, shortSecondInput ? usize{10} : usize{12}});
  REQUIRE(secondAM != nullptr);
  REQUIRE(DataArray<int32>::Create(dataStructure, "Second Values", std::make_shared<DataStore<int32>>(secondAM->getShape(), ShapeType{1}, 0), secondAM->getId()) != nullptr);
  const DataPath secondPath({"Second Data", "Second Values"});

  Filter filter;
  Arguments args;
  args.insertOrAssign(Filter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(Filter::k_CellArrayPaths_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{inputPath, secondPath}));

  const auto result = filter.preflight(dataStructure, args);
  if(tuples == 12 && !shortSecondInput)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    const auto& error = result.outputActions.errors()[0];
    REQUIRE(error.code == -3557);
    const DataPath expectedPath = tuples != 12 ? inputPath : secondPath;
    const usize expectedTuples = tuples != 12 ? tuples : usize{10};
    REQUIRE(error.message.find(expectedPath.toString()) != std::string::npos);
    REQUIRE(error.message.find(fmt::format("has {} tuples", expectedTuples)) != std::string::npos);
    REQUIRE(error.message.find("'Image' has 12 cells") != std::string::npos);
  }
}

TEST_CASE("SimplnxCore::InitializeImageGeomCellDataFilter: External tuple shapes", "[SimplnxCore][InitializeImageGeomCellDataFilter][TupleCount]")
{
  const auto tupleShape = GENERATE((ShapeType{12}), (ShapeType{3, 4}), (ShapeType{1, 1, 12}));
  CAPTURE(tupleShape);
  DataStructure dataStructure;
  auto* geometry = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(geometry != nullptr);
  geometry->setDimensions({3, 4, 1});
  auto* cellAM = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{1, 4, 3}, geometry->getId());
  REQUIRE(cellAM != nullptr);
  geometry->setCellData(*cellAM);
  auto* otherAM = AttributeMatrix::Create(dataStructure, "Other Data", tupleShape);
  REQUIRE(otherAM != nullptr);
  REQUIRE(Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Values", tupleShape, ShapeType{1}, otherAM->getId()) != nullptr);

  InitializeImageGeomCellDataFilter filter;
  Arguments args;
  args.insertOrAssign(InitializeImageGeomCellDataFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(InitializeImageGeomCellDataFilter::k_CellArrayPaths_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{DataPath({"Other Data", "Values"})}));
  args.insertOrAssign(InitializeImageGeomCellDataFilter::k_MinPoint_Key, std::make_any<std::vector<uint64>>(std::vector<uint64>{0, 0, 0}));
  args.insertOrAssign(InitializeImageGeomCellDataFilter::k_MaxPoint_Key, std::make_any<std::vector<uint64>>(std::vector<uint64>{2, 3, 0}));

  const auto result = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
