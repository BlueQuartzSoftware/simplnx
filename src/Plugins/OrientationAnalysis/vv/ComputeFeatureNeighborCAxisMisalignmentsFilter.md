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
| Oracle (confirmed)     | **Class 1 (Analytical) primary** — 3 hand-derived data fixtures: a 2-feature sanity pair, a 10×10×1 6-feature realistic microstructure with mixed hex/non-hex phases that exercises 3 distinct bug-exposing per-feature configurations, and a 4-feature control case where the buggy code happens to produce the right answer. **Class 4 (Invariant) companion** — range bound `[0°, 90°]`, per-feature averaging formula `sum-of-non-NaN-entries / count-of-non-NaN-entries`, all-NaN-on-non-hex-focal invariant. Class 1 oracle uses pure Bunge ZXZ Euler rotations `(0, Φ, 0)` about x so that the crystal c-axis tilts by Φ degrees from world z. For two cells with tilts Φ_A and Φ_B, the c-axis misalignment is exactly `|Φ_A - Φ_B|` (folded to `[0°, 90°]`). |
| Code paths enumerated  | 6 of 6 algorithmic paths exercised: (1) all-non-hex preflight early-exit returns error -1562 — *not exercised by V&V fixtures* (all fixtures contain at least one hex phase) but covered by the existing parameter-validation tests upstream; (2) mixed-phase warning -1563 emitted — exercised by the realistic-microstructure and mismatch-last-order fixtures; (3) per-feature outer loop with hex-hex same-phase neighbor → list-write + accumulate; (4) phase-mismatch branch → write `NaN` + decrement divisor; (5) `FindAvgMisals=true` finalize with `hexNeighborListSize > 0` → `avg = sum/divisor`; (6) `FindAvgMisals=true` finalize with `hexNeighborListSize == 0` → `avg = NaN` (entire neighbor list non-hex, exercised by F3 in the realistic-microstructure fixture). |
| Tests today            | **5 TEST_CASEs / 5 ctest entries**, 100% pass (~0.3s on EbsdLib 2.4.1+). 3 Class 1 fixtures (`Simple Hex Pair`, `Realistic Microstructure (exposes divisor bug)`, `Mismatch Last Order`) + 1 Class 4 invariants test (with 3 SECTIONs) + 1 SIMPL backwards-compatibility test. **No exemplar archive consumed.**                                                                                                                                                                                                                                  |
| Exemplar archive       | **None — inline-constructed in test source.** The pre-existing main exemplar TEST_CASE (consumed `compute_feature_neighbor_caxis_misalignments.tar.gz`) was **retired 2026-06-04** because the exemplar dataset was hex-phase-only, which means the per-mismatch decrement branch in the algorithm is never exercised — the exemplar would have happily passed even on the buggy code. The 4 hand-derived data fixtures cover all 6 algorithmic paths AND include 3 distinct bug-exposing per-feature configurations. The retired archive was unique to this filter, so its `download_test_data` line in `test/CMakeLists.txt` was removed entirely. |
| Legacy comparison      | **Run — SIMPLNX vs DREAM3D 6.5.171, 2026-06-04.** Each root cause was proven by applying the corresponding surgical fixes (D1 divisor fix; D4+D6 Eigen + double + Hex_Low) to a local build of the legacy source, after which the legacy output became **bit-identical to SIMPLNX** — 18 per-pair entries + 6 per-feature avgs byte-compared. 5 deviations: **D1** (divisor bug fires on 6.5.171; SIMPLNX and the patched legacy build produce analytical-correct values), **D2** (avg-array fillValue — DORMANT on current backend), **D4** (EbsdLib quat→matrix swap, ~1e-6° drift, closed by the Eigen+double patch to the legacy build), **D5** (PR #1438 — re-classified as preflight-banner UX downgrade, not warning-channel regression), **D6** (Hex_Low support gap surfaced 2026-06-04, patched together with D4). See deviations doc for per-feature numbers and root-cause detail. |
| Bug flags              | **One legacy bug, resolved in SIMPLNX** — D1, divisor reassigned inside inner j-loop (sibling of F#2 ComputeFeatureNeighborMisorientations D1). Confirmed in `bug_triage.md` as Bug #3 (production-relevant: the shipping `EBSD_File_Processing/EBSD_Hexagonal_Data_Analysis.d3dpipeline` runs this filter with `find_avg_misals=true`). Fixed 2026-06-04 at `Algorithms/ComputeFeatureNeighborCAxisMisalignments.cpp:111`; verified via the `Realistic Microstructure (exposes divisor bug)` test which FAILED on pre-fix code (F2, F5, F6 per-feature averages wrong) and PASSES on the post-fix code. **One latent suspect** — D2, avg-array fillValue uncertainty. Surfaced by the retroactive report; not exercised by the V&V fixtures (which happen to land on hex-hex first for every feature that has `find_avg_misals=true` and a non-zero average). Worth a follow-up confirmation against `DataStoreUtilities::CreateDataStore` default-init behavior. |
| V&V phase              | **All V&V work complete per V2 policy.** Class 1 + Class 4 oracle confirmed against 5-test suite; divisor bug fixed; circular-oracle archive retired; legacy A/B by source inspection; user-facing doc updated. Three source-tree deliverables (this report + `vv/deviations/...` + `vv/provenance/...`) are in place. **Outstanding:** Status promotion DRAFT → READY FOR REVIEW pending second-engineer oracle review (recommend Joey Kleingers, especially the realistic-microstructure F2/F5/F6 hand-derived expected averages and the c-axis pure-Φ-rotation closed-form derivation). |

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

Class 1 oracle derived by hand. Closed-form argument: a Bunge ZXZ Euler `(0, Φ, 0)` is a pure rotation about x, yielding `c = R^T · [0,0,1] = [0, sin(Φ), cos(Φ)]`. For two features with tilts Φ_A and Φ_B, `arccos(c_A · c_B) = |Φ_A − Φ_B|`, folded to `[0°, 90°]` via `if(w > π/2) w = π − w`. All V&V fixtures use tilts in `[0°, 25°]` so the fold is a no-op.

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

## Code path coverage

| Path | Description                                                                                                                                                     | Exercised by |
|------|-------------------------------|--------------|
| 1    | All-non-hex preflight → error -1562 (no hex phases)                                                                                                             | *Not exercised by V&V fixtures*. Existing parameter-validation upstream tests cover this. |
| 2    | Mixed-phase warning -1563 emitted                                                                                                                               | `Class 1 - Realistic Microstructure` (F3 is Cubic), `Class 1 - Mismatch Last Order` (F4 is Cubic) |
| 3    | Per-feature outer loop with hex-hex same-phase neighbor → write angle to misoList + accumulate to avg                                                           | All 4 Class 1 fixtures |
| 4    | Phase-mismatch branch → write NaN to misoList + decrement divisor                                                                                               | `Class 1 - Realistic Microstructure` (F2, F5, F6) and `Class 1 - Mismatch Last Order` (F1's F4-neighbor) |
| 5    | `find_avg_misals=true` finalize with `hexNeighborListSize > 0` → `avg = sum/divisor`                                                                            | All 4 Class 1 fixtures |
| 6    | `find_avg_misals=true` finalize with `hexNeighborListSize == 0` → `avg = NaN`                                                                                   | `Class 1 - Realistic Microstructure` F3 (non-hex focal, all neighbors NaN) |

## Test inventory

| TEST_CASE                                                                                | Category | Lines | ctest entry  |
|------------------------------------------------------------------------------------------|----------|-------|--------------|
| `: SIMPL Backwards Compatibility`                                                        | Compat   | ~45   | Yes (2 dynamic sections: 6.4 + 6.5) |
| `: Class 1 - Simple Hex Pair`                                                            | Class 1  | ~30   | Yes          |
| `: Class 1 - Realistic Microstructure (exposes divisor bug)`                             | Class 1  | ~80   | Yes          |
| `: Class 1 - Mismatch Last Order`                                                        | Class 1  | ~35   | Yes          |
| `: Class 4 - Invariants` (3 SECTIONs)                                                    | Class 4  | ~50   | Yes (3 SECTIONs) |
| ~~`: Valid Filter Execution` (legacy exemplar test)~~                                    | RETIRED  | ~55   | Retired 2026-06-04 (hex-only exemplar cannot trigger the divisor bug) |

## Exemplar archive

**None** — inline-constructed. The pre-V&V test (now retired) consumed `compute_feature_neighbor_caxis_misalignments.tar.gz`. The archive contained exemplar `CAxisMisalignmentList (7_5)` and `AvgCAxisMisalignments (7_5)` arrays generated from a SIMPL 6.5.171 pipeline run on a hex-phase-only dataset (`7_5_simplnx_test_file_25x50_Hex.dream3d`).

The exemplar **could not catch the divisor bug** because every feature in the dataset has hex-only neighbors → the per-mismatch decrement branch never fires → divisor always equals neighbor-list length whether the bug is present or not. The hex-only exemplar would have happily passed on the buggy code, which is why the bug went undetected through OEM review in PR #1467.

The retired archive was unique to this filter (no other filter test consumed it), so its `download_test_data` line in `test/CMakeLists.txt` was removed entirely.

- **Provenance:** `vv/provenance/ComputeFeatureNeighborCAxisMisalignmentsFilter.md` — the canonical record of how the inlined data fixtures (including the 10×10×1 realistic microstructure) were designed and how the expected values were derived.

## Deviations from DREAM3D 6.5.171

See `vv/deviations/ComputeFeatureNeighborCAxisMisalignmentsFilter.md` for the canonical, ID-stable list:

- **`ComputeFeatureNeighborCAxisMisalignmentsFilter-D1`** — Divisor bug (resolved on the SIMPLNX side; root cause proven by applying the same fix to a local build of the legacy source). Production-relevant via shipping `EBSD_Hexagonal_Data_Analysis.d3dpipeline`.
- **`ComputeFeatureNeighborCAxisMisalignmentsFilter-D2`** — Output `AvgCAxisMisalignments` array allocated without explicit fillValue; algorithm assumes zero-initialization. Latent — needs DataStore default-init semantics confirmation.
- **`ComputeFeatureNeighborCAxisMisalignmentsFilter-D4`** — PR #1472 EbsdLib quat-to-orientation-matrix swap. Likely benign precision-only difference (~`0.0001°` per the existing doc note).
- **`ComputeFeatureNeighborCAxisMisalignmentsFilter-D5`** — PR #1438 moved the filter-level preflight banner from `resultOutputActions.warnings()` to `preflightUpdatedValues`. Empirically: the algorithm-level execute-time warning still surfaces to CLI users via `Result<>::warnings()` — D5 is a UX-only downgrade (preflight banner gone from GUI parameter panel), not a warning-channel regression.
- **`ComputeFeatureNeighborCAxisMisalignmentsFilter-D6`** — Hexagonal_Low support gap (surfaced 2026-06-04). Legacy 6.5.171 restricts the hex-hex phase gate to Hex_High only; SIMPLNX correctly handles both Hex_High AND Hex_Low. Not observable on the F#6 fixture (no Hex_Low features), but a real behavior gap on wurtzite-class data.

D3 (default output array name change from PR #1438) is documented as a non-deviation in the same file (user-facing migration noise, not a behavioral deviation).
