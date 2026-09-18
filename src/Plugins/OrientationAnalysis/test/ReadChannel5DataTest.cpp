#include "OrientationAnalysis/Filters/Algorithms/ReadChannel5Data.hpp"
#include "OrientationAnalysis/Filters/ReadChannel5DataFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <EbsdLib/IO/HKL/CtfConstants.h>

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <atomic>
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
constexpr usize k_SyntheticXCells = 65537;
constexpr usize k_SyntheticYCells = 1;
constexpr usize k_CrcRecordBytes = 26;
constexpr usize k_FileWriteBlockCells = 16384;
constexpr usize k_ValidationChunkCells = 65536;
constexpr int32 k_CompatibleWriteError = -925101;
constexpr int32 k_PhaseSentinel = -713;
constexpr float32 k_EulerSentinel = -719.0F;
static_assert(k_CrcRecordBytes == (2 * sizeof(uint8)) + (5 * sizeof(float32)) + sizeof(int32));

template <typename T>
void AppendValue(std::vector<uint8>& buffer, usize& offset, T value)
{
  std::memcpy(buffer.data() + offset, &value, sizeof(T));
  offset += sizeof(T);
}

template <typename T>
class FailOnWriteCallDataStore : public DataStore<T>
{
public:
  FailOnWriteCallDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, T initialValue, usize failingWriteCall, int32 errorCode)
  : DataStore<T>(tupleShape, componentShape, initialValue)
  , m_FailingWriteCall(failingWriteCall)
  , m_ErrorCode(errorCode)
  {
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    m_WriteCalls++;
    if(m_WriteCalls == m_FailingWriteCall)
    {
      return MakeErrorResult(m_ErrorCode, "Injected ReadChannel5Data compatible-output write failure");
    }
    return DataStore<T>::copyFromBuffer(startIndex, buffer);
  }

  usize getWriteCalls() const
  {
    return m_WriteCalls;
  }

private:
  usize m_FailingWriteCall = 0;
  int32 m_ErrorCode = 0;
  usize m_WriteCalls = 0;
};

template <typename T>
class CancelAfterWriteDataStore : public DataStore<T>
{
public:
  CancelAfterWriteDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, T initialValue, std::atomic_bool& shouldCancel)
  : DataStore<T>(tupleShape, componentShape, initialValue)
  , m_ShouldCancel(shouldCancel)
  {
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    m_WriteCalls++;
    Result<> result = DataStore<T>::copyFromBuffer(startIndex, buffer);
    if(result.valid() && !m_DidCancel)
    {
      m_DidCancel = true;
      m_ShouldCancel.store(true);
    }
    return result;
  }

  usize getWriteCalls() const
  {
    return m_WriteCalls;
  }

private:
  std::atomic_bool& m_ShouldCancel;
  bool m_DidCancel = false;
  usize m_WriteCalls = 0;
};

float32 ExpectedX(usize index, usize xCells)
{
  return static_cast<float32>(index % xCells) * 0.25F;
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

bool WriteSyntheticChannel5Files(const fs::path& cprPath, const fs::path& crcPath, usize xCells, usize yCells)
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
            << xCells << "\n"
            << "yCells=" << yCells << "\n\n"
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

  const usize cellCount = xCells * yCells;
  std::vector<uint8> block(k_FileWriteBlockCells * k_CrcRecordBytes);
  for(usize blockStart = 0; blockStart < cellCount; blockStart += k_FileWriteBlockCells)
  {
    const usize blockCellCount = std::min(k_FileWriteBlockCells, cellCount - blockStart);
    usize offset = 0;
    for(usize blockIndex = 0; blockIndex < blockCellCount; blockIndex++)
    {
      const usize cellIndex = blockStart + blockIndex;
      AppendValue(block, offset, static_cast<uint8>(ExpectedPhase(cellIndex)));
      AppendValue(block, offset, ExpectedX(cellIndex, xCells));
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

template <typename T>
T ReadValue(const DataArray<T>& array, usize index)
{
  T value = {};
  auto getDataStoreRefResult = array.getDataStoreRef().copyIntoBuffer(index, nonstd::span<T>(&value, 1));
  SIMPLNX_RESULT_REQUIRE_VALID(getDataStoreRefResult);
  return value;
}

void VerifySyntheticChannel5Output(const DataStructure& dataStructure, bool compatibleArrays)
{
  const DataPath cellDataPath = k_DataContainerPath.createChildPath(k_Cell_Data);
  const auto& imageGeometry = dataStructure.getDataRefAs<ImageGeom>(k_DataContainerPath);
  REQUIRE(imageGeometry.getNumberOfCells() == k_SyntheticXCells * k_SyntheticYCells);

  const auto& xArray = RequireDataArray<float32>(dataStructure, cellDataPath.createChildPath(ebsdlib::Ctf::X));
  const auto& madArray = RequireDataArray<float32>(dataStructure, cellDataPath.createChildPath(ebsdlib::Ctf::MAD));
  const auto& bandContrastArray = RequireDataArray<uint8>(dataStructure, cellDataPath.createChildPath(ebsdlib::Ctf::BC));
  const auto& reliabilityArray = RequireDataArray<int32>(dataStructure, cellDataPath.createChildPath(ebsdlib::Ctf::ReliabilityIndex));
  REQUIRE(xArray.getTupleShape() == ShapeType({1, k_SyntheticYCells, k_SyntheticXCells}));
  REQUIRE(xArray.getComponentShape() == ShapeType({1}));
  REQUIRE(madArray.getComponentShape() == ShapeType({1}));
  REQUIRE(bandContrastArray.getComponentShape() == ShapeType({1}));
  REQUIRE(reliabilityArray.getComponentShape() == ShapeType({1}));
  REQUIRE(xArray.getIDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(madArray.getIDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(bandContrastArray.getIDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(reliabilityArray.getIDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);

  const usize lastTuple = k_SyntheticXCells * k_SyntheticYCells - 1;
  for(const usize tupleIndex : {usize{0}, usize{65535}, lastTuple})
  {
    CAPTURE(tupleIndex);
    REQUIRE(ReadValue(xArray, tupleIndex) == ExpectedX(tupleIndex, k_SyntheticXCells));
    REQUIRE(ReadValue(madArray, tupleIndex) == ExpectedMad(tupleIndex));
    REQUIRE(ReadValue(bandContrastArray, tupleIndex) == ExpectedBandContrast(tupleIndex));
    REQUIRE(ReadValue(reliabilityArray, tupleIndex) == ExpectedReliabilityIndex(tupleIndex));
  }
  const float64 tupleCount = static_cast<float64>(k_SyntheticXCells * k_SyntheticYCells);
  REQUIRE(SumDataArray(xArray) == 0.25 * tupleCount * (tupleCount - 1.0) / 2.0);
  REQUIRE(SumDataArray(madArray) == 0.5 * static_cast<float64>(SumRepeatingModulo(k_SyntheticXCells * k_SyntheticYCells, 16)));
  REQUIRE(SumDataArray(bandContrastArray) == static_cast<float64>(SumRepeatingModulo(k_SyntheticXCells * k_SyntheticYCells, 251)));
  REQUIRE(SumDataArray(reliabilityArray) == static_cast<float64>(SumRepeatingModulo(k_SyntheticXCells * k_SyntheticYCells, 1000)) - 500.0 * tupleCount);

  if(!compatibleArrays)
  {
    const auto& phaseArray = RequireDataArray<uint8>(dataStructure, cellDataPath.createChildPath(ebsdlib::Ctf::Phase));
    const auto& phi1Array = RequireDataArray<float32>(dataStructure, cellDataPath.createChildPath(ebsdlib::Ctf::phi1));
    const auto& phiArray = RequireDataArray<float32>(dataStructure, cellDataPath.createChildPath(ebsdlib::Ctf::Phi));
    const auto& phi2Array = RequireDataArray<float32>(dataStructure, cellDataPath.createChildPath(ebsdlib::Ctf::phi2));
    REQUIRE(phaseArray.getComponentShape() == ShapeType({1}));
    REQUIRE(phi1Array.getComponentShape() == ShapeType({1}));
    REQUIRE(phiArray.getComponentShape() == ShapeType({1}));
    REQUIRE(phi2Array.getComponentShape() == ShapeType({1}));
    REQUIRE(phaseArray.getIDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
    REQUIRE(phi1Array.getIDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
    REQUIRE(phiArray.getIDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
    REQUIRE(phi2Array.getIDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
    for(const usize tupleIndex : {usize{0}, usize{65535}, lastTuple})
    {
      CAPTURE(tupleIndex);
      REQUIRE(ReadValue(phaseArray, tupleIndex) == static_cast<uint8>(ExpectedPhase(tupleIndex)));
      REQUIRE(ReadValue(phi1Array, tupleIndex) == ExpectedPhi1(tupleIndex));
      REQUIRE(ReadValue(phiArray, tupleIndex) == ExpectedPhi(tupleIndex));
      REQUIRE(ReadValue(phi2Array, tupleIndex) == ExpectedPhi2(tupleIndex));
    }
    REQUIRE(SumDataArray(phaseArray) == static_cast<float64>(SumRepeatingModulo(k_SyntheticXCells * k_SyntheticYCells, 3)));
    REQUIRE(SumDataArray(phi1Array) == 0.25 * static_cast<float64>(SumRepeatingModulo(k_SyntheticXCells * k_SyntheticYCells, 32)));
    REQUIRE(SumDataArray(phiArray) == 0.5 * static_cast<float64>(SumRepeatingModulo(k_SyntheticXCells * k_SyntheticYCells, 16)));
    REQUIRE(SumDataArray(phi2Array) == static_cast<float64>(SumRepeatingModulo(k_SyntheticXCells * k_SyntheticYCells, 8)));
    return;
  }

  const auto& phasesArray = RequireDataArray<int32>(dataStructure, cellDataPath.createChildPath(ebsdlib::CtfFile::Phases));
  const auto& eulerArray = RequireDataArray<float32>(dataStructure, cellDataPath.createChildPath(ebsdlib::CtfFile::EulerAngles));
  REQUIRE(phasesArray.getComponentShape() == ShapeType({1}));
  REQUIRE(eulerArray.getComponentShape() == ShapeType({3}));
  REQUIRE(phasesArray.getIDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(eulerArray.getIDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(dataStructure.getDataAs<UInt8Array>(cellDataPath.createChildPath(ebsdlib::Ctf::Phase)) == nullptr);
  REQUIRE(dataStructure.getDataAs<Float32Array>(cellDataPath.createChildPath(ebsdlib::Ctf::phi1)) == nullptr);
  REQUIRE(dataStructure.getDataAs<Float32Array>(cellDataPath.createChildPath(ebsdlib::Ctf::Phi)) == nullptr);
  REQUIRE(dataStructure.getDataAs<Float32Array>(cellDataPath.createChildPath(ebsdlib::Ctf::phi2)) == nullptr);
  for(const usize tupleIndex : {usize{0}, usize{65535}, lastTuple})
  {
    CAPTURE(tupleIndex);
    REQUIRE(ReadValue(phasesArray, tupleIndex) == ExpectedPhase(tupleIndex));
    REQUIRE(ReadValue(eulerArray, tupleIndex * 3) == ExpectedPhi1(tupleIndex));
    REQUIRE(ReadValue(eulerArray, tupleIndex * 3 + 1) == ExpectedPhi(tupleIndex));
    REQUIRE(ReadValue(eulerArray, tupleIndex * 3 + 2) == ExpectedPhi2(tupleIndex));
  }
  REQUIRE(SumDataArray(phasesArray) == static_cast<float64>(SumRepeatingModulo(k_SyntheticXCells * k_SyntheticYCells, 3)));
  const auto eulerSums = SumEulerComponents(eulerArray);
  REQUIRE(eulerSums[0] == 0.25 * static_cast<float64>(SumRepeatingModulo(k_SyntheticXCells * k_SyntheticYCells, 32)));
  REQUIRE(eulerSums[1] == 0.5 * static_cast<float64>(SumRepeatingModulo(k_SyntheticXCells * k_SyntheticYCells, 16)));
  REQUIRE(eulerSums[2] == static_cast<float64>(SumRepeatingModulo(k_SyntheticXCells * k_SyntheticYCells, 8)));
}

Arguments SyntheticChannel5Arguments(const fs::path& cprPath, bool compatibleArrays)
{
  Arguments args;
  args.insertOrAssign(ReadChannel5DataFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(cprPath));
  args.insertOrAssign(ReadChannel5DataFilter::k_CreateCompatibleArrays_Key, std::make_any<bool>(compatibleArrays));
  args.insertOrAssign(ReadChannel5DataFilter::k_EdaxHexagonalAlignment_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadChannel5DataFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
  args.insertOrAssign(ReadChannel5DataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));
  args.insertOrAssign(ReadChannel5DataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_EnsembleAttributeMatrix));
  return args;
}

ReadChannel5DataInputValues SyntheticChannel5InputValues(const fs::path& cprPath)
{
  ReadChannel5DataInputValues inputValues;
  inputValues.InputFile = cprPath;
  inputValues.DataContainerName = k_DataContainerPath;
  inputValues.CellAttributeMatrixName = k_Cell_Data;
  inputValues.CellEnsembleAttributeMatrixName = k_EnsembleAttributeMatrix;
  inputValues.EdaxHexagonalAlignment = false;
  inputValues.CreateCompatibleArrays = true;
  return inputValues;
}

void ApplySyntheticChannel5RegularActions(DataStructure& dataStructure, const fs::path& cprPath)
{
  ReadChannel5DataFilter filter;
  const auto preflightResult = filter.preflight(dataStructure, SyntheticChannel5Arguments(cprPath, true));
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  // Preserve raw Phase and Euler components for the public algorithm conversion.
  auto valueResult = preflightResult.outputActions.value().applyRegular(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(valueResult);
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

TEST_CASE("OrientationAnalysis::ReadChannel5Data:Synthetic_BatchBoundaries", "[OrientationAnalysis][ReadChannel5Data]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);

  const fs::path cprPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadChannel5Data_Synthetic.cpr";
  const fs::path crcPath = cprPath.parent_path() / "ReadChannel5Data_Synthetic.crc";
  const auto fileGuard = MakeScopeGuard([&cprPath, &crcPath]() noexcept {
    std::error_code errorCode;
    fs::remove(cprPath, errorCode);
    fs::remove(crcPath, errorCode);
  });
  REQUIRE(WriteSyntheticChannel5Files(cprPath, crcPath, k_SyntheticXCells, k_SyntheticYCells));

  for(const bool compatibleArrays : {false, true})
  {
    CAPTURE(compatibleArrays);
    DataStructure dataStructure;
    ReadChannel5DataFilter filter;
    Arguments args;
    args.insertOrAssign(ReadChannel5DataFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(cprPath));
    args.insertOrAssign(ReadChannel5DataFilter::k_CreateCompatibleArrays_Key, std::make_any<bool>(compatibleArrays));
    args.insertOrAssign(ReadChannel5DataFilter::k_EdaxHexagonalAlignment_Key, std::make_any<bool>(false));
    args.insertOrAssign(ReadChannel5DataFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insertOrAssign(ReadChannel5DataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));
    args.insertOrAssign(ReadChannel5DataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_EnsembleAttributeMatrix));

    auto preflightResult2 = filter.preflight(dataStructure, args).outputActions;
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult2);
    auto executeResult2 = filter.execute(dataStructure, args).result;
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult2);
    VerifySyntheticChannel5Output(dataStructure, compatibleArrays);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("OrientationAnalysis::ReadChannel5Data:Synthetic_CompatibleOutputFailureAndCancellation", "[OrientationAnalysis][ReadChannel5Data]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);

  const fs::path cprPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadChannel5Data_CompatibleOutput.cpr";
  const fs::path crcPath = cprPath.parent_path() / "ReadChannel5Data_CompatibleOutput.crc";
  const auto fileGuard = MakeScopeGuard([&cprPath, &crcPath]() noexcept {
    std::error_code errorCode;
    fs::remove(cprPath, errorCode);
    fs::remove(crcPath, errorCode);
  });
  REQUIRE(WriteSyntheticChannel5Files(cprPath, crcPath, k_SyntheticXCells, k_SyntheticYCells));

  const DataPath cellDataPath = k_DataContainerPath.createChildPath(k_Cell_Data);

  SECTION("The compatible Phases second-page write propagates its error and leaves EulerAngles untouched")
  {
    DataStructure dataStructure;
    ApplySyntheticChannel5RegularActions(dataStructure, cprPath);
    auto* phasesArray = dataStructure.getDataAs<Int32Array>(cellDataPath.createChildPath(ebsdlib::CtfFile::Phases));
    auto* eulerArray = dataStructure.getDataAs<Float32Array>(cellDataPath.createChildPath(ebsdlib::CtfFile::EulerAngles));
    REQUIRE(phasesArray != nullptr);
    REQUIRE(eulerArray != nullptr);
    auto phasesStore = std::make_shared<FailOnWriteCallDataStore<int32>>(phasesArray->getTupleShape(), phasesArray->getComponentShape(), k_PhaseSentinel, 2, k_CompatibleWriteError);
    auto eulerStore = std::make_shared<DataStore<float32>>(eulerArray->getTupleShape(), eulerArray->getComponentShape(), k_EulerSentinel);
    auto setDataStoreResult4 = phasesArray->setDataStore(phasesStore);
    SIMPLNX_RESULT_REQUIRE_VALID(setDataStoreResult4);
    auto setDataStoreResult3 = eulerArray->setDataStore(eulerStore);
    SIMPLNX_RESULT_REQUIRE_VALID(setDataStoreResult3);

    std::atomic_bool shouldCancel = false;
    const IFilter::MessageHandler messageHandler = {};
    ReadChannel5DataInputValues inputValues = SyntheticChannel5InputValues(cprPath);
    const Result<> result = ReadChannel5Data(dataStructure, messageHandler, shouldCancel, &inputValues)();
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == k_CompatibleWriteError);
    REQUIRE(phasesStore->getWriteCalls() == 2);
    REQUIRE(phasesStore->getValue(0) == ExpectedPhase(0));
    REQUIRE(phasesStore->getValue(65535) == ExpectedPhase(65535));
    REQUIRE(phasesStore->getValue(65536) == k_PhaseSentinel);
    REQUIRE(eulerStore->getValue(0) == k_EulerSentinel);
    REQUIRE(eulerStore->getValue(eulerStore->getSize() - 1) == k_EulerSentinel);
  }

  SECTION("Cancellation after the compatible Phases first page preserves later output sentinels")
  {
    DataStructure dataStructure;
    ApplySyntheticChannel5RegularActions(dataStructure, cprPath);
    auto* phasesArray = dataStructure.getDataAs<Int32Array>(cellDataPath.createChildPath(ebsdlib::CtfFile::Phases));
    auto* eulerArray = dataStructure.getDataAs<Float32Array>(cellDataPath.createChildPath(ebsdlib::CtfFile::EulerAngles));
    REQUIRE(phasesArray != nullptr);
    REQUIRE(eulerArray != nullptr);

    std::atomic_bool shouldCancel = false;
    auto phasesStore = std::make_shared<CancelAfterWriteDataStore<int32>>(phasesArray->getTupleShape(), phasesArray->getComponentShape(), k_PhaseSentinel, shouldCancel);
    auto eulerStore = std::make_shared<DataStore<float32>>(eulerArray->getTupleShape(), eulerArray->getComponentShape(), k_EulerSentinel);
    auto setDataStoreResult2 = phasesArray->setDataStore(phasesStore);
    SIMPLNX_RESULT_REQUIRE_VALID(setDataStoreResult2);
    auto setDataStoreResult = eulerArray->setDataStore(eulerStore);
    SIMPLNX_RESULT_REQUIRE_VALID(setDataStoreResult);

    const IFilter::MessageHandler messageHandler = {};
    ReadChannel5DataInputValues inputValues = SyntheticChannel5InputValues(cprPath);
    const Result<> result = ReadChannel5Data(dataStructure, messageHandler, shouldCancel, &inputValues)();
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(shouldCancel.load());
    REQUIRE(phasesStore->getWriteCalls() == 1);
    REQUIRE(phasesStore->getValue(0) == ExpectedPhase(0));
    REQUIRE(phasesStore->getValue(65535) == ExpectedPhase(65535));
    REQUIRE(phasesStore->getValue(65536) == k_PhaseSentinel);
    REQUIRE(eulerStore->getValue(0) == k_EulerSentinel);
    REQUIRE(eulerStore->getValue(eulerStore->getSize() - 1) == k_EulerSentinel);
  }
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
  args.insertOrAssign(ReadChannel5DataFilter::k_CreateCompatibleArrays_Key, std::make_any<bool>(true));
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
