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
#include "SimplnxCore/Filters/ReadRawBinaryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const fs::path k_TestOutput = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "Output.bin";
const DataPath k_CreatedArrayPath = DataPath({"Test_Array"});

constexpr int32 k_RbrWrongType = -78707;
constexpr int32 k_RbrSkippedTooMuch = -78706;
constexpr int32 k_RbrFileTooSmall = -78708;

// -----------------------------------------------------------------------------
Arguments CreateFilterArguments(NumericType scalarType, usize N, usize file_size, usize skipBytes)
{
  Arguments args;

  args.insertOrAssign(ReadRawBinaryFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_TestOutput));
  args.insertOrAssign(ReadRawBinaryFilter::k_ScalarType_Key, std::make_any<NumericType>(scalarType));
  args.insertOrAssign(ReadRawBinaryFilter::k_AdvancedOptions_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadRawBinaryFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{static_cast<float64>(file_size)}}));
  args.insertOrAssign(ReadRawBinaryFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{static_cast<float64>(N)}}));
  args.insertOrAssign(ReadRawBinaryFilter::k_Endian_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<uint64>(endian::little)));
  args.insertOrAssign(ReadRawBinaryFilter::k_SkipHeaderBytes_Key, std::make_any<uint64>(skipBytes));
  args.insertOrAssign(ReadRawBinaryFilter::k_CreatedAttributeArrayPath_Key, k_CreatedArrayPath);

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
  ReadRawBinaryFilter filter;
  Arguments args = CreateFilterArguments(scalarType, N, tupleCount, skipHeaderBytes);

  DataStructure dataStructure;

  // Preflight, get the error condition, and check that there are no errors
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter, check that there are no errors, and compare the data
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
  ReadRawBinaryFilter filter;
  Arguments args = CreateFilterArguments(scalarType, N, dataArraySize, skipHeaderBytes);

  DataStructure dataStructure;

  // Preflight, get the error condition, and check that there are no errors
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  const std::vector<Error>& errors = preflightResult.outputActions.errors();
  REQUIRE(errors.size() == 1);
  REQUIRE(errors[0].code == k_RbrWrongType);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
  ReadRawBinaryFilter filter;
  Arguments args = CreateFilterArguments(scalarType, N, dataArraySize, skipHeaderBytes);

  DataStructure dataStructure;

  // Preflight, get the error condition, and check that there are no errors
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  const std::vector<Error>& errors = preflightResult.outputActions.errors();
  REQUIRE(errors.size() == 1);
  REQUIRE(errors[0].code == k_RbrFileTooSmall);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
  ReadRawBinaryFilter filter;
  Arguments args = CreateFilterArguments(scalarType, N, tupleCount - skipHeaderTuples, skipHeaderBytes);

  DataStructure dataStructure;

  // Preflight, get the error condition, and check that there are no errors
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter, check that there are no errors, and compare the data
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
  ReadRawBinaryFilter filter;
  Arguments args = CreateFilterArguments(scalarType, N, dataArraySize, skipHeaderBytes);

  DataStructure dataStructure;

  // Preflight, get the error condition, and check that there are no errors
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  const std::vector<Error>& errors = preflightResult.outputActions.errors();
  REQUIRE(errors.size() == 1);
  REQUIRE(errors[0].code == k_RbrSkippedTooMuch);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
TEST_CASE("SimplnxCore::ReadRawBinaryFilter(Case1)", "[SimplnxCore][ReadRawBinaryFilter]")
{
  UnitTest::LoadPlugins();

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
TEST_CASE("SimplnxCore::ReadRawBinaryFilter(Case2)", "[SimplnxCore][ReadRawBinaryFilter]")
{
  UnitTest::LoadPlugins();

  // Create the parent directory path
  fs::create_directories(k_TestOutput.parent_path());

  TestCase2_Execute();
}

// Case3: This tests when the wrong component size is chosen. (The total number of scalar elements in the file does not evenly divide by the chosen component size).
TEST_CASE("SimplnxCore::ReadRawBinaryFilter(Case3)", "[SimplnxCore][ReadRawBinaryFilter]")
{
  UnitTest::LoadPlugins();

  // Create the parent directory path
  fs::create_directories(k_TestOutput.parent_path());

  TestCase3_Execute();
}

// Case4: This tests when skipHeaderBytes is non-zero, and checks to see if the data read is the same as the data written.
TEST_CASE("SimplnxCore::ReadRawBinaryFilter(Case4)", "[SimplnxCore][ReadRawBinaryFilter]")
{
  UnitTest::LoadPlugins();

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
TEST_CASE("SimplnxCore::ReadRawBinaryFilter(Case5)", "[SimplnxCore][ReadRawBinaryFilter]")
{
  UnitTest::LoadPlugins();

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

// Case6: Tests placing the output array inside an existing AttributeMatrix with AdvancedOptions disabled
TEST_CASE("SimplnxCore::ReadRawBinaryFilter(Case6_AMPlacement)", "[SimplnxCore][ReadRawBinaryFilter]")
{
  UnitTest::LoadPlugins();
  fs::create_directories(k_TestOutput.parent_path());

  constexpr usize xDim = 10;
  constexpr usize yDim = 20;
  constexpr usize zDim = 5;
  constexpr usize tupleCount = xDim * yDim * zDim;
  constexpr usize numComp = 1;
  constexpr usize dataArraySize = tupleCount * numComp;

  std::vector<int32> exemplaryData(dataArraySize);
  std::iota(exemplaryData.begin(), exemplaryData.end(), static_cast<int32>(0));

  auto fileGuard = MakeScopeGuard([]() noexcept { fs::remove(k_TestOutput); });
  REQUIRE(CreateTestDataFile<int32>(exemplaryData));

  // Create a DataStructure with an ImageGeom and cell data AttributeMatrix
  DataStructure dataStructure;
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, "ImageGeom");
  imageGeom->setDimensions({xDim, yDim, zDim});
  AttributeMatrix* cellAM = AttributeMatrix::Create(dataStructure, "CellData", {zDim, yDim, xDim}, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  DataPath outputPath = DataPath({"ImageGeom", "CellData", "BinaryData"});

  Arguments args;
  args.insertOrAssign(ReadRawBinaryFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_TestOutput));
  args.insertOrAssign(ReadRawBinaryFilter::k_ScalarType_Key, std::make_any<NumericType>(NumericType::int32));
  args.insertOrAssign(ReadRawBinaryFilter::k_AdvancedOptions_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadRawBinaryFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{1.0}}));
  args.insertOrAssign(ReadRawBinaryFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{static_cast<float64>(numComp)}}));
  args.insertOrAssign(ReadRawBinaryFilter::k_Endian_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<uint64>(endian::little)));
  args.insertOrAssign(ReadRawBinaryFilter::k_SkipHeaderBytes_Key, std::make_any<uint64>(0));
  args.insertOrAssign(ReadRawBinaryFilter::k_CreatedAttributeArrayPath_Key, outputPath);

  ReadRawBinaryFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<int32>>(outputPath));
  const auto& createdData = dataStructure.getDataRefAs<DataArray<int32>>(outputPath);
  REQUIRE(createdData.getNumberOfTuples() == tupleCount);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case7: Tests reading a file with multi-dimensional component dimensions (e.g., 3x3 tensor)
TEST_CASE("SimplnxCore::ReadRawBinaryFilter(Case7_MultiCompDims)", "[SimplnxCore][ReadRawBinaryFilter]")
{
  UnitTest::LoadPlugins();
  fs::create_directories(k_TestOutput.parent_path());

  // 3x3 tensor = 9 components per tuple, 100 tuples
  constexpr usize tupleCount = 100;
  constexpr usize numComp = 9; // 3x3
  constexpr usize dataArraySize = tupleCount * numComp;

  std::vector<float32> exemplaryData(dataArraySize);
  std::iota(exemplaryData.begin(), exemplaryData.end(), static_cast<float32>(0));

  auto fileGuard = MakeScopeGuard([]() noexcept { fs::remove(k_TestOutput); });
  REQUIRE(CreateTestDataFile<float32>(exemplaryData));

  ReadRawBinaryFilter filter;
  Arguments args;
  args.insertOrAssign(ReadRawBinaryFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_TestOutput));
  args.insertOrAssign(ReadRawBinaryFilter::k_ScalarType_Key, std::make_any<NumericType>(NumericType::float32));
  args.insertOrAssign(ReadRawBinaryFilter::k_AdvancedOptions_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadRawBinaryFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{static_cast<float64>(tupleCount)}}));
  args.insertOrAssign(ReadRawBinaryFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{3.0, 3.0}}));
  args.insertOrAssign(ReadRawBinaryFilter::k_Endian_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<uint64>(endian::little)));
  args.insertOrAssign(ReadRawBinaryFilter::k_SkipHeaderBytes_Key, std::make_any<uint64>(0));
  args.insertOrAssign(ReadRawBinaryFilter::k_CreatedAttributeArrayPath_Key, k_CreatedArrayPath);

  DataStructure dataStructure;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<float32>>(k_CreatedArrayPath));
  const auto& createdData = dataStructure.getDataRefAs<DataArray<float32>>(k_CreatedArrayPath);
  REQUIRE(createdData.getNumberOfTuples() == tupleCount);
  REQUIRE(createdData.getNumberOfComponents() == numComp);

  const auto& store = createdData.getDataStoreRef();
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case8: Tests that preflight fails when the file is too small for the requested dimensions
TEST_CASE("SimplnxCore::ReadRawBinaryFilter(Case8_FileTooSmall)", "[SimplnxCore][ReadRawBinaryFilter]")
{
  UnitTest::LoadPlugins();
  fs::create_directories(k_TestOutput.parent_path());

  constexpr usize actualTuples = 100;
  constexpr usize requestedTuples = 200;

  std::vector<int32> exemplaryData(actualTuples);
  std::iota(exemplaryData.begin(), exemplaryData.end(), static_cast<int32>(0));

  auto fileGuard = MakeScopeGuard([]() noexcept { fs::remove(k_TestOutput); });
  REQUIRE(CreateTestDataFile<int32>(exemplaryData));

  ReadRawBinaryFilter filter;
  Arguments args = CreateFilterArguments(NumericType::int32, 1, requestedTuples, 0);

  DataStructure dataStructure;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  const std::vector<Error>& errors = preflightResult.outputActions.errors();
  REQUIRE(errors.size() == 1);
  REQUIRE(errors[0].code == k_RbrFileTooSmall);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Case9: Tests that preflight fails when output path is not inside an AttributeMatrix and AdvancedOptions is false
TEST_CASE("SimplnxCore::ReadRawBinaryFilter(Case9_NoAMNoTupleDims)", "[SimplnxCore][ReadRawBinaryFilter]")
{
  UnitTest::LoadPlugins();
  fs::create_directories(k_TestOutput.parent_path());

  std::vector<int32> exemplaryData(100);
  std::iota(exemplaryData.begin(), exemplaryData.end(), static_cast<int32>(0));

  auto fileGuard = MakeScopeGuard([]() noexcept { fs::remove(k_TestOutput); });
  REQUIRE(CreateTestDataFile<int32>(exemplaryData));

  ReadRawBinaryFilter filter;
  Arguments args;
  args.insertOrAssign(ReadRawBinaryFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_TestOutput));
  args.insertOrAssign(ReadRawBinaryFilter::k_ScalarType_Key, std::make_any<NumericType>(NumericType::int32));
  args.insertOrAssign(ReadRawBinaryFilter::k_AdvancedOptions_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadRawBinaryFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{1.0}}));
  args.insertOrAssign(ReadRawBinaryFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{1.0}}));
  args.insertOrAssign(ReadRawBinaryFilter::k_Endian_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<uint64>(endian::little)));
  args.insertOrAssign(ReadRawBinaryFilter::k_SkipHeaderBytes_Key, std::make_any<uint64>(0));
  args.insertOrAssign(ReadRawBinaryFilter::k_CreatedAttributeArrayPath_Key, k_CreatedArrayPath);

  DataStructure dataStructure;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  const std::vector<Error>& errors = preflightResult.outputActions.errors();
  REQUIRE(errors.size() == 1);
  REQUIRE(errors[0].code == -78703);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
