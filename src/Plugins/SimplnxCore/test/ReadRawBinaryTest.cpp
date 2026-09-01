/**
 * @file ReadRawBinaryTest.cpp
 * @brief Tests raw-binary reads, dimension validation, header skips, and output placement.
 *
 * The typed fixtures use enough values to require more than one filter I/O
 * block. The cases compare exact values with and without a skipped header. They
 * also verify scalar-size, component-size, and file-size errors.
 */
#include "SimplnxCore/Filters/ReadRawBinaryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
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

/**
 * @brief Creates common arguments for a typed raw-binary read.
 * @param scalarType Primitive type stored in the file.
 * @param N Number of components in each tuple.
 * @param file_size Number of output tuples.
 * @param skipBytes Number of leading file bytes to omit.
 * @return Configured filter arguments.
 */
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
  args.insertOrAssign(ReadRawBinaryFilter::k_AllowPartialFilling_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadRawBinaryFilter::k_CreatedAttributeArrayPath_Key, k_CreatedArrayPath);

  return args;
}

/**
 * @brief Writes typed exemplar values to the common raw-binary test file.
 * @tparam T Specifies the primitive file value type.
 * @param exemplaryData Values to write in host byte order.
 * @return True if the output stream opened. The helper does not check write status.
 */
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

/**
 * @brief Verifies an exact typed read without a file header.
 * @tparam T Specifies the primitive file value type.
 * @tparam N Specifies the number of components in each tuple.
 * @param scalarType Filter numeric type that corresponds to T.
 */
template <class T, usize N>
void TestCase1_Execute(NumericType scalarType)
{
  constexpr usize tupleCount = 10000000;
  constexpr usize dataArraySize = tupleCount * N;
  constexpr usize skipHeaderBytes = 0;

  std::vector<T> exemplaryData(dataArraySize);
  std::iota(exemplaryData.begin(), exemplaryData.end(), static_cast<T>(0));
  // The scope guard removes the common raw file after this case.
  auto fileGuard = MakeScopeGuard([]() noexcept { fs::remove(k_TestOutput); });
  bool result = CreateTestDataFile<T>(exemplaryData);
  REQUIRE(result);
  ReadRawBinaryFilter filter;
  Arguments args = CreateFilterArguments(scalarType, N, tupleCount, skipHeaderBytes);

  DataStructure dataStructure;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(k_CreatedArrayPath));
  const DataArray<T>& createdData = dataStructure.getDataRefAs<DataArray<T>>(k_CreatedArrayPath);
  const AbstractDataStore<T>& store = createdData.getDataStoreRef();
  bool isSame = true;
  {
    constexpr usize k_BufSize = 1000000;
    std::vector<T> readBuf(std::min(dataArraySize, k_BufSize));
    for(usize start = 0; start < dataArraySize && isSame; start += k_BufSize)
    {
      usize count = std::min(k_BufSize, dataArraySize - start);
      store.copyIntoBuffer(start, nonstd::span<T>(readBuf.data(), count));
      for(usize i = 0; i < count; ++i)
      {
        if(readBuf[i] != exemplaryData[start + i])
        {
          isSame = false;
          break;
        }
      }
    }
  }
  REQUIRE(isSame);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

/**
 * @brief Runs the no-header read case for T with one through three components.
 * @tparam T Specifies the primitive file value type.
 * @param scalarType Filter numeric type that corresponds to T.
 */
template <class T>
void TestCase1_TestPrimitives(NumericType scalarType)
{
  TestCase1_Execute<T, 1>(scalarType);
  TestCase1_Execute<T, 2>(scalarType);
  TestCase1_Execute<T, 3>(scalarType);
}

/**
 * @brief Verifies that a larger selected scalar type reports a short input file.
 *
 * The file has 10,000,001 int8 values. The requested int16 array needs
 * 20,000,002 bytes, so preflight returns the file-too-small error.
 */
void TestCase2_Execute()
{
  constexpr usize dataArraySize = 10000001; // The int16 request needs twice this byte count.
  constexpr usize skipHeaderBytes = 0;
  constexpr usize N = 1;
  constexpr NumericType scalarType = NumericType::int16;

  std::vector<int8> exemplaryData(dataArraySize);
  std::iota(exemplaryData.begin(), exemplaryData.end(), static_cast<int8>(0));
  // The scope guard removes the common raw file after this case.
  auto fileGuard = MakeScopeGuard([]() noexcept { fs::remove(k_TestOutput); });
  bool result = CreateTestDataFile<int8>(exemplaryData);
  REQUIRE(result);
  ReadRawBinaryFilter filter;
  Arguments args = CreateFilterArguments(scalarType, N, dataArraySize, skipHeaderBytes);

  DataStructure dataStructure;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  const std::vector<Error>& errors = preflightResult.outputActions.errors();
  REQUIRE(errors.size() == 1);
  REQUIRE(errors[0].code == k_RbrFileTooSmall);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

/**
 * @brief Verifies that incompatible tuple and component dimensions report a short input file.
 */
void TestCase3_Execute()
{
  constexpr usize dataArraySize = 1000001; // Three components need three times this file value count.
  constexpr usize skipHeaderBytes = 0;
  constexpr usize N = 3;
  constexpr NumericType scalarType = NumericType::int64;

  std::vector<int64> exemplaryData(dataArraySize);
  std::iota(exemplaryData.begin(), exemplaryData.end(), static_cast<int64>(0));
  // The scope guard removes the common raw file after this case.
  auto fileGuard = MakeScopeGuard([]() noexcept { fs::remove(k_TestOutput); });
  bool result = CreateTestDataFile<int64>(exemplaryData);
  REQUIRE(result);
  ReadRawBinaryFilter filter;
  Arguments args = CreateFilterArguments(scalarType, N, dataArraySize, skipHeaderBytes);

  DataStructure dataStructure;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  const std::vector<Error>& errors = preflightResult.outputActions.errors();
  REQUIRE(errors.size() == 1);
  REQUIRE(errors[0].code == k_RbrFileTooSmall);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

/**
 * @brief Verifies an exact typed read after a nonzero header skip.
 * @tparam T Specifies the primitive file value type.
 * @tparam N Specifies the number of components in each tuple.
 * @param scalarType Filter numeric type that corresponds to T.
 */
template <class T, usize N>
void TestCase4_Execute(NumericType scalarType)
{
  constexpr usize tupleCount = 10000000;
  constexpr usize dataArraySize = tupleCount * N;
  constexpr usize skipHeaderTuples = 100;
  constexpr usize skipHeaderBytes = skipHeaderTuples * N * sizeof(T);

  std::vector<T> exemplaryData(dataArraySize);
  std::iota(exemplaryData.begin(), exemplaryData.end(), static_cast<T>(0));
  // The scope guard removes the common raw file after this case.
  auto fileGuard = MakeScopeGuard([]() noexcept { fs::remove(k_TestOutput); });
  bool result = CreateTestDataFile<T>(exemplaryData);
  REQUIRE(result);
  ReadRawBinaryFilter filter;
  Arguments args = CreateFilterArguments(scalarType, N, tupleCount - skipHeaderTuples, skipHeaderBytes);

  DataStructure dataStructure;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  DataArray<T>* createdArray = dataStructure.getDataAs<DataArray<T>>(k_CreatedArrayPath);
  REQUIRE(createdArray != nullptr);
  AbstractDataStore<T>& createdStore = createdArray->getDataStoreRef();

  constexpr usize elementOffset = skipHeaderBytes / sizeof(T);
  bool isSame = true;
  usize size = createdStore.getSize();
  {
    constexpr usize k_BufSize = 1000000;
    std::vector<T> readBuf(std::min(size, k_BufSize));
    for(usize start = 0; start < size && isSame; start += k_BufSize)
    {
      usize count = std::min(k_BufSize, size - start);
      createdStore.copyIntoBuffer(start, nonstd::span<T>(readBuf.data(), count));
      for(usize i = 0; i < count; ++i)
      {
        if(readBuf[i] != exemplaryData[start + i + elementOffset])
        {
          isSame = false;
          break;
        }
      }
    }
  }
  REQUIRE(isSame);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

/**
 * @brief Verifies that skipping the complete file reports no remaining data.
 * @tparam T Specifies the primitive file value type.
 * @tparam N Specifies the number of components in each tuple.
 * @param scalarType Filter numeric type that corresponds to T.
 */
template <class T, usize N>
void TestCase5_Execute(NumericType scalarType)
{
  constexpr usize dataArraySize = 10000000 * N;
  constexpr usize skipHeaderBytes = 10000000 * N * sizeof(T);

  std::vector<T> exemplaryData(dataArraySize);
  std::iota(exemplaryData.begin(), exemplaryData.end(), static_cast<T>(0));
  // The scope guard removes the common raw file after this case.
  auto fileGuard = MakeScopeGuard([]() noexcept { fs::remove(k_TestOutput); });
  bool result = CreateTestDataFile<T>(exemplaryData);
  REQUIRE(result);
  ReadRawBinaryFilter filter;
  Arguments args = CreateFilterArguments(scalarType, N, dataArraySize, skipHeaderBytes);

  DataStructure dataStructure;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  const std::vector<Error>& errors = preflightResult.outputActions.errors();
  REQUIRE(errors.size() == 1);
  REQUIRE(errors[0].code == k_RbrSkippedTooMuch);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

/**
 * @brief Runs the complete-header-skip case for T with one through three components.
 * @tparam T Specifies the primitive file value type.
 * @param scalarType Filter numeric type that corresponds to T.
 */
template <class T>
void TestCase5_TestPrimitives(NumericType scalarType)
{
  TestCase5_Execute<T, 1>(scalarType);
  TestCase5_Execute<T, 2>(scalarType);
  TestCase5_Execute<T, 3>(scalarType);
}

/**
 * @brief Runs the nonzero-header read case for T with one through three components.
 * @tparam T Specifies the primitive file value type.
 * @param scalarType Filter numeric type that corresponds to T.
 */
template <class T>
void TestCase4_TestPrimitives(NumericType scalarType)
{
  TestCase4_Execute<T, 1>(scalarType);
  TestCase4_Execute<T, 2>(scalarType);
  TestCase4_Execute<T, 3>(scalarType);
}
} // namespace

// Case 1 verifies exact typed reads without a header skip.
TEST_CASE("SimplnxCore::ReadRawBinaryFilter(Case1)", "[SimplnxCore][ReadRawBinaryFilter]")
{
  UnitTest::LoadPlugins();
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

// Case 2 verifies a file-too-small error for a larger selected scalar type.
TEST_CASE("SimplnxCore::ReadRawBinaryFilter(Case2)", "[SimplnxCore][ReadRawBinaryFilter]")
{
  UnitTest::LoadPlugins();
  fs::create_directories(k_TestOutput.parent_path());

  TestCase2_Execute();
}

// Case 3 verifies a file-too-small error for incompatible component dimensions.
TEST_CASE("SimplnxCore::ReadRawBinaryFilter(Case3)", "[SimplnxCore][ReadRawBinaryFilter]")
{
  UnitTest::LoadPlugins();
  fs::create_directories(k_TestOutput.parent_path());

  TestCase3_Execute();
}

// Case 4 verifies exact typed reads after a nonzero header skip.
TEST_CASE("SimplnxCore::ReadRawBinaryFilter(Case4)", "[SimplnxCore][ReadRawBinaryFilter]")
{
  UnitTest::LoadPlugins();
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

// Case 5 verifies the error for a header skip equal to the file size.
TEST_CASE("SimplnxCore::ReadRawBinaryFilter(Case5)", "[SimplnxCore][ReadRawBinaryFilter]")
{
  UnitTest::LoadPlugins();
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

// Case 6 places the output in an existing AttributeMatrix without advanced options.
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
  args.insertOrAssign(ReadRawBinaryFilter::k_AllowPartialFilling_Key, std::make_any<bool>(false));
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

// Case 7 reads a file with 3 by 3 component dimensions.
TEST_CASE("SimplnxCore::ReadRawBinaryFilter(Case7_MultiCompDims)", "[SimplnxCore][ReadRawBinaryFilter]")
{
  UnitTest::LoadPlugins();
  fs::create_directories(k_TestOutput.parent_path());

  // The fixture contains 100 tuples with nine components in each tuple.
  constexpr usize tupleCount = 100;
  constexpr usize numComp = 9;
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
  args.insertOrAssign(ReadRawBinaryFilter::k_AllowPartialFilling_Key, std::make_any<bool>(false));
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

// Case 8 verifies that preflight rejects dimensions larger than the input file.
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

// Case 9 requires an AttributeMatrix parent when advanced options are disabled.
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
  args.insertOrAssign(ReadRawBinaryFilter::k_AllowPartialFilling_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadRawBinaryFilter::k_CreatedAttributeArrayPath_Key, k_CreatedArrayPath);

  DataStructure dataStructure;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  const std::vector<Error>& errors = preflightResult.outputActions.errors();
  REQUIRE(errors.size() == 1);
  REQUIRE(errors[0].code == -78703);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ReadRawBinaryFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ReadRawBinaryFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ReadRawBinaryFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ReadRawBinaryFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ReadRawBinaryFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<FileSystemPathParameter::ValueType>(ReadRawBinaryFilter::k_InputFile_Key) == fs::path("/test/path/file.txt"));
      // Successful pipeline loading verifies the NumericTypeParameterConverter value.
      CHECK(args.value<ChoicesParameter::ValueType>(ReadRawBinaryFilter::k_Endian_Key) == 0);
      CHECK(args.value<uint64>(ReadRawBinaryFilter::k_SkipHeaderBytes_Key) == 5);
      // Successful pipeline loading verifies the DataArrayCreationFilterParameterConverter value.
    }
  }
}
