/** Testing Notes:
 *
 *  Case1: This tests when skipHeaderBytes equals 0, and checks to see if the data read is the same as the data written.
 *
 *  Case2: This tests when the wrong scalar type is selected. (The total number of bytes in the file does not evenly divide by the scalar type size).
 *
 *  Case3: This tests when the wrong component size is chosen. (The total number of scalar elements in the file does not evenly divide by the chosen component size).
 *
 *  Case4: This tests when skipHeaderBytes is non-zero, and checks to see if the data read is the same as the data written.
 *
 *  Case5: This tests when skipHeaderBytes equals the file size
 */

/** we are going to use a fairly large array size because we want to exercise the
 * possibilty that we can not write the data or read the data from the file in one
 * step in the filter
 */
#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/RawBinaryReaderFilter.hpp"

#include "complex/Common/ScopeGuard.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace complex;

namespace
{
const fs::path k_TestOutput = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "Output.bin";
const DataPath k_CreatedArrayPath = DataPath({"Test_Array"});

constexpr int32 k_RbrNumComponentsError = -392;
constexpr int32 k_RbrWrongType = -393;
constexpr int32 k_RbrSkippedTooMuch = -395;

// -----------------------------------------------------------------------------
Arguments CreateFilterArguments(NumericType scalarType, usize N, usize file_size, usize skipBytes)
{
  Arguments args;

  args.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_TestOutput));
  args.insertOrAssign(RawBinaryReaderFilter::k_ScalarType_Key, std::make_any<NumericType>(scalarType));

  args.insertOrAssign(RawBinaryReaderFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{static_cast<float64>(file_size)}}));

  args.insertOrAssign(RawBinaryReaderFilter::k_NumberOfComponents_Key, std::make_any<uint64>(N));
  args.insertOrAssign(RawBinaryReaderFilter::k_Endian_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<uint64>(endian::little)));
  args.insertOrAssign(RawBinaryReaderFilter::k_SkipHeaderBytes_Key, std::make_any<uint64>(skipBytes));
  args.insertOrAssign(RawBinaryReaderFilter::k_CreatedAttributeArrayPath_Key, k_CreatedArrayPath);

  return args;
}

// -----------------------------------------------------------------------------
template <class T>
bool CreateTestDataFile(const std::vector<T>& exemplaryData)
{
  std::ofstream file(k_TestOutput, std::ios::binary);
  if(!file.is_open())
  {
    return false;
  }

  file.write(reinterpret_cast<const char*>(exemplaryData.data()), exemplaryData.size() * sizeof(T));

  return true;
}

// -----------------------------------------------------------------------------
// Case1: This tests when skipHeaderBytes equals 0, and checks to see if the data read is the same as the data written.
template <class T, usize N>
void TestCase1_Execute(NumericType scalarType)
{
  constexpr usize tupleCount = 10000000;
  constexpr usize dataArraySize = tupleCount * N;
  constexpr usize skipHeaderBytes = 0;

  std::vector<T> exemplaryData(dataArraySize);
  std::iota(exemplaryData.begin(), exemplaryData.end(), static_cast<T>(0));

  // Create scope guard to remove file after this test goes out of scope
  auto fileGuard = MakeScopeGuard([]() noexcept { fs::remove(k_TestOutput); });

  // Create the file and write to it.  If any of the information is wrong, the result will be false
  bool result = CreateTestDataFile<T>(exemplaryData);

  // Test to make sure that the file was created and written to successfully
  REQUIRE(result);

  // Create the filter, passing in the skipHeaderBytes
  RawBinaryReaderFilter filter;
  Arguments args = CreateFilterArguments(scalarType, N, tupleCount, skipHeaderBytes);

  DataStructure dataStructure;

  // Preflight, get the error condition, and check that there are no errors
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter, check that there are no errors, and compare the data
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataArray<T>& createdData = dataStructure.getDataRefAs<DataArray<T>>(k_CreatedArrayPath);
  const AbstractDataStore<T>& store = createdData.getDataStoreRef();
  bool isSame = true;
  for(usize i = 0; i < dataArraySize; ++i)
  {
    if(store[i] != exemplaryData[i])
    {
      isSame = false;
      break;
    }
  }
  REQUIRE(isSame);
}

// -----------------------------------------------------------------------------
template <class T>
void TestCase1_TestPrimitives(NumericType scalarType)
{
  TestCase1_Execute<T, 1>(scalarType);
  TestCase1_Execute<T, 2>(scalarType);
  TestCase1_Execute<T, 3>(scalarType);
}

// -----------------------------------------------------------------------------
// Case2: This tests when the wrong scalar type is selected. (The total number of bytes in the file does not evenly divide by the scalar type size).
void TestCase2_Execute()
{
  constexpr usize dataArraySize = 10000001; // We need the data array size to not be divisible by 2 (int16 byte size)
  constexpr usize skipHeaderBytes = 0;
  constexpr usize N = 1;
  constexpr NumericType scalarType = NumericType::int16;

  std::vector<int8> exemplaryData(dataArraySize);
  std::iota(exemplaryData.begin(), exemplaryData.end(), static_cast<int8>(0));

  // Create scope guard to remove file after this test goes out of scope
  auto fileGuard = MakeScopeGuard([]() noexcept { fs::remove(k_TestOutput); });

  // Create the file and write to it.  If any of the information is wrong, the result will be false
  bool result = CreateTestDataFile<int8>(exemplaryData);

  // Test to make sure that the file was created and written to successfully
  REQUIRE(result);

  // Create the filter, passing in the skipHeaderBytes
  RawBinaryReaderFilter filter;
  Arguments args = CreateFilterArguments(scalarType, N, dataArraySize, skipHeaderBytes);

  DataStructure dataStructure;

  // Preflight, get the error condition, and check that there are no errors
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  const std::vector<Error>& errors = preflightResult.outputActions.errors();
  REQUIRE(errors.size() == 1);
  REQUIRE(errors[0].code == k_RbrWrongType);
}

// -----------------------------------------------------------------------------
// Case2: This tests when the wrong scalar type is selected. (The total number of bytes in the file does not evenly divide by the scalar type size).
void TestCase3_Execute()
{
  constexpr usize dataArraySize = 1000001; // We need the data array size to not be divisible by 2 (int16 byte size)
  constexpr usize skipHeaderBytes = 0;
  constexpr usize N = 3;
  constexpr NumericType scalarType = NumericType::int64;

  std::vector<int64> exemplaryData(dataArraySize);
  std::iota(exemplaryData.begin(), exemplaryData.end(), static_cast<int64>(0));

  // Create scope guard to remove file after this test goes out of scope
  auto fileGuard = MakeScopeGuard([]() noexcept { fs::remove(k_TestOutput); });

  // Create the file and write to it.  If any of the information is wrong, the result will be false
  bool result = CreateTestDataFile<int64>(exemplaryData);

  // Test to make sure that the file was created and written to successfully
  REQUIRE(result);

  // Create the filter, passing in the skipHeaderBytes
  RawBinaryReaderFilter filter;
  Arguments args = CreateFilterArguments(scalarType, N, dataArraySize, skipHeaderBytes);

  DataStructure dataStructure;

  // Preflight, get the error condition, and check that there are no errors
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  const std::vector<Error>& errors = preflightResult.outputActions.errors();
  REQUIRE(errors.size() == 1);
  REQUIRE(errors[0].code == k_RbrNumComponentsError);
}

// -----------------------------------------------------------------------------
// Case4: This tests when skipHeaderBytes is non-zero, and checks to see if the data read is the same as the data written.
template <class T, usize N>
void TestCase4_Execute(NumericType scalarType)
{
  constexpr usize tupleCount = 10000000;
  constexpr usize dataArraySize = tupleCount * N;
  constexpr usize skipHeaderTuples = 100;
  constexpr usize skipHeaderBytes = skipHeaderTuples * N * sizeof(T);

  std::vector<T> exemplaryData(dataArraySize);
  std::iota(exemplaryData.begin(), exemplaryData.end(), static_cast<T>(0));

  // Create scope guard to remove file after this test goes out of scope
  auto fileGuard = MakeScopeGuard([]() noexcept { fs::remove(k_TestOutput); });

  // Create the file and write to it.  If any of the information is wrong, the result will be false
  bool result = CreateTestDataFile<T>(exemplaryData);

  // Test to make sure that the file was created and written to successfully
  REQUIRE(result);

  // Create the filter, passing in the skipHeaderBytes
  RawBinaryReaderFilter filter;
  Arguments args = CreateFilterArguments(scalarType, N, tupleCount - skipHeaderTuples, skipHeaderBytes);

  DataStructure dataStructure;

  // Preflight, get the error condition, and check that there are no errors
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter, check that there are no errors, and compare the data
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  DataArray<T>* createdArray = dataStructure.getDataAs<DataArray<T>>(k_CreatedArrayPath);
  REQUIRE(createdArray != nullptr);
  AbstractDataStore<T>& createdStore = createdArray->getDataStoreRef();

  constexpr usize elementOffset = skipHeaderBytes / sizeof(T);
  bool isSame = true;
  usize size = createdStore.getSize();
  for(usize i = 0; i < size; ++i)
  {
    if(createdStore[i] != exemplaryData[i + elementOffset])
    {
      isSame = false;
      break;
    }
  }
  REQUIRE(isSame);
}

// -----------------------------------------------------------------------------
// Case5: This tests when skipHeaderBytes equals the file size
template <class T, usize N>
void TestCase5_Execute(NumericType scalarType)
{
  constexpr usize dataArraySize = 10000000 * N;
  constexpr usize skipHeaderBytes = 10000000 * N * sizeof(T);

  std::vector<T> exemplaryData(dataArraySize);
  std::iota(exemplaryData.begin(), exemplaryData.end(), static_cast<T>(0));

  // Create scope guard to remove test file after this test goes out of scope
  auto fileGuard = MakeScopeGuard([]() noexcept { fs::remove(k_TestOutput); });

  // Create the test file
  bool result = CreateTestDataFile<T>(exemplaryData);

  // Test to make sure that the file was created and written to successfully
  REQUIRE(result);

  // Create the filter, passing in the skipHeaderBytes
  RawBinaryReaderFilter filter;
  Arguments args = CreateFilterArguments(scalarType, N, dataArraySize, skipHeaderBytes);

  DataStructure dataStructure;

  // Preflight, get the error condition, and check that there are no errors
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  const std::vector<Error>& errors = preflightResult.outputActions.errors();
  REQUIRE(errors.size() == 1);
  REQUIRE(errors[0].code == k_RbrSkippedTooMuch);
}

// -----------------------------------------------------------------------------
template <class T>
void TestCase5_TestPrimitives(NumericType scalarType)
{
  TestCase5_Execute<T, 1>(scalarType);
  TestCase5_Execute<T, 2>(scalarType);
  TestCase5_Execute<T, 3>(scalarType);
}

// -----------------------------------------------------------------------------
template <class T>
void TestCase4_TestPrimitives(NumericType scalarType)
{
  TestCase4_Execute<T, 1>(scalarType);
  TestCase4_Execute<T, 2>(scalarType);
  TestCase4_Execute<T, 3>(scalarType);
}
} // namespace

// Case1: This tests when skipHeaderBytes equals 0, and checks to see if the data read is the same as the data written.
TEST_CASE("ComplexCore::RawBinaryReaderFilter(Case1)", "[ComplexCore][RawBinaryReaderFilter]")
{
  // Create the parent directory path
  fs::create_directories(k_TestOutput.parent_path());

  TestCase1_TestPrimitives<int8>(NumericType::int8);
  TestCase1_TestPrimitives<uint8>(NumericType::uint8);
  TestCase1_TestPrimitives<int16>(NumericType::int16);
  TestCase1_TestPrimitives<uint16>(NumericType::uint16);
  TestCase1_TestPrimitives<int32>(NumericType::int32);
  TestCase1_TestPrimitives<uint32>(NumericType::uint32);
  TestCase1_TestPrimitives<int64>(NumericType::int64);
  TestCase1_TestPrimitives<uint64>(NumericType::uint64);
  TestCase1_TestPrimitives<float32>(NumericType::float32);
  TestCase1_TestPrimitives<float64>(NumericType::float64);
}

// Case2: This tests when the wrong scalar type is selected. (The total number of bytes in the file does not evenly divide by the scalar type size).
TEST_CASE("ComplexCore::RawBinaryReaderFilter(Case2)", "[ComplexCore][RawBinaryReaderFilter]")
{
  // Create the parent directory path
  fs::create_directories(k_TestOutput.parent_path());

  TestCase2_Execute();
}

// Case3: This tests when the wrong component size is chosen. (The total number of scalar elements in the file does not evenly divide by the chosen component size).
TEST_CASE("ComplexCore::RawBinaryReaderFilter(Case3)", "[ComplexCore][RawBinaryReaderFilter]")
{
  // Create the parent directory path
  fs::create_directories(k_TestOutput.parent_path());

  TestCase3_Execute();
}

// Case4: This tests when skipHeaderBytes is non-zero, and checks to see if the data read is the same as the data written.
TEST_CASE("ComplexCore::RawBinaryReaderFilter(Case4)", "[ComplexCore][RawBinaryReaderFilter]")
{
  // Create the parent directory path
  fs::create_directories(k_TestOutput.parent_path());

  TestCase4_TestPrimitives<int8>(NumericType::int8);
  TestCase4_TestPrimitives<uint8>(NumericType::uint8);
  TestCase4_TestPrimitives<int16>(NumericType::int16);
  TestCase4_TestPrimitives<uint16>(NumericType::uint16);
  TestCase4_TestPrimitives<int32>(NumericType::int32);
  TestCase4_TestPrimitives<uint32>(NumericType::uint32);
  TestCase4_TestPrimitives<int64>(NumericType::int64);
  TestCase4_TestPrimitives<uint64>(NumericType::uint64);
  TestCase4_TestPrimitives<float32>(NumericType::float32);
  TestCase4_TestPrimitives<float64>(NumericType::float64);
}

// Case5: This tests when skipHeaderBytes equals the file size
TEST_CASE("ComplexCore::RawBinaryReaderFilter(Case5)", "[ComplexCore][RawBinaryReaderFilter]")
{
  // Create the parent directory path
  fs::create_directories(k_TestOutput.parent_path());

  TestCase5_TestPrimitives<int8>(NumericType::int8);
  TestCase5_TestPrimitives<uint8>(NumericType::uint8);
  TestCase5_TestPrimitives<int16>(NumericType::int16);
  TestCase5_TestPrimitives<uint16>(NumericType::uint16);
  TestCase5_TestPrimitives<int32>(NumericType::int32);
  TestCase5_TestPrimitives<uint32>(NumericType::uint32);
  TestCase5_TestPrimitives<int64>(NumericType::int64);
  TestCase5_TestPrimitives<uint64>(NumericType::uint64);
  TestCase5_TestPrimitives<float32>(NumericType::float32);
  TestCase5_TestPrimitives<float64>(NumericType::float64);
}
