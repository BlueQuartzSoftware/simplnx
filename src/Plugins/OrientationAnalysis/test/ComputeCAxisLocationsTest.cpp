#include <array>
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>
#include <vector>

#include <EbsdLib/Core/EbsdLibConstants.h>

#include "simplnx/Common/Constants.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include "OrientationAnalysis/Filters/ComputeCAxisLocationsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace nx::core;
using namespace nx::core::Constants;
namespace fs = std::filesystem;

namespace
{
struct FixtureData
{
  std::array<float32, 4> inputQuat{};
  std::array<float32, 3> expectedOutput{};
};

constexpr float32 k_Sin_OneEighthPiF = 0.38268343f;
constexpr float32 k_Sin_OneEighthPi_OverSqrt3F = k_Sin_OneEighthPiF / k_Sqrt3F;

const std::vector<FixtureData> k_Fixtures = {
    // no rotation
    {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
    // +90 about x
    {{k_HalfSqrt2F, 0.0f, 0.0f, k_HalfSqrt2F}, {0.0f, 1.0f, 0.0f}},
    // +90 about y
    {{0.0f, k_HalfSqrt2F, 0.0f, k_HalfSqrt2F}, {-1.0f, 0.0f, 0.0f}},
    // +90 about z
    {{0.0f, 0.0f, k_HalfSqrt2F, k_HalfSqrt2F}, {0.0f, 0.0f, 1.0f}},
    // 180 about x; flip z sign
    {{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    // 180 about y; flip z sign
    {{0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    // 180 about z
    {{0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    // 120 about (1, 1, 1)
    {{0.5f, 0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    // +45 about x
    {{k_Sin_OneEighthPiF, 0.0f, 0.0f, k_Cos_OneEighthPiF}, {0.0f, k_HalfSqrt2F, k_HalfSqrt2F}},
    // +45 about y
    {{0.0f, k_Sin_OneEighthPiF, 0.0f, k_Cos_OneEighthPiF}, {-k_HalfSqrt2F, 0.0f, k_HalfSqrt2F}},
    // +45 about z
    {{0.0f, 0.0f, k_Sin_OneEighthPiF, k_Cos_OneEighthPiF}, {0.0f, 0.0f, 1.0f}},
    // -45 about x
    {{-k_Sin_OneEighthPiF, 0.0f, 0.0f, k_Cos_OneEighthPiF}, {0.0f, -k_HalfSqrt2F, k_HalfSqrt2F}},
    // -45 about y
    {{0.0f, -k_Sin_OneEighthPiF, 0.0f, k_Cos_OneEighthPiF}, {k_HalfSqrt2F, 0.0f, k_HalfSqrt2F}},
    // -45 about z
    {{0.0f, 0.0f, -k_Sin_OneEighthPiF, k_Cos_OneEighthPiF}, {0.0f, 0.0f, 1.0f}},
    // +45 around (1, 1, 1); v_out = ((2 - sqrt(6) - sqrt(2))/6, (2 + sqrt(6) - sqrt(2))/6, (1 + sqrt(2))/3)
    {{k_Sin_OneEighthPi_OverSqrt3F, k_Sin_OneEighthPi_OverSqrt3F, k_Sin_OneEighthPi_OverSqrt3F, k_Cos_OneEighthPiF}, {-0.3106172, 0.50587934, 0.80473787}},
};

template <class T>
bool ContainsCode(const std::vector<T>& vec, int32 code)
{
  return std::find_if(vec.cbegin(), vec.cend(), [code](const T& item) { return item.code == code; }) != vec.cend();
}
} // namespace

TEST_CASE("OrientationAnalysis::ComputeCAxisLocationsFilter: Preflight Error - Cell array tuple count mismatch (-3520)", "[OrientationAnalysis][ComputeCAxisLocationsFilter][preflight]")
{
  UnitTest::LoadPlugins();

  // Build a minimal synthetic DataStructure where the two cell-level arrays that are
  // validated together (Quats, CellPhases) do NOT share the same tuple count. This drives
  // the validateNumberOfTuples() guard in preflightImpl that emits error -3520.
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "DataContainer");
  imageGeom->setDimensions({10, 1, 1});

  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", {10}, imageGeom->getId());
  UnitTest::CreateTestDataArray<float32>(dataStructure, "Quats", {10}, {4}, cellAM->getId());

  // CellPhases lives in a separate AttributeMatrix with a deliberately different tuple
  // count (9 != 10) so the cross-array tuple-count check fails.
  auto* mismatchAM = AttributeMatrix::Create(dataStructure, "MismatchData", {9}, imageGeom->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {9}, {1}, mismatchAM->getId());

  auto* ensembleAM = AttributeMatrix::Create(dataStructure, "CellEnsembleData", {2}, imageGeom->getId());
  UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1}, ensembleAM->getId());

  ComputeCAxisLocationsFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Quats"})));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "MismatchData", "Phases"})));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellEnsembleData", "CrystalStructures"})));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CAxisLocationsArrayName_Key, std::make_any<std::string>("CAxisLocation"));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -3520);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisLocationsFilter: No hexagonal phases error", "[OrientationAnalysis][ComputeCAxisLocationsFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  UnitTest::CreateTestDataArray<float32>(dataStructure, "Quats", {10}, {4});
  UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {10}, {1});

  // Create crystal structures array with no hexagonal phases
  UInt32Array* crystalStructures = UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1});
  crystalStructures->setValue(0, ebsdlib::CrystalStructure::UnknownCrystalStructure);
  crystalStructures->setValue(1, ebsdlib::CrystalStructure::Cubic_High);

  ComputeCAxisLocationsFilter filter;
  Arguments args;

  args.insertOrAssign(ComputeCAxisLocationsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(std::vector<std::string>{"Quats"}));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(std::vector<std::string>{"Phases"}));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(std::vector<std::string>{"CrystalStructures"}));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CAxisLocationsArrayName_Key, std::make_any<std::string>("CAxisLocations"));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);

  REQUIRE(ContainsCode(executeResult.result.errors(), -3522));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisLocationsFilter: Not all hexagonal phases warning", "[OrientationAnalysis][ComputeCAxisLocationsFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  UnitTest::CreateTestDataArray<float32>(dataStructure, "Quats", {10}, {4});

  // Create crystal structures array with some non-hexagonal phases
  UInt32Array* crystalStructures = UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {3}, {1});
  crystalStructures->setValue(0, ebsdlib::CrystalStructure::UnknownCrystalStructure);
  crystalStructures->setValue(1, ebsdlib::CrystalStructure::Hexagonal_High);
  crystalStructures->setValue(2, ebsdlib::CrystalStructure::Cubic_High);

  // All non-hexagonal phases
  Int32Array* phases = UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {10}, {1});
  for(usize i = 0; i < phases->getSize(); i++)
  {
    phases->setValue(i, 2);
  }

  DataPath cAxisLocationsPath({"CAxisLocations"});

  ComputeCAxisLocationsFilter filter;
  Arguments args;

  args.insertOrAssign(ComputeCAxisLocationsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(std::vector<std::string>{"Quats"}));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(std::vector<std::string>{"Phases"}));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(std::vector<std::string>{"CrystalStructures"}));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CAxisLocationsArrayName_Key, std::make_any<std::string>(cAxisLocationsPath.getTargetName()));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Check for unconditional preflight warning about ensuring nonhexagonal data
  REQUIRE(ContainsCode(preflightResult.outputActions.warnings(), -3521));

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE(ContainsCode(executeResult.result.warnings(), -3523));

  REQUIRE(dataStructure.containsData(cAxisLocationsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(cAxisLocationsPath));
  const auto& cAxisLocations = dataStructure.getDataRefAs<Float32Array>(cAxisLocationsPath);
  REQUIRE(std::all_of(cAxisLocations.cbegin(), cAxisLocations.cend(), [](float32 value) { return std::isnan(value); }));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisLocationsFilter: Class 1 Oracle", "[OrientationAnalysis][ComputeCAxisLocationsFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;

  const usize size = k_Fixtures.size();

  Float32Array* quats = UnitTest::CreateTestDataArray<float32>(dataStructure, "Quats", {size}, {4});

  for(usize i = 0; i < size; i++)
  {
    for(usize j = 0; j < 4; j++)
    {
      quats->setComponent(i, j, k_Fixtures[i].inputQuat[j]);
    }
  }

  UInt32Array* crystalStructures = UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1});
  crystalStructures->setValue(0, ebsdlib::CrystalStructure::UnknownCrystalStructure);
  crystalStructures->setValue(1, ebsdlib::CrystalStructure::Hexagonal_High);

  Int32Array* phases = UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {size}, {1});
  for(usize i = 0; i < phases->getSize(); i++)
  {
    phases->setValue(i, 1);
  }

  DataPath cAxisLocationsPath({"CAxisLocations"});

  ComputeCAxisLocationsFilter filter;
  Arguments args;

  args.insertOrAssign(ComputeCAxisLocationsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(std::vector<std::string>{"Quats"}));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(std::vector<std::string>{"Phases"}));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(std::vector<std::string>{"CrystalStructures"}));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CAxisLocationsArrayName_Key, std::make_any<std::string>(cAxisLocationsPath.getTargetName()));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE(dataStructure.containsData(cAxisLocationsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(cAxisLocationsPath));
  auto& cAxisLocations = dataStructure.getDataRefAs<Float32Array>(cAxisLocationsPath);
  for(usize i = 0; i < size; i++)
  {
    for(usize j = 0; j < 3; j++)
    {
      INFO(fmt::format("i = {} | j = {} | input_quat = ({}) | expected_vec = ({})", i, j, fmt::join(k_Fixtures[i].inputQuat, ", "), fmt::join(k_Fixtures[i].expectedOutput, ", ")));
      REQUIRE(cAxisLocations.getComponent(i, j) == Approx(k_Fixtures[i].expectedOutput[j]).margin(1e-7f));
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisLocationsFilter: Class 1 Oracle - Mixed hexagonal and non-hexagonal phases", "[OrientationAnalysis][ComputeCAxisLocationsFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;

  const usize size = k_Fixtures.size();

  Float32Array* quats = UnitTest::CreateTestDataArray<float32>(dataStructure, "Quats", {size}, {4});

  for(usize i = 0; i < size; i++)
  {
    for(usize j = 0; j < 4; j++)
    {
      quats->setComponent(i, j, k_Fixtures[i].inputQuat[j]);
    }
  }

  UInt32Array* crystalStructures = UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {3}, {1});
  crystalStructures->setValue(0, ebsdlib::CrystalStructure::UnknownCrystalStructure);
  crystalStructures->setValue(1, ebsdlib::CrystalStructure::Hexagonal_High);
  crystalStructures->setValue(2, ebsdlib::CrystalStructure::Cubic_High);

  // Alternate hexagonal and cubic cells in one execution.
  // This checks computed c-axes, non-hexagonal NaNs, and neighboring values.
  Int32Array* phases = UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {size}, {1});
  for(usize i = 0; i < size; i++)
  {
    phases->setValue(i, i % 2 == 0 ? 1 : 2);
  }

  DataPath cAxisLocationsPath({"CAxisLocations"});

  ComputeCAxisLocationsFilter filter;
  Arguments args;

  args.insertOrAssign(ComputeCAxisLocationsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(std::vector<std::string>{"Quats"}));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(std::vector<std::string>{"Phases"}));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(std::vector<std::string>{"CrystalStructures"}));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CAxisLocationsArrayName_Key, std::make_any<std::string>(cAxisLocationsPath.getTargetName()));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE(ContainsCode(executeResult.result.warnings(), -3523));

  REQUIRE(dataStructure.containsData(cAxisLocationsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(cAxisLocationsPath));
  auto& cAxisLocations = dataStructure.getDataRefAs<Float32Array>(cAxisLocationsPath);
  for(usize i = 0; i < size; i++)
  {
    const bool isHexCell = i % 2 == 0;
    for(usize j = 0; j < 3; j++)
    {
      INFO(fmt::format("i = {} | j = {} | phase = {} | input_quat = ({}) | expected_vec = ({})", i, j, phases->getValue(i), fmt::join(k_Fixtures[i].inputQuat, ", "),
                       fmt::join(k_Fixtures[i].expectedOutput, ", ")));
      if(isHexCell)
      {
        REQUIRE(cAxisLocations.getComponent(i, j) == Approx(k_Fixtures[i].expectedOutput[j]).margin(1e-7f));
      }
      else
      {
        REQUIRE(std::isnan(cAxisLocations.getComponent(i, j)));
      }
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisLocationsFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeCAxisLocationsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeCAxisLocationsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeCAxisLocationsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeCAxisLocationsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeCAxisLocationsFilter::k_QuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeCAxisLocationsFilter::k_CAxisLocationsArrayName_Key) == "TestName");
    }
  }
}

TEST_CASE("OrientationAnalysis::ComputeCAxisLocationsFilter: Phase Index Bounds", "[OrientationAnalysis][ComputeCAxisLocationsFilter]")
{
  UnitTest::LoadPlugins();

  const int32 invalidPhaseIdx = GENERATE(-1, 2);
  CAPTURE(invalidPhaseIdx);
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);

  DataStructure dataStructure;
  auto quatsStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, DataPath({"Quats"}), {2}, {4});
  auto* quatsArrayPtr = Float32Array::Create(dataStructure, "Quats", quatsStore);
  if(Application::Instance()->getIOManager("HDF5-OOC") != nullptr)
  {
    REQUIRE(quatsArrayPtr->getDataStoreRef().getDataFormat() == "HDF5-OOC");
  }
  quatsArrayPtr->fill(0.0F);
  (*quatsArrayPtr)[3] = 1.0F;
  (*quatsArrayPtr)[7] = 1.0F;

  auto cellPhasesStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, DataPath({"Phases"}), {2}, {1});
  auto* cellPhasesArrayPtr = Int32Array::Create(dataStructure, "Phases", cellPhasesStore);
  (*cellPhasesArrayPtr)[0] = 1;
  (*cellPhasesArrayPtr)[1] = invalidPhaseIdx;

  auto crystalStructuresStore = DataStoreUtilities::CreateDataStore<uint32>(dataStructure, DataPath({"CrystalStructures"}), {2}, {1});
  auto* crystalStructuresArrayPtr = UInt32Array::Create(dataStructure, "CrystalStructures", crystalStructuresStore);
  (*crystalStructuresArrayPtr)[0] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
  (*crystalStructuresArrayPtr)[1] = ebsdlib::CrystalStructure::Hexagonal_High;

  ComputeCAxisLocationsFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(DataPath({"Quats"})));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"Phases"})));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"CrystalStructures"})));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CAxisLocationsArrayName_Key, std::make_any<std::string>("CAxisLocations"));

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -3524);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisLocationsFilter: genuine HDF5 65536-page tail oracle", "[OrientationAnalysis][ComputeCAxisLocationsFilter][.OocStoreContract]")
{
  UnitTest::LoadPlugins();
  REQUIRE(Application::Instance()->getIOManager("HDF5-OOC") != nullptr);

  constexpr usize k_PageTuples = 65536;
  constexpr usize k_CellTuples = k_PageTuples + 1;
  constexpr usize k_LastFullPageTuple = k_PageTuples - 1;
  constexpr usize k_TailTuple = k_PageTuples;
  constexpr float32 k_Tolerance = 1.0E-6F;

  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);

  const DataPath quatsPath({"Quats"});
  const DataPath phasesPath({"Phases"});
  const DataPath crystalStructuresPath({"CrystalStructures"});
  const DataPath cAxisLocationsPath({"CAxisLocations"});

  DataStructure dataStructure;
  auto quatsStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, quatsPath, {k_CellTuples}, {4});
  auto phasesStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, phasesPath, {k_CellTuples}, {1});
  auto crystalStructuresStore = DataStoreUtilities::CreateDataStore<uint32>(dataStructure, crystalStructuresPath, {2}, {1});
  auto* quats = Float32Array::Create(dataStructure, quatsPath.getTargetName(), quatsStore);
  auto* phases = Int32Array::Create(dataStructure, phasesPath.getTargetName(), phasesStore);
  auto* crystalStructures = UInt32Array::Create(dataStructure, crystalStructuresPath.getTargetName(), crystalStructuresStore);
  REQUIRE(quats != nullptr);
  REQUIRE(phases != nullptr);
  REQUIRE(crystalStructures != nullptr);

  std::vector<float32> quaternionValues(k_CellTuples * 4, 0.0F);
  std::vector<int32> phaseValues(k_CellTuples, 1);
  for(usize tupleIdx = 0; tupleIdx < k_CellTuples; tupleIdx++)
  {
    quaternionValues[tupleIdx * 4 + 3] = 1.0F;
  }

  quaternionValues[k_LastFullPageTuple * 4 + 1] = k_Sin_OneEighthPiF;
  quaternionValues[k_LastFullPageTuple * 4 + 3] = k_Cos_OneEighthPiF;
  quaternionValues[k_TailTuple * 4] = k_HalfSqrt2F;
  quaternionValues[k_TailTuple * 4 + 3] = k_HalfSqrt2F;
  const std::array<uint32, 2> crystalStructureValues = {ebsdlib::CrystalStructure::UnknownCrystalStructure, ebsdlib::CrystalStructure::Hexagonal_High};

  auto quatsWriteResult = quats->getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(quaternionValues.data(), quaternionValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(quatsWriteResult);
  auto phasesWriteResult = phases->getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(phaseValues.data(), phaseValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(phasesWriteResult);
  auto crystalStructuresWriteResult = crystalStructures->getDataStoreRef().copyFromBuffer(0, nonstd::span<const uint32>(crystalStructureValues.data(), crystalStructureValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(crystalStructuresWriteResult);

  REQUIRE(quats->getDataStoreRef().getDataFormat() == "HDF5-OOC");
  REQUIRE(phases->getDataStoreRef().getDataFormat() == "HDF5-OOC");

  ComputeCAxisLocationsFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(quatsPath));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(phasesPath));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructuresPath));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CAxisLocationsArrayName_Key, std::make_any<std::string>(cAxisLocationsPath.getTargetName()));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE(ContainsCode(executeResult.result.warnings(), -3521));
  REQUIRE_FALSE(ContainsCode(executeResult.result.warnings(), -3523));

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(cAxisLocationsPath));
  const auto& cAxisLocations = dataStructure.getDataRefAs<Float32Array>(cAxisLocationsPath);
  REQUIRE(cAxisLocations.getDataStoreRef().getDataFormat() == "HDF5-OOC");
  REQUIRE(cAxisLocations.getTupleShape() == ShapeType{k_CellTuples});
  REQUIRE(cAxisLocations.getComponentShape() == ShapeType{3});

  const usize lastFullPageOffset = k_LastFullPageTuple * 3;
  REQUIRE(cAxisLocations[lastFullPageOffset] == Approx(-k_HalfSqrt2F).margin(k_Tolerance));
  REQUIRE(cAxisLocations[lastFullPageOffset + 1] == Approx(0.0F).margin(k_Tolerance));
  REQUIRE(cAxisLocations[lastFullPageOffset + 2] == Approx(k_HalfSqrt2F).margin(k_Tolerance));

  const usize tailOffset = k_TailTuple * 3;
  REQUIRE(cAxisLocations[tailOffset] == Approx(0.0F).margin(k_Tolerance));
  REQUIRE(cAxisLocations[tailOffset + 1] == Approx(1.0F).margin(k_Tolerance));
  REQUIRE(cAxisLocations[tailOffset + 2] == Approx(0.0F).margin(k_Tolerance));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
