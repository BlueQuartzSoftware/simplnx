#include "OrientationAnalysis/Filters/Algorithms/ComputeCAxisReferenceMisalignments.hpp"
#include "OrientationAnalysis/Filters/ComputeCAxisReferenceMisalignmentsFilter.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/IParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>
#include <EbsdLib/Orientation/Euler.hpp>
#include <EbsdLib/Orientation/Quaternion.hpp>

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <utility>
#include <vector>

using namespace nx::core;

namespace
{
using FilterType = ComputeCAxisReferenceMisalignmentsFilter;
using AlgorithmType = ComputeCAxisReferenceMisalignments;
using InputValuesType = ComputeCAxisReferenceMisalignmentsInputValues;
using EulerType = std::array<float64, 3>;
constexpr float64 k_Radians = numbers::pi / 180.0;
constexpr float32 k_NaN = std::numeric_limits<float32>::quiet_NaN();
const DataPath k_Quats({"Orientations", "Quats"});
const DataPath k_Phases({"Orientations", "Phases"});
const DataPath k_Mask({"Orientations", "Mask"});
const DataPath k_CrystalStructures({"Ensembles", "CrystalStructures"});
const DataPath k_Output({"Orientations", "CAxisReferenceMisalignments"});

template <typename T>
DataArray<T>& CreateArray(DataStructure& ds, const DataPath& path, const ShapeType& shape, usize components, DataObject::IdType parent)
{
  auto storePtr = DataStoreUtilities::CreateDataStore<T>(ds, path, shape, {components});
  auto* arrayPtr = DataArray<T>::Create(ds, path.getTargetName(), storePtr, parent);
  REQUIRE(arrayPtr != nullptr);
  return *arrayPtr;
}

// Returns the path below the DataGroup named groupName, or the path itself when groupName is empty.
DataPath InGroup(const std::string& groupName, const DataPath& path)
{
  if(groupName.empty())
  {
    return path;
  }
  std::vector<std::string> names = {groupName};
  const std::vector<std::string> pathNames = path.getPathVector();
  names.insert(names.end(), pathNames.begin(), pathNames.end());
  return DataPath(names);
}

// Builds the Orientations and Ensembles Attribute Matrices at the root, or inside a DataGroup named groupName.
DataStructure MakeData(const std::vector<EulerType>& eulers, const ShapeType& shape, std::vector<int32> phaseValues = {}, usize zeroQuaternion = std::numeric_limits<usize>::max(),
                       const std::string& groupName = "")
{
  DataStructure ds;
  std::optional<DataObject::IdType> groupId;
  if(!groupName.empty())
  {
    auto* groupPtr = DataGroup::Create(ds, groupName);
    REQUIRE(groupPtr != nullptr);
    groupId = groupPtr->getId();
  }
  auto* orientationsPtr = AttributeMatrix::Create(ds, "Orientations", shape, groupId);
  REQUIRE(orientationsPtr != nullptr);
  auto& quatsRef = CreateArray<float32>(ds, InGroup(groupName, k_Quats), shape, 4, orientationsPtr->getId());
  auto& phasesRef = CreateArray<int32>(ds, InGroup(groupName, k_Phases), shape, 1, orientationsPtr->getId());
  REQUIRE(quatsRef.getNumberOfTuples() == eulers.size());
  std::vector<float32> quatValues(eulers.size() * 4);
  if(phaseValues.empty())
  {
    phaseValues.assign(eulers.size(), 1);
  }
  REQUIRE(phaseValues.size() == eulers.size());
  for(usize tupleIdx = 0; tupleIdx < eulers.size(); ++tupleIdx)
  {
    const auto& eulerRef = eulers[tupleIdx];
    const auto quat = ebsdlib::EulerDType(eulerRef[0] * k_Radians, eulerRef[1] * k_Radians, eulerRef[2] * k_Radians).toQuaternion();
    for(usize compIdx = 0; compIdx < 4; ++compIdx)
    {
      quatValues[4 * tupleIdx + compIdx] = tupleIdx == zeroQuaternion ? 0.0F : static_cast<float32>(quat[compIdx]);
    }
  }
  auto quatsWriteResult = quatsRef.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(quatValues.data(), quatValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(quatsWriteResult);
  auto phasesWriteResult = phasesRef.getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(phaseValues.data(), phaseValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(phasesWriteResult);
  auto* ensemblesPtr = AttributeMatrix::Create(ds, "Ensembles", {5}, groupId);
  REQUIRE(ensemblesPtr != nullptr);
  auto& crystalStructuresRef = CreateArray<uint32>(ds, InGroup(groupName, k_CrystalStructures), {5}, 1, ensemblesPtr->getId());
  const std::vector<uint32> crystalValues = {ebsdlib::CrystalStructure::UnknownCrystalStructure, ebsdlib::CrystalStructure::Hexagonal_High, ebsdlib::CrystalStructure::Hexagonal_Low,
                                             ebsdlib::CrystalStructure::Cubic_High, ebsdlib::CrystalStructure::Trigonal_High};
  auto crystalWriteResult = crystalStructuresRef.getDataStoreRef().copyFromBuffer(0, nonstd::span<const uint32>(crystalValues.data(), crystalValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(crystalWriteResult);
  return ds;
}

Arguments MakeArgs(const std::vector<float32>& reference = {0, 0, 1}, bool useMask = false, const std::string& groupName = "")
{
  Arguments args;
  args.insertOrAssign(FilterType::k_ReferenceDir_Key, std::make_any<VectorFloat32Parameter::ValueType>(reference));
  args.insertOrAssign(FilterType::k_UseMask_Key, std::make_any<bool>(useMask));
  args.insertOrAssign(FilterType::k_MaskArrayPath_Key, std::make_any<DataPath>(InGroup(groupName, k_Mask)));
  args.insertOrAssign(FilterType::k_QuatsArrayPath_Key, std::make_any<DataPath>(InGroup(groupName, k_Quats)));
  args.insertOrAssign(FilterType::k_PhasesArrayPath_Key, std::make_any<DataPath>(InGroup(groupName, k_Phases)));
  args.insertOrAssign(FilterType::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(InGroup(groupName, k_CrystalStructures)));
  return args;
}

// The textbook third row of the Bunge matrix is independent of quaternion conversion.
float64 ExpectedAngle(const EulerType& euler, const std::vector<float32>& reference)
{
  const float64 phi1 = euler[0] * k_Radians;
  const float64 phi = euler[1] * k_Radians;
  const std::array<float64, 3> cAxis = {std::sin(phi1) * std::sin(phi), -std::cos(phi1) * std::sin(phi), std::cos(phi)};
  const float64 norm = std::hypot(static_cast<float64>(reference[0]), static_cast<float64>(reference[1]), static_cast<float64>(reference[2]));
  const float64 dot = (cAxis[0] * reference[0] + cAxis[1] * reference[1] + cAxis[2] * reference[2]) / norm;
  return std::acos(std::clamp(std::abs(dot), 0.0, 1.0)) / k_Radians;
}

void CheckOutput(const DataStructure& ds, const std::vector<EulerType>& eulers, const std::vector<float32>& reference, const std::vector<usize>& excluded = {}, const std::string& groupName = "")
{
  const DataPath outputPath = InGroup(groupName, k_Output);
  const DataPath quatsPath = InGroup(groupName, k_Quats);
  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(outputPath));
  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(quatsPath));
  const auto& outputRef = ds.getDataRefAs<Float32Array>(outputPath);
  REQUIRE(outputRef.getNumberOfComponents() == 1);
  REQUIRE(outputRef.getIDataStoreRef().getTupleShape() == ds.getDataRefAs<Float32Array>(quatsPath).getIDataStoreRef().getTupleShape());
  std::vector<float32> outputValues(outputRef.getSize());
  auto outputReadResult = outputRef.getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(outputValues.data(), outputValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(outputReadResult);
  for(usize tupleIdx = 0; tupleIdx < eulers.size(); ++tupleIdx)
  {
    CAPTURE(tupleIdx);
    if(std::find(excluded.begin(), excluded.end(), tupleIdx) != excluded.end())
    {
      REQUIRE(std::isnan(outputValues[tupleIdx]));
    }
    else
    {
      REQUIRE(std::isfinite(outputValues[tupleIdx]));
      REQUIRE(outputValues[tupleIdx] >= 0.0F);
      REQUIRE(outputValues[tupleIdx] <= 90.0F);
      REQUIRE(outputValues[tupleIdx] == Approx(ExpectedAngle(eulers[tupleIdx], reference)).margin(2.0e-4));
    }
  }
}

// The shared algorithm has no dispatch event. Use the scenario's store guard,
// but do not call executeFilter(), which requires a Direct/Scanline dispatch.
Result<> ExecuteInScope(DataStructure& ds, const Arguments& args, const UnitTest::AlgorithmTestScope& scope, const std::string& groupName = "")
{
  FilterType filter;
  auto preflight = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const DataPath outputPath = InGroup(groupName, k_Output);
  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(outputPath));
  scope.requireExpectedStore(ds.getDataRefAs<Float32Array>(outputPath));
  return std::move(result.result);
}

void ExecuteAndCheck(DataStructure& ds, const Arguments& args, const UnitTest::AlgorithmTestScope& scope, const std::string& groupName = "")
{
  const Result<> result = ExecuteInScope(ds, args, scope, groupName);
  REQUIRE(result.warnings().empty());
}

// Valid inputs for a direct algorithm call: two tuples, a uint8 mask, and the pre-created output.
DataStructure MakeDirectCallData()
{
  DataStructure ds = MakeData({{0, 30, 0}, {0, 45, 0}}, {2});
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(k_Quats.getParent()));
  const auto parentId = ds.getDataRefAs<AttributeMatrix>(k_Quats.getParent()).getId();
  CreateArray<uint8>(ds, k_Mask, {2}, 1, parentId).fill(1);
  CreateArray<float32>(ds, k_Output, {2}, 1, parentId);
  REQUIRE(AttributeMatrix::Create(ds, "Other", {3}) != nullptr);
  return ds;
}

InputValuesType MakeDirectCallInputValues()
{
  InputValuesType inputValues;
  inputValues.ReferenceDirection = {0.0F, 0.0F, 1.0F};
  inputValues.UseMask = true;
  inputValues.QuatsArrayPath = k_Quats;
  inputValues.PhasesArrayPath = k_Phases;
  inputValues.MaskArrayPath = k_Mask;
  inputValues.CrystalStructuresArrayPath = k_CrystalStructures;
  inputValues.OutputArrayPath = k_Output;
  return inputValues;
}

Result<> RunAlgorithm(DataStructure& ds, const InputValuesType& inputValues)
{
  const std::atomic_bool shouldCancel = false;
  const IFilter::MessageHandler messageHandler{};
  return AlgorithmType(ds, messageHandler, shouldCancel, &inputValues)();
}
} // namespace

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: cell and feature analytical oracle", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const bool featureLevel = GENERATE(false, true);
  const float64 azimuthOffset = GENERATE(0.0, 43.0, 172.0);
  std::vector<EulerType> eulers = {{0, 0, 71}, {22, 30, 15}, {147, 45, 92}, {271, 90, 13}, {81, 120, 245}};
  for(auto& eulerRef : eulers)
  {
    eulerRef[0] = std::fmod(eulerRef[0] + azimuthOffset, 360.0);
    eulerRef[2] = std::fmod(eulerRef[2] + 2.0 * azimuthOffset, 360.0);
  }
  auto ds = MakeData(eulers, featureLevel ? ShapeType{5} : ShapeType{1, 1, 5});
  ExecuteAndCheck(ds, MakeArgs(), scope);
  CheckOutput(ds, eulers, {0, 0, 1});
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: nonunit reference oracle", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const auto reference = GENERATE(std::vector<float32>{3, 0, 3}, std::vector<float32>{1, 2, -2}, std::vector<float32>{-1, -2, 2});
  const std::vector<EulerType> eulers = {{0, 0, 71}, {90, 45, 31}, {147, 45, 92}, {271, 90, 13}, {81, 120, 245}};
  auto ds = MakeData(eulers, {5});
  ExecuteAndCheck(ds, MakeArgs(reference), scope);
  CheckOutput(ds, eulers, reference);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: inputs inside a DataGroup", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const std::string groupName = "Group";
  const std::vector<EulerType> eulers = {{22, 30, 15}, {147, 45, 92}, {271, 90, 13}};
  auto ds = MakeData(eulers, {3}, {}, std::numeric_limits<usize>::max(), groupName);
  ExecuteAndCheck(ds, MakeArgs({1, 2, -2}, false, groupName), scope, groupName);
  CheckOutput(ds, eulers, {1, 2, -2}, {}, groupName);
  // The output is created beside the nested quaternions, not at the root.
  REQUIRE(ds.getDataAs<Float32Array>(k_Output) == nullptr);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: fixed seed sweep", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  std::mt19937 generator(1729);
  std::uniform_real_distribution<float64> azimuth(0.0, 360.0);
  std::uniform_real_distribution<float64> inclination(0.0, 180.0);
  std::uniform_real_distribution<float32> direction(-10.0F, 10.0F);
  std::vector<EulerType> eulers(384);
  for(auto& eulerRef : eulers)
  {
    eulerRef = {azimuth(generator), inclination(generator), azimuth(generator)};
  }
  for(usize run = 0; run < 8; ++run)
  {
    const std::vector<float32> reference = {direction(generator), direction(generator), direction(generator)};
    auto ds = MakeData(eulers, {8, 8, 6});
    ExecuteAndCheck(ds, MakeArgs(reference), scope);
    CheckOutput(ds, eulers, reference);
    UnitTest::CheckArraysInheritTupleDims(ds);
  }
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: masks and chunk boundaries", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const bool byteMask = GENERATE(false, true);
  // Cross two 4096-tuple boundaries and exercise a partial final chunk.
  std::vector<EulerType> eulers(8201);
  for(usize tupleIdx = 0; tupleIdx < eulers.size(); ++tupleIdx)
  {
    eulers[tupleIdx] = {static_cast<float64>(tupleIdx % 360), static_cast<float64>(tupleIdx % 181), static_cast<float64>((tupleIdx * 7) % 360)};
  }
  auto ds = MakeData(eulers, {eulers.size()});
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(k_Quats.getParent()));
  const auto parentId = ds.getDataRefAs<AttributeMatrix>(k_Quats.getParent()).getId();
  const std::vector<usize> excluded = {0, 4095, 4096, 8192, 8200};
  if(byteMask)
  {
    auto& maskRef = CreateArray<uint8>(ds, k_Mask, {eulers.size()}, 1, parentId);
    maskRef.fill(255);
    for(auto tupleIdx : excluded)
    {
      maskRef[tupleIdx] = 0;
    }
  }
  else
  {
    auto& maskRef = CreateArray<bool>(ds, k_Mask, {eulers.size()}, 1, parentId);
    maskRef.fill(true);
    for(auto tupleIdx : excluded)
    {
      maskRef[tupleIdx] = false;
    }
  }
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(k_Phases));
  ds.getDataRefAs<Int32Array>(k_Phases)[4096] = -1; // Masking takes precedence over phase validation.
  ExecuteAndCheck(ds, MakeArgs({1, 2, -2}, true), scope);
  CheckOutput(ds, eulers, {1, 2, -2}, excluded);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: out-of-core stores", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  UnitTest::LoadPlugins();
  const bool byteMask = GENERATE(false, true);
  CAPTURE(byteMask);
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);

  // Cross three 4096-tuple boundaries and exercise a partial final chunk.
  constexpr usize k_NumTuples = 3 * 4096 + 17;
  constexpr usize k_ZeroQuaternion = 3 * 4096 + 1; // Unmasked hexagonal tuple in the final chunk.
  std::vector<EulerType> eulers(k_NumTuples);
  for(usize i = 0; i < eulers.size(); ++i)
  {
    eulers[i] = {static_cast<double>(i % 360), static_cast<double>(i % 181), static_cast<double>((i * 7) % 360)};
  }
  std::vector<int32> phaseValues(k_NumTuples);
  usize expectedNonHexagonalCount = 0;
  std::vector<usize> excluded;
  for(usize i = 0; i < k_NumTuples; ++i)
  {
    // Mix unknown (0), hexagonal high (1), hexagonal low (2), and cubic (3) phases.
    // The period 6 does not divide 4096, so each chunk has a different phase pattern
    // and a chunk read at the wrong offset changes the result.
    phaseValues[i] = static_cast<int32>((i % 6) % 4);
    const bool masked = i % 11 == 0;
    const bool hexagonal = phaseValues[i] == 1 || phaseValues[i] == 2;
    if(!masked && phaseValues[i] == 3)
    {
      ++expectedNonHexagonalCount;
    }
    if(masked || !hexagonal || i == k_ZeroQuaternion)
    {
      excluded.push_back(i);
    }
  }
  auto ds = MakeData(eulers, {k_NumTuples}, phaseValues, k_ZeroQuaternion);
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(k_Quats.getParent()));
  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(k_Quats));
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(k_Phases));
  REQUIRE_NOTHROW(ds.getDataRefAs<UInt32Array>(k_CrystalStructures));
  const auto parent = ds.getDataRefAs<AttributeMatrix>(k_Quats.getParent()).getId();
  auto& quats = ds.getDataRefAs<Float32Array>(k_Quats);
  auto& phases = ds.getDataRefAs<Int32Array>(k_Phases);
  const auto& crystalStructures = ds.getDataRefAs<UInt32Array>(k_CrystalStructures);
  if(byteMask)
  {
    auto& mask = CreateArray<uint8>(ds, k_Mask, {k_NumTuples}, 1, parent);
    std::vector<uint8> maskValues(k_NumTuples);
    for(usize i = 0; i < k_NumTuples; ++i)
    {
      maskValues[i] = i % 11 == 0 ? 0 : 255;
    }
    auto maskWriteResult = mask.getDataStoreRef().copyFromBuffer(0, nonstd::span<const uint8>(maskValues.data(), maskValues.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(maskWriteResult);
  }
  else
  {
    auto& mask = CreateArray<bool>(ds, k_Mask, {k_NumTuples}, 1, parent);
    // Project-rule exception: std::vector<bool> cannot provide the contiguous bool* required for bulk I/O.
    std::unique_ptr<bool[]> maskValues = std::make_unique<bool[]>(k_NumTuples);
    for(usize i = 0; i < k_NumTuples; ++i)
    {
      maskValues[i] = i % 11 != 0;
    }
    auto maskWriteResult = mask.getDataStoreRef().copyFromBuffer(0, nonstd::span<const bool>(maskValues.get(), k_NumTuples));
    SIMPLNX_RESULT_REQUIRE_VALID(maskWriteResult);
  }
  REQUIRE_NOTHROW(ds.getDataRefAs<IDataArray>(k_Mask));
  if(Application::Instance()->getIOManager("HDF5-OOC") != nullptr)
  {
    REQUIRE(quats.getDataStoreRef().getDataFormat() == "HDF5-OOC");
    REQUIRE(phases.getDataStoreRef().getDataFormat() == "HDF5-OOC");
    REQUIRE(crystalStructures.getDataStoreRef().getDataFormat() == "HDF5-OOC");
    REQUIRE(ds.getDataRefAs<IDataArray>(k_Mask).getIDataStoreRef().getDataFormat() == "HDF5-OOC");
  }

  const std::vector<float32> reference = {1, 2, -2};
  const auto args = MakeArgs(reference, true);
  FilterType filter;
  auto preflight = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().size() == 1);
  REQUIRE(result.result.warnings()[0].code == AlgorithmType::k_NonHexagonalPhasesWarning);
  REQUIRE(result.result.warnings()[0].message.find(std::to_string(expectedNonHexagonalCount) + " tuples") == 0);
  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(k_Output));
  if(Application::Instance()->getIOManager("HDF5-OOC") != nullptr)
  {
    REQUIRE(ds.getDataRefAs<Float32Array>(k_Output).getDataStoreRef().getDataFormat() == "HDF5-OOC");
  }
  CheckOutput(ds, eulers, reference, excluded);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: hexagonal phases only", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const std::vector<EulerType> eulers(5, {23, 37, 89});
  auto ds = MakeData(eulers, {5});
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(k_Phases));
  auto& phasesRef = ds.getDataRefAs<Int32Array>(k_Phases);
  phasesRef[2] = 2;
  phasesRef[3] = 0;
  // A zero quaternion has no c-axis and must not be reported as perpendicular (90 degrees).
  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(k_Quats));
  auto& quatsRef = ds.getDataRefAs<Float32Array>(k_Quats);
  for(usize compIdx = 0; compIdx < 4; ++compIdx)
  {
    quatsRef[compIdx] = 0.0F;
  }
  usize expectedNonHexagonalCount = 0;
  std::vector<usize> excluded = {0, 3};
  SECTION("Unknown crystal structure produces NaN without a warning")
  {
  }
  SECTION("Cubic phase produces NaN and counts only the known non-hexagonal tuple")
  {
    phasesRef[1] = 3;
    expectedNonHexagonalCount = 1;
    excluded.push_back(1);
  }
  SECTION("Cubic and trigonal phases count both known non-hexagonal tuples")
  {
    phasesRef[1] = 3;
    phasesRef[4] = 4;
    expectedNonHexagonalCount = 2;
    excluded = {0, 1, 3, 4};
  }
  const Result<> result = ExecuteInScope(ds, MakeArgs(), scope);
  CheckOutput(ds, eulers, {0, 0, 1}, excluded);
  if(expectedNonHexagonalCount == 0)
  {
    REQUIRE(result.warnings().empty());
  }
  else
  {
    REQUIRE(result.warnings().size() == 1);
    REQUIRE(result.warnings()[0].code == AlgorithmType::k_NonHexagonalPhasesWarning);
    REQUIRE(result.warnings()[0].message.find(std::to_string(expectedNonHexagonalCount) + " tuples") == 0);
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: masked non-hexagonal tuples are not counted", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const bool byteMask = GENERATE(false, true);
  CAPTURE(byteMask);
  const std::vector<EulerType> eulers(6, {23, 37, 89});
  auto ds = MakeData(eulers, {6});
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(k_Phases));
  auto& phasesRef = ds.getDataRefAs<Int32Array>(k_Phases);
  phasesRef[1] = 3;
  phasesRef[2] = 4;
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(k_Quats.getParent()));
  const auto parentId = ds.getDataRefAs<AttributeMatrix>(k_Quats.getParent()).getId();
  if(byteMask)
  {
    auto& maskRef = CreateArray<uint8>(ds, k_Mask, {6}, 1, parentId);
    maskRef.fill(1);
    maskRef[1] = 0;
    maskRef[2] = 0;
  }
  else
  {
    auto& maskRef = CreateArray<bool>(ds, k_Mask, {6}, 1, parentId);
    maskRef.fill(true);
    maskRef[1] = false;
    maskRef[2] = false;
  }
  usize expectedNonHexagonalCount = 0;
  std::vector<usize> excluded = {1, 2};
  SECTION("All non-hexagonal tuples masked")
  {
  }
  SECTION("One unmasked cubic tuple is counted")
  {
    phasesRef[4] = 3;
    expectedNonHexagonalCount = 1;
    excluded.push_back(4);
  }
  const Result<> result = ExecuteInScope(ds, MakeArgs({0, 0, 1}, true), scope);
  CheckOutput(ds, eulers, {0, 0, 1}, excluded);
  if(expectedNonHexagonalCount == 0)
  {
    REQUIRE(result.warnings().empty());
  }
  else
  {
    REQUIRE(result.warnings().size() == 1);
    REQUIRE(result.warnings()[0].code == AlgorithmType::k_NonHexagonalPhasesWarning);
    REQUIRE(result.warnings()[0].message.find("1 tuples") == 0);
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: zero quaternion is undefined", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const usize count = GENERATE(1, 4096, 4097, 8192);
  CAPTURE(count);
  const std::vector<EulerType> eulers(count, {23, 37, 89});
  auto ds = MakeData(eulers, {count});
  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(k_Quats));
  auto& quatsRef = ds.getDataRefAs<Float32Array>(k_Quats);
  for(usize compIdx = 0; compIdx < 4; ++compIdx)
  {
    quatsRef[4 * (count - 1) + compIdx] = 0.0F;
  }
  ExecuteAndCheck(ds, MakeArgs(), scope);
  // A zero quaternion defines no orientation, so it cannot have a finite angle.
  CheckOutput(ds, eulers, {0, 0, 1}, {count - 1});
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: nonfinite quaternions", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const usize compIdx = GENERATE(0, 1, 2, 3);
  const float32 value = GENERATE(as<float32>{}, k_NaN, std::numeric_limits<float32>::infinity(), -std::numeric_limits<float32>::infinity());
  CAPTURE(compIdx, value);
  REQUIRE_FALSE(std::isfinite(value));
  const std::vector<EulerType> eulers(2, {23, 37, 89});
  auto ds = MakeData(eulers, {2});
  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(k_Quats));
  ds.getDataRefAs<Float32Array>(k_Quats)[compIdx] = value;
  ExecuteAndCheck(ds, MakeArgs(), scope);
  CheckOutput(ds, eulers, {0, 0, 1}, {0});
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: quaternion scale invariance", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const float32 scale = GENERATE(-2.0F, 0.5F, 1.0e-30F, 1.0e30F);
  CAPTURE(scale);
  const std::vector<EulerType> eulers = {{0, 0, 71}, {90, 45, 31}, {147, 45, 92}, {271, 90, 13}, {81, 120, 245}};
  auto ds = MakeData(eulers, {eulers.size()});
  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(k_Quats));
  auto& quatsRef = ds.getDataRefAs<Float32Array>(k_Quats);
  for(usize valueIdx = 0; valueIdx < quatsRef.getSize(); ++valueIdx)
  {
    quatsRef[valueIdx] *= scale;
  }
  ExecuteAndCheck(ds, MakeArgs({1, 2, -2}), scope);
  CheckOutput(ds, eulers, {1, 2, -2});
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: empty input", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  auto ds = MakeData({}, {0});
  ExecuteAndCheck(ds, MakeArgs(), scope);
  CheckOutput(ds, {}, {0, 0, 1});
  REQUIRE(ds.getDataRefAs<Float32Array>(k_Output).getNumberOfTuples() == 0);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: cancellation", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const bool cancelBeforeExecute = GENERATE(false, true);
  CAPTURE(cancelBeforeExecute);
  std::atomic_bool shouldCancel = cancelBeforeExecute;
  const std::vector<EulerType> eulers(4097, {23, 37, 89});
  auto ds = MakeData(eulers, {eulers.size()});
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(k_Phases));
  // If cancellation is ignored at the next chunk boundary, this phase gives k_PhaseOutOfRangeError.
  ds.getDataRefAs<Int32Array>(k_Phases)[4096] = -1;
  const IFilter::MessageHandler messageHandler{[&shouldCancel](const IFilter::Message& message) {
    if(message.type == IFilter::Message::Type::Progress)
    {
      shouldCancel = true;
    }
  }};
  auto result = FilterType().execute(ds, MakeArgs(), nullptr, messageHandler, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  // IFilter reports a cancelled run with code -1.
  REQUIRE(result.result.errors()[0].code == -1);
  if(cancelBeforeExecute)
  {
    REQUIRE(ds.getDataAs<Float32Array>(k_Output) == nullptr);
  }
  else
  {
    REQUIRE(shouldCancel.load());
    CheckOutput(ds, std::vector<EulerType>(4096, eulers.front()), {0, 0, 1});
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: invalid reference", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const auto reference =
      GENERATE(std::vector<float32>{0, 0, 0}, std::vector<float32>{k_NaN, 0, 1}, std::vector<float32>{0, std::numeric_limits<float32>::infinity(), 1}, std::vector<float32>{0, 0, 1.0e-13F});
  auto ds = MakeData({{0, 30, 0}}, {1});
  auto result = FilterType().preflight(ds, MakeArgs(reference));
  REQUIRE(result.outputActions.invalid());
  REQUIRE(result.outputActions.errors()[0].code == AlgorithmType::k_InvalidReferenceDirectionError);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: reference with wrong component count", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  // VectorParameter rejects a wrong-sized vector before preflightImpl, so only a direct call reaches this check.
  const auto reference = GENERATE(std::vector<float32>{}, std::vector<float32>{0.0F, 1.0F}, std::vector<float32>{0.0F, 0.0F, 1.0F, 0.0F});
  const Result<> result = AlgorithmType::ValidateReferenceDirection(reference);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  REQUIRE(result.errors()[0].code == AlgorithmType::k_ReferenceDirectionSizeError);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: tuple mismatch", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const bool mismatchMask = GENERATE(false, true);
  auto ds = MakeData({{0, 30, 0}, {0, 45, 0}}, {2});
  auto* otherPtr = AttributeMatrix::Create(ds, "Other", {3});
  REQUIRE(otherPtr != nullptr);
  const DataPath otherPath({"Other", "Mismatched"});
  auto args = MakeArgs({0, 0, 1}, mismatchMask);
  if(mismatchMask)
  {
    CreateArray<uint8>(ds, otherPath, {3}, 1, otherPtr->getId());
    args.insertOrAssign(FilterType::k_MaskArrayPath_Key, std::make_any<DataPath>(otherPath));
  }
  else
  {
    CreateArray<int32>(ds, otherPath, {3}, 1, otherPtr->getId());
    args.insertOrAssign(FilterType::k_PhasesArrayPath_Key, std::make_any<DataPath>(otherPath));
  }
  auto result = FilterType().preflight(ds, args);
  REQUIRE(result.outputActions.invalid());
  REQUIRE(result.outputActions.errors()[0].code == AlgorithmType::k_TupleCountMismatchError);
  const auto& messageRef = result.outputActions.errors()[0].message;
  REQUIRE(messageRef.find(mismatchMask ? "Mask array" : "Phases array") == 0);
  REQUIRE(messageRef.find("Quats") != std::string::npos);
  REQUIRE(messageRef.find("Mismatched") != std::string::npos);
  REQUIRE(messageRef.find("2") != std::string::npos);
  REQUIRE(messageRef.find("3") != std::string::npos);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: phase index error", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const int32 phase = GENERATE(-1, 5, std::numeric_limits<int32>::max());
  auto ds = MakeData({{0, 30, 0}, {0, 45, 0}}, {2});
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(k_Phases));
  ds.getDataRefAs<Int32Array>(k_Phases)[1] = phase;
  auto result = FilterType().execute(ds, MakeArgs());
  REQUIRE(result.result.invalid());
  REQUIRE(result.result.errors()[0].code == AlgorithmType::k_PhaseOutOfRangeError);
  const auto& messageRef = result.result.errors()[0].message;
  REQUIRE(messageRef.find("Phases") != std::string::npos);
  REQUIRE(messageRef.find("CrystalStructures") != std::string::npos);
  REQUIRE(messageRef.find(std::to_string(phase)) != std::string::npos);
  REQUIRE(messageRef.find("tuple index 1") != std::string::npos);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: rejects Euler angle input", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  auto ds = MakeData({{0, 30, 0}, {0, 45, 0}}, {2});
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(k_Quats.getParent()));
  const auto parentId = ds.getDataRefAs<AttributeMatrix>(k_Quats.getParent()).getId();
  const DataPath eulersPath({"Orientations", "EulerAngles"});
  CreateArray<float32>(ds, eulersPath, {2}, 3, parentId);
  auto args = MakeArgs();
  args.insertOrAssign(FilterType::k_QuatsArrayPath_Key, std::make_any<DataPath>(eulersPath));
  auto result = FilterType().preflight(ds, args);
  REQUIRE(result.outputActions.invalid());
  REQUIRE(result.outputActions.errors().size() == 1);
  REQUIRE(result.outputActions.errors()[0].code == FilterParameter::Constants::k_Validate_TupleShapeValue);
  REQUIRE(ds.getDataAs<Float32Array>(k_Output) == nullptr);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: output name equal to an input name", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  auto ds = MakeData({{0, 30, 0}, {0, 45, 0}}, {2});
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(k_Quats.getParent()));
  CreateArray<uint8>(ds, k_Mask, {2}, 1, ds.getDataRefAs<AttributeMatrix>(k_Quats.getParent()).getId()).fill(1);
  const std::string inputName = GENERATE(std::string("Quats"), std::string("Phases"), std::string("Mask"));
  CAPTURE(inputName);
  auto args = MakeArgs({0, 0, 1}, true);
  args.insertOrAssign(FilterType::k_CAxisReferenceMisalignmentsArrayName_Key, std::make_any<std::string>(inputName));
  auto result = FilterType().preflight(ds, args);
  REQUIRE(result.outputActions.invalid());
  REQUIRE(result.outputActions.errors().size() == 1);
  REQUIRE(result.outputActions.errors()[0].code == AlgorithmType::k_OutputPathCollisionError);
  REQUIRE(result.outputActions.errors()[0].message.find("Orientations/" + inputName) != std::string::npos);
  REQUIRE(ds.getDataRefAs<IDataArray>(k_Quats.replaceName(inputName)).getNumberOfComponents() == (inputName == "Quats" ? 4 : 1));
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: preflight rejects string array selections", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  // ArraySelectionParameter applies its numeric type and component-shape lists only to IDataArray,
  // so a StringArray with the right tuple count reaches preflightImpl.
  auto ds = MakeData({{0, 30, 0}, {0, 45, 0}}, {2});
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(k_Quats.getParent()));
  const auto parentId = ds.getDataRefAs<AttributeMatrix>(k_Quats.getParent()).getId();
  const DataPath stringsPath({"Orientations", "Strings"});
  REQUIRE(StringArray::CreateWithValues(ds, stringsPath.getTargetName(), {2}, {"a", "b"}, parentId) != nullptr);
  const bool useMask = GENERATE(false, true);
  auto args = MakeArgs({0, 0, 1}, useMask);
  int32 expectedCode = AlgorithmType::k_InvalidMaskArrayError;
  if(useMask)
  {
    args.insertOrAssign(FilterType::k_MaskArrayPath_Key, std::make_any<DataPath>(stringsPath));
  }
  else
  {
    args.insertOrAssign(FilterType::k_QuatsArrayPath_Key, std::make_any<DataPath>(stringsPath));
    expectedCode = AlgorithmType::k_InvalidQuatsArrayError;
  }
  IFilter::PreflightResult result;
  REQUIRE_NOTHROW(result = FilterType().preflight(ds, args));
  REQUIRE(result.outputActions.invalid());
  REQUIRE(result.outputActions.errors().size() == 1);
  REQUIRE(result.outputActions.errors()[0].code == expectedCode);
  REQUIRE(result.outputActions.errors()[0].message.find("has type 'StringArray'") != std::string::npos);
  REQUIRE(ds.getDataAs<Float32Array>(k_Output) == nullptr);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: algorithm validates its own inputs", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  DataStructure ds = MakeDirectCallData();
  InputValuesType inputValues = MakeDirectCallInputValues();
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(k_Quats.getParent()));
  const auto parentId = ds.getDataRefAs<AttributeMatrix>(k_Quats.getParent()).getId();
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Other"})));
  const auto otherId = ds.getDataRefAs<AttributeMatrix>(DataPath({"Other"})).getId();
  const DataPath wrongPath({"Orientations", "Wrong"});
  const DataPath otherPath({"Other", "Mismatched"});
  // 0 means the inputs are valid and the call must succeed.
  int32 expectedCode = 0;
  // A phrase that only the message of the expected condition contains, when the code alone is not enough.
  std::string expectedPhrase;

  SECTION("Valid inputs succeed")
  {
  }
  SECTION("Missing quaternion array")
  {
    inputValues.QuatsArrayPath = DataPath({"Orientations", "Missing"});
    expectedCode = AlgorithmType::k_InvalidQuatsArrayError;
    expectedPhrase = "does not exist";
  }
  SECTION("Quaternion array of the wrong type")
  {
    CreateArray<float64>(ds, wrongPath, {2}, 4, parentId);
    inputValues.QuatsArrayPath = wrongPath;
    expectedCode = AlgorithmType::k_InvalidQuatsArrayError;
    expectedPhrase = "has type 'float64', but type 'float32' is required";
  }
  SECTION("Quaternion array with five components")
  {
    CreateArray<float32>(ds, wrongPath, {2}, 5, parentId);
    inputValues.QuatsArrayPath = wrongPath;
    expectedCode = AlgorithmType::k_QuatsComponentShapeError;
  }
  SECTION("Phases array of the wrong type")
  {
    CreateArray<uint32>(ds, wrongPath, {2}, 1, parentId);
    inputValues.PhasesArrayPath = wrongPath;
    expectedCode = AlgorithmType::k_InvalidPhasesArrayError;
  }
  SECTION("Phases array with two components")
  {
    CreateArray<int32>(ds, wrongPath, {2}, 2, parentId);
    inputValues.PhasesArrayPath = wrongPath;
    expectedCode = AlgorithmType::k_PhasesComponentShapeError;
  }
  SECTION("Phases array with a different tuple count")
  {
    CreateArray<int32>(ds, otherPath, {3}, 1, otherId);
    inputValues.PhasesArrayPath = otherPath;
    expectedCode = AlgorithmType::k_TupleCountMismatchError;
    expectedPhrase = "Phases array 'Other/Mismatched' has 3 tuples";
  }
  SECTION("Mask array of the wrong type")
  {
    CreateArray<int32>(ds, wrongPath, {2}, 1, parentId);
    inputValues.MaskArrayPath = wrongPath;
    expectedCode = AlgorithmType::k_InvalidMaskArrayError;
    expectedPhrase = "has type 'int32', but type 'bool' or 'uint8' is required";
  }
  SECTION("Mask array with two components")
  {
    CreateArray<uint8>(ds, wrongPath, {2}, 2, parentId);
    inputValues.MaskArrayPath = wrongPath;
    expectedCode = AlgorithmType::k_MaskComponentShapeError;
  }
  SECTION("Mask array with a different tuple count")
  {
    CreateArray<bool>(ds, otherPath, {3}, 1, otherId);
    inputValues.MaskArrayPath = otherPath;
    expectedCode = AlgorithmType::k_TupleCountMismatchError;
    expectedPhrase = "Mask array 'Other/Mismatched' has 3 tuples";
  }
  SECTION("Crystal Structures array of the wrong type")
  {
    CreateArray<int32>(ds, otherPath, {3}, 1, otherId);
    inputValues.CrystalStructuresArrayPath = otherPath;
    expectedCode = AlgorithmType::k_InvalidCrystalStructuresArrayError;
  }
  SECTION("Crystal Structures array with two components")
  {
    CreateArray<uint32>(ds, otherPath, {3}, 2, otherId);
    inputValues.CrystalStructuresArrayPath = otherPath;
    expectedCode = AlgorithmType::k_CrystalStructuresComponentShapeError;
  }
  SECTION("Missing output array")
  {
    inputValues.OutputArrayPath = DataPath({"Orientations", "Missing"});
    expectedCode = AlgorithmType::k_InvalidOutputArrayError;
  }
  SECTION("Output array with two components")
  {
    CreateArray<float32>(ds, wrongPath, {2}, 2, parentId);
    inputValues.OutputArrayPath = wrongPath;
    expectedCode = AlgorithmType::k_OutputComponentShapeError;
  }
  SECTION("Output array with a different tuple count")
  {
    CreateArray<float32>(ds, otherPath, {3}, 1, otherId);
    inputValues.OutputArrayPath = otherPath;
    expectedCode = AlgorithmType::k_OutputTupleCountMismatchError;
  }
  SECTION("Non-finite reference direction")
  {
    inputValues.ReferenceDirection = {k_NaN, 0.0F, 1.0F};
    expectedCode = AlgorithmType::k_InvalidReferenceDirectionError;
  }
  SECTION("Reference direction with two components")
  {
    inputValues.ReferenceDirection = {0.0F, 1.0F};
    expectedCode = AlgorithmType::k_ReferenceDirectionSizeError;
  }
  SECTION("Phase outside the Crystal Structures array")
  {
    REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(k_Phases));
    ds.getDataRefAs<Int32Array>(k_Phases)[1] = 5;
    expectedCode = AlgorithmType::k_PhaseOutOfRangeError;
  }
  CAPTURE(expectedCode);
  Result<> result;
  REQUIRE_NOTHROW(result = RunAlgorithm(ds, inputValues));
  if(expectedCode == 0)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    return;
  }
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  REQUIRE(result.errors()[0].code == expectedCode);
  if(!expectedPhrase.empty())
  {
    REQUIRE(result.errors()[0].message.find(expectedPhrase) != std::string::npos);
  }
}
