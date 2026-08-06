# Deviations from DREAM3D 6.5.171: BadDataNeighborOrientationCheckFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent (`BadDataNeighborOrientationCheck`, source at `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/BadDataNeighborOrientationCheck.{h,cpp}` in DREAM3D 6.5.171).

Entries are referenced by stable ID (`BadDataNeighborOrientationCheckFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

## Comparison summary

A direct A/B comparison was run on 2026-05-29 across all 27 algorithmic test fixtures defined by the engineer in the V&V test data archive (`bad_data_neighbor_orientation_check_v2/test_design.md`). Inputs were identical (same `Quats`, `Phases`, `Mask`, `CrystalStructures`, `MisorientationTolerance`, `NumberOfNeighbors`) for both implementations.

| | Cases | Mask bytes affected |
|---|---|---|
| Bit-identical SIMPLNX = 6.5.171 | 12 of 27 (all Case 1.X.{2,3} — "should not flip" scenarios) | 0 |
| SIMPLNX ≠ 6.5.171, direction 1→0 (SIMPLNX flips, 6.5.171 misses) | 15 of 27 (all Case 1.X.1 + all Case 2.X + both Case 3.X + Case 4) | 288 |
| SIMPLNX ≠ 6.5.171, direction 0→1 (SIMPLNX correct, 6.5.171 false-flips) | 0 | 0 |

**100% of observed diffs are direction 1→0**, consistent with a single root cause: the legacy convergence-loop bound that terminates one level early (D1 below). The D2 stale-`w` defect is real and code-evident but does not produce a 0→1 diff in any of the engineer's tests because D1 prevents the bumped count from ever crossing threshold — the two legacy bugs mask each other.

---

## BadDataNeighborOrientationCheckFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `BadDataNeighborOrientationCheckFilter-D1` |
| **Filter UUID** | `3f342977-aea1-49e1-a9c2-f73760eba0d3` |
| **Status** | active |

**Symptom:** DREAM3D 6.5.171 fails to flip a bad voxel whose good-neighbor count is exactly equal to the user-supplied `NumberOfNeighbors`. SIMPLNX correctly flips such voxels. Observable in 15 of the 27 V&V fixtures (288 mask bytes total) — every case where the algorithm depends on reaching the bottom level of the iterative-decay loop.

**Root cause:** Bug in DREAM3D 6.5.171.

The legacy iterative-decay loop is `while(currentLevel > m_NumberOfNeighbors)` (`Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/BadDataNeighborOrientationCheck.cpp:299`). With user-supplied `NumberOfNeighbors = N`, this walks `currentLevel` from 6 down through `N + 1` and never executes the `currentLevel == N` iteration. A bad voxel with exactly N good neighbors can therefore never be flipped — contradicting the parameter description ("Required Number of Neighbors") which implies that count to be sufficient.

SIMPLNX corrects this to `while(currentLevel >= m_InputValues->NumberOfNeighbors)` (`src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/BadDataNeighborOrientationCheck.cpp:129`). The fix is also explicitly documented as "BUG: Fix only checking values greater than the supplied min number of neighbors" in the merge commit of PR #1499 and in the engineer's V&V test archive README at `bad_data_neighbor_orientation_check_v2/README.md` §"Issue 1".

**Affected users:** Anyone running the filter in DREAM3D 6.5.171 with `NumberOfNeighbors < 6` on a dataset where the bottom level matters (i.e., where any bad voxel's eligible-neighbor count equals the user's `NumberOfNeighbors`). In practice this is the typical usage — the Small IN100 reconstruction pipeline (the canonical DREAM3D example) uses `NumberOfNeighbors = 4`. The 6.5.171 output left bad voxels unflipped that should have been flipped, manifesting downstream as smaller-than-expected grain reconstructions, more "rough" grain boundaries, and lower fraction of good voxels.

**Recommendation:** Trust SIMPLNX. The 6.5.171 result was mathematically incorrect for the stated parameter semantics. The minimal legacy patch is a one-line change from `>` to `>=`; the root cause was proven by applying this fix (bundled with D2) to a local build of the legacy source — contact the DREAM3D team for the legacy-parity patch.

---

## BadDataNeighborOrientationCheckFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `BadDataNeighborOrientationCheckFilter-D2` |
| **Filter UUID** | `3f342977-aea1-49e1-a9c2-f73760eba0d3` |
| **Status** | active (latent — not observable in the V&V test suite, but real and code-evident) |

**Symptom:** Latent. DREAM3D 6.5.171 can count a different-phase neighbor's misorientation as within tolerance if a *previous* same-phase neighbor's `w` was small, because the misorientation-threshold check sits outside the same-phase conditional and inherits the stale `w` from the prior iteration. SIMPLNX prevents this by moving the threshold check inside the same-phase conditional.

This bug is not directly observable in any of the 27 V&V test fixtures, because the D1 loop-bound bug (above) terminates the iterative-decay loop before any voxel whose count was incorrectly bumped by D2 could be flipped. The two legacy bugs cancel each other in the engineer's test inputs.

**Root cause:** Bug in DREAM3D 6.5.171.

The legacy per-neighbor loop body is (`BadDataNeighborOrientationCheck.cpp:283-291`):

```cpp
if(m_CellPhases[i] == m_CellPhases[neighbor] && m_CellPhases[i] > 0)
{
  w = m_OrientationOps[phase1]->getMisoQuat(q1, q2, n1, n2, n3);
}
if(w < misorientationTolerance)  // <-- outside the same-phase conditional!
{
  neighborCount[i]++;
}
```

When the current neighbor has a different phase than the voxel, the `w = getMisoQuat(...)` assignment is skipped, and the subsequent `if(w < misorientationTolerance)` reads `w` from the *previous neighbor iteration* (or from `w`'s initial value `10000.0f` if no previous iteration matched). The previous iteration's `w` may be small (e.g., from a same-phase good neighbor with an identical orientation), in which case the comparison succeeds and the count is incorrectly bumped.

SIMPLNX moves both the misorientation computation AND the increment inside the same-phase conditional (`Algorithms/BadDataNeighborOrientationCheck.cpp:105-117`):

```cpp
if(cellPhases[voxelIndex] == cellPhases[neighborPoint] && cellPhases[voxelIndex] > 0)
{
  ebsdlib::QuatD quat2(quats[neighborPoint * 4], ...);
  quat2.positiveOrientation();
  ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClass1]->calculateMisorientation(quat1, quat2);
  if(axisAngle[3] < misorientationTolerance)
  {
    neighborCount[voxelIndex]++;
  }
}
```

The bug is documented as "Issue 2" in the engineer's V&V test archive README at `bad_data_neighbor_orientation_check_v2/README.md`, and was bundled into PR #1499's REV cleanup.

**Affected users:** Anyone running the filter in DREAM3D 6.5.171 on a dataset with mixed phases adjacent to grain boundaries. The bug would manifest as voxels at phase boundaries being incorrectly flipped to "good" because they appear to have more within-tolerance neighbors than they actually do.

**Why not observable in V&V A/B:** The D1 loop-bound bug prevents iteration from reaching the level where the bumped count would matter. With `NumberOfNeighbors = N`, D1 stops iteration at `currentLevel = N + 1`, so a voxel with count = N (true count) or N + 1 (bumped count) cannot be flipped at the N level. To isolate D2, one would need to patch legacy 6.5.171 with just the D1 fix (without D2 fix), then run a mixed-phase fixture where a bad voxel's neighbor sequence includes a same-phase good neighbor followed by a different-phase neighbor. This is a future Phase 8 regression test addition.

**Recommendation:** Trust SIMPLNX. The 6.5.171 result was mathematically incorrect. Both the D1 and D2 fixes were applied together to a local build of the legacy source for the root-cause proof (see D1). Note: applying only the D1 fix to 6.5.171 without also applying the D2 fix would UNCOVER D2 as new false-positive flips at phase boundaries — both fixes belong together.

---

### EbsdLib 2.4.1 CubicOps precision improvement (precision improvement; not a behavioral deviation in this filter's test data)

SIMPLNX delegates misorientation math to `ebsdlib::LaueOps::calculateMisorientation` (EbsdLib 2.4.1+); legacy 6.5.171 delegates to `OrientationLib::CubicOps::getMisoQuat` (DREAM3D 6.5.x). The modern API recovers ~0.02° of precision for cubic misorientations that lie on a 4-fold, 3-fold, or 2-fold symmetry op (replacing the precision-fragile `acos(w)` near 1 with the numerically stable `2·atan2(|v|, w)` using explicit reduced-quaternion v components). The improvement is documented in the EbsdLib 2.4.1 release notes (commit `5c8c993` on `/Users/mjackson/Workspace6/EbsdLib`, 2026-05-29).

**Not observed as a deviation in this filter** because the engineer's test fixtures do not include any voxel pair whose misorientation lands on a cubic sym op. The improvement is real and affects other downstream filters (see `ComputeFeatureFaceMisorientationFilter` V&V cycle's D4); for `BadDataNeighborOrientationCheck` specifically, this is a transparent dependency upgrade.

---

## Comparison artifacts

Verification fixtures + comparison results are at `/Users/mjackson/Workspace6/DREAM3D_Data/TestFiles/bad_data_neighbor_orientation_check_v2/`:

- `case_*/case_*_*/case_*_*_cell_arrays.csv` — 27 CSV files, one per algorithmic case. Generated from engineer's hand-derived fixtures per `test_design.md`.
- `case_*/case_*_*/6_5_case_*_*_input.json` — 27 legacy DREAM3D pipelines that generate v7.0 `.dream3d` input + run `BadDataNeighborOrientationCheck` + write output.
- `vv_comparison/output_legacy/6_5_171_case_*.dream3d` — 27 legacy outputs from the official 6.5.171 PipelineRunner (`/Users/mjackson/Applications/DREAM3D.app/Contents/bin/PipelineRunner`).
- `bad_data_neighbor_orientation_check_v2/test_design.md` — engineer's hand-derived expected outputs (the Class 1 oracle SIMPLNX is verified against in Phase 6).
- `bad_data_neighbor_orientation_check_v2/README.md` — engineer's documentation of Issues 1 and 2.

Comparison script (saved at `/tmp/diff_legacy_vs_simplnx.py`) extracts SIMPLNX expected output from the inline `expectedMask` arrays in `BadDataNeighborOrientationCheckTest.cpp` and diffs against the 6.5.171 outputs. Re-runnable.
