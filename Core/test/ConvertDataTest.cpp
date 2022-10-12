#include <memory>

#include <cassert>
#include <cstdint>
#include <vector>

#include "complex/Common/TypesUtility.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include "Core/Core_test_dirs.hpp"
#include "Core/Filters/Algorithms/ConvertData.hpp"
#include "Core/Filters/ConvertDataFilter.hpp"

using namespace complex;

const std::string AttributeMatrixName = "AttributeMatrix";
const std::string DataArrayName = "DataArray";
const DataPath AttributeMatrixPath({AttributeMatrixName});
const DataPath DataArrayPath = AttributeMatrixPath.createChildPath(DataArrayName);
const size_t TUPLE_DIM = 2;
const size_t COMPONENT_DIM = 2;

// -----------------------------------------------------------------------------
template <typename T>
void createDataStructure(DataStructure& ds)
{
  std::vector<size_t> tdims = {TUPLE_DIM};
  AttributeMatrix* am = AttributeMatrix::Create(ds, AttributeMatrixName);
  am->setShape(tdims);
  std::vector<size_t> cdims = {COMPONENT_DIM};
  DataArray<T>* da = DataArray<T>::template CreateWithStore<DataStore<T>>(ds, DataArrayName, tdims, cdims, am->getId());
  da->fill(static_cast<T>(0.0));
}

// -----------------------------------------------------------------------------
Arguments getArgs(const DataPath& inputArray, DataType type, const std::string& outputArrayName)
{
  Arguments args;

  args.insertOrAssign(ConvertDataFilter::k_ScalarType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<uint8>(type)));
  args.insertOrAssign(ConvertDataFilter::k_ArrayToConvert_Key, std::make_any<DataPath>(inputArray));
  args.insertOrAssign(ConvertDataFilter::k_ConvertedArray_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));

  return args;
}

// -----------------------------------------------------------------------------
template <typename T, typename U>
void checkConvertedArray(DataArray<T>* originalDataArray, DataArray<U>* convertedDataArray)
{
  REQUIRE(nullptr != originalDataArray);
  REQUIRE(nullptr != convertedDataArray);

  REQUIRE(originalDataArray->getNumberOfComponents() == convertedDataArray->getNumberOfComponents());

  size_t numComponents = originalDataArray->getNumberOfComponents();
  int componentDims = static_cast<int>(originalDataArray->getNumberOfTuples()) / numComponents;

  for(size_t i = 0; i < originalDataArray->getNumberOfTuples(); i++)
  {
    for(int j = 0; j < componentDims; j++)
    {
      auto index = i * numComponents + j;
      T value1 = (*originalDataArray)[index];
      U value2 = (*convertedDataArray)[index];
      if(static_cast<U>(value1) != value2)
      {
        std::cout << "orig: " << value1 << "  converted: " << value2 << std::endl;
      }
      REQUIRE(static_cast<U>(value1) == value2);
    }
  }
}

// -----------------------------------------------------------------------------
template <typename T, typename U>
void TestConversion(DataStructure& ds, ConvertDataFilter& filter, std::string arrayName, DataType newType, std::string newArrayName, bool checkArray = true)
{
  Arguments args = getArgs(DataArrayPath, newType, newArrayName);
  auto executeResults = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResults.result);

  if(checkArray)
  {
    AttributeMatrix* am = ds.getDataAs<AttributeMatrix>(AttributeMatrixPath);

    DataArray<T>* dataArray = ds.getDataAs<DataArray<T>>(DataArrayPath);
    DataArray<U>* convertedDataArray = ds.getDataAs<DataArray<U>>(AttributeMatrixPath.createChildPath(newArrayName));

    checkConvertedArray<T, U>(dataArray, convertedDataArray);
  }
}

// -----------------------------------------------------------------------------
void TestInt8Signed()
{
  ConvertDataFilter filter;
  DataStructure ds;
  createDataStructure<int8>(ds);

  TestConversion<int8, int8>(ds, filter, DataArrayName, DataType::int8, "NewArrayChar");
  TestConversion<int8, int16>(ds, filter, DataArrayName, DataType::int16, "NewArrayShort");
  TestConversion<int8, int32>(ds, filter, DataArrayName, DataType::int32, "NewArrayInt");
  TestConversion<int8, int64>(ds, filter, DataArrayName, DataType::int64, "NewArrayLong");

  TestConversion<int8, float32>(ds, filter, DataArrayName, DataType::float32, "NewArrayFloat");
  TestConversion<int8, float64>(ds, filter, DataArrayName, DataType::float64, "NewArrayDouble");
  TestConversion<int8, bool>(ds, filter, DataArrayName, DataType::boolean, "NewArrayBool");
}

// -----------------------------------------------------------------------------
void TestInt8Unsigned()
{
  ConvertDataFilter filter;
  DataStructure ds;
  createDataStructure<int8>(ds);

  TestConversion<int8, uint8>(ds, filter, DataArrayName, DataType::uint8, "NewArrayUChar");
  TestConversion<int8, uint16>(ds, filter, DataArrayName, DataType::uint16, "NewArrayUShort");
  TestConversion<int8, uint32>(ds, filter, DataArrayName, DataType::uint32, "NewArrayUInt");
  TestConversion<int8, uint64>(ds, filter, DataArrayName, DataType::uint64, "NewArrayULong");
  TestConversion<int8, bool>(ds, filter, DataArrayName, DataType::boolean, "NewArrayBool");
}

// -----------------------------------------------------------------------------
void TestUInt8Signed()
{
  ConvertDataFilter filter;
  DataStructure ds;
  createDataStructure<uint8>(ds);

  TestConversion<uint8, int8>(ds, filter, DataArrayName, DataType::int8, "NewArrayChar");
  TestConversion<uint8, int16>(ds, filter, DataArrayName, DataType::int16, "NewArrayShort");
  TestConversion<uint8, int32>(ds, filter, DataArrayName, DataType::int32, "NewArrayInt");
  TestConversion<uint8, int64>(ds, filter, DataArrayName, DataType::int64, "NewArrayLong");

  TestConversion<uint8, float32>(ds, filter, DataArrayName, DataType::float32, "NewArrayFloat");
  TestConversion<uint8, float64>(ds, filter, DataArrayName, DataType::float64, "NewArrayDouble");
  TestConversion<uint8, bool>(ds, filter, DataArrayName, DataType::boolean, "NewArrayBool");
}

// -----------------------------------------------------------------------------
void TestUInt8Unsigned()
{
  ConvertDataFilter filter;
  DataStructure ds;
  createDataStructure<uint8>(ds);

  TestConversion<uint8, uint8>(ds, filter, DataArrayName, DataType::uint8, "NewArrayUChar");
  TestConversion<uint8, uint16>(ds, filter, DataArrayName, DataType::uint16, "NewArrayUShort");
  TestConversion<uint8, uint32>(ds, filter, DataArrayName, DataType::uint32, "NewArrayUInt");
  TestConversion<uint8, uint64>(ds, filter, DataArrayName, DataType::uint64, "NewArrayULong");
  TestConversion<uint8, bool>(ds, filter, DataArrayName, DataType::boolean, "NewArrayBool");
}

// -----------------------------------------------------------------------------
void TestInt16Signed()
{
  ConvertDataFilter filter;
  DataStructure ds;
  createDataStructure<int16>(ds);

  TestConversion<int16, int8>(ds, filter, DataArrayName, DataType::int8, "NewArrayChar");
  TestConversion<int16, int16>(ds, filter, DataArrayName, DataType::int16, "NewArrayShort");
  TestConversion<int16, int32>(ds, filter, DataArrayName, DataType::int32, "NewArrayInt");
  TestConversion<int16, int64>(ds, filter, DataArrayName, DataType::int64, "NewArrayLong");

  TestConversion<int16, float>(ds, filter, DataArrayName, DataType::float32, "NewArrayFloat");
  TestConversion<int16, double>(ds, filter, DataArrayName, DataType::float64, "NewArrayDouble");
  TestConversion<int16, bool>(ds, filter, DataArrayName, DataType::boolean, "NewArrayBool");
}

// -----------------------------------------------------------------------------
void TestInt16Unsigned()
{
  ConvertDataFilter filter;
  DataStructure ds;
  createDataStructure<int16>(ds);

  TestConversion<int16, uint8>(ds, filter, DataArrayName, DataType::uint8, "NewArrayUChar");
  TestConversion<int16, uint16>(ds, filter, DataArrayName, DataType::uint16, "NewArrayUShort");
  TestConversion<int16, uint32>(ds, filter, DataArrayName, DataType::uint32, "NewArrayUInt");
  TestConversion<int16, uint64>(ds, filter, DataArrayName, DataType::uint64, "NewArrayULong");
  TestConversion<int16, bool>(ds, filter, DataArrayName, DataType::boolean, "NewArrayBool");
}

// -----------------------------------------------------------------------------
void TestUInt16Signed()
{
  ConvertDataFilter filter;
  DataStructure ds;
  createDataStructure<uint16>(ds);

  TestConversion<uint16, int8>(ds, filter, DataArrayName, DataType::int8, "NewArrayChar");
  TestConversion<uint16, int16>(ds, filter, DataArrayName, DataType::int16, "NewArrayShort");
  TestConversion<uint16, int32>(ds, filter, DataArrayName, DataType::int32, "NewArrayInt");
  TestConversion<uint16, int64>(ds, filter, DataArrayName, DataType::int64, "NewArrayLong");

  TestConversion<uint16, float32>(ds, filter, DataArrayName, DataType::float32, "NewArrayFloat");
  TestConversion<uint16, float64>(ds, filter, DataArrayName, DataType::float64, "NewArrayDouble");
  TestConversion<uint16, bool>(ds, filter, DataArrayName, DataType::boolean, "NewArrayBool");
}

// -----------------------------------------------------------------------------
void TestUInt16Unsigned()
{
  ConvertDataFilter filter;
  DataStructure ds;
  createDataStructure<uint16>(ds);

  TestConversion<uint16, uint8>(ds, filter, DataArrayName, DataType::uint8, "NewArrayUChar");
  TestConversion<uint16, uint16>(ds, filter, DataArrayName, DataType::uint16, "NewArrayUShort");
  TestConversion<uint16, uint32>(ds, filter, DataArrayName, DataType::uint32, "NewArrayUInt");
  TestConversion<uint16, uint64>(ds, filter, DataArrayName, DataType::uint64, "NewArrayULong");
  TestConversion<uint16, bool>(ds, filter, DataArrayName, DataType::boolean, "NewArrayBool");
}

// -----------------------------------------------------------------------------
void TestInt32Signed()
{
  ConvertDataFilter filter;
  DataStructure ds;
  createDataStructure<int32>(ds);

  TestConversion<int32, int8>(ds, filter, DataArrayName, DataType::int8, "NewArrayChar");
  TestConversion<int32, int16>(ds, filter, DataArrayName, DataType::int16, "NewArrayShort");
  TestConversion<int32, int32>(ds, filter, DataArrayName, DataType::int32, "NewArrayInt");
  TestConversion<int32, int64>(ds, filter, DataArrayName, DataType::int64, "NewArrayLong");

  TestConversion<int32, float32>(ds, filter, DataArrayName, DataType::float32, "NewArrayFloat");
  TestConversion<int32, float64>(ds, filter, DataArrayName, DataType::float64, "NewArrayDouble");
  TestConversion<int32, bool>(ds, filter, DataArrayName, DataType::boolean, "NewArrayBool");
}

// -----------------------------------------------------------------------------
void TestInt32Unsigned()
{
  ConvertDataFilter filter;
  DataStructure ds;
  createDataStructure<int32>(ds);

  TestConversion<int32, uint8>(ds, filter, DataArrayName, DataType::uint8, "NewArrayUChar");
  TestConversion<int32, uint16>(ds, filter, DataArrayName, DataType::uint16, "NewArrayUShort");
  TestConversion<int32, uint32>(ds, filter, DataArrayName, DataType::uint32, "NewArrayUInt");
  TestConversion<int32, uint64>(ds, filter, DataArrayName, DataType::uint64, "NewArrayULong");
  TestConversion<int32, bool>(ds, filter, DataArrayName, DataType::boolean, "NewArrayBool");
}

// -----------------------------------------------------------------------------
void TestUInt32Signed()
{
  ConvertDataFilter filter;
  DataStructure ds;
  createDataStructure<uint32>(ds);

  TestConversion<uint32, int8>(ds, filter, DataArrayName, DataType::int8, "NewArrayChar");
  TestConversion<uint32, int16>(ds, filter, DataArrayName, DataType::int16, "NewArrayShort");
  TestConversion<uint32, int32>(ds, filter, DataArrayName, DataType::int32, "NewArrayInt");
  TestConversion<uint32, int64>(ds, filter, DataArrayName, DataType::int64, "NewArrayLong");

  TestConversion<uint32, float32>(ds, filter, DataArrayName, DataType::float32, "NewArrayFloat");
  TestConversion<uint32, float64>(ds, filter, DataArrayName, DataType::float64, "NewArrayDouble");
  TestConversion<uint32, bool>(ds, filter, DataArrayName, DataType::boolean, "NewArrayBool");
}

// -----------------------------------------------------------------------------
void TestUInt32Unsigned()
{
  ConvertDataFilter filter;
  DataStructure ds;
  createDataStructure<uint32>(ds);

  TestConversion<uint32, uint8>(ds, filter, DataArrayName, DataType::uint8, "NewArrayUChar");
  TestConversion<uint32, uint16>(ds, filter, DataArrayName, DataType::uint16, "NewArrayUShort");
  TestConversion<uint32, uint32>(ds, filter, DataArrayName, DataType::uint32, "NewArrayUInt");
  TestConversion<uint32, uint64>(ds, filter, DataArrayName, DataType::uint64, "NewArrayULong");
  TestConversion<uint32, bool>(ds, filter, DataArrayName, DataType::boolean, "NewArrayBool");
}

// -----------------------------------------------------------------------------
void TestInt64Signed()
{
  ConvertDataFilter filter;
  DataStructure ds;
  createDataStructure<int64>(ds);

  TestConversion<int64, int8>(ds, filter, DataArrayName, DataType::int8, "NewArrayChar");
  TestConversion<int64, int16>(ds, filter, DataArrayName, DataType::int16, "NewArrayShort");
  TestConversion<int64, int32>(ds, filter, DataArrayName, DataType::int32, "NewArrayInt");
  TestConversion<int64, int64>(ds, filter, DataArrayName, DataType::int64, "NewArrayLong");

  TestConversion<int64, float32>(ds, filter, DataArrayName, DataType::float32, "NewArrayFloat");
  TestConversion<int64, float64>(ds, filter, DataArrayName, DataType::float64, "NewArrayDouble");
  TestConversion<int64, bool>(ds, filter, DataArrayName, DataType::boolean, "NewArrayBool");
}

// -----------------------------------------------------------------------------
void TestInt64Unsigned()
{
  ConvertDataFilter filter;
  DataStructure ds;
  createDataStructure<int64>(ds);

  TestConversion<int64, uint8>(ds, filter, DataArrayName, DataType::uint8, "NewArrayUChar");
  TestConversion<int64, uint16>(ds, filter, DataArrayName, DataType::uint16, "NewArrayUShort");
  TestConversion<int64, uint32>(ds, filter, DataArrayName, DataType::uint32, "NewArrayUInt");
  TestConversion<int64, uint64>(ds, filter, DataArrayName, DataType::uint64, "NewArrayULong");
  TestConversion<int64, bool>(ds, filter, DataArrayName, DataType::boolean, "NewArrayBool");
}

// -----------------------------------------------------------------------------
void TestUInt64Signed()
{
  ConvertDataFilter filter;
  DataStructure ds;
  createDataStructure<uint64>(ds);

  TestConversion<uint64, int8>(ds, filter, DataArrayName, DataType::int8, "NewArrayChar");
  TestConversion<uint64, int16>(ds, filter, DataArrayName, DataType::int16, "NewArrayShort");
  TestConversion<uint64, int32>(ds, filter, DataArrayName, DataType::int32, "NewArrayInt");
  TestConversion<uint64, int64>(ds, filter, DataArrayName, DataType::int64, "NewArrayLong");

  TestConversion<uint64, float32>(ds, filter, DataArrayName, DataType::float32, "NewArrayFloat");
  TestConversion<uint64, float64>(ds, filter, DataArrayName, DataType::float64, "NewArrayDouble");
  TestConversion<uint64, bool>(ds, filter, DataArrayName, DataType::boolean, "NewArrayBool");
}

// -----------------------------------------------------------------------------
void TestUInt64Unsigned()
{
  ConvertDataFilter filter;
  DataStructure ds;
  createDataStructure<uint64>(ds);

  TestConversion<uint64, uint8>(ds, filter, DataArrayName, DataType::uint8, "NewArrayUChar");
  TestConversion<uint64, uint16>(ds, filter, DataArrayName, DataType::uint16, "NewArrayUShort");
  TestConversion<uint64, uint32>(ds, filter, DataArrayName, DataType::uint32, "NewArrayUInt");
  TestConversion<uint64, uint64>(ds, filter, DataArrayName, DataType::uint64, "NewArrayULong");
  TestConversion<uint64, bool>(ds, filter, DataArrayName, DataType::boolean, "NewArrayBool");
}

// -----------------------------------------------------------------------------
void TestFloat()
{
  ConvertDataFilter filter;
  DataStructure ds;
  createDataStructure<float32>(ds);

  TestConversion<float32, int8>(ds, filter, DataArrayName, DataType::int8, "NewArrayChar");
  TestConversion<float32, int16>(ds, filter, DataArrayName, DataType::int16, "NewArrayShort");
  TestConversion<float32, int32>(ds, filter, DataArrayName, DataType::int32, "NewArrayInt");
  TestConversion<float32, int64>(ds, filter, DataArrayName, DataType::int64, "NewArrayLong");

  TestConversion<float32, float32>(ds, filter, DataArrayName, DataType::float32, "NewArrayFloat");
  TestConversion<float32, float64>(ds, filter, DataArrayName, DataType::float64, "NewArrayDouble");
  TestConversion<float, bool>(ds, filter, DataArrayName, DataType::boolean, "NewArrayBool");
}

// -----------------------------------------------------------------------------
void TestDouble()
{
  ConvertDataFilter filter;
  DataStructure ds;
  createDataStructure<float64>(ds);

  TestConversion<float64, int8>(ds, filter, DataArrayName, DataType::int8, "NewArrayChar");
  TestConversion<float64, int16>(ds, filter, DataArrayName, DataType::int16, "NewArrayShort");
  TestConversion<float64, int32>(ds, filter, DataArrayName, DataType::int32, "NewArrayInt");
  TestConversion<float64, int64>(ds, filter, DataArrayName, DataType::int64, "NewArrayLong");

  TestConversion<float64, float32>(ds, filter, DataArrayName, DataType::float32, "NewArrayFloat");
  TestConversion<float64, float64>(ds, filter, DataArrayName, DataType::float64, "NewArrayDouble");
  TestConversion<double, bool>(ds, filter, DataArrayName, DataType::boolean, "NewArrayBool");
}

// -----------------------------------------------------------------------------
void TestBoolSigned()
{
  ConvertDataFilter filter;
  DataStructure ds;
  createDataStructure<bool>(ds);

  TestConversion<bool, int8>(ds, filter, DataArrayName, DataType::int8, "NewArrayChar");
  TestConversion<bool, int16>(ds, filter, DataArrayName, DataType::int16, "NewArrayShort");
  TestConversion<bool, int32>(ds, filter, DataArrayName, DataType::int32, "NewArrayInt");
  TestConversion<bool, int64>(ds, filter, DataArrayName, DataType::int64, "NewArrayLong");

  TestConversion<bool, float32>(ds, filter, DataArrayName, DataType::float32, "NewArrayFloat");
  TestConversion<bool, float64>(ds, filter, DataArrayName, DataType::float64, "NewArrayDouble");
}

// -----------------------------------------------------------------------------
void TestBoolUnsigned()
{
  ConvertDataFilter filter;
  DataStructure ds;
  createDataStructure<bool>(ds);

  TestConversion<bool, uint8>(ds, filter, DataArrayName, DataType::uint8, "NewArrayUChar");
  TestConversion<bool, uint16>(ds, filter, DataArrayName, DataType::uint16, "NewArrayUShort");
  TestConversion<bool, uint32>(ds, filter, DataArrayName, DataType::uint32, "NewArrayUInt");
  TestConversion<bool, uint64>(ds, filter, DataArrayName, DataType::uint64, "NewArrayULong");
}

// -----------------------------------------------------------------------------
void TestInvalidDataArray()
{
  ConvertDataFilter filter;
  DataStructure ds;
  createDataStructure<int8>(ds);

  Arguments args = getArgs(AttributeMatrixPath.createChildPath("Array1"), DataType::int8, "NewArray");
  auto executeResults = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_INVALID(executeResults.result);
}

// -----------------------------------------------------------------------------
void TestOverwriteArray()
{
  ConvertDataFilter filter;
  DataStructure ds;
  createDataStructure<int8>(ds);

  Arguments args = getArgs(DataArrayPath, DataType::int8, "DataArray");
  auto executeResults = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_INVALID(executeResults.result);
}

// -----------------------------------------------------------------------------
TEST_CASE("Core::ConvertData: Instantiation and Parameter Check", "[Core][ConvertDataFilter]")
{
  std::cout << "#### ConvertDataTest Starting ####" << std::endl;

  TestInt8Signed();
  TestInt8Unsigned();

  TestInt16Signed();
  TestInt16Unsigned();

  TestInt32Signed();
  TestInt32Unsigned();

  TestInt64Signed();
  TestInt64Unsigned();

  TestUInt8Signed();
  TestUInt8Unsigned();

  TestUInt16Signed();
  TestUInt16Unsigned();

  TestUInt32Signed();
  TestUInt32Unsigned();

  TestUInt64Signed();
  TestUInt64Unsigned();

  TestFloat();
  TestDouble();

  TestBoolSigned();
  TestBoolUnsigned();

  TestInvalidDataArray();
  TestOverwriteArray();
}