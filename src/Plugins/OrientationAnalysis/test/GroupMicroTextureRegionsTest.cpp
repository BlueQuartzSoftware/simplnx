#include "OrientationAnalysis/Filters/GroupMicroTextureRegionsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <set>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::UnitTest;

namespace
{
namespace AnalyticalFixtures
{
const std::string k_GeomName = "ImageGeometry";
const DataPath k_ImageGeomPath = DataPath({k_GeomName});
const DataPath k_CellDataPath = k_ImageGeomPath.createChildPath("CellData");
const DataPath k_FeatureDataPath = k_ImageGeomPath.createChildPath("CellFeatureData");
const DataPath k_EnsembleDataPath = k_ImageGeomPath.createChildPath("CellEnsembleData");

const std::string k_FeatureIdsName = "FeatureIds";
const std::string k_FeaturePhasesName = "FeaturePhases";
const std::string k_VolumesName = "Volumes";
const std::string k_AvgQuatsName = "AvgQuats";
const std::string k_ContigNeighborListName = "ContiguousNeighborList";
const std::string k_NonContigNeighborListName = "NonContiguousNeighborList";
const std::string k_CrystalStructuresName = "CrystalStructures";

const std::string k_NewFeatureAMName = "MicroTextureFeatureData";
const std::string k_CellParentIdsName = "CellParentIds";
const std::string k_FeatureParentIdsName = "FeatureParentIds";
const std::string k_SeedArrayName = "GroupMicroTextureRegions_Seed";

const DataPath k_FeatureIdsPath = k_CellDataPath.createChildPath(k_FeatureIdsName);
const DataPath k_FeaturePhasesPath = k_FeatureDataPath.createChildPath(k_FeaturePhasesName);
const DataPath k_VolumesPath = k_FeatureDataPath.createChildPath(k_VolumesName);
const DataPath k_AvgQuatsPath = k_FeatureDataPath.createChildPath(k_AvgQuatsName);
const DataPath k_ContigNeighborListPath = k_FeatureDataPath.createChildPath(k_ContigNeighborListName);
const DataPath k_NonContigNeighborListPath = k_FeatureDataPath.createChildPath(k_NonContigNeighborListName);
const DataPath k_CrystalStructuresPath = k_EnsembleDataPath.createChildPath(k_CrystalStructuresName);
const DataPath k_NewFeatureAMPath = k_ImageGeomPath.createChildPath(k_NewFeatureAMName);

// Pure-Phi Bunge rotations tilt the c-axis by phiDeg. The c-axis metric folds
// antiparallel directions into [0,90]. Quaternions use x,y,z,w order.
std::array<float32, 4> QuatFromPhiDeg(float32 phiDeg)
{
  const float32 halfAngleRad = (phiDeg * 0.5f) * 3.14159265358979323846f / 180.0f;
  return {std::sin(halfAngleRad), 0.0f, 0.0f, std::cos(halfAngleRad)};
}

/**
 * @struct FixtureData
 * @brief Holds arrays for one analytical microtexture fixture.
 */
struct FixtureData
{
  DataStructure ds;
  ImageGeom* geom = nullptr;
  AttributeMatrix* cellAM = nullptr;
  AttributeMatrix* featureAM = nullptr;
  AttributeMatrix* ensembleAM = nullptr;
  Int32Array* featureIds = nullptr;
  Int32Array* featurePhases = nullptr;
  Float32Array* volumes = nullptr;
  Float32Array* avgQuats = nullptr;
  NeighborList<int32>* neighborList = nullptr;
  NeighborList<int32>* nonContiguousNeighborList = nullptr;
  UInt32Array* crystalStructures = nullptr;
};

// Build one cell per real feature. Feature zero remains available as the
// background tuple. Valid features use Hexagonal_High.
FixtureData CreateScaffold(usize numFeatures)
{
  FixtureData td;
  const usize nX = numFeatures - 1;
  const usize numCells = nX;
  const usize numCrystalStructures = 3;

  td.geom = ImageGeom::Create(td.ds, k_GeomName);
  td.geom->setSpacing({1.0f, 1.0f, 1.0f});
  td.geom->setOrigin({0.0f, 0.0f, 0.0f});
  td.geom->setDimensions({nX, 1, 1});

  td.cellAM = AttributeMatrix::Create(td.ds, "CellData", ShapeType{1, 1, nX}, td.geom->getId());
  td.featureAM = AttributeMatrix::Create(td.ds, "CellFeatureData", ShapeType{numFeatures}, td.geom->getId());
  td.ensembleAM = AttributeMatrix::Create(td.ds, "CellEnsembleData", ShapeType{numCrystalStructures}, td.geom->getId());

  td.featureIds = CreateTestDataArray<int32>(td.ds, k_FeatureIdsName, {1, 1, nX}, {1}, td.cellAM->getId());
  td.featurePhases = CreateTestDataArray<int32>(td.ds, k_FeaturePhasesName, {numFeatures}, {1}, td.featureAM->getId());
  td.volumes = CreateTestDataArray<float32>(td.ds, k_VolumesName, {numFeatures}, {1}, td.featureAM->getId());
  td.avgQuats = CreateTestDataArray<float32>(td.ds, k_AvgQuatsName, {numFeatures}, {4}, td.featureAM->getId());
  td.neighborList = NeighborList<int32>::Create(td.ds, k_ContigNeighborListName, ShapeType{numFeatures}, td.featureAM->getId());
  td.nonContiguousNeighborList = NeighborList<int32>::Create(td.ds, k_NonContigNeighborListName, ShapeType{numFeatures}, td.featureAM->getId());
  td.crystalStructures = CreateTestDataArray<uint32>(td.ds, k_CrystalStructuresName, {numCrystalStructures}, {1}, td.ensembleAM->getId());

  for(usize k = 0; k < numCells; k++)
  {
    (*td.featureIds)[k] = static_cast<int32>(k + 1);
  }

  (*td.featurePhases)[0] = 0;
  for(usize f = 1; f < numFeatures; f++)
  {
    (*td.featurePhases)[f] = 1;
  }

  // Uniform volumes are the baseline for running-average tests.
  for(usize f = 0; f < numFeatures; f++)
  {
    (*td.volumes)[f] = 1.0f;
  }

  for(usize f = 0; f < numFeatures; f++)
  {
    (*td.avgQuats)[f * 4 + 0] = 0.0f;
    (*td.avgQuats)[f * 4 + 1] = 0.0f;
    (*td.avgQuats)[f * 4 + 2] = 0.0f;
    (*td.avgQuats)[f * 4 + 3] = 1.0f;
  }

  for(usize f = 0; f < numFeatures; f++)
  {
    td.neighborList->setList(static_cast<int32>(f), std::make_shared<std::vector<int32>>(std::vector<int32>{}));
    td.nonContiguousNeighborList->setList(static_cast<int32>(f), std::make_shared<std::vector<int32>>(std::vector<int32>{}));
  }

  (*td.crystalStructures)[0] = 999u;
  (*td.crystalStructures)[1] = static_cast<uint32>(ebsdlib::CrystalStructure::Hexagonal_High);
  (*td.crystalStructures)[2] = static_cast<uint32>(ebsdlib::CrystalStructure::Cubic_High);

  return td;
}

void SetAvgQuat(FixtureData& td, usize featureIdx, const std::array<float32, 4>& q)
{
  (*td.avgQuats)[featureIdx * 4 + 0] = q[0];
  (*td.avgQuats)[featureIdx * 4 + 1] = q[1];
  (*td.avgQuats)[featureIdx * 4 + 2] = q[2];
  (*td.avgQuats)[featureIdx * 4 + 3] = q[3];
}

void SetNeighbors(FixtureData& td, int32 featureIdx, std::vector<int32> neighbors)
{
  td.neighborList->setList(featureIdx, std::make_shared<std::vector<int32>>(std::move(neighbors)));
}

void SetNonContiguousNeighbors(FixtureData& td, int32 featureIdx, std::vector<int32> neighbors)
{
  td.nonContiguousNeighborList->setList(featureIdx, std::make_shared<std::vector<int32>>(std::move(neighbors)));
}

// Builds a cell-dense, feature-sparse fixture. The 16 real features form one connected chain
// with identical c-axes, so every feature must receive the same parent id. Each Z slice maps to
// one feature, ensuring the filter's final cell remap visits all 8,000,000 cells.
FixtureData BuildLargeBenchmarkFixture(usize dimension)
{
  constexpr usize k_RealFeatureCount = 16;
  const usize numFeatures = k_RealFeatureCount + 1;
  const ShapeType cellTupleShape = {dimension, dimension, dimension};

  FixtureData td;
  td.geom = ImageGeom::Create(td.ds, k_GeomName);
  td.geom->setSpacing({1.0f, 1.0f, 1.0f});
  td.geom->setOrigin({0.0f, 0.0f, 0.0f});
  td.geom->setDimensions({dimension, dimension, dimension});

  td.cellAM = AttributeMatrix::Create(td.ds, "CellData", cellTupleShape, td.geom->getId());
  td.featureAM = AttributeMatrix::Create(td.ds, "CellFeatureData", ShapeType{numFeatures}, td.geom->getId());
  td.ensembleAM = AttributeMatrix::Create(td.ds, "CellEnsembleData", ShapeType{2}, td.geom->getId());

  auto featureIdsStore = DataStoreUtilities::CreateDataStore<int32>(td.ds, k_FeatureIdsPath, cellTupleShape, {1}, IDataAction::Mode::Execute);
  td.featureIds = Int32Array::Create(td.ds, k_FeatureIdsName, featureIdsStore, td.cellAM->getId());

  auto featurePhasesStore = DataStoreUtilities::CreateDataStore<int32>(td.ds, k_FeaturePhasesPath, {numFeatures}, {1}, IDataAction::Mode::Execute);
  td.featurePhases = Int32Array::Create(td.ds, k_FeaturePhasesName, featurePhasesStore, td.featureAM->getId());

  auto volumesStore = DataStoreUtilities::CreateDataStore<float32>(td.ds, k_VolumesPath, {numFeatures}, {1}, IDataAction::Mode::Execute);
  td.volumes = Float32Array::Create(td.ds, k_VolumesName, volumesStore, td.featureAM->getId());

  auto avgQuatsStore = DataStoreUtilities::CreateDataStore<float32>(td.ds, k_AvgQuatsPath, {numFeatures}, {4}, IDataAction::Mode::Execute);
  td.avgQuats = Float32Array::Create(td.ds, k_AvgQuatsName, avgQuatsStore, td.featureAM->getId());

  td.neighborList = NeighborList<int32>::Create(td.ds, k_ContigNeighborListName, ShapeType{numFeatures}, td.featureAM->getId());

  auto crystalStructuresStore = DataStoreUtilities::CreateDataStore<uint32>(td.ds, k_CrystalStructuresPath, {2}, {1}, IDataAction::Mode::Execute);
  td.crystalStructures = UInt32Array::Create(td.ds, k_CrystalStructuresName, crystalStructuresStore, td.ensembleAM->getId());

  const usize sliceSize = dimension * dimension;
  std::vector<int32> featureIdsSlice(sliceSize);
  for(usize z = 0; z < dimension; z++)
  {
    const int32 featureId = static_cast<int32>((z % k_RealFeatureCount) + 1);
    std::fill(featureIdsSlice.begin(), featureIdsSlice.end(), featureId);
    const Result<> writeResult = featureIdsStore->copyFromBuffer(z * sliceSize, nonstd::span<const int32>(featureIdsSlice.data(), featureIdsSlice.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  }

  (*td.featurePhases)[0] = 0;
  (*td.volumes)[0] = 1.0f;
  for(usize f = 1; f < numFeatures; f++)
  {
    (*td.featurePhases)[f] = 1;
    (*td.volumes)[f] = 1.0f;
  }

  for(usize f = 0; f < numFeatures; f++)
  {
    SetAvgQuat(td, f, QuatFromPhiDeg(0.0f));
    std::vector<int32> neighbors;
    if(f > 1)
    {
      neighbors.push_back(static_cast<int32>(f - 1));
    }
    if(f < k_RealFeatureCount)
    {
      neighbors.push_back(static_cast<int32>(f + 1));
    }
    SetNeighbors(td, static_cast<int32>(f), std::move(neighbors));
  }

  (*td.crystalStructures)[0] = 999u;
  (*td.crystalStructures)[1] = static_cast<uint32>(ebsdlib::CrystalStructure::Hexagonal_High);

  return td;
}

// The five-feature chain yields {F1,F2}, {F3,F4}, and {F5} at 10 degrees.
FixtureData Build5FeaturePureBunge()
{
  FixtureData td = CreateScaffold(/*numFeatures=*/6);

  SetAvgQuat(td, 1, QuatFromPhiDeg(0.0f));
  SetAvgQuat(td, 2, QuatFromPhiDeg(5.0f));
  SetAvgQuat(td, 3, QuatFromPhiDeg(60.0f));
  SetAvgQuat(td, 4, QuatFromPhiDeg(63.0f));
  SetAvgQuat(td, 5, QuatFromPhiDeg(25.0f));

  SetNeighbors(td, 1, {2});
  SetNeighbors(td, 2, {1, 3});
  SetNeighbors(td, 3, {2, 4});
  SetNeighbors(td, 4, {3});
  SetNeighbors(td, 5, {});

  return td;
}

Arguments BuildArgs(float32 cAxisToleranceDeg, bool useRunningAverage, bool randomizeParentIds, uint64 seed)
{
  Arguments args;
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_UseNonContiguousNeighbors_Key, std::make_any<bool>(false));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_NonContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(k_NonContigNeighborListPath));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_ContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(k_ContigNeighborListPath));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_UseRunningAverage_Key, std::make_any<bool>(useRunningAverage));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_CAxisTolerance_Key, std::make_any<float32>(cAxisToleranceDeg));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_FeaturePhasesPath));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_VolumesArrayPath_Key, std::make_any<DataPath>(k_VolumesPath));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(k_AvgQuatsPath));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresPath));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_NewCellFeatureAttributeMatrixName_Key, std::make_any<DataPath>(k_NewFeatureAMPath));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_CellParentIdsArrayName_Key, std::make_any<std::string>(k_CellParentIdsName));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_FeatureParentIdsArrayName_Key, std::make_any<std::string>(k_FeatureParentIdsName));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_RandomizeParentIds_Key, std::make_any<bool>(randomizeParentIds));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_UseSeed_Key, std::make_any<bool>(true));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_SeedValue_Key, std::make_any<uint64>(seed));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_SeedArrayName_Key, std::make_any<std::string>(k_SeedArrayName));
  return args;
}
} // namespace AnalyticalFixtures
} // namespace

TEST_CASE("OrientationAnalysis::GroupMicroTextureRegionsFilter: Class 1 Analytical (Pure-Phi Bunge)", "[OrientationAnalysis][GroupMicroTextureRegionsFilter][Class1]")
{
  using namespace AnalyticalFixtures;

  FixtureData td = Build5FeaturePureBunge();

  // Fixed seed and disabled randomization make parent IDs deterministic.
  Arguments args = BuildArgs(/*cAxisToleranceDeg=*/10.0f, /*useRunningAverage=*/false, /*randomizeParentIds=*/false, /*seed=*/42ULL);

  GroupMicroTextureRegionsFilter filter;
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(td.ds.getDataRefAs<Int32Array>(k_FeatureDataPath.createChildPath(k_FeatureParentIdsName)));
  const auto& featureParentIds = td.ds.getDataRefAs<Int32Array>(k_FeatureDataPath.createChildPath(k_FeatureParentIdsName));
  REQUIRE_NOTHROW(td.ds.getDataRefAs<Int32Array>(k_CellDataPath.createChildPath(k_CellParentIdsName)));
  const auto& cellParentIds = td.ds.getDataRefAs<Int32Array>(k_CellDataPath.createChildPath(k_CellParentIdsName));

  // Class 1 checks the three expected parent groups.
  CHECK(featureParentIds[1] == featureParentIds[2]);
  CHECK(featureParentIds[3] == featureParentIds[4]);
  CHECK(featureParentIds[1] != featureParentIds[3]);
  CHECK(featureParentIds[5] != featureParentIds[1]);
  CHECK(featureParentIds[5] != featureParentIds[3]);

  // Class 4 requires positive parent IDs and three groups.
  CHECK(featureParentIds[1] > 0);
  CHECK(featureParentIds[2] > 0);
  CHECK(featureParentIds[3] > 0);
  CHECK(featureParentIds[4] > 0);
  CHECK(featureParentIds[5] > 0);
  std::set<int32> distinctParents{featureParentIds[1], featureParentIds[2], featureParentIds[3], featureParentIds[4], featureParentIds[5]};
  CHECK(distinctParents.size() == 3);

  // Class 4 requires cell and feature parent IDs to agree.
  REQUIRE_NOTHROW(td.ds.getDataRefAs<Int32Array>(k_FeatureIdsPath));
  const auto& featureIds = td.ds.getDataRefAs<Int32Array>(k_FeatureIdsPath);
  for(usize k = 0; k < featureIds.getNumberOfTuples(); k++)
  {
    CHECK(cellParentIds[k] == featureParentIds[featureIds[k]]);
  }

  // The parent matrix includes the reserved zero index.
  const auto& newFeatureAM = td.ds.getDataRefAs<AttributeMatrix>(k_NewFeatureAMPath);
  int32 maxParent = 0;
  for(usize f = 1; f < featureParentIds.getNumberOfTuples(); f++)
  {
    if(featureParentIds[f] > maxParent)
    {
      maxParent = featureParentIds[f];
    }
  }
  CHECK(newFeatureAM.getNumberOfTuples() == static_cast<usize>(maxParent + 1));

  // The parent Attribute Matrix stores the number of microtexture regions. No unused Active array is created.
  CHECK(td.ds.getDataAs<BoolArray>(k_NewFeatureAMPath.createChildPath("Active")) == nullptr);

  REQUIRE_NOTHROW(td.ds.getDataRefAs<UInt64Array>(DataPath({k_SeedArrayName})));
  CHECK(td.ds.getDataRefAs<UInt64Array>(DataPath({k_SeedArrayName}))[0] == 42ULL);

  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::GroupMicroTextureRegionsFilter: RandomizeParentIds invariants", "[OrientationAnalysis][GroupMicroTextureRegionsFilter][Class4]")
{
  // Randomization can relabel parents but must preserve grouping, positivity,
  // cell-parent mapping, and deterministic same-seed output.
  using namespace AnalyticalFixtures;

  FixtureData tdA = Build5FeaturePureBunge();
  Arguments argsA = BuildArgs(10.0f, /*useRunningAverage=*/false, /*randomizeParentIds=*/false, /*seed=*/42ULL);
  GroupMicroTextureRegionsFilter filter;
  {
    auto preflightResult = filter.preflight(tdA.ds, argsA);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(tdA.ds, argsA);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  const auto& parentIdsA = tdA.ds.getDataRefAs<Int32Array>(k_FeatureDataPath.createChildPath(k_FeatureParentIdsName));

  FixtureData tdB = Build5FeaturePureBunge();
  Arguments argsB = BuildArgs(10.0f, /*useRunningAverage=*/false, /*randomizeParentIds=*/true, /*seed=*/42ULL);
  {
    auto preflightResult = filter.preflight(tdB.ds, argsB);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(tdB.ds, argsB);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  const auto& parentIdsB = tdB.ds.getDataRefAs<Int32Array>(k_FeatureDataPath.createChildPath(k_FeatureParentIdsName));
  const auto& cellParentIdsB = tdB.ds.getDataRefAs<Int32Array>(k_CellDataPath.createChildPath(k_CellParentIdsName));
  const auto& featureIdsB = tdB.ds.getDataRefAs<Int32Array>(k_FeatureIdsPath);

  FixtureData tdC = Build5FeaturePureBunge();
  Arguments argsC = BuildArgs(10.0f, /*useRunningAverage=*/false, /*randomizeParentIds=*/true, /*seed=*/42ULL);
  {
    auto preflightResult = filter.preflight(tdC.ds, argsC);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(tdC.ds, argsC);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  const auto& parentIdsC = tdC.ds.getDataRefAs<Int32Array>(k_FeatureDataPath.createChildPath(k_FeatureParentIdsName));

  for(int32 i = 1; i <= 5; i++)
  {
    for(int32 j = i + 1; j <= 5; j++)
    {
      const bool sameInA = parentIdsA[i] == parentIdsA[j];
      const bool sameInB = parentIdsB[i] == parentIdsB[j];
      CHECK(sameInA == sameInB);
    }
  }

  std::set<int32> distinctA{parentIdsA[1], parentIdsA[2], parentIdsA[3], parentIdsA[4], parentIdsA[5]};
  std::set<int32> distinctB{parentIdsB[1], parentIdsB[2], parentIdsB[3], parentIdsB[4], parentIdsB[5]};
  CHECK(distinctA.size() == 3);
  CHECK(distinctB.size() == 3);

  for(usize k = 0; k < featureIdsB.getNumberOfTuples(); k++)
  {
    CHECK(cellParentIdsB[k] == parentIdsB[featureIdsB[k]]);
  }

  for(int32 f = 1; f <= 5; f++)
  {
    CHECK(parentIdsB[f] > 0);
  }

  for(usize f = 0; f < parentIdsB.getNumberOfTuples(); f++)
  {
    CHECK(parentIdsB[f] == parentIdsC[f]);
  }

  // An identity permutation is generally valid. This fixed fixture and seed
  // must change at least one parent label to prove randomization executed.
  bool anyDifferent = false;
  for(int32 f = 1; f <= 5; f++)
  {
    if(parentIdsA[f] != parentIdsB[f])
    {
      anyDifferent = true;
      break;
    }
  }
  CHECK(anyDifferent);

  UnitTest::CheckArraysInheritTupleDims(tdB.ds);
}

TEST_CASE("OrientationAnalysis::GroupMicroTextureRegionsFilter: Class 1 Analytical (Tolerance Boundary)", "[OrientationAnalysis][GroupMicroTextureRegionsFilter][Class1]")
{
  using namespace AnalyticalFixtures;

  // 3 real features on a chain F1 -- F2 -- F3. F1 c-axis at Phi=0, F2 at Phi=8, F3 at Phi=20.
  //
  // With UseRunningAverage=false, determineGrouping() computes the reference c-axis from its
  // referenceFeature argument, which execute() supplies as groupList[j] -- the current BFS frontier
  // feature, NOT the seed and NOT a running average. (The local is named firstFeature, but groupList
  // grows as neighbors are accepted, so it advances past the seed.) Grouping is therefore the
  // transitive closure of the pairwise-tolerance relation along neighbor chains.
  //
  //   - F1 -- F2 : 8 deg,  compared from F1's c-axis -- under 10 deg tolerance -> GROUP
  //   - F2 -- F3 : 12 deg, compared from F2's c-axis -- over 10 deg tolerance  -> DO NOT BRIDGE
  //
  // Expected: 2 distinct groups -> {F1, F2}, {F3}.
  //
  // This expectation is independent of which feature getSeed() happens to pick first (it draws
  // randomly among unparented features). Seeding at F1, F2, or F3 all produce the same partition,
  // because the only bridging edge (F2--F3) exceeds tolerance when evaluated from either end.
  FixtureData td = CreateScaffold(/*numFeatures=*/4);

  SetAvgQuat(td, 1, QuatFromPhiDeg(0.0f));
  SetAvgQuat(td, 2, QuatFromPhiDeg(8.0f));
  SetAvgQuat(td, 3, QuatFromPhiDeg(20.0f));

  SetNeighbors(td, 1, {2});
  SetNeighbors(td, 2, {1, 3});
  SetNeighbors(td, 3, {2});

  Arguments args = BuildArgs(10.0f, /*useRunningAverage=*/false, /*randomizeParentIds=*/false, /*seed=*/42ULL);

  GroupMicroTextureRegionsFilter filter;
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& featureParentIds = td.ds.getDataRefAs<Int32Array>(k_FeatureDataPath.createChildPath(k_FeatureParentIdsName));
  CHECK(featureParentIds[1] == featureParentIds[2]);
  CHECK(featureParentIds[2] != featureParentIds[3]);
  CHECK(featureParentIds[1] > 0);
  CHECK(featureParentIds[3] > 0);
  std::set<int32> distinctParents{featureParentIds[1], featureParentIds[2], featureParentIds[3]};
  CHECK(distinctParents.size() == 2);

  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::GroupMicroTextureRegionsFilter: Class 1 Analytical (Running Average)", "[OrientationAnalysis][GroupMicroTextureRegionsFilter][Class1]")
{
  using namespace AnalyticalFixtures;

  // Three features form the chain F1 -- F2 -- F3, with c-axis tilts of 0, 9, and 18 degrees.
  // Each touching pair is within the 10 degree tolerance, so neighbor-to-neighbor grouping would
  // merge all three. Running-average grouping accepts one adjacent pair, then compares the remaining
  // end member with the pair's 4.5 or 13.5 degree average. That 13.5 degree difference is outside the
  // tolerance. The seed can change which adjacent pair forms, but every seed order produces two groups.
  FixtureData td = CreateScaffold(/*numFeatures=*/4);

  SetAvgQuat(td, 1, QuatFromPhiDeg(0.0f));
  SetAvgQuat(td, 2, QuatFromPhiDeg(9.0f));
  SetAvgQuat(td, 3, QuatFromPhiDeg(18.0f));

  SetNeighbors(td, 1, {2});
  SetNeighbors(td, 2, {1, 3});
  SetNeighbors(td, 3, {2});

  Arguments args = BuildArgs(10.0f, /*useRunningAverage=*/true, /*randomizeParentIds=*/false, /*seed=*/42ULL);

  GroupMicroTextureRegionsFilter filter;
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(td.ds.getDataRefAs<Int32Array>(k_FeatureDataPath.createChildPath(k_FeatureParentIdsName)));
  const auto& featureParentIds = td.ds.getDataRefAs<Int32Array>(k_FeatureDataPath.createChildPath(k_FeatureParentIdsName));

  CHECK(featureParentIds[1] != featureParentIds[3]);
  CHECK((featureParentIds[1] == featureParentIds[2] || featureParentIds[2] == featureParentIds[3]));
  std::set<int32> distinctParents{featureParentIds[1], featureParentIds[2], featureParentIds[3]};
  CHECK(distinctParents.size() == 2);

  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::GroupMicroTextureRegionsFilter: Class 1 Analytical (Mixed-Laue Rejection)", "[OrientationAnalysis][GroupMicroTextureRegionsFilter][Class1][D3]")
{
  using namespace AnalyticalFixtures;

  // Twenty isolated touching pairs reproduce the D3 discriminator. Each pair contains one
  // Cubic_High feature and one Hexagonal_High feature with identical c-axes. The running-average
  // path must reject every pair because both features must resolve to Hexagonal_High. The recorded
  // legacy production comparison accepted 19 of 20 analogous pairs; this deterministic fixture
  // detects the same one-sided-check defect without depending on that run's clock-derived seed.
  constexpr int32 k_NumPairs = 20;
  FixtureData td = CreateScaffold(/*numFeatures=*/2 * k_NumPairs + 1);

  for(int32 pairIdx = 0; pairIdx < k_NumPairs; pairIdx++)
  {
    const int32 cubicFeature = 2 * pairIdx + 1;
    const int32 hexFeature = cubicFeature + 1;
    (*td.featurePhases)[cubicFeature] = 2;
    (*td.featurePhases)[hexFeature] = 1;
    SetNeighbors(td, cubicFeature, {hexFeature});
    SetNeighbors(td, hexFeature, {cubicFeature});
  }

  Arguments args = BuildArgs(10.0f, /*useRunningAverage=*/true, /*randomizeParentIds=*/false, /*seed=*/42ULL);

  GroupMicroTextureRegionsFilter filter;
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(td.ds.getDataRefAs<Int32Array>(k_FeatureDataPath.createChildPath(k_FeatureParentIdsName)));
  const auto& featureParentIds = td.ds.getDataRefAs<Int32Array>(k_FeatureDataPath.createChildPath(k_FeatureParentIdsName));
  for(int32 pairIdx = 0; pairIdx < k_NumPairs; pairIdx++)
  {
    const int32 cubicFeature = 2 * pairIdx + 1;
    const int32 hexFeature = cubicFeature + 1;
    CHECK(featureParentIds[cubicFeature] != featureParentIds[hexFeature]);
  }

  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::GroupMicroTextureRegionsFilter: Non-contiguous neighbor grouping", "[OrientationAnalysis][GroupMicroTextureRegionsFilter][Class1]")
{
  using namespace AnalyticalFixtures;

  // The contiguous lists are empty. F1 and F2 can group only through the optional non-contiguous
  // lists, so the equality assertion depends on the UseNonContiguousNeighbors=true path executing.
  FixtureData td = CreateScaffold(/*numFeatures=*/3);
  SetAvgQuat(td, 1, QuatFromPhiDeg(0.0f));
  SetAvgQuat(td, 2, QuatFromPhiDeg(2.0f));
  SetNonContiguousNeighbors(td, 1, {2});
  SetNonContiguousNeighbors(td, 2, {1});

  Arguments args = BuildArgs(10.0f, /*useRunningAverage=*/false, /*randomizeParentIds=*/false, /*seed=*/42ULL);
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_UseNonContiguousNeighbors_Key, std::make_any<bool>(true));

  GroupMicroTextureRegionsFilter filter;
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(td.ds.getDataRefAs<Int32Array>(k_FeatureDataPath.createChildPath(k_FeatureParentIdsName)));
  const auto& featureParentIds = td.ds.getDataRefAs<Int32Array>(k_FeatureDataPath.createChildPath(k_FeatureParentIdsName));
  CHECK(featureParentIds[1] == featureParentIds[2]);

  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::GroupMicroTextureRegionsFilter: Default contiguous-only neighbor mode", "[OrientationAnalysis][GroupMicroTextureRegionsFilter][Class4]")
{
  // Covers the default configuration: UseNonContiguousNeighbors=false, so only the contiguous
  // neighbor list drives the BFS walk and the non-contiguous list is neither required nor read.
  // This is the mode most users run, and it must preflight and execute cleanly.
  using namespace AnalyticalFixtures;

  FixtureData td = CreateScaffold(/*numFeatures=*/3);
  SetAvgQuat(td, 1, QuatFromPhiDeg(0.0f));
  SetAvgQuat(td, 2, QuatFromPhiDeg(2.0f));
  SetNeighbors(td, 1, {2});
  SetNeighbors(td, 2, {1});

  Arguments args = BuildArgs(10.0f, false, false, 42ULL);

  GroupMicroTextureRegionsFilter filter;
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

TEST_CASE("OrientationAnalysis::GroupMicroTextureRegionsFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][GroupMicroTextureRegionsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "GroupMicroTextureRegionsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "GroupMicroTextureRegionsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<GroupMicroTextureRegionsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(GroupMicroTextureRegionsFilter::k_AvgQuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<float32>(GroupMicroTextureRegionsFilter::k_CAxisTolerance_Key) == 2.5f);
      CHECK(args.value<std::string>(GroupMicroTextureRegionsFilter::k_CellParentIdsArrayName_Key) == "TestName");
      CHECK(args.value<DataPath>(GroupMicroTextureRegionsFilter::k_ContiguousNeighborListArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(GroupMicroTextureRegionsFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(GroupMicroTextureRegionsFilter::k_FeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(GroupMicroTextureRegionsFilter::k_FeatureParentIdsArrayName_Key) == "TestName");
      CHECK(args.value<DataPath>(GroupMicroTextureRegionsFilter::k_FeaturePhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(GroupMicroTextureRegionsFilter::k_NewCellFeatureAttributeMatrixName_Key) == DataPath({"TestName"}));
      CHECK(args.value<DataPath>(GroupMicroTextureRegionsFilter::k_NonContiguousNeighborListArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<bool>(GroupMicroTextureRegionsFilter::k_UseNonContiguousNeighbors_Key) == true);
      CHECK(args.value<bool>(GroupMicroTextureRegionsFilter::k_UseRunningAverage_Key) == true);
      CHECK(args.value<DataPath>(GroupMicroTextureRegionsFilter::k_VolumesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}
