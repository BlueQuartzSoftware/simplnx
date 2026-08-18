#include "SimplnxCore/Filters/RequireMinimumSizeFeaturesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <filesystem>
#include <ios>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const DataPath k_ImageGeomPath({"ImageGeometry"});
const DataPath k_FeatureIdsPath({"ImageGeometry", "CellData", "FeatureIds"});
const DataPath k_CopiedScalarPath({"ImageGeometry", "CellData", "CopiedScalar"});
const DataPath k_CopiedVectorPath({"ImageGeometry", "CellData", "CopiedVector"});
const DataPath k_FeatureDataPath({"ImageGeometry", "FeatureData"});
const DataPath k_NumCellsPath({"ImageGeometry", "FeatureData", "NumElements"});
const DataPath k_PhasesPath({"ImageGeometry", "FeatureData", "Phases"});

// -----------------------------------------------------------------------------
// Class 4 (invariant) oracle. These four properties must hold for every successful
// run of the filter regardless of the fixture, so they are asserted from one helper.
// -----------------------------------------------------------------------------
void CheckClass4Invariants(const Int32Array& featureIds, const AttributeMatrix& featureAmRef, usize expectedCellCount)
{
  // Invariant 1: the total cell count is unchanged. The filter only relabels cells;
  // it never adds or drops a tuple from the cell-level arrays.
  REQUIRE(featureIds.getNumberOfTuples() == expectedCellCount);

  std::set<int32> observedIds;
  for(usize index = 0; index < featureIds.getNumberOfTuples(); index++)
  {
    observedIds.insert(featureIds[index]);
  }
  REQUIRE_FALSE(observedIds.empty());

  // Invariant 2: no cell retains a removed feature's id. RemoveInactiveObjects maps
  // every removed feature to 0 and an unassigned cell carries -1, so a minimum
  // observed id of 1 proves that every cell was successfully reassigned and remapped.
  CHECK(*observedIds.begin() == 1);

  // Invariant 3: the surviving ids are contiguous starting at 1 with no gaps.
  const int32 maxFeatureId = *observedIds.rbegin();
  CHECK(static_cast<int32>(observedIds.size()) == maxFeatureId);

  // Invariant 4: every feature-level array has max(FeatureId) + 1 tuples (the +1 is
  // the reserved feature-0 tuple, which the filter never removes).
  usize checkedCount = 0;
  for(const auto& [identifier, dataObject] : featureAmRef)
  {
    const auto* dataArrayPtr = dynamic_cast<const IDataArray*>(dataObject.get());
    if(dataArrayPtr != nullptr)
    {
      INFO("feature array: " + dataArrayPtr->getName());
      CHECK(dataArrayPtr->getNumberOfTuples() == static_cast<usize>(maxFeatureId) + 1);
      checkedCount++;
    }
  }
  // Guard against a vacuous pass: the loop above only inspects children that are
  // IDataArrays, so at least one must have been checked for Invariant 4 to mean anything.
  CHECK(checkedCount >= 1);
}

// -----------------------------------------------------------------------------
// The two-phase 6x6x6 discriminating fixture.
//
// Threshold: MinAllowedFeaturesSize = 4.
//
// Feature roles (NumElements / Phase):
//   1  200 / 1  large survivor, fills everything not claimed below
//   2    4 / 1  EXACTLY at the threshold - must survive
//   3    3 / 1  one cell below the threshold - must be removed
//   4    3 / 1  removed, and every one of its cells sits on the volume boundary
//   5    1 / 2  removed in all-phase mode, RETAINED when ApplySinglePhase=true,
//               PhaseNumber=1 because its phase is 2
//   6    5 / 1  a second survivor, comfortably above the threshold
//
// Cell layout (index = z*36 + y*6 + x):
//   feature 2: (2,3,5)=200 (2,4,5)=206 (3,4,5)=207 (3,5,5)=213
//   feature 3: (1,0,0)=1   (0,1,0)=6   (0,0,1)=36
//   feature 4: (0,5,5)=210 (2,5,5)=212 (5,5,5)=215
//   feature 5: (0,0,0)=0
//   feature 6: (5,4,4)=173 (5,5,4)=179 (4,4,5)=208 (5,4,5)=209 (4,5,5)=214
//   feature 1: the remaining 200 cells
// -----------------------------------------------------------------------------
namespace DiscriminatingFixture
{
constexpr usize k_Dimension = 6;
constexpr usize k_CellCount = k_Dimension * k_Dimension * k_Dimension;
constexpr usize k_FeatureCount = 7;
constexpr int64 k_MinAllowedFeaturesSize = 4;
constexpr int32 k_TargetPhase = 1;
constexpr std::array<float32, 3> k_VectorOffsets = {0.25F, 0.5F, 0.75F};

struct Coordinate
{
  usize x = 0;
  usize y = 0;
  usize z = 0;

  constexpr bool isAt(usize pointX, usize pointY, usize pointZ) const
  {
    return x == pointX && y == pointY && z == pointZ;
  }
};

constexpr usize GetIndex(usize x, usize y, usize z)
{
  return z * k_Dimension * k_Dimension + y * k_Dimension + x;
}

constexpr usize GetIndex(const Coordinate& coordinate)
{
  return GetIndex(coordinate.x, coordinate.y, coordinate.z);
}

// Feature 2 - exactly at the threshold, so it must survive.
constexpr std::array<Coordinate, 4> k_Feature2Cells = {Coordinate{2, 3, 5}, Coordinate{2, 4, 5}, Coordinate{3, 4, 5}, Coordinate{3, 5, 5}};
// Feature 3 - one cell below the threshold. Its three cells wrap the origin corner.
constexpr std::array<Coordinate, 3> k_Feature3Cells = {Coordinate{1, 0, 0}, Coordinate{0, 1, 0}, Coordinate{0, 0, 1}};
// Feature 4 - below the threshold and entirely on the +Y/+Z boundary faces.
constexpr std::array<Coordinate, 3> k_Feature4Cells = {Coordinate{0, 5, 5}, Coordinate{2, 5, 5}, Coordinate{5, 5, 5}};
// Feature 5 - the phase-2 offender, a single cell at the origin corner.
constexpr Coordinate k_Feature5Cell = {0, 0, 0};
// Feature 6 - a second survivor above the threshold, hugging the (5,5,5) corner.
constexpr std::array<Coordinate, 5> k_Feature6Cells = {Coordinate{5, 4, 4}, Coordinate{5, 5, 4}, Coordinate{4, 4, 5}, Coordinate{5, 4, 5}, Coordinate{4, 5, 5}};

template <usize N>
constexpr bool Contains(const std::array<Coordinate, N>& cells, usize x, usize y, usize z)
{
  for(const auto& cell : cells)
  {
    if(cell.isAt(x, y, z))
    {
      return true;
    }
  }
  return false;
}

constexpr int32 GetInputFeatureId(usize x, usize y, usize z)
{
  if(Contains(k_Feature2Cells, x, y, z))
  {
    return 2;
  }
  if(Contains(k_Feature3Cells, x, y, z))
  {
    return 3;
  }
  if(Contains(k_Feature4Cells, x, y, z))
  {
    return 4;
  }
  if(k_Feature5Cell.isAt(x, y, z))
  {
    return 5;
  }
  if(Contains(k_Feature6Cells, x, y, z))
  {
    return 6;
  }
  return 1;
}

// ---------------------------------------------------------------------------
// Class 1 oracle - reassignment source index, derived by hand from
// Algorithms/RequireMinimumSizeFeatures.cpp::assignBadVoxels().
//
// Face neighbours are visited in the fixed order -Z, -Y, -X, +X, +Y, +Z
// (initializeFaceNeighborInternalIdx<Image3D>(), NeighborUtilities.hpp:204-208).
// Each valid neighbour whose FeatureId is >= 0 casts one vote for its feature,
// and the *voxel* that pushed a feature's running count strictly above the
// current maximum becomes the copy source ("currentVoteCount > maxVoteCount",
// RequireMinimumSizeFeatures.cpp:231). Strict > means a tie is kept by whoever
// reached the count first, i.e. the earlier neighbour in traversal order.
//
// The values below are the *effective* source index: the index whose ORIGINAL
// tuple ends up in the cell, which for the multi-pass corner cell is one hop
// further back than its immediate copy source.
//
// All-phase run (features 3, 4 and 5 removed; -1 marks a removed cell):
//
//   (0,0,0) idx 0   pass 1: +X=1(-1) +Y=6(-1) +Z=36(-1); no valid vote, unresolved.
//                   pass 2: +X=1 -> f1 count 1 > 0, src 1
//                           +Y=6 -> f1 count 2 > 1, src 6
//                           +Z=36 -> f1 count 3 > 2, src 36  <-- winner
//                   Index 36 was itself filled in pass 1 from index 72, so the
//                   tuple that lands at index 0 originated at index 72.
//   (1,0,0) idx 1   -Z,-Y invalid; -X=0(-1) skipped;
//                   +X=2 -> f1 1 > 0, src 2; +Y=7 -> f1 2 > 1, src 7;
//                   +Z=37 -> f1 3 > 2, src 37  <-- winner
//   (0,1,0) idx 6   -Z,-X invalid; -Y=0(-1) skipped;
//                   +X=7 -> src 7; +Y=12 -> src 12; +Z=42 -> src 42  <-- winner
//   (0,0,1) idx 36  -Y,-X invalid; -Z=0(-1) skipped;
//                   +X=37 -> src 37; +Y=42 -> src 42; +Z=72 -> src 72  <-- winner
//   (0,5,5) idx 210 -X,+Y,+Z invalid;
//                   -Z=174 -> f1 1 > 0, src 174; -Y=204 -> f1 2 > 1, src 204;
//                   +X=211 -> f1 3 > 2, src 211  <-- winner
//   (2,5,5) idx 212 +Y,+Z invalid. THE TIE CELL: two feature-1 and two feature-2
//                   neighbours.
//                   -Z=176 (f1) -> count[1]=1 > 0, src 176
//                   -Y=206 (f2) -> count[2]=1 > 1? no
//                   -X=211 (f1) -> count[1]=2 > 1, src 211  <-- winner
//                   +X=213 (f2) -> count[2]=2 > 2? no
//                   A 2-vs-2 tie resolves to feature 1 only because the test is
//                   strictly greater-than. A >= test would hand it to index 213.
//   (5,5,5) idx 215 +X,+Y,+Z invalid. Every valid neighbour belongs to feature 6,
//                   so a non-feature-1 survivor wins outright:
//                   -Z=179 -> count[6]=1 > 0, src 179
//                   -Y=209 -> count[6]=2 > 1, src 209
//                   -X=214 -> count[6]=3 > 2, src 214  <-- winner
//
// Single-phase run (PhaseNumber=1, so only features 3 and 4 are removed):
//   Feature 5 survives, so (0,0,0) keeps its own tuple and never needs a second
//   pass. Its retained id also becomes a *candidate* vote for the three feature-3
//   cells, but feature 1 still out-votes it 3-to-1 in each case, so indices 1, 6
//   and 36 keep the same sources as the all-phase run. Indices 210, 212 and 215
//   are untouched by the phase filter and are identical too.
// ---------------------------------------------------------------------------
constexpr usize GetExpectedSourceIndex(bool applySinglePhase, usize x, usize y, usize z)
{
  if(k_Feature5Cell.isAt(x, y, z))
  {
    return applySinglePhase ? GetIndex(0, 0, 0) : GetIndex(0, 0, 2);
  }
  if(Coordinate{1, 0, 0}.isAt(x, y, z))
  {
    return GetIndex(1, 0, 1);
  }
  if(Coordinate{0, 1, 0}.isAt(x, y, z))
  {
    return GetIndex(0, 1, 1);
  }
  if(Coordinate{0, 0, 1}.isAt(x, y, z))
  {
    return GetIndex(0, 0, 2);
  }
  if(Coordinate{0, 5, 5}.isAt(x, y, z) || Coordinate{2, 5, 5}.isAt(x, y, z))
  {
    return GetIndex(1, 5, 5);
  }
  if(Coordinate{5, 5, 5}.isAt(x, y, z))
  {
    return GetIndex(4, 5, 5);
  }
  return GetIndex(x, y, z);
}

// ---------------------------------------------------------------------------
// Class 1 oracle - final FeatureIds after RemoveInactiveObjects compaction.
//
// All-phase:    keep list {1, 2, 6} -> newNames 1->1, 2->2, 6->3 (removed -> 0)
// Single-phase: keep list {1, 2, 5, 6} -> newNames 1->1, 2->2, 5->3, 6->4
// ---------------------------------------------------------------------------
constexpr int32 GetExpectedFeatureId(bool applySinglePhase, usize x, usize y, usize z)
{
  if(Contains(k_Feature2Cells, x, y, z))
  {
    return 2;
  }
  if(Contains(k_Feature6Cells, x, y, z))
  {
    return applySinglePhase ? 4 : 3;
  }
  // (5,5,5) is a removed feature-4 cell whose vote was won by feature 6.
  if(Coordinate{5, 5, 5}.isAt(x, y, z))
  {
    return applySinglePhase ? 4 : 3;
  }
  // The phase-2 single-cell feature survives only in the single-phase run.
  if(k_Feature5Cell.isAt(x, y, z))
  {
    return applySinglePhase ? 3 : 1;
  }
  return 1;
}

void PopulateDataStructure(DataStructure& dataStructure)
{
  const SizeVec3 imageSize = {k_Dimension, k_Dimension, k_Dimension};
  // ShapeType is slowest-to-fastest (z, y, x); this fixture is a cube, so the order is moot here.
  const ShapeType cellShape = {k_Dimension, k_Dimension, k_Dimension};

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions(imageSize);
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", cellShape, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  auto* featureAmPtr = AttributeMatrix::Create(dataStructure, "FeatureData", {k_FeatureCount}, imageGeomPtr->getId());

  auto featureIdsStore = DataStoreUtilities::CreateDataStore<int32>(cellShape, {1}, IDataAction::Mode::Execute);
  auto* featureIdsPtr = DataArray<int32>::Create(dataStructure, "FeatureIds", featureIdsStore, cellAmPtr->getId());
  auto copiedScalarStore = DataStoreUtilities::CreateDataStore<int32>(cellShape, {1}, IDataAction::Mode::Execute);
  auto* copiedScalarPtr = DataArray<int32>::Create(dataStructure, "CopiedScalar", copiedScalarStore, cellAmPtr->getId());
  auto copiedVectorStore = DataStoreUtilities::CreateDataStore<float32>(cellShape, {3}, IDataAction::Mode::Execute);
  auto* copiedVectorPtr = DataArray<float32>::Create(dataStructure, "CopiedVector", copiedVectorStore, cellAmPtr->getId());
  auto numCellsStore = DataStoreUtilities::CreateDataStore<int32>({k_FeatureCount}, {1}, IDataAction::Mode::Execute);
  auto* numCellsPtr = DataArray<int32>::Create(dataStructure, "NumElements", numCellsStore, featureAmPtr->getId());
  auto phasesStore = DataStoreUtilities::CreateDataStore<int32>({k_FeatureCount}, {1}, IDataAction::Mode::Execute);
  auto* phasesPtr = DataArray<int32>::Create(dataStructure, "Phases", phasesStore, featureAmPtr->getId());

  for(usize z = 0; z < k_Dimension; z++)
  {
    for(usize y = 0; y < k_Dimension; y++)
    {
      for(usize x = 0; x < k_Dimension; x++)
      {
        const usize index = GetIndex(x, y, z);
        featureIdsPtr->getDataStoreRef()[index] = GetInputFeatureId(x, y, z);
        copiedScalarPtr->getDataStoreRef()[index] = static_cast<int32>(10000 + index);
        for(usize component = 0; component < k_VectorOffsets.size(); component++)
        {
          copiedVectorPtr->getDataStoreRef()[index * k_VectorOffsets.size() + component] = static_cast<float32>(index) + k_VectorOffsets[component];
        }
      }
    }
  }

  const std::array<int32, k_FeatureCount> inputNumCells = {0, 200, 4, 3, 3, 1, 5};
  const std::array<int32, k_FeatureCount> inputPhases = {0, 1, 1, 1, 1, 2, 1};
  for(usize featureId = 0; featureId < k_FeatureCount; featureId++)
  {
    numCellsPtr->getDataStoreRef()[featureId] = inputNumCells[featureId];
    phasesPtr->getDataStoreRef()[featureId] = inputPhases[featureId];
  }
}

// Guards the hand-written NumElements array against the hand-written grid drifting
// apart. The filter trusts NumElements, so the two must agree for the oracle to mean
// anything.
void CheckFixtureSelfConsistency()
{
  std::array<int32, k_FeatureCount> counted = {0, 0, 0, 0, 0, 0, 0};
  for(usize z = 0; z < k_Dimension; z++)
  {
    for(usize y = 0; y < k_Dimension; y++)
    {
      for(usize x = 0; x < k_Dimension; x++)
      {
        counted[static_cast<usize>(GetInputFeatureId(x, y, z))]++;
      }
    }
  }
  const std::array<int32, k_FeatureCount> expected = {0, 200, 4, 3, 3, 1, 5};
  for(usize featureId = 0; featureId < k_FeatureCount; featureId++)
  {
    INFO("feature id " + std::to_string(featureId));
    CHECK(counted[featureId] == expected[featureId]);
  }
}
} // namespace DiscriminatingFixture
} // namespace

TEST_CASE("SimplnxCore::RequireMinimumSizeFeaturesFilter: Two-Phase 6x6x6 Analytical Oracle", "[SimplnxCore][RequireMinimumSizeFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  const bool applySinglePhase = GENERATE(false, true);
  // std::boolalpha so the generated section renders as "ApplySinglePhase=false" /
  // "ApplySinglePhase=true" in the ctest log rather than "=0" / "=1", matching how the
  // V&V report and coverage table name the two runs.
  DYNAMIC_SECTION("ApplySinglePhase=" << std::boolalpha << applySinglePhase)
  {
    // Runs once per generated section. That is deliberate: it is a pure in-memory count
    // over 216 cells, and keeping it beside the fixture it guards is worth the repeat.
    DiscriminatingFixture::CheckFixtureSelfConsistency();

    DataStructure dataStructure;
    DiscriminatingFixture::PopulateDataStructure(dataStructure);

    RequireMinimumSizeFeaturesFilter filter;
    Arguments args;
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_MinAllowedFeaturesSize_Key, std::make_any<int64>(DiscriminatingFixture::k_MinAllowedFeaturesSize));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_ApplySinglePhase_Key, std::make_any<bool>(applySinglePhase));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_SinglePhaseNumber_Key, std::make_any<int32>(DiscriminatingFixture::k_TargetPhase));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureNumCellsPath_Key, std::make_any<DataPath>(k_NumCellsPath));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeaturePhasesPath_Key, std::make_any<DataPath>(k_PhasesPath));

    const auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    const auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_CopiedScalarPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_CopiedVectorPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_NumCellsPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_PhasesPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(k_FeatureDataPath));
    const auto& outputFeatureIds = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
    const auto& outputCopiedScalar = dataStructure.getDataRefAs<Int32Array>(k_CopiedScalarPath);
    const auto& outputCopiedVector = dataStructure.getDataRefAs<Float32Array>(k_CopiedVectorPath);
    const auto& outputNumCells = dataStructure.getDataRefAs<Int32Array>(k_NumCellsPath);
    const auto& outputPhases = dataStructure.getDataRefAs<Int32Array>(k_PhasesPath);
    const auto& outputFeatureAmRef = dataStructure.getDataRefAs<AttributeMatrix>(k_FeatureDataPath);

    // Class 1: every one of the 216 cells, with the reassignment source proven by the
    // index-encoded companion arrays rather than merely by the winning feature id.
    for(usize z = 0; z < DiscriminatingFixture::k_Dimension; z++)
    {
      for(usize y = 0; y < DiscriminatingFixture::k_Dimension; y++)
      {
        for(usize x = 0; x < DiscriminatingFixture::k_Dimension; x++)
        {
          const usize index = DiscriminatingFixture::GetIndex(x, y, z);
          const usize sourceIndex = DiscriminatingFixture::GetExpectedSourceIndex(applySinglePhase, x, y, z);
          CAPTURE(x, y, z, index, sourceIndex, applySinglePhase);
          CHECK(outputFeatureIds[index] == DiscriminatingFixture::GetExpectedFeatureId(applySinglePhase, x, y, z));
          CHECK(outputCopiedScalar[index] == static_cast<int32>(10000 + sourceIndex));
          for(usize component = 0; component < DiscriminatingFixture::k_VectorOffsets.size(); component++)
          {
            CHECK(outputCopiedVector[index * DiscriminatingFixture::k_VectorOffsets.size() + component] == static_cast<float32>(sourceIndex) + DiscriminatingFixture::k_VectorOffsets[component]);
          }
        }
      }
    }

    // Class 1: the compacted feature-level arrays. Feature 2 has exactly
    // MinAllowedFeaturesSize cells and is present in both runs, which is the
    // at-threshold survival assertion carried on the full fixture.
    const std::vector<int32> expectedNumCells = applySinglePhase ? std::vector<int32>{0, 200, 4, 1, 5} : std::vector<int32>{0, 200, 4, 5};
    const std::vector<int32> expectedPhases = applySinglePhase ? std::vector<int32>{0, 1, 1, 2, 1} : std::vector<int32>{0, 1, 1, 1};
    REQUIRE(outputNumCells.getNumberOfTuples() == expectedNumCells.size());
    REQUIRE(outputPhases.getNumberOfTuples() == expectedPhases.size());
    for(usize featureId = 0; featureId < expectedNumCells.size(); featureId++)
    {
      CAPTURE(featureId, applySinglePhase);
      CHECK(outputNumCells[featureId] == expectedNumCells[featureId]);
      CHECK(outputPhases[featureId] == expectedPhases[featureId]);
    }

    CheckClass4Invariants(outputFeatureIds, outputFeatureAmRef, DiscriminatingFixture::k_CellCount);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::RequireMinimumSizeFeaturesFilter: At-Threshold Boundary", "[SimplnxCore][RequireMinimumSizeFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  // A 5x1x1 strip holding feature 1 (3 cells) and feature 2 (2 cells):
  //
  //   index      0  1  2  3  4
  //   FeatureIds 1  1  1  2  2
  //
  // RequireMinimumSizeFeatures.cpp:297 keeps a feature when
  // "NumElements >= MinAllowedFeaturesSize", so removal happens strictly BELOW the
  // threshold. Sweeping the threshold across both feature sizes proves it:
  //
  //   threshold 2: feature 2 sits exactly AT the threshold -> both survive, nothing
  //                changes. A "<=" removal rule would delete feature 2 here.
  //   threshold 3: feature 2 (2 < 3) is removed, feature 1 sits exactly AT the
  //                threshold and survives. Cell 3 votes -X=2 (feature 1, the only
  //                valid non-negative neighbour) and takes index 2's tuple. Cell 4
  //                has no valid non-negative neighbour on pass one (+X is off the
  //                volume, -X is the still-removed cell 3), so it resolves on pass
  //                two from cell 3 - which by then carries index 2's tuple.
  //   threshold 4: both features fall below the threshold, so the algorithm's
  //                "all Features would be removed" guard fires.
  // The strip holds 5 cells and 3 feature tuples (the reserved feature 0 plus features
  // 1 and 2). Each cell's CopiedScalar carries k_StripScalarBase + index so the
  // assertion names the winning source voxel, not merely the winning feature.
  constexpr usize k_StripCellCount = 5;
  constexpr usize k_StripFeatureCount = 3;
  constexpr int32 k_StripScalarBase = 100;

  const int64 minAllowedFeaturesSize = GENERATE(int64{2}, int64{3}, int64{4});
  DYNAMIC_SECTION("MinAllowedFeaturesSize=" << minAllowedFeaturesSize)
  {
    DataStructure dataStructure;

    const SizeVec3 imageSize = {k_StripCellCount, 1, 1};
    const ShapeType arraySize(std::reverse_iterator(imageSize.end()), std::reverse_iterator(imageSize.begin()));

    auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
    imageGeomPtr->setDimensions(imageSize);
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", arraySize, imageGeomPtr->getId());
    imageGeomPtr->setCellData(*cellAmPtr);
    auto* featureAmPtr = AttributeMatrix::Create(dataStructure, "FeatureData", {k_StripFeatureCount}, imageGeomPtr->getId());

    auto* featureIdsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", arraySize, {1}, cellAmPtr->getId());
    auto* copiedScalarPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "CopiedScalar", arraySize, {1}, cellAmPtr->getId());
    auto* numCellsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "NumElements", {k_StripFeatureCount}, {1}, featureAmPtr->getId());
    auto* phasesPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {k_StripFeatureCount}, {1}, featureAmPtr->getId());

    const std::array<int32, k_StripCellCount> inputFeatureIds = {1, 1, 1, 2, 2};
    for(usize index = 0; index < inputFeatureIds.size(); index++)
    {
      featureIdsPtr->getDataStoreRef()[index] = inputFeatureIds[index];
      copiedScalarPtr->getDataStoreRef()[index] = k_StripScalarBase + static_cast<int32>(index);
    }
    const std::array<int32, k_StripFeatureCount> inputNumCells = {0, 3, 2};
    const std::array<int32, k_StripFeatureCount> inputPhases = {0, 1, 1};
    for(usize featureId = 0; featureId < inputNumCells.size(); featureId++)
    {
      numCellsPtr->getDataStoreRef()[featureId] = inputNumCells[featureId];
      phasesPtr->getDataStoreRef()[featureId] = inputPhases[featureId];
    }

    RequireMinimumSizeFeaturesFilter filter;
    Arguments args;
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_MinAllowedFeaturesSize_Key, std::make_any<int64>(minAllowedFeaturesSize));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_ApplySinglePhase_Key, std::make_any<bool>(false));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_SinglePhaseNumber_Key, std::make_any<int32>(1));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureNumCellsPath_Key, std::make_any<DataPath>(k_NumCellsPath));
    args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeaturePhasesPath_Key, std::make_any<DataPath>(k_PhasesPath));

    const auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    const auto executeResult = filter.execute(dataStructure, args);

    if(minAllowedFeaturesSize == 4)
    {
      SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
      REQUIRE(executeResult.result.errors().size() == 1);
      CHECK(executeResult.result.errors()[0].code == -1);
      UnitTest::CheckArraysInheritTupleDims(dataStructure);
      return;
    }

    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_CopiedScalarPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_NumCellsPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(k_FeatureDataPath));
    const auto& outputFeatureIds = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
    const auto& outputCopiedScalar = dataStructure.getDataRefAs<Int32Array>(k_CopiedScalarPath);
    const auto& outputNumCells = dataStructure.getDataRefAs<Int32Array>(k_NumCellsPath);
    const auto& outputFeatureAmRef = dataStructure.getDataRefAs<AttributeMatrix>(k_FeatureDataPath);

    using StripArray = std::array<int32, k_StripCellCount>;
    const StripArray expectedFeatureIds = (minAllowedFeaturesSize == 2) ? StripArray{1, 1, 1, 2, 2} : StripArray{1, 1, 1, 1, 1};
    const StripArray expectedCopiedScalar = (minAllowedFeaturesSize == 2) ? StripArray{100, 101, 102, 103, 104} : StripArray{100, 101, 102, 102, 102};
    const std::vector<int32> expectedNumCells = (minAllowedFeaturesSize == 2) ? std::vector<int32>{0, 3, 2} : std::vector<int32>{0, 3};
    for(usize index = 0; index < expectedFeatureIds.size(); index++)
    {
      CAPTURE(index, minAllowedFeaturesSize);
      CHECK(outputFeatureIds[index] == expectedFeatureIds[index]);
      CHECK(outputCopiedScalar[index] == expectedCopiedScalar[index]);
    }
    REQUIRE(outputNumCells.getNumberOfTuples() == expectedNumCells.size());
    for(usize featureId = 0; featureId < expectedNumCells.size(); featureId++)
    {
      CAPTURE(featureId, minAllowedFeaturesSize);
      CHECK(outputNumCells[featureId] == expectedNumCells[featureId]);
    }

    CheckClass4Invariants(outputFeatureIds, outputFeatureAmRef, k_StripCellCount);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::RequireMinimumSizeFeaturesFilter: Vote Counter Reset Between Cells", "[SimplnxCore][RequireMinimumSizeFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  // ---------------------------------------------------------------------------
  // Class 1 oracle isolating the per-cell vote-counter reset
  // (`std::fill(voteCounter.begin(), voteCounter.end(), 0)`,
  // Algorithms/RequireMinimumSizeFeatures.cpp:240).
  //
  // `voteCounter` is allocated once outside the sweep (`:193`) while `maxVoteCount` is
  // re-declared per orphan cell (`:215`). The reset at `:240` is therefore the ONLY thing
  // stopping one orphan cell's tallies from being carried into the next cell swept. The
  // 6x6x6 fixture cannot see a broken reset: every one of its orphan cells is decided by
  // a margin wider than any counter an earlier cell could leak. This fixture is built so
  // that a leak flips exactly one cell.
  //
  // A 9x1x1 strip, MinAllowedFeaturesSize = 3. Sweep order is ascending flat index
  // (the triple loop at `:198-208` over z, then y, then x; on a 9x1x1 strip that is
  // simply x = 0..8). Face-neighbour traversal order is [-Z, -Y, -X, +X, +Y, +Z]
  // (initializeFaceNeighborInternalIdx<Image3D>()); on this strip only -X and +X are
  // ever valid, so -X is always visited before +X.
  //
  //   index        0  1  2  3  4  5  6  7  8
  //   FeatureIds   1  3  1  2  2  2  3  1  1
  //   CopiedScalar 10000 + index
  //
  //   NumElements  {0, 4, 3, 2}   (f1: 0,2,7,8 | f2: 3,4,5 | f3: 1,6)
  //   Phases       {0, 1, 1, 1}
  //
  // removeSmallFeatures (`:297`): f1 (4 >= 3) keep, f2 (3 >= 3) keep, f3 (2 < 3) REMOVED.
  // The two f3 cells at indices 1 and 6 become -1 and must be voted on.
  //
  // --- Scenario A: WITH the reset (the shipped code; this is what is asserted) --------
  //
  //   idx 1 (LEAK SOURCE, swept first): both neighbours belong to feature 1.
  //       -X = idx 0 (f1): voteCounter[1] = 1, 1 > 0 -> max = 1, src = 0
  //       +X = idx 2 (f1): voteCounter[1] = 2, 2 > 1 -> max = 2, src = 2
  //       recorded src = 2. Then `:240` zeroes voteCounter, discarding the count of 2.
  //
  //   idx 6 (THE TIE CELL, swept second): a genuine 1-vs-1 CROSS-FEATURE tie.
  //       -X = idx 5 (f2): voteCounter[2] = 1, 1 > 0 -> max = 1, src = 5
  //       +X = idx 7 (f1): voteCounter[1] = 1, 1 > 1 is FALSE -> src unchanged
  //       recorded src = 5, i.e. the tie goes to the earlier neighbour, feature 2.
  //
  //   Pass 1 transfer: cell 1 <- cell 2, cell 6 <- cell 5. Pass 2 finds no negative
  //   FeatureId, so counter == 0 and the loop at `:195` exits.
  //
  //   FeatureIds   1  1  1  2  2  2 [2] 1  1
  //   CopiedScalar 10000 10002 10002 10003 10004 10005 [10005] 10007 10008
  //
  //   Compaction keeps {0, 1, 2} and maps 3 -> 0, so the surviving ids are unchanged and
  //   the feature arrays shrink from 4 tuples to 3: NumElements {0, 4, 3}.
  //
  // --- Scenario B: WITHOUT the reset (delete `:240` - the mutant) ---------------------
  //
  //   idx 1 is unaffected (voteCounter starts zeroed on the first cell), src = 2, but it
  //   leaves voteCounter[1] == 2 behind.
  //
  //   idx 6 now starts from voteCounter = {0, 2, 0, 0}:
  //       -X = idx 5 (f2): voteCounter[2] = 1, 1 > 0 -> max = 1, src = 5
  //       +X = idx 7 (f1): voteCounter[1] = 3, 3 > 1 -> max = 3, src = 7   <-- FLIPPED
  //       recorded src = 7, i.e. feature 1 wins on two votes it never received.
  //
  //   FeatureIds[6]   = 1     (oracle: 2)
  //   CopiedScalar[6] = 10007 (oracle: 10005)
  //
  //   Index 6 is the only cell that changes, and the Class 4 invariants still hold under
  //   the mutant (ids are still {1, 2} over 3 feature tuples), so the two Class 1 checks
  //   at index 6 are what kill it.
  // ---------------------------------------------------------------------------
  constexpr usize k_StripCellCount = 9;
  constexpr usize k_StripFeatureCount = 4;
  constexpr int32 k_StripScalarBase = 10000;
  constexpr int64 k_MinAllowedFeaturesSize = 3;
  constexpr usize k_LeakSourceIndex = 1;
  constexpr usize k_TieCellIndex = 6;

  using StripArray = std::array<int32, k_StripCellCount>;

  DataStructure dataStructure;

  const SizeVec3 imageSize = {k_StripCellCount, 1, 1};
  const ShapeType arraySize(std::reverse_iterator(imageSize.end()), std::reverse_iterator(imageSize.begin()));

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions(imageSize);
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", arraySize, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);
  auto* featureAmPtr = AttributeMatrix::Create(dataStructure, "FeatureData", {k_StripFeatureCount}, imageGeomPtr->getId());

  auto* featureIdsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", arraySize, {1}, cellAmPtr->getId());
  auto* copiedScalarPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "CopiedScalar", arraySize, {1}, cellAmPtr->getId());
  auto* numCellsPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "NumElements", {k_StripFeatureCount}, {1}, featureAmPtr->getId());
  auto* phasesPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {k_StripFeatureCount}, {1}, featureAmPtr->getId());

  const StripArray inputFeatureIds = {1, 3, 1, 2, 2, 2, 3, 1, 1};
  for(usize index = 0; index < inputFeatureIds.size(); index++)
  {
    featureIdsPtr->getDataStoreRef()[index] = inputFeatureIds[index];
    copiedScalarPtr->getDataStoreRef()[index] = k_StripScalarBase + static_cast<int32>(index);
  }
  const std::array<int32, k_StripFeatureCount> inputNumCells = {0, 4, 3, 2};
  const std::array<int32, k_StripFeatureCount> inputPhases = {0, 1, 1, 1};
  for(usize featureId = 0; featureId < inputNumCells.size(); featureId++)
  {
    numCellsPtr->getDataStoreRef()[featureId] = inputNumCells[featureId];
    phasesPtr->getDataStoreRef()[featureId] = inputPhases[featureId];
  }

  // Guard the hand-written NumElements array against the hand-written grid: the filter
  // trusts NumElements, so if the two ever drift apart the oracle above stops meaning
  // anything.
  std::array<int32, k_StripFeatureCount> countedCells = {0, 0, 0, 0};
  for(const auto& featureId : inputFeatureIds)
  {
    countedCells[static_cast<usize>(featureId)]++;
  }
  for(usize featureId = 0; featureId < k_StripFeatureCount; featureId++)
  {
    INFO("feature id " + std::to_string(featureId));
    CHECK(countedCells[featureId] == inputNumCells[featureId]);
  }

  RequireMinimumSizeFeaturesFilter filter;
  Arguments args;
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_MinAllowedFeaturesSize_Key, std::make_any<int64>(k_MinAllowedFeaturesSize));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_ApplySinglePhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_SinglePhaseNumber_Key, std::make_any<int32>(1));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureNumCellsPath_Key, std::make_any<DataPath>(k_NumCellsPath));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeaturePhasesPath_Key, std::make_any<DataPath>(k_PhasesPath));

  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_CopiedScalarPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_NumCellsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_PhasesPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(k_FeatureDataPath));
  const auto& outputFeatureIds = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
  const auto& outputCopiedScalar = dataStructure.getDataRefAs<Int32Array>(k_CopiedScalarPath);
  const auto& outputNumCells = dataStructure.getDataRefAs<Int32Array>(k_NumCellsPath);
  const auto& outputPhases = dataStructure.getDataRefAs<Int32Array>(k_PhasesPath);
  const auto& outputFeatureAmRef = dataStructure.getDataRefAs<AttributeMatrix>(k_FeatureDataPath);

  // Scenario A, asserted. Deleting the reset at :240 changes index 6 only, to
  // FeatureId 1 / CopiedScalar 10007.
  const StripArray expectedFeatureIds = {1, 1, 1, 2, 2, 2, 2, 1, 1};
  const StripArray expectedCopiedScalar = {10000, 10002, 10002, 10003, 10004, 10005, 10005, 10007, 10008};
  for(usize index = 0; index < expectedFeatureIds.size(); index++)
  {
    CAPTURE(index);
    CHECK(outputFeatureIds[index] == expectedFeatureIds[index]);
    CHECK(outputCopiedScalar[index] == expectedCopiedScalar[index]);
  }

  // Restate the two discriminating assertions on their own so a failure names the
  // mechanism rather than just an index.
  INFO("index " + std::to_string(k_LeakSourceIndex) + " is the leak source: both its neighbours are feature 1, so it leaves voteCounter[1] == 2 behind if :240 is removed");
  CHECK(outputCopiedScalar[k_LeakSourceIndex] == 10002);
  INFO("index " + std::to_string(k_TieCellIndex) + " is a 1-vs-1 cross-feature tie that must resolve to the -X neighbour at index 5 (feature 2); a leaked count flips it to index 7 (feature 1)");
  CHECK(outputFeatureIds[k_TieCellIndex] == 2);
  CHECK(outputCopiedScalar[k_TieCellIndex] == 10005);

  const std::vector<int32> expectedNumCells = {0, 4, 3};
  const std::vector<int32> expectedPhases = {0, 1, 1};
  REQUIRE(outputNumCells.getNumberOfTuples() == expectedNumCells.size());
  REQUIRE(outputPhases.getNumberOfTuples() == expectedPhases.size());
  for(usize featureId = 0; featureId < expectedNumCells.size(); featureId++)
  {
    CAPTURE(featureId);
    CHECK(outputNumCells[featureId] == expectedNumCells[featureId]);
    CHECK(outputPhases[featureId] == expectedPhases[featureId]);
  }

  CheckClass4Invariants(outputFeatureIds, outputFeatureAmRef, k_StripCellCount);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RequireMinimumSizeFeaturesFilter: Execute Error - unavailable phase (-5555)", "[SimplnxCore][RequireMinimumSizeFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  DiscriminatingFixture::PopulateDataStructure(dataStructure);

  RequireMinimumSizeFeaturesFilter filter;
  Arguments args;
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_MinAllowedFeaturesSize_Key, std::make_any<int64>(DiscriminatingFixture::k_MinAllowedFeaturesSize));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_ApplySinglePhase_Key, std::make_any<bool>(true));
  // The fixture only carries phases 1 and 2.
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_SinglePhaseNumber_Key, std::make_any<int32>(7));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeatureNumCellsPath_Key, std::make_any<DataPath>(k_NumCellsPath));
  args.insertOrAssign(RequireMinimumSizeFeaturesFilter::k_FeaturePhasesPath_Key, std::make_any<DataPath>(k_PhasesPath));

  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  CHECK(executeResult.result.errors()[0].code == -5555);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RequireMinimumSizeFeaturesFilter: SIMPL Backwards Compatibility", "[SimplnxCore][RequireMinimumSizeFeaturesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "RequireMinimumSizeFeaturesFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "RequireMinimumSizeFeaturesFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<RequireMinimumSizeFeaturesFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<int64>(RequireMinimumSizeFeaturesFilter::k_MinAllowedFeaturesSize_Key) == 5);
      CHECK(args.value<bool>(RequireMinimumSizeFeaturesFilter::k_ApplySinglePhase_Key) == true);
      CHECK(args.value<int32>(RequireMinimumSizeFeaturesFilter::k_SinglePhaseNumber_Key) == 5);
      CHECK(args.value<DataPath>(RequireMinimumSizeFeaturesFilter::k_ImageGeomPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(RequireMinimumSizeFeaturesFilter::k_FeatureIdsPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(RequireMinimumSizeFeaturesFilter::k_FeaturePhasesPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(RequireMinimumSizeFeaturesFilter::k_FeatureNumCellsPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}
