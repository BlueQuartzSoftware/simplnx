#include "SimplnxCore/Filters/Algorithms/ConvertData.hpp"
#include "SimplnxCore/Filters/ConvertDataFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>
#include <vector>

using namespace nx::core;
namespace fs = std::filesystem;

const std::string k_AttributeMatrixName = "AttributeMatrix";
const std::string k_DataArrayName = "DataArray";
const DataPath AttributeMatrixPath({k_AttributeMatrixName});
const DataPath k_DataArrayPath = AttributeMatrixPath.createChildPath(k_DataArrayName);
const size_t k_TupleDim = 2;
const size_t k_ComponentDim = 2;

template <typename T>
static void CreateDataStructure(DataStructure& dataStructure)
{
  const std::vector<size_t> tdims = {k_TupleDim};
  AttributeMatrix* amPtr = AttributeMatrix::Create(dataStructure, k_AttributeMatrixName, tdims);
  std::vector<size_t> cdims = {k_ComponentDim};
  DataArray<T>* daPtr = DataArray<T>::template CreateWithStore<DataStore<T>>(dataStructure, k_DataArrayName, tdims, cdims, amPtr->getId());
  daPtr->fill(static_cast<T>(0.0));
}

static Arguments GetArgs(const DataPath& inputArray, DataType type, const std::string& outputArrayName)
{
  Arguments args;

  args.insertOrAssign(ConvertDataFilter::k_ScalarType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<uint8>(type)));
  args.insertOrAssign(ConvertDataFilter::k_ArrayToConvertPath_Key, std::make_any<DataPath>(inputArray));
  args.insertOrAssign(ConvertDataFilter::k_ConvertedArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));

  return args;
}

template <typename T, typename U>
static void CheckConvertedArray(DataArray<T>* originalDataArray, DataArray<U>* convertedDataArray)
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

template <typename T, typename U>
static void TestConversion(DataStructure& dataStructure, ConvertDataFilter& filter, const std::string& /*arrayName*/, DataType newType, const std::string& newArrayName, bool checkArray = true)
{
  Arguments args = GetArgs(k_DataArrayPath, newType, newArrayName);
  auto executeResults = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResults.result);

  if(checkArray)
  {
    AttributeMatrix* amPtr = dataStructure.getDataAs<AttributeMatrix>(AttributeMatrixPath);

    DataArray<T>* dataArrayPtr = dataStructure.getDataAs<DataArray<T>>(k_DataArrayPath);
    DataArray<U>* convertedDataArrayPtr = dataStructure.getDataAs<DataArray<U>>(AttributeMatrixPath.createChildPath(newArrayName));

    CheckConvertedArray<T, U>(dataArrayPtr, convertedDataArrayPtr);
  }
}

static void TestInt8Signed()
{
  ConvertDataFilter filter;
  DataStructure dataStructure;
  CreateDataStructure<int8>(dataStructure);

  TestConversion<int8, int8>(dataStructure, filter, k_DataArrayName, DataType::int8, "NewArrayChar");
  TestConversion<int8, int16>(dataStructure, filter, k_DataArrayName, DataType::int16, "NewArrayShort");
  TestConversion<int8, int32>(dataStructure, filter, k_DataArrayName, DataType::int32, "NewArrayInt");
  TestConversion<int8, int64>(dataStructure, filter, k_DataArrayName, DataType::int64, "NewArrayLong");

  TestConversion<int8, float32>(dataStructure, filter, k_DataArrayName, DataType::float32, "NewArrayFloat");
  TestConversion<int8, float64>(dataStructure, filter, k_DataArrayName, DataType::float64, "NewArrayDouble");
  TestConversion<int8, bool>(dataStructure, filter, k_DataArrayName, DataType::boolean, "NewArrayBool");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

static void TestInt8Unsigned()
{
  ConvertDataFilter filter;
  DataStructure dataStructure;
  CreateDataStructure<int8>(dataStructure);

  TestConversion<int8, uint8>(dataStructure, filter, k_DataArrayName, DataType::uint8, "NewArrayUChar");
  TestConversion<int8, uint16>(dataStructure, filter, k_DataArrayName, DataType::uint16, "NewArrayUShort");
  TestConversion<int8, uint32>(dataStructure, filter, k_DataArrayName, DataType::uint32, "NewArrayUInt");
  TestConversion<int8, uint64>(dataStructure, filter, k_DataArrayName, DataType::uint64, "NewArrayULong");
  TestConversion<int8, bool>(dataStructure, filter, k_DataArrayName, DataType::boolean, "NewArrayBool");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

static void TestUInt8Signed()
{
  ConvertDataFilter filter;
  DataStructure dataStructure;
  CreateDataStructure<uint8>(dataStructure);

  TestConversion<uint8, int8>(dataStructure, filter, k_DataArrayName, DataType::int8, "NewArrayChar");
  TestConversion<uint8, int16>(dataStructure, filter, k_DataArrayName, DataType::int16, "NewArrayShort");
  TestConversion<uint8, int32>(dataStructure, filter, k_DataArrayName, DataType::int32, "NewArrayInt");
  TestConversion<uint8, int64>(dataStructure, filter, k_DataArrayName, DataType::int64, "NewArrayLong");

  TestConversion<uint8, float32>(dataStructure, filter, k_DataArrayName, DataType::float32, "NewArrayFloat");
  TestConversion<uint8, float64>(dataStructure, filter, k_DataArrayName, DataType::float64, "NewArrayDouble");
  TestConversion<uint8, bool>(dataStructure, filter, k_DataArrayName, DataType::boolean, "NewArrayBool");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

static void TestUInt8Unsigned()
{
  ConvertDataFilter filter;
  DataStructure dataStructure;
  CreateDataStructure<uint8>(dataStructure);

  TestConversion<uint8, uint8>(dataStructure, filter, k_DataArrayName, DataType::uint8, "NewArrayUChar");
  TestConversion<uint8, uint16>(dataStructure, filter, k_DataArrayName, DataType::uint16, "NewArrayUShort");
  TestConversion<uint8, uint32>(dataStructure, filter, k_DataArrayName, DataType::uint32, "NewArrayUInt");
  TestConversion<uint8, uint64>(dataStructure, filter, k_DataArrayName, DataType::uint64, "NewArrayULong");
  TestConversion<uint8, bool>(dataStructure, filter, k_DataArrayName, DataType::boolean, "NewArrayBool");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

static void TestInt16Signed()
{
  ConvertDataFilter filter;
  DataStructure dataStructure;
  CreateDataStructure<int16>(dataStructure);

  TestConversion<int16, int8>(dataStructure, filter, k_DataArrayName, DataType::int8, "NewArrayChar");
  TestConversion<int16, int16>(dataStructure, filter, k_DataArrayName, DataType::int16, "NewArrayShort");
  TestConversion<int16, int32>(dataStructure, filter, k_DataArrayName, DataType::int32, "NewArrayInt");
  TestConversion<int16, int64>(dataStructure, filter, k_DataArrayName, DataType::int64, "NewArrayLong");

  TestConversion<int16, float>(dataStructure, filter, k_DataArrayName, DataType::float32, "NewArrayFloat");
  TestConversion<int16, double>(dataStructure, filter, k_DataArrayName, DataType::float64, "NewArrayDouble");
  TestConversion<int16, bool>(dataStructure, filter, k_DataArrayName, DataType::boolean, "NewArrayBool");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

static void TestInt16Unsigned()
{
  ConvertDataFilter filter;
  DataStructure dataStructure;
  CreateDataStructure<int16>(dataStructure);

  TestConversion<int16, uint8>(dataStructure, filter, k_DataArrayName, DataType::uint8, "NewArrayUChar");
  TestConversion<int16, uint16>(dataStructure, filter, k_DataArrayName, DataType::uint16, "NewArrayUShort");
  TestConversion<int16, uint32>(dataStructure, filter, k_DataArrayName, DataType::uint32, "NewArrayUInt");
  TestConversion<int16, uint64>(dataStructure, filter, k_DataArrayName, DataType::uint64, "NewArrayULong");
  TestConversion<int16, bool>(dataStructure, filter, k_DataArrayName, DataType::boolean, "NewArrayBool");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

static void TestUInt16Signed()
{
  ConvertDataFilter filter;
  DataStructure dataStructure;
  CreateDataStructure<uint16>(dataStructure);

  TestConversion<uint16, int8>(dataStructure, filter, k_DataArrayName, DataType::int8, "NewArrayChar");
  TestConversion<uint16, int16>(dataStructure, filter, k_DataArrayName, DataType::int16, "NewArrayShort");
  TestConversion<uint16, int32>(dataStructure, filter, k_DataArrayName, DataType::int32, "NewArrayInt");
  TestConversion<uint16, int64>(dataStructure, filter, k_DataArrayName, DataType::int64, "NewArrayLong");

  TestConversion<uint16, float32>(dataStructure, filter, k_DataArrayName, DataType::float32, "NewArrayFloat");
  TestConversion<uint16, float64>(dataStructure, filter, k_DataArrayName, DataType::float64, "NewArrayDouble");
  TestConversion<uint16, bool>(dataStructure, filter, k_DataArrayName, DataType::boolean, "NewArrayBool");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

static void TestUInt16Unsigned()
{
  ConvertDataFilter filter;
  DataStructure dataStructure;
  CreateDataStructure<uint16>(dataStructure);

  TestConversion<uint16, uint8>(dataStructure, filter, k_DataArrayName, DataType::uint8, "NewArrayUChar");
  TestConversion<uint16, uint16>(dataStructure, filter, k_DataArrayName, DataType::uint16, "NewArrayUShort");
  TestConversion<uint16, uint32>(dataStructure, filter, k_DataArrayName, DataType::uint32, "NewArrayUInt");
  TestConversion<uint16, uint64>(dataStructure, filter, k_DataArrayName, DataType::uint64, "NewArrayULong");
  TestConversion<uint16, bool>(dataStructure, filter, k_DataArrayName, DataType::boolean, "NewArrayBool");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

static void TestInt32Signed()
{
  ConvertDataFilter filter;
  DataStructure dataStructure;
  CreateDataStructure<int32>(dataStructure);

  TestConversion<int32, int8>(dataStructure, filter, k_DataArrayName, DataType::int8, "NewArrayChar");
  TestConversion<int32, int16>(dataStructure, filter, k_DataArrayName, DataType::int16, "NewArrayShort");
  TestConversion<int32, int32>(dataStructure, filter, k_DataArrayName, DataType::int32, "NewArrayInt");
  TestConversion<int32, int64>(dataStructure, filter, k_DataArrayName, DataType::int64, "NewArrayLong");

  TestConversion<int32, float32>(dataStructure, filter, k_DataArrayName, DataType::float32, "NewArrayFloat");
  TestConversion<int32, float64>(dataStructure, filter, k_DataArrayName, DataType::float64, "NewArrayDouble");
  TestConversion<int32, bool>(dataStructure, filter, k_DataArrayName, DataType::boolean, "NewArrayBool");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

static void TestInt32Unsigned()
{
  ConvertDataFilter filter;
  DataStructure dataStructure;
  CreateDataStructure<int32>(dataStructure);

  TestConversion<int32, uint8>(dataStructure, filter, k_DataArrayName, DataType::uint8, "NewArrayUChar");
  TestConversion<int32, uint16>(dataStructure, filter, k_DataArrayName, DataType::uint16, "NewArrayUShort");
  TestConversion<int32, uint32>(dataStructure, filter, k_DataArrayName, DataType::uint32, "NewArrayUInt");
  TestConversion<int32, uint64>(dataStructure, filter, k_DataArrayName, DataType::uint64, "NewArrayULong");
  TestConversion<int32, bool>(dataStructure, filter, k_DataArrayName, DataType::boolean, "NewArrayBool");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

static void TestUInt32Signed()
{
  ConvertDataFilter filter;
  DataStructure dataStructure;
  CreateDataStructure<uint32>(dataStructure);

  TestConversion<uint32, int8>(dataStructure, filter, k_DataArrayName, DataType::int8, "NewArrayChar");
  TestConversion<uint32, int16>(dataStructure, filter, k_DataArrayName, DataType::int16, "NewArrayShort");
  TestConversion<uint32, int32>(dataStructure, filter, k_DataArrayName, DataType::int32, "NewArrayInt");
  TestConversion<uint32, int64>(dataStructure, filter, k_DataArrayName, DataType::int64, "NewArrayLong");

  TestConversion<uint32, float32>(dataStructure, filter, k_DataArrayName, DataType::float32, "NewArrayFloat");
  TestConversion<uint32, float64>(dataStructure, filter, k_DataArrayName, DataType::float64, "NewArrayDouble");
  TestConversion<uint32, bool>(dataStructure, filter, k_DataArrayName, DataType::boolean, "NewArrayBool");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

static void TestUInt32Unsigned()
{
  ConvertDataFilter filter;
  DataStructure dataStructure;
  CreateDataStructure<uint32>(dataStructure);

  TestConversion<uint32, uint8>(dataStructure, filter, k_DataArrayName, DataType::uint8, "NewArrayUChar");
  TestConversion<uint32, uint16>(dataStructure, filter, k_DataArrayName, DataType::uint16, "NewArrayUShort");
  TestConversion<uint32, uint32>(dataStructure, filter, k_DataArrayName, DataType::uint32, "NewArrayUInt");
  TestConversion<uint32, uint64>(dataStructure, filter, k_DataArrayName, DataType::uint64, "NewArrayULong");
  TestConversion<uint32, bool>(dataStructure, filter, k_DataArrayName, DataType::boolean, "NewArrayBool");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

static void TestInt64Signed()
{
  ConvertDataFilter filter;
  DataStructure dataStructure;
  CreateDataStructure<int64>(dataStructure);

  TestConversion<int64, int8>(dataStructure, filter, k_DataArrayName, DataType::int8, "NewArrayChar");
  TestConversion<int64, int16>(dataStructure, filter, k_DataArrayName, DataType::int16, "NewArrayShort");
  TestConversion<int64, int32>(dataStructure, filter, k_DataArrayName, DataType::int32, "NewArrayInt");
  TestConversion<int64, int64>(dataStructure, filter, k_DataArrayName, DataType::int64, "NewArrayLong");

  TestConversion<int64, float32>(dataStructure, filter, k_DataArrayName, DataType::float32, "NewArrayFloat");
  TestConversion<int64, float64>(dataStructure, filter, k_DataArrayName, DataType::float64, "NewArrayDouble");
  TestConversion<int64, bool>(dataStructure, filter, k_DataArrayName, DataType::boolean, "NewArrayBool");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

static void TestInt64Unsigned()
{
  ConvertDataFilter filter;
  DataStructure dataStructure;
  CreateDataStructure<int64>(dataStructure);

  TestConversion<int64, uint8>(dataStructure, filter, k_DataArrayName, DataType::uint8, "NewArrayUChar");
  TestConversion<int64, uint16>(dataStructure, filter, k_DataArrayName, DataType::uint16, "NewArrayUShort");
  TestConversion<int64, uint32>(dataStructure, filter, k_DataArrayName, DataType::uint32, "NewArrayUInt");
  TestConversion<int64, uint64>(dataStructure, filter, k_DataArrayName, DataType::uint64, "NewArrayULong");
  TestConversion<int64, bool>(dataStructure, filter, k_DataArrayName, DataType::boolean, "NewArrayBool");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

static void TestUInt64Signed()
{
  ConvertDataFilter filter;
  DataStructure dataStructure;
  CreateDataStructure<uint64>(dataStructure);

  TestConversion<uint64, int8>(dataStructure, filter, k_DataArrayName, DataType::int8, "NewArrayChar");
  TestConversion<uint64, int16>(dataStructure, filter, k_DataArrayName, DataType::int16, "NewArrayShort");
  TestConversion<uint64, int32>(dataStructure, filter, k_DataArrayName, DataType::int32, "NewArrayInt");
  TestConversion<uint64, int64>(dataStructure, filter, k_DataArrayName, DataType::int64, "NewArrayLong");

  TestConversion<uint64, float32>(dataStructure, filter, k_DataArrayName, DataType::float32, "NewArrayFloat");
  TestConversion<uint64, float64>(dataStructure, filter, k_DataArrayName, DataType::float64, "NewArrayDouble");
  TestConversion<uint64, bool>(dataStructure, filter, k_DataArrayName, DataType::boolean, "NewArrayBool");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

static void TestUInt64Unsigned()
{
  ConvertDataFilter filter;
  DataStructure dataStructure;
  CreateDataStructure<uint64>(dataStructure);

  TestConversion<uint64, uint8>(dataStructure, filter, k_DataArrayName, DataType::uint8, "NewArrayUChar");
  TestConversion<uint64, uint16>(dataStructure, filter, k_DataArrayName, DataType::uint16, "NewArrayUShort");
  TestConversion<uint64, uint32>(dataStructure, filter, k_DataArrayName, DataType::uint32, "NewArrayUInt");
  TestConversion<uint64, uint64>(dataStructure, filter, k_DataArrayName, DataType::uint64, "NewArrayULong");
  TestConversion<uint64, bool>(dataStructure, filter, k_DataArrayName, DataType::boolean, "NewArrayBool");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

static void TestFloat()
{
  ConvertDataFilter filter;
  DataStructure dataStructure;
  CreateDataStructure<float32>(dataStructure);

  TestConversion<float32, int8>(dataStructure, filter, k_DataArrayName, DataType::int8, "NewArrayChar");
  TestConversion<float32, int16>(dataStructure, filter, k_DataArrayName, DataType::int16, "NewArrayShort");
  TestConversion<float32, int32>(dataStructure, filter, k_DataArrayName, DataType::int32, "NewArrayInt");
  TestConversion<float32, int64>(dataStructure, filter, k_DataArrayName, DataType::int64, "NewArrayLong");

  TestConversion<float32, float32>(dataStructure, filter, k_DataArrayName, DataType::float32, "NewArrayFloat");
  TestConversion<float32, float64>(dataStructure, filter, k_DataArrayName, DataType::float64, "NewArrayDouble");
  TestConversion<float, bool>(dataStructure, filter, k_DataArrayName, DataType::boolean, "NewArrayBool");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

static void TestDouble()
{
  ConvertDataFilter filter;
  DataStructure dataStructure;
  CreateDataStructure<float64>(dataStructure);

  TestConversion<float64, int8>(dataStructure, filter, k_DataArrayName, DataType::int8, "NewArrayChar");
  TestConversion<float64, int16>(dataStructure, filter, k_DataArrayName, DataType::int16, "NewArrayShort");
  TestConversion<float64, int32>(dataStructure, filter, k_DataArrayName, DataType::int32, "NewArrayInt");
  TestConversion<float64, int64>(dataStructure, filter, k_DataArrayName, DataType::int64, "NewArrayLong");

  TestConversion<float64, float32>(dataStructure, filter, k_DataArrayName, DataType::float32, "NewArrayFloat");
  TestConversion<float64, float64>(dataStructure, filter, k_DataArrayName, DataType::float64, "NewArrayDouble");
  TestConversion<double, bool>(dataStructure, filter, k_DataArrayName, DataType::boolean, "NewArrayBool");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

static void TestBoolSigned()
{
  ConvertDataFilter filter;
  DataStructure dataStructure;
  CreateDataStructure<bool>(dataStructure);

  TestConversion<bool, int8>(dataStructure, filter, k_DataArrayName, DataType::int8, "NewArrayChar");
  TestConversion<bool, int16>(dataStructure, filter, k_DataArrayName, DataType::int16, "NewArrayShort");
  TestConversion<bool, int32>(dataStructure, filter, k_DataArrayName, DataType::int32, "NewArrayInt");
  TestConversion<bool, int64>(dataStructure, filter, k_DataArrayName, DataType::int64, "NewArrayLong");

  TestConversion<bool, float32>(dataStructure, filter, k_DataArrayName, DataType::float32, "NewArrayFloat");
  TestConversion<bool, float64>(dataStructure, filter, k_DataArrayName, DataType::float64, "NewArrayDouble");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

static void TestBoolUnsigned()
{
  ConvertDataFilter filter;
  DataStructure dataStructure;
  CreateDataStructure<bool>(dataStructure);

  TestConversion<bool, uint8>(dataStructure, filter, k_DataArrayName, DataType::uint8, "NewArrayUChar");
  TestConversion<bool, uint16>(dataStructure, filter, k_DataArrayName, DataType::uint16, "NewArrayUShort");
  TestConversion<bool, uint32>(dataStructure, filter, k_DataArrayName, DataType::uint32, "NewArrayUInt");
  TestConversion<bool, uint64>(dataStructure, filter, k_DataArrayName, DataType::uint64, "NewArrayULong");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

static void TestInvalidDataArray()
{
  ConvertDataFilter filter;
  DataStructure dataStructure;
  CreateDataStructure<int8>(dataStructure);

  Arguments args = GetArgs(AttributeMatrixPath.createChildPath("Array1"), DataType::int8, "NewArray");
  auto executeResults = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResults.result);
}

static void TestOverwriteArray()
{
  ConvertDataFilter filter;
  DataStructure dataStructure;
  CreateDataStructure<int8>(dataStructure);

  Arguments args = GetArgs(k_DataArrayPath, DataType::int8, "DataArray");
  auto executeResults = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResults.result);
}

TEST_CASE("SimplnxCore::ConvertData: Valid Execution", "[SimplnxCore][ConvertDataFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  unit_test::LoadPlugins();

  std::cout << "#### ConvertDataTest Starting ####" << '\n';

  scope.execute([&] { TestInt8Signed(); });
  scope.execute([&] { TestInt8Unsigned(); });

  scope.execute([&] { TestInt16Signed(); });
  scope.execute([&] { TestInt16Unsigned(); });

  scope.execute([&] { TestInt32Signed(); });
  scope.execute([&] { TestInt32Unsigned(); });

  scope.execute([&] { TestInt64Signed(); });
  scope.execute([&] { TestInt64Unsigned(); });

  scope.execute([&] { TestUInt8Signed(); });
  scope.execute([&] { TestUInt8Unsigned(); });

  scope.execute([&] { TestUInt16Signed(); });
  scope.execute([&] { TestUInt16Unsigned(); });

  scope.execute([&] { TestUInt32Signed(); });
  scope.execute([&] { TestUInt32Unsigned(); });

  scope.execute([&] { TestUInt64Signed(); });
  scope.execute([&] { TestUInt64Unsigned(); });

  scope.execute([&] { TestFloat(); });
  scope.execute([&] { TestDouble(); });

  scope.execute([&] { TestBoolSigned(); });
  scope.execute([&] { TestBoolUnsigned(); });

  TestInvalidDataArray();
  TestOverwriteArray();
}

TEST_CASE("SimplnxCore::ConvertData: Preflight rejects unsupported array kinds", "[SimplnxCore][ConvertDataFilter]")
{
  const bool useStringArray = GENERATE(false, true);
  DataStructure dataStructure;
  const DataPath inputPath({"Input"});
  if(useStringArray)
  {
    REQUIRE(StringArray::CreateWithValues(dataStructure, inputPath.getTargetName(), {1}, {"1"}) != nullptr);
  }
  else
  {
    REQUIRE(NeighborList<int32>::Create(dataStructure, inputPath.getTargetName(), {1}) != nullptr);
  }

  const auto args = GetArgs(inputPath, DataType::float32, "Converted");
  IFilter::PreflightResult preflight;
  REQUIRE_NOTHROW(preflight = ConvertDataFilter().preflight(dataStructure, args));
  SIMPLNX_RESULT_REQUIRE_INVALID(preflight.output_actions);
  REQUIRE(preflight.output_actions.errors().size() == 1);
  CHECK(preflight.output_actions.errors().front().code == filter_parameter::constants::k_Validate_ArrayType_Error);
  CHECK_FALSE(dataStructure.containsData(DataPath({"Converted"})));
}

TEST_CASE("SimplnxCore::ConvertData: In Place Execution", "[SimplnxCore][ConvertDataFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  unit_test::LoadPlugins();

  ConvertDataFilter filter;
  DataStructure dataStructure;
  CreateDataStructure<int8>(dataStructure);

  REQUIRE(dataStructure.getDataAs<IDataArray>(k_DataArrayPath)->getDataType() == DataType::int8);
  Arguments args = GetArgs(k_DataArrayPath, DataType::int32, "DataArray");
  args.insertOrAssign(ConvertDataFilter::k_DeleteOriginal_Key, true);
  auto executeResults = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResults.result);
  REQUIRE(dataStructure.getDataAs<IDataArray>(k_DataArrayPath)->getDataType() == DataType::int32);
}

TEST_CASE("SimplnxCore::ConvertDataFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ConvertDataFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ConvertDataFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ConvertDataFilter.json"},
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
      REQUIRE(filterPtr->uuid() == FilterTraits<ConvertDataFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      CHECK(args.value<ChoicesParameter::ValueType>(ConvertDataFilter::k_ScalarType_Key) == 0);
      CHECK(args.value<DataPath>(ConvertDataFilter::k_ArrayToConvertPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}
