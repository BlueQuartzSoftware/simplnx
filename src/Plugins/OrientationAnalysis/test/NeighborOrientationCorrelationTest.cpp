#include "OrientationAnalysis/Filters/NeighborOrientationCorrelationFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

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
// Oracle rotations give closed-form misorientations. Decisions are at least one
// degree from the 5-degree tolerance. Derivations and the independent reference
// are in oracle/DERIVATIONS.md and oracle/reference_noc.py. The deviation
// document is src/Plugins/OrientationAnalysis/vv/deviations/NeighborOrientationCorrelationFilter.md.
// Ties select the final valid neighbor in -Z, -Y, -X, +X, +Y, +Z order.
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

/**
 * @struct OracleFixture
 * @brief Holds one analytical neighbor-correlation fixture.
 */
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

/**
 * @struct CellSnapshot
 * @brief Stores all transferable tuples before execution.
 */
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
 * @brief Verifies tuple transfers after one pass.
 * @param dataStructure Contains the post-execution arrays.
 * @param before Captures the original tuple values.
 * @param expectedSource Maps a replaced cell to its source cell.
 * @param ignoredArrayNames Identifies arrays that must retain destination values.
 *
 * Replaced cells copy the original source tuple. Other cells remain unchanged.
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

namespace SmallIn100Invariants
{
/**
 * @brief Widens a numeric cell array for snapshot comparison.
 * @tparam T Specifies the array value type.
 * @param iDataArray Provides the source array.
 * @return Values widened to float64.
 *
 * Small IN100 values are exactly representable as float64.
 */
template <typename T>
std::vector<float64> ToDoubles(const IDataArray& iDataArray)
{
  const auto& storeRef = dynamic_cast<const DataArray<T>&>(iDataArray).getDataStoreRef();
  std::vector<float64> values(storeRef.getSize());
  for(usize i = 0; i < storeRef.getSize(); i++)
  {
    values[i] = static_cast<float64>(storeRef[i]);
  }
  return values;
}

std::vector<float64> SnapshotArray(const IDataArray& iDataArray)
{
  switch(iDataArray.getDataType())
  {
  case DataType::boolean:
    return ToDoubles<bool>(iDataArray);
  case DataType::int8:
    return ToDoubles<int8>(iDataArray);
  case DataType::int16:
    return ToDoubles<int16>(iDataArray);
  case DataType::int32:
    return ToDoubles<int32>(iDataArray);
  case DataType::uint8:
    return ToDoubles<uint8>(iDataArray);
  case DataType::uint16:
    return ToDoubles<uint16>(iDataArray);
  case DataType::uint32:
    return ToDoubles<uint32>(iDataArray);
  case DataType::float32:
    return ToDoubles<float32>(iDataArray);
  case DataType::float64:
    return ToDoubles<float64>(iDataArray);
  default: {
    // int64/uint64 are not exactly representable as float64 above 2^53; none occur in
    // the Small IN100 cell data. Fail loudly if that ever changes.
    FAIL("Unsupported cell array type in Small IN100 invariant snapshot");
    return {};
  }
  }
}

/**
 * @struct CellArraySnapshot
 * @brief Stores one numeric cell-array snapshot.
 */
struct CellArraySnapshot
{
  DataPath path;
  usize numComponents = 0;
  std::vector<float64> values;
};

std::vector<CellArraySnapshot> SnapshotCellArrays(const DataStructure& dataStructure, const DataPath& cellAMPath)
{
  std::vector<CellArraySnapshot> snapshots;
  const auto& cellDataGroup = dataStructure.getDataRefAs<AttributeMatrix>(cellAMPath);
  for(const auto& child : cellDataGroup)
  {
    const DataPath arrayPath = cellAMPath.createChildPath(child.second->getName());
    const auto* arrayPtr = dataStructure.getDataAs<IDataArray>(arrayPath);
    if(arrayPtr == nullptr)
    {
      // This snapshot supports numeric IDataArrays. Oracle F13 covers
      // NeighborList and StringArray transfer.
      continue;
    }
    snapshots.push_back({arrayPath, arrayPtr->getNumberOfComponents(), SnapshotArray(*arrayPtr)});
  }
  return snapshots;
}
} // namespace SmallIn100Invariants
} // namespace

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Preflight Error - Cell array tuple count mismatch (-580093)",
          "[OrientationAnalysis][NeighborOrientationCorrelationFilter][preflight]")
{
  UnitTest::LoadPlugins();

  // Cell arrays have different tuple counts and must report -580093.
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "DataContainer");
  imageGeom->setDimensions({10, 1, 1});

  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", {10}, imageGeom->getId());
  UnitTest::CreateTestDataArray<float32>(dataStructure, "ConfidenceIndex", {10}, {1}, cellAM->getId());
  UnitTest::CreateTestDataArray<float32>(dataStructure, "Quats", {10}, {4}, cellAM->getId());

  // The separate group gives CellPhases nine tuples instead of ten.
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

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Preflight - Level validation (-580094 error, -580095 warning)",
          "[OrientationAnalysis][NeighborOrientationCorrelationFilter][preflight]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  OracleFixture fixture(5, 5, 5, {999, 1});
  DataStructure dataStructure = BuildDataStructure(fixture);

  NeighborOrientationCorrelationFilter filter;
  Arguments args;
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_MinConfidence_Key, std::make_any<float32>(k_MinConfidence));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_MisorientationTolerance_Key, std::make_any<float32>(k_ToleranceDeg));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CorrelationArrayPath_Key, std::make_any<DataPath>(k_CIPath));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_CellPhasesPath));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_CellQuatsPath));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_XtalPath));
  args.insertOrAssign(NeighborOrientationCorrelationFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));

  SECTION("negative Level is a preflight error")
  {
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_Level_Key, std::make_any<int32>(-1));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors()[0].code == -580094);
  }

  SECTION("Level >= 6 preflights valid but warns that zero passes will run")
  {
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_Level_Key, std::make_any<int32>(6));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    bool foundWarning = false;
    for(const auto& warning : preflightResult.outputActions.warnings())
    {
      if(warning.code == -580095)
      {
        foundWarning = true;
      }
    }
    REQUIRE(foundWarning);
  }

  SECTION("Level below 6 does not warn")
  {
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_Level_Key, std::make_any<int32>(2));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    for(const auto& warning : preflightResult.outputActions.warnings())
    {
      REQUIRE(warning.code != -580095);
    }
  }
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
      // Pipeline loading verifies MultiDataArraySelectionFilterParameterConverter.
    }
  }
}

// Oracle fixtures use z*ny*nx + y*nx + x linear indexing.

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F01 - uniform neighbors 3D", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // Fully tied neighbors select the final +Z neighbor at index 87.
  OracleFixture fixture(5, 5, 5, {999, 1});
  const usize center = fixture.idx(2, 2, 2);
  fixture.ci[center] = k_BadCI;
  fixture.anglesDeg[center] = 30.0;

  DataStructure dataStructure = BuildDataStructure(fixture);
  const CellSnapshot before = Capture(dataStructure);
  ExecuteFixture(dataStructure, 5);

  // Only the center receives the original tuple from index 87.
  VerifyAgainstSnapshot(dataStructure, before, {{62, 87}});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F02 - dissimilar neighbors untouched", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // All neighbor pairs exceed the tolerance, so no tuple is replaced.
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

namespace NOCOocTest
{
const std::string k_GeomName("Image Geometry");
const std::string k_CellDataName("Cell Data");

const DataPath k_GeomPath({k_GeomName});
const DataPath k_CellDataPath = k_GeomPath.createChildPath(k_CellDataName);
const DataPath k_CIPath = k_CellDataPath.createChildPath("Confidence Index");
const DataPath k_QuatsPath = k_CellDataPath.createChildPath("Quats");
const DataPath k_PhasesPath = k_CellDataPath.createChildPath("Phases");
const DataPath k_CrystalStructuresPath = k_GeomPath.createChildPath("Ensemble Data").createChildPath("CrystalStructures");

void BuildTestData(DataStructure& dataStructure, usize dimX, usize dimY, usize dimZ, usize blockSize)
{
  const ShapeType cellTupleShape = {dimZ, dimY, dimX};
  const usize sliceSize = dimX * dimY;

  auto* imageGeom = ImageGeom::Create(dataStructure, k_GeomName);
  imageGeom->setDimensions({dimX, dimY, dimZ});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(dataStructure, k_CellDataName, cellTupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  auto quatsDataStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, k_QuatsPath, cellTupleShape, {4}, IDataAction::Mode::Execute);
  auto* quatsArray = DataArray<float32>::Create(dataStructure, "Quats", quatsDataStore, cellAM->getId());
  auto& quatsStore = quatsArray->getDataStoreRef();

  auto phasesDataStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_PhasesPath, cellTupleShape, {1}, IDataAction::Mode::Execute);
  auto* phasesArray = DataArray<int32>::Create(dataStructure, "Phases", phasesDataStore, cellAM->getId());
  auto& phasesStore = phasesArray->getDataStoreRef();

  auto ciDataStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, k_CIPath, cellTupleShape, {1}, IDataAction::Mode::Execute);
  auto* ciArray = DataArray<float32>::Create(dataStructure, "Confidence Index", ciDataStore, cellAM->getId());
  auto& ciStore = ciArray->getDataStoreRef();

  const usize blocksPerDimX = dimX / blockSize;
  const usize blocksPerDimY = dimY / blockSize;

  std::vector<float32> quatsBuf(sliceSize * 4);
  std::vector<int32> phasesBuf(sliceSize);
  std::vector<float32> ciBuf(sliceSize);

  for(usize z = 0; z < dimZ; z++)
  {
    for(usize y = 0; y < dimY; y++)
    {
      for(usize x = 0; x < dimX; x++)
      {
        const usize inSlice = y * dimX + x;
        phasesBuf[inSlice] = 1;

        usize bx = x / blockSize;
        usize by = y / blockSize;
        usize bz = z / blockSize;
        float32 angle = static_cast<float32>(bz * blocksPerDimY * blocksPerDimX + by * blocksPerDimX + bx) * 0.1f;
        float32 sinHalf = std::sin(angle * 0.5f);
        float32 cosHalf = std::cos(angle * 0.5f);

        const usize qIdx = inSlice * 4;
        quatsBuf[qIdx] = cosHalf;
        quatsBuf[qIdx + 1] = sinHalf * 0.577350269f; // 1/sqrt(3)
        quatsBuf[qIdx + 2] = sinHalf * 0.577350269f;
        quatsBuf[qIdx + 3] = sinHalf * 0.577350269f;

        bool isBoundary = (x % blockSize == 0) || (y % blockSize == 0) || (z % blockSize == 0);
        bool isNoisy = ((x * 7 + y * 13 + z * 29) % 10 == 0);
        ciBuf[inSlice] = (isBoundary || isNoisy) ? 0.05f : 0.9f;
      }
    }
    const usize zOffset = z * sliceSize;
    quatsStore.copyFromBuffer(zOffset * 4, nonstd::span<const float32>(quatsBuf.data(), sliceSize * 4));
    phasesStore.copyFromBuffer(zOffset, nonstd::span<const int32>(phasesBuf.data(), sliceSize));
    ciStore.copyFromBuffer(zOffset, nonstd::span<const float32>(ciBuf.data(), sliceSize));
  }

  auto* ensembleAM = AttributeMatrix::Create(dataStructure, "Ensemble Data", {2}, imageGeom->getId());
  auto crystalStructuresDataStore = DataStoreUtilities::CreateDataStore<uint32>(dataStructure, k_CrystalStructuresPath, {2}, {1}, IDataAction::Mode::Execute);
  auto* crystalStructuresArray = DataArray<uint32>::Create(dataStructure, "CrystalStructures", crystalStructuresDataStore, ensembleAM->getId());
  std::array<uint32, 2> csData = {999, 1};
  crystalStructuresArray->getDataStoreRef().copyFromBuffer(0, nonstd::span<const uint32>(csData.data(), 2));
}
} // namespace NOCOocTest

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Generate Test Data", "[OrientationAnalysis][NeighborOrientationCorrelationFilter][.GenerateTestData]")
{
  using namespace NOCOocTest;
  const auto outputDir = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "generated_test_data" / "neighbor_orientation_correlation";
  fs::create_directories(outputDir);

  // The 200-cubed fixture uses 25-tuple blocks to exercise bounded I/O.
  {
    DataStructure buildDS;
    BuildTestData(buildDS, 200, 200, 200, 25);
    UnitTest::WriteTestDataStructure(buildDS, outputDir / "large_input.dream3d");
  }
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F03 - argmax selection (D3 regression)", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // The four-neighbor clique must choose +X, the final highest-count neighbor.
  // This prevents a lower-count +Z neighbor from winning.
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

  VerifyAgainstSnapshot(dataStructure, before, {{62, 63}});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

namespace
{
// A dissimilar 3x3x3 bad cube needs three erosion passes. This fixture detects
// an incorrect cleanup-pass count.
NOCOracle::OracleFixture BuildCascadeFixture()
{
  NOCOracle::OracleFixture fixture(7, 7, 7, {999, 1});
  for(usize z = 2; z <= 4; z++)
  {
    for(usize y = 2; y <= 4; y++)
    {
      for(usize x = 2; x <= 4; x++)
      {
        const usize i = fixture.idx(x, y, z);
        fixture.ci[i] = NOCOracle::k_BadCI;
        fixture.anglesDeg[i] = 6.0 + 6.0 * static_cast<float64>((x + 2 * y + 3 * z) % 7);
      }
    }
  }
  return fixture;
}
} // namespace

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F04 - pass schedule Level 2 (D2 regression)", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // Level two gives four passes, enough to fill the three-pass cascade.
  OracleFixture fixture = BuildCascadeFixture();
  DataStructure dataStructure = BuildDataStructure(fixture);
  ExecuteFixture(dataStructure, 2);

  const auto& ciRef = dataStructure.getDataRefAs<Float32Array>(k_CIPath).getDataStoreRef();
  const auto& quatsRef = dataStructure.getDataRefAs<Float32Array>(k_CellQuatsPath).getDataStoreRef();
  for(usize i = 0; i < fixture.count(); i++)
  {
    CAPTURE(i);
    REQUIRE(ciRef[i] >= k_MinConfidence);
    // Filled cells must copy an identity quaternion from the good region.
    REQUIRE(quatsRef[i * 4 + 2] == 0.0f);
    REQUIRE(quatsRef[i * 4 + 3] == 1.0f);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F10 - pass schedule Level 4 (D2 regression)", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // Level four gives two passes and leaves only the cube center.
  OracleFixture fixture = BuildCascadeFixture();
  DataStructure dataStructure = BuildDataStructure(fixture);
  ExecuteFixture(dataStructure, 4);

  const usize cubeCenter = fixture.idx(3, 3, 3);
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

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F05 - mixed-phase pair never similar (D1 regression)", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // Only same-phase neighbors can contribute. The mixed-phase +X neighbor must
  // not replace the center.
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

  VerifyAgainstSnapshot(dataStructure, before, {{12, 11}});
  const auto& phasesRef = dataStructure.getDataRefAs<Int32Array>(k_CellPhasesPath).getDataStoreRef();
  REQUIRE(phasesRef[center] == 1);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F06 - phase-0 neighbors never counted", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // Phase-zero neighbors must not contribute or become source tuples.
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

  VerifyAgainstSnapshot(dataStructure, before, {{62, 57}});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F07 - 2D image", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // A z-degenerate image selects the final in-plane +Y neighbor.
  OracleFixture fixture(5, 5, 1, {999, 1});
  const usize center = fixture.idx(2, 2, 0);
  fixture.ci[center] = k_BadCI;
  fixture.anglesDeg[center] = 30.0;

  DataStructure dataStructure = BuildDataStructure(fixture);
  const CellSnapshot before = Capture(dataStructure);
  ExecuteFixture(dataStructure, 5);

  VerifyAgainstSnapshot(dataStructure, before, {{12, 17}});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F08 - Laue-class folding (hex vs cubic)", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // The 37/87 pair differs by 58 degrees. Hexagonal folds it to 2 degrees;
  // cubic folds it to 32. Hex replaces the center with 87; cubic keeps it.
  const std::array<usize, 6> neighbors = {37, 57, 61, 63, 67, 87};
  const std::array<float64, 6> angles = {0.0, 20.0, 27.0, 34.0, 41.0, 58.0};

  SECTION("Hexagonal-High: 58 deg folds to 2 deg -> center replaced by +Z neighbor 87")
  {
    OracleFixture fixture(5, 5, 5, {999, 0});
    const usize center = fixture.idx(2, 2, 2);
    fixture.ci[center] = k_BadCI;
    for(usize j = 0; j < 6; j++)
    {
      fixture.anglesDeg[neighbors[j]] = angles[j];
    }
    DataStructure dataStructure = BuildDataStructure(fixture);
    const CellSnapshot before = Capture(dataStructure);
    ExecuteFixture(dataStructure, 5);
    VerifyAgainstSnapshot(dataStructure, before, {{62, 87}});
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("Cubic-High: 58 deg folds to 32 deg -> no similar pair, center untouched")
  {
    OracleFixture fixture(5, 5, 5, {999, 1});
    const usize center = fixture.idx(2, 2, 2);
    fixture.ci[center] = k_BadCI;
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
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F09 - ignored arrays untouched (I2)", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // Ignored Payload2 must retain its original tuple while other arrays transfer.
  OracleFixture fixture(5, 5, 5, {999, 1});
  const usize center = fixture.idx(2, 2, 2);
  fixture.ci[center] = k_BadCI;
  fixture.anglesDeg[center] = 30.0;

  DataStructure dataStructure = BuildDataStructure(fixture);
  const CellSnapshot before = Capture(dataStructure);
  ExecuteFixture(dataStructure, 5, {k_Payload2Path});

  VerifyAgainstSnapshot(dataStructure, before, {{62, 87}}, {"Payload2"});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F13 - NeighborList and String cell arrays transferred", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // F13 adds NeighborList and StringArray to F01. Their transfer matches legacy
  // DREAM3D 6.5.171 behavior: a replaced cell copies the complete source tuple.
  OracleFixture fixture(5, 5, 5, {999, 1});
  const usize center = fixture.idx(2, 2, 2);
  const usize source = 87;
  fixture.ci[center] = k_BadCI;
  fixture.anglesDeg[center] = 30.0;

  DataStructure dataStructure = BuildDataStructure(fixture);
  auto* cellAMPtr = dataStructure.getDataAs<AttributeMatrix>(k_CellAMPath);
  REQUIRE(cellAMPtr != nullptr);
  const std::vector<usize> tupleShape = {fixture.nz, fixture.ny, fixture.nx};

  auto* neighborListPtr = NeighborList<int32>::Create(dataStructure, "NeighborIds", tupleShape, cellAMPtr->getId());
  std::vector<std::string> strings(fixture.count());
  for(usize i = 0; i < fixture.count(); i++)
  {
    neighborListPtr->setList(static_cast<int32>(i), std::vector<int32>{static_cast<int32>(i)});
    strings[i] = fmt::format("cell_{}", i);
  }
  const DataPath neighborListPath = k_CellAMPath.createChildPath("NeighborIds");
  auto* stringArrayPtr = StringArray::CreateWithValues(dataStructure, "Labels", tupleShape, strings, cellAMPtr->getId());
  REQUIRE(stringArrayPtr != nullptr);
  const DataPath stringArrayPath = k_CellAMPath.createChildPath("Labels");

  const CellSnapshot before = Capture(dataStructure);
  ExecuteFixture(dataStructure, 5);

  VerifyAgainstSnapshot(dataStructure, before, {{center, source}});

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<int32>>(neighborListPath));
  const auto& resultNeighborList = dataStructure.getDataRefAs<NeighborList<int32>>(neighborListPath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(stringArrayPath));
  const auto& resultStrings = dataStructure.getDataRefAs<StringArray>(stringArrayPath);
  for(usize i = 0; i < fixture.count(); i++)
  {
    const usize expected = (i == center) ? source : i;
    CAPTURE(i, expected);
    REQUIRE(resultNeighborList.getList(static_cast<int32>(i)) == std::vector<int32>{static_cast<int32>(expected)});
    REQUIRE(resultStrings[i] == fmt::format("cell_{}", expected));
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F11 - volume corner", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // A corner has three valid neighbors and selects the final +Z neighbor.
  OracleFixture fixture(4, 4, 4, {999, 1});
  fixture.ci[0] = k_BadCI;
  fixture.anglesDeg[0] = 20.0;
  fixture.anglesDeg[fixture.idx(1, 0, 0)] = 0.0;
  fixture.anglesDeg[fixture.idx(0, 1, 0)] = 1.0;
  fixture.anglesDeg[fixture.idx(0, 0, 1)] = 2.0;

  DataStructure dataStructure = BuildDataStructure(fixture);
  const CellSnapshot before = Capture(dataStructure);
  ExecuteFixture(dataStructure, 5);

  VerifyAgainstSnapshot(dataStructure, before, {{0, 16}});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F12 - anisotropic dims", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // Asymmetric dimensions detect x, y, or z stride swaps. The highest-count
  // tie must select -X.
  OracleFixture fixture(4, 5, 3, {999, 1});
  const usize center = fixture.idx(1, 2, 1);
  REQUIRE(center == 29);
  fixture.ci[center] = k_BadCI;
  fixture.anglesDeg[center] = 10.0;
  const std::array<usize, 6> neighbors = {9, 25, 28, 30, 33, 49};
  const std::array<float64, 6> angles = {0.0, 1.0, 2.0, 20.0, 21.0, 40.0};
  for(usize j = 0; j < 6; j++)
  {
    fixture.anglesDeg[neighbors[j]] = angles[j];
  }

  DataStructure dataStructure = BuildDataStructure(fixture);
  const CellSnapshot before = Capture(dataStructure);
  ExecuteFixture(dataStructure, 5);

  VerifyAgainstSnapshot(dataStructure, before, {{29, 28}});
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
