#include "ComplexCore/Filters/InitializeData.hpp"
#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/Common/TypeTraits.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <stdexcept>

using namespace complex;

namespace
{
const DataPath k_ImageGeomPath{{"ImageGeom"}};
const DataPath k_Int32ArrayPath = k_ImageGeomPath.createChildPath("Int32Array");
const DataPath k_Float32ArrayPath = k_ImageGeomPath.createChildPath("Float32Array");

const std::vector<usize> k_ImageDims = {25, 25, 25};
const std::vector<usize> k_ArrayDims(k_ImageDims.crbegin(), k_ImageDims.crend());
const std::vector<usize> k_ComponentDims = {3};

Arguments CreateArgs(std::vector<DataPath> cellArrayPaths, DataPath imageGeomPath, uint64 xMin, uint64 yMin, uint64 zMin, uint64 xMax, uint64 yMax, uint64 zMax, InitializeData::InitType initType,
                     float64 initValue, std::pair<float64, float64> initRange)
{
  Arguments args;

  args.insert(InitializeData::k_CellArrayPaths_Key, std::make_any<std::vector<DataPath>>(std::move(cellArrayPaths)));
  args.insert(InitializeData::k_ImageGeometryPath_Key, std::make_any<DataPath>(std::move(imageGeomPath)));
  args.insert(InitializeData::k_MinPoint_Key, std::make_any<std::vector<uint64>>({xMin, yMin, zMin}));
  args.insert(InitializeData::k_MaxPoint_Key, std::make_any<std::vector<uint64>>({xMax, yMax, zMax}));
  args.insert(InitializeData::k_InitType_Key, std::make_any<uint64>(to_underlying(initType)));
  args.insert(InitializeData::k_InitValue_Key, std::make_any<float64>(initValue));
  args.insert(InitializeData::k_InitRange_Key, std::make_any<std::vector<float64>>({initRange.first, initRange.second}));

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
bool DoesRangeSatisfyCondition(const IDataStore& dataStore, const std::array<usize, 3>& dims, const std::vector<usize>& cDims, uint64 xMin, uint64 yMin, uint64 zMin, uint64 xMax, uint64 yMax,
                               uint64 zMax, PredicateT&& predicate)
{
  auto& dataStoreTyped = dynamic_cast<const AbstractDataStore<T>&>(dataStore);
  usize numComps = std::accumulate(cDims.begin(), cDims.end(), 1, std::multiplies<>());

  for(uint64 k = zMin; k < zMax + 1; k++)
  {
    for(uint64 j = yMin; j < yMax + 1; j++)
    {
      for(uint64 i = xMin; i < xMax + 1; i++)
      {
        usize tuple = (k * dims[0] * dims[1]) + (j * dims[0]) + i;
        for(int c = 0; c < numComps; c++)
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

template <class T>
bool DoesRangeEqualValue(const IDataStore& dataStore, const std::array<usize, 3>& dims, const std::vector<usize>& cDims, uint64 xMin, uint64 xMax, uint64 yMin, uint64 yMax, uint64 zMin, uint64 zMax,
                         float64 expectedValue)
{
  return DoesRangeSatisfyCondition<T>(dataStore, dims, cDims, xMin, yMin, zMin, xMax, yMax, zMax, [expectedValue](T value) { return value == expectedValue; });
}

template <class T>
bool IsDataWithinInclusiveRange(const IDataStore& dataStore, const std::array<usize, 3>& dims, const std::vector<usize>& cDims, uint64 xMin, uint64 xMax, uint64 yMin, uint64 yMax, uint64 zMin,
                                uint64 zMax, std::pair<float64, float64> range)
{
  return DoesRangeSatisfyCondition<T>(dataStore, dims, cDims, xMin, yMin, zMin, xMax, yMax, zMax, [range](T value) { return value >= range.first && value <= range.second; });
}
} // namespace

TEST_CASE("ComplexCore::InitializeData(Manual)", "[ComplexCore][InitializeData]")
{
  InitializeData filter;
  DataStructure dataStructure = CreateDataStructure();

  constexpr uint64 xMin = 5;
  constexpr uint64 yMin = 5;
  constexpr uint64 zMin = 5;
  constexpr uint64 xMax = 15;
  constexpr uint64 yMax = 15;
  constexpr uint64 zMax = 15;
  constexpr float64 initValue = 42.0;
  const std::vector<DataPath> cellArrayPaths = {k_Int32ArrayPath, k_Float32ArrayPath};
  Arguments args = CreateArgs(cellArrayPaths, k_ImageGeomPath, xMin, yMin, zMin, xMax, yMax, zMax, InitializeData::InitType::Manual, initValue, {0.0, 0.0});

  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  std::array<usize, 3> imageDims = {k_ImageDims.at(0), k_ImageDims.at(1), k_ImageDims.at(2)};

  for(const auto& path : cellArrayPaths)
  {
    const auto& dataArray = dataStructure.getDataRefAs<IDataArray>(path);
    const auto& dataStore = dataArray.getIDataStoreRef();

    DataType type = dataStore.getDataType();

    // Check that the data inside the range is changed to the correct value and the data outside it is unchanged

    switch(type)
    {
    case DataType::int8: {
      REQUIRE(DoesRangeEqualValue<int8>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(DoesRangeEqualValue<int8>(dataStore, imageDims, k_ComponentDims, xMin, xMax, yMin, yMax, zMin, zMax, initValue));
      REQUIRE(DoesRangeEqualValue<int8>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::int16: {
      REQUIRE(DoesRangeEqualValue<int16>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(DoesRangeEqualValue<int16>(dataStore, imageDims, k_ComponentDims, xMin, xMax, yMin, yMax, zMin, zMax, initValue));
      REQUIRE(DoesRangeEqualValue<int16>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::int32: {
      REQUIRE(DoesRangeEqualValue<int32>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(DoesRangeEqualValue<int32>(dataStore, imageDims, k_ComponentDims, xMin, xMax, yMin, yMax, zMin, zMax, initValue));
      REQUIRE(DoesRangeEqualValue<int32>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::int64: {
      REQUIRE(DoesRangeEqualValue<int64>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(DoesRangeEqualValue<int64>(dataStore, imageDims, k_ComponentDims, xMin, xMax, yMin, yMax, zMin, zMax, initValue));
      REQUIRE(DoesRangeEqualValue<int64>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::uint8: {
      REQUIRE(DoesRangeEqualValue<uint8>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(DoesRangeEqualValue<uint8>(dataStore, imageDims, k_ComponentDims, xMin, xMax, yMin, yMax, zMin, zMax, initValue));
      REQUIRE(DoesRangeEqualValue<uint8>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::uint16: {
      REQUIRE(DoesRangeEqualValue<uint16>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(DoesRangeEqualValue<uint16>(dataStore, imageDims, k_ComponentDims, xMin, xMax, yMin, yMax, zMin, zMax, initValue));
      REQUIRE(DoesRangeEqualValue<uint16>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::uint32: {
      REQUIRE(DoesRangeEqualValue<uint32>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(DoesRangeEqualValue<uint32>(dataStore, imageDims, k_ComponentDims, xMin, xMax, yMin, yMax, zMin, zMax, initValue));
      REQUIRE(DoesRangeEqualValue<uint32>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::uint64: {
      REQUIRE(DoesRangeEqualValue<uint64>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(DoesRangeEqualValue<uint64>(dataStore, imageDims, k_ComponentDims, xMin, xMax, yMin, yMax, zMin, zMax, initValue));
      REQUIRE(DoesRangeEqualValue<uint64>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::float32: {
      REQUIRE(DoesRangeEqualValue<float32>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(DoesRangeEqualValue<float32>(dataStore, imageDims, k_ComponentDims, xMin, xMax, yMin, yMax, zMin, zMax, initValue));
      REQUIRE(DoesRangeEqualValue<float32>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::float64: {
      REQUIRE(DoesRangeEqualValue<float64>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(DoesRangeEqualValue<float64>(dataStore, imageDims, k_ComponentDims, xMin, xMax, yMin, yMax, zMin, zMax, initValue));
      REQUIRE(DoesRangeEqualValue<float64>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    default: {
      throw std::runtime_error("Unhandled DataType");
    }
    }
  }
}

TEST_CASE("ComplexCore::InitializeData(Random)", "[ComplexCore][InitializeData]")
{
  InitializeData filter;
  DataStructure dataStructure = CreateDataStructure();

  constexpr uint64 xMin = 5;
  constexpr uint64 yMin = 5;
  constexpr uint64 zMin = 5;
  constexpr uint64 xMax = 15;
  constexpr uint64 yMax = 15;
  constexpr uint64 zMax = 15;
  const std::vector<DataPath> cellArrayPaths = {k_Int32ArrayPath, k_Float32ArrayPath};
  Arguments args = CreateArgs(cellArrayPaths, k_ImageGeomPath, xMin, yMin, zMin, xMax, yMax, zMax, InitializeData::InitType::Random, 0.0, {0.0, 0.0});

  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  std::array<usize, 3> imageDims = {k_ImageDims.at(0), k_ImageDims.at(1), k_ImageDims.at(2)};

  for(const auto& path : cellArrayPaths)
  {
    const auto& dataArray = dataStructure.getDataRefAs<IDataArray>(path);
    const auto& dataStore = dataArray.getIDataStoreRef();

    DataType type = dataStore.getDataType();

    // Check that the data outside the range is not changed
    // Since the data inside the range is random, we cannot check it

    switch(type)
    {
    case DataType::int8: {
      REQUIRE(DoesRangeEqualValue<int8>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(DoesRangeEqualValue<int8>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::int16: {
      REQUIRE(DoesRangeEqualValue<int16>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(DoesRangeEqualValue<int16>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::int32: {
      REQUIRE(DoesRangeEqualValue<int32>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(DoesRangeEqualValue<int32>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::int64: {
      REQUIRE(DoesRangeEqualValue<int64>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(DoesRangeEqualValue<int64>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::uint8: {
      REQUIRE(DoesRangeEqualValue<uint8>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(DoesRangeEqualValue<uint8>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::uint16: {
      REQUIRE(DoesRangeEqualValue<uint16>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(DoesRangeEqualValue<uint16>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::uint32: {
      REQUIRE(DoesRangeEqualValue<uint32>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(DoesRangeEqualValue<uint32>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::uint64: {
      REQUIRE(DoesRangeEqualValue<uint64>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(DoesRangeEqualValue<uint64>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::float32: {
      REQUIRE(DoesRangeEqualValue<float32>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(DoesRangeEqualValue<float32>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::float64: {
      REQUIRE(DoesRangeEqualValue<float64>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(DoesRangeEqualValue<float64>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    default: {
      throw std::runtime_error("Unhandled DataType");
    }
    }
  }
}

TEST_CASE("ComplexCore::InitializeData(RandomWithRange)", "[ComplexCore][InitializeData]")
{
  InitializeData filter;
  DataStructure dataStructure = CreateDataStructure();

  constexpr uint64 xMin = 5;
  constexpr uint64 yMin = 5;
  constexpr uint64 zMin = 5;
  constexpr uint64 xMax = 15;
  constexpr uint64 yMax = 15;
  constexpr uint64 zMax = 15;
  constexpr std::pair<float64, float64> initRange = {1.0, 25.0};
  const std::vector<DataPath> cellArrayPaths = {k_Int32ArrayPath, k_Float32ArrayPath};
  Arguments args = CreateArgs(cellArrayPaths, k_ImageGeomPath, xMin, yMin, zMin, xMax, yMax, zMax, InitializeData::InitType::RandomWithRange, 0.0, initRange);

  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  std::array<usize, 3> imageDims = {k_ImageDims.at(0), k_ImageDims.at(1), k_ImageDims.at(2)};

  for(const auto& path : cellArrayPaths)
  {
    const auto& dataArray = dataStructure.getDataRefAs<IDataArray>(path);
    const auto& dataStore = dataArray.getIDataStoreRef();

    DataType type = dataStore.getDataType();

    // Check that the data inside the range is within the given range and that the data outside it is unchanged

    switch(type)
    {
    case DataType::int8: {
      REQUIRE(DoesRangeEqualValue<int8>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(IsDataWithinInclusiveRange<int8>(dataStore, imageDims, k_ComponentDims, xMin, xMax, yMin, yMax, zMin, zMax, initRange));
      REQUIRE(DoesRangeEqualValue<int8>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::int16: {
      REQUIRE(DoesRangeEqualValue<int16>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(IsDataWithinInclusiveRange<int16>(dataStore, imageDims, k_ComponentDims, xMin, xMax, yMin, yMax, zMin, zMax, initRange));
      REQUIRE(DoesRangeEqualValue<int16>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::int32: {
      REQUIRE(DoesRangeEqualValue<int32>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(IsDataWithinInclusiveRange<int32>(dataStore, imageDims, k_ComponentDims, xMin, xMax, yMin, yMax, zMin, zMax, initRange));
      REQUIRE(DoesRangeEqualValue<int32>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::int64: {
      REQUIRE(DoesRangeEqualValue<int64>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(IsDataWithinInclusiveRange<int64>(dataStore, imageDims, k_ComponentDims, xMin, xMax, yMin, yMax, zMin, zMax, initRange));
      REQUIRE(DoesRangeEqualValue<int64>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::uint8: {
      REQUIRE(DoesRangeEqualValue<uint8>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(IsDataWithinInclusiveRange<uint8>(dataStore, imageDims, k_ComponentDims, xMin, xMax, yMin, yMax, zMin, zMax, initRange));
      REQUIRE(DoesRangeEqualValue<uint8>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::uint16: {
      REQUIRE(DoesRangeEqualValue<uint16>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(IsDataWithinInclusiveRange<uint16>(dataStore, imageDims, k_ComponentDims, xMin, xMax, yMin, yMax, zMin, zMax, initRange));
      REQUIRE(DoesRangeEqualValue<uint16>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::uint32: {
      REQUIRE(DoesRangeEqualValue<uint32>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(IsDataWithinInclusiveRange<uint32>(dataStore, imageDims, k_ComponentDims, xMin, xMax, yMin, yMax, zMin, zMax, initRange));
      REQUIRE(DoesRangeEqualValue<uint32>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::uint64: {
      REQUIRE(DoesRangeEqualValue<uint64>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(IsDataWithinInclusiveRange<uint64>(dataStore, imageDims, k_ComponentDims, xMin, xMax, yMin, yMax, zMin, zMax, initRange));
      REQUIRE(DoesRangeEqualValue<uint64>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::float32: {
      REQUIRE(DoesRangeEqualValue<float32>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(IsDataWithinInclusiveRange<float32>(dataStore, imageDims, k_ComponentDims, xMin, xMax, yMin, yMax, zMin, zMax, initRange));
      REQUIRE(DoesRangeEqualValue<float32>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    case DataType::float64: {
      REQUIRE(DoesRangeEqualValue<float64>(dataStore, imageDims, k_ComponentDims, 0, xMin - 1, 0, yMin - 1, 0, zMin - 1, 0.0));
      REQUIRE(IsDataWithinInclusiveRange<float64>(dataStore, imageDims, k_ComponentDims, xMin, xMax, yMin, yMax, zMin, zMax, initRange));
      REQUIRE(DoesRangeEqualValue<float64>(dataStore, imageDims, k_ComponentDims, xMax + 1, imageDims[0] - 1, yMax + 1, imageDims[1] - 1, zMax + 1, imageDims[2] - 1, 0.0));
      break;
    }
    default: {
      throw std::runtime_error("Unhandled DataType");
    }
    }
  }
}
