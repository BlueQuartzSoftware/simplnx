#include "OrientationAnalysis/Filters/ReadChannel5DataFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
constexpr usize k_LargeXCells = 8000;
constexpr usize k_LargeYCells = 1000;
constexpr usize k_LargeCellCount = k_LargeXCells * k_LargeYCells;
constexpr usize k_CrcRecordBytes = 26;
constexpr usize k_FileWriteBlockCells = 16384;
constexpr usize k_ValidationChunkCells = 65536;
constexpr int64 k_RowBytes = static_cast<int64>(k_LargeXCells * sizeof(float32));
static_assert(k_CrcRecordBytes == (2 * sizeof(uint8)) + (5 * sizeof(float32)) + sizeof(int32));

template <typename T>
void AppendValue(std::vector<uint8>& buffer, usize& offset, T value)
{
  std::memcpy(buffer.data() + offset, &value, sizeof(T));
  offset += sizeof(T);
}

float32 ExpectedX(usize index)
{
  return static_cast<float32>(index % k_LargeXCells) * 0.25F;
}

float32 ExpectedPhi1(usize index)
{
  return static_cast<float32>(index % 32) * 0.25F;
}

float32 ExpectedPhi(usize index)
{
  return static_cast<float32>(index % 16) * 0.5F;
}

float32 ExpectedPhi2(usize index)
{
  return static_cast<float32>(index % 8);
}

float32 ExpectedMad(usize index)
{
  return static_cast<float32>(index % 16) * 0.5F;
}

uint8 ExpectedBandContrast(usize index)
{
  return static_cast<uint8>(index % 251);
}

int32 ExpectedReliabilityIndex(usize index)
{
  return static_cast<int32>(index % 1000) - 500;
}

int32 ExpectedPhase(usize index)
{
  return static_cast<int32>(index % 3);
}

uint64 SumRepeatingModulo(usize count, usize period)
{
  const uint64 fullCycles = count / period;
  const uint64 remainder = count % period;
  const uint64 periodSum = static_cast<uint64>(period) * static_cast<uint64>(period - 1) / 2;
  const uint64 partialSum = remainder == 0 ? 0 : remainder * (remainder - 1) / 2;
  return fullCycles * periodSum + partialSum;
}

bool WriteSyntheticChannel5Files(const fs::path& cprPath, const fs::path& crcPath)
{
  {
    std::ofstream cprFile(cprPath, std::ios::out | std::ios::trunc);
    if(!cprFile.is_open())
    {
      return false;
    }

    cprFile << "[General]\n"
               "Author=ReadChannel5DataTest\n"
               "JobMode=Grid\n\n"
               "[Job]\n"
               "Magnification=100\n"
               "Coverage=100\n"
               "Device=1\n"
               "kV=20\n"
               "TiltAngle=70\n"
               "TiltAxis=0\n"
               "GridDistX=0.25\n"
               "GridDistY=0.5\n"
               "xCells="
            << k_LargeXCells << "\n"
            << "yCells=" << k_LargeYCells << "\n\n"
            << "[Phases]\n"
               "Count=2\n\n"
               "[Phase1]\n"
               "StructureName=Synthetic Cubic\n"
               "a=1\n"
               "b=1\n"
               "c=1\n"
               "alpha=90\n"
               "beta=90\n"
               "gamma=90\n"
               "LaueGroup=11\n"
               "Reference=Deterministic phase one\n"
               "SpaceGroup=225\n"
               "ID1=1\n"
               "ID2=1\n\n"
               "[Phase2]\n"
               "StructureName=Synthetic Hexagonal\n"
               "a=2\n"
               "b=2\n"
               "c=3\n"
               "alpha=90\n"
               "beta=90\n"
               "gamma=120\n"
               "LaueGroup=9\n"
               "Reference=Deterministic phase two\n"
               "SpaceGroup=194\n"
               "ID1=2\n"
               "ID2=2\n\n"
               "[Fields]\n"
               "Count=7\n"
               "Field1=1\n"
               "Field2=3\n"
               "Field3=4\n"
               "Field4=5\n"
               "Field5=6\n"
               "Field6=7\n"
               "Field7=12\n";

    if(!cprFile.good())
    {
      return false;
    }
  }

  std::ofstream crcFile(crcPath, std::ios::binary | std::ios::trunc);
  if(!crcFile.is_open())
  {
    return false;
  }

  // Write fixed-size blocks so the large fixture does not require a full CRC
  // buffer.
  std::vector<uint8> block(k_FileWriteBlockCells * k_CrcRecordBytes);
  for(usize blockStart = 0; blockStart < k_LargeCellCount; blockStart += k_FileWriteBlockCells)
  {
    const usize blockCellCount = std::min(k_FileWriteBlockCells, k_LargeCellCount - blockStart);
    usize offset = 0;
    for(usize blockIndex = 0; blockIndex < blockCellCount; blockIndex++)
    {
      const usize cellIndex = blockStart + blockIndex;
      AppendValue(block, offset, static_cast<uint8>(ExpectedPhase(cellIndex)));
      AppendValue(block, offset, ExpectedX(cellIndex));
      AppendValue(block, offset, ExpectedPhi1(cellIndex));
      AppendValue(block, offset, ExpectedPhi(cellIndex));
      AppendValue(block, offset, ExpectedPhi2(cellIndex));
      AppendValue(block, offset, ExpectedMad(cellIndex));
      AppendValue(block, offset, ExpectedBandContrast(cellIndex));
      AppendValue(block, offset, ExpectedReliabilityIndex(cellIndex));
    }

    crcFile.write(reinterpret_cast<const char*>(block.data()), static_cast<std::streamsize>(offset));
    if(!crcFile.good())
    {
      return false;
    }
  }

  return true;
}

template <typename T>
const DataArray<T>& RequireDataArray(const DataStructure& dataStructure, const DataPath& path)
{
  const DataArray<T>* array = nullptr;
  REQUIRE_NOTHROW(array = &dataStructure.getDataRefAs<DataArray<T>>(path));
  return *array;
}

template <typename T>
float64 SumDataArray(const DataArray<T>& array)
{
  const auto& store = array.getDataStoreRef();
  // Read fixed pages so validation uses bulk I/O without full materialization.
  auto buffer = std::make_unique<T[]>(k_ValidationChunkCells);
  float64 sum = 0.0;
  for(usize offset = 0; offset < array.getSize(); offset += k_ValidationChunkCells)
  {
    const usize count = std::min(k_ValidationChunkCells, array.getSize() - offset);
    const Result<> readResult = store.copyIntoBuffer(offset, nonstd::span<T>(buffer.get(), count));
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    for(usize index = 0; index < count; index++)
    {
      sum += static_cast<float64>(buffer[index]);
    }
  }
  return sum;
}

std::array<float64, 3> SumEulerComponents(const Float32Array& eulerAngles)
{
  const auto& store = eulerAngles.getDataStoreRef();
  // Read fixed pages so validation uses bulk I/O without full materialization.
  auto buffer = std::make_unique<float32[]>(k_ValidationChunkCells * 3);
  std::array<float64, 3> sums = {0.0, 0.0, 0.0};
  for(usize tupleOffset = 0; tupleOffset < eulerAngles.getNumberOfTuples(); tupleOffset += k_ValidationChunkCells)
  {
    const usize tupleCount = std::min(k_ValidationChunkCells, eulerAngles.getNumberOfTuples() - tupleOffset);
    const Result<> readResult = store.copyIntoBuffer(tupleOffset * 3, nonstd::span<float32>(buffer.get(), tupleCount * 3));
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    for(usize tupleIndex = 0; tupleIndex < tupleCount; tupleIndex++)
    {
      sums[0] += buffer[tupleIndex * 3];
      sums[1] += buffer[tupleIndex * 3 + 1];
      sums[2] += buffer[tupleIndex * 3 + 2];
    }
  }
  return sums;
}
} // namespace

TEST_CASE("OrientationAnalysis::ReadChannel5Data:Native_Data", "[OrientationAnalysis][ReadChannel5Data]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "7_ReadChannel5_Test.tar.gz", "7_ReadChannel5_Test");

  auto exemplarFilePath = fs::path(fmt::format("{}/7_ReadChannel5_Test/7_ReadChannel5_Test.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = LoadDataStructure(exemplarFilePath);

  ReadChannel5DataFilter filter;
  DataStructure dataStructure;
  Arguments args;

  const fs::path inputCtfFile(fmt::format("{}/7_ReadChannel5_Test/17NZ42_Dauphinetwinnedsample_ plaglens.cpr", unit_test::k_TestFilesDir));

  args.insertOrAssign(ReadChannel5DataFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(inputCtfFile));
  args.insertOrAssign(ReadChannel5DataFilter::k_CreateCompatibleArrays_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadChannel5DataFilter::k_EdaxHexagonalAlignment_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadChannel5DataFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
  args.insertOrAssign(ReadChannel5DataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));
  args.insertOrAssign(ReadChannel5DataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_EnsembleAttributeMatrix));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  {
    DataPath exemplarAttributeMatrixPath({"Exemplar-No-Options", "Cell Data"});
    DataPath computedAttributeNatrixPath = k_DataContainerPath.createChildPath(k_Cell_Data);
    CompareExemplarToGenerateAttributeMatrix(exemplarDataStructure, exemplarAttributeMatrixPath, dataStructure, computedAttributeNatrixPath);
  }
  {
    DataPath exemplarAttributeMatrixPath({"Exemplar-No-Options", "Cell Ensemble Data"});
    DataPath computedAttributeNatrixPath = k_DataContainerPath.createChildPath("Cell Ensemble Data");
    CompareExemplarToGenerateAttributeMatrix(exemplarDataStructure, exemplarAttributeMatrixPath, dataStructure, computedAttributeNatrixPath);
  }
  {
    auto* exemplarPtr = exemplarDataStructure.getDataAs<ImageGeom>(DataPath({"Exemplar-All-Options"}));
    auto* computedPtr = dataStructure.getDataAs<ImageGeom>(k_DataContainerPath);

    CompareImageGeometry(exemplarPtr, computedPtr);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ReadChannel5Data:SIMPLNX_Data", "[OrientationAnalysis][ReadChannel5Data]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "7_ReadChannel5_Test.tar.gz", "7_ReadChannel5_Test");

  auto exemplarFilePath = fs::path(fmt::format("{}/7_ReadChannel5_Test/7_ReadChannel5_Test.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = LoadDataStructure(exemplarFilePath);

  ReadChannel5DataFilter filter;
  DataStructure dataStructure;
  Arguments args;

  const fs::path inputCtfFile(fmt::format("{}/7_ReadChannel5_Test/17NZ42_Dauphinetwinnedsample_ plaglens.cpr", unit_test::k_TestFilesDir));

  args.insertOrAssign(ReadChannel5DataFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(inputCtfFile));
  args.insertOrAssign(ReadChannel5DataFilter::k_CreateCompatibleArrays_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadChannel5DataFilter::k_EdaxHexagonalAlignment_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadChannel5DataFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
  args.insertOrAssign(ReadChannel5DataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));
  args.insertOrAssign(ReadChannel5DataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_EnsembleAttributeMatrix));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  {
    DataPath exemplarAttributeMatrixPath({"Exemplar-All-Options", "Cell Data"});
    DataPath computedAttributeNatrixPath = k_DataContainerPath.createChildPath(k_Cell_Data);
    CompareExemplarToGenerateAttributeMatrix(exemplarDataStructure, exemplarAttributeMatrixPath, dataStructure, computedAttributeNatrixPath);
  }
  {
    DataPath exemplarAttributeMatrixPath({"Exemplar-All-Options", "Cell Ensemble Data"});
    DataPath computedAttributeNatrixPath = k_DataContainerPath.createChildPath("Cell Ensemble Data");
    CompareExemplarToGenerateAttributeMatrix(exemplarDataStructure, exemplarAttributeMatrixPath, dataStructure, computedAttributeNatrixPath);
  }

  {
    auto* exemplarPtr = exemplarDataStructure.getDataAs<ImageGeom>(DataPath({"Exemplar-All-Options"}));
    auto* computedPtr = dataStructure.getDataAs<ImageGeom>(k_DataContainerPath);

    CompareImageGeometry(exemplarPtr, computedPtr);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
