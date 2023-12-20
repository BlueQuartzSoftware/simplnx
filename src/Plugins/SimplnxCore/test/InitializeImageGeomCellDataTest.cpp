#include "SimplnxCore/Filters/InitializeImageGeomCellData.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <catch2/catch.hpp>

#include <stdexcept>

using namespace nx::core;

namespace
{
const DataPath k_ImageGeomPath{{"ImageGeom"}};
const DataPath k_Int32ArrayPath = k_ImageGeomPath.createChildPath("Int32Array");
const DataPath k_Float32ArrayPath = k_ImageGeomPath.createChildPath("Float32Array");

const std::vector<usize> k_ImageDims = {23, 24, 25};
const std::vector<usize> k_ArrayDims(k_ImageDims.crbegin(), k_ImageDims.crend());
const std::vector<usize> k_ComponentDims = {3};

Arguments CreateArgs(std::vector<DataPath> cellArrayPaths, DataPath imageGeomPath, uint64 xMin, uint64 yMin, uint64 zMin, uint64 xMax, uint64 yMax, uint64 zMax,
                     InitializeImageGeomCellData::InitType initType, float64 initValue, std::pair<float64, float64> initRange)
{
  Arguments args;

  args.insert(InitializeImageGeomCellData::k_CellArrayPaths_Key, std::make_any<std::vector<DataPath>>(std::move(cellArrayPaths)));
  args.insert(InitializeImageGeomCellData::k_ImageGeometryPath_Key, std::make_any<DataPath>(std::move(imageGeomPath)));
  args.insert(InitializeImageGeomCellData::k_MinPoint_Key, std::make_any<std::vector<uint64>>({xMin, yMin, zMin}));
  args.insert(InitializeImageGeomCellData::k_MaxPoint_Key, std::make_any<std::vector<uint64>>({xMax, yMax, zMax}));
  args.insert(InitializeImageGeomCellData::k_InitType_Key, std::make_any<uint64>(to_underlying(initType)));
  args.insert(InitializeImageGeomCellData::k_InitValue_Key, std::make_any<float64>(initValue));
  args.insert(InitializeImageGeomCellData::k_InitRange_Key, std::make_any<std::vector<float64>>({initRange.first, initRange.second}));

  return args;
}

DataStructure CreateDataStructure()
{
  DataStructure dataStructure;

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomPath.getTargetName());
  REQUIRE(imageGeom != nullptr);

  imageGeom->setDimensions(k_ImageDims);

  Int32Array* int32Array = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_Int32ArrayPath.getTargetName(), k_ArrayDims, k_ComponentDims, imageGeom->getId());
  REQUIRE(int32Array != nullptr);

  int32Array->fill(0);

  Float32Array* float32Array = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, k_Float32ArrayPath.getTargetName(), k_ArrayDims, k_ComponentDims, imageGeom->getId());
  REQUIRE(float32Array != nullptr);

  float32Array->fill(0.0f);

  return dataStructure;
}

template <class T, class PredicateT>
bool DoesRangeSatisfyCondition(const IDataStore& dataStore, uint64 xMin, uint64 yMin, uint64 zMin, uint64 xMax, uint64 yMax, uint64 zMax, PredicateT&& predicate)
{
  auto& dataStoreTyped = dynamic_cast<const AbstractDataStore<T>&>(dataStore);
  // Z Y X
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

TEST_CASE("SimplnxCore::InitializeImageGeomCellData(Manual)", "[SimplnxCore][InitializeImageGeomCellData]")
{
  InitializeImageGeomCellData filter;
  DataStructure dataStructure = CreateDataStructure();

  constexpr uint64 xMin = 3;
  constexpr uint64 yMin = 4;
  constexpr uint64 zMin = 0;
  constexpr uint64 xMax = 13;
  constexpr uint64 yMax = 14;
  constexpr uint64 zMax = 24;
  constexpr float64 initValue = 42.0;
  const std::vector<DataPath> cellArrayPaths = {k_Int32ArrayPath, k_Float32ArrayPath};
  Arguments args = CreateArgs(cellArrayPaths, k_ImageGeomPath, xMin, yMin, zMin, xMax, yMax, zMax, InitializeImageGeomCellData::InitType::Manual, initValue, {0.0, 0.0});

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  for(const auto& path : cellArrayPaths)
  {
    const auto& dataArray = dataStructure.getDataRefAs<IDataArray>(path);
    const auto& dataStore = dataArray.getIDataStoreRef();

    DataType type = dataStore.getDataType();

    // Check that the data inside the range is changed to the correct value and the data outside it is unchanged
    REQUIRE(ExecuteNeighborFunction(DoesRangeEqualValueFunctor{}, type, dataStore, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));                                                    // No bool
    REQUIRE(ExecuteNeighborFunction(DoesRangeEqualValueFunctor{}, type, dataStore, xMin, xMax, yMin, yMax, zMin, zMax, initValue));                                                 // No bool
    REQUIRE(ExecuteNeighborFunction(DoesRangeEqualValueFunctor{}, type, dataStore, xMax + 1, k_ImageDims[0] - 1, yMax + 1, k_ImageDims[1] - 1, zMax + 1, k_ImageDims[2] - 1, 0.0)); // No bool
  }
}

TEST_CASE("SimplnxCore::InitializeImageGeomCellData(Random)", "[SimplnxCore][InitializeImageGeomCellData]")
{
  InitializeImageGeomCellData filter;
  DataStructure dataStructure = CreateDataStructure();

  constexpr uint64 xMin = 3;
  constexpr uint64 yMin = 4;
  constexpr uint64 zMin = 0;
  constexpr uint64 xMax = 13;
  constexpr uint64 yMax = 14;
  constexpr uint64 zMax = 24;
  const std::vector<DataPath> cellArrayPaths = {k_Int32ArrayPath, k_Float32ArrayPath};
  Arguments args = CreateArgs(cellArrayPaths, k_ImageGeomPath, xMin, yMin, zMin, xMax, yMax, zMax, InitializeImageGeomCellData::InitType::Random, 0.0, {0.0, 0.0});

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  for(const auto& path : cellArrayPaths)
  {
    const auto& dataArray = dataStructure.getDataRefAs<IDataArray>(path);
    const auto& dataStore = dataArray.getIDataStoreRef();

    DataType type = dataStore.getDataType();

    // Check that the data outside the range is not changed
    // Since the data inside the range is random, we cannot check it
    REQUIRE(ExecuteNeighborFunction(DoesRangeEqualValueFunctor{}, type, dataStore, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));                                                    // No bool
    REQUIRE(ExecuteNeighborFunction(DoesRangeEqualValueFunctor{}, type, dataStore, xMax + 1, k_ImageDims[0] - 1, yMax + 1, k_ImageDims[1] - 1, zMax + 1, k_ImageDims[2] - 1, 0.0)); // No bool
  }
}

TEST_CASE("SimplnxCore::InitializeImageGeomCellData(RandomWithRange)", "[SimplnxCore][InitializeImageGeomCellData]")
{
  InitializeImageGeomCellData filter;
  DataStructure dataStructure = CreateDataStructure();

  constexpr uint64 xMin = 3;
  constexpr uint64 yMin = 4;
  constexpr uint64 zMin = 0;
  constexpr uint64 xMax = 13;
  constexpr uint64 yMax = 14;
  constexpr uint64 zMax = 24;
  constexpr std::pair<float64, float64> initRange = {1.0, 25.0};
  const std::vector<DataPath> cellArrayPaths = {k_Int32ArrayPath, k_Float32ArrayPath};
  Arguments args = CreateArgs(cellArrayPaths, k_ImageGeomPath, xMin, yMin, zMin, xMax, yMax, zMax, InitializeImageGeomCellData::InitType::RandomWithRange, 0.0, initRange);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  for(const auto& path : cellArrayPaths)
  {
    const auto& dataArray = dataStructure.getDataRefAs<IDataArray>(path);
    const auto& dataStore = dataArray.getIDataStoreRef();

    DataType type = dataStore.getDataType();

    // Check that the data inside the range is within the given range and that the data outside it is unchanged
    REQUIRE(ExecuteNeighborFunction(DoesRangeEqualValueFunctor{}, type, dataStore, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));                                                    // No bool
    REQUIRE(ExecuteNeighborFunction(IsDataWithinInclusiveRangeFunctor{}, type, dataStore, xMin, xMax, yMin, yMax, zMin, zMax, initRange));                                          // No bool
    REQUIRE(ExecuteNeighborFunction(DoesRangeEqualValueFunctor{}, type, dataStore, xMax + 1, k_ImageDims[0] - 1, yMax + 1, k_ImageDims[1] - 1, zMax + 1, k_ImageDims[2] - 1, 0.0)); // No bool
  }
}
