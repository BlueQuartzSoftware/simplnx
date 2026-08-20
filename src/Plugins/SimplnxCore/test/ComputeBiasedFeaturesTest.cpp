#include "SimplnxCore/Filters/ComputeBiasedFeaturesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

/**
 * V&V oracle notes for ComputeBiasedFeaturesFilter (Class 1 hand-derived oracles + Class 4 invariants).
 *
 * Algorithm under test (Algorithms/ComputeBiasedFeatures.cpp), restated from source:
 *
 *  3D path (all three dimensions > 1):
 *    boundBox = {xMin, xMax, yMin, yMax, zMin, zMax} = the full ImageGeom bounds
 *               (origin, origin + dims * spacing) -- reset at the start of every phase iteration.
 *    Shrink loop, features i = 1 .. N-1 in ASCENDING INDEX ORDER (feature 0 is always skipped):
 *      Only surface features (and, when Apply Phase by Phase is on, only features whose phase
 *      equals the current iteration) participate.
 *      coords = {x, x, y, y, z, z}. For each of the six faces f = 0..5:
 *        min faces (f = 0, 2, 4): if coords[f] >  boundBox[f] then dist = coords[f] - boundBox[f]
 *                                 if coords[f] <= boundBox[f] then move = 0
 *        max faces (f = 1, 3, 5): if coords[f] <  boundBox[f] then dist = boundBox[f] - coords[f]
 *                                 if coords[f] >= boundBox[f] then move = 0
 *        otherwise dist = FLT_MAX. The face with the SMALLEST dist wins; the comparison is a
 *        strict '<', so on an exact tie the LOWEST face index wins.
 *      If move == 1 (i.e. the centroid is strictly inside all six faces) the single nearest face
 *      is pulled onto the centroid: boundBox[winner] = coords[winner]. Otherwise the box is
 *      unchanged. The box therefore only ever shrinks, and the result is ORDER DEPENDENT because
 *      shrinking one face changes which face is nearest for a later feature.
 *    Classify loop, features j = 1 .. N-1 (feature 0 always skipped, and when Apply Phase by Phase
 *    is on only features whose phase equals the current iteration):
 *      biased[j] = (x <= xMin) || (x >= xMax) || (y <= yMin) || (y >= yMax) || (z <= zMin) || (z >= zMax)
 *      Note the comparisons are inclusive, so a centroid sitting exactly on a face is biased.
 *
 *  2D path (any one dimension == 1): the flat axis is dropped and the box becomes a 2D rectangle
 *    over the two in-plane axes. The two in-plane centroid components are selected by
 *    centroidShift0/centroidShift1:
 *      X flat -> (Y, Z);  Y flat -> (X, Z);  Z flat -> (X, Y)
 *    The shrink/classify rules are otherwise identical with four faces instead of six. Phases are
 *    ignored entirely on the 2D path (Apply Phase by Phase has no effect).
 *
 * Class 4 invariants asserted for every valid fixture (see CheckInvariants):
 *   I1  biased[0] == false -- feature 0 is the "unassigned" bucket and must never be touched,
 *       even when its centroid or surface flag contain garbage.
 *   I2  every surface feature with index >= 1 that is actually classified (phase >= 1 when
 *       Apply Phase by Phase is on) must be biased. A surface feature either shrinks a face onto
 *       its own centroid (making it inclusive-equal to that face) or already sat on/outside a
 *       face (move == 0). Either way the inclusive classification must flag it.
 *   I3  the box never grows: fixture H pins the box to the exact full geometry bounds when no
 *       feature is a surface feature, which is the floor of the monotone shrink.
 *
 * Findings this file discriminates (see the V&V report for the adjudications):
 *   BF-1 (NX-only port regression) -- the 2D box used spacing[0]/spacing[1] regardless of which
 *        axis was flat, instead of the two in-plane spacings. Killed by fixtures F and G.
 *   BF-2 (bug shared with DREAM3D 6.5.171) -- the 2D classify loop compared the raw X/Y centroid
 *        components against the axis-shifted box. Killed by fixtures F and G (and, on G, by
 *        invariant I2, which the pre-fix code violates).
 *   BF-3 (legacy-only) -- DREAM3D 6.5.171 hardcodes the 2D box origin to 0. SIMPLNX uses the real
 *        origin; fixture E pins the correct origin-aware behaviour.
 */

namespace
{
const std::string k_ImageGeomName = "ImageGeom";
const DataPath k_ImageGeomPath({k_ImageGeomName});

const std::string k_FeatureAMName = "FeatureData";
const DataPath k_FeatureAMPath = k_ImageGeomPath.createChildPath(k_FeatureAMName);

const std::string k_CentroidsName = "Centroids";
const DataPath k_CentroidsPath = k_FeatureAMPath.createChildPath(k_CentroidsName);

const std::string k_SurfaceFeaturesName = "SurfaceFeatures";
const DataPath k_SurfaceFeaturesPath = k_FeatureAMPath.createChildPath(k_SurfaceFeaturesName);

const std::string k_PhasesName = "Phases";
const DataPath k_PhasesPath = k_FeatureAMPath.createChildPath(k_PhasesName);

const std::string k_BiasedFeaturesName = "BiasedFeatures";
const DataPath k_BiasedFeaturesPath = k_FeatureAMPath.createChildPath(k_BiasedFeaturesName);

enum class MaskType
{
  Boolean,
  UInt8
};

/**
 * @brief A completely self-contained ComputeBiasedFeatures input fixture. Every geometry value is
 * dyadic (exactly representable as a float32) so that the inclusive face comparisons in the
 * classify loop are exact and the expected booleans are unambiguous.
 */
struct Fixture
{
  std::string name;
  SizeVec3 dims;
  FloatVec3 origin;
  FloatVec3 spacing;
  std::vector<float32> centroids;     // 3 * numFeatures, interleaved x, y, z
  std::vector<uint8> surfaceFeatures; // 1 per feature
  std::vector<int32> phases;          // 1 per feature
  std::vector<uint8> expectedBiased;  // 1 per feature, hand-derived
  bool calcByPhase = false;
  MaskType maskType = MaskType::Boolean;
  // Invariant I2 ("every classified surface feature is biased") presupposes that a box was actually
  // built, so it does not apply to the degenerate-geometry fixture where neither branch runs.
  bool checkInvariantI2 = true;
};

DataStructure BuildDataStructure(const Fixture& fixture)
{
  DataStructure dataStructure;

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, k_ImageGeomName);
  imageGeomPtr->setDimensions(fixture.dims);
  imageGeomPtr->setOrigin(fixture.origin);
  imageGeomPtr->setSpacing(fixture.spacing);

  const usize numFeatures = fixture.surfaceFeatures.size();
  REQUIRE(fixture.centroids.size() == 3 * numFeatures);
  REQUIRE(fixture.phases.size() == numFeatures);
  REQUIRE(fixture.expectedBiased.size() == numFeatures);

  auto* featureAMPtr = AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{numFeatures}, imageGeomPtr->getId());

  auto* centroidsPtr = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, k_CentroidsName, featureAMPtr->getShape(), ShapeType{3}, featureAMPtr->getId());
  for(usize i = 0; i < fixture.centroids.size(); i++)
  {
    (*centroidsPtr)[i] = fixture.centroids[i];
  }

  if(fixture.maskType == MaskType::Boolean)
  {
    auto* surfacePtr = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, k_SurfaceFeaturesName, featureAMPtr->getShape(), ShapeType{1}, featureAMPtr->getId());
    for(usize i = 0; i < numFeatures; i++)
    {
      (*surfacePtr)[i] = fixture.surfaceFeatures[i] != 0;
    }
  }
  else
  {
    auto* surfacePtr = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, k_SurfaceFeaturesName, featureAMPtr->getShape(), ShapeType{1}, featureAMPtr->getId());
    for(usize i = 0; i < numFeatures; i++)
    {
      (*surfacePtr)[i] = fixture.surfaceFeatures[i];
    }
  }

  auto* phasesPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_PhasesName, featureAMPtr->getShape(), ShapeType{1}, featureAMPtr->getId());
  for(usize i = 0; i < numFeatures; i++)
  {
    (*phasesPtr)[i] = fixture.phases[i];
  }

  return dataStructure;
}

Arguments BuildArguments(const Fixture& fixture)
{
  Arguments args;
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_CalcByPhase_Key, std::make_any<bool>(fixture.calcByPhase));
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_GeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CentroidsPath));
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_SurfaceFeaturesArrayPath_Key, std::make_any<DataPath>(k_SurfaceFeaturesPath));
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_PhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_BiasedFeaturesArrayName_Key, std::make_any<std::string>(k_BiasedFeaturesName));
  return args;
}

/**
 * @brief Asserts the Class 4 invariants I1 and I2 described in the file header. These hold for
 * every valid fixture regardless of geometry, and I2 in particular is the automatic detector for
 * BF-2 on X-normal and Y-normal 2D slabs.
 */
void CheckInvariants(const Fixture& fixture, const BoolArray& biasedFeatures)
{
  // I1: feature 0 is never classified.
  INFO("Invariant I1: biased[0] must always be false");
  REQUIRE(biasedFeatures[0] == false);

  // I2: every classified surface feature must be biased.
  if(!fixture.checkInvariantI2)
  {
    return;
  }
  for(usize i = 1; i < fixture.surfaceFeatures.size(); i++)
  {
    if(fixture.surfaceFeatures[i] == 0)
    {
      continue;
    }
    if(fixture.calcByPhase && fixture.phases[i] < 1)
    {
      // Not classified in any phase iteration, so the invariant does not apply.
      continue;
    }
    CAPTURE(fixture.name, i);
    INFO("Invariant I2: a classified surface feature must be biased");
    REQUIRE(biasedFeatures[i] == true);
  }
}

void RunFixture(const Fixture& fixture)
{
  DataStructure dataStructure = BuildDataStructure(fixture);

  ComputeBiasedFeaturesFilter filter;
  const Arguments args = BuildArguments(fixture);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<BoolArray>(k_BiasedFeaturesPath));
  const auto& biasedFeatures = dataStructure.getDataRefAs<BoolArray>(k_BiasedFeaturesPath);
  REQUIRE(biasedFeatures.getNumberOfTuples() == fixture.expectedBiased.size());

  for(usize i = 0; i < fixture.expectedBiased.size(); i++)
  {
    CAPTURE(fixture.name, i);
    REQUIRE(biasedFeatures[i] == (fixture.expectedBiased[i] != 0));
  }

  CheckInvariants(fixture, biasedFeatures);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

// -----------------------------------------------------------------------------
// Fixture A -- 3D baseline, Apply Phase by Phase off.
//
// Geometry: dims (4, 4, 4), origin (0, 0, 0), spacing (1, 1, 1) -> box [0,4] x [0,4] x [0,4].
//
// Feature 0 is deliberately a garbage "unassigned" record: it is flagged as a surface feature and
// its centroid sits strictly inside the box, so if either loop failed to skip index 0 the box or
// the output would change (see feature 5).
//
//   idx  centroid            surface
//    0   (2.00, 2.00, 0.50)   yes   garbage; must be skipped by BOTH loops
//    1   (0.50, 2.00, 2.00)   yes
//    2   (2.00, 2.00, 2.00)   no
//    3   (3.75, 2.00, 2.00)   yes
//    4   (2.00, 0.50, 2.00)   no
//    5   (2.00, 2.00, 0.50)   no    sentinel: mirrors feature 0's centroid
//
// Shrink trace (box starts {0, 4, 0, 4, 0, 4}):
//   i=1 (0.5, 2, 2): dists = xMin 0.50, xMax 3.50, yMin 2, yMax 2, zMin 2, zMax 2
//                    -> nearest is xMin, move=1 -> box = {0.5, 4, 0, 4, 0, 4}
//   i=2 not a surface feature -> skipped
//   i=3 (3.75, 2, 2): dists = xMin 3.25, xMax 0.25, yMin 2, yMax 2, zMin 2, zMax 2
//                    -> nearest is xMax, move=1 -> box = {0.5, 3.75, 0, 4, 0, 4}
//   i=4, i=5 not surface features -> skipped
//   Final box = {0.5, 3.75, 0, 4, 0, 4}
//
// Classify trace:
//   1 (0.50, 2, 2)    x <= 0.5    -> true
//   2 (2.00, 2, 2)    interior    -> false
//   3 (3.75, 2, 2)    x >= 3.75   -> true
//   4 (2, 0.50, 2)    interior    -> false
//   5 (2, 2, 0.50)    interior    -> false   (would be true if feature 0 had shrunk zMin to 0.5)
//
// Expected: {false, true, false, true, false, false}
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ComputeBiasedFeaturesFilter: Fixture A - 3D baseline", "[SimplnxCore][ComputeBiasedFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  Fixture fixture;
  fixture.name = "A: 3D baseline";
  fixture.dims = SizeVec3{4, 4, 4};
  fixture.origin = FloatVec3{0.0f, 0.0f, 0.0f};
  fixture.spacing = FloatVec3{1.0f, 1.0f, 1.0f};
  fixture.centroids = {2.00f, 2.00f, 0.50f, 0.50f, 2.00f, 2.00f, 2.00f, 2.00f, 2.00f, 3.75f, 2.00f, 2.00f, 2.00f, 0.50f, 2.00f, 2.00f, 2.00f, 0.50f};
  fixture.surfaceFeatures = {1, 1, 0, 1, 0, 0};
  fixture.phases = {1, 1, 1, 1, 1, 1};
  fixture.expectedBiased = {0, 1, 0, 1, 0, 0};
  fixture.calcByPhase = false;

  SECTION("Boolean Surface Features")
  {
    fixture.maskType = MaskType::Boolean;
    RunFixture(fixture);
  }
  SECTION("UInt8 Surface Features")
  {
    // The Surface Features parameter accepts bool or uint8; both must take the same code path.
    fixture.maskType = MaskType::UInt8;
    RunFixture(fixture);
  }
}

// -----------------------------------------------------------------------------
// Fixture B1 -- 3D, surface feature exactly on a face: move == 0, no shrink, still biased.
//
// Geometry: dims (4, 4, 4), origin (0, 0, 0), spacing (1, 1, 1) -> box [0,4]^3.
//
//   idx  centroid            surface
//    0   (2.0, 2.0, 2.0)      no
//    1   (0.0, 2.0, 2.0)      yes   sits exactly on the xMin face
//    2   (2.0, 2.0, 2.0)      no
//
// Shrink trace: i=1 has x == 0 == boundBox[0], so the xMin branch sets move = 0 and no face is
// pulled. Final box = {0, 4, 0, 4, 0, 4} (unchanged).
//
// Classify trace:
//   1 (0, 2, 2)   x <= 0     -> true   (biased WITHOUT having shrunk anything)
//   2 (2, 2, 2)   interior   -> false
//
// Expected: {false, true, false}
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ComputeBiasedFeaturesFilter: Fixture B1 - 3D no-shrink surface feature", "[SimplnxCore][ComputeBiasedFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  Fixture fixture;
  fixture.name = "B1: 3D no-shrink";
  fixture.dims = SizeVec3{4, 4, 4};
  fixture.origin = FloatVec3{0.0f, 0.0f, 0.0f};
  fixture.spacing = FloatVec3{1.0f, 1.0f, 1.0f};
  fixture.centroids = {2.0f, 2.0f, 2.0f, 0.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f};
  fixture.surfaceFeatures = {0, 1, 0};
  fixture.phases = {1, 1, 1};
  fixture.expectedBiased = {0, 1, 0};

  RunFixture(fixture);
}

// -----------------------------------------------------------------------------
// Fixtures B2 / B3 -- 3D greedy order dependence.
//
// The same two surface features processed in the two possible index orders produce DIFFERENT final
// boxes, because pulling one face changes which face is nearest for the feature processed next.
// Both fixtures use dims (4, 4, 4), origin (0, 0, 0), spacing (1, 1, 1) -> box [0,4]^3.
//
//   p = (1.0, 2.0, 2.0) surface
//   q = (2.0, 1.5, 2.0) surface
//   probe = (1.5, 3.0, 2.0) NOT a surface feature
//
// B2, order p then q:
//   i=1 p: dists = xMin 1.0, xMax 3.0, yMin 2.0, yMax 2.0, zMin 2.0, zMax 2.0
//          -> xMin wins -> box = {1.0, 4, 0, 4, 0, 4}
//   i=2 q: dists = xMin 1.0, xMax 2.0, yMin 1.5, yMax 2.5, zMin 2.0, zMax 2.0
//          -> xMin wins again (1.0 < 1.5) -> box = {2.0, 4, 0, 4, 0, 4}
//   Classify: p x=1.0 <= 2.0 -> true; q x=2.0 <= 2.0 -> true; probe x=1.5 <= 2.0 -> true
//   Expected: {false, true, true, true}
//
// B3, order q then p:
//   i=1 q: dists = xMin 2.0, xMax 2.0, yMin 1.5, yMax 2.5, zMin 2.0, zMax 2.0
//          -> yMin wins -> box = {0, 4, 1.5, 4, 0, 4}
//   i=2 p: dists = xMin 1.0, xMax 3.0, yMin 0.5, yMax 2.0, zMin 2.0, zMax 2.0
//          -> yMin wins (0.5 < 1.0) -> box = {0, 4, 2.0, 4, 0, 4}
//   Classify: q y=1.5 <= 2.0 -> true; p y=2.0 <= 2.0 -> true;
//             probe (1.5, 3.0, 2.0) interior on every face -> false
//   Expected: {false, true, true, false}
//
// The probe flips between the two orders, which is the observable proof of order dependence.
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ComputeBiasedFeaturesFilter: Fixtures B2/B3 - 3D greedy order dependence", "[SimplnxCore][ComputeBiasedFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  Fixture fixture;
  fixture.dims = SizeVec3{4, 4, 4};
  fixture.origin = FloatVec3{0.0f, 0.0f, 0.0f};
  fixture.spacing = FloatVec3{1.0f, 1.0f, 1.0f};
  fixture.surfaceFeatures = {0, 1, 1, 0};
  fixture.phases = {1, 1, 1, 1};

  SECTION("B2: p then q")
  {
    fixture.name = "B2: order p,q";
    fixture.centroids = {2.0f, 2.0f, 2.0f, 1.0f, 2.0f, 2.0f, 2.0f, 1.5f, 2.0f, 1.5f, 3.0f, 2.0f};
    fixture.expectedBiased = {0, 1, 1, 1};
    RunFixture(fixture);
  }
  SECTION("B3: q then p")
  {
    fixture.name = "B3: order q,p";
    fixture.centroids = {2.0f, 2.0f, 2.0f, 2.0f, 1.5f, 2.0f, 1.0f, 2.0f, 2.0f, 1.5f, 3.0f, 2.0f};
    fixture.expectedBiased = {0, 1, 1, 0};
    RunFixture(fixture);
  }
}

// -----------------------------------------------------------------------------
// Fixture C -- 3D with a non-zero origin and anisotropic spacing.
//
// Geometry: dims (2, 3, 4), origin (10, 20, 30), spacing (2, 0.5, 4)
//   -> box = [10, 10 + 2*2] x [20, 20 + 3*0.5] x [30, 30 + 4*4]
//          = [10, 14] x [20, 21.5] x [30, 46]
//
//   idx  centroid                 surface
//    0   (0.00,  0.00,  0.00)      no    garbage: far outside the geometry, must be skipped
//    1   (11.00, 21.00, 38.00)     yes
//    2   (10.25, 20.50, 34.00)     yes
//    3   (12.00, 20.50, 38.00)     no
//    4   (12.00, 21.00, 38.00)     no
//
// Shrink trace (box starts {10, 14, 20, 21.5, 30, 46}):
//   i=1 (11, 21, 38): dists = xMin 1.00, xMax 3.00, yMin 1.00, yMax 0.50, zMin 8, zMax 8
//                     -> yMax wins -> box = {10, 14, 20, 21.0, 30, 46}
//   i=2 (10.25, 20.5, 34): dists = xMin 0.25, xMax 3.75, yMin 0.50, yMax 0.50, zMin 4, zMax 12
//                     -> xMin wins -> box = {10.25, 14, 20, 21.0, 30, 46}
//   Final box = {10.25, 14, 20, 21.0, 30, 46}
//
// Classify trace:
//   1 (11, 21, 38)      y >= 21.0     -> true
//   2 (10.25, 20.5, 34) x <= 10.25    -> true
//   3 (12, 20.5, 38)    interior      -> false
//   4 (12, 21, 38)      y >= 21.0     -> true   (a non-surface feature biased by a SHRUNK face)
//
// Expected: {false, true, true, false, true}
//
// Feature 0's centroid (0, 0, 0) is below every min face, so it also pins invariant I1: if the
// classify loop started at index 0 it would be flagged biased.
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ComputeBiasedFeaturesFilter: Fixture C - 3D origin and anisotropic spacing", "[SimplnxCore][ComputeBiasedFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  Fixture fixture;
  fixture.name = "C: 3D origin + anisotropy";
  fixture.dims = SizeVec3{2, 3, 4};
  fixture.origin = FloatVec3{10.0f, 20.0f, 30.0f};
  fixture.spacing = FloatVec3{2.0f, 0.5f, 4.0f};
  fixture.centroids = {0.0f, 0.0f, 0.0f, 11.0f, 21.0f, 38.0f, 10.25f, 20.5f, 34.0f, 12.0f, 20.5f, 38.0f, 12.0f, 21.0f, 38.0f};
  fixture.surfaceFeatures = {0, 1, 1, 0, 0};
  fixture.phases = {1, 1, 1, 1, 1};
  fixture.expectedBiased = {0, 1, 1, 0, 1};

  RunFixture(fixture);
}

// -----------------------------------------------------------------------------
// Fixture D -- 3D with Apply Phase by Phase on.
//
// Geometry: dims (4, 4, 4), origin (0, 0, 0), spacing (1, 1, 1) -> box [0,4]^3 (reset per phase).
//
//   idx  centroid            surface  phase
//    0   (2.00, 2.0, 2.0)     no        5    garbage phase; pins the numPhases quirk (below)
//    1   (0.50, 2.0, 2.0)     yes       1
//    2   (2.00, 2.0, 2.0)     no        1
//    3   (3.75, 2.0, 2.0)     yes       2    positioned so it does NOT shrink phase 1's box
//    4   (2.00, 2.0, 2.0)     no        2
//    5   (0.00, 2.0, 2.0)     yes       0    phase 0 is never iterated -> never classified
//
// SIMPLNX computes numPhases with std::max_element over the WHOLE Phases array, so index 0's
// garbage value of 5 is included and the phase loop runs iter = 1..5. DREAM3D 6.5.171 starts its
// max scan at index 1 and runs iter = 1..2. Iterations 3, 4 and 5 match no feature in either the
// shrink loop or the classify loop, so both loops are no-ops and the quirk is benign -- this
// fixture proves that rather than assuming it.
//
//   iter 1: box {0,4,0,4,0,4}; surface+phase1 = {1} -> nearest face xMin (0.50) -> box[0] = 0.5
//           classify phase1: 1 -> x <= 0.5 -> true;  2 -> interior -> false
//   iter 2: box reset {0,4,0,4,0,4}; surface+phase2 = {3} -> nearest face xMax (0.25) -> box[1] = 3.75
//           classify phase2: 3 -> x >= 3.75 -> true;  4 -> interior -> false
//   iter 3..5: no features match -> no writes
//   feature 5 (phase 0): never classified -> stays false from the initial fill
//
// Expected: {false, true, false, true, false, false}
//
// Feature 5 is also the phase-gate killer: with the phase gate removed, phase 1's box would shrink
// to {0.5, 3.75, ...} and feature 5 (x = 0) would be classified as biased.
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ComputeBiasedFeaturesFilter: Fixture D - 3D apply phase by phase", "[SimplnxCore][ComputeBiasedFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  Fixture fixture;
  fixture.name = "D: 3D CalcByPhase";
  fixture.dims = SizeVec3{4, 4, 4};
  fixture.origin = FloatVec3{0.0f, 0.0f, 0.0f};
  fixture.spacing = FloatVec3{1.0f, 1.0f, 1.0f};
  fixture.centroids = {2.00f, 2.0f, 2.0f, 0.50f, 2.0f, 2.0f, 2.00f, 2.0f, 2.0f, 3.75f, 2.0f, 2.0f, 2.00f, 2.0f, 2.0f, 0.00f, 2.0f, 2.0f};
  fixture.surfaceFeatures = {0, 1, 0, 1, 0, 1};
  fixture.phases = {5, 1, 1, 2, 2, 0};
  fixture.expectedBiased = {0, 1, 0, 1, 0, 0};
  fixture.calcByPhase = true;

  RunFixture(fixture);
}

// -----------------------------------------------------------------------------
// Fixture E -- 2D Z-normal slab with a NON-ZERO origin and anisotropic in-plane spacing.
//
// Geometry: dims (4, 5, 1), origin (4, 8, 0), spacing (2, 0.5, 8).
// The flat axis is Z, so the in-plane axes are (X, Y) and the box is
//   [4, 4 + 4*2] x [8, 8 + 5*0.5] = [4, 12] x [8, 10.5]
// All centroids use z = 4.0 (the mid-plane of the single 8-thick layer); the Z component is
// irrelevant on the 2D path.
//
//   idx  centroid                 surface
//    0   (0.00,  0.00, 0.0)        no    garbage; must be skipped
//    1   (4.50,  9.00, 4.0)        yes
//    2   (11.50, 9.00, 4.0)        yes
//    3   (8.00,  8.25, 4.0)        yes
//    4   (8.00,  9.00, 4.0)        no
//
// Shrink trace (box starts {4, 12, 8, 10.5}):
//   i=1 (4.5, 9.0):   dists = xMin 0.50, xMax 7.50, yMin 1.00, yMax 1.50 -> xMin -> box[0] = 4.5
//   i=2 (11.5, 9.0):  dists = xMin 7.00, xMax 0.50, yMin 1.00, yMax 1.50 -> xMax -> box[1] = 11.5
//   i=3 (8.0, 8.25):  dists = xMin 3.50, xMax 3.50, yMin 0.25, yMax 2.25 -> yMin -> box[2] = 8.25
//   Final box = {4.5, 11.5, 8.25, 10.5}
//
// Classify trace:
//   1 (4.50,  9.00)  x <= 4.50   -> true
//   2 (11.50, 9.00)  x >= 11.50  -> true
//   3 (8.00,  8.25)  y <= 8.25   -> true
//   4 (8.00,  9.00)  interior    -> false
//
// Expected: {false, true, true, true, false}
//
// This is the BF-3 discriminator: DREAM3D 6.5.171 hardcodes the 2D box origin to (0, 0), giving
// [0, 8] x [0, 2.5] and flagging feature 4 as biased. SIMPLNX honours the real origin, which is
// the geometrically correct behaviour, so the legacy A/B is expected to diverge here.
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ComputeBiasedFeaturesFilter: Fixture E - 2D Z-normal with origin", "[SimplnxCore][ComputeBiasedFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  Fixture fixture;
  fixture.name = "E: 2D Z-normal, origin (4,8,0)";
  fixture.dims = SizeVec3{4, 5, 1};
  fixture.origin = FloatVec3{4.0f, 8.0f, 0.0f};
  fixture.spacing = FloatVec3{2.0f, 0.5f, 8.0f};
  fixture.centroids = {0.0f, 0.0f, 0.0f, 4.5f, 9.0f, 4.0f, 11.5f, 9.0f, 4.0f, 8.0f, 8.25f, 4.0f, 8.0f, 9.0f, 4.0f};
  fixture.surfaceFeatures = {0, 1, 1, 1, 0};
  fixture.phases = {1, 1, 1, 1, 1};
  fixture.expectedBiased = {0, 1, 1, 1, 0};

  RunFixture(fixture);
}

// -----------------------------------------------------------------------------
// Fixture E0 -- 2D Z-normal slab at the origin. Apply Phase by Phase is toggled on to prove that
// the 2D path ignores phases entirely (the algorithm's 2D branch has no phase handling at all).
//
// Geometry: dims (4, 5, 1), origin (0, 0, 0), spacing (2, 0.5, 8) -> box [0, 8] x [0, 2.5].
//
//   idx  centroid              surface  phase
//    0   (0.00, 0.00, 4.0)      no        3
//    1   (0.50, 1.00, 4.0)      yes       1
//    2   (7.50, 1.00, 4.0)      yes       2
//    3   (4.00, 0.25, 4.0)      yes       1
//    4   (4.00, 1.00, 4.0)      no        2
//    5   (0.00, 1.00, 4.0)      yes       1   sits outside the already-shrunk xMin face
//
// Shrink trace (box starts {0, 8, 0, 2.5}), phases ignored:
//   i=1 (0.5, 1.0):  dists = xMin 0.50, xMax 7.50, yMin 1.00, yMax 1.50 -> xMin -> box[0] = 0.5
//   i=2 (7.5, 1.0):  dists = xMin 7.00, xMax 0.50, yMin 1.00, yMax 1.50 -> xMax -> box[1] = 7.5
//   i=3 (4.0, 0.25): dists = xMin 3.50, xMax 3.50, yMin 0.25, yMax 2.25 -> yMin -> box[2] = 0.25
//   i=5 (0.0, 1.0):  x == 0.0 <= xMin == 0.5, so move = 0 and nothing is pulled -- this is the 2D
//                    counterpart of fixture B1's no-shrink case
//   Final box = {0.5, 7.5, 0.25, 2.5}
//
// Classify trace (all features, no phase gate):
//   1 (0.50, 1.00)  x <= 0.50   -> true
//   2 (7.50, 1.00)  x >= 7.50   -> true
//   3 (4.00, 0.25)  y <= 0.25   -> true
//   4 (4.00, 1.00)  interior    -> false
//   5 (0.00, 1.00)  x <= 0.50   -> true
//
// Expected: {false, true, true, true, false, true}
//
// This is the clean 2D A/B match case: Z-normal, zero origin, so BF-1, BF-2 and BF-3 are all inert
// and SIMPLNX and DREAM3D 6.5.171 must agree exactly.
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ComputeBiasedFeaturesFilter: Fixture E0 - 2D Z-normal at origin ignores phases", "[SimplnxCore][ComputeBiasedFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  Fixture fixture;
  fixture.name = "E0: 2D Z-normal at origin";
  fixture.dims = SizeVec3{4, 5, 1};
  fixture.origin = FloatVec3{0.0f, 0.0f, 0.0f};
  fixture.spacing = FloatVec3{2.0f, 0.5f, 8.0f};
  fixture.centroids = {0.0f, 0.0f, 4.0f, 0.5f, 1.0f, 4.0f, 7.5f, 1.0f, 4.0f, 4.0f, 0.25f, 4.0f, 4.0f, 1.0f, 4.0f, 0.0f, 1.0f, 4.0f};
  fixture.surfaceFeatures = {0, 1, 1, 1, 0, 1};
  fixture.phases = {3, 1, 2, 1, 2, 1};
  fixture.expectedBiased = {0, 1, 1, 1, 0, 1};
  fixture.calcByPhase = true;

  RunFixture(fixture);
}

// -----------------------------------------------------------------------------
// Fixture F -- 2D X-normal slab, anisotropic spacing. RED evidence for BF-1 and BF-2.
//
// Geometry: dims (1, 5, 6), origin (0, 0, 0), spacing (1, 2, 3).
// The flat axis is X, so the in-plane axes are (Y, Z) and the geometrically correct box is
//   [0, 0 + 5*2] x [0, 0 + 6*3] = [0, 10] x [0, 18]
// Every centroid has x = 0.5 (the mid-plane of the single 1-thick layer).
//
//   idx  centroid              surface
//    0   (0.5, 0.0,  0.0)       no    garbage; must be skipped
//    1   (0.5, 1.0,  9.0)       yes
//    2   (0.5, 5.0, 16.0)       yes
//    3   (0.5, 9.5,  8.0)       yes
//    4   (0.5, 5.0,  8.0)       no    interior probe
//    5   (0.5, 9.5,  4.0)       no    probe on the shrunk yMax face
//    6   (0.5, 5.0, 16.0)       no    probe on the shrunk zMax face
//
// Shrink trace over (Y, Z), box starts {0, 10, 0, 18}:
//   i=1 (1.0, 9.0):  dists = yMin 1.00, yMax 9.00, zMin 9.00, zMax 9.00 -> yMin -> box[0] = 1.0
//   i=2 (5.0, 16.0): dists = yMin 4.00, yMax 5.00, zMin 16.00, zMax 2.00 -> zMax -> box[3] = 16.0
//   i=3 (9.5, 8.0):  dists = yMin 8.50, yMax 0.50, zMin 8.00, zMax 8.00 -> yMax -> box[1] = 9.5
//   Final box = {1.0, 9.5, 0, 16.0}
//
// Classify trace over (Y, Z):
//   1 (1.0,  9.0)  y <= 1.0    -> true
//   2 (5.0, 16.0)  z >= 16.0   -> true
//   3 (9.5,  8.0)  y >= 9.5    -> true
//   4 (5.0,  8.0)  interior    -> false
//   5 (9.5,  4.0)  y >= 9.5    -> true
//   6 (5.0, 16.0)  z >= 16.0   -> true
//
// Expected: {false, true, true, true, false, true, true}
//
// Pre-fix behaviour, derived from source: BF-1 built the box from spacing[0]/spacing[1], giving
// {0, 5, 0, 12} instead of {0, 10, 0, 18}; the shrink loop then only managed box[0] = 1.0 (i=2 and
// i=3 both fell outside the undersized box and set move = 0). BF-2 then classified the raw X and Y
// components, and since every x = 0.5 <= 1.0 every feature came out biased, i.e.
// {false, true, true, true, TRUE, true, true}. EXECUTED: against the pre-fix code this test failed
// at index 4 with "true == false", which is the first index the derivation predicts.
//
// DREAM3D 6.5.171 gets the box right (it does remap the spacing per axis) but has the same BF-2
// classification bug, so the legacy A/B on this fixture EXECUTED as {0,1,1,1,1,1,1} -- also
// differing from the oracle at index 4, for a different underlying reason.
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ComputeBiasedFeaturesFilter: Fixture F - 2D X-normal anisotropic", "[SimplnxCore][ComputeBiasedFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  Fixture fixture;
  fixture.name = "F: 2D X-normal, spacing (1,2,3)";
  fixture.dims = SizeVec3{1, 5, 6};
  fixture.origin = FloatVec3{0.0f, 0.0f, 0.0f};
  fixture.spacing = FloatVec3{1.0f, 2.0f, 3.0f};
  fixture.centroids = {0.5f, 0.0f, 0.0f, 0.5f, 1.0f, 9.0f, 0.5f, 5.0f, 16.0f, 0.5f, 9.5f, 8.0f, 0.5f, 5.0f, 8.0f, 0.5f, 9.5f, 4.0f, 0.5f, 5.0f, 16.0f};
  fixture.surfaceFeatures = {0, 1, 1, 1, 0, 0, 0};
  fixture.phases = {1, 1, 1, 1, 1, 1, 1};
  fixture.expectedBiased = {0, 1, 1, 1, 0, 1, 1};

  RunFixture(fixture);
}

// -----------------------------------------------------------------------------
// Fixture G -- 2D Y-normal slab, anisotropic spacing. RED evidence for BF-1 and BF-2, and the case
// where the pre-fix code VIOLATES invariant I2 (surface features come back unbiased).
//
// Geometry: dims (5, 1, 6), origin (0, 0, 0), spacing (2, 1, 3).
// The flat axis is Y, so the in-plane axes are (X, Z) and the geometrically correct box is
//   [0, 0 + 5*2] x [0, 0 + 6*3] = [0, 10] x [0, 18]
// Every centroid has y = 0.5 (the mid-plane of the single 1-thick layer).
//
//   idx  centroid              surface
//    0   (0.0, 0.5,  0.0)       no    garbage; must be skipped
//    1   (1.0, 0.5,  9.0)       yes
//    2   (5.0, 0.5, 16.0)       yes
//    3   (9.5, 0.5,  8.0)       yes
//    4   (5.0, 0.5,  8.0)       no    interior probe
//    5   (5.0, 0.5, 16.0)       no    probe on the shrunk zMax face
//
// Shrink trace over (X, Z), box starts {0, 10, 0, 18}:
//   i=1 (1.0, 9.0):  dists = xMin 1.00, xMax 9.00, zMin 9.00, zMax 9.00 -> xMin -> box[0] = 1.0
//   i=2 (5.0, 16.0): dists = xMin 4.00, xMax 5.00, zMin 16.00, zMax 2.00 -> zMax -> box[3] = 16.0
//   i=3 (9.5, 8.0):  dists = xMin 8.50, xMax 0.50, zMin 8.00, zMax 8.00 -> xMax -> box[1] = 9.5
//   Final box = {1.0, 9.5, 0, 16.0}
//
// Classify trace over (X, Z):
//   1 (1.0,  9.0)  x <= 1.0    -> true
//   2 (5.0, 16.0)  z >= 16.0   -> true
//   3 (9.5,  8.0)  x >= 9.5    -> true
//   4 (5.0,  8.0)  interior    -> false
//   5 (5.0, 16.0)  z >= 16.0   -> true
//
// Expected: {false, true, true, true, false, true}
//
// Pre-fix behaviour, derived from source: BF-1 built the box as {0, 10, 0, 6} (spacing[1] = 1 used
// for the second in-plane axis instead of spacing[2] = 3), so every surface feature's Z exceeded
// the 6 ceiling, move was forced to 0 for all three, and the box never shrank at all. BF-2 then
// classified X (correct here by luck) and Y (always 0.5, strictly inside {0, 6}), so NOTHING was
// flagged, i.e. {false, false, false, false, false, false} -- which violates invariant I2 for
// indices 1, 2 and 3. EXECUTED: against the pre-fix code this test failed at index 1 with
// "false == true", the first index the derivation predicts.
//
// DREAM3D 6.5.171 remaps the spacing correctly, so BF-1 is SIMPLNX-only; but 6.5.171 still has the
// BF-2 classification bug, so the legacy A/B on this fixture EXECUTED as {0,1,0,1,0,0}, differing
// from the oracle at indices 2 and 5 and violating invariant I2 at index 2.
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ComputeBiasedFeaturesFilter: Fixture G - 2D Y-normal anisotropic", "[SimplnxCore][ComputeBiasedFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  Fixture fixture;
  fixture.name = "G: 2D Y-normal, spacing (2,1,3)";
  fixture.dims = SizeVec3{5, 1, 6};
  fixture.origin = FloatVec3{0.0f, 0.0f, 0.0f};
  fixture.spacing = FloatVec3{2.0f, 1.0f, 3.0f};
  fixture.centroids = {0.0f, 0.5f, 0.0f, 1.0f, 0.5f, 9.0f, 5.0f, 0.5f, 16.0f, 9.5f, 0.5f, 8.0f, 5.0f, 0.5f, 8.0f, 5.0f, 0.5f, 16.0f};
  fixture.surfaceFeatures = {0, 1, 1, 1, 0, 0};
  fixture.phases = {1, 1, 1, 1, 1, 1};
  fixture.expectedBiased = {0, 1, 1, 1, 0, 1};

  RunFixture(fixture);
}

// -----------------------------------------------------------------------------
// Fixture H -- invariant I3: with no surface features the box is exactly the full geometry bounds.
//
// Geometry: dims (4, 4, 4), origin (0, 0, 0), spacing (1, 1, 1) -> box [0,4]^3.
// No feature is a surface feature, so the shrink loop never runs and the box stays at the full
// bounds. One probe sits exactly on each of the six faces and one sits at the centre, which pins
// all six faces of the initial box simultaneously and is the floor of the monotone shrink.
//
//   idx  centroid          expected
//    0   (2, 2, 2)          false   (feature 0 is never classified)
//    1   (0, 2, 2)          true    x <= xMin = 0
//    2   (4, 2, 2)          true    x >= xMax = 4
//    3   (2, 0, 2)          true    y <= yMin = 0
//    4   (2, 4, 2)          true    y >= yMax = 4
//    5   (2, 2, 0)          true    z <= zMin = 0
//    6   (2, 2, 4)          true    z >= zMax = 4
//    7   (2, 2, 2)          false   interior
//
// Expected: {false, true, true, true, true, true, true, false}
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ComputeBiasedFeaturesFilter: Fixture H - box floor with no surface features", "[SimplnxCore][ComputeBiasedFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  Fixture fixture;
  fixture.name = "H: box floor";
  fixture.dims = SizeVec3{4, 4, 4};
  fixture.origin = FloatVec3{0.0f, 0.0f, 0.0f};
  fixture.spacing = FloatVec3{1.0f, 1.0f, 1.0f};
  fixture.centroids = {2.0f, 2.0f, 2.0f, 0.0f, 2.0f, 2.0f, 4.0f, 2.0f, 2.0f, 2.0f, 0.0f, 2.0f, 2.0f, 4.0f, 2.0f, 2.0f, 2.0f, 0.0f, 2.0f, 2.0f, 4.0f, 2.0f, 2.0f, 2.0f};
  fixture.surfaceFeatures = {0, 0, 0, 0, 0, 0, 0, 0};
  fixture.phases = {1, 1, 1, 1, 1, 1, 1, 1};
  fixture.expectedBiased = {0, 1, 1, 1, 1, 1, 1, 0};

  RunFixture(fixture);
}

// -----------------------------------------------------------------------------
// Fixture I -- degenerate geometry: neither the 3D nor the 2D branch runs.
//
// operator() dispatches to the 3D path only when every dimension is > 1, and to the 2D path only
// when some dimension is exactly 1. A dimension of 0 satisfies neither test, so BOTH branches are
// skipped and the algorithm returns success without writing anything. Since both branches begin
// with their own biasedFeaturesStore.fill(false), that fill is also skipped -- so the output array
// must be initialized by the preflight CreateArrayAction rather than by the algorithm, otherwise
// the filter hands back a bool array full of indeterminate values (DataStore allocates with
// `new value_type[n]`, which default-initializes and therefore does not zero bool).
//
// DREAM3D 6.5.171 has the same dispatch fall-through, but its dataCheck created BiasedFeatures via
// createNonPrereqArrayFromPath<..., bool>(this, tempPath, false, cDims) -- an explicit false init
// value -- so its output was well defined. This fixture pins the same guarantee for SIMPLNX.
//
// Expected: the filter succeeds and every value, including index 0, is false.
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ComputeBiasedFeaturesFilter: Fixture I - degenerate geometry yields an initialized false output", "[SimplnxCore][ComputeBiasedFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  Fixture fixture;
  fixture.name = "I: degenerate geometry (0, 4, 4)";
  fixture.dims = SizeVec3{0, 4, 4};
  fixture.origin = FloatVec3{0.0f, 0.0f, 0.0f};
  fixture.spacing = FloatVec3{1.0f, 1.0f, 1.0f};
  fixture.centroids = {2.0f, 2.0f, 2.0f, 0.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 4.0f, 2.0f, 2.0f};
  fixture.surfaceFeatures = {0, 1, 0, 1};
  fixture.phases = {1, 1, 1, 1};
  fixture.expectedBiased = {0, 0, 0, 0};
  fixture.checkInvariantI2 = false;

  RunFixture(fixture);
}

// -----------------------------------------------------------------------------
// Error handling
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ComputeBiasedFeaturesFilter: Invalid filter execution", "[SimplnxCore][ComputeBiasedFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  const std::string k_OtherAMName = "OtherFeatureData";
  const DataPath k_OtherAMPath = k_ImageGeomPath.createChildPath(k_OtherAMName);

  DataStructure dataStructure;
  {
    auto* imageGeomPtr = ImageGeom::Create(dataStructure, k_ImageGeomName);
    imageGeomPtr->setDimensions(SizeVec3{4, 4, 4});
    imageGeomPtr->setOrigin(FloatVec3{0.0f, 0.0f, 0.0f});
    imageGeomPtr->setSpacing(FloatVec3{1.0f, 1.0f, 1.0f});

    // The main feature AttributeMatrix has 5 tuples.
    auto* featureAMPtr = AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{5}, imageGeomPtr->getId());
    Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, k_CentroidsName, featureAMPtr->getShape(), ShapeType{3}, featureAMPtr->getId());
    BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, k_SurfaceFeaturesName, featureAMPtr->getShape(), ShapeType{1}, featureAMPtr->getId());
    Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_PhasesName, featureAMPtr->getShape(), ShapeType{1}, featureAMPtr->getId());

    // A second AttributeMatrix with a DIFFERENT tuple count, used to trigger the tuple check, plus
    // a float32 array used to prove the Surface Features type restriction.
    auto* otherAMPtr = AttributeMatrix::Create(dataStructure, k_OtherAMName, ShapeType{4}, imageGeomPtr->getId());
    BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, k_SurfaceFeaturesName, otherAMPtr->getShape(), ShapeType{1}, otherAMPtr->getId());
    Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "FloatMask", otherAMPtr->getShape(), ShapeType{1}, otherAMPtr->getId());
  }

  ComputeBiasedFeaturesFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_CalcByPhase_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_GeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CentroidsPath));
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_PhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_BiasedFeaturesArrayName_Key, std::make_any<std::string>(k_BiasedFeaturesName));

  SECTION("Mismatched tuple counts between Centroids and Surface Features")
  {
    args.insertOrAssign(ComputeBiasedFeaturesFilter::k_SurfaceFeaturesArrayPath_Key, std::make_any<DataPath>(k_OtherAMPath.createChildPath(k_SurfaceFeaturesName)));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  }
  SECTION("Surface Features array of a disallowed type")
  {
    args.insertOrAssign(ComputeBiasedFeaturesFilter::k_SurfaceFeaturesArrayPath_Key, std::make_any<DataPath>(k_OtherAMPath.createChildPath("FloatMask")));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
// Zero-tuple Feature data. dataStructure.validateNumberOfTuples() only checks that the selected
// arrays AGREE, so a Feature AttributeMatrix with zero tuples satisfies it: Centroids,
// SurfaceFeatures and Phases all report 0 and the check passes. With Apply Phase by Phase on, the
// algorithm then evaluates
//
//   numPhases = *std::max_element(phasesStorePtr->begin(), phasesStorePtr->end());
//
// on an empty range. std::max_element returns `end()` for an empty range, so the dereference reads
// one past the last element -- undefined behavior, and the phase-loop bound is whatever that read
// produced. A Feature array is also expected to carry at least the index-0 "unassigned" tuple, so
// zero tuples is malformed input in its own right on both the 2D and 3D paths.
//
// The preflight guard rejects the input before any of that. This TEST_CASE is the guard's RED:
// against the pre-guard code preflight returned VALID here.
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ComputeBiasedFeaturesFilter: Zero-tuple Feature arrays are rejected in preflight", "[SimplnxCore][ComputeBiasedFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  {
    auto* imageGeomPtr = ImageGeom::Create(dataStructure, k_ImageGeomName);
    imageGeomPtr->setDimensions(SizeVec3{4, 4, 4});
    imageGeomPtr->setOrigin(FloatVec3{0.0f, 0.0f, 0.0f});
    imageGeomPtr->setSpacing(FloatVec3{1.0f, 1.0f, 1.0f});

    // An empty Feature AttributeMatrix: every array agrees at zero tuples, so the tuple-equality
    // check is satisfied and only an explicit count check can reject this.
    auto* featureAMPtr = AttributeMatrix::Create(dataStructure, k_FeatureAMName, ShapeType{0}, imageGeomPtr->getId());
    Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, k_CentroidsName, featureAMPtr->getShape(), ShapeType{3}, featureAMPtr->getId());
    BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, k_SurfaceFeaturesName, featureAMPtr->getShape(), ShapeType{1}, featureAMPtr->getId());
    Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_PhasesName, featureAMPtr->getShape(), ShapeType{1}, featureAMPtr->getId());
  }

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_CentroidsPath));
  REQUIRE(dataStructure.getDataRefAs<Float32Array>(k_CentroidsPath).getNumberOfTuples() == 0);

  ComputeBiasedFeaturesFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_GeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CentroidsPath));
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_SurfaceFeaturesArrayPath_Key, std::make_any<DataPath>(k_SurfaceFeaturesPath));
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_PhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
  args.insertOrAssign(ComputeBiasedFeaturesFilter::k_BiasedFeaturesArrayName_Key, std::make_any<std::string>(k_BiasedFeaturesName));

  SECTION("Apply Phase by Phase on - the max_element dereference is the undefined read")
  {
    args.insertOrAssign(ComputeBiasedFeaturesFilter::k_CalcByPhase_Key, std::make_any<bool>(true));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  }
  SECTION("Apply Phase by Phase off - zero-tuple Feature data is still malformed")
  {
    args.insertOrAssign(ComputeBiasedFeaturesFilter::k_CalcByPhase_Key, std::make_any<bool>(false));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeBiasedFeaturesFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeBiasedFeaturesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeBiasedFeaturesFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeBiasedFeaturesFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeBiasedFeaturesFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<bool>(ComputeBiasedFeaturesFilter::k_CalcByPhase_Key) == true);
      CHECK(args.value<DataPath>(ComputeBiasedFeaturesFilter::k_GeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ComputeBiasedFeaturesFilter::k_CentroidsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeBiasedFeaturesFilter::k_SurfaceFeaturesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeBiasedFeaturesFilter::k_PhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeBiasedFeaturesFilter::k_BiasedFeaturesArrayName_Key) == "TestName");
    }
  }
}
