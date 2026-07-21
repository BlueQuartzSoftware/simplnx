#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

#include <EbsdLib/Core/EbsdLibConstants.h>

#include "simplnx/Common/Constants.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

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
}

TEST_CASE("OrientationAnalysis::ComputeCAxisLocationsFilter: No hexagonal phases error", "[OrientationAnalysis][ComputeCAxisLocationsFilter]")
{
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
}

TEST_CASE("OrientationAnalysis::ComputeCAxisLocationsFilter: Not all hexagonal phases warning", "[OrientationAnalysis][ComputeCAxisLocationsFilter]")
{
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

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE(ContainsCode(executeResult.result.warnings(), -3523));

  REQUIRE(dataStructure.containsData(cAxisLocationsPath));
  const auto& cAxisLocations = dataStructure.getDataRefAs<Float32Array>(cAxisLocationsPath);
  REQUIRE(std::all_of(cAxisLocations.cbegin(), cAxisLocations.cend(), [](float32 value) { return std::isnan(value); }));
}

TEST_CASE("OrientationAnalysis::ComputeCAxisLocationsFilter: Class 1 Oracle", "[OrientationAnalysis][ComputeCAxisLocationsFilter]")
{
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
  auto& cAxisLocations = dataStructure.getDataRefAs<Float32Array>(cAxisLocationsPath);
  for(usize i = 0; i < size; i++)
  {
    for(usize j = 0; j < 3; j++)
    {
      INFO(fmt::format("i = {} | j = {} | input_quat = ({}) | expected_vec = ({})", i, j, fmt::join(k_Fixtures[i].inputQuat, ", "), fmt::join(k_Fixtures[i].expectedOutput, ", ")));
      REQUIRE(cAxisLocations.getComponent(i, j) == Approx(k_Fixtures[i].expectedOutput[j]));
    }
  }
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
