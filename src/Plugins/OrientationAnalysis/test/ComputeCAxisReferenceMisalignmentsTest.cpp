#include "OrientationAnalysis/Filters/ComputeCAxisReferenceMisalignmentsFilter.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
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
#include <random>
#include <vector>

using namespace nx::core;

namespace
{
using Filter = ComputeCAxisReferenceMisalignmentsFilter;
using Euler = std::array<double, 3>;
constexpr double k_Radians = numbers::pi / 180.0;
constexpr float32 k_NaN = std::numeric_limits<float32>::quiet_NaN();
const DataPath k_Quats({"Orientations", "Quats"});
const DataPath k_Phases({"Orientations", "Phases"});
const DataPath k_Mask({"Orientations", "Mask"});
const DataPath k_CrystalStructures({"Ensembles", "CrystalStructures"});
const DataPath k_Output({"Orientations", "CAxisReferenceMisalignments"});

template <typename T>
DataArray<T>& CreateArray(DataStructure& ds, const DataPath& path, const ShapeType& shape, usize components, DataObject::IdType parent)
{
  auto store = DataStoreUtilities::CreateDataStore<T>(ds, path, shape, {components});
  auto* array = DataArray<T>::Create(ds, path.getTargetName(), store, parent);
  REQUIRE(array != nullptr);
  return *array;
}

DataStructure MakeData(const std::vector<Euler>& eulers, const ShapeType& shape, std::vector<int32> phaseValues = {}, usize zeroQuaternion = std::numeric_limits<usize>::max())
{
  DataStructure ds;
  auto* am = AttributeMatrix::Create(ds, "Orientations", shape);
  REQUIRE(am != nullptr);
  auto& quats = CreateArray<float32>(ds, k_Quats, shape, 4, am->getId());
  auto& phases = CreateArray<int32>(ds, k_Phases, shape, 1, am->getId());
  REQUIRE(quats.getNumberOfTuples() == eulers.size());
  std::vector<float32> quatValues(eulers.size() * 4);
  if(phaseValues.empty())
  {
    phaseValues.assign(eulers.size(), 1);
  }
  REQUIRE(phaseValues.size() == eulers.size());
  for(usize i = 0; i < eulers.size(); ++i)
  {
    const auto& eu = eulers[i];
    const auto quat = ebsdlib::EulerDType(eu[0] * k_Radians, eu[1] * k_Radians, eu[2] * k_Radians).toQuaternion();
    for(usize j = 0; j < 4; ++j)
    {
      quatValues[4 * i + j] = i == zeroQuaternion ? 0.0F : static_cast<float32>(quat[j]);
    }
  }
  auto quatsWriteResult = quats.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(quatValues.data(), quatValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(quatsWriteResult);
  auto phasesWriteResult = phases.getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(phaseValues.data(), phaseValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(phasesWriteResult);
  auto* ensembles = AttributeMatrix::Create(ds, "Ensembles", {5});
  REQUIRE(ensembles != nullptr);
  auto& cs = CreateArray<uint32>(ds, k_CrystalStructures, {5}, 1, ensembles->getId());
  const std::vector<uint32> crystalValues = {ebsdlib::CrystalStructure::UnknownCrystalStructure, ebsdlib::CrystalStructure::Hexagonal_High, ebsdlib::CrystalStructure::Hexagonal_Low,
                                             ebsdlib::CrystalStructure::Cubic_High, ebsdlib::CrystalStructure::Trigonal_High};
  auto crystalWriteResult = cs.getDataStoreRef().copyFromBuffer(0, nonstd::span<const uint32>(crystalValues.data(), crystalValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(crystalWriteResult);
  return ds;
}

Arguments MakeArgs(const std::vector<float32>& reference = {0, 0, 1}, bool useMask = false)
{
  Arguments args;
  args.insertOrAssign(Filter::k_ReferenceDir_Key, std::make_any<VectorFloat32Parameter::ValueType>(reference));
  args.insertOrAssign(Filter::k_UseMask_Key, std::make_any<bool>(useMask));
  args.insertOrAssign(Filter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_Mask));
  args.insertOrAssign(Filter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_Quats));
  args.insertOrAssign(Filter::k_PhasesArrayPath_Key, std::make_any<DataPath>(k_Phases));
  args.insertOrAssign(Filter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructures));
  return args;
}

// The textbook third row of the Bunge matrix is independent of quaternion conversion.
double ExpectedAngle(const Euler& eu, const std::vector<float32>& reference)
{
  const double phi1 = eu[0] * k_Radians;
  const double phi = eu[1] * k_Radians;
  const std::array<double, 3> c = {std::sin(phi1) * std::sin(phi), -std::cos(phi1) * std::sin(phi), std::cos(phi)};
  const double norm = std::hypot(static_cast<double>(reference[0]), static_cast<double>(reference[1]), static_cast<double>(reference[2]));
  const double dot = (c[0] * reference[0] + c[1] * reference[1] + c[2] * reference[2]) / norm;
  return std::acos(std::clamp(std::abs(dot), 0.0, 1.0)) / k_Radians;
}

void CheckOutput(const DataStructure& ds, const std::vector<Euler>& eulers, const std::vector<float32>& reference, const std::vector<usize>& excluded = {})
{
  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(k_Output));
  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(k_Quats));
  const auto& output = ds.getDataRefAs<Float32Array>(k_Output);
  REQUIRE(output.getNumberOfComponents() == 1);
  REQUIRE(output.getIDataStoreRef().getTupleShape() == ds.getDataRefAs<Float32Array>(k_Quats).getIDataStoreRef().getTupleShape());
  std::vector<float32> outputValues(output.getSize());
  auto outputReadResult = output.getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(outputValues.data(), outputValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(outputReadResult);
  for(usize i = 0; i < eulers.size(); ++i)
  {
    CAPTURE(i);
    if(std::find(excluded.begin(), excluded.end(), i) != excluded.end())
    {
      REQUIRE(std::isnan(outputValues[i]));
    }
    else
    {
      REQUIRE(std::isfinite(outputValues[i]));
      REQUIRE(outputValues[i] >= 0.0F);
      REQUIRE(outputValues[i] <= 90.0F);
      REQUIRE(outputValues[i] == Approx(ExpectedAngle(eulers[i], reference)).margin(2.0e-4));
    }
  }
}

// The shared algorithm has no dispatch event. Use the scenario's store guard,
// but do not call executeFilter(), which requires a Direct/Scanline dispatch.
void ExecuteAndCheck(DataStructure& ds, const Arguments& args, const UnitTest::AlgorithmTestScope& scope)
{
  Filter filter;
  auto preflight = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().empty());
  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(k_Output));
  scope.requireExpectedStore(ds.getDataRefAs<Float32Array>(k_Output));
}
} // namespace

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: cell and feature analytical oracle", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const bool featureLevel = GENERATE(false, true);
  const double azimuthOffset = GENERATE(0.0, 43.0, 172.0);
  std::vector<Euler> eulers = {{0, 0, 71}, {22, 30, 15}, {147, 45, 92}, {271, 90, 13}, {81, 120, 245}};
  for(auto& eu : eulers)
  {
    eu[0] = std::fmod(eu[0] + azimuthOffset, 360.0);
    eu[2] = std::fmod(eu[2] + 2.0 * azimuthOffset, 360.0);
  }
  auto ds = MakeData(eulers, featureLevel ? ShapeType{5} : ShapeType{1, 1, 5});
  ExecuteAndCheck(ds, MakeArgs(), scope);
  CheckOutput(ds, eulers, {0, 0, 1});
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: nonunit reference oracle", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const auto reference = GENERATE(std::vector<float32>{3, 0, 3}, std::vector<float32>{1, 2, -2}, std::vector<float32>{-1, -2, 2});
  const std::vector<Euler> eulers = {{0, 0, 71}, {90, 45, 31}, {147, 45, 92}, {271, 90, 13}, {81, 120, 245}};
  auto ds = MakeData(eulers, {5});
  ExecuteAndCheck(ds, MakeArgs(reference), scope);
  CheckOutput(ds, eulers, reference);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: fixed seed sweep", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  std::mt19937 generator(1729);
  std::uniform_real_distribution<double> azimuth(0.0, 360.0);
  std::uniform_real_distribution<double> inclination(0.0, 180.0);
  std::uniform_real_distribution<float32> direction(-10.0F, 10.0F);
  std::vector<Euler> eulers(384);
  for(auto& eu : eulers)
  {
    eu = {azimuth(generator), inclination(generator), azimuth(generator)};
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
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const bool byteMask = GENERATE(false, true);
  // Cross two 4096-tuple boundaries and exercise a partial final chunk.
  std::vector<Euler> eulers(8201);
  for(usize i = 0; i < eulers.size(); ++i)
  {
    eulers[i] = {static_cast<double>(i % 360), static_cast<double>(i % 181), static_cast<double>((i * 7) % 360)};
  }
  auto ds = MakeData(eulers, {eulers.size()});
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(k_Quats.getParent()));
  const auto parent = ds.getDataRefAs<AttributeMatrix>(k_Quats.getParent()).getId();
  const std::vector<usize> excluded = {0, 4095, 4096, 8192, 8200};
  if(byteMask)
  {
    auto& mask = CreateArray<uint8>(ds, k_Mask, {eulers.size()}, 1, parent);
    mask.fill(255);
    for(auto i : excluded)
    {
      mask[i] = 0;
    }
  }
  else
  {
    auto& mask = CreateArray<bool>(ds, k_Mask, {eulers.size()}, 1, parent);
    mask.fill(true);
    for(auto i : excluded)
    {
      mask[i] = false;
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
  std::vector<Euler> eulers(k_NumTuples);
  for(usize i = 0; i < eulers.size(); ++i)
  {
    eulers[i] = {static_cast<double>(i % 360), static_cast<double>(i % 181), static_cast<double>((i * 7) % 360)};
  }
  std::vector<int32> phaseValues(k_NumTuples);
  usize expectedNonHexagonalCount = 0;
  std::vector<usize> excluded;
  for(usize i = 0; i < k_NumTuples; ++i)
  {
    // Alternate unknown, hexagonal high, hexagonal low, and cubic phases.
    phaseValues[i] = static_cast<int32>(i % 4);
    if(i % 11 != 0 && i % 4 == 3)
    {
      ++expectedNonHexagonalCount;
    }
    if(i % 11 == 0 || i % 4 == 0 || i % 4 == 3 || i == k_ZeroQuaternion)
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
  Filter filter;
  auto preflight = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().size() == 1);
  REQUIRE(result.result.warnings()[0].code == -98713);
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
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const std::vector<Euler> eulers(5, {23, 37, 89});
  auto ds = MakeData(eulers, {5});
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(k_Phases));
  auto& phases = ds.getDataRefAs<Int32Array>(k_Phases);
  phases[2] = 2;
  phases[3] = 0;
  usize expectedNonHexagonalCount = 0;
  std::vector<usize> excluded = {3};
  SECTION("Unknown crystal structure produces NaN without a warning")
  {
  }
  SECTION("Cubic phase produces NaN and counts only the known non-hexagonal tuple")
  {
    phases[1] = 3;
    expectedNonHexagonalCount = 1;
    excluded.push_back(1);
  }
  SECTION("Cubic and trigonal phases count both known non-hexagonal tuples")
  {
    phases[1] = 3;
    phases[4] = 4;
    expectedNonHexagonalCount = 2;
    excluded = {1, 3, 4};
  }
  auto result = Filter().execute(ds, MakeArgs());
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  CheckOutput(ds, eulers, {0, 0, 1}, excluded);
  if(expectedNonHexagonalCount == 0)
  {
    REQUIRE(result.result.warnings().empty());
  }
  else
  {
    REQUIRE(result.result.warnings().size() == 1);
    REQUIRE(result.result.warnings()[0].code == -98713);
    REQUIRE(result.result.warnings()[0].message.find(std::to_string(expectedNonHexagonalCount) + " tuples") == 0);
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: masked non-hexagonal tuples are not counted", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const bool byteMask = GENERATE(false, true);
  CAPTURE(byteMask);
  const std::vector<Euler> eulers(6, {23, 37, 89});
  auto ds = MakeData(eulers, {6});
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(k_Phases));
  auto& phases = ds.getDataRefAs<Int32Array>(k_Phases);
  phases[1] = 3;
  phases[2] = 4;
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(k_Quats.getParent()));
  const auto parent = ds.getDataRefAs<AttributeMatrix>(k_Quats.getParent()).getId();
  if(byteMask)
  {
    auto& mask = CreateArray<uint8>(ds, k_Mask, {6}, 1, parent);
    mask.fill(1);
    mask[1] = 0;
    mask[2] = 0;
  }
  else
  {
    auto& mask = CreateArray<bool>(ds, k_Mask, {6}, 1, parent);
    mask.fill(true);
    mask[1] = false;
    mask[2] = false;
  }
  usize expectedNonHexagonalCount = 0;
  std::vector<usize> excluded = {1, 2};
  SECTION("All non-hexagonal tuples masked")
  {
  }
  SECTION("One unmasked cubic tuple is counted")
  {
    phases[4] = 3;
    expectedNonHexagonalCount = 1;
    excluded.push_back(4);
  }
  auto result = Filter().execute(ds, MakeArgs({0, 0, 1}, true));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  CheckOutput(ds, eulers, {0, 0, 1}, excluded);
  if(expectedNonHexagonalCount == 0)
  {
    REQUIRE(result.result.warnings().empty());
  }
  else
  {
    REQUIRE(result.result.warnings().size() == 1);
    REQUIRE(result.result.warnings()[0].code == -98713);
    REQUIRE(result.result.warnings()[0].message.find("1 tuples") == 0);
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: invalid reference", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const auto reference =
      GENERATE(std::vector<float32>{0, 0, 0}, std::vector<float32>{k_NaN, 0, 1}, std::vector<float32>{0, std::numeric_limits<float32>::infinity(), 1}, std::vector<float32>{0, 0, 1.0e-13F});
  auto ds = MakeData({{0, 30, 0}}, {1});
  auto result = Filter().preflight(ds, MakeArgs(reference));
  REQUIRE(result.outputActions.invalid());
  REQUIRE(result.outputActions.errors()[0].code == -98710);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: tuple mismatch", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const bool mismatchMask = GENERATE(false, true);
  auto ds = MakeData({{0, 30, 0}, {0, 45, 0}}, {2});
  auto* other = AttributeMatrix::Create(ds, "Other", {3});
  REQUIRE(other != nullptr);
  const DataPath otherPath({"Other", "Mismatched"});
  auto args = MakeArgs({0, 0, 1}, mismatchMask);
  if(mismatchMask)
  {
    CreateArray<uint8>(ds, otherPath, {3}, 1, other->getId());
    args.insertOrAssign(Filter::k_MaskArrayPath_Key, std::make_any<DataPath>(otherPath));
  }
  else
  {
    CreateArray<int32>(ds, otherPath, {3}, 1, other->getId());
    args.insertOrAssign(Filter::k_PhasesArrayPath_Key, std::make_any<DataPath>(otherPath));
  }
  auto result = Filter().preflight(ds, args);
  REQUIRE(result.outputActions.invalid());
  REQUIRE(result.outputActions.errors()[0].code == -98711);
  const auto& message = result.outputActions.errors()[0].message;
  REQUIRE(message.find("Quats") != std::string::npos);
  REQUIRE(message.find("Mismatched") != std::string::npos);
  REQUIRE(message.find("2") != std::string::npos);
  REQUIRE(message.find("3") != std::string::npos);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: phase index error", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const int32 phase = GENERATE(-1, 5, std::numeric_limits<int32>::max());
  auto ds = MakeData({{0, 30, 0}, {0, 45, 0}}, {2});
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(k_Phases));
  ds.getDataRefAs<Int32Array>(k_Phases)[1] = phase;
  auto result = Filter().execute(ds, MakeArgs());
  REQUIRE(result.result.invalid());
  REQUIRE(result.result.errors()[0].code == -98712);
  const auto& message = result.result.errors()[0].message;
  REQUIRE(message.find("Phases") != std::string::npos);
  REQUIRE(message.find("CrystalStructures") != std::string::npos);
  REQUIRE(message.find(std::to_string(phase)) != std::string::npos);
  REQUIRE(message.find("tuple index 1") != std::string::npos);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: rejects Euler angle input", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  auto ds = MakeData({{0, 30, 0}, {0, 45, 0}}, {2});
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(k_Quats.getParent()));
  const auto parent = ds.getDataRefAs<AttributeMatrix>(k_Quats.getParent()).getId();
  const DataPath eulersPath({"Orientations", "EulerAngles"});
  CreateArray<float32>(ds, eulersPath, {2}, 3, parent);
  auto args = MakeArgs();
  args.insertOrAssign(Filter::k_QuatsArrayPath_Key, std::make_any<DataPath>(eulersPath));
  auto result = Filter().preflight(ds, args);
  REQUIRE(result.outputActions.invalid());
  REQUIRE(ds.getDataAs<Float32Array>(k_Output) == nullptr);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: zero quaternion is undefined", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const usize count = GENERATE(1, 4096, 4097, 8192);
  CAPTURE(count);
  const std::vector<Euler> eulers(count, {23, 37, 89});
  auto ds = MakeData(eulers, {count});
  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(k_Quats));
  auto& quats = ds.getDataRefAs<Float32Array>(k_Quats);
  for(usize component = 0; component < 4; ++component)
  {
    quats[4 * (count - 1) + component] = 0.0F;
  }
  ExecuteAndCheck(ds, MakeArgs(), scope);
  // A zero quaternion defines no orientation, so it cannot have a finite angle.
  CAPTURE(ds.getDataRefAs<Float32Array>(k_Output)[count - 1]);
  CheckOutput(ds, eulers, {0, 0, 1}, {count - 1});
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: nonfinite quaternions", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const usize component = GENERATE(0, 1, 2, 3);
  const float32 value = GENERATE(as<float32>{}, k_NaN, std::numeric_limits<float32>::infinity(), -std::numeric_limits<float32>::infinity());
  CAPTURE(component, value);
  REQUIRE_FALSE(std::isfinite(value));
  const std::vector<Euler> eulers(2, {23, 37, 89});
  auto ds = MakeData(eulers, {2});
  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(k_Quats));
  ds.getDataRefAs<Float32Array>(k_Quats)[component] = value;
  ExecuteAndCheck(ds, MakeArgs(), scope);
  CheckOutput(ds, eulers, {0, 0, 1}, {0});
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: quaternion scale invariance", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const float32 scale = GENERATE(-2.0F, 0.5F, 1.0e-30F, 1.0e30F);
  CAPTURE(scale);
  const std::vector<Euler> eulers = {{0, 0, 71}, {90, 45, 31}, {147, 45, 92}, {271, 90, 13}, {81, 120, 245}};
  auto ds = MakeData(eulers, {eulers.size()});
  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(k_Quats));
  auto& quats = ds.getDataRefAs<Float32Array>(k_Quats);
  for(usize index = 0; index < quats.getSize(); ++index)
  {
    quats[index] *= scale;
  }
  ExecuteAndCheck(ds, MakeArgs({1, 2, -2}), scope);
  CheckOutput(ds, eulers, {1, 2, -2});
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisReferenceMisalignments: empty input", "[OrientationAnalysis][ComputeCAxisReferenceMisalignments]")
{
  UnitTest::LoadPlugins();
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
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const bool cancelBeforeExecute = GENERATE(false, true);
  CAPTURE(cancelBeforeExecute);
  std::atomic_bool shouldCancel = cancelBeforeExecute;
  const std::vector<Euler> eulers(4097, {23, 37, 89});
  auto ds = MakeData(eulers, {eulers.size()});
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(k_Phases));
  // If cancellation is ignored at the next boundary, this produces -98712.
  ds.getDataRefAs<Int32Array>(k_Phases)[4096] = -1;
  const IFilter::MessageHandler handler{[&shouldCancel](const IFilter::Message& message) {
    if(message.type == IFilter::Message::Type::Progress)
    {
      shouldCancel = true;
    }
  }};
  auto result = Filter().execute(ds, MakeArgs(), nullptr, handler, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors()[0].code == -1);
  if(cancelBeforeExecute)
  {
    REQUIRE(ds.getDataAs<Float32Array>(k_Output) == nullptr);
  }
  else
  {
    REQUIRE(shouldCancel.load());
    CheckOutput(ds, std::vector<Euler>(4096, eulers.front()), {0, 0, 1});
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}
