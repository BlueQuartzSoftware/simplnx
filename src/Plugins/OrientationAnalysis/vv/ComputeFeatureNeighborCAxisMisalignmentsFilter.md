# V&V Report: ComputeFeatureNeighborCAxisMisalignmentsFilter

|           |                          |
|-----------|--------------------------|
| Plugin    | OrientationAnalysis      |
| SIMPLNX UUID               | `636ee030-9f07-4f16-a4f3-592eff8ef1ee`                                                                                                           |
| SIMPLNX Human Name         | Compute Feature Neighbor C-Axis Misalignments                                                                                                    |
| DREAM3D 6.5.171 equivalent | `FindFeatureNeighborCAxisMisalignments` — `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindFeatureNeighborCAxisMisalignments.{h,cpp}` (UUID `cdd50b83-ea09-5499-b008-4b253cf4c246`) |
| Verified commit            | *<filled at SBIR deliverable assembly>*                                                                                                          |
| Status | COMPLETE     |
| Sign-off                   | *Michael Jackson <mike.jackson@bluequartz.net> (V&V cycle completion + divisor bug fix, 2026-06-04)*                                             |

## At a glance

| Aspect                 | Current state            |
|------------------------|--------------------------|
| Algorithm Relationship | **Port with one inherited bug corrected** — same outer/inner loop structure + hex-hex phase gate. `QuatF`→`QuatD`; hand-rolled 3×3 matrix math → Eigen; direct `arccos(c1·c2)` (scalar projection, not full crystal miso → not affected by EbsdLib 2.4.1 precision fix). D1 (divisor-loop bug) corrected during this V&V cycle. UUID reassigned; `Find`→`Compute` rename.       |
| Oracle (confirmed) | Class 1 analytical and Class 4 invariants. Three small fixtures retain the independent expected values. The hidden `genuine HDF5 1MiB-chunk tail oracle` checks 15° and 30° across the quaternion chunk boundary. |
| Code paths enumerated | 9 of 12 exercised. No-hex rejection, average-disabled output, and cancellation remain outside the boundary regression. |
| Tests today | 7 registered tests plus 1 hidden OOC contract test. Both serial CTest selections pass 7/7. |
| Exemplar archive       | **None — inline-constructed in test source.** The pre-existing main exemplar TEST_CASE (consumed `compute_feature_neighbor_caxis_misalignments.tar.gz`) was **retired 2026-06-04** because the exemplar dataset was hex-phase-only, which means the per-mismatch decrement branch in the algorithm is never exercised — the exemplar would have happily passed even on the buggy code. The 4 hand-derived data fixtures cover all 6 algorithmic paths AND include 3 distinct bug-exposing per-feature configurations. The retired archive was unique to this filter, so its `download_test_data` line in `test/CMakeLists.txt` was removed entirely. |
| Legacy comparison      | **Run — SIMPLNX vs DREAM3D 6.5.171, 2026-06-04.** Each root cause was proven by applying the corresponding surgical fixes (D1 divisor fix; D4+D6 Eigen + double + Hex_Low) to a local build of the legacy source, after which the legacy output became **bit-identical to SIMPLNX** — 18 per-pair entries + 6 per-feature avgs byte-compared. 5 deviations: **D1** (divisor bug fires on 6.5.171; SIMPLNX and the patched legacy build produce analytical-correct values), **D2** (avg-array fillValue — DORMANT on current backend), **D4** (EbsdLib quat→matrix swap, ~1e-6° drift, closed by the Eigen+double patch to the legacy build), **D5** (PR #1438 — re-classified as preflight-banner UX downgrade, not warning-channel regression), **D6** (Hex_Low support gap surfaced 2026-06-04, patched together with D4). See deviations doc for per-feature numbers and root-cause detail. |
| Bug flags | Existing D1 divisor defect remains fixed. The current implementation explicitly initializes the average buffer to zero, removing the D2 output-initialization dependency. |
| V&V phase | The historical COMPLETE status and sign-off are retained. OOC boundary recertification adds the chunk-tail oracle and dual-build checks; the recorded second-engineer review recommendation remains below. |

## Summary

`ComputeFeatureNeighborCAxisMisalignmentsFilter` computes the **per-feature-pair c-axis misalignment** for every same-phase hexagonal neighbor pair: for each feature, the filter iterates the user-supplied `NeighborList`, looks up each neighbor's average quaternion, computes the c-axis vectors `c_i = R_i^T · [0, 0, 1]` (where `R_i` is the orientation matrix from the average quat), and writes the angle `arccos(c_focal · c_neighbor)` folded to `[0°, 90°]`. Phase mismatches and non-hexagonal Laue classes write `NaN` instead. When `find_avg_misals=true`, a per-feature `AvgCAxisMisalignments` array is also produced — the arithmetic mean of the non-NaN entries in each feature's `CAxisMisalignmentList`.

The filter is the c-axis analog of `ComputeFeatureNeighborMisorientationsFilter`: same outer/inner loop structure, same `find_avg_misals` per-feature aggregation, same `NeighborList<float32>` output shape. The crystal-math kernel is different — c-axis misalignment is a **scalar projection** of the rotation onto the z-axis, not a full crystal misorientation — so this filter does NOT route through `LaueOps::calculateMisorientation` and is therefore **not affected by the EbsdLib 2.4.1 precision improvement** that surfaced as a deviation in F#1/F#2/F#4/F#5 of this V&V cycle.

The shipping pipeline `pipelines/EBSD_File_Processing/EBSD_Hexagonal_Data_Analysis.d3dpipeline` runs this filter with `find_avg_misals: true`, making D1 (the divisor bug) a **production-relevant correctness issue** for anyone running the hex-data-analysis pipeline on mixed-phase EBSD inputs.

## Algorithm Relationship

*Classification:* **Port (with UUID reassignment + name rename + one inherited divisor-bug fix).**

*Evidence:* Cross-checked SIMPLNX `Algorithms/ComputeFeatureNeighborCAxisMisalignments.cpp::operator()()` against legacy `FindFeatureNeighborCAxisMisalignments.cpp::execute()`. Same per-feature outer loop, same per-neighbor inner loop, same hex-hex same-phase gate, same per-feature average with non-hex-decrement of divisor. Port-time deltas:

- `QuatF` → `QuatD` (single-precision → double-precision throughout).
- Hand-rolled 3×3 matrix math (`MatrixMath::Transpose3x3`, `MatrixMath::Multiply3x3with3x1`, `MatrixMath::Normalize3x1`) → Eigen (`Eigen::Vector3d`, `Eigen::Matrix3d`, `.transpose()`, `.normalize()`).
- `GeometryMath::CosThetaBetweenVectors(c1, c2)` → `ImageRotationUtilities::CosBetweenVectors(c1, c2)`.
- `SIMPLibMath::boundF(w, -1, 1)` → `std::clamp(w, -1.0, 1.0)`.
- Quat-to-orientation-matrix: `FOrientTransformsType::qu2om(FOrientArrayType(q), om)` → `ebsdlib::QuaternionDType(q).toOrientationMatrix()` (PR #1472, see D4).
- One legacy divisor bug corrected in SIMPLNX (D1).
- Hex-symmetry crystal-structure warning moved from `resultOutputActions.warnings()` to `preflightUpdatedValues` (PR #1438, see D5).
- Name rename `Find` → `Compute` per platform-wide convention.
- New UUID.

*Material PRs since baseline:*

- **PR #1438** ("Microtexture cleanup") — renamed default output arrays, moved the hex-warning to a GUI-only banner (D5), fixed a `find_avg_misals = false` crash. Did NOT touch the divisor bug.
- **PR #1467** ("OEM-reviewed cleanup") — reviewed and signed off by OEMs on a version that retained the divisor bug. Review focused on naming, comments, structure — not on the inner-loop divisor invariant.
- **PR #1472** ("EbsdLib bump") — swapped two pieces of orientation math (D4).
- **PR #1588** ("SIMPL conversion sweep") — added SIMPL 6.4 + 6.5 conversion test (retained in suite).

## Oracle

*Confirmed class:* **Class 1 (Analytical) primary, Class 4 (Invariant) companion.**

### Class 1 (Analytical)

Class 1 oracle derived by hand. Closed-form argument: a Bunge ZXZ Euler `(0, Φ, 0)` is a pure rotation about x, yielding `c = R^T · [0,0,1] = [0, sin(Φ), cos(Φ)]`. For two features with tilts Φ_A and Φ_B, `arccos(c_A · c_B) = |Φ_A − Φ_B|`, folded to `[0°, 90°]` via `if(w > π/2) w = π − w`. The small fixtures use tilts in `[0°, 25°]`; the chunk-tail fixture extends this to 30°. The fold is a no-op for these fixtures.

**Per-fixture expected outputs:**

| Fixture                                            | Geometry             | Expected per-feature outputs                                                                |
|----------------------------------------------------|----------------------|---------------------------------------------------------------------------------------------|
| `Class 1 - Simple Hex Pair`                        | 1×1×1, 2 features    | `misoList[F1]=[10°], misoList[F2]=[10°], avg[F1]=avg[F2]=10°`                              |
| `Class 1 - Realistic Microstructure`               | 10×10×1, 6 features  | See per-feature table below (3 bug-exposing configurations)                                |
| `Class 1 - Mismatch Last Order`                    | 1×1×1, 4 features    | `misoList[F1]=[5°, 10°, NaN], avg[F1]=7.5°` (buggy code also produces 7.5° — control case) |

Realistic-microstructure expected per-feature outputs (the meaty fixture):

| Feature | Phase | Φ | NeighborList | `misalignmentList[F]`                | divisor | sum | avg (post-fix) | avg (pre-fix bug) |
|---------|-------|----|--------------|--------------------------------------|---------|-----|----------------|-------------------|
| F1      | Hex   | 0° | [F2, F4]     | [5°, 15°]                            | 2       | 20° | **10.000°**    | 10.000° (ok)      |
| F2      | Hex   | 5° | [F1, F3, F4, F5] | [5°, NaN, 10°, 15°]              | 3       | 30° | **10.000°**    | 7.500° (30/4)     |
| F3      | Cubic | —  | [F2, F5, F6] | [NaN, NaN, NaN]                      | 0       | —   | **NaN**        | NaN (ok)          |
| F4      | Hex   | 15°| [F1, F2, F5] | [15°, 10°, 5°]                       | 3       | 30° | **10.000°**    | 10.000° (ok)      |
| F5      | Hex   | 20°| [F2, F3, F4, F6] | [15°, NaN, 5°, 5°]               | 3       | 25° | **8.3333°**    | 6.250° (25/4)     |
| F6      | Hex   | 25°| [F3, F5]     | [NaN, 5°]                            | 1       | 5°  | **5.000°**     | 2.500° (5/2)      |

F2, F5, and F6 are bug-exposing — the pre-fix algorithm reassigned `hexNeighborListSize` on every j-iteration, so the per-mismatch decrement at line 150 was clobbered by the next iteration's reassignment.

### Class 4 (Invariant)

Class 4 invariants asserted in the `Class 4 - Invariants` TEST_CASE across 3 SECTIONs, using the realistic-microstructure fixture:

1. **Range:** every `misalignmentList[F][j]` is either NaN (phase mismatch) or in `[0°, 90°]`. The 90° upper bound is enforced by the algorithm's `if(w > π/2) w = π - w` fold.
2. **Per-feature averaging formula:** for each feature `F`, `avg[F] == (sum of non-NaN entries in misalignmentList[F]) / (count of non-NaN entries in misalignmentList[F])`, or `NaN` if count == 0. **This is the load-bearing invariant for D1 — it failed on F2, F5, F6 under the pre-fix code.**
3. **Non-hex focal feature:** every entry in `misalignmentList[F]` is NaN, and `avg[F]` is NaN. (F3 in the realistic-microstructure fixture has Cubic_High phase.)

### Class 2, 3, 5

N/A — Class 1 + Class 4 are sufficient. No reference library invocation, no published-paper figure reproduction, no expert-visual sign-off needed.

### Second-engineer oracle review

Recommended pending Joey Kleingers or another OA-domain engineer review. Two areas warrant the second pair of eyes:

1. The realistic-microstructure F2/F5/F6 hand-derived expected averages — these are the load-bearing values for the bug-exposing assertion. The neighbor lists and phase assignments are tightly coupled.
2. The closed-form derivation of "pure Bunge ZXZ `(0, Φ, 0)` tilts c-axis by Φ" — straightforward but worth confirming the Bunge convention matches the algorithm's quat-to-orientation-matrix expectation.

## Bugs found and fixed

The current branch retains the divisor correction.

| Deviation | Defect | Affected released versions | Resolution in this branch |
|-----------|--------|----------------------------|---------------------------|
| `ComputeFeatureNeighborCAxisMisalignmentsFilter-D1` | Resetting the divisor inside the neighbor loop counted excluded neighbors in the average. | DREAM.3D 6.5.171; DREAM3D-NX v7.0.0 through v7.4.1. | The divisor is initialized once per feature and decremented for each excluded neighbor. |

## Code path coverage

9 of 12 paths exercised. Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureNeighborCAxisMisalignments.cpp` (190 lines).

| Path | Description                                                                                                                                                     | Exercised by |
|------|-------------------------------|--------------|
| 1    | All-non-hex preflight → error -1562 (no hex phases)                                                                                                             | *Not directly tested. No named current test proves this rejection; it is outside this boundary regression.* |
| 2    | Mixed-phase warning -1563 emitted                                                                                                                               | `Class 1 - Realistic Microstructure` (F3 is Cubic), `Class 1 - Mismatch Last Order` (F4 is Cubic) |
| 3    | Per-feature outer loop with hex-hex same-phase neighbor → write angle to misoList + accumulate to avg                                                           | The three small Class 1 fixtures and the chunk-tail fixture |
| 4    | Phase-mismatch branch → write NaN to misoList + decrement divisor                                                                                               | `Class 1 - Realistic Microstructure` (F2, F5, F6) and `Class 1 - Mismatch Last Order` (F1's F4-neighbor) |
| 5    | `find_avg_misals=true` finalize with `hexNeighborListSize > 0` → `avg = sum/divisor`                                                                            | The three small Class 1 fixtures and the chunk-tail fixture |
| 6    | `find_avg_misals=true` finalize with `hexNeighborListSize == 0` → `avg = NaN`                                                                                   | `Class 1 - Realistic Microstructure` F3 (non-hex focal, all neighbors NaN) |
| 7 | Whole-array cache reads cross the quaternion chunk boundary | `genuine HDF5 1MiB-chunk tail oracle` — 15° and 30° pair values and averages |
| 8 | Invalid current feature phase → error -1564 | `Phase Index Bounds / Current Feature Phase returns an error` |
| 9 | Invalid neighbor phase → error -1564 | `Phase Index Bounds / Neighbor Feature Phase returns an error` |
| 10 | Unequal feature tuple counts → error -1560 | `Preflight Error - Feature array tuple count mismatch (-1560)` |
| 11 | Average output disabled | *Not directly tested by the current analytical fixtures. A minimal follow-up can run Simple Hex Pair with the option off and check the list and absent average.* |
| 12 | Cancellation | *Not directly tested. Requires cancel-signal injection.* |


## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `SIMPL Backwards Compatibility` | kept | SIMPL 6.4 and 6.5 conversion sections. |
| `Class 1 - Simple Hex Pair` | kept | Literal 10° pair and average values. |
| `Class 1 - Realistic Microstructure (exposes divisor bug)` | kept | Mixed-phase values and independent divisor-sensitive averages. |
| `Class 1 - Mismatch Last Order` | kept | Literal [5°, 10°, NaN] and 7.5° average. |
| `Class 4 - Invariants` | kept | Range, average formula, and non-hex NaN sections. |
| `Preflight Error - Feature array tuple count mismatch (-1560)` | kept | Rejects unequal feature tuple counts. |
| `Phase Index Bounds` | kept | Current/neighbor invalid phases and ignored feature zero; uses HDF5 stores when available. |
| `genuine HDF5 1MiB-chunk tail oracle` | new-for-V&V | 65,537 feature quaternions; asserts the store chunk shape is 65,536 and checks 15°/30° pair and average outputs at the boundary and tail. |
| `Valid Filter Execution` | retired | Original hex-only exemplar could not expose the divisor bug; replaced upstream by independent analytical tests. |

OOC recertification (2026-09-18): the upstream/develop analytical assertions remain unchanged. Serial CTest passes 7/7 in both DREAM3DNX builds. The hidden boundary test runs separately in the OOC build because it requires the HDF5-OOC manager. D1 remains corrected. D2 no longer depends on output-store initialization: the current algorithm explicitly initializes its average buffer to zero.

OOC recertification, 2026-09-18: serial CTest passed 7/7 in `NX-Com-Qt69-Vtk96-Rel` and 7/7 in `NX-Com-Qt69-Vtk96-OoC-Rel`. The new hidden boundary case passed 37 assertions in the OOC binary and is included in the OOC-only `OrientationAnalysisOocStoreContracts` CTest entry. The original report status and sign-off above are historical and unchanged.

## Exemplar archive

**None** — inline-constructed. The pre-V&V test (now retired) consumed `compute_feature_neighbor_caxis_misalignments.tar.gz`. The archive contained exemplar `CAxisMisalignmentList (7_5)` and `AvgCAxisMisalignments (7_5)` arrays generated from a SIMPL 6.5.171 pipeline run on a hex-phase-only dataset (`7_5_simplnx_test_file_25x50_Hex.dream3d`).

The exemplar **could not catch the divisor bug** because every feature in the dataset has hex-only neighbors → the per-mismatch decrement branch never fires → divisor always equals neighbor-list length whether the bug is present or not. The hex-only exemplar would have happily passed on the buggy code, which is why the bug went undetected through OEM review in PR #1467.

The retired archive was unique to this filter (no other filter test consumed it), so its `download_test_data` line in `test/CMakeLists.txt` was removed entirely.

- **Provenance:** `vv/provenance/ComputeFeatureNeighborCAxisMisalignmentsFilter.md` — the canonical record of how the inlined data fixtures (including the 10×10×1 realistic microstructure) were designed and how the expected values were derived.

## Deviations from DREAM3D 6.5.171

See `vv/deviations/ComputeFeatureNeighborCAxisMisalignmentsFilter.md` for the canonical, ID-stable list:

- **`ComputeFeatureNeighborCAxisMisalignmentsFilter-D1`** — Divisor bug (resolved on the SIMPLNX side; root cause proven by applying the same fix to a local build of the legacy source). Production-relevant via shipping `EBSD_Hexagonal_Data_Analysis.d3dpipeline`.
- **`ComputeFeatureNeighborCAxisMisalignmentsFilter-D2`** — Output `AvgCAxisMisalignments` array allocated without explicit fillValue; algorithm assumes zero-initialization. The current OOC implementation explicitly zero-initializes its average buffer, so this dependency is removed.
- **`ComputeFeatureNeighborCAxisMisalignmentsFilter-D4`** — PR #1472 EbsdLib quat-to-orientation-matrix swap. Likely benign precision-only difference (~`0.0001°` per the existing doc note).
- **`ComputeFeatureNeighborCAxisMisalignmentsFilter-D5`** — PR #1438 moved the filter-level preflight banner from `resultOutputActions.warnings()` to `preflightUpdatedValues`. Empirically: the algorithm-level execute-time warning still surfaces to CLI users via `Result<>::warnings()` — D5 is a UX-only downgrade (preflight banner gone from GUI parameter panel), not a warning-channel regression.
- **`ComputeFeatureNeighborCAxisMisalignmentsFilter-D6`** — Hexagonal_Low support gap (surfaced 2026-06-04). Legacy 6.5.171 restricts the hex-hex phase gate to Hex_High only; SIMPLNX correctly handles both Hex_High AND Hex_Low. Not observable on the F#6 fixture (no Hex_Low features), but a real behavior gap on wurtzite-class data.

D3 (default output array name change from PR #1438) is documented as a non-deviation in the same file (user-facing migration noise, not a behavioral deviation).
**SIMPLNX-side fix ships in DREAM3D-NX 7.4.2** — the deviation from legacy remains, since 6.5.171 is unchanged: `ComputeFeatureNeighborCAxisMisalignmentsFilter-D1`.
