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
| Oracle (confirmed)     | **Class 2 (Reference implementation) + Class 1 (Analytical) + Class 4 (Invariant)** — NumPy reference (`reference_noc.py`) with 35 hand-derivation cross-checks; encoded as 12 inline fixtures `Oracle F01`–`F12` + invariant tests in `test/NeighborOrientationCorrelationTest.cpp`, all pass. |
| Code paths enumerated  | 20 of 21 exercised; the cancel path is not directly tested (requires cancel-signal injection). |
| Tests today            | 17 ctest cases — 13 oracle/invariant fixtures + 1 production-scale invariant verification (Small IN100, archive-free snapshot) + 2 preflight validation tests + 1 SIMPL backward-compat (2 DYNAMIC_SECTIONs). |
| Exemplar archive       | **None — fully retired.** All oracle data is inline (programmatic toy fixtures); the Small IN100 test uses archive-free invariant checks. v1 was retired for a hollow comparison — bisect-proven hollow from its 2022-07-24 introduction (`d199bc749`), archive SHA unchanged since first registration (`e34baf1f2`, 2022-12-02); a briefly-created v2 was retired the same day as a circular oracle. |
| Legacy comparison      | **Run — SIMPLNX vs DREAM3D 6.5.171**, fixtures + production scale. Fixtures: 6.5.171 differs on exactly the 5 of 12 whose outcome depends on a defect (D1–D3); the 7 fully-tied fixtures are identical because SIMPLNX's argmax resolves ties to the same last-in-scan-order neighbor 6.5.171 picked. Production (Small IN100, 4.44M cells, Level 2): 14.29% of cells differ, decomposed per deviation entry. Each root cause proven by applying the surgical fix to a local build of the legacy source — then bit-identical to SIMPLNX on all 12 fixtures **and all 4,444,713 production cells** (which also bounds D4 precision at zero observed). |
| Bug flags              | D1 (legacy stale-w), D2 (double level decrement, was also in SIMPLNX — fixed), D3 (last-wins selection, was also in SIMPLNX — fixed). D4 is precision, not a bug. Plus: hollow exemplar comparison in the v1 test (fixed). |
| V&V phase              | All phases complete. Outstanding: second-engineer oracle review at PR; fresh before/after doc screenshots (existing images predate the fixes). |

## Summary

Neighbor Orientation Correlation replaces low-confidence EBSD cells with the attributes of their "best" face neighbor — the one most orientation-similar to the other neighbors — over `6 − Level` passes. It was verified against a NumPy reference implementation with hand-derived (convention-free, co-axial rotation) expected outputs on 12 fixtures covering every logic branch, which exposed and fixed two inherited legacy defects in SIMPLNX (halved pass count; last-wins instead of arg-max selection; the corrected argmax deliberately resolves ties to the same neighbor 6.5.171 picked, confining migration diffs to genuinely defective decisions). Post-fix SIMPLNX matches the oracle exactly on all fixtures and at production scale; five deviations from DREAM3D 6.5.171 are documented — quantified on Small IN100 (14.29% of cells) — each root cause proven by a surgical patch to a local build of the legacy source.

## Algorithm Relationship

**Minor changes** (port with two deliberate correctness deltas)

*Evidence:* SIMPL UUID `6427cd5e-0ad2-5a24-8847-29f8e0720f4f` maps to this filter in `OrientationAnalysisLegacyUUIDMapping.hpp`; the algorithm body is a statement-for-statement translation of legacy `NeighborOrientationCorrelation::execute()` (same 6-face neighbor order `{-XY, -X, -1, +1, +X, +XY}`, same pair-similarity counting, same per-level in-place tuple transfer; as originally ported it also reproduced legacy's last-similar-neighbor-wins selection, corrected during this V&V — deviation D3).

**Port-time deltas:**

1. **Stale-misorientation fix** — legacy computes `w = getMisoQuat(...)` only inside the same-phase conditional but tests `w` against the tolerance unconditionally, so a mixed-phase (or phase-0) pair inherits the previous pair's `w`. SIMPLNX re-initializes the axis-angle to `std::numeric_limits<double>::max()` before every pair. Changes output on mixed-phase datasets (see deviations).
2. **float → double quaternion math** — legacy uses `QuatF`/`getMisoQuat` (float32); SIMPLNX uses `ebsdlib::QuatD`/`calculateMisorientation` (float64, angle at index `[3]` of the returned Axis-Angle `<XYZ>W`). Only affects pairs whose misorientation sits within float rounding of the tolerance.
3. **Parallel transfer stage** — legacy TBB `task_group` over arrays vs SIMPLNX `ParallelTaskAlgorithm` over arrays; both copy tuples per-array in ascending voxel order, so results are identical.
4. **Face-neighbor refactor (PR #1523)** and 2D standardization (PR #1590) — replaced the hand-rolled bounds checks with `NeighborUtilities` (`initializeFaceNeighborOffsets` / `computeValidFaceNeighbors`); offset table and iteration order match legacy exactly.

**Legacy quirks found ported into SIMPLNX** (disposition per this V&V):

- The level loop double-decremented `currentLevel` (loop `currentLevel--` plus a trailing `currentLevel = currentLevel - 1`), introduced in legacy commit `0bbeb1d49` (2014-08-15) when the original 2013 `while(currentLevel > m_Level)` single-decrement loop was converted to a `for` loop for progress reporting — halving the original design's `6 - Level` pass count. **Fixed in SIMPLNX** (deviation D2). Note: the legacy user doc's own example implied `7 - Level` and never matched any shipped code; the 2013 loop is the statement of intent.
- `best = 0` was reset inside the neighbor-selection loop (present since the filter's 2013 birth), so the *last* valid neighbor with any similar pair won, not the arg-max neighbor. **Fixed in SIMPLNX** (deviation D3) with a last-of-ties argmax so fully-tied neighborhoods still match 6.5.171.
- `neighborDiffCount` was accumulated but never read — the once-documented "at least *Cleanup Level* neighbors must ..." threshold has never been enforced by any version; `currentLevel` never appears in the loop body. **Dead code removed** in SIMPLNX; docs corrected instead of changing behavior.
- `bestNeighbor` persists across passes (re-copying previously repaired cells). Identical in both versions; **characterized, kept as-is**, mirrored by the oracle.

**Material PRs since baseline:** #1340 (threaded messaging), #1472 (EbsdLib 2.0 API), #1523 (face-neighbor utilities), #1590 (2D image standardization) — all verified structure-preserving by diff inspection.

## Oracle

*Class:* **2 (Reference implementation)**, with Class 1 (Analytical) and Class 4 (Invariant) companions.

*Applied:* A NumPy reference implementation of the intended algorithm (`reference_noc.py`, NumPy 2.4.2, pinned in the provenance sidecar) computes expected outputs for 12 synthetic fixtures. The intended selection rule is an argmax over neighbor similarity counts with **last-of-ties** resolution (`>=` with a count > 0 guard), deliberately tie-compatible with 6.5.171 (revised 2026-07-07, adversarial-review item A1). All fixture orientations are co-axial rotations about +Z with deltas ≤ 45° (cubic) / ≤ 30° (hex), where every misorientation-fold convention reduces to `|Δθ|` — so each single-pass fixture is *also* hand-derivable (Class 1; `check_derivations.py`, 35 assertions). Quirk toggles in the same script exactly predict pre-fix SIMPLNX and stock 6.5.171 behavior, which is how the defects were localized. Class 4 invariants: high-CI cells never modified (I1), ignored arrays untouched (I2), transfer only copies existing tuples (I3), Level ≥ 6 is a no-op (I4).

*Encoded:* `test/NeighborOrientationCorrelationTest.cpp::Oracle F01..F12` (12 TEST_CASEs) + `::Class 4 - Level >= 6 is a no-op (I4)` + `::Preflight - Level validation` — all pass in in-core and OOC builds. Derivations embedded as comments beside each fixture.

*Second-engineer review:* skipped in-session (single engineer) — requested at PR review; recorded in `vv/provenance/neighbor_orientation_correlation_v2.md`.

## Code path coverage

**20 of 21 paths exercised.** The single gap (cancel path) needs cancel-signal injection and is a
low-value guard exercised implicitly by the GUI.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/NeighborOrientationCorrelation.cpp` (216 lines).

Logical phases: (a) preflight validation, (b) per-pass voxel scan (neighbor-pair similarity
counting + best-neighbor selection), (c) per-pass in-place tuple transfer, repeated `6 − Level` times.

| #  | Phase | Path | Test case |
|----|-------|------|-----------|
| 1  | (a) Preflight | cell arrays with mismatched tuple counts → error `-580093` | `Preflight Error - Cell array tuple count mismatch` |
| 2  | (a) Preflight | `Level < 0` → error `-580094` | `Preflight - Level validation` |
| 3  | (a) Preflight | `Level >= 6` → warning `-580095`, proceed | `Preflight - Level validation` |
| 4  | (a) Preflight | valid inputs → modified-arrays notification, proceed | every oracle fixture |
| 5  | (b) Scan | `CI >= MinConfidence` → cell skipped | all fixtures (invariant I1 checks) |
| 6  | (b) Scan | invalid face neighbor at volume boundary → skipped | `Oracle F11` (corner, 3 valid), `Oracle F07` (2D) |
| 7  | (b) Scan | linear-index → (x,y,z) decomposition on anisotropic dims | `Oracle F12` (4×5×3) |
| 8  | (b) Scan | pair same phase > 0, `w < tol` → similar, both counts credited | `Oracle F01`, `F03`, `F12` |
| 9  | (b) Scan | pair same phase, `w >= tol` → not counted | `Oracle F02` |
| 10 | (b) Scan | pair with mismatched phases → never similar (D1 guard) | `Oracle F05` |
| 11 | (b) Scan | pair involving phase 0 → never similar | `Oracle F06` |
| 12 | (b) Scan | non-cubic LaueOps dispatch (`CrystalStructures = 0`, hex) | `Oracle F08` |
| 13 | (b) Select | all counts zero → `bestNeighbor` stays −1, cell untouched | `Oracle F02` |
| 14 | (b) Select | tied counts → last neighbor in −Z…+Z scan order (6.5.171-compatible) | `Oracle F01`, `F07`, `F11` |
| 15 | (b) Select | unequal counts → arg-max wins, last of maxes (D3 fix) | `Oracle F03`, `F05`, `F06`, `F12` |
| 16 | (c) Transfer | all non-ignored cell arrays copied from best neighbor | `Oracle F01` (all 6 arrays verified) |
| 17 | (c) Transfer | ignored arrays excluded | `Oracle F09` |
| 18 | (b+c) | multi-pass chaining: inherited CI enables next-pass fills (D2 fix) | `Oracle F04` (Level 2), `Oracle F10` (Level 4) |
| 19 | (b+c) | `Level >= 6` → zero passes, output identical to input | `Class 4 - Level >= 6 is a no-op (I4)` |
| 20 | (b) | 2D image (`dims[2] == 1`) degenerate-z validity masks | `Oracle F05`, `F07` |
| 21 | (b+c) | cancel requested → abort scan / abort transfer tasks | *Not directly tested. Requires cancel-signal injection; low-value guard.* |

Non-algorithm coverage (not code paths of the algorithm, tracked in the test inventory):
SIMPL 6.4/6.5 JSON parameter conversion (`SIMPL Backwards Compatibility`, 2 DYNAMIC_SECTIONs)
and the production-scale invariant verification (`Small IN100 Pipeline`, 4.44M cells, Level 2).

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `Small IN100 Pipeline` | kept (modified) | Runs the 6-filter Small IN100 chain + this filter, then verifies the Class 4 invariants at 4.4M cells against an in-memory pre-filter snapshot of all 8 CellData arrays (high-confidence cells untouched everywhere; every modified cell was low-confidence; ≥ 1 cell modified). **Modified for V&V:** the previous exemplar comparison was hollow — bisect-proven hollow from its introduction (`d199bc749`, 2022-07-24: already mapped to `Exemplar Data` with a silent `continue`; archive SHA512 unchanged from `e34baf1f2`, 2022-12-02, to retirement) — and a regenerated exemplar would be a circular oracle, so the archive dependency was removed entirely. |
| `Preflight Error - Cell array tuple count mismatch (-580093)` | kept | Verifies the `validateNumberOfTuples` guard and error code. |
| `Preflight - Level validation (-580094 error, -580095 warning)` | new-for-V&V | 3 SECTIONs: negative Level errors; Level ≥ 6 warns (zero passes); Level < 6 does not warn. |
| `SIMPL Backwards Compatibility` | kept | 2 DYNAMIC_SECTIONs (6.5 UUID / 6.4 Filter_Name), 9 argument checks each. |
| `Oracle F01 - uniform neighbors 3D` | new-for-V&V | Full-tuple snapshot verify of the replacement (+Z, last of 6 tied counts) + I1 on all other cells (6 arrays × 125 cells). |
| `Oracle F02 - dissimilar neighbors untouched` | new-for-V&V | Whole-volume untouched verify; covers the no-similar-pair path. |
| `Oracle F03 - argmax selection (D3 regression)` | new-for-V&V | Count-3 clique vs count-1 pair; the primary pin that the max wins over a later low count. |
| `Oracle F04 - pass schedule Level 2 (D2 regression)` | new-for-V&V | 3×3×3 cascade fully filled in 4 passes; the legacy schedule's 2 passes left the center bad. |
| `Oracle F10 - pass schedule Level 4 (D2 regression)` | new-for-V&V | Same cascade, 2 passes: only the cube center remains unfilled. |
| `Oracle F05 - mixed-phase pair never similar (D1 regression)` | new-for-V&V | Pins the fresh-misorientation behavior; asserts phase never crosses. |
| `Oracle F06 - phase-0 neighbors never counted` | new-for-V&V | Unindexed neighbors excluded from counting and selection. |
| `Oracle F07 - 2D image` | new-for-V&V | Degenerate-z masks; 4-neighbor tie resolves to +Y (last in scan order). |
| `Oracle F08 - hexagonal Laue class` | new-for-V&V | Non-cubic ops dispatch. |
| `Oracle F09 - ignored arrays untouched (I2)` | new-for-V&V | IgnoredDataArrayPaths honored; all other arrays copied. |
| `Oracle F11 - volume corner` | new-for-V&V | 3-valid-neighbor boundary case. |
| `Oracle F12 - anisotropic dims` | new-for-V&V | 4×5×3 (nx≠ny≠nz) so stride/axis-swap bugs cannot hide behind dimension symmetry; secondary D3 pin (last-of-maxes beats a later count-1 pair). |
| `Class 4 - Level >= 6 is a no-op (I4)` | new-for-V&V | Default parameter value performs zero passes (now also surfaced as preflight warning `-580095`). |

All 17 pass at the verified commit in both in-core (`simplnx-Rel`) and OOC (`simplnx-ooc-Rel`) builds.

## Exemplar archive

- **Archive:** None — this filter has no exemplar-archive dependency. All oracle data is
  built inline by the test fixtures, and the production-scale test verifies invariants
  against an in-memory pre-filter snapshot.
- **Retired:** `neighbor_orientation_correlation.tar.gz` (v1 — hollow comparison: the
  container name mismatch caused every array lookup to fail and the silent `continue` to
  skip all comparisons. Bisect-proven hollow from birth: the loop was introduced already
  mapping to `Exemplar Data` in `d199bc749` (2022-07-24) and the archive SHA512 never
  changed from first registration, `e34baf1f2` 2022-12-02, through retirement) and
  `neighbor_orientation_correlation_v2.tar.gz` (created and retired 2026-07-06 — a
  regression pin generated from post-fix SIMPLNX output is a circular oracle, which the
  V&V policy forbids).
- **Provenance (retirement record):** `src/Plugins/OrientationAnalysis/vv/provenance/neighbor_orientation_correlation_v2.md`

## Deviations from DREAM3D 6.5.171

Comparison run (SIMPLNX vs DREAM3D 6.5.171) on all 12 legacy-native V&V fixtures and at production scale (Small IN100, 4,444,713 cells: 14.29% of cells differ; full decomposition in the deviations file); both binaries matched their reference-implementation predictions exactly. Root causes were proven by applying the D1–D3 fixes to a local build of the legacy source, which then reproduced SIMPLNX's output bit-for-bit on all fixtures and on every production cell.

- `NeighborOrientationCorrelationFilter-D1` — 6.5.171 stale-`w` can replace a cell with **different-phase** data — see `vv/deviations/NeighborOrientationCorrelationFilter.md`
- `NeighborOrientationCorrelationFilter-D2` — 6.5.171 (and pre-fix SIMPLNX) ran half the intended cleanup passes (measured: 115,380 Small IN100 cells filled only by SIMPLNX) — see `vv/deviations/NeighborOrientationCorrelationFilter.md`
- `NeighborOrientationCorrelationFilter-D3` — 6.5.171 (and pre-fix SIMPLNX) copied from the last similar neighbor, not the most similar; ties are unaffected by design — see `vv/deviations/NeighborOrientationCorrelationFilter.md`
- `NeighborOrientationCorrelationFilter-D4` — float32 vs float64 misorientation precision (latent; measured zero effect on Small IN100) — see `vv/deviations/NeighborOrientationCorrelationFilter.md`
- `NeighborOrientationCorrelationFilter-D5` — legacy transfers NeighborList/String cell arrays, SIMPLNX transfers only DataArray types — see `vv/deviations/NeighborOrientationCorrelationFilter.md`
