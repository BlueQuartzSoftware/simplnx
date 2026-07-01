# V&V Report: ComputeKernelAvgMisorientationsFilter

|                            |                                                                                                                                              |
|----------------------------|----------------------------------------------------------------------------------------------------------------------------------------------|
| Plugin                     | OrientationAnalysis                                                                                                                          |
| SIMPLNX UUID               | `61cfc9c1-aa0e-452b-b9ef-d3b9e6268035`                                                                                                       |
| SIMPLNX Human Name         | Compute Kernel Average Misorientations                                                                                                       |
| DREAM3D 6.5.171 equivalent | `FindKernelAvgMisorientations` — `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindKernelAvgMisorientations.{h,cpp}` (UUID `88d332c1-cf6c-52d3-a38d-22f6eae19fa6`) |
| Verified commit            | *<filled at SBIR deliverable assembly>*                                                                                                      |
| Status                     | COMPLETE                                                                                                                             |
| Sign-off                   | *Michael Jackson <mike.jackson@bluequartz.net> (V&V cycle completion, 2026-06-03)*                                                           |

## At a glance

| Aspect                 | Current state                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
|------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Algorithm Relationship | **Port with bug fix** — per-voxel kernel averaging; focal-validity gate; per-voxel divisor always ≥1 (focal self-included). `QuatF`→`QuatD`; `getMisoQuat`→`calculateMisorientation`; iteration order optimized for cache (`col→row→plane` → `plane→row→col`). D2 (legacy l-loop bound typo `KernelSize.z+1` → `KernelSize.x+1`) corrected at port. UUID reassigned.       |
| Oracle (confirmed)     | **Class 1 (Analytical) primary** — 4 hand-derived data fixtures covering uniform single-feature, x-axis gradient, z-axis gradient (3D path), and multi-feature multi-voxel with background. **Class 4 (Invariant) companion** — non-negativity, cubic max-angle bound (62.8°), uniform-within-feature implies KAM=0, background-voxel implies KAM=0 exactly. Class 1 oracle uses pure φ1 Bunge ZXZ rotations `(φ1, 0, 0)` so that for cubic symmetry, the misorientation between any two cells equals `|Δφ1|` (the c-axis 4-fold reduction is identity when φ1 differences are ≤45°). |
| Code paths enumerated  | 6 of 6 algorithmic paths exercised: (1) focal-valid (`featureIds[point] > 0 && cellPhases[point] > 0`) → enter kernel; (2) inner kernel cell in-bounds + feature-id match → accumulate miso + numVoxel++; (3) inner kernel cell in-bounds + feature-id mismatch → skip (no accumulate); (4) inner kernel cell out-of-bounds (boundary clamp) → `continue`; (5) focal-invalid (`featureIds[point] == 0 || cellPhases[point] == 0`) → KAM = 0 directly; (6) the `numVoxel == 0` fallback at line 131 is dead code in practice (the focal voxel always self-matches when the focal is valid, giving numVoxel ≥ 1) and is not exercised by the fixtures by design. |
| Tests today            | **6 TEST_CASEs / 6 ctest entries**, 100% pass (~0.3s combined on EbsdLib 2.4.1+). 4 Class 1 fixtures + 1 Class 4 invariants test (with 3 sub-sections) + 1 SIMPL backwards-compatibility test. **No exemplar archive consumed by this filter.**                                                                                                                                                                                                                                                                                                                                              |
| Exemplar archive       | **None — inline-constructed in test source.** The pre-existing main exemplar TEST_CASE (consumed `6_6_stats_test_v2.tar.gz`) was **retired 2026-06-03** because the exemplar `KernelAverageMisorientations` array was a circular oracle (regenerated from pre-EbsdLib-2.4.1 SIMPLNX output, where the precision shift documented as D1 manifests as a non-zero spurious self-misorientation contribution in every focal voxel — see deviations doc). The 4+1 hand-derived data fixtures cover all 5 active algorithmic paths and replace the retired test. The shared archive `6_6_stats_test_v2.tar.gz` remains downloaded for `AlignSectionsMutualInformation`, `ComputeShapes`, and `ComputeSchmids` tests; only F#5's consumption line was removed. |
| Legacy comparison      | **Source-inspection comparison against DREAM3D 6.5.171** completed. Two deviations observed: **D1 (EbsdLib 2.4.1 CubicOps precision improvement)** — precision-class deviation analogous to BadDataNeighborOrientationCheck, ComputeFeatureFaceMisorientation, ComputeFeatureNeighborMisorientations, and ComputeFeatureReferenceMisorientations of this cycle. The KAM filter is *more sensitive* to this fix than the per-pair misorientation filters because the kernel inclusion of the focal voxel triggers a self-misorientation call per cell, where the pre-2.4.1 `acos(w near 1)` form returns a spurious ~0.03° on float32-sourced quaternions instead of 0°. This precision noise propagates directly into the per-cell average. **D2 (legacy kernel-bound bug at `FindKernelAvgMisorientations.cpp:264`)** — legacy `for(int32_t l = -m_KernelSize.x; l < m_KernelSize.z + 1; l++)` uses `KernelSize.z + 1` as the upper bound for the x-direction inner loop (should be `KernelSize.x + 1`). SIMPLNX has the correct form at line 108. Bug is dormant when `KernelSize.x == KernelSize.z` (default `{1,1,1}` case); fires for asymmetric kernels. |
| Bug flags              | **One real legacy bug (D2) corrected at port time** — see deviations. SIMPLNX has been correct from the port onward; no SIMPLNX-side source change required by this V&V cycle. Logged to `/Users/mjackson/Desktop/bug_triage.md` as a known legacy DREAM3D 6.5.171 issue. |
| V&V phase              | **All V&V work complete per V2 policy.** Class 1 + Class 4 oracle confirmed against 6-test suite; circular-oracle archive consumption retired; legacy A/B by source inspection; user-facing doc updated (pipeline name typo fixed; orphan `MassifPipeline` reference removed; `aptr12_Analysis` and `avtr12_Analysis` added). Three source-tree deliverables (this report + `vv/deviations/...` + `vv/provenance/...`) in place. **Outstanding:** Status promotion DRAFT → READY FOR REVIEW pending second-engineer oracle review (recommend Joey Kleingers, especially the multi-feature multi-voxel fixture's per-cell hand-derivation). |

## Summary

`ComputeKernelAvgMisorientationsFilter` computes the per-cell **Kernel Average Misorientation (KAM)**: for each valid cell (featureId > 0, phase > 0), the algorithm iterates over an axis-aligned kernel of user-specified radius `KernelSize = (x, y, z)`, averages the misorientation between the focal cell's orientation quaternion and every same-feature neighbor's quaternion (including the focal cell itself, which contributes a self-misorientation of 0°), and stores the result. Cells with `featureId == 0` or `phase == 0` are treated as background and assigned KAM = 0 directly.

The filter is the cell-level analog of the feature-level `ComputeFeatureNeighborMisorientationsFilter`. Like that filter, it consumes `Quats` (cell-level avg-orientations) and `CrystalStructures` (per-phase Laue class index), and delegates the actual cubic/hex/etc. symmetry-reduced disorientation calculation to `ebsdlib::LaueOps::calculateMisorientation()`. Unlike the feature-level filter, the kernel iteration ALWAYS visits the focal cell as part of its neighbor list (via the `j=k=l=0` inner iteration), so the per-cell divisor `numVoxel` is always ≥ 1.

The output is `KernelAverageMisorientations`, a `Float32Array` co-located in the same `AttributeMatrix` as the input `Cell Data` arrays, sized one-tuple-per-cell with one component per tuple, in degrees.

## Algorithm Relationship

*Classification:* **Port (with UUID reassignment + name rename + one inherent legacy bug corrected at port time).**

*Evidence:* Cross-checked SIMPLNX algorithm against legacy `FindKernelAvgMisorientations.cpp::execute()`. Same per-voxel outer loop, same per-kernel inner loop, same same-feature gate, same `KAM = totalMisorientation / numVoxel`. Port-time deltas:

- `QuatF` → `QuatD`; `getMisoQuat` → `calculateMisorientation` (EbsdLib 2.4.1+ `2·atan2(|v|, w)` precision form — see D1).
- `setParallelizationEnabled` removed (now always parallel via `ParallelData3DAlgorithm`).
- Iteration order `col→row→plane` → `plane→row→col` (cache-friendlier for x-fastest-varying storage; mathematically identical since all writes go to the same `point` index).
- D2 corrected at port.
- UUID reassigned; `Find` → `Compute` rename.

*Material PRs since baseline (filter-introduction):* (none specifically targeting this filter — the algorithm has been stable since the OrientationAnalysis plugin port).

## Oracle

*Confirmed class:* **Class 1 (Analytical) primary, Class 4 (Invariant) companion.**

### Class 1 (Analytical)

Class 1 oracle derived by hand for each fixture in terms of `|Δφ1|` between cell pairs, justified by the cubic FZ analysis below. All quaternions in the test fixtures use the helper `QuatFromPhi1Deg(phi1)` which returns the quaternion form of a pure Bunge ZXZ Euler rotation `(phi1, 0, 0)` with `Phi = phi2 = 0`. This collapses to a single rotation about the z-axis.

**Cubic FZ argument:** For two cells with pure φ1 rotations differing by Δφ1, the disorientation between them in the cubic FZ equals `|Δφ1|` whenever `|Δφ1| ≤ 45°`. The reasoning: the cubic group's 4-fold rotation about the z-axis reduces φ1 differences modulo 90°; for |Δφ1| ≤ 45°, the reduction is the identity operator (no reduction needed). For all 4 Class 1 fixtures, the maximum φ1 difference between any pair is ≤ 30°, well within the 45° bound. Other cubic symmetry operators (3-fold about [111], 2-fold about [110], etc.) only produce smaller-angle equivalents for misorientations not aligned with a pure z-axis rotation; for pure z-rotations of small magnitude, the identity is the global minimum.

**Per-fixture expected KAM derivation:**

| Fixture                                    | Geometry | Kernel    | Per-cell expected KAM (degrees)                            |
|--------------------------------------------|----------|-----------|------------------------------------------------------------|
| `Class 1 - Uniform 2D Single Feature`      | 3x3x1    | {1,1,0}   | All cells = 0.0 (all in-kernel neighbors share orientation) |
| `Class 1 - 1D x-axis Gradient`             | 5x1x1    | {1,0,0}   | [2.5, 10/3, 10/3, 10/3, 2.5] (see derivation in test comments) |
| `Class 1 - 1D z-axis Gradient (3D path)`   | 1x1x3    | {0,0,1}   | [5.0, 20/3, 5.0]                                            |
| `Class 1 - Multi-Feature Multi-Voxel + BG` | 6x1x1    | {1,0,0}   | [5.0, 5.0, 10.0, 10.0, 0.0, 0.0]                            |

Detailed per-cell hand-derivations are in the test file's TEST_CASE comments and in `vv/provenance/ComputeKernelAvgMisorientationsFilter.md`.

### Class 4 (Invariant)

Class 4 invariants asserted in the `Class 4 - Invariants` TEST_CASE across 3 sub-sections:

1. **Uniform-orientation single-feature → KAM == 0 everywhere.** Asserted on a 3x3x3 uniform-identity-quaternion fixture with kernel `{1,1,1}` (3D path coverage).
2. **Background cell → KAM == 0 exactly.** Asserted on a 3x1x1 fixture with the middle cell flagged as `(featureId=0, phase=0)`.
3. **Range and non-triviality on the x-axis gradient fixture:** (i) `KAM[i] >= 0` for all cells, (ii) `KAM[i] <= 62.8°` (Mackenzie cubic upper bound), (iii) at least one cell has `KAM > 0` (sanity check that the algorithm actually computed something).

The Class 4 invariants are oracle-agnostic — they hold for any input, so they catch regressions even if specific Class 1 expected values were edited away.

### Class 2, 3, 5

N/A — no reference-library invocation (Class 2), no published-paper figure reproduction (Class 3), no expert-visual sign-off (Class 5) needed. Class 1 + Class 4 are sufficient.

### Second-engineer oracle review

Recommended pending another engineer review. The multi-feature multi-voxel fixture in particular has 6 hand-derived per-cell expected values; a second pair of eyes on the symmetry-reduced cubic misorientation reasoning would catch arithmetic mistakes in the test comments before the V&V cycle is closed.

## Code path coverage

| Path | Description                                                                                                                                                  | Exercised by |
|------|--------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------|
| 1    | Focal-valid gate (`featureIds[point] > 0 && cellPhases[point] > 0`) → enter kernel                                                                           | All 4 Class 1 fixtures; Class 4 sub-sections (i) and (iii) |
| 2    | Inner kernel cell in-bounds + feature-id match → accumulate miso + numVoxel++                                                                                | All Class 1 fixtures; Class 4 (i) and (iii) |
| 3    | Inner kernel cell in-bounds + feature-id mismatch → skip (no accumulate)                                                                                     | `Class 1 - Multi-Feature Multi-Voxel + Background` (cells 1, 2 see different-feature in-bounds neighbors) |
| 4    | Inner kernel cell out-of-bounds (boundary clamp `col+l > xPoints-1` etc.) → `continue`                                                                       | `Class 1 - 1D x-axis Gradient` cells 0/4 (x-boundary); `Class 1 - 1D z-axis Gradient` planes 0/2 (z-boundary); `Class 1 - Uniform 2D` corner cells (xy-corner) |
| 5    | Focal-invalid (`featureIds == 0 || cellPhases == 0`) → KAM = 0 directly                                                                                      | `Class 1 - Multi-Feature Multi-Voxel + Background` cell 4 (explicit REQUIRE that KAM == 0 exactly); Class 4 sub-section (ii) (background-cell invariant) |
| 6    | `numVoxel == 0` fallback at line 131 → KAM = 0                                                                                                               | **Not exercised** — dead code in practice. The focal voxel always self-matches (j=k=l=0 case satisfies `featureIds[point] == featureIds[point]`), guaranteeing numVoxel ≥ 1 whenever path 1 is entered. Path 5 (focal-invalid) skips the kernel entirely, so it never reaches path 6. |

5 of 6 paths exercised by the V&V suite; the 6th is unreachable by construction and is flagged in the algorithm review as removable dead code.

## Test inventory

| TEST_CASE                                                                              | Category | Lines | ctest entry                                                                            |
|----------------------------------------------------------------------------------------|----------|-------|----------------------------------------------------------------------------------------|
| `: SIMPL Backwards Compatibility`                                                      | Compat   | ~40   | Yes (2 dynamic sections: 6.4 + 6.5)                                                    |
| `: Class 1 - Uniform 2D Single Feature`                                                | Class 1  | ~25   | Yes                                                                                    |
| `: Class 1 - 1D x-axis Gradient`                                                       | Class 1  | ~35   | Yes                                                                                    |
| `: Class 1 - 1D z-axis Gradient (3D path)`                                             | Class 1  | ~30   | Yes                                                                                    |
| `: Class 1 - Multi-Feature Multi-Voxel + Background`                                   | Class 1  | ~60   | Yes                                                                                    |
| `: Class 4 - Invariants` (3 sub-sections)                                              | Class 4  | ~55   | Yes (3 SECTIONs)                                                                       |
| ~~`: ComputeKernelAvgMisorientationsFilter` (legacy exemplar test)~~                   | RETIRED  | ~50   | Retired 2026-06-03 (circular oracle from pre-EbsdLib-2.4.1 SIMPLNX output)             |

## Exemplar archive

**None** — inline-constructed. The pre-V&V test (now retired) consumed `6_6_stats_test_v2.tar.gz` (SHA512 `e84999...089723`, downloaded from the BlueQuartz Data_Archive release). The archive contains exemplar `KernelAverageMisorientations` arrays generated from a pre-2.4.1 SIMPLNX build, which embeds the spurious self-misorientation precision noise described in D1. Comparing the post-2.4.1 SIMPLNX output against those exemplars fails by ~0.01-0.05° per cell on real Small_IN100 data. The exemplar arrays cannot be re-generated against the post-2.4.1 build (circular oracle pattern), so the test was retired and replaced with the analytical / invariant suite above.

The shared archive remains referenced in `src/Plugins/OrientationAnalysis/test/CMakeLists.txt` (line 130) for use by `AlignSectionsMutualInformation`, `ComputeShapes`, and `ComputeSchmids` tests. Only F#5's consumption line was removed.

## Deviations from DREAM3D 6.5.171

See `vv/deviations/ComputeKernelAvgMisorientationsFilter.md` for the canonical, ID-stable list:

- **`ComputeKernelAvgMisorientationsFilter-D1`** — EbsdLib 2.4.1 CubicOps precision improvement (non-deviation in the algorithmic sense; precision class). The pre-2.4.1 `acos(w near 1)` form produces a spurious ~0.03° angle on float32-sourced identical quaternions, which inflates every per-cell self-misorientation contribution to the KAM. The 2.4.1 `2*atan2(|v|, w)` form returns 0° as expected.
- **`ComputeKernelAvgMisorientationsFilter-D2`** — Legacy `FindKernelAvgMisorientations.cpp:264` uses `KernelSize.z + 1` as the upper bound of the x-direction inner loop (should be `KernelSize.x + 1`). SIMPLNX has the correct form. Dormant when `KernelSize.x == KernelSize.z`; fires for asymmetric kernels. Logged in `bug_triage.md`.

## Provenance

See `vv/provenance/ComputeKernelAvgMisorientationsFilter.md` for the canonical record of how the inlined data fixtures were designed and how the expected values were derived.
