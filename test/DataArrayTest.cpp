#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Common/Array.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"

#include <catch2/catch.hpp>

#include <cmath>
#include <vector>

using namespace nx::core;

namespace
{
constexpr StringLiteral k_BuildDir = SIMPLNX_BUILD_DIR;
}

TEST_CASE("Array")
{
  FloatVec3 vec0(3.0F, 4.0F, 5.0F);

  IntVec3 v0(1, 2, 3);
  IntVec3 v1(0, 0, 0);
  v1.setValues(1, 2, 3);
  REQUIRE(v1[0] == 1);
  REQUIRE(v1[1] == 2);
  REQUIRE(v1[2] == 3);

  auto v1Tuple = v1.toTuple();
  FloatVec3 v1F32 = v1.convertType<float>();

  FloatVec3 v2(-3.0F, -4.0F, -5.0F);
  FloatVec3 cross = vec0.cross(v2);
  REQUIRE(cross[0] == 0);
  REQUIRE(cross[1] == 0);
  REQUIRE(cross[2] == 0);

  float32 dot = v2.dot(vec0);
  REQUIRE(dot == -50.0F);

  float32 mag = vec0.magnitude();
  REQUIRE(mag == std::sqrt(50.0F));
}

TEST_CASE("DataArrayCreation")
{
  nx::core::DataStructure dataStructure;

  using DataStoreType = nx::core::DataStore<int32_t>;
  DataStoreType data_array = DataStoreType(nx::core::ShapeType{0}, nx::core::ShapeType{2}, 0);
  size_t numTuples = data_array.getNumberOfTuples();
  REQUIRE(numTuples == 0);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("nx::core::DataArray Copy TupleTest", "[simplnx][DataArray]")
{
  UnitTest::LoadPlugins();

  const std::string k_DataArrayName("DataArray");
  const DataPath k_DataPath({k_DataArrayName});
  const usize k_NumTuples = 5;
  const usize k_NumComponents = 3;

  DataStructure dataStructure;
  ShapeType tupleShape{k_NumTuples};
  ShapeType componentShape{k_NumComponents};
  Result<> result = ArrayCreationUtilities::CreateArray<int32>(dataStructure, tupleShape, componentShape, k_DataPath, IDataAction::Mode::Execute);
  REQUIRE(result.valid() == true);

  auto& dataArray = dataStructure.getDataRefAs<DataArray<int32>>(k_DataPath);

  for(usize i = 0; i < k_NumTuples; i++)
  {
    dataArray.initializeTuple(i, static_cast<int32>(i));
  }

  for(usize tupleIndex = 0; tupleIndex < k_NumTuples; tupleIndex++)
  {
    for(usize componentIndex = 0; componentIndex < k_NumComponents; componentIndex++)
    {
      uint64 index = tupleIndex * 3 + componentIndex;
      REQUIRE(dataArray[index] == static_cast<int32>(tupleIndex));
    }
  }

  dataArray.copyTuple(4, 0);
  REQUIRE(dataArray[0] == 4);
  REQUIRE(dataArray[1] == 4);
  REQUIRE(dataArray[2] == 4);

  dataArray.copyTuple(1, 4);
  REQUIRE(dataArray[12] == 1);
  REQUIRE(dataArray[13] == 1);
  REQUIRE(dataArray[14] == 1);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("DataStore Test")
{
  ShapeType tupleShape{5};
  ShapeType componentShape{3};
  DataStore<int32> dataStore(tupleShape, componentShape, 5);

  REQUIRE(dataStore.getSize() == 15);

  for(uint64_t i = 0; i < dataStore.getSize(); i++)
  {
    REQUIRE(dataStore[i] == 5);
  }

  int32 newArrayValues[] = {6, 7, 8};
  dataStore.setTuple(0, newArrayValues);
  for(uint64 i = 0; i < 3; i++)
  {
    REQUIRE(dataStore[i] == newArrayValues[i]);
    REQUIRE(dataStore.getComponentValue(0, i) == newArrayValues[i]);
  }

  std::vector<int32> newValues{1, 2, 3};
  dataStore.setTuple(1, newValues);
  usize offset = dataStore.getNumberOfComponents();
  for(usize i = 0; i < newValues.size(); i++)
  {
    REQUIRE(dataStore[offset + i] == newValues[i]);
    REQUIRE(dataStore.getComponentValue(1, i) == newValues[i]);
  }

  dataStore.setComponent(2, 2, 99);
  REQUIRE(dataStore[8] == 99);
  REQUIRE(dataStore.getComponentValue(2, 2) == 99);
}

TEST_CASE("Copy DataStore", "DataArray")
{
  ShapeType tupleShape{5};
  ShapeType componentShape{3};
  DataStore<int32> dataStore(tupleShape, componentShape, 5);
  usize size = dataStore.getSize();
  for(usize i = 0; i < size; i++)
  {
    dataStore[i] = i;
  }

  DataStore<int32> dataStore2(tupleShape, componentShape, 5);
  dataStore2.copy(dataStore);

  for(usize i = 0; i < size; i++)
  {
    REQUIRE(dataStore[i] == dataStore2[i]);
  }
}

template <typename T>
void TestDataArrayToFromString(DataStructure& datastructure, const std::string& arrayName, const std::string& minCompareStr, const std::string& maxCompareStr)
{
  constexpr usize index1 = 5;
  constexpr usize index2 = 6;
  constexpr usize index3 = 7;
  constexpr usize index4 = 8;
  constexpr usize index5 = 9;
  constexpr usize index6 = 10;
  const ShapeType tupleShape = {5, 4, 3};
  const ShapeType compShape = {1};
  auto* dataArray = UnitTest::CreateTestDataArray<T>(datastructure, arrayName, tupleShape, compShape);
  constexpr T minVal = std::numeric_limits<T>::min();
  constexpr T maxVal = std::numeric_limits<T>::max();
  dataArray->setValue(index2, minVal);
  dataArray->setValue(index3, maxVal);
  const std::string minStr = dataArray->toString(index2, 0);
  const std::string maxStr = dataArray->toString(index3, 0);
  // std::cout << "DataArray<T>\n\tmin = " << minStr << "\n\tmax = " << maxStr << "\n";
  REQUIRE(minStr == minCompareStr);
  REQUIRE(maxStr == maxCompareStr);
  REQUIRE(dataArray->setValueFromString(index5, 0, minStr));
  REQUIRE(dataArray->setValueFromString(index6, 0, maxStr));
  // std::cout << "DataArray<T>\n\tmin : " << dataArray->getValue(index5) << " == " << minVal << "\n\tmax : " << dataArray->getValue(index6) << " == " << maxVal << "\n";
  REQUIRE(dataArray->getValue(index5) == minVal);
  REQUIRE(dataArray->getValue(index6) == maxVal);
}

template <>
void TestDataArrayToFromString<float32>(DataStructure& datastructure, const std::string& arrayName, const std::string& minCompareStr, const std::string& maxCompareStr)
{
  constexpr usize index1 = 5;
  constexpr usize index2 = 6;
  constexpr usize index3 = 7;
  constexpr usize index4 = 8;
  constexpr usize index5 = 9;
  constexpr usize index6 = 10;
  constexpr usize index7 = 11;
  constexpr usize index8 = 12;
  constexpr usize index9 = 13;
  constexpr usize index10 = 14;
  const ShapeType tupleShape = {5, 4, 3};
  const ShapeType compShape = {1};
  auto* dataArray = UnitTest::CreateTestDataArray<float32>(datastructure, arrayName, tupleShape, compShape);
  constexpr float32 lowestVal = std::numeric_limits<float32>::lowest();
  constexpr float32 minVal = std::numeric_limits<float32>::min();
  constexpr float32 maxVal = std::numeric_limits<float32>::max();
  constexpr float32 infVal = std::numeric_limits<float32>::infinity();
  const float32 nanVal = std::nanf("1");
  dataArray->setValue(index1, lowestVal);
  dataArray->setValue(index2, minVal);
  dataArray->setValue(index3, maxVal);
  dataArray->setValue(index4, infVal);
  dataArray->setValue(index5, nanVal);
  const std::string lowestStr = dataArray->toString(index1, 0);
  const std::string minStr = dataArray->toString(index2, 0);
  const std::string maxStr = dataArray->toString(index3, 0);
  const std::string infStr = dataArray->toString(index4, 0);
  const std::string nanStr = dataArray->toString(index5, 0);
  // std::cout << "DataArray<float32>\n\tlowest = " << lowestStr << "\n\tmin = " << minStr << "\n\tmax = " << maxStr << "\n\tinf = " << infStr << "\n\tnan = " << nanStr << "\n";
  REQUIRE(lowestStr == "-3.4028235e+38");
  REQUIRE(minStr == minCompareStr);
  REQUIRE(maxStr == maxCompareStr);
  REQUIRE(infStr == "inf");
  REQUIRE(nanStr == "nan");
  REQUIRE(dataArray->toString(index2, 0, "{:.8g}") == "1.1754944e-38");
  REQUIRE(dataArray->toString(index3, 0, "{:.6f}") == "340282346638528859811704183484516925440.000000");
  REQUIRE(dataArray->toString(index2, 0, "{:.4e}") == "1.1755e-38");
  REQUIRE(dataArray->setValueFromString(index6, 0, lowestStr));
  REQUIRE(dataArray->setValueFromString(index7, 0, minStr));
  REQUIRE(dataArray->setValueFromString(index8, 0, maxStr));
  REQUIRE(dataArray->setValueFromString(index9, 0, infStr));
  REQUIRE(dataArray->setValueFromString(index10, 0, nanStr));
  // std::cout << "DataArray<float32>\n\tlowest : " << dataArray->getValue(index6) << " == " << lowestVal << "\n\tmin : " << dataArray->getValue(index7) << " == " << minVal
  //<< "\n\tmax : " << dataArray->getValue(index8) << " == " << maxVal << "\n\tinf : " << dataArray->getValue(index9) << " == " << infVal << "\n\tnan : " << dataArray->getValue(index10)
  //<< " == " << nanVal << "\n";
  REQUIRE(dataArray->getValue(index6) == lowestVal);
  REQUIRE(dataArray->getValue(index7) == minVal);
  REQUIRE(dataArray->getValue(index8) == maxVal);
  REQUIRE(dataArray->getValue(index9) == infVal);
  REQUIRE(std::isnan(dataArray->getValue(index10)));
}
template <>
void TestDataArrayToFromString<float64>(DataStructure& datastructure, const std::string& arrayName, const std::string& minCompareStr, const std::string& maxCompareStr)
{
  constexpr usize index1 = 5;
  constexpr usize index2 = 6;
  constexpr usize index3 = 7;
  constexpr usize index4 = 8;
  constexpr usize index5 = 9;
  constexpr usize index6 = 10;
  constexpr usize index7 = 11;
  constexpr usize index8 = 12;
  constexpr usize index9 = 13;
  constexpr usize index10 = 14;
  const ShapeType tupleShape = {5, 4, 3};
  const ShapeType compShape = {1};
  auto* dataArray = UnitTest::CreateTestDataArray<float64>(datastructure, arrayName, tupleShape, compShape);
  constexpr float64 lowestVal = std::numeric_limits<float64>::lowest();
  constexpr float64 minVal = std::numeric_limits<float64>::min();
  constexpr float64 maxVal = std::numeric_limits<float64>::max();
  constexpr float64 infVal = std::numeric_limits<float64>::infinity();
  const float64 nanVal = std::nan("1");
  dataArray->setValue(index1, lowestVal);
  dataArray->setValue(index2, minVal);
  dataArray->setValue(index3, maxVal);
  dataArray->setValue(index4, infVal);
  dataArray->setValue(index5, nanVal);
  const std::string lowestStr = dataArray->toString(index1, 0);
  const std::string minStr = dataArray->toString(index2, 0);
  const std::string maxStr = dataArray->toString(index3, 0);
  const std::string infStr = dataArray->toString(index4, 0);
  const std::string nanStr = dataArray->toString(index5, 0);
  // std::cout << "DataArray<float64>\n\tlowest = " << lowestStr << "\n\tmin = " << minStr << "\n\tmax = " << maxStr << "\n\tinf = " << infStr << "\n\tnan = " << nanStr << "\n";
  REQUIRE(lowestStr == "-1.7976931348623157e+308");
  REQUIRE(minStr == minCompareStr);
  REQUIRE(maxStr == maxCompareStr);
  REQUIRE(infStr == "inf");
  REQUIRE(nanStr == "nan");
  REQUIRE(dataArray->toString(index2, 0, "{:.8g}") == "2.2250739e-308");
  REQUIRE(dataArray->toString(index2, 0, "{:.6f}") == "0.000000");
  REQUIRE(dataArray->toString(index2, 0, "{:.4e}") == "2.2251e-308");
  REQUIRE(dataArray->setValueFromString(index6, 0, lowestStr));
  REQUIRE(dataArray->setValueFromString(index7, 0, minStr));
  REQUIRE(dataArray->setValueFromString(index8, 0, maxStr));
  REQUIRE(dataArray->setValueFromString(index9, 0, infStr));
  REQUIRE(dataArray->setValueFromString(index10, 0, nanStr));
  // std::cout << "DataArray<float64>\n\tlowest : " << dataArray->getValue(index6) << " == " << lowestVal << "\n\tmin : " << dataArray->getValue(index7) << " == " << minVal
  //<< "\n\tmax : " << dataArray->getValue(index8) << " == " << maxVal << "\n\tinf : " << dataArray->getValue(index9) << " == " << infVal << "\n\tnan : " << dataArray->getValue(index10)
  //<< " == " << nanVal << "\n";
  REQUIRE(dataArray->getValue(index6) == lowestVal);
  REQUIRE(dataArray->getValue(index7) == minVal);
  REQUIRE(dataArray->getValue(index8) == maxVal);
  REQUIRE(dataArray->getValue(index9) == infVal);
  REQUIRE(std::isnan(dataArray->getValue(index10)));
}

template <typename T>
void TestNeighborListArrayToFromString(DataStructure& datastructure, const std::string& arrayName, const std::string& minCompareStr, const std::string& maxCompareStr)
{
  constexpr usize index1 = 5;
  constexpr usize index2 = 6;
  constexpr usize index3 = 7;
  constexpr usize index4 = 8;
  constexpr usize index5 = 9;
  constexpr usize index6 = 10;
  const ShapeType tupleShape = {5, 4, 3};
  const ShapeType compShape = {1};
  auto* neighborListArray = UnitTest::CreateTestNeighborList<T>(datastructure, arrayName, 10, {});
  constexpr T minVal = std::numeric_limits<T>::min();
  constexpr T maxVal = std::numeric_limits<T>::max();
  neighborListArray->addEntry(index2, minVal);
  neighborListArray->addEntry(index3, maxVal);
  neighborListArray->addEntry(index5, 0);
  neighborListArray->addEntry(index6, 0);
  const std::string minStr = neighborListArray->toString(index2, 0);
  const std::string maxStr = neighborListArray->toString(index3, 0);
  // std::cout << "NeighborListArray<T>\n\tmin = " << minStr << "\n\tmax = " << maxStr << "\n";
  REQUIRE(minStr == minCompareStr);
  REQUIRE(maxStr == maxCompareStr);
  REQUIRE(neighborListArray->setValueFromString(index5, 0, minStr));
  REQUIRE(neighborListArray->setValueFromString(index6, 0, maxStr));
  bool ok;
  // std::cout << "NeighborListArray<T>\n\tmin : " << neighborListArray->getValue(index5, 0, ok) << " == " << minVal << "\n\tmax : " << neighborListArray->getValue(index6, 0, ok) << " == " << maxVal
  //<< "\n";
  REQUIRE(neighborListArray->getValue(index5, 0, ok) == minVal);
  REQUIRE(ok);
  REQUIRE(neighborListArray->getValue(index6, 0, ok) == maxVal);
  REQUIRE(ok);
}

template <>
void TestNeighborListArrayToFromString<float32>(DataStructure& datastructure, const std::string& arrayName, const std::string& minCompareStr, const std::string& maxCompareStr)
{
  constexpr usize index1 = 5;
  constexpr usize index2 = 6;
  constexpr usize index3 = 7;
  constexpr usize index4 = 8;
  constexpr usize index5 = 9;
  constexpr usize index6 = 10;
  constexpr usize index7 = 11;
  constexpr usize index8 = 12;
  constexpr usize index9 = 13;
  constexpr usize index10 = 14;
  const ShapeType tupleShape = {5, 4, 3};
  const ShapeType compShape = {1};
  auto* neighborListFloat32 = UnitTest::CreateTestNeighborList<float32>(datastructure, arrayName, 10, {});
  constexpr float32 lowestVal = std::numeric_limits<float32>::lowest();
  constexpr float32 minVal = std::numeric_limits<float32>::min();
  constexpr float32 maxVal = std::numeric_limits<float32>::max();
  constexpr float32 infVal = std::numeric_limits<float32>::infinity();
  const float32 nanVal = std::nanf("1");
  neighborListFloat32->addEntry(index1, lowestVal);
  neighborListFloat32->addEntry(index2, minVal);
  neighborListFloat32->addEntry(index3, maxVal);
  neighborListFloat32->addEntry(index4, infVal);
  neighborListFloat32->addEntry(index5, nanVal);
  neighborListFloat32->addEntry(index6, 0);
  neighborListFloat32->addEntry(index7, 0);
  neighborListFloat32->addEntry(index8, 0);
  neighborListFloat32->addEntry(index9, 0);
  neighborListFloat32->addEntry(index10, 0);
  const std::string lowestStr = neighborListFloat32->toString(index1, 0);
  const std::string minStr = neighborListFloat32->toString(index2, 0);
  const std::string maxStr = neighborListFloat32->toString(index3, 0);
  const std::string infStr = neighborListFloat32->toString(index4, 0);
  const std::string nanStr = neighborListFloat32->toString(index5, 0);
  // std::cout << "NeighborListArray<float32>\n\tlowest = " << lowestStr << "\n\tmin = " << minStr << "\n\tmax = " << maxStr << "\n\tinf = " << infStr << "\n\tnan = " << nanStr << "\n";
  REQUIRE(lowestStr == "-3.4028235e+38");
  REQUIRE(minStr == minCompareStr);
  REQUIRE(maxStr == maxCompareStr);
  REQUIRE(infStr == "inf");
  REQUIRE(nanStr == "nan");
  REQUIRE(neighborListFloat32->toString(index2, 0, "{:.8g}") == "1.1754944e-38");
  REQUIRE(neighborListFloat32->toString(index3, 0, "{:.6f}") == "340282346638528859811704183484516925440.000000");
  REQUIRE(neighborListFloat32->toString(index2, 0, "{:.4e}") == "1.1755e-38");
  REQUIRE(neighborListFloat32->setValueFromString(index6, 0, lowestStr));
  REQUIRE(neighborListFloat32->setValueFromString(index7, 0, minStr));
  REQUIRE(neighborListFloat32->setValueFromString(index8, 0, maxStr));
  REQUIRE(neighborListFloat32->setValueFromString(index9, 0, infStr));
  REQUIRE(neighborListFloat32->setValueFromString(index10, 0, nanStr));
  // std::cout << "NeighborListArray<float32>\n\tlowest : " << neighborListFloat32->getValue(index6) << " == " << lowestVal << "\n\tmin : " << neighborListFloat32->getValue(index7) << " == " << minVal
  //<< "\n\tmax : " << neighborListFloat32->getValue(index8) << " == " << maxVal << "\n\tinf : " << neighborListFloat32->getValue(index9) << " == " << infVal << "\n\tnan : " <<
  // neighborListFloat32->getValue(index10) << " == " << nanVal << "\n";
  bool ok;
  REQUIRE(neighborListFloat32->getValue(index6, 0, ok) == lowestVal);
  REQUIRE(ok);
  REQUIRE(neighborListFloat32->getValue(index7, 0, ok) == minVal);
  REQUIRE(ok);
  REQUIRE(neighborListFloat32->getValue(index8, 0, ok) == maxVal);
  REQUIRE(ok);
  REQUIRE(neighborListFloat32->getValue(index9, 0, ok) == infVal);
  REQUIRE(ok);
  REQUIRE(std::isnan(neighborListFloat32->getValue(index10, 0, ok)));
  REQUIRE(ok);
}
template <>
void TestNeighborListArrayToFromString<float64>(DataStructure& datastructure, const std::string& arrayName, const std::string& minCompareStr, const std::string& maxCompareStr)
{
  constexpr usize index1 = 5;
  constexpr usize index2 = 6;
  constexpr usize index3 = 7;
  constexpr usize index4 = 8;
  constexpr usize index5 = 9;
  constexpr usize index6 = 10;
  constexpr usize index7 = 11;
  constexpr usize index8 = 12;
  constexpr usize index9 = 13;
  constexpr usize index10 = 14;
  const ShapeType tupleShape = {5, 4, 3};
  const ShapeType compShape = {1};
  auto* neighborListFloat64 = UnitTest::CreateTestNeighborList<float64>(datastructure, arrayName, 10, {});
  constexpr float64 lowestVal = std::numeric_limits<float64>::lowest();
  constexpr float64 minVal = std::numeric_limits<float64>::min();
  constexpr float64 maxVal = std::numeric_limits<float64>::max();
  constexpr float64 infVal = std::numeric_limits<float64>::infinity();
  const float64 nanVal = std::nan("1");
  neighborListFloat64->addEntry(index1, lowestVal);
  neighborListFloat64->addEntry(index2, minVal);
  neighborListFloat64->addEntry(index3, maxVal);
  neighborListFloat64->addEntry(index4, infVal);
  neighborListFloat64->addEntry(index5, nanVal);
  neighborListFloat64->addEntry(index6, 0);
  neighborListFloat64->addEntry(index7, 0);
  neighborListFloat64->addEntry(index8, 0);
  neighborListFloat64->addEntry(index9, 0);
  neighborListFloat64->addEntry(index10, 0);
  const std::string lowestStr = neighborListFloat64->toString(index1, 0);
  const std::string minStr = neighborListFloat64->toString(index2, 0);
  const std::string maxStr = neighborListFloat64->toString(index3, 0);
  const std::string infStr = neighborListFloat64->toString(index4, 0);
  const std::string nanStr = neighborListFloat64->toString(index5, 0);
  // std::cout << "NeighborListArray<float64>\n\tlowest = " << lowestStr << "\n\tmin = " << minStr << "\n\tmax = " << maxStr << "\n\tinf = " << infStr << "\n\tnan = " << nanStr << "\n";
  REQUIRE(lowestStr == "-1.7976931348623157e+308");
  REQUIRE(minStr == minCompareStr);
  REQUIRE(maxStr == maxCompareStr);
  REQUIRE(infStr == "inf");
  REQUIRE(nanStr == "nan");
  REQUIRE(neighborListFloat64->toString(index2, 0, "{:.8g}") == "2.2250739e-308");
  REQUIRE(neighborListFloat64->toString(index2, 0, "{:.6f}") == "0.000000");
  REQUIRE(neighborListFloat64->toString(index2, 0, "{:.4e}") == "2.2251e-308");
  REQUIRE(neighborListFloat64->setValueFromString(index6, 0, lowestStr));
  REQUIRE(neighborListFloat64->setValueFromString(index7, 0, minStr));
  REQUIRE(neighborListFloat64->setValueFromString(index8, 0, maxStr));
  REQUIRE(neighborListFloat64->setValueFromString(index9, 0, infStr));
  REQUIRE(neighborListFloat64->setValueFromString(index10, 0, nanStr));
  // std::cout << "NeighborListArray<float64>\n\tlowest : " << neighborListFloat64->getValue(index6) << " == " << lowestVal << "\n\tmin : " << neighborListFloat64->getValue(index7) << " == " << minVal
  //<< "\n\tmax : " << neighborListFloat64->getValue(index8) << " == " << maxVal << "\n\tinf : " << neighborListFloat64->getValue(index9) << " == " << infVal << "\n\tnan : " <<
  // neighborListFloat64->getValue(index10) << " == " << nanVal << "\n";
  bool ok;
  REQUIRE(neighborListFloat64->getValue(index6, 0, ok) == lowestVal);
  REQUIRE(ok);
  REQUIRE(neighborListFloat64->getValue(index7, 0, ok) == minVal);
  REQUIRE(ok);
  REQUIRE(neighborListFloat64->getValue(index8, 0, ok) == maxVal);
  REQUIRE(ok);
  REQUIRE(neighborListFloat64->getValue(index9, 0, ok) == infVal);
  REQUIRE(ok);
  REQUIRE(std::isnan(neighborListFloat64->getValue(index10, 0, ok)));
  REQUIRE(ok);
}

TEST_CASE("IArray ToFromString")
{
  DataStructure datastructure;

  SECTION("DataArray<int8>")
  {
    TestDataArrayToFromString<int8>(datastructure, "Int8Array", "-128", "127");
  }
  SECTION("DataArray<uint8>")
  {
    TestDataArrayToFromString<uint8>(datastructure, "UInt8Array", "0", "255");
  }
  SECTION("DataArray<int16>")
  {
    TestDataArrayToFromString<int16>(datastructure, "Int16Array", "-32768", "32767");
  }
  SECTION("DataArray<uint16>")
  {
    TestDataArrayToFromString<uint16>(datastructure, "UInt16Array", "0", "65535");
  }
  SECTION("DataArray<int32>")
  {
    TestDataArrayToFromString<int32>(datastructure, "Int32Array", "-2147483648", "2147483647");
  }
  SECTION("DataArray<uint32>")
  {
    TestDataArrayToFromString<uint32>(datastructure, "UInt32Array", "0", "4294967295");
  }
  SECTION("DataArray<int64>")
  {
    TestDataArrayToFromString<int64>(datastructure, "Int64Array", "-9223372036854775808", "9223372036854775807");
  }
  SECTION("DataArray<uint64>")
  {
    TestDataArrayToFromString<uint64>(datastructure, "UInt64Array", "0", "18446744073709551615");
  }
  SECTION("DataArray<float32>")
  {
    TestDataArrayToFromString<float32>(datastructure, "Float32Array", "1.1754944e-38", "3.4028235e+38");
  }
  SECTION("DataArray<float64>")
  {
    TestDataArrayToFromString<float64>(datastructure, "Float64Array", "2.2250738585072014e-308", "1.7976931348623157e+308");
  }
  SECTION("DataArray<bool>")
  {
    TestDataArrayToFromString<bool>(datastructure, "BoolArray", "false", "true");
  }

  SECTION("NeighborList<int8>")
  {
    TestNeighborListArrayToFromString<int8>(datastructure, "Int8NeighborListArray", "-128", "127");
  }
  SECTION("NeighborList<uint8>")
  {
    TestNeighborListArrayToFromString<uint8>(datastructure, "UInt8NeighborListArray", "0", "255");
  }
  SECTION("NeighborList<int16>")
  {
    TestNeighborListArrayToFromString<int16>(datastructure, "Int16NeighborListArray", "-32768", "32767");
  }
  SECTION("NeighborList<uint16>")
  {
    TestNeighborListArrayToFromString<uint16>(datastructure, "UInt16NeighborListArray", "0", "65535");
  }
  SECTION("NeighborList<int32>")
  {
    TestNeighborListArrayToFromString<int32>(datastructure, "Int32NeighborListArray", "-2147483648", "2147483647");
  }
  SECTION("NeighborList<uint32>")
  {
    TestNeighborListArrayToFromString<uint32>(datastructure, "UInt32NeighborListArray", "0", "4294967295");
  }
  SECTION("NeighborList<int64>")
  {
    TestNeighborListArrayToFromString<int64>(datastructure, "Int64NeighborListArray", "-9223372036854775808", "9223372036854775807");
  }
  SECTION("NeighborList<uint64>")
  {
    TestNeighborListArrayToFromString<uint64>(datastructure, "UInt64NeighborListArray", "0", "18446744073709551615");
  }
  SECTION("NeighborList<float32>")
  {
    TestNeighborListArrayToFromString<float32>(datastructure, "Float32NeighborListArray", "1.1754944e-38", "3.4028235e+38");
  }
  SECTION("NeighborList<float64>")
  {
    TestNeighborListArrayToFromString<float64>(datastructure, "Float64NeighborListArray", "2.2250738585072014e-308", "1.7976931348623157e+308");
  }

  SECTION("StringArray")
  {
    constexpr usize index1 = 5;
    constexpr usize index2 = 6;
    constexpr usize index3 = 7;
    constexpr usize index4 = 8;
    constexpr usize index5 = 9;
    constexpr usize index6 = 10;
    const ShapeType tupleShape = {5, 4, 3};
    std::vector<std::string> values(60, "This is a string");
    StringArray* stringArray = StringArray::CreateWithValues(datastructure, "StringArray", tupleShape, values, {});
    REQUIRE(stringArray->setValueFromString(index1, 0, "AllLetters"));
    REQUIRE(stringArray->setValueFromString(index2, 0, "MixedLetters&Numbers"));
    REQUIRE(stringArray->setValueFromString(index3, 0, "234523"));
    REQUIRE(stringArray->setValueFromString(index4, 0, "45.78"));
    REQUIRE(stringArray->setValueFromString(index5, 0, "inf"));
    REQUIRE(stringArray->setValueFromString(index6, 0, "nan"));
    REQUIRE(stringArray->toString(index1, 0) == "AllLetters");
    REQUIRE(stringArray->toString(index2, 0) == "MixedLetters&Numbers");
    REQUIRE(stringArray->toString(index3, 0) == "234523");
    REQUIRE(stringArray->toString(index4, 0) == "45.78");
    REQUIRE(stringArray->toString(index5, 0) == "inf");
    REQUIRE(stringArray->toString(index6, 0) == "nan");
  }
}