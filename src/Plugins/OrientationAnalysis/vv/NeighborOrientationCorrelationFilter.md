# V&V Report: NeighborOrientationCorrelationFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | 4625c192-7e46-4333-a294-67a2eb64cb37 |
| DREAM3D 6.5.171 equivalent | NeighborOrientationCorrelation (SIMPL UUID 6427cd5e-0ad2-5a24-8847-29f8e0720f4f) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | READY FOR REVIEW |
| Sign-off | *<pending second-engineer review>* |

## At a glance

| Aspect                 | Current state                                                                                                                |
|------------------------|------------------------------------------------------------------------------------------------------------------------------|
| Algorithm Relationship | **Minor changes** — statement-for-statement port of legacy `NeighborOrientationCorrelation` with two port-time corrections (stale-w fix, double-precision quats) plus two V&V-cycle bug fixes (pass schedule, argmax selection). |
| Oracle (confirmed)     | **Class 2 (Reference implementation) + Class 1 (Analytical) + Class 4 (Invariant)** — NumPy reference (`reference_noc.py`) with 33 hand-derivation cross-checks; encoded as 11 inline fixtures `Oracle F01`–`F11` + invariant tests in `test/NeighborOrientationCorrelationTest.cpp`, all pass. |
| Code paths enumerated  | 19 of 20 exercised; the cancel path is not directly tested (requires cancel-signal injection). |
| Tests today            | 14 ctest cases — 11 oracle/invariant fixtures + 1 production-scale invariant verification (Small IN100, archive-free snapshot) + 1 preflight error + 1 SIMPL backward-compat (2 DYNAMIC_SECTIONs). |
| Exemplar archive       | **None — fully retired.** All oracle data is inline (programmatic toy fixtures); the Small IN100 test uses archive-free invariant checks. v1 was retired for a hollow comparison (container name mismatch silently skipped every array since 2022); a briefly-created v2 was retired the same day as a circular oracle. |
| Legacy comparison      | **Three-way (SIMPLNX vs 6.5.171 vs 6.5.172)** on 11 legacy-native fixtures — SIMPLNX (fixed) and 6.5.172 (patched, commit `e00baedb0`) are bit-identical and match the oracle; stock 6.5.171 differs on 10 of 11, fully explained by D1–D3. |
| Bug flags              | D1 (legacy stale-w), D2 (double level decrement, was also in SIMPLNX — fixed), D3 (last-wins selection, was also in SIMPLNX — fixed). D4 is precision, not a bug. Plus: hollow exemplar comparison in the v1 test (fixed). |
| V&V phase              | All phases complete. Outstanding: second-engineer oracle review at PR; fresh before/after doc screenshots (existing images predate the fixes). |

## Summary

Neighbor Orientation Correlation replaces low-confidence EBSD cells with the attributes of their "best" face neighbor — the one most orientation-similar to the other neighbors — over `6 − Level` passes. It was verified against a NumPy reference implementation with hand-derived (convention-free, co-axial rotation) expected outputs on 11 fixtures covering every logic branch, which exposed and fixed two inherited legacy defects in SIMPLNX (halved pass count; last-wins instead of arg-max selection). Post-fix SIMPLNX matches the oracle exactly on all fixtures and is bit-identical to the patched 6.5.172 proof build; four deviations from stock 6.5.171 are documented.

## Algorithm Relationship

**Minor changes** (port with two deliberate correctness deltas)

*Evidence:* SIMPL UUID `6427cd5e-0ad2-5a24-8847-29f8e0720f4f` maps to this filter in `OrientationAnalysisLegacyUUIDMapping.hpp`; the algorithm body is a statement-for-statement translation of legacy `NeighborOrientationCorrelation::execute()` (same 6-face neighbor order `{-XY, -X, -1, +1, +X, +XY}`, same pair-similarity counting, same last-similar-neighbor-wins selection, same per-level in-place tuple transfer).

**Port-time deltas:**

1. **Stale-misorientation fix** — legacy computes `w = getMisoQuat(...)` only inside the same-phase conditional but tests `w` against the tolerance unconditionally, so a mixed-phase (or phase-0) pair inherits the previous pair's `w`. SIMPLNX re-initializes the axis-angle to `std::numeric_limits<double>::max()` before every pair. Changes output on mixed-phase datasets (see deviations).
2. **float → double quaternion math** — legacy uses `QuatF`/`getMisoQuat` (float32); SIMPLNX uses `ebsdlib::QuatD`/`calculateMisorientation` (float64, angle at index `[3]` of the returned Axis-Angle `<XYZ>W`). Only affects pairs whose misorientation sits within float rounding of the tolerance.
3. **Parallel transfer stage** — legacy TBB `task_group` over arrays vs SIMPLNX `ParallelTaskAlgorithm` over arrays; both copy tuples per-array in ascending voxel order, so results are identical.
4. **Face-neighbor refactor (PR #1523)** and 2D standardization (PR #1590) — replaced the hand-rolled bounds checks with `NeighborUtilities` (`initializeFaceNeighborOffsets` / `computeValidFaceNeighbors`); offset table and iteration order match legacy exactly.

**Shared legacy quirks faithfully ported** (identical in both implementations; characterized in this V&V):

- The level loop double-decrements `currentLevel` (loop `currentLevel--` plus a trailing `currentLevel = currentLevel - 1`), introduced in legacy commit `0bbeb1d49` (2014-08-15) when the original 2013 `while(currentLevel > m_Level)` single-decrement loop was converted to a `for` loop for progress reporting. Both versions therefore run `ceil((6 - Level)/2)` passes instead of the documented `6 - Level`.
- `neighborDiffCount` is accumulated but never read — the documented "at least *Cleanup Level* neighbors must ..." threshold is never enforced; `currentLevel` never appears in the loop body.
- `best = 0` is reset inside the neighbor-selection loop (present since the filter's 2013 birth), so the *last* valid neighbor with any similar pair wins, not the arg-max neighbor.

**Material PRs since baseline:** #1340 (threaded messaging), #1472 (EbsdLib 2.0 API), #1523 (face-neighbor utilities), #1590 (2D image standardization) — all verified structure-preserving by diff inspection.

## Oracle

*Class:* **2 (Reference implementation)**, with Class 1 (Analytical) and Class 4 (Invariant) companions.

*Applied:* A NumPy reference implementation of the intended algorithm (`reference_noc.py`, NumPy 2.4.2, pinned in the provenance sidecar) computes expected outputs for 11 synthetic fixtures. All fixture orientations are co-axial rotations about +Z with deltas ≤ 45° (cubic) / ≤ 30° (hex), where every misorientation-fold convention reduces to `|Δθ|` — so each single-pass fixture is *also* hand-derivable (Class 1; `check_derivations.py`, 33 assertions). Quirk toggles in the same script exactly predict pre-fix SIMPLNX and stock 6.5.171 behavior, which is how the defects were localized. Class 4 invariants: high-CI cells never modified (I1), ignored arrays untouched (I2), transfer only copies existing tuples (I3), Level ≥ 6 is a no-op (I4).

*Encoded:* `test/NeighborOrientationCorrelationTest.cpp::Oracle F01..F11` (10 TEST_CASEs, one with 2 SECTIONs) + `::Class 4 - Level >= 6 is a no-op (I4)` — all pass in in-core and OOC builds. Derivations embedded as comments beside each fixture.

*Second-engineer review:* skipped in-session (single engineer) — requested at PR review; recorded in `vv/provenance/neighbor_orientation_correlation_v2.md`.

## Code path coverage

**19 of 20 paths exercised.** The single gap (cancel path) needs cancel-signal injection and is a
low-value guard exercised implicitly by the GUI.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/NeighborOrientationCorrelation.cpp` (206 lines).

Logical phases: (a) preflight validation, (b) per-pass voxel scan (neighbor-pair similarity
counting + best-neighbor selection), (c) per-pass in-place tuple transfer, repeated `6 − Level` times.

| #  | Phase | Path | Test case |
|----|-------|------|-----------|
| 1  | (a) Preflight | cell arrays with mismatched tuple counts → error `-580093` | `Preflight Error - Cell array tuple count mismatch` |
| 2  | (a) Preflight | valid inputs → modified-arrays notification, proceed | every oracle fixture |
| 3  | (b) Scan | `CI >= MinConfidence` → cell skipped | all fixtures (invariant I1 checks) |
| 4  | (b) Scan | invalid face neighbor at volume boundary → skipped | `Oracle F11` (corner, 3 valid), `Oracle F07` (2D) |
| 5  | (b) Scan | pair same phase > 0, `w < tol` → similar, both counts credited | `Oracle F01`, `F03` |
| 6  | (b) Scan | pair same phase, `w >= tol` → not counted | `Oracle F02` |
| 7  | (b) Scan | pair with mismatched phases → never similar (D1 guard) | `Oracle F05` |
| 8  | (b) Scan | pair involving phase 0 → never similar | `Oracle F06` |
| 9  | (b) Scan | non-cubic LaueOps dispatch (`CrystalStructures = 0`, hex) | `Oracle F08` |
| 10 | (b) Select | all counts zero → `bestNeighbor` stays −1, cell untouched | `Oracle F02` |
| 11 | (b) Select | tied counts → first neighbor in −Z…+Z scan order | `Oracle F01`, `F07`, `F11` |
| 12 | (b) Select | unequal counts → arg-max wins (D3 fix) | `Oracle F03`, `F05`, `F06` |
| 13 | (c) Transfer | all non-ignored cell arrays copied from best neighbor | `Oracle F01` (all 6 arrays verified) |
| 14 | (c) Transfer | ignored arrays excluded | `Oracle F09` |
| 15 | (b+c) | multi-pass chaining: inherited CI enables next-pass fills (D2 fix) | `Oracle F04/F10 - pass schedule` (both SECTIONs) |
| 16 | (b+c) | `Level >= 6` → zero passes, output identical to input | `Class 4 - Level >= 6 is a no-op (I4)` |
| 17 | (b) | 2D image (`dims[2] == 1`) degenerate-z validity masks | `Oracle F05`, `F07` |
| 18 | (b+c) | cancel requested → abort before transfer | *Not directly tested. Requires cancel-signal injection; low-value guard.* |
| 19 | — | SIMPL 6.4/6.5 JSON parameter conversion | `SIMPL Backwards Compatibility` (2 DYNAMIC_SECTIONs) |
| 20 | — | production-scale invariant verification (Small IN100, 4.4M cells, Level 2) | `Small IN100 Pipeline` |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `Small IN100 Pipeline` | kept (modified) | Runs the 6-filter Small IN100 chain + this filter, then verifies the Class 4 invariants at 4.4M cells against an in-memory pre-filter snapshot of all 8 CellData arrays (high-confidence cells untouched everywhere; every modified cell was low-confidence; ≥ 1 cell modified). **Modified for V&V:** the previous exemplar comparison was hollow (v1 archive container name mismatch → silent skip of every array since 2022) and its replacement would have been a circular oracle, so the exemplar dependency was removed entirely. |
| `Preflight Error - Cell array tuple count mismatch (-580093)` | kept | Verifies the `validateNumberOfTuples` guard and error code. |
| `SIMPL Backwards Compatibility` | kept | 2 DYNAMIC_SECTIONs (6.5 UUID / 6.4 Filter_Name), 9 argument checks each. |
| `Oracle F01 - uniform neighbors 3D` | new-for-V&V | Full-tuple snapshot verify of the replacement + I1 on all other cells (6 arrays × 125 cells). |
| `Oracle F02 - dissimilar neighbors untouched` | new-for-V&V | Whole-volume untouched verify; covers the no-similar-pair path. |
| `Oracle F03 - argmax selection (D3 regression)` | new-for-V&V | Count-3 clique vs count-1 pair; pins the arg-max fix. |
| `Oracle F04/F10 - pass schedule (D2 regression)` | new-for-V&V | 2 SECTIONs (Level 2 / Level 4); pins the single-decrement fix via erosion depth. |
| `Oracle F05 - mixed-phase pair never similar (D1 regression)` | new-for-V&V | Pins the fresh-misorientation behavior; asserts phase never crosses. |
| `Oracle F06 - phase-0 neighbors never counted` | new-for-V&V | Unindexed neighbors excluded from counting and selection. |
| `Oracle F07 - 2D image` | new-for-V&V | Degenerate-z masks; 4-neighbor tie-break. |
| `Oracle F08 - hexagonal Laue class` | new-for-V&V | Non-cubic ops dispatch. |
| `Oracle F09 - ignored arrays untouched (I2)` | new-for-V&V | IgnoredDataArrayPaths honored; all other arrays copied. |
| `Oracle F11 - volume corner` | new-for-V&V | 3-valid-neighbor boundary case. |
| `Class 4 - Level >= 6 is a no-op (I4)` | new-for-V&V | Default parameter value performs zero passes. |

All 14 pass at the verified commit in both in-core (`simplnx-Rel`) and OOC (`simplnx-ooc-Rel`) builds.

## Exemplar archive

- **Archive:** None — this filter has no exemplar-archive dependency. All oracle data is
  built inline by the test fixtures, and the production-scale test verifies invariants
  against an in-memory pre-filter snapshot.
- **Retired:** `neighbor_orientation_correlation.tar.gz` (v1 — hollow comparison: container
  name mismatch caused every array lookup to fail and the silent `continue` to skip all
  comparisons since 2022) and `neighbor_orientation_correlation_v2.tar.gz` (created and
  retired 2026-07-06 — a regression pin generated from post-fix SIMPLNX output is a
  circular oracle, which the V&V policy forbids).
- **Provenance (retirement record):** `src/Plugins/OrientationAnalysis/vv/provenance/neighbor_orientation_correlation_v2.md`

## Deviations from DREAM3D 6.5.171

Comparison run three-way (stock 6.5.171 / patched 6.5.172 `e00baedb0` / fixed SIMPLNX) on all 11 legacy-native V&V fixtures; every binary matched its reference-implementation prediction exactly.

- `NeighborOrientationCorrelationFilter-D1` — 6.5.171 stale-`w` can replace a cell with **different-phase** data — see `vv/deviations/NeighborOrientationCorrelationFilter.md`
- `NeighborOrientationCorrelationFilter-D2` — 6.5.171 (and pre-fix SIMPLNX) ran half the documented cleanup passes — see `vv/deviations/NeighborOrientationCorrelationFilter.md`
- `NeighborOrientationCorrelationFilter-D3` — 6.5.171 (and pre-fix SIMPLNX) copied from the last similar neighbor, not the most similar — see `vv/deviations/NeighborOrientationCorrelationFilter.md`
- `NeighborOrientationCorrelationFilter-D4` — float32 vs float64 misorientation precision (latent) — see `vv/deviations/NeighborOrientationCorrelationFilter.md`
