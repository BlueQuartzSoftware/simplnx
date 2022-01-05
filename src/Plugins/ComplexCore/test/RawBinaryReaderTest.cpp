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

#include <catch2/catch.hpp>

#include "complex/Common/ScopeGuard.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "ComplexCore/Filters/RawBinaryReaderFilter.hpp"
#include "complex/unit_test/complex_test_dirs.hpp"

using namespace complex;

const std::string k_TestOutput = (fs::path(complex::unit_test::k_BinaryDir.str()).append("RawBinaryReaderTest").append("Output.bin")).string();
const DataPath k_CreatedArrayPath = DataPath(std::vector<std::string>{"Test_Array"});

constexpr int32_t k_RbrNumComponentsError = -392;
constexpr int32_t k_RbrWrongType = -393;
constexpr int32_t k_RbrSkippedTooMuch = -395;

namespace Detail
{
enum Endian
{
  Little = 0,
  Big
};
} // namespace Detail

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
Arguments createFilterArguments(complex::NumericType scalarType, usize N, usize skipBytes)
{
  Arguments args;

  args.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(k_TestOutput)));
  args.insertOrAssign(RawBinaryReaderFilter::k_ScalarType_Key, std::make_any<NumericType>(scalarType));
  args.insertOrAssign(RawBinaryReaderFilter::k_NumberOfComponents_Key, std::make_any<uint64>(N));
  args.insertOrAssign(RawBinaryReaderFilter::k_Endian_Key, std::make_any<ChoicesParameter::ValueType>(Detail::Little));
  args.insertOrAssign(RawBinaryReaderFilter::k_SkipHeaderBytes_Key, std::make_any<uint64>(skipBytes));
  args.insertOrAssign(RawBinaryReaderFilter::k_CreatedAttributeArrayPath_Key, k_CreatedArrayPath);

  return args;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
template <typename T>
bool createAndWriteToFile(std::string filePath, T* dataArray, usize dataSize)
{
  // Create the output file to dump some data into
  FILE* f = fopen(filePath.c_str(), "wb");

  // Write the data to the file
  usize numWritten = 0;
  while(1)
  {
    numWritten += fwrite(dataArray, sizeof(T), dataSize, f);
    if(numWritten == dataSize)
    {
      break;
    }
    dataArray = dataArray + numWritten;
  }

  // Close the file
  fclose(f);

  // Return successful
  return true;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
// Case1: This tests when skipHeaderBytes equals 0, and checks to see if the data read is the same as the data written.
template <typename T, usize N>
void testCase1_Execute(complex::NumericType scalarType)
{
  usize dataArraySize = 10000000 * N;
  usize skipHeaderBytes = 0;
  DataStructure ds;

  // Allocate an array, and get the dataArray from that array
  std::shared_ptr<DataStore<T>> store = std::make_shared<DataStore<T>>(dataArraySize);
  T* dataArray = store->data();

  // Write some data into the data array
  for(usize i = 0; i < dataArraySize; ++i)
  {
    dataArray[i] = static_cast<T>(i);
  }

  // Create scope guard to remove file after this test goes out of scope
  fs::path testPath = k_TestOutput;
  auto fileGuard = MakeScopeGuard([testPath]() noexcept { fs::remove(testPath); });

  // Create the file and write to it.  If any of the information is wrong, the result will be false
  bool result = createAndWriteToFile(k_TestOutput, dataArray, dataArraySize);

  // Test to make sure that the file was created and written to successfully
  REQUIRE(result);

  // Create the filter, passing in the skipHeaderBytes
  RawBinaryReaderFilter filt;
  Arguments args = createFilterArguments(scalarType, N, skipHeaderBytes);

  // Preflight, get the error condition, and check that there are no errors
  auto preflightResult = filt.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter, check that there are no errors, and compare the data
  auto executeResult = filt.execute(ds, args);
  REQUIRE(executeResult.result.valid());

  DataArray<T>* createdData = ds.getDataAs<DataArray<T>>(k_CreatedArrayPath);
  for(usize i = 0; i < dataArraySize; ++i)
  {
    T d = createdData->at(i);
    T p = store->getValue(i);
    REQUIRE(d == p);
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
template <typename T>
void testCase1_TestPrimitives(complex::NumericType scalarType)
{
  testCase1_Execute<T, 1>(scalarType);
  testCase1_Execute<T, 2>(scalarType);
  testCase1_Execute<T, 3>(scalarType);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
// Case2: This tests when the wrong scalar type is selected. (The total number of bytes in the file does not evenly divide by the scalar type size).
void testCase2_Execute()
{
  usize dataArraySize = 10000001; // We need the data array size to not be divisible by 2 (int16 byte size)
  usize skipHeaderBytes = 0;
  usize N = 1;
  complex::NumericType scalarType = complex::NumericType::int16;
  DataStructure ds;

  // Allocate an array, and get the dataArray from that array
  std::shared_ptr<DataStore<int8>> store = std::make_shared<DataStore<int8>>(dataArraySize);
  int8* dataArray = store->data();

  // Write some data into the data array
  for(usize i = 0; i < dataArraySize; ++i)
  {
    dataArray[i] = static_cast<int8>(i);
  }

  // Create scope guard to remove file after this test goes out of scope
  fs::path testPath = k_TestOutput;
  auto fileGuard = MakeScopeGuard([testPath]() noexcept { fs::remove(testPath); });

  // Create the file and write to it.  If any of the information is wrong, the result will be false
  bool result = createAndWriteToFile(k_TestOutput, dataArray, dataArraySize);

  // Test to make sure that the file was created and written to successfully
  REQUIRE(result == true);

  // Create the filter, passing in the skipHeaderBytes
  RawBinaryReaderFilter filt;
  Arguments args = createFilterArguments(scalarType, N, skipHeaderBytes);

  // Preflight, get the error condition, and check that there are no errors
  auto preflightResult = filt.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.invalid());

  std::vector<Error> errors = preflightResult.outputActions.errors();
  REQUIRE(errors.size() == 1);
  REQUIRE(errors[0].code == k_RbrWrongType);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
// Case2: This tests when the wrong scalar type is selected. (The total number of bytes in the file does not evenly divide by the scalar type size).
void testCase3_Execute()
{
  usize dataArraySize = 1000001; // We need the data array size to not be divisible by 2 (int16 byte size)
  usize skipHeaderBytes = 0;
  usize N = 3;
  complex::NumericType scalarType = complex::NumericType::int64;
  DataStructure ds;

  // Allocate an array, and get the dataArray from that array
  std::shared_ptr<DataStore<int64>> store = std::make_shared<DataStore<int64>>(dataArraySize);
  int64* dataArray = store->data();

  // Write some data into the data array
  for(usize i = 0; i < dataArraySize; ++i)
  {
    dataArray[i] = static_cast<int64>(i);
  }

  // Create scope guard to remove file after this test goes out of scope
  fs::path testPath = k_TestOutput;
  auto fileGuard = MakeScopeGuard([testPath]() noexcept { fs::remove(testPath); });

  // Create the file and write to it.  If any of the information is wrong, the result will be false
  bool result = createAndWriteToFile(k_TestOutput, dataArray, dataArraySize);

  // Test to make sure that the file was created and written to successfully
  REQUIRE(result == true);

  // Create the filter, passing in the skipHeaderBytes
  RawBinaryReaderFilter filt;
  Arguments args = createFilterArguments(scalarType, N, skipHeaderBytes);

  // Preflight, get the error condition, and check that there are no errors
  auto preflightResult = filt.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.invalid());

  std::vector<Error> errors = preflightResult.outputActions.errors();
  REQUIRE(errors.size() == 1);
  REQUIRE(errors[0].code == k_RbrNumComponentsError);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
// Case4: This tests when skipHeaderBytes is non-zero, and checks to see if the data read is the same as the data written.
template <typename T, usize N>
void testCase4_Execute(complex::NumericType scalarType)
{
  usize dataArraySize = 10000000 * N;
  usize skipHeaderBytes = 100 * N * sizeof(T);
  DataStructure ds;

  // Allocate an array, and get the dataArray from that array
  std::shared_ptr<DataStore<T>> store = std::make_shared<DataStore<T>>(dataArraySize);
  T* exemplaryData = store->data();

  // Write some data into the data array
  for(usize i = 0; i < dataArraySize; ++i)
  {
    exemplaryData[i] = static_cast<T>(i);
  }

  // Create scope guard to remove file after this test goes out of scope
  fs::path testPath = k_TestOutput;
  auto fileGuard = MakeScopeGuard([testPath]() noexcept { fs::remove(testPath); });

  // Create the file and write to it.  If any of the information is wrong, the result will be false
  bool result = createAndWriteToFile(k_TestOutput, exemplaryData, dataArraySize);

  // Test to make sure that the file was created and written to successfully
  REQUIRE(result == true);

  // Create the filter, passing in the skipHeaderBytes
  RawBinaryReaderFilter filt;
  Arguments args = createFilterArguments(scalarType, N, skipHeaderBytes);

  // Preflight, get the error condition, and check that there are no errors
  auto preflightResult = filt.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter, check that there are no errors, and compare the data
  auto executeResult = filt.execute(ds, args);
  REQUIRE(executeResult.result.valid());

  DataArray<T>* createdArray = ds.getDataAs<DataArray<T>>(k_CreatedArrayPath);
  REQUIRE(createdArray != nullptr);
  DataStore<T>* createdStore = createdArray->template getIDataStoreAs<DataStore<T>>();
  REQUIRE(createdStore != nullptr);
  T* createdData = createdStore->data();

  usize elementOffset = skipHeaderBytes / sizeof(T);
  for(usize i = 0; i < createdStore->getSize(); ++i)
  {
    T c = createdData[i];
    T e = exemplaryData[i + elementOffset];
    REQUIRE(c == e);
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
// Case5: This tests when skipHeaderBytes equals the file size
template <typename T, usize N>
void testCase5_Execute(complex::NumericType scalarType)
{
  usize dataArraySize = 10000000 * N;
  usize skipHeaderBytes = 10000000 * N * sizeof(T);
  DataStructure ds;

  // Allocate an array, and get the dataArray from that array
  std::shared_ptr<DataStore<T>> store = std::make_shared<DataStore<T>>(dataArraySize);
  T* exemplaryData = store->data();

  // Write some data into the data array
  for(usize i = 0; i < dataArraySize; ++i)
  {
    exemplaryData[i] = static_cast<T>(i);
  }

  // Create scope guard to remove file after this test goes out of scope
  fs::path testPath = k_TestOutput;
  auto fileGuard = MakeScopeGuard([testPath]() noexcept { fs::remove(testPath); });

  // Create the file and write to it.  If any of the information is wrong, the result will be false
  bool result = createAndWriteToFile(k_TestOutput, exemplaryData, dataArraySize);

  // Test to make sure that the file was created and written to successfully
  REQUIRE(result == true);

  // Create the filter, passing in the skipHeaderBytes
  RawBinaryReaderFilter filt;
  Arguments args = createFilterArguments(scalarType, N, skipHeaderBytes);

  // Preflight, get the error condition, and check that there are no errors
  auto preflightResult = filt.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.invalid());

  std::vector<Error> errors = preflightResult.outputActions.errors();
  REQUIRE(errors.size() == 1);
  REQUIRE(errors[0].code == k_RbrSkippedTooMuch);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
template <typename T>
void testCase5_TestPrimitives(complex::NumericType scalarType)
{
  testCase5_Execute<T, 1>(scalarType);
  testCase5_Execute<T, 2>(scalarType);
  testCase5_Execute<T, 3>(scalarType);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
template <typename T>
void testCase4_TestPrimitives(complex::NumericType scalarType)
{
  testCase4_Execute<T, 1>(scalarType);
  testCase4_Execute<T, 2>(scalarType);
  testCase4_Execute<T, 3>(scalarType);
}

// Case1: This tests when skipHeaderBytes equals 0, and checks to see if the data read is the same as the data written.
TEST_CASE("RawBinaryReaderFilter(Case1)", "[ComplexCore][RawBinaryReaderFilter][Case1]")
{
  // Create the parent directory path
  fs::create_directories(fs::path(k_TestOutput).parent_path());

  testCase1_TestPrimitives<int8_t>(complex::NumericType::int8);
  testCase1_TestPrimitives<uint8_t>(complex::NumericType::uint8);
  testCase1_TestPrimitives<int16_t>(complex::NumericType::int16);
  testCase1_TestPrimitives<uint16_t>(complex::NumericType::uint16);
  testCase1_TestPrimitives<int32_t>(complex::NumericType::int32);
  testCase1_TestPrimitives<uint32_t>(complex::NumericType::uint32);
  testCase1_TestPrimitives<int64_t>(complex::NumericType::int64);
  testCase1_TestPrimitives<uint64_t>(complex::NumericType::uint64);
  testCase1_TestPrimitives<float>(complex::NumericType::float32);
  testCase1_TestPrimitives<double>(complex::NumericType::float64);
}

// Case2: This tests when the wrong scalar type is selected. (The total number of bytes in the file does not evenly divide by the scalar type size).
TEST_CASE("RawBinaryReaderFilter(Case2)", "[ComplexCore][RawBinaryReaderFilter][Case2]")
{
  // Create the parent directory path
  fs::create_directories(fs::path(k_TestOutput).parent_path());

  testCase2_Execute();
}

// Case3: This tests when the wrong component size is chosen. (The total number of scalar elements in the file does not evenly divide by the chosen component size).
TEST_CASE("RawBinaryReaderFilter(Case3)", "[ComplexCore][RawBinaryReaderFilter][Case3]")
{
  // Create the parent directory path
  fs::create_directories(fs::path(k_TestOutput).parent_path());

  testCase3_Execute();
}

// Case4: This tests when skipHeaderBytes is non-zero, and checks to see if the data read is the same as the data written.
TEST_CASE("RawBinaryReaderFilter(Case4)", "[ComplexCore][RawBinaryReaderFilter][Case4]")
{
  // Create the parent directory path
  fs::create_directories(fs::path(k_TestOutput).parent_path());

  testCase4_TestPrimitives<int8_t>(complex::NumericType::int8);
  testCase4_TestPrimitives<uint8_t>(complex::NumericType::uint8);
  testCase4_TestPrimitives<int16_t>(complex::NumericType::int16);
  testCase4_TestPrimitives<uint16_t>(complex::NumericType::uint16);
  testCase4_TestPrimitives<int32_t>(complex::NumericType::int32);
  testCase4_TestPrimitives<uint32_t>(complex::NumericType::uint32);
  testCase4_TestPrimitives<int64_t>(complex::NumericType::int64);
  testCase4_TestPrimitives<uint64_t>(complex::NumericType::uint64);
  testCase4_TestPrimitives<float>(complex::NumericType::float32);
  testCase4_TestPrimitives<double>(complex::NumericType::float64);
}

// Case5: This tests when skipHeaderBytes equals the file size
TEST_CASE("RawBinaryReaderFilter(Case5)", "[ComplexCore][RawBinaryReaderFilter][Case5]")
{
  // Create the parent directory path
  fs::create_directories(fs::path(k_TestOutput).parent_path());

  testCase5_TestPrimitives<int8_t>(complex::NumericType::int8);
  testCase5_TestPrimitives<uint8_t>(complex::NumericType::uint8);
  testCase5_TestPrimitives<int16_t>(complex::NumericType::int16);
  testCase5_TestPrimitives<uint16_t>(complex::NumericType::uint16);
  testCase5_TestPrimitives<int32_t>(complex::NumericType::int32);
  testCase5_TestPrimitives<uint32_t>(complex::NumericType::uint32);
  testCase5_TestPrimitives<int64_t>(complex::NumericType::int64);
  testCase5_TestPrimitives<uint64_t>(complex::NumericType::uint64);
  testCase5_TestPrimitives<float>(complex::NumericType::float32);
  testCase5_TestPrimitives<double>(complex::NumericType::float64);
}
