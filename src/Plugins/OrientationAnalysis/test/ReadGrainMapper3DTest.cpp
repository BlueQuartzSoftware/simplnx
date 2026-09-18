#include <catch2/catch.hpp>

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ReadGrainMapper3D.hpp"
#include "OrientationAnalysis/Filters/ReadGrainMapper3DFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "H5Support/H5Lite.h"
#include "H5Support/H5Utilities.h"

#include <array>
#include <atomic>
#include <cmath>
#include <filesystem>
#include <vector>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
const std::string k_LabDCTGeometryName("LabDCT");
const std::string k_AbsorptionCTGeometryName("AbsorptionCT");
const std::string k_CellAttributeMatrixName("Cell Data");
const std::string k_CellEnsembleAttributeMatrixName("Cell Ensemble Data");

struct GrainMapperFixtureSpec
{
  std::array<usize, 3> dimensions = {1, 1, 1}; // X, Y, Z
  usize rodriguesComponentCount = 3;
  usize phaseTupleCount = 0;
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

template <typename T>
std::vector<T> ReadAllValues(const DataArray<T>& array)
{
  std::vector<T> values(array.getSize());
  auto arrayReadResult = array.getDataStoreRef().copyIntoBuffer(0, nonstd::span<T>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(arrayReadResult);
  return values;
}

template <typename T>
void WriteVector(hid_t groupId, const std::string& name, const std::vector<hsize_t>& dimensions, const std::vector<T>& values)
{
  REQUIRE(H5Support::H5Lite::writeVectorDataset<T>(groupId, name, dimensions, values) >= 0);
}

hid_t OpenOrCreateGroup(hid_t fileId, const std::string& path)
{
  REQUIRE(H5Support::H5Utilities::createGroupsFromPath(path, fileId) >= 0);
  const hid_t groupId = H5Gopen(fileId, path.c_str(), H5P_DEFAULT);
  REQUIRE(groupId >= 0);
  return groupId;
}

float32 RodriguesValue(usize tupleIndex, usize componentIndex)
{
  return static_cast<float32>(1 + ((tupleIndex + componentIndex * 3) % 7));
}

uint8 IpfValue(usize directionIndex, usize tupleIndex, usize componentIndex)
{
  return static_cast<uint8>((directionIndex * 79 + tupleIndex * 17 + componentIndex * 61) % 255);
}

uint8 ConvertedIpfValue(usize directionIndex, usize tupleIndex, usize componentIndex)
{
  const float32 source = static_cast<float32>(IpfValue(directionIndex, tupleIndex, componentIndex)) / 255.0F;
  return static_cast<uint8>(source * 255.0F);
}

float32 QuaternionValue(usize tupleIndex, usize componentIndex)
{
  return static_cast<float32>(tupleIndex * 4 + componentIndex + 1) * 0.25F;
}

fs::path WriteGrainMapperFixture(const std::string& fileName, const GrainMapperFixtureSpec& spec)
{
  const fs::path filePath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / fileName;
  const hid_t fileId = H5Fcreate(filePath.string().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  REQUIRE(fileId >= 0);

  const auto fileGuard = MakeScopeGuard([fileId]() noexcept { H5Fclose(fileId); });
  const usize tupleCount = spec.dimensions[0] * spec.dimensions[1] * spec.dimensions[2];
  const usize phaseTupleCount = spec.phaseTupleCount == 0 ? tupleCount : spec.phaseTupleCount;
  const std::vector<hsize_t> scalarDimensions = {spec.dimensions[2], spec.dimensions[1], spec.dimensions[0]};

  const hid_t labDctGroupId = OpenOrCreateGroup(fileId, "LabDCT");
  const auto labDctGroupGuard = MakeScopeGuard([labDctGroupId]() noexcept { H5Gclose(labDctGroupId); });
  WriteVector<float64>(labDctGroupId, "Extent", {3}, {static_cast<float64>(spec.dimensions[0]), static_cast<float64>(spec.dimensions[1]), static_cast<float64>(spec.dimensions[2])});
  WriteVector<float64>(labDctGroupId, "Spacing", {3}, {1.0, 1.0, 1.0});
  WriteVector<float64>(labDctGroupId, "Center", {3}, {0.0, 0.0, 0.0});
  WriteVector<float64>(labDctGroupId, "VirtualShift", {3}, {0.0, 0.0, 0.0});

  const hid_t phaseGroupId = OpenOrCreateGroup(fileId, "PhaseInfo/Phase01");
  const auto phaseGroupGuard = MakeScopeGuard([phaseGroupId]() noexcept { H5Gclose(phaseGroupId); });
  REQUIRE(H5Support::H5Lite::writeStringDataset(phaseGroupId, "Name", "Synthetic Phase") >= 0);
  REQUIRE(H5Support::H5Lite::writeStringDataset(phaseGroupId, "UniversalHermannMauguin", "P 1") >= 0);
  REQUIRE(H5Support::H5Lite::writeScalarDataset<int32>(phaseGroupId, "SpaceGroup", 1) >= 0);
  WriteVector<float64>(phaseGroupId, "UnitCell", {6}, {1.0, 1.0, 1.0, 90.0, 90.0, 90.0});

  const hid_t dataGroupId = OpenOrCreateGroup(fileId, "LabDCT/Data");
  const auto dataGroupGuard = MakeScopeGuard([dataGroupId]() noexcept { H5Gclose(dataGroupId); });
  std::vector<float32> completeness(tupleCount);
  std::vector<uint8> phases(phaseTupleCount);
  std::vector<float32> rodrigues(tupleCount * spec.rodriguesComponentCount);
  std::vector<float32> ipf001(tupleCount * 3);
  std::vector<float32> ipf010(tupleCount * 3);
  std::vector<float32> ipf100(tupleCount * 3);
  std::vector<float32> quaternions(tupleCount * 4);
  for(usize tupleIndex = 0; tupleIndex < tupleCount; tupleIndex++)
  {
    completeness[tupleIndex] = static_cast<float32>(tupleIndex);
    for(usize componentIndex = 0; componentIndex < spec.rodriguesComponentCount; componentIndex++)
    {
      rodrigues[tupleIndex * spec.rodriguesComponentCount + componentIndex] = RodriguesValue(tupleIndex, componentIndex);
    }
    for(usize componentIndex = 0; componentIndex < 3; componentIndex++)
    {
      ipf001[tupleIndex * 3 + componentIndex] = static_cast<float32>(IpfValue(0, tupleIndex, componentIndex)) / 255.0F;
      ipf010[tupleIndex * 3 + componentIndex] = static_cast<float32>(IpfValue(1, tupleIndex, componentIndex)) / 255.0F;
      ipf100[tupleIndex * 3 + componentIndex] = static_cast<float32>(IpfValue(2, tupleIndex, componentIndex)) / 255.0F;
    }
    for(usize componentIndex = 0; componentIndex < 4; componentIndex++)
    {
      quaternions[tupleIndex * 4 + componentIndex] = QuaternionValue(tupleIndex, componentIndex);
    }
  }
  for(usize tupleIndex = 0; tupleIndex < phaseTupleCount; tupleIndex++)
  {
    phases[tupleIndex] = static_cast<uint8>(tupleIndex % 251);
  }

  WriteVector<float32>(dataGroupId, "Completeness", scalarDimensions, completeness);
  const std::vector<hsize_t> phaseDimensions = phaseTupleCount == tupleCount ? scalarDimensions : std::vector<hsize_t>{1, 1, phaseTupleCount};
  WriteVector<uint8>(dataGroupId, "PhaseId", phaseDimensions, phases);
  auto vectorDimensions = scalarDimensions;
  vectorDimensions.push_back(spec.rodriguesComponentCount);
  WriteVector<float32>(dataGroupId, "Rodrigues", vectorDimensions, rodrigues);
  vectorDimensions.back() = 3;
  WriteVector<float32>(dataGroupId, "IPF001", vectorDimensions, ipf001);
  WriteVector<float32>(dataGroupId, "IPF010", vectorDimensions, ipf010);
  WriteVector<float32>(dataGroupId, "IPF100", vectorDimensions, ipf100);
  vectorDimensions.back() = 4;
  WriteVector<float32>(dataGroupId, "Quaternion", vectorDimensions, quaternions);

  return filePath;
}

Arguments GrainMapperArguments(const fs::path& filePath, const DataPath& geometryPath)
{
  Arguments args;
  args.insertOrAssign(ReadGrainMapper3DFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(filePath));
  args.insertOrAssign(ReadGrainMapper3DFilter::k_ReadLabDCT_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadGrainMapper3DFilter::k_CreatedDCTImageGeometryPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(ReadGrainMapper3DFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_CellAttributeMatrixName));
  args.insertOrAssign(ReadGrainMapper3DFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_CellEnsembleAttributeMatrixName));
  args.insertOrAssign(ReadGrainMapper3DFilter::k_ConvertPhaseToInt32_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadGrainMapper3DFilter::k_ConvertOrientationData_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadGrainMapper3DFilter::k_ConvertIPFColorData_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadGrainMapper3DFilter::k_ReadAbsorptionCT_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadGrainMapper3DFilter::k_CreatedAbsorptionGeometryPath_Key, std::make_any<DataPath>(DataPath({"Unused Absorption"})));
  args.insertOrAssign(ReadGrainMapper3DFilter::k_CellAbsorptionAttributeMatrixName_Key, std::make_any<std::string>(k_CellAttributeMatrixName));
  return args;
}

ReadGrainMapper3DInputValues GrainMapperInputValues(const fs::path& filePath, const DataPath& geometryPath)
{
  ReadGrainMapper3DInputValues inputValues;
  inputValues.InputFile = filePath;
  inputValues.ReadDctData = true;
  inputValues.DctImageGeometryPath = geometryPath;
  inputValues.DctCellAttributeMatrixName = k_CellAttributeMatrixName;
  inputValues.DctCellEnsembleAttributeMatrixName = k_CellEnsembleAttributeMatrixName;
  inputValues.ConvertPhaseData = true;
  inputValues.ConvertOrientationData = true;
  inputValues.ConvertIPFColors = true;
  inputValues.ReadAbsorptionData = false;
  inputValues.AbsorptionImageGeometryPath = DataPath({"Unused Absorption"});
  inputValues.AbsorptionCellAttributeMatrixName = k_CellAttributeMatrixName;
  return inputValues;
}

void ApplyGrainMapperRegularActions(DataStructure& dataStructure, const fs::path& filePath, const DataPath& geometryPath)
{
  ReadGrainMapper3DFilter filter;
  const auto preflightResult = filter.preflight(dataStructure, GrainMapperArguments(filePath, geometryPath));
  auto assertionResult = preflightResult.outputActions;
  SIMPLNX_RESULT_REQUIRE_VALID(assertionResult);
  auto valueResult = preflightResult.outputActions.value().applyRegular(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(valueResult);
}

template <typename T>
const DataArray<T>& RequireDataArray(const DataStructure& dataStructure, const DataPath& path)
{
  const DataArray<T>* array = nullptr;
  REQUIRE_NOTHROW(array = &dataStructure.getDataRefAs<DataArray<T>>(path));
  return *array;
}

void VerifyGrainMapperOutput(const DataStructure& dataStructure, const DataPath& cellDataPath, const ShapeType& expectedTupleShape)
{
  const auto& phaseArray = RequireDataArray<int32>(dataStructure, cellDataPath.createChildPath("PhaseId"));
  const auto& rodArray = RequireDataArray<float32>(dataStructure, cellDataPath.createChildPath("Rodrigues"));
  const auto& ipf001Array = RequireDataArray<uint8>(dataStructure, cellDataPath.createChildPath("IPF001"));
  const auto& ipf010Array = RequireDataArray<uint8>(dataStructure, cellDataPath.createChildPath("IPF010"));
  const auto& ipf100Array = RequireDataArray<uint8>(dataStructure, cellDataPath.createChildPath("IPF100"));
  const auto& quatArray = RequireDataArray<float32>(dataStructure, cellDataPath.createChildPath("Quaternion"));
  REQUIRE(phaseArray.getTupleShape() == expectedTupleShape);
  REQUIRE(phaseArray.getComponentShape() == ShapeType({1}));
  REQUIRE(rodArray.getComponentShape() == ShapeType({4}));
  REQUIRE(ipf001Array.getComponentShape() == ShapeType({3}));
  REQUIRE(ipf010Array.getComponentShape() == ShapeType({3}));
  REQUIRE(ipf100Array.getComponentShape() == ShapeType({3}));
  REQUIRE(quatArray.getComponentShape() == ShapeType({4}));
  REQUIRE(phaseArray.getIDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(rodArray.getIDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(ipf001Array.getIDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(ipf010Array.getIDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(ipf100Array.getIDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(quatArray.getIDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);

  const auto phases = ReadAllValues(phaseArray);
  const auto rodrigues = ReadAllValues(rodArray);
  const auto ipf001Colors = ReadAllValues(ipf001Array);
  const auto ipf010Colors = ReadAllValues(ipf010Array);
  const auto ipf100Colors = ReadAllValues(ipf100Array);
  const auto quaternions = ReadAllValues(quatArray);
  REQUIRE(rodrigues.size() == phases.size() * 4);
  REQUIRE(ipf001Colors.size() == phases.size() * 3);
  REQUIRE(ipf010Colors.size() == phases.size() * 3);
  REQUIRE(ipf100Colors.size() == phases.size() * 3);
  REQUIRE(quaternions.size() == phases.size() * 4);
  for(usize tupleIndex = 0; tupleIndex < phases.size(); tupleIndex++)
  {
    CAPTURE(tupleIndex);
    REQUIRE(phases[tupleIndex] == static_cast<int32>(tupleIndex % 251));
    const float32 r0 = -RodriguesValue(tupleIndex, 0);
    const float32 r1 = -RodriguesValue(tupleIndex, 1);
    const float32 r2 = -RodriguesValue(tupleIndex, 2);
    const float32 length = std::sqrt(r0 * r0 + r1 * r1 + r2 * r2);
    REQUIRE(rodrigues[tupleIndex * 4] == r0 / length);
    REQUIRE(rodrigues[tupleIndex * 4 + 1] == r1 / length);
    REQUIRE(rodrigues[tupleIndex * 4 + 2] == r2 / length);
    REQUIRE(rodrigues[tupleIndex * 4 + 3] == length);
    for(usize componentIndex = 0; componentIndex < 3; componentIndex++)
    {
      REQUIRE(ipf001Colors[tupleIndex * 3 + componentIndex] == ConvertedIpfValue(0, tupleIndex, componentIndex));
      REQUIRE(ipf010Colors[tupleIndex * 3 + componentIndex] == ConvertedIpfValue(1, tupleIndex, componentIndex));
      REQUIRE(ipf100Colors[tupleIndex * 3 + componentIndex] == ConvertedIpfValue(2, tupleIndex, componentIndex));
    }
    REQUIRE(quaternions[tupleIndex * 4] == -QuaternionValue(tupleIndex, 1));
    REQUIRE(quaternions[tupleIndex * 4 + 1] == -QuaternionValue(tupleIndex, 2));
    REQUIRE(quaternions[tupleIndex * 4 + 2] == -QuaternionValue(tupleIndex, 3));
    REQUIRE(quaternions[tupleIndex * 4 + 3] == QuaternionValue(tupleIndex, 0));
  }
}

} // namespace

TEST_CASE("OrientationAnalysis::ReadGrainMapper3D:Default_Parameters", "[OrientationAnalysis][ReadGrainMapper3D]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "GrainMapper3D_Test_Files.tar.gz", "GrainMapper3D_Test_Files");

  auto exemplarFilePath = fs::path(fmt::format("{}/GrainMapper3D_Test_Files/7_0_SimulatedMultiPhase.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  DataPath computedDCTGeometryPath({fmt::format("{} computed", k_LabDCTGeometryName)});
  DataPath computedAbsorptionCTGeometryPath({fmt::format("{} computed", k_AbsorptionCTGeometryName)});

  DataPath exemplarDCTGeometryPath({fmt::format("{} (default)", k_LabDCTGeometryName)});
  DataPath exemplarAbsorptionCTGeometryPath({fmt::format("{} (default)", k_AbsorptionCTGeometryName)});

  auto inputGM3DFilePath = fs::path(fmt::format("{}/GrainMapper3D_Test_Files/SimulatedMultiPhase.h5", unit_test::k_TestFilesDir));
  {
    ReadGrainMapper3DFilter filter;
    Arguments args;

    args.insertOrAssign(ReadGrainMapper3DFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputGM3DFilePath)));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_ReadLabDCT_Key, std::make_any<bool>(true));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_CreatedDCTImageGeometryPath_Key, std::make_any<DataPath>(computedDCTGeometryPath));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Ensemble_Data));

    args.insertOrAssign(ReadGrainMapper3DFilter::k_ConvertPhaseToInt32_Key, std::make_any<bool>(true));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_ConvertOrientationData_Key, std::make_any<bool>(true));

    args.insertOrAssign(ReadGrainMapper3DFilter::k_ReadAbsorptionCT_Key, std::make_any<bool>(true));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_CreatedAbsorptionGeometryPath_Key, std::make_any<DataPath>(computedAbsorptionCTGeometryPath));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_CellAbsorptionAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));

    auto preflightResult = filter.preflight(dataStructure, args);
    auto assertionResult2 = preflightResult.outputActions;
    SIMPLNX_RESULT_REQUIRE_VALID(assertionResult2);

    auto executeResult = filter.execute(dataStructure, args);
    auto assertionResult3 = executeResult.result;
    SIMPLNX_RESULT_REQUIRE_VALID(assertionResult3);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/read_grainmapper_3d_default.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CompareImageGeometry(dataStructure, exemplarDCTGeometryPath, computedDCTGeometryPath);
  UnitTest::CompareImageGeometry(dataStructure, exemplarAbsorptionCTGeometryPath, computedAbsorptionCTGeometryPath);

  UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarDCTGeometryPath.createChildPath(k_Cell_Data), dataStructure, computedDCTGeometryPath.createChildPath(k_Cell_Data));
  UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarAbsorptionCTGeometryPath.createChildPath(k_Cell_Data), dataStructure,
                                                     computedAbsorptionCTGeometryPath.createChildPath(k_Cell_Data));
  UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarDCTGeometryPath.createChildPath(k_Cell_Ensemble_Data), dataStructure,
                                                     computedDCTGeometryPath.createChildPath(k_Cell_Ensemble_Data));

  // The exemplar predates UniversalHermannMauguin and cannot validate that
  // array. Compare parsed values with known phase metadata. Index zero is the
  // reserved invalid phase.
  {
    const std::vector<std::string> expectedHermannMauguin = {
        "Invalid Phase",    "F d -3 m :2 (a-1/8,b-1/8,c-1/8)", "F d -3 m :2 (a-1/8,b-1/8,c-1/8)", "P 63/m m c", "P 42/m n m", "R -3 c :H (-y+z,x+z,-x+y+z)", "P 32 2 1", "P b c n", "C 1 2/m 1",
        "P -1 (a+b,a-b,-c)"};
    const DataPath hermannMauguinPath = computedDCTGeometryPath.createChildPath(k_Cell_Ensemble_Data).createChildPath("UniversalHermannMauguin");
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(hermannMauguinPath));
    const auto& hermannMauguinArray = dataStructure.getDataRefAs<StringArray>(hermannMauguinPath);
    REQUIRE(hermannMauguinArray.getNumberOfTuples() == expectedHermannMauguin.size());
    for(usize i = 0; i < expectedHermannMauguin.size(); i++)
    {
      CAPTURE(i);
      REQUIRE(hermannMauguinArray[i] == expectedHermannMauguin[i]);
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ReadGrainMapper3D:NonCompatible_Parameters", "[OrientationAnalysis][ReadGrainMapper3D]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "GrainMapper3D_Test_Files.tar.gz", "GrainMapper3D_Test_Files");

  auto exemplarFilePath = fs::path(fmt::format("{}/GrainMapper3D_Test_Files/7_0_SimulatedMultiPhase.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  DataPath computedDCTGeometryPath({fmt::format("{} computed", k_LabDCTGeometryName)});
  DataPath computedAbsorptionCTGeometryPath({fmt::format("{} computed", k_AbsorptionCTGeometryName)});

  DataPath exemplarDCTGeometryPath({fmt::format("{} (non-compatible)", k_LabDCTGeometryName)});
  DataPath exemplarAbsorptionCTGeometryPath({fmt::format("{} (non-compatible)", k_AbsorptionCTGeometryName)});

  auto inputGM3DFilePath = fs::path(fmt::format("{}/GrainMapper3D_Test_Files/SimulatedMultiPhase.h5", unit_test::k_TestFilesDir));
  {
    ReadGrainMapper3DFilter filter;
    Arguments args;

    args.insertOrAssign(ReadGrainMapper3DFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputGM3DFilePath)));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_ReadLabDCT_Key, std::make_any<bool>(true));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_CreatedDCTImageGeometryPath_Key, std::make_any<DataPath>(computedDCTGeometryPath));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Ensemble_Data));

    args.insertOrAssign(ReadGrainMapper3DFilter::k_ConvertPhaseToInt32_Key, std::make_any<bool>(false));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_ConvertOrientationData_Key, std::make_any<bool>(false));

    args.insertOrAssign(ReadGrainMapper3DFilter::k_ReadAbsorptionCT_Key, std::make_any<bool>(true));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_CreatedAbsorptionGeometryPath_Key, std::make_any<DataPath>(computedAbsorptionCTGeometryPath));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_CellAbsorptionAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));

    auto preflightResult = filter.preflight(dataStructure, args);
    auto assertionResult4 = preflightResult.outputActions;
    SIMPLNX_RESULT_REQUIRE_VALID(assertionResult4);

    auto executeResult = filter.execute(dataStructure, args);
    auto assertionResult5 = executeResult.result;
    SIMPLNX_RESULT_REQUIRE_VALID(assertionResult5);
  }
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/read_grainmapper_3d_non_compatible.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CompareImageGeometry(dataStructure, exemplarDCTGeometryPath, computedDCTGeometryPath);
  UnitTest::CompareImageGeometry(dataStructure, exemplarAbsorptionCTGeometryPath, computedAbsorptionCTGeometryPath);

  UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarDCTGeometryPath.createChildPath(k_Cell_Data), dataStructure, computedDCTGeometryPath.createChildPath(k_Cell_Data));
  UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarAbsorptionCTGeometryPath.createChildPath(k_Cell_Data), dataStructure,
                                                     computedAbsorptionCTGeometryPath.createChildPath(k_Cell_Data));
  UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarDCTGeometryPath.createChildPath(k_Cell_Ensemble_Data), dataStructure,
                                                     computedDCTGeometryPath.createChildPath(k_Cell_Ensemble_Data));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ReadGrainMapper3D:Synthetic_BatchBoundaries", "[OrientationAnalysis][ReadGrainMapper3D]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);

  SECTION("Datasets below the transfer boundary retain component order")
  {
    const GrainMapperFixtureSpec spec{{7, 3, 1}};
    const fs::path inputFile = WriteGrainMapperFixture("ReadGrainMapper3D_Small.h5", spec);
    const auto fileGuard = MakeScopeGuard([&inputFile]() noexcept {
      std::error_code errorCode;
      fs::remove(inputFile, errorCode);
    });

    DataStructure dataStructure;
    const DataPath geometryPath({"Synthetic Small"});
    ReadGrainMapper3DFilter filter;
    const Arguments args = GrainMapperArguments(inputFile, geometryPath);
    auto preflightResult2 = filter.preflight(dataStructure, args).outputActions;
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult2);
    auto executeResult2 = filter.execute(dataStructure, args).result;
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult2);

    VerifyGrainMapperOutput(dataStructure, geometryPath.createChildPath(k_CellAttributeMatrixName), {1, 3, 7});
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("Large row and plane batches retain values through the partial tail")
  {
    const GrainMapperFixtureSpec spec{{256, 257, 2}};
    const fs::path inputFile = WriteGrainMapperFixture("ReadGrainMapper3D_Batched.h5", spec);
    const auto fileGuard = MakeScopeGuard([&inputFile]() noexcept {
      std::error_code errorCode;
      fs::remove(inputFile, errorCode);
    });

    DataStructure dataStructure;
    const DataPath geometryPath({"Synthetic Batched"});
    ReadGrainMapper3DFilter filter;
    const Arguments args = GrainMapperArguments(inputFile, geometryPath);
    auto preflightResult3 = filter.preflight(dataStructure, args).outputActions;
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult3);
    auto executeResult3 = filter.execute(dataStructure, args).result;
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult3);

    VerifyGrainMapperOutput(dataStructure, geometryPath.createChildPath(k_CellAttributeMatrixName), {2, 257, 256});
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("OrientationAnalysis::ReadGrainMapper3D:Synthetic_ConversionCancellation", "[OrientationAnalysis][ReadGrainMapper3D]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);

  const fs::path inputFile = WriteGrainMapperFixture("ReadGrainMapper3D_Cancel.h5", GrainMapperFixtureSpec{{256, 257, 2}});
  const auto fileGuard = MakeScopeGuard([&inputFile]() noexcept {
    std::error_code errorCode;
    fs::remove(inputFile, errorCode);
  });
  const DataPath geometryPath({"Synthetic Cancellation"});
  const DataPath cellDataPath = geometryPath.createChildPath(k_CellAttributeMatrixName);
  DataStructure dataStructure;
  ApplyGrainMapperRegularActions(dataStructure, inputFile, geometryPath);
  auto* phaseArray = dataStructure.getDataAs<Int32Array>(cellDataPath.createChildPath("PhaseId"));
  auto* rodArray = dataStructure.getDataAs<Float32Array>(cellDataPath.createChildPath("Rodrigues"));
  REQUIRE(phaseArray != nullptr);
  REQUIRE(rodArray != nullptr);

  constexpr int32 k_PhaseSentinel = -713;
  constexpr float32 k_RodriguesSentinel = -719.0F;
  std::atomic_bool shouldCancel = false;
  auto phaseStore = std::make_shared<CancelAfterWriteDataStore<int32>>(phaseArray->getTupleShape(), phaseArray->getComponentShape(), k_PhaseSentinel, shouldCancel);
  auto rodStore = std::make_shared<DataStore<float32>>(rodArray->getTupleShape(), rodArray->getComponentShape(), k_RodriguesSentinel);
  auto setDataStoreResult2 = phaseArray->setDataStore(phaseStore);
  SIMPLNX_RESULT_REQUIRE_VALID(setDataStoreResult2);
  auto setDataStoreResult = rodArray->setDataStore(rodStore);
  SIMPLNX_RESULT_REQUIRE_VALID(setDataStoreResult);

  const IFilter::MessageHandler messageHandler = {};
  ReadGrainMapper3DInputValues inputValues = GrainMapperInputValues(inputFile, geometryPath);
  const Result<> result = ReadGrainMapper3D(dataStructure, messageHandler, shouldCancel, &inputValues)();
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(shouldCancel.load());
  REQUIRE(phaseStore->getWriteCalls() == 1);
  REQUIRE(phaseStore->getValue(0) == 0);
  REQUIRE(phaseStore->getValue(65535) == static_cast<int32>(65535 % 251));
  REQUIRE(phaseStore->getValue(65536) == k_PhaseSentinel);
  REQUIRE(rodStore->getValue(0) == k_RodriguesSentinel);
  REQUIRE(rodStore->getValue(rodStore->getSize() - 1) == k_RodriguesSentinel);
}

TEST_CASE("OrientationAnalysis::ReadGrainMapper3D:Synthetic_MalformedShapes", "[OrientationAnalysis][ReadGrainMapper3D]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);

  SECTION("Rodrigues dimensions that do not contain triples fail")
  {
    const fs::path inputFile = WriteGrainMapperFixture("ReadGrainMapper3D_BadRodrigues.h5", GrainMapperFixtureSpec{{2, 1, 1}, 2});
    const auto fileGuard = MakeScopeGuard([&inputFile]() noexcept {
      std::error_code errorCode;
      fs::remove(inputFile, errorCode);
    });
    DataStructure dataStructure;
    ReadGrainMapper3DFilter filter;
    const Arguments args = GrainMapperArguments(inputFile, DataPath({"Malformed Rodrigues"}));
    auto preflightResult4 = filter.preflight(dataStructure, args).outputActions;
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult4);
    const auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.invalid());
    REQUIRE(executeResult.result.errors().front().code == -89366);
  }

  SECTION("A dataset longer than the geometry reports its reachable destination mismatch")
  {
    const fs::path inputFile = WriteGrainMapperFixture("ReadGrainMapper3D_PhaseOverflow.h5", GrainMapperFixtureSpec{{2, 1, 1}, 3, 3});
    const auto fileGuard = MakeScopeGuard([&inputFile]() noexcept {
      std::error_code errorCode;
      fs::remove(inputFile, errorCode);
    });
    DataStructure dataStructure;
    ReadGrainMapper3DFilter filter;
    const Arguments args = GrainMapperArguments(inputFile, DataPath({"Phase Overflow"}));
    auto preflightResult5 = filter.preflight(dataStructure, args).outputActions;
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult5);
    const auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.invalid());
    REQUIRE(executeResult.result.errors().front().code == -89365);
  }
}
