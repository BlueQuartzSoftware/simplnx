#include "SimplnxCore/Filters/Algorithms/RemoveFlaggedFeatures.hpp"
#include "SimplnxCore/Filters/RemoveFlaggedFeaturesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <numeric>
#include <vector>

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;
namespace fs = std::filesystem;

/*
 * V&V test suite for RemoveFlaggedFeaturesFilter. See src/Plugins/SimplnxCore/vv/RemoveFlaggedFeaturesFilter.md.
 *
 * Oracle: Class 1 (analytical) on hand-built fixtures, plus Class 4 (invariant) on Small IN100.
 *
 * Every fixture carries three Cell arrays so the fill pass can be checked cell-by-cell:
 *   FeatureIds    - the segmentation under test.
 *   CellValue     - 100 + cell index. Unique per cell, so after a fill the value in a vacated cell
 *                   identifies exactly WHICH neighbor cell it was copied from.
 *   IgnoredValue  - 500 + cell index. Passed as an ignored array and must never change.
 * One Feature array, Int32DataSet = 1000 + feature id, lets the compaction be checked tuple-by-tuple.
 *
 * Fixtures are lettered A, B and E to match the legacy A/B comparison set, where C, D and F are the
 * error-path fixtures that appear here under their error names.
 *
 * Fill semantics under test (shared with Keep/Remove Ranked Features via FeatureRemovalUtilities):
 * every vacated cell (FeatureId -1) polls its six face neighbors in the order
 * -Z, -Y, -X, +X, +Y, +Z, tallies each non-negative neighbor FeatureId (0 counts), and copies the Cell
 * tuple of the neighbor whose feature reached the highest tally first. Ties resolve to the feature seen
 * first in that order. Cells with no non-negative neighbor wait for the next iteration.
 */
namespace
{
constexpr StringLiteral k_CellValueName = "CellValue";
constexpr StringLiteral k_IgnoredValueName = "IgnoredValue";
const std::string k_NewImgGeomPrefix = "NewImgGeom";

const DataPath k_ImageGeomPath({k_DataContainer});
const DataPath k_CellDataPath({k_DataContainer, k_CellData});
const DataPath k_FeatureIdsPath({k_DataContainer, k_CellData, k_FeatureIds});
const DataPath k_CellValuePath({k_DataContainer, k_CellData, k_CellValueName});
const DataPath k_IgnoredValuePath({k_DataContainer, k_CellData, k_IgnoredValueName});
const DataPath k_FeatureAMPath({k_DataContainer, k_CellFeatureData});
const DataPath k_FlaggedFeaturesPath({k_DataContainer, k_CellFeatureData, k_ActiveName});
const DataPath k_FeatureValuePath({k_DataContainer, k_CellFeatureData, k_Int32DataSet});

constexpr uint64 k_Remove = to_underlying(Functionality::Remove);
constexpr uint64 k_Extract = to_underlying(Functionality::Extract);
constexpr uint64 k_ExtractThenRemove = to_underlying(Functionality::ExtractThenRemove);

constexpr int32 k_AllFeaturesFlaggedError = -45433;
constexpr int32 k_FeatureIdOutOfRangeError = -45435;
constexpr int32 k_NoFillProgressError = -45436;
constexpr int32 k_TupleCountMismatchError = -45437;
constexpr int32 k_FeatureIdsCannotBeIgnoredWarning = -45438;
constexpr int32 k_ParentNotAttributeMatrixError = -9892;
constexpr int32 k_NeighborListRemovalWarning = -5558;
constexpr int32 k_EmptyFeatureSkippedWarning = -53905;

struct FixtureSpec
{
  SizeVec3 dims;                 // (x, y, z)
  std::vector<int32> featureIds; // x fastest, then y, then z
  usize numFeatures;             // tuple count of the Feature Attribute Matrix (index 0 is the unused feature)
  std::vector<bool> flagged;     // size numFeatures; true means remove/extract
};

std::vector<usize> TupleDims(const SizeVec3& dims)
{
  return {dims[2], dims[1], dims[0]};
}

/**
 * @brief Builds an Image Geometry fixture. Every cell array is populated so that copies can be traced.
 */
void BuildFixture(DataStructure& dataStructure, const FixtureSpec& spec)
{
  REQUIRE(spec.featureIds.size() == spec.dims[0] * spec.dims[1] * spec.dims[2]);
  REQUIRE(spec.flagged.size() == spec.numFeatures);

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_DataContainer);
  imageGeom->setDimensions(spec.dims);
  imageGeom->setOrigin(FloatVec3{0.0f, 0.0f, 0.0f});
  imageGeom->setSpacing(FloatVec3{1.0f, 1.0f, 1.0f});

  const std::vector<usize> tupleDims = TupleDims(spec.dims);
  auto* cellAM = AttributeMatrix::Create(dataStructure, k_CellData, tupleDims, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  auto& featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, k_FeatureIds, tupleDims, {1}, cellAM->getId())->getDataStoreRef();
  auto& cellValue = UnitTest::CreateTestDataArray<int32>(dataStructure, k_CellValueName, tupleDims, {1}, cellAM->getId())->getDataStoreRef();
  auto& ignoredValue = UnitTest::CreateTestDataArray<int32>(dataStructure, k_IgnoredValueName, tupleDims, {1}, cellAM->getId())->getDataStoreRef();
  for(usize i = 0; i < spec.featureIds.size(); i++)
  {
    // Cell values are unique per cell so a fill copy is traceable to its source cell.
    featureIds[i] = spec.featureIds[i];
    cellValue[i] = static_cast<int32>(100 + i);
    ignoredValue[i] = static_cast<int32>(500 + i);
  }

  auto* featureAM = AttributeMatrix::Create(dataStructure, k_CellFeatureData, {spec.numFeatures}, imageGeom->getId());
  auto& flags = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, k_ActiveName, {spec.numFeatures}, {1}, featureAM->getId())->getDataStoreRef();
  auto& featureValue = UnitTest::CreateTestDataArray<int32>(dataStructure, k_Int32DataSet, {spec.numFeatures}, {1}, featureAM->getId())->getDataStoreRef();
  for(usize i = 0; i < spec.numFeatures; i++)
  {
    flags[i] = spec.flagged[i];
    featureValue[i] = static_cast<int32>(1000 + i);
  }
}

Arguments MakeArgs(uint64 functionality, bool fill, const std::vector<DataPath>& ignored = {k_IgnoredValuePath})
{
  Arguments args;
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_Functionality_Key, std::make_any<ChoicesParameter::ValueType>(functionality));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_FillRemovedFeatures_Key, std::make_any<bool>(fill));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_CreatedImageGeometryPrefix_Key, std::make_any<std::string>(k_NewImgGeomPrefix));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_FlaggedFeaturesArrayPath_Key, std::make_any<DataPath>(k_FlaggedFeaturesPath));
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(ignored));
  return args;
}

std::vector<int32> ReadInt32(const DataStructure& dataStructure, const DataPath& path)
{
  const auto& store = dataStructure.getDataRefAs<Int32Array>(path).getDataStoreRef();
  std::vector<int32> values(store.getNumberOfTuples());
  for(usize i = 0; i < values.size(); i++)
  {
    values[i] = store[i];
  }
  return values;
}

std::vector<int32> Sequence(int32 start, usize count)
{
  std::vector<int32> values(count);
  std::iota(values.begin(), values.end(), start);
  return values;
}

/**
 * @brief Runs preflight and execute, requiring both to be valid, and returns the execute result.
 */
IFilter::ExecuteResult RunValid(DataStructure& dataStructure, const Arguments& args)
{
  RemoveFlaggedFeaturesFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  return executeResult;
}

/**
 * @brief Runs preflight (required valid) and execute (required invalid) and returns the first error code.
 */
int32 RunExecuteError(DataStructure& dataStructure, const Arguments& args)
{
  RemoveFlaggedFeaturesFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(!executeResult.result.errors().empty());
  return executeResult.result.errors()[0].code;
}

bool HasWarningCode(const std::vector<Warning>& warnings, int32 code)
{
  return std::any_of(warnings.begin(), warnings.end(), [code](const Warning& warning) { return warning.code == code; });
}

// ---------------------------------------------------------------------------------------------------
// Fixture A: the reproduction from GitHub issue #1698. 5x2x1, features 2 and 3 removed.
//
//   y=0:  1 1 1 2 3      idx 0..4
//   y=1:  1 4 4 4 3      idx 5..9
//
// Vacated cells 3, 4 and 9. Every valid neighbor of cell 3 is a distinct feature (1 at -X, 4 at +Y)
// and cell 9 has exactly one valid neighbor (4 at -X), so a tally that only records a source on the
// second sighting of a feature never fills either cell.
// ---------------------------------------------------------------------------------------------------
FixtureSpec FixtureA()
{
  return {SizeVec3{5, 2, 1}, {1, 1, 1, 2, 3, 1, 4, 4, 4, 3}, 5, {false, false, true, true, false}};
}

// ---------------------------------------------------------------------------------------------------
// Fixture B: 4x4x1 with background (FeatureId 0) cells. Feature 3 removed.
//
//   y=0:  0 1 1 1      idx  0..3
//   y=1:  1 0 2 2      idx  4..7
//   y=2:  2 2 0 1      idx  8..11
//   y=3:  2 3 3 0      idx 12..15
//
// Cell 14's only non-negative neighbors are background cells 10 (-Y) and 15 (+X), so it is filled FROM
// background and ends at 0. Background cells themselves are never fill targets.
// ---------------------------------------------------------------------------------------------------
FixtureSpec FixtureB()
{
  return {SizeVec3{4, 4, 1}, {0, 1, 1, 1, 1, 0, 2, 2, 2, 2, 0, 1, 2, 3, 3, 0}, 4, {false, false, false, true}};
}

// ---------------------------------------------------------------------------------------------------
// Fixture E: 3x3x3 exercising all six face directions, a 4-vs-2 majority vote where the minority
// feature is seen first, and a vacated cell (0,0,0) enclosed by other vacated cells so that it can
// only be filled on the second iteration. Features 3 and 4 removed.
//
//   z=0:  4 4 1 / 4 2 1 / 1 1 1      idx  0..8
//   z=1:  4 1 1 / 2 3 2 / 1 1 1      idx  9..17
//   z=2:  1 1 1 / 1 2 1 / 1 1 1      idx 18..26
// ---------------------------------------------------------------------------------------------------
FixtureSpec FixtureE()
{
  return {SizeVec3{3, 3, 3},
          {
              4, 4, 1, 4, 2, 1, 1, 1, 1, // z = 0
              4, 1, 1, 2, 3, 2, 1, 1, 1, // z = 1
              1, 1, 1, 1, 2, 1, 1, 1, 1, // z = 2
          },
          5,
          {false, false, false, true, true}};
}

/**
 * @brief Checks that the Feature Attribute Matrix was compacted to exactly the expected surviving tuples, in order.
 */
void CheckFeatureArraysCompacted(const DataStructure& dataStructure, const std::vector<int32>& expectedFeatureValues)
{
  const auto& featureAM = dataStructure.getDataRefAs<AttributeMatrix>(k_FeatureAMPath);
  REQUIRE(featureAM.getNumberOfTuples() == expectedFeatureValues.size());
  REQUIRE(ReadInt32(dataStructure, k_FeatureValuePath) == expectedFeatureValues);
  const auto& flags = dataStructure.getDataRefAs<BoolArray>(k_FlaggedFeaturesPath);
  REQUIRE(flags.getNumberOfTuples() == expectedFeatureValues.size());
}
} // namespace

// =====================================================================================================
// Class 1 analytical oracle: Remove
// =====================================================================================================

TEST_CASE("SimplnxCore::RemoveFlaggedFeaturesFilter: Class 1 Oracle - Remove without fill", "[SimplnxCore][RemoveFlaggedFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("Fixture A (5x2x1)")
  {
    DataStructure dataStructure;
    BuildFixture(dataStructure, FixtureA());
    RunValid(dataStructure, MakeArgs(k_Remove, false));

    // Removed cells go to 0. Survivors 1 and 4 renumber to 1 and 2.
    REQUIRE(ReadInt32(dataStructure, k_FeatureIdsPath) == std::vector<int32>{1, 1, 1, 0, 0, 1, 2, 2, 2, 0});
    // No fill, so no Cell tuple is copied.
    REQUIRE(ReadInt32(dataStructure, k_CellValuePath) == Sequence(100, 10));
    REQUIRE(ReadInt32(dataStructure, k_IgnoredValuePath) == Sequence(500, 10));
    // Feature tuples 0, 1 and 4 are kept in order.
    CheckFeatureArraysCompacted(dataStructure, {1000, 1001, 1004});
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("Fixture B (4x4x1 with background)")
  {
    DataStructure dataStructure;
    BuildFixture(dataStructure, FixtureB());
    RunValid(dataStructure, MakeArgs(k_Remove, false));

    REQUIRE(ReadInt32(dataStructure, k_FeatureIdsPath) == std::vector<int32>{0, 1, 1, 1, 1, 0, 2, 2, 2, 2, 0, 1, 2, 0, 0, 0});
    REQUIRE(ReadInt32(dataStructure, k_CellValuePath) == Sequence(100, 16));
    CheckFeatureArraysCompacted(dataStructure, {1000, 1001, 1002});
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::RemoveFlaggedFeaturesFilter: Class 1 Oracle - Fill from distinct first-sighting neighbors (issue #1698)", "[SimplnxCore][RemoveFlaggedFeaturesFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  BuildFixture(dataStructure, FixtureA());
  RunValid(dataStructure, MakeArgs(k_Remove, true));

  // Iteration 1 (state 1 1 1 -1 -1 / 1 4 4 4 -1):
  //   cell 3: -X = cell 2 (feature 1, first sighting -> source), +X = cell 4 (-1), +Y = cell 8 (feature 4, tally 1, not > 1).
  //           -> feature 1 from cell 2.
  //   cell 4: -X = cell 3 (-1), +Y = cell 9 (-1). No source; waits.
  //   cell 9: -Y = cell 4 (-1), -X = cell 8 (feature 4). -> feature 4 from cell 8.
  // Iteration 2 (state 1 1 1 1 -1 / 1 4 4 4 4):
  //   cell 4: -X = cell 3 (feature 1, first sighting -> source), +Y = cell 9 (feature 4, tie, loses).
  //           -> feature 1 from cell 3, whose CellValue is already the copy of cell 2 (102).
  // Compaction: 1 -> 1, 4 -> 2.
  REQUIRE(ReadInt32(dataStructure, k_FeatureIdsPath) == std::vector<int32>{1, 1, 1, 1, 1, 1, 2, 2, 2, 2});

  std::vector<int32> expectedCellValue = Sequence(100, 10);
  expectedCellValue[3] = 102;
  expectedCellValue[9] = 108;
  expectedCellValue[4] = 102;
  REQUIRE(ReadInt32(dataStructure, k_CellValuePath) == expectedCellValue);

  // The ignored array is never touched by the fill.
  REQUIRE(ReadInt32(dataStructure, k_IgnoredValuePath) == Sequence(500, 10));

  CheckFeatureArraysCompacted(dataStructure, {1000, 1001, 1004});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RemoveFlaggedFeaturesFilter: Class 1 Oracle - Fill copies every non-ignored Cell array", "[SimplnxCore][RemoveFlaggedFeaturesFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  BuildFixture(dataStructure, FixtureA());
  // Empty ignore list: IgnoredValue must now follow the same copies as CellValue.
  RunValid(dataStructure, MakeArgs(k_Remove, true, {}));

  REQUIRE(ReadInt32(dataStructure, k_FeatureIdsPath) == std::vector<int32>{1, 1, 1, 1, 1, 1, 2, 2, 2, 2});
  std::vector<int32> expectedIgnored = Sequence(500, 10);
  expectedIgnored[3] = 502;
  expectedIgnored[9] = 508;
  expectedIgnored[4] = 502;
  REQUIRE(ReadInt32(dataStructure, k_IgnoredValuePath) == expectedIgnored);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RemoveFlaggedFeaturesFilter: Fill cannot ignore the Feature Ids array (-45438 warning)", "[SimplnxCore][RemoveFlaggedFeaturesFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  BuildFixture(dataStructure, FixtureA());
  // Listing FeatureIds among the ignored arrays used to leave every vacated cell at -1 forever, because
  // sources were chosen but the FeatureIds array was never copied. The array is now always copied.
  auto executeResult = RunValid(dataStructure, MakeArgs(k_Remove, true, {k_FeatureIdsPath, k_IgnoredValuePath}));
  REQUIRE(executeResult.result.warnings().size() == 1);
  REQUIRE(executeResult.result.warnings()[0].code == k_FeatureIdsCannotBeIgnoredWarning);

  // Same result as the issue #1698 test.
  REQUIRE(ReadInt32(dataStructure, k_FeatureIdsPath) == std::vector<int32>{1, 1, 1, 1, 1, 1, 2, 2, 2, 2});
  std::vector<int32> expectedCellValue = Sequence(100, 10);
  expectedCellValue[3] = 102;
  expectedCellValue[9] = 108;
  expectedCellValue[4] = 102;
  REQUIRE(ReadInt32(dataStructure, k_CellValuePath) == expectedCellValue);
  REQUIRE(ReadInt32(dataStructure, k_IgnoredValuePath) == Sequence(500, 10));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RemoveFlaggedFeaturesFilter: Class 1 Oracle - Fill treats FeatureId 0 as a source and never a target", "[SimplnxCore][RemoveFlaggedFeaturesFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  BuildFixture(dataStructure, FixtureB());
  RunValid(dataStructure, MakeArgs(k_Remove, true));

  // Iteration 1 (row y=3 is 2 -1 -1 0):
  //   cell 13: -Y = cell 9 (feature 2, tally 1 -> source 9), -X = cell 12 (feature 2, tally 2 -> source 12), +X = cell 14 (-1).
  //            -> feature 2 from cell 12.
  //   cell 14: -Y = cell 10 (feature 0, tally 1 -> source 10), -X = cell 13 (-1), +X = cell 15 (feature 0, tally 2 -> source 15).
  //            -> feature 0 from cell 15. Background is a legal fill source, matching DREAM3D 6.5.171.
  // Iteration 2: no negative cells remain. The five background cells were never candidates.
  // Compaction: 1 -> 1, 2 -> 2.
  REQUIRE(ReadInt32(dataStructure, k_FeatureIdsPath) == std::vector<int32>{0, 1, 1, 1, 1, 0, 2, 2, 2, 2, 0, 1, 2, 2, 0, 0});

  std::vector<int32> expectedCellValue = Sequence(100, 16);
  expectedCellValue[13] = 112;
  expectedCellValue[14] = 115;
  REQUIRE(ReadInt32(dataStructure, k_CellValuePath) == expectedCellValue);
  REQUIRE(ReadInt32(dataStructure, k_IgnoredValuePath) == Sequence(500, 16));

  CheckFeatureArraysCompacted(dataStructure, {1000, 1001, 1002});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RemoveFlaggedFeaturesFilter: Class 1 Oracle - Fill majority vote and second iteration (3x3x3)", "[SimplnxCore][RemoveFlaggedFeaturesFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  BuildFixture(dataStructure, FixtureE());
  RunValid(dataStructure, MakeArgs(k_Remove, true));

  // Vacated cells: 0, 1, 3, 9 (feature 4) and 13 (feature 3). Neighbor order is -Z, -Y, -X, +X, +Y, +Z.
  // Iteration 1:
  //   cell 0  (0,0,0): +X = 1 (-1), +Y = 3 (-1), +Z = 9 (-1). No source; waits.
  //   cell 1  (1,0,0): -X = 0 (-1), +X = 2 (f1, tally 1 -> src 2), +Y = 4 (f2, tally 1), +Z = 10 (f1, tally 2 -> src 10).
  //                    -> feature 1 from cell 10.
  //   cell 3  (0,1,0): -Y = 0 (-1), +X = 4 (f2, tally 1 -> src 4), +Y = 6 (f1, tally 1), +Z = 12 (f2, tally 2 -> src 12).
  //                    -> feature 2 from cell 12.
  //   cell 9  (0,0,1): -Z = 0 (-1), +X = 10 (f1, tally 1 -> src 10), +Y = 12 (f2, tally 1), +Z = 18 (f1, tally 2 -> src 18).
  //                    -> feature 1 from cell 18.
  //   cell 13 (1,1,1): -Z = 4 (f2 -> src 4), -Y = 10 (f1, tally 1), -X = 12 (f2, tally 2 -> src 12), +X = 14 (f2, tally 3 -> src 14),
  //                    +Y = 16 (f1, tally 2), +Z = 22 (f2, tally 4 -> src 22).
  //                    -> feature 2 wins 4 to 2 from cell 22, even though feature 1 was in the running.
  // Iteration 2:
  //   cell 0: +X = 1 (f1, tally 1 -> src 1), +Y = 3 (f2, tally 1), +Z = 9 (f1, tally 2 -> src 9).
  //           -> feature 1 from cell 9, whose CellValue is already the copy of cell 18 (118).
  // Iteration 3: nothing left. Compaction: 1 -> 1, 2 -> 2.
  const std::vector<int32> expectedFeatureIds = {
      1, 1, 1, 2, 2, 1, 1, 1, 1, // z = 0
      1, 1, 1, 2, 2, 2, 1, 1, 1, // z = 1
      1, 1, 1, 1, 2, 1, 1, 1, 1, // z = 2
  };
  REQUIRE(ReadInt32(dataStructure, k_FeatureIdsPath) == expectedFeatureIds);

  std::vector<int32> expectedCellValue = Sequence(100, 27);
  expectedCellValue[1] = 110;
  expectedCellValue[3] = 112;
  expectedCellValue[9] = 118;
  expectedCellValue[13] = 122;
  expectedCellValue[0] = 118;
  REQUIRE(ReadInt32(dataStructure, k_CellValuePath) == expectedCellValue);
  REQUIRE(ReadInt32(dataStructure, k_IgnoredValuePath) == Sequence(500, 27));

  CheckFeatureArraysCompacted(dataStructure, {1000, 1001, 1002});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// =====================================================================================================
// Execute errors
// =====================================================================================================

TEST_CASE("SimplnxCore::RemoveFlaggedFeaturesFilter: Execute Error - all features flagged (-45433)", "[SimplnxCore][RemoveFlaggedFeaturesFilter]")
{
  UnitTest::LoadPlugins();
  for(const bool fill : {false, true})
  {
    DYNAMIC_SECTION("Fill = " << fill)
    {
      DataStructure dataStructure;
      FixtureSpec spec = FixtureA();
      std::fill(spec.flagged.begin() + 1, spec.flagged.end(), true);
      BuildFixture(dataStructure, spec);
      REQUIRE(RunExecuteError(dataStructure, MakeArgs(k_Remove, fill)) == k_AllFeaturesFlaggedError);
      // Nothing was modified.
      REQUIRE(ReadInt32(dataStructure, k_FeatureIdsPath) == FixtureA().featureIds);
      CheckFeatureArraysCompacted(dataStructure, Sequence(1000, 5));
    }
  }
}

TEST_CASE("SimplnxCore::RemoveFlaggedFeaturesFilter: Execute Error - FeatureId out of range (-45435)", "[SimplnxCore][RemoveFlaggedFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("FeatureId equal to the feature tuple count")
  {
    DataStructure dataStructure;
    FixtureSpec spec = FixtureA();
    spec.featureIds[6] = 5; // feature Attribute Matrix holds tuples 0..4
    BuildFixture(dataStructure, spec);
    REQUIRE(RunExecuteError(dataStructure, MakeArgs(k_Remove, false)) == k_FeatureIdOutOfRangeError);
    REQUIRE(ReadInt32(dataStructure, k_FeatureIdsPath) == spec.featureIds);
  }

  SECTION("Negative FeatureId")
  {
    DataStructure dataStructure;
    FixtureSpec spec = FixtureA();
    spec.featureIds[0] = -1;
    BuildFixture(dataStructure, spec);
    REQUIRE(RunExecuteError(dataStructure, MakeArgs(k_Remove, true)) == k_FeatureIdOutOfRangeError);
    REQUIRE(ReadInt32(dataStructure, k_FeatureIdsPath) == spec.featureIds);
  }
}

TEST_CASE("SimplnxCore::RemoveFlaggedFeaturesFilter: Execute Error - Feature Ids tuple count differs from the geometry (-45437)", "[SimplnxCore][RemoveFlaggedFeaturesFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  BuildFixture(dataStructure, FixtureA());
  // A 4-tuple int32 array outside the geometry (an Attribute Matrix rejects a child with the wrong
  // tuple shape, so the mismatch can only come from an array that lives elsewhere).
  auto* otherGroup = DataGroup::Create(dataStructure, "OtherData");
  const DataPath shortIdsPath({"OtherData", "ShortIds"});
  auto* shortIdsArray = UnitTest::CreateTestDataArray<int32>(dataStructure, "ShortIds", {4}, {1}, otherGroup->getId());
  REQUIRE(shortIdsArray != nullptr);
  shortIdsArray->fill(1);

  Arguments args = MakeArgs(k_Remove, false);
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(shortIdsPath));
  REQUIRE(RunExecuteError(dataStructure, args) == k_TupleCountMismatchError);
  REQUIRE(ReadInt32(dataStructure, shortIdsPath) == std::vector<int32>{1, 1, 1, 1});
}

TEST_CASE("SimplnxCore::RemoveFlaggedFeaturesFilter: Execute Error - no fill progress (-45436)", "[SimplnxCore][RemoveFlaggedFeaturesFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  // Feature 2 owns no cells but is not flagged, so the all-flagged guard passes. Every cell belongs to
  // the flagged feature 1, so after marking no cell has a non-negative face neighbor and the dilation
  // can never make progress.
  BuildFixture(dataStructure, {SizeVec3{4, 1, 1}, {1, 1, 1, 1}, 3, {false, true, false}});
  REQUIRE(RunExecuteError(dataStructure, MakeArgs(k_Remove, true)) == k_NoFillProgressError);
  // The marking pass has already run; the error reports that the array was modified.
  REQUIRE(ReadInt32(dataStructure, k_FeatureIdsPath) == std::vector<int32>{-1, -1, -1, -1});
}

// =====================================================================================================
// Class 1 analytical oracle: Extract
// =====================================================================================================

TEST_CASE("SimplnxCore::RemoveFlaggedFeaturesFilter: Class 1 Oracle - Extract crops the bounding box of each flagged feature", "[SimplnxCore][RemoveFlaggedFeaturesFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  BuildFixture(dataStructure, FixtureB());
  RunValid(dataStructure, MakeArgs(k_Extract, false));

  // The original geometry is untouched by Extract.
  REQUIRE(ReadInt32(dataStructure, k_FeatureIdsPath) == FixtureB().featureIds);
  REQUIRE(ReadInt32(dataStructure, k_CellValuePath) == Sequence(100, 16));
  CheckFeatureArraysCompacted(dataStructure, Sequence(1000, 4));

  // Feature 3 occupies cells 13 (x=1,y=3) and 14 (x=2,y=3): bounds x 1..2, y 3..3, z 0..0.
  // Four feature tuples -> one digit of zero padding -> "NewImgGeom-3".
  const DataPath newGeomPath({k_NewImgGeomPrefix + "-3"});
  const auto& newGeom = dataStructure.getDataRefAs<ImageGeom>(newGeomPath);
  REQUIRE(newGeom.getDimensions() == SizeVec3{2, 1, 1});
  REQUIRE(newGeom.getOrigin() == FloatVec3{1.0f, 3.0f, 0.0f});
  REQUIRE(newGeom.getSpacing() == FloatVec3{1.0f, 1.0f, 1.0f});

  REQUIRE(ReadInt32(dataStructure, newGeomPath.createChildPath(k_CellData).createChildPath(k_FeatureIds)) == std::vector<int32>{3, 3});
  REQUIRE(ReadInt32(dataStructure, newGeomPath.createChildPath(k_CellData).createChildPath(k_CellValueName)) == std::vector<int32>{113, 114});
  REQUIRE(ReadInt32(dataStructure, newGeomPath.createChildPath(k_CellData).createChildPath(k_IgnoredValueName)) == std::vector<int32>{513, 514});

  // The Feature Attribute Matrix is carried over unchanged (features are not renumbered).
  const auto& newFeatureAM = dataStructure.getDataRefAs<AttributeMatrix>(newGeomPath.createChildPath(k_CellFeatureData));
  REQUIRE(newFeatureAM.getNumberOfTuples() == 4);
  REQUIRE(ReadInt32(dataStructure, newGeomPath.createChildPath(k_CellFeatureData).createChildPath(k_Int32DataSet)) == Sequence(1000, 4));

  // The temporary bounds array is deleted, and it was not copied into the extracted geometry.
  REQUIRE(dataStructure.getData(k_FeatureAMPath.createChildPath("tempBounds")) == nullptr);
  REQUIRE(dataStructure.getData(newGeomPath.createChildPath(k_CellFeatureData).createChildPath("tempBounds")) == nullptr);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RemoveFlaggedFeaturesFilter: Class 1 Oracle - Extract zero-pads names and handles several features", "[SimplnxCore][RemoveFlaggedFeaturesFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  // 12x1x1 with cell i belonging to feature i (cell 0 is background). 12 feature tuples -> two digits.
  std::vector<bool> flagged(12, false);
  flagged[3] = true;
  flagged[11] = true;
  BuildFixture(dataStructure, {SizeVec3{12, 1, 1}, Sequence(0, 12), 12, flagged});
  RunValid(dataStructure, MakeArgs(k_Extract, false));

  for(const int32 featureId : {3, 11})
  {
    const DataPath newGeomPath({fmt::format("{}-{:02d}", k_NewImgGeomPrefix, featureId)});
    const auto& newGeom = dataStructure.getDataRefAs<ImageGeom>(newGeomPath);
    REQUIRE(newGeom.getDimensions() == SizeVec3{1, 1, 1});
    REQUIRE(newGeom.getOrigin() == FloatVec3{static_cast<float32>(featureId), 0.0f, 0.0f});
    REQUIRE(ReadInt32(dataStructure, newGeomPath.createChildPath(k_CellData).createChildPath(k_FeatureIds)) == std::vector<int32>{featureId});
    REQUIRE(ReadInt32(dataStructure, newGeomPath.createChildPath(k_CellData).createChildPath(k_CellValueName)) == std::vector<int32>{100 + featureId});
  }
  // Unflagged features produce no geometry.
  REQUIRE(dataStructure.getData(DataPath({k_NewImgGeomPrefix + "-01"})) == nullptr);
  REQUIRE(dataStructure.getData(DataPath({k_NewImgGeomPrefix + "-1"})) == nullptr);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RemoveFlaggedFeaturesFilter: Extract - flagged feature with no cells is skipped with a warning", "[SimplnxCore][RemoveFlaggedFeaturesFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  // Features 4 and 5 exist in the Feature Attribute Matrix but own no cell.
  FixtureSpec spec = FixtureB();
  spec.numFeatures = 6;
  spec.flagged = {false, false, false, true, true, true};
  BuildFixture(dataStructure, spec);
  auto executeResult = RunValid(dataStructure, MakeArgs(k_Extract, false));
  // One aggregated warning that names both features, not one warning per feature.
  REQUIRE(executeResult.result.warnings().size() == 1);
  REQUIRE(executeResult.result.warnings()[0].code == k_EmptyFeatureSkippedWarning);
  REQUIRE(executeResult.result.warnings()[0].message.find("2 flagged feature(s)") != std::string::npos);
  REQUIRE(executeResult.result.warnings()[0].message.find("4, 5") != std::string::npos);

  // Feature 3 is still extracted; features 4 and 5 produce no geometry.
  REQUIRE(dataStructure.getData(DataPath({k_NewImgGeomPrefix + "-3"})) != nullptr);
  REQUIRE(dataStructure.getData(DataPath({k_NewImgGeomPrefix + "-4"})) == nullptr);
  REQUIRE(dataStructure.getData(DataPath({k_NewImgGeomPrefix + "-5"})) == nullptr);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RemoveFlaggedFeaturesFilter: Class 1 Oracle - Extract then Remove with fill", "[SimplnxCore][RemoveFlaggedFeaturesFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  BuildFixture(dataStructure, FixtureB());
  RunValid(dataStructure, MakeArgs(k_ExtractThenRemove, true));

  // Extraction happens first, on the unmodified data: same expectations as the Extract test.
  const DataPath newGeomPath({k_NewImgGeomPrefix + "-3"});
  const auto& newGeom = dataStructure.getDataRefAs<ImageGeom>(newGeomPath);
  REQUIRE(newGeom.getDimensions() == SizeVec3{2, 1, 1});
  REQUIRE(newGeom.getOrigin() == FloatVec3{1.0f, 3.0f, 0.0f});
  REQUIRE(ReadInt32(dataStructure, newGeomPath.createChildPath(k_CellData).createChildPath(k_FeatureIds)) == std::vector<int32>{3, 3});
  REQUIRE(ReadInt32(dataStructure, newGeomPath.createChildPath(k_CellData).createChildPath(k_CellValueName)) == std::vector<int32>{113, 114});
  REQUIRE(ReadInt32(dataStructure, newGeomPath.createChildPath(k_CellFeatureData).createChildPath(k_Int32DataSet)) == Sequence(1000, 4));

  // Then removal with fill, on the original: same expectations as the FeatureId 0 fill test.
  REQUIRE(ReadInt32(dataStructure, k_FeatureIdsPath) == std::vector<int32>{0, 1, 1, 1, 1, 0, 2, 2, 2, 2, 0, 1, 2, 2, 0, 0});
  std::vector<int32> expectedCellValue = Sequence(100, 16);
  expectedCellValue[13] = 112;
  expectedCellValue[14] = 115;
  REQUIRE(ReadInt32(dataStructure, k_CellValuePath) == expectedCellValue);
  CheckFeatureArraysCompacted(dataStructure, {1000, 1001, 1002});

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// =====================================================================================================
// Preflight
// =====================================================================================================

TEST_CASE("SimplnxCore::RemoveFlaggedFeaturesFilter: Preflight Error - flag array parent is not an Attribute Matrix (-9892)", "[SimplnxCore][RemoveFlaggedFeaturesFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  BuildFixture(dataStructure, FixtureA());
  // Put a second flag array in a plain DataGroup.
  auto* group = DataGroup::Create(dataStructure, "FlagGroup");
  BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, k_ActiveName, {5}, {1}, group->getId());

  Arguments args = MakeArgs(k_Remove, false);
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_FlaggedFeaturesArrayPath_Key, std::make_any<DataPath>(DataPath({"FlagGroup", k_ActiveName})));

  RemoveFlaggedFeaturesFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == k_ParentNotAttributeMatrixError);
}

TEST_CASE("SimplnxCore::RemoveFlaggedFeaturesFilter: Preflight Warning - NeighborLists are removed (-5558)", "[SimplnxCore][RemoveFlaggedFeaturesFilter]")
{
  UnitTest::LoadPlugins();
  for(const uint64 functionality : {k_Remove, k_ExtractThenRemove})
  {
    DYNAMIC_SECTION("Functionality = " << functionality)
    {
      DataStructure dataStructure;
      BuildFixture(dataStructure, FixtureA());
      const auto& featureAM = dataStructure.getDataRefAs<AttributeMatrix>(k_FeatureAMPath);
      const DataPath neighborListPath = k_FeatureAMPath.createChildPath("NeighborList");
      Int32NeighborList::Create(dataStructure, "NeighborList", featureAM.getShape(), featureAM.getId());
      REQUIRE(dataStructure.getData(neighborListPath) != nullptr);

      RemoveFlaggedFeaturesFilter filter;
      Arguments args = MakeArgs(functionality, false);
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
      // Exactly one warning for the NeighborList: the filter must not report it twice.
      REQUIRE(preflightResult.outputActions.warnings().size() == 1);
      REQUIRE(preflightResult.outputActions.warnings()[0].code == k_NeighborListRemovalWarning);

      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
      REQUIRE(dataStructure.getData(neighborListPath) == nullptr);
    }
  }

  SECTION("Extract alone does not remove NeighborLists")
  {
    DataStructure dataStructure;
    BuildFixture(dataStructure, FixtureA());
    const auto& featureAM = dataStructure.getDataRefAs<AttributeMatrix>(k_FeatureAMPath);
    const DataPath neighborListPath = k_FeatureAMPath.createChildPath("NeighborList");
    Int32NeighborList::Create(dataStructure, "NeighborList", featureAM.getShape(), featureAM.getId());

    RemoveFlaggedFeaturesFilter filter;
    Arguments args = MakeArgs(k_Extract, false);
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.warnings().empty());
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(dataStructure.getData(neighborListPath) != nullptr);
  }
}

// =====================================================================================================
// Class 4 invariants on Small IN100
// =====================================================================================================

TEST_CASE("SimplnxCore::RemoveFlaggedFeaturesFilter: Class 4 Invariants - Small IN100 remove small features with fill", "[SimplnxCore][RemoveFlaggedFeaturesFilter]")
{
  UnitTest::LoadPlugins();
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");
  const auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  const DataPath featureIdsPath({k_DataContainer, k_CellData, k_FeatureIds});
  const DataPath cellPhasesPath({k_DataContainer, k_CellData, "Phases"});
  const DataPath featureAMPath({k_DataContainer, k_CellFeatureData});
  const DataPath numElementsPath = featureAMPath.createChildPath("NumElements");
  const DataPath featurePhasesPath = featureAMPath.createChildPath("Phases");
  const DataPath flagsPath = featureAMPath.createChildPath("SmallFeatures");
  const DataPath neighborListPath = featureAMPath.createChildPath("NeighborList");
  REQUIRE(dataStructure.getData(neighborListPath) != nullptr);

  // Snapshot the input.
  const std::vector<int32> oldFeatureIds = ReadInt32(dataStructure, featureIdsPath);
  const std::vector<int32> oldCellPhases = ReadInt32(dataStructure, cellPhasesPath);
  const std::vector<int32> oldNumElements = ReadInt32(dataStructure, numElementsPath);
  const std::vector<int32> oldFeaturePhases = ReadInt32(dataStructure, featurePhasesPath);
  const usize numOldFeatures = oldNumElements.size();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({k_DataContainer}));
  const SizeVec3 dims = imageGeom.getDimensions();
  REQUIRE(oldFeatureIds.size() == dims[0] * dims[1] * dims[2]);

  // Input sanity: no background, every id in range, cell phase equals its feature's phase.
  usize inputViolations = 0;
  for(usize c = 0; c < oldFeatureIds.size(); c++)
  {
    if(oldFeatureIds[c] <= 0 || static_cast<usize>(oldFeatureIds[c]) >= numOldFeatures || oldCellPhases[c] != oldFeaturePhases[oldFeatureIds[c]])
    {
      inputViolations++;
    }
  }
  REQUIRE(inputViolations == 0);

  // Flag every feature with fewer than 100 cells (275 of 846 features on this data set).
  constexpr int32 k_MinCells = 100;
  const auto& featureAM = dataStructure.getDataRefAs<AttributeMatrix>(featureAMPath);
  auto& flags = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "SmallFeatures", {numOldFeatures}, {1}, featureAM.getId())->getDataStoreRef();
  // The flag array itself is compacted by the filter, so keep an independent copy for the checks below.
  std::vector<bool> flagged(numOldFeatures, false);
  std::vector<int32> newNames(numOldFeatures, 0);
  std::vector<int32> expectedNumElements = {oldNumElements[0]};
  std::vector<int32> expectedFeaturePhases = {oldFeaturePhases[0]};
  usize numFlagged = 0;
  for(usize f = 1; f < numOldFeatures; f++)
  {
    flagged[f] = oldNumElements[f] < k_MinCells;
    flags[f] = flagged[f];
    if(flagged[f])
    {
      numFlagged++;
    }
    else
    {
      newNames[f] = static_cast<int32>(expectedNumElements.size());
      expectedNumElements.push_back(oldNumElements[f]);
      expectedFeaturePhases.push_back(oldFeaturePhases[f]);
    }
  }
  REQUIRE(numFlagged == 275);
  const usize numSurvivors = expectedNumElements.size() - 1;

  Arguments args = MakeArgs(k_Remove, true, {});
  args.insertOrAssign(RemoveFlaggedFeaturesFilter::k_FlaggedFeaturesArrayPath_Key, std::make_any<DataPath>(flagsPath));

  RemoveFlaggedFeaturesFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(HasWarningCode(preflightResult.outputActions.warnings(), k_NeighborListRemovalWarning));
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const std::vector<int32> newFeatureIds = ReadInt32(dataStructure, featureIdsPath);
  const std::vector<int32> newCellPhases = ReadInt32(dataStructure, cellPhasesPath);
  REQUIRE(newFeatureIds.size() == oldFeatureIds.size());

  // I1: the Feature Attribute Matrix is compacted in order; feature-level arrays are copied, not recomputed.
  REQUIRE(featureAM.getNumberOfTuples() == numSurvivors + 1);
  REQUIRE(ReadInt32(dataStructure, numElementsPath) == expectedNumElements);
  REQUIRE(ReadInt32(dataStructure, featurePhasesPath) == expectedFeaturePhases);

  // I2: NeighborLists are gone.
  REQUIRE(dataStructure.getData(neighborListPath) == nullptr);

  // I3: every cell is assigned to a surviving feature; untouched cells follow the renumbering exactly.
  // I4: a cell's phase still matches its (new) feature's phase, so the copied tuples are self-consistent.
  // I5: every vacated cell shares its final id with at least one face neighbor (it was copied from one).
  std::vector<int32> newCounts(numSurvivors + 1, 0);
  usize numFilled = 0;
  const auto neighborHasSameId = [&](usize c, int32 id) {
    const usize z = c / (dims[0] * dims[1]);
    const usize y = (c / dims[0]) % dims[1];
    const usize x = c % dims[0];
    const std::array<std::pair<bool, int64>, 6> candidates = {{
        {z > 0, -static_cast<int64>(dims[0] * dims[1])},
        {y > 0, -static_cast<int64>(dims[0])},
        {x > 0, -1},
        {x < dims[0] - 1, 1},
        {y < dims[1] - 1, static_cast<int64>(dims[0])},
        {z < dims[2] - 1, static_cast<int64>(dims[0] * dims[1])},
    }};
    return std::any_of(candidates.begin(), candidates.end(), [&](const auto& cand) { return cand.first && newFeatureIds[static_cast<usize>(static_cast<int64>(c) + cand.second)] == id; });
  };
  // One million cells: count violations and assert once per invariant so a failure stays readable.
  usize outOfRange = 0;
  usize phaseMismatch = 0;
  usize filledWithoutMatchingNeighbor = 0;
  usize untouchedRenumberedWrong = 0;
  for(usize c = 0; c < newFeatureIds.size(); c++)
  {
    const int32 id = newFeatureIds[c];
    if(id <= 0 || static_cast<usize>(id) > numSurvivors)
    {
      outOfRange++;
      continue;
    }
    newCounts[id]++;
    if(newCellPhases[c] != expectedFeaturePhases[id])
    {
      phaseMismatch++;
    }
    if(flagged[oldFeatureIds[c]])
    {
      numFilled++;
      if(!neighborHasSameId(c, id))
      {
        filledWithoutMatchingNeighbor++;
      }
    }
    else if(id != newNames[oldFeatureIds[c]])
    {
      untouchedRenumberedWrong++;
    }
  }
  REQUIRE(outOfRange == 0);
  REQUIRE(phaseMismatch == 0);
  REQUIRE(filledWithoutMatchingNeighbor == 0);
  REQUIRE(untouchedRenumberedWrong == 0);
  // Snapshot of 6_5_test_data_1_v2: the cell count of the 275 flagged features. Changes only if the archive changes.
  REQUIRE(numFilled == 8535);

  // I6: surviving features only grow, and the growth accounts for every vacated cell.
  usize totalGrowth = 0;
  for(usize id = 1; id <= numSurvivors; id++)
  {
    REQUIRE(newCounts[id] >= expectedNumElements[id]);
    totalGrowth += static_cast<usize>(newCounts[id] - expectedNumElements[id]);
  }
  REQUIRE(totalGrowth == numFilled);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// =====================================================================================================
// SIMPL conversion
// =====================================================================================================

TEST_CASE("SimplnxCore::RemoveFlaggedFeaturesFilter: SIMPL Backwards Compatibility", "[SimplnxCore][RemoveFlaggedFeaturesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "RemoveFlaggedFeaturesFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "RemoveFlaggedFeaturesFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<RemoveFlaggedFeaturesFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<bool>(RemoveFlaggedFeaturesFilter::k_FillRemovedFeatures_Key) == true);
      CHECK(args.value<MultiArraySelectionParameter::ValueType>(RemoveFlaggedFeaturesFilter::k_IgnoredDataArrayPaths_Key) ==
            MultiArraySelectionParameter::ValueType{DataPath({"DC", "AM", "DA1"}), DataPath({"DC", "AM", "DA2"})});
      CHECK(args.value<DataPath>(RemoveFlaggedFeaturesFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(RemoveFlaggedFeaturesFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(RemoveFlaggedFeaturesFilter::k_FlaggedFeaturesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}
