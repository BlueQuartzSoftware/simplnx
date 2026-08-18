#include "SimplnxCore/Filters/ErodeDilateCoordinationNumberFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <fmt/format.h>

#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
namespace CoordinationFixture
{
constexpr usize k_XDim = 5;
constexpr usize k_YDim = 5;
constexpr usize k_ZDim = 1;
constexpr usize k_CellCount = k_XDim * k_YDim * k_ZDim;

using CellValues = std::array<int32, k_CellCount>;

const std::string k_ImageGeometryName("ImageGeometry");
const std::string k_CellDataName("CellData");
const std::string k_FeatureIdsName("FeatureIds");
const std::string k_CopiedValuesName("CopiedValues");
const std::string k_IgnoredValuesName("IgnoredValues");

constexpr int32 k_CopiedValueOffset = 100;
constexpr int32 k_IgnoredValueOffset = 200;

constexpr usize GetIndex(usize x, usize y)
{
  return y * k_XDim + x;
}

/**
 * @brief The fourteen GOOD voxels that the good-voxel branch reassigns during the
 * single sweep of Fixture A at CoordinationNumber <= 1. Index 12 is deliberately
 * absent: it is the one voxel the BAD-voxel branch reassigns, and it takes both a
 * different feature id and a different source tuple, so it is asserted separately.
 */
constexpr std::array<usize, 14> k_GoodVoxelBranchChangedIndices = {GetIndex(2, 1), GetIndex(3, 1), GetIndex(4, 1), GetIndex(1, 2), GetIndex(3, 2), GetIndex(4, 2), GetIndex(1, 3),
                                                                   GetIndex(2, 3), GetIndex(3, 3), GetIndex(4, 3), GetIndex(1, 4), GetIndex(2, 4), GetIndex(3, 4), GetIndex(4, 4)};

DataPath GeomPath()
{
  return DataPath({k_ImageGeometryName});
}

DataPath CellDataPath()
{
  return GeomPath().createChildPath(k_CellDataName);
}

DataPath FeatureIdsPath()
{
  return CellDataPath().createChildPath(k_FeatureIdsName);
}

DataPath CopiedValuesPath()
{
  return CellDataPath().createChildPath(k_CopiedValuesName);
}

DataPath IgnoredValuesPath()
{
  return CellDataPath().createChildPath(k_IgnoredValuesName);
}

/**
 * @brief Builds the 5x5x1 fixture. The companion arrays carry index-encoded
 * values so that the assertions pin down *which* neighbor tuple was copied,
 * not merely which feature won the vote.
 */
DataStructure BuildDataStructure(const CellValues& inputFeatureIds)
{
  DataStructure dataStructure;

  const SizeVec3 imageSize = {k_XDim, k_YDim, k_ZDim};
  const ShapeType cellShape = {k_ZDim, k_YDim, k_XDim};

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, k_ImageGeometryName);
  imageGeomPtr->setDimensions(imageSize);
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, k_CellDataName, cellShape, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);

  auto* featureIdsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, k_FeatureIdsName, cellShape, {1}, cellAmPtr->getId());
  auto* copiedValuesPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, k_CopiedValuesName, cellShape, {1}, cellAmPtr->getId());
  auto* ignoredValuesPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, k_IgnoredValuesName, cellShape, {1}, cellAmPtr->getId());

  for(usize i = 0; i < k_CellCount; i++)
  {
    featureIdsPtr->getDataStoreRef()[i] = inputFeatureIds[i];
    copiedValuesPtr->getDataStoreRef()[i] = k_CopiedValueOffset + static_cast<int32>(i);
    ignoredValuesPtr->getDataStoreRef()[i] = k_IgnoredValueOffset + static_cast<int32>(i);
  }

  return dataStructure;
}

Arguments MakeArgs(int32 coordinationNumber, bool loop)
{
  Arguments args;

  args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_CoordinationNumber_Key, std::make_any<int32>(coordinationNumber));
  args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_Loop_Key, std::make_any<bool>(loop));
  args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(GeomPath()));
  args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(FeatureIdsPath()));
  args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_IgnoredDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{IgnoredValuesPath()}));

  return args;
}

void RunFilter(DataStructure& dataStructure, int32 coordinationNumber, bool loop)
{
  const ErodeDilateCoordinationNumberFilter filter;
  const Arguments args = MakeArgs(coordinationNumber, loop);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
}

/**
 * @brief Asserts that preflight rejects the given parameter pair with exactly one error
 * carrying @p expectedCode, and that the message names the offending value. The message
 * substrings are asserted deliberately: an error a user cannot act on is not a guard,
 * and the V&V policy requires the actual value in the text.
 */
void CheckPreflightRejects(const DataStructure& dataStructure, int32 coordinationNumber, bool loop, int32 expectedCode, const std::vector<std::string>& expectedMessageParts)
{
  const ErodeDilateCoordinationNumberFilter filter;

  auto preflightResult = filter.preflight(dataStructure, MakeArgs(coordinationNumber, loop));
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  const auto& errors = preflightResult.outputActions.errors();
  REQUIRE(errors.size() == 1);
  REQUIRE(errors[0].code == expectedCode);
  for(const std::string& part : expectedMessageParts)
  {
    CAPTURE(errors[0].message, part);
    REQUIRE(errors[0].message.find(part) != std::string::npos);
  }
}

/**
 * @brief Asserts that preflight accepts the given parameter pair *silently* - valid and with no
 * warnings at all. The empty-warnings assertion is what pins the non-termination warning's
 * non-overreach: a guard widened past its intended combination shows up here as a stray warning.
 */
void CheckPreflightAccepts(const DataStructure& dataStructure, int32 coordinationNumber, bool loop)
{
  const ErodeDilateCoordinationNumberFilter filter;

  auto preflightResult = filter.preflight(dataStructure, MakeArgs(coordinationNumber, loop));
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  REQUIRE(preflightResult.outputActions.warnings().empty());
}

/**
 * @brief Asserts that preflight accepts the given parameter pair but attaches exactly one warning
 * carrying @p expectedCode whose message contains every entry of @p expectedMessageParts. Used for
 * the data-dependent non-termination combination, which must run - it terminates on a boundary-free
 * volume - while still telling the user what it may do on a volume that has a boundary.
 */
void CheckPreflightWarns(const DataStructure& dataStructure, int32 coordinationNumber, bool loop, int32 expectedCode, const std::vector<std::string>& expectedMessageParts)
{
  const ErodeDilateCoordinationNumberFilter filter;

  auto preflightResult = filter.preflight(dataStructure, MakeArgs(coordinationNumber, loop));
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  const auto& warnings = preflightResult.outputActions.warnings();
  REQUIRE(warnings.size() == 1);
  REQUIRE(warnings[0].code == expectedCode);
  for(const std::string& part : expectedMessageParts)
  {
    CAPTURE(warnings[0].message, part);
    REQUIRE(warnings[0].message.find(part) != std::string::npos);
  }
}

void CheckCellArray(const DataStructure& dataStructure, const DataPath& arrayPath, const CellValues& expectedValues)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(arrayPath));
  const auto& valuesRef = dataStructure.getDataRefAs<Int32Array>(arrayPath).getDataStoreRef();
  REQUIRE(valuesRef.getNumberOfTuples() == k_CellCount);
  for(usize i = 0; i < k_CellCount; i++)
  {
    CAPTURE(i);
    REQUIRE(valuesRef[i] == expectedValues[i]);
  }
}

void CheckFeatureIds(const DataStructure& dataStructure, const CellValues& expectedIds)
{
  CheckCellArray(dataStructure, FeatureIdsPath(), expectedIds);
}

/**
 * @brief The untouched companion values: value == k_CopiedValueOffset + index.
 */
CellValues UntouchedCopiedValues()
{
  CellValues values{};
  for(usize i = 0; i < k_CellCount; i++)
  {
    values[i] = k_CopiedValueOffset + static_cast<int32>(i);
  }
  return values;
}

/**
 * @brief The ignored array is never rewritten: value == k_IgnoredValueOffset + index.
 */
CellValues UntouchedIgnoredValues()
{
  CellValues values{};
  for(usize i = 0; i < k_CellCount; i++)
  {
    values[i] = k_IgnoredValueOffset + static_cast<int32>(i);
  }
  return values;
}
} // namespace CoordinationFixture
} // namespace

// ORACLE DERIVATION (Class 1 — analytical, hand-derived from
// Algorithms/ErodeDilateCoordinationNumber.cpp before any execution).
//
// The algorithm sweeps voxels in x-fastest / z-slowest order. For each voxel it
// counts how many of its valid face neighbors sit across the bad/good boundary,
// where the boundary test is `(featureName > 0 && feature == 0) ||
// (featureName == 0 && feature > 0)`. That count is the coordination number.
// A voxel with `coordination >= CoordinationNumber && coordination > 0` has every
// non-ignored cell array's tuple overwritten from the winning neighbor.
//
// Face-neighbor traversal order is fixed by initializeFaceNeighborInternalIdx():
// -Z, -Y, -X, +X, +Y, +Z. Because the winner is replaced only when a vote count
// is STRICTLY greater than the running maximum, the winner is the first feature
// to reach the maximum vote count in that order, and among equal-feature votes
// the LAST neighbor of that feature supplies the tuple. Both rules are fully
// deterministic — there is no hash ordering, no parallelism, and no dependence on
// container iteration order in the vote.
//
// For a 5x5x1 grid, computeValidFaceNeighbors() marks -Z and +Z invalid on every
// voxel (dims[2] == 1), so only the four in-plane neighbors ever vote.
//
// FIXTURE A — single bad voxel at (2, 2), index 12, feature 1 everywhere else:
//
//     1 1 1 1 1
//     1 1 1 1 1
//     1 1 0 1 1
//     1 1 1 1 1
//     1 1 1 1 1
//
//   Voxel 12 (featureName == 0) sees feature 1 at -Y(7), -X(11), +X(13), +Y(17)
//   => coordination 4. Votes for feature 1 run 1, 2, 3, 4, so the winner is the
//   last one visited: neighbor 17. Every good voxel touching index 12 (7, 11,
//   13, 17) has coordination 1; all remaining voxels have coordination 0.
//   * CoordinationNumber = 4: 4 >= 4, so index 12 copies tuple 17. FeatureIds
//     becomes all 1 and CopiedValues[12] becomes 117. Voxels 13 and 17 are
//     visited after index 12 and by then see no bad neighbor, so nothing else
//     changes.
//   * CoordinationNumber = 5: 4 >= 5 is false, so the grid is unchanged.
//
// FIXTURE B — two-feature seam, one bad voxel on the seam at index 12:
//
//     1 1 1 1 1
//     1 1 1 1 1
//     1 1 0 2 2
//     2 2 2 2 2
//     2 2 2 2 2
//
//   Voxel 12 sees feature 1 at -Y(7) and -X(11), feature 2 at +X(13) and +Y(17):
//   coordination 4 with a genuine 2-2 vote tie. Walking the fixed order:
//     -Y(7)  feature 1 -> count 1, 1 > 0  -> winner 7
//     -X(11) feature 1 -> count 2, 2 > 1  -> winner 11
//     +X(13) feature 2 -> count 1, 1 > 2? no
//     +Y(17) feature 2 -> count 2, 2 > 2? no
//   The tie therefore resolves to feature 1 via neighbor 11 — the first feature
//   to reach the maximum count wins, and CopiedValues[12] == 111 pins the exact
//   neighbor rather than merely the winning feature.
//
// Loop == true does NOT behave the same way at both thresholds, so the two cases
// are stated separately:
//   * CoordinationNumber = 4 (Threshold Met, Two Feature Seam, Majority Vote): the
//     trailing counter loop (:171-186 of the algorithm) re-reads the
//     coordinationNumber[] array that the sweep just filled in, and that array still
//     holds the PRE-copy value 4 for the reassigned voxel. So the counter is 1, not
//     0, after sweep 1 — which is precisely why a second sweep runs. Sweep 2 finds no
//     bad voxel, recomputes every coordination number as 0, the counter is 0, and the
//     loop exits. Output is unchanged by sweep 2.
//   * CoordinationNumber = 5 (Threshold Not Met): nothing is reassigned and the only
//     non-zero coordination number in the grid is the 4 at index 12, which is < 5. The
//     counter is 0 straight after sweep 1, so there is NO second sweep.
// Both Loop values are asserted in every CoordinationNumber = 4 test case.
//
// FIXTURE C — decisive majority vote, one feature-1 neighbor visited FIRST and three
// feature-2 neighbors after it:
//
//     1 1 1 1 1
//     1 1 1 1 1
//     2 2 0 2 2
//     2 2 2 2 2
//     2 2 2 2 2
//
//   Voxel 12 (featureName == 0) sees feature 1 at -Y(7) and feature 2 at -X(11),
//   +X(13), +Y(17) => coordination 4. Walking the fixed order with the
//   strictly-greater rule:
//     -Y(7)  feature 1 -> featureCount[1] = 1, 1 > 0  -> most = 1, winner 7
//     -X(11) feature 2 -> featureCount[2] = 1, 1 > 1? no
//     +X(13) feature 2 -> featureCount[2] = 2, 2 > 1  -> most = 2, winner 13
//     +Y(17) feature 2 -> featureCount[2] = 3, 3 > 2  -> most = 3, winner 17
//   Feature 2's three votes decisively overtake feature 1's single earlier vote, so
//   the majority genuinely changes hands mid-scan — this is the case the tie fixture
//   (Fixture B) cannot exercise. At CoordinationNumber = 4 voxel 12 copies tuple 17:
//   FeatureIds[12] becomes 2 and CopiedValues[12] becomes 117. Every other voxel has
//   coordination 0 or 1, both below the threshold, so nothing else changes. If the
//   leader were wrongly retained at the first vote the assertions would read
//   FeatureIds[12] == 1 and CopiedValues[12] == 107, so the test bites on both arrays.
//
// FIXTURE A at CoordinationNumber = 1 — the good-voxel vote branch.
//
//   Every test case above reassigns only BAD voxels, which exercises only the
//   `featureName == 0 && feature > 0` half of the boundary predicate. The other half
//   (`featureName > 0 && feature == 0`, a good voxel adjacent to bad) accumulates into
//   featureCount[0] and is only ever *acted on* when CoordinationNumber <= 3, because
//   a good voxel touching a single bad voxel has coordination 1. CoordinationNumber = 1
//   with Loop = false is therefore the case that drives it.
//
//   Loop = true is deliberately NOT swept here: on THIS fixture, at CoordinationNumber = 1,
//   the sweep creates a good/bad boundary as fast as it removes one, the trailing counter
//   does not reach 0, and the run did not terminate under a 45 s probe in BOTH SIMPLNX and
//   DREAM3D 6.5.171 — see the deviations document. Termination at this threshold is data
//   dependent, not universally impossible: a boundary-free volume converges in one no-op
//   sweep, which `Boundary-Free Volume Terminates` runs to completion.
//
//   IN-SWEEP WRITES ARE VISIBLE TO LATER VOXELS. `featureIds` (:47) is a reference to
//   the live Int32Array, and DataArrayCopyTupleFunctor copies the DataArray<T> by value
//   but shares the underlying m_DataStore, so copyTuple() mutates the very array the
//   vote reads. Voxel N therefore sees every reassignment made by voxels < N in the same
//   sweep. This is load-bearing: without it only indices 7, 11, 12, 13 and 17 could
//   change; with it the bad region propagates down and to the right across the grid.
//
//   Walking the sweep in x-fastest order (only voxels that change are listed; each
//   "copy from S" writes featureIds[i] = featureIds[S] and copiedValues[i] =
//   copiedValues[S] using the CURRENT contents of index S):
//     7  good, -Y(2)=1 -X(6)=1 +X(8)=1 +Y(12)=0 -> coord 1 -> copy from 12  -> ids[7]=0,  copied[7]=112
//     8  good, -X(7)=0 (just written)            -> coord 1 -> copy from 7   -> ids[8]=0,  copied[8]=112
//     9  good, -X(8)=0                           -> coord 1 -> copy from 8   -> ids[9]=0,  copied[9]=112
//     11 good, +X(12)=0                          -> coord 1 -> copy from 12  -> ids[11]=0, copied[11]=112
//     12 bad,  -Y(7)=0 -X(11)=0 (both rewritten) +X(13)=1 +Y(17)=1 -> coord 2,
//              featureCount[1] runs 1 then 2, so the LAST feature-1 neighbor wins
//                                                -> copy from 17  -> ids[12]=1, copied[12]=117
//     13 good, -Y(8)=0 (-X(12) is now 1)         -> coord 1 -> copy from 8   -> ids[13]=0, copied[13]=112
//     14 good, -Y(9)=0 then -X(13)=0             -> coord 2 -> copy from 13  -> ids[14]=0, copied[14]=112
//     16 good, -Y(11)=0                          -> coord 1 -> copy from 11  -> ids[16]=0, copied[16]=112
//     17 good, -X(16)=0 (-Y(12) is now 1)        -> coord 1 -> copy from 16  -> ids[17]=0, copied[17]=112
//     18 good, -Y(13)=0 then -X(17)=0            -> coord 2 -> copy from 17  -> ids[18]=0, copied[18]=112
//     19 good, -Y(14)=0 then -X(18)=0            -> coord 2 -> copy from 18  -> ids[19]=0, copied[19]=112
//     21 good, -Y(16)=0                          -> coord 1 -> copy from 16  -> ids[21]=0, copied[21]=112
//     22 good, -Y(17)=0 then -X(21)=0            -> coord 2 -> copy from 21  -> ids[22]=0, copied[22]=112
//     23 good, -Y(18)=0 then -X(22)=0            -> coord 2 -> copy from 22  -> ids[23]=0, copied[23]=112
//     24 good, -Y(19)=0 then -X(23)=0            -> coord 2 -> copy from 23  -> ids[24]=0, copied[24]=112
//   Voxels 0-6, 10, 15 and 20 keep coordination 0 throughout and are untouched.
//
//   Expected FeatureIds (15 of 25 change):
//
//     1 1 1 1 1
//     1 1 0 0 0
//     1 0 1 0 0
//     1 0 0 0 0
//     1 0 0 0 0
//
//   Expected CopiedValues: 112 at every changed index except index 12, which is 117.
//   Index 12 is the one voxel reassigned by the BAD-voxel branch, and its source (17)
//   had not yet been rewritten when it was read.
//
//   Note the featureCount[0] leak documented in the deviations file is live throughout
//   this case: it reaches 20 by the end of the sweep and is never reset. It cannot
//   change any of the choices above, because in the good-voxel branch every vote lands
//   on the single key 0, so each successive vote is strictly greater than the last and
//   the LAST qualifying neighbor wins either way.
//
// FIXTURE D — Loop discrimination. Two bad voxels in the bottom-right corner region,
// at index 19 (4, 3) and index 23 (3, 4), feature 1 everywhere else, CoordinationNumber = 2:
//
//     1 1 1 1 1
//     1 1 1 1 1
//     1 1 1 1 1
//     1 1 1 1 0
//     1 1 1 0 1
//
//   The point of this fixture is that Loop = false and Loop = true produce DIFFERENT
//   grids, which none of Fixtures A/B/C can show (in every one of those the second
//   sweep is a no-op). It additionally discriminates the trailing counter loop
//   (:171-186) and its `>=` predicate (:180), because after sweep 1 the LARGEST recorded
//   coordination number in the grid is exactly 2 — equal to CoordinationNumber. So a
//   counter written with `>` instead of `>=` would report 0 and stop after one sweep,
//   while `>=` reports 3 and runs sweep 2, which changes the output.
//
//   SWEEP 1 (voxels not listed keep coordination 0 and are untouched):
//     14 good, +Y(19)=0                      -> coord 1, below threshold, no copy
//     18 good, +X(19)=0 then +Y(23)=0        -> coord 2 -> both votes land on featureCount[0],
//              so the LAST one wins -> copy from 23 -> ids[18]=0, copied[18]=123
//     19 bad,  -Y(14)=1 (-X(18) is now 0, skipped) +Y(24)=1
//                                            -> coord 2, featureCount[1] runs 1 then 2,
//              so the last feature-1 neighbor wins -> copy from 24 -> ids[19]=1, copied[19]=124
//     22 good, +X(23)=0                      -> coord 1, below threshold, no copy
//     23 bad,  (-Y(18) is now 0, skipped) -X(22)=1 then +X(24)=1
//                                            -> coord 2 -> copy from 24 -> ids[23]=1, copied[23]=124
//     24 good, -Y(19)=1 and -X(23)=1 (both already rewritten) -> coord 0, untouched
//
//   Recorded coordinationNumber[] after sweep 1: 1 at 14, 2 at 18, 2 at 19, 1 at 22,
//   2 at 23, 0 everywhere else. Maximum is 2 == CoordinationNumber, so counter is 3
//   under `>=` and would be 0 under `>`.
//
//   Loop = false expected FeatureIds (only index 18 differs from all-ones):
//
//     1 1 1 1 1
//     1 1 1 1 1
//     1 1 1 1 1
//     1 1 1 0 1
//     1 1 1 1 1
//
//   Loop = false expected CopiedValues: 123 at 18, 124 at 19, 124 at 23, untouched elsewhere.
//
//   SWEEP 2 (Loop = true only). The grid now holds one bad voxel at 18:
//     13 good, +Y(18)=0                      -> coord 1, below threshold
//     17 good, +X(18)=0                      -> coord 1, below threshold
//     18 bad,  -Y(13)=1 -X(17)=1 +X(19)=1 +Y(23)=1 -> coord 4, featureCount[1] runs 1..4,
//              so the last feature-1 neighbor wins -> copy from 23 -> ids[18]=1, copied[18]=124
//              (copied[23] was rewritten to 124 in sweep 1, which is what makes the
//               Loop = true CopiedValues[18] differ from the Loop = false 123)
//   Recorded coordinationNumber[] after sweep 2: 1 at 13, 1 at 17, 4 at 18, 0 elsewhere.
//   counter is 1, so a third sweep runs; the grid is all-ones by then, every coordination
//   number is 0, counter is 0 and the loop EXITS. Three sweeps, terminating.
//
//   Loop = true expected FeatureIds: all 1.
//   Loop = true expected CopiedValues: 124 at 18, 124 at 19, 124 at 23, untouched elsewhere.
//
//   The two runs therefore differ at index 18 in BOTH arrays (0 vs 1, and 123 vs 124).
//
//   Design note: the geometry above is NOT the plus-shaped sketch (bad at 7, 11, 13, 17
//   with CoordinationNumber = 2) that was proposed for this fixture. That sketch does
//   discriminate Loop (it ends with bad voxels at 6 and 12 under Loop = false and an
//   all-ones grid under Loop = true), but it does NOT discriminate the counter's `>=`:
//   its sweep-1 coordination numbers reach 3, so a `>` counter still reports 4, still
//   runs the same three sweeps, and still produces the same output. Fixture D was
//   redesigned to put the sweep-1 maximum exactly ON the threshold, which is the only
//   way the `>=` boundary is observable in the output.
//
// FIXTURE E — featureCount reset discrimination. Two bad voxels far enough apart that
// neither can see the other, one surrounded by feature 1 and one carrying a near-tie
// between feature 2 and feature 1, with CoordinationNumber = 3, Loop = false:
//
//     1 1 0 1 1
//     1 1 1 1 1
//     1 1 1 1 1
//     2 2 2 2 2
//     2 2 0 1 2
//
//   The reset loop (:154-167) zeroes featureCount[f] for every valid neighbor with
//   f > 0 after each voxel is processed. Deleting it lets counts accumulate across
//   voxels. This fixture makes that leak change the ANSWER rather than just the
//   bookkeeping.
//
//   Voxel 2 (2, 0) is bad, on the top edge, so only -X(1), +X(3), +Y(7) are valid and all
//   three are feature 1:
//     -X(1)  f1 -> featureCount[1] = 1, 1 > 0 -> winner 1
//     +X(3)  f1 -> featureCount[1] = 2, 2 > 1 -> winner 3
//     +Y(7)  f1 -> featureCount[1] = 3, 3 > 2 -> winner 7
//   coordination 3 >= 3, so it copies tuple 7: ids[2] = 1, copied[2] = 107. The reset loop
//   then clears featureCount[1] back to 0.
//
//   Voxel 22 (2, 4) is bad, on the bottom edge, so only -Y(17), -X(21), +X(23) are valid.
//   17 and 21 are feature 2; 23 is feature 1:
//     -Y(17) f2 -> featureCount[2] = 1, 1 > 0 -> most = 1, winner 17
//     -X(21) f2 -> featureCount[2] = 2, 2 > 1 -> most = 2, winner 21
//     +X(23) f1 -> featureCount[1] = 1, 1 > 2? NO -> winner stays 21
//   coordination 3 >= 3, so it copies tuple 21: ids[22] = 2, copied[22] = 121.
//
//   WITHOUT the reset, featureCount[1] would still hold the 3 left over from voxel 2 (no
//   voxel in between votes for a feature > 0 — the only other voters are good voxels, which
//   accumulate on key 0). The final vote at voxel 22 would then read:
//     +X(23) f1 -> featureCount[1] = 4, 4 > 2? YES -> winner 23
//   giving ids[22] = 1 and copied[22] = 123. Both assertions bite: the winning FEATURE
//   changes (2 -> 1) and the source NEIGHBOR changes (21 -> 23).
//
//   No other voxel qualifies at CoordinationNumber = 3: every good voxel adjacent to a bad
//   one has coordination 1 (voxels 1, 17, 21), and after each bad voxel is filled its
//   neighbors see no boundary at all.
//
//   Expected FeatureIds (Loop = false):
//
//     1 1 1 1 1
//     1 1 1 1 1
//     1 1 1 1 1
//     2 2 2 2 2
//     2 2 2 1 2
//
//   Expected CopiedValues: 107 at index 2, 121 at index 22, untouched elsewhere.
//
// FIXTURE A at CoordinationNumber = 0 — the `&& coordinationNumber[voxelIndex] > 0` guard.
//
//   The transfer condition (:140) is
//     coordinationNumber[voxelIndex] >= CoordinationNumber && coordinationNumber[voxelIndex] > 0
//   With CoordinationNumber = 0 the first clause is universally true, so the SECOND clause
//   is the only thing standing between the filter and `copyTuple(neighbors[i], i)` on every
//   coordination-0 voxel — where neighbors[i] is still the -1 that :50 initialised it to,
//   which converts to SIZE_MAX as a source tuple index. That guard is exercised by no
//   other test case, because every other case uses CoordinationNumber >= 1, where the
//   first clause already implies the second.
//
//   Note that CoordinationNumber = 0 is NOT a no-op: coordination is a property of the
//   GRID, not of the parameter, so voxels 7, 11, 13, 17 still have coordination 1 and
//   voxel 12 still has coordination 4. `coord >= 0 && coord > 0` is exactly `coord > 0`,
//   which is exactly `coord >= 1`, so CoordinationNumber = 0 and CoordinationNumber = 1
//   select the identical voxel set and produce the identical output — the 15-of-25 change
//   footprint derived for the Good Voxel Branch case above. That equality is itself the
//   assertion: it is only true because the `> 0` clause is present.
//
//   Loop = false is pinned. With Loop = true and CoordinationNumber = 0 the trailing
//   counter loop counts every voxel unconditionally (`coord >= 0` is always true), so the
//   run never terminates in either implementation — see the deviations document.

TEST_CASE("SimplnxCore::ErodeDilateCoordinationNumberFilter: Class 1 Oracle - Threshold Met", "[SimplnxCore][ErodeDilateCoordinationNumberFilter]")
{
  UnitTest::LoadPlugins();

  const bool loop = GENERATE(false, true);
  DYNAMIC_SECTION("Loop=" << loop)
  {
    using namespace CoordinationFixture;

    CellValues inputFeatureIds{};
    inputFeatureIds.fill(1);
    inputFeatureIds[GetIndex(2, 2)] = 0;

    DataStructure dataStructure = BuildDataStructure(inputFeatureIds);

    // Coordination number of the single bad voxel is 4 and the threshold is 4,
    // so the bad voxel is reassigned from its +Y neighbor (index 17).
    RunFilter(dataStructure, 4, loop);

    CellValues expectedFeatureIds{};
    expectedFeatureIds.fill(1);

    CellValues expectedCopiedValues = UntouchedCopiedValues();
    expectedCopiedValues[GetIndex(2, 2)] = k_CopiedValueOffset + static_cast<int32>(GetIndex(2, 3));

    CheckFeatureIds(dataStructure, expectedFeatureIds);
    CheckCellArray(dataStructure, CopiedValuesPath(), expectedCopiedValues);
    CheckCellArray(dataStructure, IgnoredValuesPath(), UntouchedIgnoredValues());

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ErodeDilateCoordinationNumberFilter: Class 1 Oracle - Threshold Not Met", "[SimplnxCore][ErodeDilateCoordinationNumberFilter]")
{
  UnitTest::LoadPlugins();

  const bool loop = GENERATE(false, true);
  DYNAMIC_SECTION("Loop=" << loop)
  {
    using namespace CoordinationFixture;

    CellValues inputFeatureIds{};
    inputFeatureIds.fill(1);
    inputFeatureIds[GetIndex(2, 2)] = 0;

    DataStructure dataStructure = BuildDataStructure(inputFeatureIds);

    // Coordination number of the single bad voxel is 4, one short of the
    // threshold of 5, so no tuple is copied anywhere in the grid.
    RunFilter(dataStructure, 5, loop);

    CheckFeatureIds(dataStructure, inputFeatureIds);
    CheckCellArray(dataStructure, CopiedValuesPath(), UntouchedCopiedValues());
    CheckCellArray(dataStructure, IgnoredValuesPath(), UntouchedIgnoredValues());

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ErodeDilateCoordinationNumberFilter: Class 1 Oracle - Two Feature Seam", "[SimplnxCore][ErodeDilateCoordinationNumberFilter]")
{
  UnitTest::LoadPlugins();

  const bool loop = GENERATE(false, true);
  DYNAMIC_SECTION("Loop=" << loop)
  {
    using namespace CoordinationFixture;

    CellValues inputFeatureIds{};
    for(usize y = 0; y < k_YDim; y++)
    {
      for(usize x = 0; x < k_XDim; x++)
      {
        inputFeatureIds[GetIndex(x, y)] = (y < 2 || (y == 2 && x < 2)) ? 1 : 2;
      }
    }
    inputFeatureIds[GetIndex(2, 2)] = 0;

    DataStructure dataStructure = BuildDataStructure(inputFeatureIds);

    // The bad voxel has two feature-1 votes (-Y, -X) and two feature-2 votes
    // (+X, +Y). The strictly-greater comparison keeps the first feature to reach
    // the maximum count, so feature 1 wins by way of its -X neighbor (index 11).
    RunFilter(dataStructure, 4, loop);

    CellValues expectedFeatureIds = inputFeatureIds;
    expectedFeatureIds[GetIndex(2, 2)] = 1;

    CellValues expectedCopiedValues = UntouchedCopiedValues();
    expectedCopiedValues[GetIndex(2, 2)] = k_CopiedValueOffset + static_cast<int32>(GetIndex(1, 2));

    CheckFeatureIds(dataStructure, expectedFeatureIds);
    CheckCellArray(dataStructure, CopiedValuesPath(), expectedCopiedValues);
    CheckCellArray(dataStructure, IgnoredValuesPath(), UntouchedIgnoredValues());

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ErodeDilateCoordinationNumberFilter: Class 1 Oracle - Majority Vote Overtakes", "[SimplnxCore][ErodeDilateCoordinationNumberFilter]")
{
  UnitTest::LoadPlugins();

  const bool loop = GENERATE(false, true);
  DYNAMIC_SECTION("Loop=" << loop)
  {
    using namespace CoordinationFixture;

    // Fixture C: rows 0 and 1 are feature 1, rows 2 through 4 are feature 2, with the
    // bad voxel on the boundary at (2, 2). That puts the single feature-1 neighbor at
    // -Y (index 7, visited first) and all three feature-2 neighbors after it.
    CellValues inputFeatureIds{};
    for(usize y = 0; y < k_YDim; y++)
    {
      for(usize x = 0; x < k_XDim; x++)
      {
        inputFeatureIds[GetIndex(x, y)] = (y < 2) ? 1 : 2;
      }
    }
    inputFeatureIds[GetIndex(2, 2)] = 0;

    DataStructure dataStructure = BuildDataStructure(inputFeatureIds);

    // Feature 1 leads after the first vote, then feature 2 reaches 2 and 3 votes and
    // takes the lead twice. The last vote wins, so the tuple comes from +Y (index 17).
    RunFilter(dataStructure, 4, loop);

    CellValues expectedFeatureIds = inputFeatureIds;
    expectedFeatureIds[GetIndex(2, 2)] = 2;

    CellValues expectedCopiedValues = UntouchedCopiedValues();
    expectedCopiedValues[GetIndex(2, 2)] = k_CopiedValueOffset + static_cast<int32>(GetIndex(2, 3));

    CheckFeatureIds(dataStructure, expectedFeatureIds);
    CheckCellArray(dataStructure, CopiedValuesPath(), expectedCopiedValues);
    CheckCellArray(dataStructure, IgnoredValuesPath(), UntouchedIgnoredValues());

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ErodeDilateCoordinationNumberFilter: Class 1 Oracle - Good Voxel Branch", "[SimplnxCore][ErodeDilateCoordinationNumberFilter]")
{
  UnitTest::LoadPlugins();

  using namespace CoordinationFixture;

  // Fixture A again, but at the threshold that makes GOOD voxels qualify. Loop is
  // pinned to false: on this boundary-bearing fixture, CoordinationNumber = 1 with
  // Loop = true did not terminate under a 45 s probe in either SIMPLNX or DREAM3D
  // 6.5.171. Termination at this threshold is data dependent - see the block comment.
  CellValues inputFeatureIds{};
  inputFeatureIds.fill(1);
  inputFeatureIds[GetIndex(2, 2)] = 0;

  DataStructure dataStructure = BuildDataStructure(inputFeatureIds);

  // Every good voxel touching a bad one has coordination 1 and therefore qualifies,
  // taking the `featureName > 0 && feature == 0` half of the boundary predicate and
  // the featureCount[0] accumulation. Because copyTuple writes into the same array the
  // vote reads, the bad region propagates through the sweep. Full step-by-step
  // derivation is in the block comment above.
  RunFilter(dataStructure, 1, false);

  CellValues expectedFeatureIds{};
  expectedFeatureIds.fill(1);
  CellValues expectedCopiedValues = UntouchedCopiedValues();
  for(const usize index : k_GoodVoxelBranchChangedIndices)
  {
    expectedFeatureIds[index] = 0;
    expectedCopiedValues[index] = k_CopiedValueOffset + static_cast<int32>(GetIndex(2, 2));
  }
  // Index 12 is reassigned by the bad-voxel branch instead, from neighbor 17.
  expectedFeatureIds[GetIndex(2, 2)] = 1;
  expectedCopiedValues[GetIndex(2, 2)] = k_CopiedValueOffset + static_cast<int32>(GetIndex(2, 3));

  CheckFeatureIds(dataStructure, expectedFeatureIds);
  CheckCellArray(dataStructure, CopiedValuesPath(), expectedCopiedValues);
  CheckCellArray(dataStructure, IgnoredValuesPath(), UntouchedIgnoredValues());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ErodeDilateCoordinationNumberFilter: Class 1 Oracle - Loop Discriminates", "[SimplnxCore][ErodeDilateCoordinationNumberFilter]")
{
  UnitTest::LoadPlugins();

  const bool loop = GENERATE(false, true);
  DYNAMIC_SECTION("Loop=" << loop)
  {
    using namespace CoordinationFixture;

    // Fixture D: feature 1 everywhere except two bad voxels at (4, 3) and (3, 4). This is
    // the only fixture whose Loop=false and Loop=true results DIFFER, and it is the only
    // one whose sweep-1 maximum coordination number lands exactly ON the threshold, which
    // is what makes the trailing counter's `>=` predicate observable in the output.
    CellValues inputFeatureIds{};
    inputFeatureIds.fill(1);
    inputFeatureIds[GetIndex(4, 3)] = 0;
    inputFeatureIds[GetIndex(3, 4)] = 0;

    DataStructure dataStructure = BuildDataStructure(inputFeatureIds);

    RunFilter(dataStructure, 2, loop);

    // Sweep 1 is common to both runs: voxel 18 turns bad from its +Y neighbor 23, and
    // voxels 19 and 23 are both filled from the corner voxel 24.
    CellValues expectedFeatureIds{};
    expectedFeatureIds.fill(1);

    CellValues expectedCopiedValues = UntouchedCopiedValues();
    expectedCopiedValues[GetIndex(4, 3)] = k_CopiedValueOffset + static_cast<int32>(GetIndex(4, 4));
    expectedCopiedValues[GetIndex(3, 4)] = k_CopiedValueOffset + static_cast<int32>(GetIndex(4, 4));

    if(loop)
    {
      // Sweep 2 fills the lone remaining bad voxel 18 from neighbor 23, whose CopiedValues
      // entry sweep 1 had already rewritten to 124. Sweep 3 finds nothing and exits.
      expectedCopiedValues[GetIndex(3, 3)] = k_CopiedValueOffset + static_cast<int32>(GetIndex(4, 4));
    }
    else
    {
      // A single sweep leaves voxel 18 bad, holding the tuple it took from voxel 23.
      expectedFeatureIds[GetIndex(3, 3)] = 0;
      expectedCopiedValues[GetIndex(3, 3)] = k_CopiedValueOffset + static_cast<int32>(GetIndex(3, 4));
    }

    CheckFeatureIds(dataStructure, expectedFeatureIds);
    CheckCellArray(dataStructure, CopiedValuesPath(), expectedCopiedValues);
    CheckCellArray(dataStructure, IgnoredValuesPath(), UntouchedIgnoredValues());

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ErodeDilateCoordinationNumberFilter: Class 1 Oracle - Accumulator Reset", "[SimplnxCore][ErodeDilateCoordinationNumberFilter]")
{
  using namespace CoordinationFixture;

  UnitTest::LoadPlugins();

  // Fixture E: rows 0-2 feature 1, rows 3-4 feature 2, one bad voxel at (2, 0) whose three
  // valid neighbors are all feature 1, and a second bad voxel at (2, 4) whose vote is a
  // near-tie (two feature-2 votes then one feature-1 vote). Index 23 is forced to feature 1
  // to create that near-tie. If the per-voxel featureCount reset were removed, the three
  // feature-1 votes banked at voxel 2 would carry into voxel 22 and flip its winner.
  CellValues inputFeatureIds{};
  for(usize y = 0; y < k_YDim; y++)
  {
    for(usize x = 0; x < k_XDim; x++)
    {
      inputFeatureIds[GetIndex(x, y)] = (y < 3) ? 1 : 2;
    }
  }
  inputFeatureIds[GetIndex(2, 0)] = 0;
  inputFeatureIds[GetIndex(2, 4)] = 0;
  inputFeatureIds[GetIndex(3, 4)] = 1;

  DataStructure dataStructure = BuildDataStructure(inputFeatureIds);

  RunFilter(dataStructure, 3, false);

  CellValues expectedFeatureIds = inputFeatureIds;
  expectedFeatureIds[GetIndex(2, 0)] = 1;
  // Feature 2 wins voxel 22 with two votes against feature 1's one. With a leaked
  // featureCount[1] of 3 this would read 1, sourced from index 23 instead of 21.
  expectedFeatureIds[GetIndex(2, 4)] = 2;

  CellValues expectedCopiedValues = UntouchedCopiedValues();
  expectedCopiedValues[GetIndex(2, 0)] = k_CopiedValueOffset + static_cast<int32>(GetIndex(2, 1));
  expectedCopiedValues[GetIndex(2, 4)] = k_CopiedValueOffset + static_cast<int32>(GetIndex(1, 4));

  CheckFeatureIds(dataStructure, expectedFeatureIds);
  CheckCellArray(dataStructure, CopiedValuesPath(), expectedCopiedValues);
  CheckCellArray(dataStructure, IgnoredValuesPath(), UntouchedIgnoredValues());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ErodeDilateCoordinationNumberFilter: Class 1 Oracle - Zero Coordination Number Guard", "[SimplnxCore][ErodeDilateCoordinationNumberFilter]")
{
  using namespace CoordinationFixture;

  UnitTest::LoadPlugins();

  // Fixture A at CoordinationNumber = 0, the only setting under which the transfer
  // condition's `&& coordinationNumber[voxelIndex] > 0` clause carries the whole decision.
  // Without it every coordination-0 voxel would call copyTuple() with the -1 that
  // neighbors[] was initialised to. Loop is pinned to false: CoordinationNumber = 0 with
  // Loop = true does not terminate in either SIMPLNX or DREAM3D 6.5.171.
  CellValues inputFeatureIds{};
  inputFeatureIds.fill(1);
  inputFeatureIds[GetIndex(2, 2)] = 0;

  DataStructure dataStructure = BuildDataStructure(inputFeatureIds);

  RunFilter(dataStructure, 0, false);

  // `coord >= 0 && coord > 0` is exactly `coord >= 1`, so the qualifying set — and
  // therefore the output — is identical to the CoordinationNumber = 1 case, not a no-op.
  CellValues expectedFeatureIds{};
  expectedFeatureIds.fill(1);
  CellValues expectedCopiedValues = UntouchedCopiedValues();
  for(const usize index : k_GoodVoxelBranchChangedIndices)
  {
    expectedFeatureIds[index] = 0;
    expectedCopiedValues[index] = k_CopiedValueOffset + static_cast<int32>(GetIndex(2, 2));
  }
  expectedFeatureIds[GetIndex(2, 2)] = 1;
  expectedCopiedValues[GetIndex(2, 2)] = k_CopiedValueOffset + static_cast<int32>(GetIndex(2, 3));

  CheckFeatureIds(dataStructure, expectedFeatureIds);
  CheckCellArray(dataStructure, CopiedValuesPath(), expectedCopiedValues);
  CheckCellArray(dataStructure, IgnoredValuesPath(), UntouchedIgnoredValues());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ErodeDilateCoordinationNumberFilter: Invalid Parameters", "[SimplnxCore][ErodeDilateCoordinationNumberFilter]")
{
  using namespace CoordinationFixture;

  UnitTest::LoadPlugins();

  // Fixture A. The grid is irrelevant to both guards - they read only the two scalar
  // parameters - but a real DataStructure is needed so that the selection parameters
  // resolve and preflight reaches the guards rather than failing on a missing path.
  CellValues inputFeatureIds{};
  inputFeatureIds.fill(1);
  inputFeatureIds[GetIndex(2, 2)] = 0;

  const DataStructure dataStructure = BuildDataStructure(inputFeatureIds);

  // Codes are duplicated from the anonymous namespace of ErodeDilateCoordinationNumberFilter.cpp;
  // they are part of the filter's user-visible contract, so pinning them here is deliberate.
  constexpr int32 k_InvalidCoordinationNumberError = -16800;
  constexpr int32 k_NonTerminatingLoopError = -16801;
  constexpr int32 k_OscillatingLoopWarning = -16802;

  SECTION("Guard A - CoordinationNumber outside [0,6] is rejected (DREAM3D 6.5.171 parity)")
  {
    // Legacy ErodeDilateCoordinationNumber.cpp:118-123 rejects the same interval with -5555.
    const int32 coordinationNumber = GENERATE(-1, 7);
    DYNAMIC_SECTION("CoordinationNumber=" << coordinationNumber)
    {
      CheckPreflightRejects(dataStructure, coordinationNumber, false, k_InvalidCoordinationNumberError, {fmt::format("({})", coordinationNumber), "[0,6]"});
      // The interval guard is independent of Loop.
      CheckPreflightRejects(dataStructure, coordinationNumber, true, k_InvalidCoordinationNumberError, {fmt::format("({})", coordinationNumber), "[0,6]"});
    }
  }

  SECTION("Guard B - CoordinationNumber 0 with Loop enabled is rejected (beyond legacy parity)")
  {
    // ErodeDilateCoordinationNumberFilter-D1. The sweep's trailing counter tests
    // `coordinationNumber[voxelIndex] >= CoordinationNumber`; at a threshold of 0 that predicate is
    // satisfied by *every* voxel of *every* volume, so `counter` can never reach 0 and the sweep can
    // never converge - non-termination here is universal, independent of the data. 6.5.171 accepts
    // the value and hangs; SIMPLNX rejects it at preflight instead.
    CheckPreflightRejects(dataStructure, 0, true, k_NonTerminatingLoopError, {"(0)", "Loop"});
  }

  SECTION("Guard C - CoordinationNumber 1 with Loop enabled warns but is accepted")
  {
    // At a threshold of 1 the trailing predicate is satisfied only by voxels that actually sit on a
    // good/bad boundary, so non-termination is *data dependent*: a volume containing a boundary may
    // oscillate indefinitely (bad cells convert to good and good cells to bad on alternating
    // sweeps), while a boundary-free volume - all one feature, or all bad, a realistic post-cleanup
    // input - converges after a single no-op sweep in both implementations. Rejecting it would
    // break a legacy pipeline that ran to completion, so the filter warns instead. The expected
    // substrings pin the value, the parameter name, and the data-dependent framing, so a message
    // that regressed to an absolute non-termination claim fails here.
    CheckPreflightWarns(dataStructure, 1, true, k_OscillatingLoopWarning, {"(1)", "Loop", "data-dependent", "may oscillate indefinitely"});
  }

  SECTION("Neither guard fires with Loop disabled")
  {
    // Both low thresholds are unconditionally legal - and still asserted by the `Good Voxel Branch`
    // and `Zero Coordination Number Guard` oracles - when Loop is off. Silence is asserted too:
    // CheckPreflightAccepts requires the warning list to be empty.
    const int32 coordinationNumber = GENERATE(0, 1);
    DYNAMIC_SECTION("CoordinationNumber=" << coordinationNumber)
    {
      CheckPreflightAccepts(dataStructure, coordinationNumber, false);
    }
  }

  SECTION("Positive control - the lowest terminating threshold with Loop enabled is accepted")
  {
    // CoordinationNumber = 2 with Loop = true is the boundary neither guard may overreach past; it
    // is the setting the `Loop Discriminates` oracle runs to completion. Accepted *and silent*.
    CheckPreflightAccepts(dataStructure, 2, true);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// ORACLE DERIVATION - Boundary-Free Volume Terminates.
//
// Guard C (Invalid Parameters, above) pins that CoordinationNumber = 1, Loop = true is
// accepted with warning -16802 rather than rejected. The warning's own text claims that a
// boundary-free volume "completes after a single no-op sweep" while a volume containing a
// boundary "may oscillate indefinitely". The boundary-free half of that claim has so far
// rested on reading the algorithm; this case runs it to completion in NX to confirm it
// empirically.
//
// Fixture: all 25 voxels are feature 1, so there is no featureName == 0 anywhere and the
// boundary test `(featureName > 0 && feature == 0) || (featureName == 0 && feature > 0)`
// (:126) is false for every voxel/neighbor pair in the grid. Therefore coordination == 0 for
// every voxel of sweep 1 (:138), the transfer condition `coordination >= 1 && coordination > 0`
// (:140) never holds, and copyTuple() is never called - FeatureIds, CopiedValues and
// IgnoredValues all leave the sweep identical to the input.
//
// Termination: the trailing counter loop's predicate `coordinationNumber[voxelIndex] >=
// m_InputValues->CoordinationNumber` (:180) is evaluated against the coordinationNumber[]
// array sweep 1 just filled, which is all zeros here, so it is false for every voxel and
// counter is 0 when sweep 1 finishes (:171-186). The outer `while(counter > 0 && keepGoing)`
// (:85) started with counter seeded to 1 purely to force one mandatory pass through the body;
// with counter now 0, the while condition fails on the very next check and the loop exits
// having executed exactly that one no-op sweep - Loop = true adds no second sweep on this
// input, because there is nothing left for a second sweep to find.
TEST_CASE("SimplnxCore::ErodeDilateCoordinationNumberFilter: Boundary-Free Volume Terminates", "[SimplnxCore][ErodeDilateCoordinationNumberFilter]")
{
  using namespace CoordinationFixture;

  UnitTest::LoadPlugins();

  // No zero-valued (bad) FeatureIds anywhere - a boundary-free volume, the realistic
  // post-cleanup input the -16802 warning message describes as terminating.
  CellValues inputFeatureIds{};
  inputFeatureIds.fill(1);

  DataStructure dataStructure = BuildDataStructure(inputFeatureIds);

  constexpr int32 k_OscillatingLoopWarning = -16802;

  const ErodeDilateCoordinationNumberFilter filter;
  const Arguments args = MakeArgs(1, true);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  const auto& warnings = preflightResult.outputActions.warnings();
  REQUIRE(warnings.size() == 1);
  REQUIRE(warnings[0].code == k_OscillatingLoopWarning);
  CAPTURE(warnings[0].message);
  REQUIRE(warnings[0].message.find("(1)") != std::string::npos);
  REQUIRE(warnings[0].message.find("Loop") != std::string::npos);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Boundary-free input, so the output is the UNCHANGED input on all three arrays.
  CheckFeatureIds(dataStructure, inputFeatureIds);
  CheckCellArray(dataStructure, CopiedValuesPath(), UntouchedCopiedValues());
  CheckCellArray(dataStructure, IgnoredValuesPath(), UntouchedIgnoredValues());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ErodeDilateCoordinationNumberFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ErodeDilateCoordinationNumberFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ErodeDilateCoordinationNumberFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ErodeDilateCoordinationNumberFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ErodeDilateCoordinationNumberFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<int32>(ErodeDilateCoordinationNumberFilter::k_CoordinationNumber_Key) == 5);
      CHECK(args.value<bool>(ErodeDilateCoordinationNumberFilter::k_Loop_Key) == true);
      CHECK(args.value<DataPath>(ErodeDilateCoordinationNumberFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ErodeDilateCoordinationNumberFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      // Complex type (MultiDataArraySelectionFilterParameterConverter) - verified by successful pipeline loading
    }
  }
}
