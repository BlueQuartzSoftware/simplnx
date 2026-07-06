#include "OrientationAnalysis/Filters/NeighborOrientationCorrelationFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "simplnx/Common/Numbers.hpp"

#include <catch2/catch.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
/**
 * V&V oracle fixtures (Class 1 analytical + Class 2 reference implementation).
 *
 * Every orientation is a rotation about +Z by an angle <= 45 deg (cubic) / <= 30 deg (hex),
 * where the misorientation under any Laue fold convention is exactly |theta1 - theta2| —
 * the expected outputs below are hand-derivable and convention-free. Full derivations and
 * the NumPy reference implementation live in the V&V archive (oracle/DERIVATIONS.md,
 * oracle/reference_noc.py); the deviation IDs cited below are documented in
 * src/Plugins/OrientationAnalysis/vv/deviations/NeighborOrientationCorrelationFilter.md.
 *
 * All fixtures: MinConfidence = 0.1, MisorientationTolerance = 5 deg, good CI = 0.9,
 * bad CI = 0.01; every similar/dissimilar decision sits >= 1 deg away from the tolerance.
 * Neighbor scan order is -Z, -Y, -X, +X, +Y, +Z; ties resolve to the first neighbor.
 */
namespace NOCOracle
{
constexpr float32 k_GoodCI = 0.9f;
constexpr float32 k_BadCI = 0.01f;
constexpr float32 k_MinConfidence = 0.1f;
constexpr float32 k_ToleranceDeg = 5.0f;

const std::string k_DCName = "DataContainer";
const DataPath k_GeomPath({k_DCName});
const DataPath k_CellAMPath = k_GeomPath.createChildPath("CellData");
const DataPath k_CIPath = k_CellAMPath.createChildPath("Confidence Index");
const DataPath k_CellPhasesPath = k_CellAMPath.createChildPath("Phases");
const DataPath k_CellQuatsPath = k_CellAMPath.createChildPath("Quats");
const DataPath k_SourceIndexPath = k_CellAMPath.createChildPath("SourceIndex");
const DataPath k_Payload2Path = k_CellAMPath.createChildPath("Payload2");
const DataPath k_MaskPath = k_CellAMPath.createChildPath("Mask");
const DataPath k_EnsembleAMPath = k_GeomPath.createChildPath("Ensemble Data");
const DataPath k_XtalPath = k_EnsembleAMPath.createChildPath("CrystalStructures");

struct OracleFixture
{
  usize nx = 0;
  usize ny = 0;
  usize nz = 0;
  std::vector<float64> anglesDeg;
  std::vector<int32> phases;
  std::vector<float32> ci;
  std::vector<uint32> crystalStructures;

  OracleFixture(usize dimX, usize dimY, usize dimZ, std::vector<uint32> xtal)
  : nx(dimX)
  , ny(dimY)
  , nz(dimZ)
  , anglesDeg(dimX * dimY * dimZ, 0.0)
  , phases(dimX * dimY * dimZ, 1)
  , ci(dimX * dimY * dimZ, k_GoodCI)
  , crystalStructures(std::move(xtal))
  {
  }

  usize idx(usize x, usize y, usize z) const
  {
    return z * ny * nx + y * nx + x;
  }

  usize count() const
  {
    return nx * ny * nz;
  }
};

DataStructure BuildDataStructure(const OracleFixture& fixture)
{
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, k_DCName);
  imageGeomPtr->setDimensions({fixture.nx, fixture.ny, fixture.nz});

  const std::vector<usize> tupleShape = {fixture.nz, fixture.ny, fixture.nx};
  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, "CellData", tupleShape, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAMPtr);

  auto* ciArrayPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, "Confidence Index", tupleShape, {1}, cellAMPtr->getId());
  auto* phasesArrayPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", tupleShape, {1}, cellAMPtr->getId());
  auto* quatsArrayPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, "Quats", tupleShape, {4}, cellAMPtr->getId());
  auto* sourceIndexArrayPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "SourceIndex", tupleShape, {1}, cellAMPtr->getId());
  auto* payload2ArrayPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Payload2", tupleShape, {1}, cellAMPtr->getId());
  auto* maskArrayPtr = UnitTest::CreateTestDataArray<uint8>(dataStructure, "Mask", tupleShape, {1}, cellAMPtr->getId());

  for(usize i = 0; i < fixture.count(); i++)
  {
    (*ciArrayPtr)[i] = fixture.ci[i];
    (*phasesArrayPtr)[i] = fixture.phases[i];
    const float64 halfAngle = fixture.anglesDeg[i] * numbers::pi_v<float64> / 360.0;
    (*quatsArrayPtr)[i * 4 + 0] = 0.0f;
    (*quatsArrayPtr)[i * 4 + 1] = 0.0f;
    (*quatsArrayPtr)[i * 4 + 2] = static_cast<float32>(std::sin(halfAngle));
    (*quatsArrayPtr)[i * 4 + 3] = static_cast<float32>(std::cos(halfAngle));
    (*sourceIndexArrayPtr)[i] = static_cast<int32>(i);
    (*payload2ArrayPtr)[i] = static_cast<int32>(1000 + i);
    (*maskArrayPtr)[i] = static_cast<uint8>(i % 251);
  }

  auto* ensembleAMPtr = AttributeMatrix::Create(dataStructure, "Ensemble Data", {fixture.crystalStructures.size()}, imageGeomPtr->getId());
  auto* xtalArrayPtr = UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {fixture.crystalStructures.size()}, {1}, ensembleAMPtr->getId());
  for(usize i = 0; i < fixture.crystalStructures.size(); i++)
  {
    (*xtalArrayPtr)[i] = fixture.crystalStructures[i];
  }
  return dataStructure;
}

void ExecuteFixture(DataStructure& dataStructure, int32 level, const std::vector<DataPath>& ignoredPaths = {})
{
  NeighborOrientationCorrelationFilter filter;
  Arguments args;
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_MinConfidence_Key, std::make_any<float32>(k_MinConfidence));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_MisorientationTolerance_Key, std::make_any<float32>(k_ToleranceDeg));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_Level_Key, std::make_any<int32>(level));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CorrelationArrayPath_Key, std::make_any<DataPath>(k_CIPath));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_CellPhasesPath));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_CellQuatsPath));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_XtalPath));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(ignoredPaths));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
}

struct CellSnapshot
{
  std::vector<float32> ci;
  std::vector<int32> phases;
  std::vector<float32> quats;
  std::vector<int32> sourceIndex;
  std::vector<int32> payload2;
  std::vector<uint8> mask;
};

CellSnapshot Capture(const DataStructure& dataStructure)
{
  CellSnapshot snapshot;
  const auto& ciRef = dataStructure.getDataRefAs<Float32Array>(k_CIPath).getDataStoreRef();
  const auto& phasesRef = dataStructure.getDataRefAs<Int32Array>(k_CellPhasesPath).getDataStoreRef();
  const auto& quatsRef = dataStructure.getDataRefAs<Float32Array>(k_CellQuatsPath).getDataStoreRef();
  const auto& sourceRef = dataStructure.getDataRefAs<Int32Array>(k_SourceIndexPath).getDataStoreRef();
  const auto& payloadRef = dataStructure.getDataRefAs<Int32Array>(k_Payload2Path).getDataStoreRef();
  const auto& maskRef = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath).getDataStoreRef();
  const usize numCells = ciRef.getNumberOfTuples();
  for(usize i = 0; i < numCells; i++)
  {
    snapshot.ci.push_back(ciRef[i]);
    snapshot.phases.push_back(phasesRef[i]);
    snapshot.sourceIndex.push_back(sourceRef[i]);
    snapshot.payload2.push_back(payloadRef[i]);
    snapshot.mask.push_back(maskRef[i]);
    for(usize c = 0; c < 4; c++)
    {
      snapshot.quats.push_back(quatsRef[i * 4 + c]);
    }
  }
  return snapshot;
}

/**
 * Verifies a single-pass fixture result against the pre-execution snapshot.
 * expectedSource maps each replaced cell to the cell whose ORIGINAL tuple it must now hold;
 * every other cell must be untouched in every array (Class 4 invariant I1).
 * ignoredArrayNames lists arrays excluded from the transfer (invariant I2: never modified).
 */
void VerifyAgainstSnapshot(const DataStructure& dataStructure, const CellSnapshot& before, const std::map<usize, usize>& expectedSource, const std::set<std::string>& ignoredArrayNames = {})
{
  const CellSnapshot after = Capture(dataStructure);
  const usize numCells = before.ci.size();
  for(usize i = 0; i < numCells; i++)
  {
    auto it = expectedSource.find(i);
    const bool ignoreSourceLookup = it == expectedSource.end();
    const usize src = ignoreSourceLookup ? i : it->second;
    const usize ciSrc = ignoredArrayNames.count("Confidence Index") > 0 ? i : src;
    const usize phasesSrc = ignoredArrayNames.count("Phases") > 0 ? i : src;
    const usize quatsSrc = ignoredArrayNames.count("Quats") > 0 ? i : src;
    const usize sourceIndexSrc = ignoredArrayNames.count("SourceIndex") > 0 ? i : src;
    const usize payloadSrc = ignoredArrayNames.count("Payload2") > 0 ? i : src;
    const usize maskSrc = ignoredArrayNames.count("Mask") > 0 ? i : src;
    CAPTURE(i, src);
    REQUIRE(after.ci[i] == before.ci[ciSrc]);
    REQUIRE(after.phases[i] == before.phases[phasesSrc]);
    REQUIRE(after.sourceIndex[i] == before.sourceIndex[sourceIndexSrc]);
    REQUIRE(after.payload2[i] == before.payload2[payloadSrc]);
    REQUIRE(after.mask[i] == before.mask[maskSrc]);
    for(usize c = 0; c < 4; c++)
    {
      REQUIRE(after.quats[i * 4 + c] == before.quats[quatsSrc * 4 + c]);
    }
  }
}
} // namespace NOCOracle
} // namespace

/**
 * Read H5Ebsd File
 * MultiThreshold Objects
 * Convert Orientation Representation (Euler->Quats)
 * Align Sections Misorientation
 * Identify Sample
 * Align Sections Feature Centroid
 *
 * Read DREAM3D File (read the exemplar 'align_sections_feature_centroid.dream3d' file from
 * [Optional] Write out dream3d file
 *
 *
 * Compare the shifts file 'align_sections_feature_centroid.txt' to what was written
 *
 * Compare all the data arrays from the "Exemplar Data / CellData"
 */

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Small IN100 Pipeline", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "neighbor_orientation_correlation_v2.tar.gz", "neighbor_orientation_correlation_v2.dream3d");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

  auto* filterList = Application::Instance()->getFilterList();

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/neighbor_orientation_correlation_v2.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // MultiThreshold Objects Filter (From SimplnxCore Plugins)
  SmallIn100::ExecuteMultiThresholdObjects(dataStructure, *filterList);

  // Convert Orientations Filter (From OrientationAnalysis Plugin)
  SmallIn100::ExecuteConvertOrientations(dataStructure, *filterList);

  // Align Sections Misorientation Filter (From OrientationAnalysis Plugin)
  SmallIn100::ExecuteAlignSectionsMisorientation(dataStructure, *filterList, fs::path(fmt::format("{}/AlignSectionsMisorientation_1.txt", unit_test::k_BinaryDir)));

  // Identify Sample Filter
  SmallIn100::ExecuteIdentifySample(dataStructure, *filterList);

  // Align Sections Feature Centroid Filter
  SmallIn100::ExecuteAlignSectionsFeatureCentroid(dataStructure, *filterList, fs::path(fmt::format("{}/AlignSectionsFeatureCentroid_1.txt", unit_test::k_BinaryDir)));

  // Bad Data Neighbor Orientation Check Filter
  SmallIn100::ExecuteBadDataNeighborOrientationCheck(dataStructure, *filterList);

  // Neighbor Orientation Correlation Filter
  {
    auto filter = filterList->createFilter(k_NeighborOrientationCorrelationFilterHandle);
    REQUIRE(nullptr != filter);

    Arguments args;
    // Create default Parameters for the filter.
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_MinConfidence_Key, std::make_any<float32>(0.2f));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_Level_Key, std::make_any<int32>(2));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CorrelationArrayPath_Key, std::make_any<DataPath>(k_ConfidenceIndexArrayPath));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_IgnoredDataArrayPaths_Key, std::make_any<std::vector<DataPath>>());

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Loop and compare each array from the 'Exemplar Data / CellData' to the 'Data Container / CellData' group.
  // A missing or type-mismatched exemplar array is a hard failure: the v1 archive's container was named
  // 'DataContainer', so every path lookup against 'Exemplar Data' returned null and the original
  // silent 'continue' skipped ALL comparisons — the test passed without comparing a single array.
  {
    auto& cellDataGroup = dataStructure.getDataRefAs<AttributeMatrix>(k_CellAttributeMatrix);
    std::vector<DataPath> selectedCellArrays;

    // Create the vector of selected cell DataPaths
    for(auto& child : cellDataGroup)
    {
      selectedCellArrays.push_back(k_CellAttributeMatrix.createChildPath(child.second->getName()));
    }
    REQUIRE(!selectedCellArrays.empty());

    for(const auto& cellArrayPath : selectedCellArrays)
    {
      const auto& generatedDataArray = dataStructure.getDataRefAs<IDataArray>(cellArrayPath);
      DataType type = generatedDataArray.getDataType();

      // Now generate the path to the exemplar data set in the exemplar data structure.
      std::vector<std::string> generatedPathVector = cellArrayPath.getPathVector();
      generatedPathVector[0] = k_ExemplarDataContainer;
      DataPath exemplarDataArrayPath(generatedPathVector);

      CAPTURE(exemplarDataArrayPath.toString());
      REQUIRE(nullptr != exemplarDataStructure.getDataAs<IDataArray>(exemplarDataArrayPath));

      auto& exemplarDataArray = exemplarDataStructure.getDataRefAs<IDataArray>(exemplarDataArrayPath);
      REQUIRE(type == exemplarDataArray.getDataType());

      switch(type)
      {
      case DataType::boolean: {
        UnitTest::CompareDataArrays<bool>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::int8: {
        UnitTest::CompareDataArrays<int8>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::int16: {
        UnitTest::CompareDataArrays<int16>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::int32: {
        UnitTest::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::int64: {
        UnitTest::CompareDataArrays<int64>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::uint8: {
        UnitTest::CompareDataArrays<uint8>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::uint16: {
        UnitTest::CompareDataArrays<uint16>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::uint32: {
        UnitTest::CompareDataArrays<uint32>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::uint64: {
        UnitTest::CompareDataArrays<uint64>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::float32: {
        UnitTest::CompareDataArrays<float32>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::float64: {
        UnitTest::CompareDataArrays<float64>(generatedDataArray, exemplarDataArray);
        break;
      }
      default: {
        throw std::runtime_error("Invalid DataType");
      }
      }
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/neighbor_orientation_correlation.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Preflight Error - Cell array tuple count mismatch (-580093)",
          "[OrientationAnalysis][NeighborOrientationCorrelationFilter][preflight]")
{
  UnitTest::LoadPlugins();

  // Build a minimal synthetic DataStructure where the cell-level arrays validated
  // together (ConfidenceIndex, CellPhases, Quats, plus the sibling arrays of the
  // ConfidenceIndex parent group) do NOT all share the same tuple count. This drives
  // the validateNumberOfTuples() guard in preflightImpl that emits k_InvalidNumTuples (-580093).
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "DataContainer");
  imageGeom->setDimensions({10, 1, 1});

  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", {10}, imageGeom->getId());
  UnitTest::CreateTestDataArray<float32>(dataStructure, "ConfidenceIndex", {10}, {1}, cellAM->getId());
  UnitTest::CreateTestDataArray<float32>(dataStructure, "Quats", {10}, {4}, cellAM->getId());

  // CellPhases lives in a separate AttributeMatrix with a deliberately different tuple
  // count (9 != 10) so the cross-array tuple-count check fails.
  auto* mismatchAM = AttributeMatrix::Create(dataStructure, "MismatchData", {9}, imageGeom->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {9}, {1}, mismatchAM->getId());

  auto* ensembleAM = AttributeMatrix::Create(dataStructure, "Ensemble Data", {2}, imageGeom->getId());
  UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1}, ensembleAM->getId());

  NeighborOrientationCorrelationFilter filter;
  Arguments args;
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_MinConfidence_Key, std::make_any<float32>(0.1f));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_Level_Key, std::make_any<int32>(6));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CorrelationArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "ConfidenceIndex"})));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "MismatchData", "Phases"})));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Quats"})));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "Ensemble Data", "CrystalStructures"})));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -580093);
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][NeighborOrientationCorrelationFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "NeighborOrientationCorrelationFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "NeighborOrientationCorrelationFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<NeighborOrientationCorrelationFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<float32>(NeighborOrientationCorrelationFilter::k_MinConfidence_Key) == 2.5f);
      CHECK(args.value<float32>(NeighborOrientationCorrelationFilter::k_MisorientationTolerance_Key) == 2.5f);
      CHECK(args.value<int32>(NeighborOrientationCorrelationFilter::k_Level_Key) == 5);
      CHECK(args.value<DataPath>(NeighborOrientationCorrelationFilter::k_ImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(NeighborOrientationCorrelationFilter::k_CorrelationArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(NeighborOrientationCorrelationFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(NeighborOrientationCorrelationFilter::k_QuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(NeighborOrientationCorrelationFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      // Complex type (MultiDataArraySelectionFilterParameterConverter) - verified by successful pipeline loading
    }
  }
}

// -----------------------------------------------------------------------------
// V&V oracle fixtures. Derivations: V&V archive oracle/DERIVATIONS.md.
// Linear index = z*ny*nx + y*nx + x. 5x5x5 center = 62; its face neighbors in scan
// order (-Z,-Y,-X,+X,+Y,+Z) are 37, 57, 61, 63, 67, 87.
// -----------------------------------------------------------------------------

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F01 - uniform neighbors 3D", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // Center (2,2,2)=62 is bad (CI 0.01, 30 deg); all 6 neighbors are 0 deg -> all 15
  // neighbor pairs similar -> every simCount = 5 (tie). First-of-ties = -Z neighbor 37.
  OracleFixture fixture(5, 5, 5, {999, 1});
  const usize center = fixture.idx(2, 2, 2);
  fixture.ci[center] = k_BadCI;
  fixture.anglesDeg[center] = 30.0;

  DataStructure dataStructure = BuildDataStructure(fixture);
  const CellSnapshot before = Capture(dataStructure);
  ExecuteFixture(dataStructure, 5);

  // Class 1 expected: cell 62 holds the full ORIGINAL tuple of cell 37; nothing else moves (I1).
  VerifyAgainstSnapshot(dataStructure, before, {{62, 37}});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F02 - dissimilar neighbors untouched", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // Neighbor angles 10,17,24,31,38,45 deg: pairwise >= 7 deg > 5 deg tolerance -> all
  // simCounts 0 -> no replacement anywhere.
  OracleFixture fixture(5, 5, 5, {999, 1});
  const usize center = fixture.idx(2, 2, 2);
  fixture.ci[center] = k_BadCI;
  fixture.anglesDeg[center] = 44.0;
  const std::array<usize, 6> neighbors = {37, 57, 61, 63, 67, 87};
  const std::array<float64, 6> angles = {10.0, 17.0, 24.0, 31.0, 38.0, 45.0};
  for(usize j = 0; j < 6; j++)
  {
    fixture.anglesDeg[neighbors[j]] = angles[j];
  }

  DataStructure dataStructure = BuildDataStructure(fixture);
  const CellSnapshot before = Capture(dataStructure);
  ExecuteFixture(dataStructure, 5);

  VerifyAgainstSnapshot(dataStructure, before, {});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F03 - argmax selection (D3 regression)", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // -Z,-Y,-X,+X = 0,1,2,1.5 deg (4-clique, simCount 3 each); +Y,+Z = 20,21 deg (pair,
  // simCount 1 each). The argmax must pick -Z (37). The legacy last-wins defect
  // (deviation D3) picked +Z (87) - a count-1 neighbor beating a count-3 neighbor.
  OracleFixture fixture(5, 5, 5, {999, 1});
  const usize center = fixture.idx(2, 2, 2);
  fixture.ci[center] = k_BadCI;
  fixture.anglesDeg[center] = 10.0;
  const std::array<usize, 6> neighbors = {37, 57, 61, 63, 67, 87};
  const std::array<float64, 6> angles = {0.0, 1.0, 2.0, 1.5, 20.0, 21.0};
  for(usize j = 0; j < 6; j++)
  {
    fixture.anglesDeg[neighbors[j]] = angles[j];
  }

  DataStructure dataStructure = BuildDataStructure(fixture);
  const CellSnapshot before = Capture(dataStructure);
  ExecuteFixture(dataStructure, 5);

  VerifyAgainstSnapshot(dataStructure, before, {{62, 37}});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F04/F10 - pass schedule (D2 regression)", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // 3x3x3 bad cube (x,y,z in 2..4) of pairwise-dissimilar garbage (distance-2 mod-7
  // coloring, angles 6..42 deg) inside a uniform 0-deg 7x7x7 good volume. Erosion fills
  // corners+edges (pass 1), face centers (pass 2), cube center (pass 3). The legacy
  // double-decrement defect (deviation D2) halved the documented 6-Level pass count.
  auto buildCascadeFixture = []() {
    OracleFixture fixture(7, 7, 7, {999, 1});
    for(usize z = 2; z <= 4; z++)
    {
      for(usize y = 2; y <= 4; y++)
      {
        for(usize x = 2; x <= 4; x++)
        {
          const usize i = fixture.idx(x, y, z);
          fixture.ci[i] = k_BadCI;
          fixture.anglesDeg[i] = 6.0 + 6.0 * static_cast<float64>((x + 2 * y + 3 * z) % 7);
        }
      }
    }
    return fixture;
  };

  SECTION("Level 2 -> 4 passes: cube fully filled")
  {
    OracleFixture fixture = buildCascadeFixture();
    DataStructure dataStructure = BuildDataStructure(fixture);
    ExecuteFixture(dataStructure, 2);

    const auto& ciRef = dataStructure.getDataRefAs<Float32Array>(k_CIPath).getDataStoreRef();
    const auto& quatsRef = dataStructure.getDataRefAs<Float32Array>(k_CellQuatsPath).getDataStoreRef();
    for(usize i = 0; i < fixture.count(); i++)
    {
      CAPTURE(i);
      REQUIRE(ciRef[i] >= k_MinConfidence);
      // Every filled cell must carry the good region's identity quaternion: a garbage
      // neighbor is pairwise-dissimilar to everything so it can never win the argmax.
      REQUIRE(quatsRef[i * 4 + 2] == 0.0f);
      REQUIRE(quatsRef[i * 4 + 3] == 1.0f);
    }
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("Level 4 -> 2 passes: only the cube center remains unfilled")
  {
    OracleFixture fixture = buildCascadeFixture();
    DataStructure dataStructure = BuildDataStructure(fixture);
    ExecuteFixture(dataStructure, 4);

    const usize cubeCenter = fixture.idx(3, 3, 3); // = 171
    const auto& ciRef = dataStructure.getDataRefAs<Float32Array>(k_CIPath).getDataStoreRef();
    for(usize i = 0; i < fixture.count(); i++)
    {
      CAPTURE(i);
      if(i == cubeCenter)
      {
        REQUIRE(ciRef[i] < k_MinConfidence);
      }
      else
      {
        REQUIRE(ciRef[i] >= k_MinConfidence);
      }
    }
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F05 - mixed-phase pair never similar (D1 regression)", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // 2D 5x5x1, center (2,2)=12 bad, phase 1, 10 deg. Neighbors: -Y(7)=0 deg ph1,
  // -X(11)=1 deg ph1, +X(13)=2 deg PHASE 2, +Y(17)=40 deg ph1. Only legitimate similar
  // pair is (-Y,-X) -> counts (1,1,0,0) -> argmax-first picks -Y (7) and the phase stays 1.
  // The legacy stale-w defect (deviation D1) counted the mixed-phase (-Y,+X) pair via the
  // inherited w and copied PHASE-2 data from +X (13).
  OracleFixture fixture(5, 5, 1, {999, 1, 1});
  const usize center = fixture.idx(2, 2, 0);
  fixture.ci[center] = k_BadCI;
  fixture.anglesDeg[center] = 10.0;
  fixture.anglesDeg[fixture.idx(2, 1, 0)] = 0.0;
  fixture.anglesDeg[fixture.idx(1, 2, 0)] = 1.0;
  fixture.anglesDeg[fixture.idx(3, 2, 0)] = 2.0;
  fixture.phases[fixture.idx(3, 2, 0)] = 2;
  fixture.anglesDeg[fixture.idx(2, 3, 0)] = 40.0;

  DataStructure dataStructure = BuildDataStructure(fixture);
  const CellSnapshot before = Capture(dataStructure);
  ExecuteFixture(dataStructure, 5);

  VerifyAgainstSnapshot(dataStructure, before, {{12, 7}});
  const auto& phasesRef = dataStructure.getDataRefAs<Int32Array>(k_CellPhasesPath).getDataStoreRef();
  REQUIRE(phasesRef[center] == 1);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F06 - phase-0 neighbors never counted", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // -Z(37)=0 deg / -Y(57)=1 deg are the only similar pair; -X(61)=30, +X(63)=38 deg;
  // +Y(67), +Z(87) are phase 0 (unindexed) and must never be counted or chosen.
  OracleFixture fixture(5, 5, 5, {999, 1, 1});
  const usize center = fixture.idx(2, 2, 2);
  fixture.ci[center] = k_BadCI;
  fixture.anglesDeg[center] = 10.0;
  fixture.anglesDeg[37] = 0.0;
  fixture.anglesDeg[57] = 1.0;
  fixture.anglesDeg[61] = 30.0;
  fixture.anglesDeg[63] = 38.0;
  fixture.phases[67] = 0;
  fixture.phases[87] = 0;

  DataStructure dataStructure = BuildDataStructure(fixture);
  const CellSnapshot before = Capture(dataStructure);
  ExecuteFixture(dataStructure, 5);

  VerifyAgainstSnapshot(dataStructure, before, {{62, 37}});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F07 - 2D image", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // 5x5x1: center (2,2)=12 bad; the 4 in-plane neighbors (7,11,13,17) all 0 deg ->
  // counts all 3 (tie) -> first-of-ties = -Y neighbor 7. Exercises the z-degenerate
  // boundary masks.
  OracleFixture fixture(5, 5, 1, {999, 1});
  const usize center = fixture.idx(2, 2, 0);
  fixture.ci[center] = k_BadCI;
  fixture.anglesDeg[center] = 30.0;

  DataStructure dataStructure = BuildDataStructure(fixture);
  const CellSnapshot before = Capture(dataStructure);
  ExecuteFixture(dataStructure, 5);

  VerifyAgainstSnapshot(dataStructure, before, {{12, 7}});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F08 - hexagonal Laue class", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // F01 with CrystalStructures[1] = 0 (Hexagonal-High). 30 deg about c folds to
  // min(30, 60-30) = 30 deg > tolerance. Exercises the non-cubic LaueOps dispatch.
  OracleFixture fixture(5, 5, 5, {999, 0});
  const usize center = fixture.idx(2, 2, 2);
  fixture.ci[center] = k_BadCI;
  fixture.anglesDeg[center] = 30.0;

  DataStructure dataStructure = BuildDataStructure(fixture);
  const CellSnapshot before = Capture(dataStructure);
  ExecuteFixture(dataStructure, 5);

  VerifyAgainstSnapshot(dataStructure, before, {{62, 37}});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F09 - ignored arrays untouched (I2)", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // F01 with Payload2 in IgnoredDataArrayPaths: Payload2 must be bit-identical to input
  // while every other array is copied from cell 37.
  OracleFixture fixture(5, 5, 5, {999, 1});
  const usize center = fixture.idx(2, 2, 2);
  fixture.ci[center] = k_BadCI;
  fixture.anglesDeg[center] = 30.0;

  DataStructure dataStructure = BuildDataStructure(fixture);
  const CellSnapshot before = Capture(dataStructure);
  ExecuteFixture(dataStructure, 5, {k_Payload2Path});

  VerifyAgainstSnapshot(dataStructure, before, {{62, 37}}, {"Payload2"});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F11 - volume corner", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // 4x4x4, bad cell at corner (0,0,0): only +X(1)=0 deg, +Y(4)=1 deg, +Z(16)=2 deg are
  // valid; all 3 pairs similar -> counts 2 each (tie) -> first-of-ties = +X neighbor 1.
  // Exercises the volume-boundary validity masks.
  OracleFixture fixture(4, 4, 4, {999, 1});
  fixture.ci[0] = k_BadCI;
  fixture.anglesDeg[0] = 20.0;
  fixture.anglesDeg[fixture.idx(1, 0, 0)] = 0.0;
  fixture.anglesDeg[fixture.idx(0, 1, 0)] = 1.0;
  fixture.anglesDeg[fixture.idx(0, 0, 1)] = 2.0;

  DataStructure dataStructure = BuildDataStructure(fixture);
  const CellSnapshot before = Capture(dataStructure);
  ExecuteFixture(dataStructure, 5);

  VerifyAgainstSnapshot(dataStructure, before, {{0, 1}});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Class 4 - Level >= 6 is a no-op (I4)", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // The level loop runs 6 - Level passes; Level = 6 (the parameter default) runs zero
  // passes, so the output must be bit-identical to the input.
  OracleFixture fixture(5, 5, 5, {999, 1});
  const usize center = fixture.idx(2, 2, 2);
  fixture.ci[center] = k_BadCI;
  fixture.anglesDeg[center] = 30.0;

  DataStructure dataStructure = BuildDataStructure(fixture);
  const CellSnapshot before = Capture(dataStructure);
  ExecuteFixture(dataStructure, 6);

  VerifyAgainstSnapshot(dataStructure, before, {});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
