#include "SimplnxCore/Filters/ComputeSurfaceAreaToVolumeFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <cmath>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::UnitTest;

// =============================================================================
// V&V Class 1 (Analytical) oracle support — added 2026-08-20.
//
// ORACLE, derived by hand from the definition of the quantities and NOT from any
// DREAM3D/SIMPLNX source or output:
//
//   Surface area A(f) = sum over every cell c of feature f, and every one of c's
//   six face neighbours n, of the area of the shared face — counted if and only if
//     (a) n lies INSIDE the grid, and
//     (b) FeatureIds[n] != FeatureIds[c].
//   The face shared with the +/-Z neighbour lies in the XY plane -> area dx*dy.
//   The face shared with the +/-Y neighbour lies in the XZ plane -> area dx*dz.
//   The face shared with the +/-X neighbour lies in the YZ plane -> area dy*dz.
//   Two consequences that the fixtures below pin down deliberately:
//     * A face on the OUTER BOUNDARY of the grid is never counted (rule (a)) —
//       so a feature touching the sample edge under-reports its area, and its
//       sphericity can exceed 1 (see the "Corner voxel" section).
//     * FeatureId 0 is an ordinary differing id (rule (b)) — a feature embedded in
//       id-0 "bad data" gets exactly the same area as one embedded in a positive
//       neighbour (see the "Id 0 counts as surface" section).
//
//   Volume V(f) = NumCells[f] * dx*dy*dz — the USER-SUPPLIED NumCells value, never
//   a recount of the FeatureIds array.
//   SurfaceAreaVolumeRatio(f) = A(f) / V(f).
//   Sphericity(f) = pi^(1/3) * (6 V(f))^(2/3) / A(f)     [Wadell 1935]
//
// Every expected number in this file was derived from the rules above by an
// independent brute-force face enumeration written before the filter was ever run
// (see the V&V report's Oracle section and ww_work/ComputeSurfaceAreaToVolume/derive.py).
//
// Spacings are dyadic (0.5) or small integers (1, 2, 4) so that every expected area,
// volume and ratio is exactly representable in float32; the ratio assertions are
// therefore exact equalities. Sphericity involves cube roots, so those assertions
// use a 1e-6 relative tolerance (the fixed float32 expression lands within 1.3e-7).
//
// These inline fixtures replace the retired 6_6_stats_test_v2 exemplar test, which
// compared SIMPLNX against a sibling array in the same archive that had been produced
// by the very code under test — a circular oracle that additionally baked in the two
// bugs fixed on this branch.
//
// Reference: src/Plugins/SimplnxCore/vv/ComputeSurfaceAreaToVolumeFilter.md
// =============================================================================

namespace SavToy
{
const std::string k_GeomName = "Image";
const std::string k_CellAMName = "Cell Data";
const std::string k_FeatureAMName = "Cell Feature Data";
const std::string k_FeatureIdsName = "FeatureIds";
const std::string k_NumCellsName = "NumElements";
const std::string k_SavrName = "SurfaceAreaVolumeRatio";
// Legacy SIMPL pins the sphericity array name to "Sphericity" (it never reads the name from
// the pipeline file), so the A/B fixtures use that name here too.
const std::string k_SphericityName = "Sphericity";

struct Scaffold
{
  DataStructure ds;
  DataPath geomPath;
  DataPath featureIdsPath;
  DataPath featureAMPath;
  DataPath numCellsPath;
  DataPath savrPath;
  DataPath sphericityPath;
};

/**
 * @brief Builds an ImageGeom + Cell Data AM (FeatureIds) + Cell Feature Data AM (NumElements).
 * @param dims (x, y, z) cell counts
 * @param spacing (dx, dy, dz)
 * @param featureIdValues flat FeatureIds in (z, y, x) row-major order, length dims[0]*dims[1]*dims[2]
 * @param numCellsValues NumElements, one per feature tuple (index 0 included)
 */
inline Scaffold Build(std::array<usize, 3> dims, std::array<float32, 3> spacing, const std::vector<int32>& featureIdValues, const std::vector<int32>& numCellsValues)
{
  Scaffold s;
  auto* imageGeomPtr = ImageGeom::Create(s.ds, k_GeomName);
  imageGeomPtr->setDimensions({dims[0], dims[1], dims[2]});
  imageGeomPtr->setSpacing({spacing[0], spacing[1], spacing[2]});
  imageGeomPtr->setOrigin({0.0F, 0.0F, 0.0F});

  const ShapeType cellTupleShape{dims[2], dims[1], dims[0]};
  auto* cellAMPtr = AttributeMatrix::Create(s.ds, k_CellAMName, cellTupleShape, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAMPtr);

  REQUIRE(featureIdValues.size() == dims[0] * dims[1] * dims[2]);
  auto* featureIdsPtr = CreateTestDataArray<int32>(s.ds, k_FeatureIdsName, cellTupleShape, {1}, cellAMPtr->getId());
  for(usize i = 0; i < featureIdValues.size(); ++i)
  {
    (*featureIdsPtr)[i] = featureIdValues[i];
  }

  auto* featureAMPtr = AttributeMatrix::Create(s.ds, k_FeatureAMName, ShapeType{numCellsValues.size()}, imageGeomPtr->getId());
  auto* numCellsPtr = CreateTestDataArray<int32>(s.ds, k_NumCellsName, ShapeType{numCellsValues.size()}, {1}, featureAMPtr->getId());
  for(usize i = 0; i < numCellsValues.size(); ++i)
  {
    (*numCellsPtr)[i] = numCellsValues[i];
  }

  s.geomPath = DataPath({k_GeomName});
  s.featureIdsPath = s.geomPath.createChildPath(k_CellAMName).createChildPath(k_FeatureIdsName);
  s.featureAMPath = s.geomPath.createChildPath(k_FeatureAMName);
  s.numCellsPath = s.featureAMPath.createChildPath(k_NumCellsName);
  s.savrPath = s.featureAMPath.createChildPath(k_SavrName);
  s.sphericityPath = s.featureAMPath.createChildPath(k_SphericityName);
  return s;
}

inline Arguments MakeArgs(const Scaffold& s, bool calculateSphericity)
{
  Arguments args;
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(s.geomPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(s.featureIdsPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_NumCellsArrayPath_Key, std::make_any<DataPath>(s.numCellsPath));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CalculateSphericity_Key, std::make_any<bool>(calculateSphericity));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SurfaceAreaVolumeRatioArrayName_Key, std::make_any<std::string>(k_SavrName));
  args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_SphericityArrayName_Key, std::make_any<std::string>(k_SphericityName));
  return args;
}

struct Output
{
  std::vector<float32> savr;
  std::vector<float32> sphericity; // empty when CalculateSphericity was off
};

inline Output Run(Scaffold& s, bool calculateSphericity = true)
{
  ComputeSurfaceAreaToVolumeFilter filter;
  const Arguments args = MakeArgs(s, calculateSphericity);

  auto preflightResult = filter.preflight(s.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(s.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  Output out;
  REQUIRE_NOTHROW(s.ds.getDataRefAs<Float32Array>(s.savrPath));
  const auto& savrRef = s.ds.getDataRefAs<Float32Array>(s.savrPath).getDataStoreRef();
  out.savr.resize(savrRef.getNumberOfTuples());
  for(usize i = 0; i < savrRef.getNumberOfTuples(); ++i)
  {
    out.savr[i] = savrRef[i];
  }

  if(calculateSphericity)
  {
    REQUIRE_NOTHROW(s.ds.getDataRefAs<Float32Array>(s.sphericityPath));
    const auto& sphericityRef = s.ds.getDataRefAs<Float32Array>(s.sphericityPath).getDataStoreRef();
    out.sphericity.resize(sphericityRef.getNumberOfTuples());
    for(usize i = 0; i < sphericityRef.getNumberOfTuples(); ++i)
    {
      out.sphericity[i] = sphericityRef[i];
    }
  }
  return out;
}

// Relative tolerance for sphericity. The fixed float32 expression
// pow(pi, 1/3) * pow(6V, 2/3) / A reproduces the double-precision oracle to
// within 1.3e-7 relative on every fixture in this file, so 1e-6 is a real
// constraint and not a rubber stamp: the pre-fix truncated exponents
// (0.333333 / 0.66666) miss by 1.4e-6 .. 3.6e-5 and fail it.
constexpr float64 k_SphericityEpsilon = 1.0e-6;

inline void RequireSphericity(const std::vector<float32>& sphericity, usize featureId, float64 expected)
{
  CAPTURE(featureId, sphericity[featureId], expected);
  REQUIRE(static_cast<float64>(sphericity[featureId]) == Approx(expected).epsilon(k_SphericityEpsilon));
}

// (pi/6)^(1/3) — the sphericity of any perfect cube whose faces are all counted.
constexpr float64 k_CubeSphericity = 0.8059959770082348;
} // namespace SavToy

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: Class 1 - Analytical Surface Area & Sphericity", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("F1 single interior voxel")
  {
    // 3x3x3, spacing a = 0.5. One voxel of feature 1 at (1,1,1); everything else id 0.
    // All six neighbours exist and differ  ->  A = 6a^2 = 1.5
    // V = NumCells[1] * a^3 = 1 * 0.125     ->  SAVR = 6/a = 12
    // Psi = pi^(1/3)(6V)^(2/3)/A = (pi/6)^(1/3), independent of a.
    //
    // NumCells[0] is deliberately set to a non-zero sentinel (5). Both output loops start at
    // feature 1, so index 0 must remain at its preflight-created 0.0F. Had the sphericity loop
    // started at 0 it would have written pi^(1/3)(6*5*a^3)^(2/3)/0 = +inf there, so this
    // assertion actually discriminates the loop start.
    std::vector<int32> ids(27, 0);
    ids[1 * 9 + 1 * 3 + 1] = 1;
    auto s = SavToy::Build({3, 3, 3}, {0.5F, 0.5F, 0.5F}, ids, {5, 1});
    auto out = SavToy::Run(s);

    REQUIRE(out.savr.size() == 2);
    REQUIRE(out.savr[1] == 12.0F);
    SavToy::RequireSphericity(out.sphericity, 1, SavToy::k_CubeSphericity);

    // Index 0 is never written by either loop.
    REQUIRE(out.savr[0] == 0.0F);
    REQUIRE(out.sphericity[0] == 0.0F);

    UnitTest::CheckArraysInheritTupleDims(s.ds);
  }

  SECTION("F1b over-provisioned Feature AM is accepted")
  {
    // Same geometry as F1, but the Feature AttributeMatrix has 4 tuples while the largest
    // FeatureId present is 1. SIMPLNX accepts this -- the execute-time guard only rejects ids
    // that would index PAST the feature arrays -- and simply never writes the unused tuples'
    // area, so they finish with A = 0:
    //     feature 2: SAVR = 0 / (3 * a^3) = 0,  Psi = (positive)/0 = +inf
    //     feature 3: SAVR = 0 / (5 * a^3) = 0,  Psi = +inf
    // DREAM3D 6.5.171 REJECTS this input outright (error -5555: it requires
    // max(FeatureIds) == numFeatures - 1 exactly), so this section is SIMPLNX-only and is
    // recorded as a deviation. See vv/deviations/ComputeSurfaceAreaToVolumeFilter.md.
    std::vector<int32> ids(27, 0);
    ids[1 * 9 + 1 * 3 + 1] = 1;
    auto s = SavToy::Build({3, 3, 3}, {0.5F, 0.5F, 0.5F}, ids, {0, 1, 3, 5});
    auto out = SavToy::Run(s);

    REQUIRE(out.savr.size() == 4);
    REQUIRE(out.savr[1] == 12.0F);
    SavToy::RequireSphericity(out.sphericity, 1, SavToy::k_CubeSphericity);
    for(usize i = 2; i < 4; ++i)
    {
      CAPTURE(i);
      REQUIRE(out.savr[i] == 0.0F);
      REQUIRE(std::isinf(out.sphericity[i]));
    }

    UnitTest::CheckArraysInheritTupleDims(s.ds);
  }

  SECTION("F2 2x2x2 interior cube - scale invariance")
  {
    // 4x4x4, spacing a = 0.5. Feature 1 fills the interior 2x2x2 block.
    // Each of the 8 cells has 3 same-feature neighbours and 3 differing ones
    //   -> 24 counted faces, A = 24a^2 = 6.0
    // V = 8a^3 = 1.0, SAVR = 3/a = 6.0.
    // Psi is (pi/6)^(1/3) again — identical to F1's single voxel, which is the
    // scale-invariance invariant (Class 4) riding along on this fixture.
    std::vector<int32> ids(64, 0);
    for(usize z = 1; z <= 2; ++z)
    {
      for(usize y = 1; y <= 2; ++y)
      {
        for(usize x = 1; x <= 2; ++x)
        {
          ids[z * 16 + y * 4 + x] = 1;
        }
      }
    }
    auto s = SavToy::Build({4, 4, 4}, {0.5F, 0.5F, 0.5F}, ids, {0, 8});
    auto out = SavToy::Run(s);

    REQUIRE(out.savr[1] == 6.0F);
    SavToy::RequireSphericity(out.sphericity, 1, SavToy::k_CubeSphericity);

    UnitTest::CheckArraysInheritTupleDims(s.ds);
  }

  SECTION("F2b NumCells is user-supplied, never recounted")
  {
    // The volume is NumCells[f] * dx*dy*dz, where NumCells is whatever the user selected --
    // the filter must NOT recount the cells of each feature from the FeatureIds array.
    // Here exactly ONE cell carries id 1 but NumCells[1] is 8, so:
    //   A = 6a^2 = 1.5  (from the FeatureIds geometry)
    //   V = 8 * a^3 = 1.0  (from NumCells, NOT from the one cell present)
    //   SAVR = 1.5,  Psi = pi^(1/3) * 6^(2/3) / 1.5 = 3.22398...
    // A recount would give V = 0.125, SAVR = 12 and Psi = 0.806 instead.
    std::vector<int32> ids(27, 0);
    ids[1 * 9 + 1 * 3 + 1] = 1;
    auto s = SavToy::Build({3, 3, 3}, {0.5F, 0.5F, 0.5F}, ids, {0, 8});
    auto out = SavToy::Run(s);

    REQUIRE(out.savr[1] == 1.5F);
    SavToy::RequireSphericity(out.sphericity, 1, 3.2239839080329387);

    UnitTest::CheckArraysInheritTupleDims(s.ds);
  }

  SECTION("F3 anisotropic rods - per-axis face areas")
  {
    // THE discriminating fixture for the +/-X <-> +/-Y face-area assignment.
    //
    // 6x8x3, spacing (dx, dy, dz) = (1, 2, 4)  ->  voxelVol = 8, and the three
    // distinct face areas are all different:  dx*dy = 2, dx*dz = 4, dy*dz = 8.
    //
    // Feature 1 - a 4-cell rod along X at (x=1..4, y=1, z=1), fully interior:
    //   2 end faces normal to X  -> 2 * (dy*dz = 8) = 16
    //   8 side faces normal to Y -> 8 * (dx*dz = 4) = 32
    //   8 side faces normal to Z -> 8 * (dx*dy = 2) = 16
    //   A1 = 64,  V1 = 4 * 8 = 32,  SAVR1 = 2.0
    //
    // Feature 2 - the mirror, a 4-cell rod along Y at (x=1, y=3..6, z=1):
    //   2 end faces normal to Y  -> 2 * (dx*dz = 4) = 8
    //   8 side faces normal to X -> 8 * (dy*dz = 8) = 64
    //   8 side faces normal to Z -> 8 * (dx*dy = 2) = 16
    //   A2 = 88,  V2 = 32,  SAVR2 = 2.75
    //
    // Swapping the +/-X and +/-Y face areas exchanges A1 and A2 exactly (A1 -> 88,
    // A2 -> 64, i.e. SAVR1 -> 2.75 and SAVR2 -> 2.0), which is what this section
    // caught on the pre-fix code.
    std::vector<int32> ids(6 * 8 * 3, 0);
    const auto idx = [](usize x, usize y, usize z) { return z * 6 * 8 + y * 6 + x; };
    for(usize x = 1; x <= 4; ++x)
    {
      ids[idx(x, 1, 1)] = 1;
    }
    for(usize y = 3; y <= 6; ++y)
    {
      ids[idx(1, y, 1)] = 2;
    }
    auto s = SavToy::Build({6, 8, 3}, {1.0F, 2.0F, 4.0F}, ids, {0, 4, 4});
    auto out = SavToy::Run(s);

    REQUIRE(out.savr[1] == 2.0F);  // A1 = 64 / V = 32
    REQUIRE(out.savr[2] == 2.75F); // A2 = 88 / V = 32
    SavToy::RequireSphericity(out.sphericity, 1, 0.7616184727713847);
    SavToy::RequireSphericity(out.sphericity, 2, 0.5539043438337707);

    // Class 4 invariant: the two rods are geometric mirrors of one another under the
    // relabelling (x<->y, dx<->dy), so the multiset of their areas must be {64, 88}
    // whichever way round the implementation assigns them. Asserting the SUM as well
    // as the individual values documents that the pre-fix failure was an exchange and
    // not a scale error: the sum was right, the assignment was not.
    REQUIRE((out.savr[1] + out.savr[2]) == 4.75F);

    UnitTest::CheckArraysInheritTupleDims(s.ds);
  }

  SECTION("F4 corner voxel - outer boundary faces are NOT counted")
  {
    // !!! READ THIS BEFORE "FIXING" THE EXPECTED VALUE BELOW !!!
    //
    // 3x3x3, spacing a = 0.5, one voxel of feature 1 at the (0,0,0) CORNER.
    // Its -X, -Y and -Z neighbours are outside the grid, so those three faces are
    // skipped entirely -- they are NOT treated as exposed surface. Only the three
    // faces toward +X, +Y, +Z are counted:
    //     A = 3a^2 = 0.75   (not 6a^2)
    //     V = a^3 = 0.125,  SAVR = 3/a = 6.0
    //     Psi = pi^(1/3)(6a^3)^(2/3)/(3a^2) = 2*(pi/6)^(1/3) = 1.61199...
    //
    // Psi > 1 IS THE EXPECTED, CORRECT OUTPUT of this filter for a feature that
    // touches the sample boundary. A sphericity above 1 is geometrically impossible
    // for a real solid; it appears here because the denominator A is a systematic
    // UNDER-estimate of the true surface area whenever the boundary-face rule kicks
    // in. This is a documented property of the algorithm (it matches DREAM3D 6.5.171),
    // not a defect, and it is exactly why the user documentation now warns that
    // boundary-touching features have skewed values. Do not "correct" 1.612 to
    // something <= 1.
    std::vector<int32> ids(27, 0);
    ids[0] = 1;
    auto s = SavToy::Build({3, 3, 3}, {0.5F, 0.5F, 0.5F}, ids, {0, 1});
    auto out = SavToy::Run(s);

    REQUIRE(out.savr[1] == 6.0F);
    SavToy::RequireSphericity(out.sphericity, 1, 2.0 * SavToy::k_CubeSphericity);
    REQUIRE(out.sphericity[1] > 1.0F); // stated explicitly so the intent survives a refactor

    UnitTest::CheckArraysInheritTupleDims(s.ds);
  }

  SECTION("F5 feature fills the whole volume - degenerate divide")
  {
    // 3x3x3, spacing a = 0.5, every cell is feature 1. No neighbour differs and every
    // face that would be exposed lies on the outer boundary, so A = 0 exactly.
    //   SAVR = 0/V = 0
    //   Psi  = (positive numerator)/0 = +inf   <- documented degenerate case
    std::vector<int32> ids(27, 1);
    auto s = SavToy::Build({3, 3, 3}, {0.5F, 0.5F, 0.5F}, ids, {0, 27});
    auto out = SavToy::Run(s);

    REQUIRE(out.savr[1] == 0.0F);
    REQUIRE(std::isinf(out.sphericity[1]));
    REQUIRE(out.sphericity[1] > 0.0F);

    UnitTest::CheckArraysInheritTupleDims(s.ds);
  }

  SECTION("F6 id 0 counts as surface")
  {
    // Falsifies the old documentation claim that a face is only counted "as long as that
    // neighboring featureId is > 0". A single voxel of feature 1 sits at the centre of a
    // 3x3x3 grid; every other cell is given the SAME id, once 0 and once a positive 2.
    // Feature 1's area is 6a^2 = 1.5 in BOTH cases: id 0 is an ordinary differing id.
    // If id-0 neighbours were skipped, the shellId == 0 run would report A = 0 (SAVR = 0,
    // Psi = +inf) instead.
    const int32 shellId = GENERATE(0, 2);
    DYNAMIC_SECTION("shell id = " << shellId)
    {
      std::vector<int32> ids(27, shellId);
      ids[1 * 9 + 1 * 3 + 1] = 1;
      // max(FeatureIds) == numFeatures - 1 in both cases, so both fixtures are also legal
      // input to the legacy DREAM3D filter (which errors -5555 otherwise).
      std::vector<int32> numCells = (shellId == 0) ? std::vector<int32>{0, 1} : std::vector<int32>{0, 1, 26};
      auto s = SavToy::Build({3, 3, 3}, {0.5F, 0.5F, 0.5F}, ids, numCells);
      auto out = SavToy::Run(s);

      REQUIRE(out.savr[1] == 12.0F); // A = 1.5 either way
      SavToy::RequireSphericity(out.sphericity, 1, SavToy::k_CubeSphericity);

      if(shellId == 2)
      {
        // The shell itself: A = 1.5 (only the six faces it shares with feature 1; every
        // other exposed face of the shell lies on the outer boundary), V = 26 * 0.125 = 3.25.
        REQUIRE(out.savr[2] == Approx(1.5F / 3.25F).epsilon(1.0e-6));
        SavToy::RequireSphericity(out.sphericity, 2, 7.073729355035006);
      }

      UnitTest::CheckArraysInheritTupleDims(s.ds);
    }
  }

  SECTION("F7 2D slab - no +/-Z faces")
  {
    // 7x3x1, spacing (1, 2, 4). zPoints == 1, so for every cell BOTH z-neighbours are
    // outside the grid and no dx*dy face is ever counted.
    //
    // Feature 1 - a single cell at (1,1,0):
    //   2 faces normal to X -> 2 * (dy*dz = 8) = 16
    //   2 faces normal to Y -> 2 * (dx*dz = 4) = 8
    //   A1 = 24  (a 3D slab would have added 2 * (dx*dy = 2) = 4 on top)
    //   V1 = 1 * 8 = 8,  SAVR1 = 3.0
    //
    // Feature 2 - a 2-cell rod along X at (x=3..4, y=1, z=0):
    //   2 end faces normal to X  -> 2 * 8 = 16
    //   4 side faces normal to Y -> 4 * 4 = 16
    //   A2 = 32,  V2 = 2 * 8 = 16,  SAVR2 = 2.0
    // Feature 2 also discriminates the face-area assignment in the 2D path: swapping the
    // X and Y face areas would give A2 = 2*4 + 4*8 = 40 and SAVR2 = 2.5.
    std::vector<int32> ids(7 * 3 * 1, 0);
    ids[1 * 7 + 1] = 1;
    ids[1 * 7 + 3] = 2;
    ids[1 * 7 + 4] = 2;
    auto s = SavToy::Build({7, 3, 1}, {1.0F, 2.0F, 4.0F}, ids, {0, 1, 2});
    auto out = SavToy::Run(s);

    REQUIRE(out.savr[1] == 3.0F);
    REQUIRE(out.savr[2] == 2.0F);
    SavToy::RequireSphericity(out.sphericity, 1, SavToy::k_CubeSphericity);
    SavToy::RequireSphericity(out.sphericity, 2, 0.9595791455066552);

    UnitTest::CheckArraysInheritTupleDims(s.ds);
  }
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: Sphericity Toggle Off", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();

  // F8. Same fixture as F1, but CalculateSphericity = false: the Sphericity array must not
  // be created at all, and the ratio must still be computed.
  //
  // This path is SIMPLNX-only and cannot be compared against DREAM3D 6.5.171: legacy's
  // readFilterParameters() never reads the CalculateSphericity key, so a legacy pipeline
  // run always takes the sphericity-on path regardless of what the pipeline file says.
  std::vector<int32> ids(27, 0);
  ids[1 * 9 + 1 * 3 + 1] = 1;
  auto s = SavToy::Build({3, 3, 3}, {0.5F, 0.5F, 0.5F}, ids, {0, 1});
  auto out = SavToy::Run(s, false);

  REQUIRE(out.savr[1] == 12.0F);
  REQUIRE(s.ds.getDataAs<Float32Array>(s.sphericityPath) == nullptr);

  UnitTest::CheckArraysInheritTupleDims(s.ds);
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: Error Paths", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter]")
{
  UnitTest::LoadPlugins();

  // The F1 geometry is reused throughout: 3x3x3, spacing 0.5, feature 1 at the centre.
  const auto makeIds = []() {
    std::vector<int32> ids(27, 0);
    ids[1 * 9 + 1 * 3 + 1] = 1;
    return ids;
  };

  SECTION("Execute error - negative FeatureIds (-5355)")
  {
    auto ids = makeIds();
    ids[0] = -1;
    auto s = SavToy::Build({3, 3, 3}, {0.5F, 0.5F, 0.5F}, ids, {0, 1});

    ComputeSurfaceAreaToVolumeFilter filter;
    const Arguments args = SavToy::MakeArgs(s, true);
    auto preflightResult = filter.preflight(s.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(s.ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
    REQUIRE(executeResult.result.errors()[0].code == -5355);
  }

  SECTION("Execute error - FeatureId exceeds the Feature AM (-5351)")
  {
    auto ids = makeIds();
    ids[2] = 5; // NumElements has only 2 tuples
    auto s = SavToy::Build({3, 3, 3}, {0.5F, 0.5F, 0.5F}, ids, {0, 1});

    ComputeSurfaceAreaToVolumeFilter filter;
    const Arguments args = SavToy::MakeArgs(s, true);
    auto preflightResult = filter.preflight(s.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(s.ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
    REQUIRE(executeResult.result.errors()[0].code == -5351);
  }

  SECTION("Preflight error - NumCells not in an AttributeMatrix (-12802)")
  {
    auto s = SavToy::Build({3, 3, 3}, {0.5F, 0.5F, 0.5F}, makeIds(), {0, 1});

    // A NumElements array parented to a plain DataGroup rather than an AttributeMatrix.
    auto* groupPtr = DataGroup::Create(s.ds, "Loose Group");
    CreateTestDataArray<int32>(s.ds, SavToy::k_NumCellsName, ShapeType{2}, {1}, groupPtr->getId());

    Arguments args = SavToy::MakeArgs(s, true);
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_NumCellsArrayPath_Key, std::make_any<DataPath>(DataPath({"Loose Group", SavToy::k_NumCellsName})));

    ComputeSurfaceAreaToVolumeFilter filter;
    auto preflightResult = filter.preflight(s.ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
    REQUIRE(preflightResult.outputActions.errors()[0].code == -12802);
  }

  SECTION("Preflight error - FeatureIds tuple count does not match the geometry (-12803)")
  {
    // The algorithm walks the ImageGeom's cell extents and indexes FeatureIds with the
    // resulting flat index. If the selected FeatureIds array has fewer tuples than the
    // geometry has cells -- easy to do, since nothing forces the selection to come from
    // the geometry's own cell AttributeMatrix -- every read past the end is out of bounds.
    // Preflight must reject that before execute() can run off the end of the store.
    auto s = SavToy::Build({3, 3, 3}, {0.5F, 0.5F, 0.5F}, makeIds(), {0, 1});

    auto* groupPtr = DataGroup::Create(s.ds, "Other Data");
    CreateTestDataArray<int32>(s.ds, SavToy::k_FeatureIdsName, ShapeType{26}, {1}, groupPtr->getId());

    Arguments args = SavToy::MakeArgs(s, true);
    args.insertOrAssign(ComputeSurfaceAreaToVolumeFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Other Data", SavToy::k_FeatureIdsName})));

    ComputeSurfaceAreaToVolumeFilter filter;
    auto preflightResult = filter.preflight(s.ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
    REQUIRE(preflightResult.outputActions.errors()[0].code == -12803);
  }
}

TEST_CASE("SimplnxCore::ComputeSurfaceAreaToVolumeFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeSurfaceAreaToVolumeFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeSurfaceAreaToVolumeFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeSurfaceAreaToVolumeFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeSurfaceAreaToVolumeFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeSurfaceAreaToVolumeFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ComputeSurfaceAreaToVolumeFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeSurfaceAreaToVolumeFilter::k_NumCellsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeSurfaceAreaToVolumeFilter::k_SurfaceAreaVolumeRatioArrayName_Key) == "TestName");
      CHECK(args.value<bool>(ComputeSurfaceAreaToVolumeFilter::k_CalculateSphericity_Key) == true);
      CHECK(args.value<std::string>(ComputeSurfaceAreaToVolumeFilter::k_SphericityArrayName_Key) == "TestName");
    }
  }
}
