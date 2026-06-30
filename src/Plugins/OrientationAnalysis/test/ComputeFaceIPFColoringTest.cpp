#include "OrientationAnalysis/Filters/ComputeFaceIPFColoringFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>

using namespace nx::core;
using namespace nx::core::UnitTest;
namespace fs = std::filesystem;

namespace
{
constexpr StringLiteral k_FirstNXFaceIPFColors("NXFaceIPFColors 0");
constexpr StringLiteral k_SecondNXFaceIPFColors("NXFaceIPFColors 1");

DataPath smallIn100Group({nx::core::Constants::k_SmallIN100});
DataPath featureDataPath = smallIn100Group.createChildPath(nx::core::Constants::k_Grain_Data);
DataPath avgEulerAnglesPath = featureDataPath.createChildPath(nx::core::Constants::k_AvgEulerAngles);
DataPath featurePhasesPath = featureDataPath.createChildPath(nx::core::Constants::k_Phases);
DataPath crystalStructurePath = smallIn100Group.createChildPath(nx::core::Constants::k_Phase_Data).createChildPath(nx::core::Constants::k_CrystalStructures);
DataPath avgQuatsPath = featureDataPath.createChildPath("AvgQuats");

DataPath triangleDataContainerPath({nx::core::Constants::k_TriangleDataContainerName});
DataPath faceDataGroup = triangleDataContainerPath.createChildPath(nx::core::Constants::k_FaceData);

DataPath faceLabels = faceDataGroup.createChildPath(nx::core::Constants::k_FaceLabels);
DataPath faceNormals = faceDataGroup.createChildPath(nx::core::Constants::k_FaceNormals);
DataPath faceAreas = faceDataGroup.createChildPath(nx::core::Constants::k_FaceAreas);
} // namespace

// =============================================================================
// Class 1 (Analytical) oracle for issue #1635.
//
// The filter colors each side of a surface-mesh face with the IPF color of the
// adjacent feature: the FIRST color uses feature1's orientation + the face
// normal; the SECOND color uses feature2's orientation + the *negated* face
// normal. Each side must use ITS OWN phase's Laue symmetry operator.
//
// Oracle: IPF colors are a closed-form function of (orientation, reference
// direction, Laue symmetry). At the standard-triangle corners the color is a
// pure primary, independent of any implementation detail (EbsdLib 3.0.0,
// TSL color key):
//   - Cubic-high <100>  -> (255,  0,  0)  (red corner)
//   - Cubic-high <111>  -> (  0,  0,255)  (blue corner)
//   - Hex-high  c-axis  -> (255,  0,  0)  (red corner)
//   - Hex-high  basal   -> (  0,255,  0)  (green corner; RED channel is exactly
//                                          0 because a basal direction sits at
//                                          chi = chiMax, so r = 1 - chi/chiMax = 0)
// The crystal-structure enum values that index ops[] are Hexagonal_High = 0,
// Cubic_High = 1 (EbsdLib/Core/EbsdLibConstants.h).
//
// The fixture (identity orientations throughout) builds four faces:
//   Face 0  labels (1,2)  n=(-1,0,0)  mixed cubic/hex  -- the headline discriminator
//   Face 1  labels (-1,2) n=(-1,0,0)  feature1 invalid -- boundary discriminator
//   Face 2  labels (1,-1) n=(-1,0,0)  feature2 invalid -- control (second black)
//   Face 3  labels (1,3)  n=(1,1,1)   same-phase cubic -- control (bug cannot manifest)
//
// Why each side's expected color is what it is (FIX = correct behavior):
//   F0 first  = cubic(+(-1,0,0)) = <100> red   = (255,  0,  0)
//   F0 second = hex (-(-1,0,0)=(1,0,0)) basal   = (  0,255,  0)   [BUG -> cubic <100> = (255,0,0)]
//   F1 first  = black (feature1 = -1 -> phase1 = 0)              = (  0,  0,  0)
//   F1 second = hex (1,0,0) basal                = (  0,255,  0)   [BUG -> guard reads CrystalStructures[0]
//                                                                   sentinel, fails, leaves black (0,0,0)]
//   F2 first  = cubic(-1,0,0) = <100> red        = (255,  0,  0)
//   F2 second = black (feature2 = -1 -> phase2 = 0)             = (  0,  0,  0)
//   F3 first  = cubic(1,1,1) = <111> blue        = (  0,  0,255)
//   F3 second = cubic(-(1,1,1)) = <111> blue      = (  0,  0,255)  [phase1==phase2, bug cannot manifest]
//
// Faces 0 and 1 are the discriminators: under the pre-fix bug both their SECOND
// colors are wrong (F0 -> red, F1 -> black); after the fix they are green.
TEST_CASE("OrientationAnalysis::ComputeFaceIPFColoringFilter: Class 1 Oracle - mixed-phase analytical", "[OrientationAnalysis][ComputeFaceIPFColoringFilter]")
{
  UnitTest::LoadPlugins();

  // EbsdLib crystal-structure enum values used as ops[] indices.
  constexpr uint32 k_HexHigh = 0;   // Hexagonal_High (6/mmm)
  constexpr uint32 k_CubicHigh = 1; // Cubic_High (m-3m)
  constexpr uint32 k_Unknown = 999; // UnknownCrystalStructure sentinel

  DataStructure dataStructure;
  auto* topGroup = DataGroup::Create(dataStructure, "Data");

  const DataPath faceLabelsPath({"Data", "FaceLabels"});
  const DataPath faceNormalsPath({"Data", "FaceNormals"});
  const DataPath eulerAnglesPath({"Data", "FeatureEulerAngles"});
  const DataPath phasesPath({"Data", "FeaturePhases"});
  const DataPath crystalStructuresPath({"Data", "CrystalStructures"});

  // 4 faces, 2-component (feature1, feature2)
  auto* faceLabelsArray = UnitTest::CreateTestDataArray<int32>(dataStructure, "FaceLabels", {4}, {2}, topGroup->getId());
  const std::vector<int32> faceLabelValues = {1, 2, -1, 2, 1, -1, 1, 3};
  std::copy(faceLabelValues.begin(), faceLabelValues.end(), faceLabelsArray->begin());

  // 4 faces, 3-component face normals (not normalized; EbsdLib normalizes internally)
  auto* faceNormalsArray = UnitTest::CreateTestDataArray<float64>(dataStructure, "FaceNormals", {4}, {3}, topGroup->getId());
  const std::vector<float64> faceNormalValues = {-1.0, 0.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 0.0, 1.0, 1.0, 1.0};
  std::copy(faceNormalValues.begin(), faceNormalValues.end(), faceNormalsArray->begin());

  // 4 features (index 0 unused), identity Euler angles throughout
  auto* eulerAngles = UnitTest::CreateTestDataArray<float32>(dataStructure, "FeatureEulerAngles", {4}, {3}, topGroup->getId());
  std::fill(eulerAngles->begin(), eulerAngles->end(), 0.0F);

  // Feature phases: feature 1 -> phase 1 (cubic), feature 2 -> phase 2 (hex), feature 3 -> phase 1 (cubic)
  auto* phases = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeaturePhases", {4}, {1}, topGroup->getId());
  const std::vector<int32> phaseValues = {0, 1, 2, 1};
  std::copy(phaseValues.begin(), phaseValues.end(), phases->begin());

  // Crystal structures per phase: phase 0 -> unknown, phase 1 -> cubic-high, phase 2 -> hex-high
  auto* crystalStructures = UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {3}, {1}, topGroup->getId());
  (*crystalStructures)[0] = k_Unknown;
  (*crystalStructures)[1] = k_CubicHigh;
  (*crystalStructures)[2] = k_HexHigh;

  ComputeFaceIPFColoringFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabelsPath));
  args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(faceNormalsPath));
  args.insertOrAssign(ComputeFaceIPFColoringFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(eulerAnglesPath));
  args.insertOrAssign(ComputeFaceIPFColoringFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(phasesPath));
  args.insertOrAssign(ComputeFaceIPFColoringFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructuresPath));
  args.insertOrAssign(ComputeFaceIPFColoringFilter::k_FirstFaceIPFColorsArrayName_Key, std::make_any<std::string>(::k_FirstNXFaceIPFColors));
  args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SecondFaceIPFColorsArrayName_Key, std::make_any<std::string>(::k_SecondNXFaceIPFColors));
  args.insertOrAssign(ComputeFaceIPFColoringFilter::k_ColorKey_Key, std::make_any<ChoicesParameter::ValueType>(0)); // TSL

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& first = dataStructure.getDataRefAs<UInt8Array>(faceLabelsPath.replaceName(::k_FirstNXFaceIPFColors));
  const auto& second = dataStructure.getDataRefAs<UInt8Array>(faceLabelsPath.replaceName(::k_SecondNXFaceIPFColors));

  // Expected colors (the FIX = correct behavior). Each row is one face.
  const std::array<std::array<uint8, 3>, 4> expectedFirst = {{{255, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 255}}};
  const std::array<std::array<uint8, 3>, 4> expectedSecond = {{{0, 255, 0}, {0, 255, 0}, {0, 0, 0}, {0, 0, 255}}};

  for(usize face = 0; face < 4; face++)
  {
    INFO(fmt::format("face {}", face));
    REQUIRE(first[face * 3 + 0] == expectedFirst[face][0]);
    REQUIRE(first[face * 3 + 1] == expectedFirst[face][1]);
    REQUIRE(first[face * 3 + 2] == expectedFirst[face][2]);

    REQUIRE(second[face * 3 + 0] == expectedSecond[face][0]);
    REQUIRE(second[face * 3 + 1] == expectedSecond[face][1]);
    REQUIRE(second[face * 3 + 2] == expectedSecond[face][2]);
  }

  // Class 4 companion invariant: the Phase-2 (hex) side of a basal-plane face has
  // a RED channel of exactly 0. The pre-fix bug colored it with cubic <100> ops,
  // which produces (255,0,0) -- a non-zero red channel. This single byte is the
  // crispest signature of the wrong-Laue-operator bug.
  REQUIRE(second[0 * 3 + 0] == 0); // Face 0 hex second color, red channel

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeFaceIPFColoringFilter: Invalid filter execution", "[OrientationAnalysis][ComputeFaceIPFColoringFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeFaceIPFColoringFilter filter;
  Arguments args;

  SECTION("Inconsistent face data tuple dimensions")
  {
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabels));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(avgQuatsPath));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(avgEulerAnglesPath));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(featurePhasesPath));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructurePath));
  }
  SECTION("Inconsistent cell data tuple dimensions")
  {
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabels));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(faceNormals));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(avgEulerAnglesPath));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(faceAreas));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructurePath));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
// Plumbing test: the k_ColorKey_Key choice index must route through executeImpl's
// switch into the right `ebsdlib::ColorKeyKind` and reach generateIPFColor. The
// per-Laue-class correctness of TSL / PUCM / Nolze-Hielscher is covered by
// EbsdLib's ColorKeyKindTest; here we only assert that the simplnx side wiring
// is intact -- non-default choices must produce a different output array than
// the default (TSL) run on the same input data.
TEST_CASE("OrientationAnalysis::ComputeFaceIPFColoringFilter: ColorKey choice reaches algorithm", "[OrientationAnalysis][ComputeFaceIPFColoringFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");

  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Run the filter once per kind, writing into uniquely-named output arrays.
  auto runWithKind = [&](ChoicesParameter::ValueType kindIndex, const std::string& firstName, const std::string& secondName) {
    ComputeFaceIPFColoringFilter filter;
    Arguments args;
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabels));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(faceNormals));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(avgEulerAnglesPath));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(featurePhasesPath));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructurePath));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_FirstFaceIPFColorsArrayName_Key, std::make_any<std::string>(firstName));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SecondFaceIPFColorsArrayName_Key, std::make_any<std::string>(secondName));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_ColorKey_Key, std::make_any<ChoicesParameter::ValueType>(kindIndex));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  };

  runWithKind(0, "FirstIPF_TSL", "SecondIPF_TSL");
  runWithKind(1, "FirstIPF_PUCM", "SecondIPF_PUCM");
  runWithKind(2, "FirstIPF_NH", "SecondIPF_NH");

  const auto& tslFirst = dataStructure.getDataRefAs<UInt8Array>(faceDataGroup.createChildPath("FirstIPF_TSL"));
  const auto& pucmFirst = dataStructure.getDataRefAs<UInt8Array>(faceDataGroup.createChildPath("FirstIPF_PUCM"));
  const auto& nhFirst = dataStructure.getDataRefAs<UInt8Array>(faceDataGroup.createChildPath("FirstIPF_NH"));

  REQUIRE(tslFirst.getSize() == pucmFirst.getSize());
  REQUIRE(tslFirst.getSize() == nhFirst.getSize());

  // Sanity: at least one tuple must differ between TSL and each other kind. If
  // the switch in executeImpl ever silently collapsed every kind onto TSL,
  // these arrays would be identical.
  REQUIRE(!std::equal(tslFirst.cbegin(), tslFirst.cend(), pucmFirst.cbegin()));
  REQUIRE(!std::equal(tslFirst.cbegin(), tslFirst.cend(), nhFirst.cbegin()));
}

TEST_CASE("OrientationAnalysis::ComputeFaceIPFColoringFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeFaceIPFColoringFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeFaceIPFColoringFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeFaceIPFColoringFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeFaceIPFColoringFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceLabelsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceNormalsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFaceIPFColoringFilter::k_FeatureEulerAnglesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFaceIPFColoringFilter::k_FeaturePhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFaceIPFColoringFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeFaceIPFColoringFilter::k_FirstFaceIPFColorsArrayName_Key) == "TestName");
    }
  }
}
