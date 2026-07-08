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
 * Neighbor scan order is -Z, -Y, -X, +X, +Y, +Z; the argmax resolves ties to the LAST
 * neighbor in scan order ('>=' with count > 0), so fully-tied neighborhoods pick the
 * same neighbor as DREAM3D 6.5.171 and migration diffs concentrate where the D3
 * ranking defect actually mattered.
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

namespace SmallIn100Invariants
{
/**
 * Losslessly widen a numeric cell array to float64 for before/after comparison.
 * Every type in the Small IN100 cell data (bool/uint8/int32/float32) is exactly
 * representable as float64, so equality of the widened values is equality of the
 * originals.
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
      // Non-IDataArray members (NeighborList, StringArray) are excluded from the NX
      // transfer by GenerateDataArrayList and therefore from this snapshot. Legacy
      // 6.5.171 DID copy them - see deviation NeighborOrientationCorrelationFilter-D5.
      // If the transfer's scope ever widens, widen this snapshot with it.
      continue;
    }
    snapshots.push_back({arrayPath, arrayPtr->getNumberOfComponents(), SnapshotArray(*arrayPtr)});
  }
  return snapshots;
}
} // namespace SmallIn100Invariants
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

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

  auto* filterList = Application::Instance()->getFilterList();

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

  // Snapshot the full cell data before running the filter. There is deliberately NO
  // golden-output (exemplar) comparison in this test: exact expected outputs are pinned
  // by the inline oracle fixtures below, whose values are derived independently of the
  // implementation. This test verifies the Class 4 invariants at production scale.
  const std::vector<SmallIn100Invariants::CellArraySnapshot> preFilterCellData = SmallIn100Invariants::SnapshotCellArrays(dataStructure, k_CellAttributeMatrix);
  REQUIRE(!preFilterCellData.empty());

  constexpr float32 k_SmallIn100MinConfidence = 0.2f;

  // Neighbor Orientation Correlation Filter
  {
    auto filter = filterList->createFilter(k_NeighborOrientationCorrelationFilterHandle);
    REQUIRE(nullptr != filter);

    Arguments args;
    // Create default Parameters for the filter.
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_MinConfidence_Key, std::make_any<float32>(k_SmallIn100MinConfidence));
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

  // Class 4 invariant verification against the pre-filter snapshot (archive-free):
  //   I1  - a cell whose pre-filter Confidence Index was >= MinConfidence is never modified,
  //         in any cell array.
  //   I1b - every modified cell was a low-confidence cell.
  //   Smoke - the filter modified at least one cell (Small IN100 has low-confidence cells).
  {
    const std::vector<SmallIn100Invariants::CellArraySnapshot> postFilterCellData = SmallIn100Invariants::SnapshotCellArrays(dataStructure, k_CellAttributeMatrix);
    REQUIRE(postFilterCellData.size() == preFilterCellData.size());

    // Locate the pre-filter Confidence Index values
    const std::vector<float64>* preCIPtr = nullptr;
    for(const auto& snapshot : preFilterCellData)
    {
      if(snapshot.path == k_ConfidenceIndexArrayPath)
      {
        preCIPtr = &snapshot.values;
      }
    }
    REQUIRE(preCIPtr != nullptr);
    const std::vector<float64>& preCI = *preCIPtr;
    const usize numCells = preCI.size();

    // Mark every cell whose tuple changed in ANY cell array
    std::vector<bool> cellModified(numCells, false);
    for(usize arrayIdx = 0; arrayIdx < preFilterCellData.size(); arrayIdx++)
    {
      const auto& before = preFilterCellData[arrayIdx];
      const auto& after = postFilterCellData[arrayIdx];
      REQUIRE(after.path == before.path);
      REQUIRE(after.values.size() == before.values.size());
      const usize comps = before.numComponents;
      for(usize valueIdx = 0; valueIdx < before.values.size(); valueIdx++)
      {
        if(before.values[valueIdx] != after.values[valueIdx])
        {
          cellModified[valueIdx / comps] = true;
        }
      }
    }

    usize modifiedCount = 0;
    usize highConfidenceViolations = 0;
    for(usize cell = 0; cell < numCells; cell++)
    {
      if(cellModified[cell])
      {
        modifiedCount++;
        if(preCI[cell] >= static_cast<float64>(k_SmallIn100MinConfidence))
        {
          highConfidenceViolations++;
        }
      }
    }
    INFO(fmt::format("{} of {} cells modified; {} high-confidence cells illegally modified", modifiedCount, numCells, highConfidenceViolations));
    REQUIRE(highConfidenceViolations == 0);
    REQUIRE(modifiedCount > 0);
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
  // neighbor pairs similar -> every simCount = 5 (tie). Last-of-ties = +Z neighbor 87
  // (same pick as 6.5.171 on fully-tied neighborhoods).
  OracleFixture fixture(5, 5, 5, {999, 1});
  const usize center = fixture.idx(2, 2, 2);
  fixture.ci[center] = k_BadCI;
  fixture.anglesDeg[center] = 30.0;

  DataStructure dataStructure = BuildDataStructure(fixture);
  const CellSnapshot before = Capture(dataStructure);
  ExecuteFixture(dataStructure, 5);

  // Class 1 expected: cell 62 holds the full ORIGINAL tuple of cell 87; nothing else moves (I1).
  VerifyAgainstSnapshot(dataStructure, before, {{62, 87}});
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
  // simCount 1 each). The argmax must pick the last of the count-3 maxes, +X (63).
  // The legacy last-wins defect (deviation D3) picked +Z (87) - a count-1 neighbor
  // beating a count-3 neighbor. This fixture is the primary D3 regression pin.
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
// 3x3x3 bad cube (x,y,z in 2..4) of pairwise-dissimilar garbage (distance-2 mod-7
// coloring, angles 6..42 deg) inside a uniform 0-deg 7x7x7 good volume. Erosion fills
// corners+edges (pass 1), face centers (pass 2), cube center (pass 3). The legacy
// double-decrement defect (deviation D2) halved the intended 6-Level pass count.
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

  // Level 2 -> 4 passes; the cascade needs 3, so the cube must be fully filled. The
  // legacy double-decrement schedule ran only 2 passes and left the cube center bad.
  OracleFixture fixture = BuildCascadeFixture();
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

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F10 - pass schedule Level 4 (D2 regression)", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // Level 4 -> 2 passes: corners+edges then face centers fill; only the cube center
  // remains unfilled. The legacy double-decrement schedule ran a single pass.
  OracleFixture fixture = BuildCascadeFixture();
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

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F05 - mixed-phase pair never similar (D1 regression)", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // 2D 5x5x1, center (2,2)=12 bad, phase 1, 10 deg. Neighbors: -Y(7)=0 deg ph1,
  // -X(11)=1 deg ph1, +X(13)=2 deg PHASE 2, +Y(17)=40 deg ph1. Only legitimate similar
  // pair is (-Y,-X) -> counts (1,1,0,0) -> last-of-ties picks -X (11) and the phase stays 1.
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

  VerifyAgainstSnapshot(dataStructure, before, {{12, 11}});
  const auto& phasesRef = dataStructure.getDataRefAs<Int32Array>(k_CellPhasesPath).getDataStoreRef();
  REQUIRE(phasesRef[center] == 1);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F06 - phase-0 neighbors never counted", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // -Z(37)=0 deg / -Y(57)=1 deg are the only similar pair (counts 1,1 -> last of ties
  // = -Y, 57); -X(61)=30, +X(63)=38 deg; +Y(67), +Z(87) are phase 0 (unindexed) and
  // must never be counted or chosen.
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

  // 5x5x1: center (2,2)=12 bad; the 4 in-plane neighbors (7,11,13,17) all 0 deg ->
  // counts all 3 (tie) -> last-of-ties = +Y neighbor 17. Exercises the z-degenerate
  // boundary masks.
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

  // Discriminating Laue-class test. The bad center 62's six face neighbors are set so that exactly
  // ONE pair — the -Z neighbor 37 (0 deg) and the +Z neighbor 87 (58 deg) — has a c-axis
  // misorientation of 58 deg. The four remaining neighbors (57,61,63,67 at 20,27,34,41 deg) are
  // mutually dissimilar and dissimilar to both 0 and 58 under either Laue class.
  //
  // A 58 deg misorientation about the c-axis folds to:
  //   * Hexagonal-High (6/mmm, 60 deg periodicity): min(58, 60-58) = 2 deg  -> < 5 deg tol -> SIMILAR
  //   * Cubic-High     (m-3m, 90 deg periodicity):  min(58, 90-58) = 32 deg -> > 5 deg tol -> NOT similar
  //
  // So the SAME fixture must give different results, which pins the Laue-class dispatch's folding
  // (a bug that folded hex with cubic periodicity, or vice versa, would break exactly one section):
  //   * Hex:   the 37/87 pair is similar; both get count 1, last-of-ties (+Z, 87) wins -> center<-87.
  //   * Cubic: no pair is similar; all counts 0 -> center untouched.
  const std::array<usize, 6> neighbors = {37, 57, 61, 63, 67, 87};
  const std::array<float64, 6> angles = {0.0, 20.0, 27.0, 34.0, 41.0, 58.0};

  SECTION("Hexagonal-High: 58 deg folds to 2 deg -> center replaced by +Z neighbor 87")
  {
    OracleFixture fixture(5, 5, 5, {999, 0}); // phase 1 = Hexagonal-High
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
    OracleFixture fixture(5, 5, 5, {999, 1}); // phase 1 = Cubic-High
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

  // F01 with Payload2 in IgnoredDataArrayPaths: Payload2 must be bit-identical to input
  // while every other array is copied from cell 87.
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

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F11 - volume corner", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // 4x4x4, bad cell at corner (0,0,0): only +X(1)=0 deg, +Y(4)=1 deg, +Z(16)=2 deg are
  // valid; all 3 pairs similar -> counts 2 each (tie) -> last-of-ties = +Z neighbor 16.
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

  VerifyAgainstSnapshot(dataStructure, before, {{0, 16}});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F12 - anisotropic dims", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  using namespace NOCOracle;
  UnitTest::LoadPlugins();

  // 4x5x3 (nx != ny != nz) so an x/y/z stride or axis-swap bug cannot hide behind
  // dimension symmetry. Bad cell (1,2,1) = 29; neighbors in scan order:
  // -Z(9)=0, -Y(25)=1, -X(28)=2, +X(30)=20, +Y(33)=21, +Z(49)=40 deg.
  // Counts: clique {-Z,-Y,-X} = 2 each, pair {+X,+Y} = 1 each, +Z = 0.
  // Argmax picks the last of the count-2 maxes, -X (28); the legacy last-wins defect
  // (deviation D3) would pick +Y (33).
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
