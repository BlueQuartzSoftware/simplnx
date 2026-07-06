# Deviations from DREAM3D 6.5.171: NeighborOrientationCorrelationFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent (`NeighborOrientationCorrelation`, SIMPL UUID `6427cd5e-0ad2-5a24-8847-29f8e0720f4f`).

Entries are referenced by stable ID (`NeighborOrientationCorrelationFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

**Comparison summary (2026-07-06):** three-way A/B/C on the 11 legacy-native V&V fixtures (`neighbor_orientation_correlation_v2` archive). Stock 6.5.171 reproduces the diagnostic `legacy` prediction of the reference implementation on all 11; SIMPLNX (post-fix) and the patched 6.5.172 proof branch reproduce the canonical `intended` oracle on all 11 and are **bit-identical to each other**. 10 of 11 fixtures show 6.5.171 vs SIMPLNX differences, fully explained by D1–D3 below; D4 (precision) is latent by fixture design.

---

## NeighborOrientationCorrelationFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `NeighborOrientationCorrelationFilter-D1` |
| **Filter UUID** | `4625c192-7e46-4333-a294-67a2eb64cb37` |
| **Status** | active |

**Symptom:** 6.5.171 can replace a low-confidence cell with the attributes of a *different-phase* neighbor (fixture `F05_stale_w_mixed_phase`: a phase-1 cell is overwritten with phase-2 data), and more generally can count a mixed-phase or phase-0 neighbor pair as "similar" when it should never be.

**Root cause:** Bug in 6.5.171 (stale `w`). In legacy `NeighborOrientationCorrelation::execute()` the misorientation `w` is a function-scope variable computed only inside the same-phase conditional, but the `w < misorientationToleranceR` similarity test runs unconditionally — a mixed-phase/phase-0 pair inherits whatever `w` the previous pair produced. SIMPLNX re-initializes the axis-angle to `max()` before every pair (`Algorithms/NeighborOrientationCorrelation.cpp`). Same defect class as `BadDataNeighborOrientationCheckFilter-D2`.

**Affected users:** Anyone running 6.5.171 on multi-phase (or partially-indexed, phase-0-containing) datasets. Cells at phase boundaries can be filled from the wrong phase; single-phase fully-indexed datasets are unaffected.

**Recommendation:** Trust SIMPLNX. The 6.5.171 behavior is mathematically incorrect. Legacy-parity proof patch: 6.5.172 proof-branch commit "BUG: NeighborOrientationCorrelation stale-w, double level decrement, best-neighbor argmax".

---

## NeighborOrientationCorrelationFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `NeighborOrientationCorrelationFilter-D2` |
| **Filter UUID** | `4625c192-7e46-4333-a294-67a2eb64cb37` |
| **Status** | active |

**Symptom:** 6.5.171 runs only half the documented cleanup passes — `ceil((6 − Level)/2)` instead of `6 − Level` — so multi-pass fills stop early (fixture `F04_multipass_cascade_L2`, Level 2: a 3×3×3 bad region that needs 3 passes is fully filled by SIMPLNX's 4 passes but left with an unfilled center after legacy's 2 passes).

**Root cause:** Bug in 6.5.171 (introduced 2014, DREAM3D commit `0bbeb1d49`). Converting the original 2013 `while(currentLevel > m_Level)` loop to a `for(...; currentLevel--)` loop for progress reporting left the trailing `currentLevel = currentLevel - 1;` in place, decrementing twice per pass. Both codebases' documentation states "the filter will run with a level of 6, then 5, then 4". SIMPLNX inherited the double decrement at port time; fixed during this V&V.

**Affected users:** Anyone using *Cleanup Level* ≤ 4 on data where bad regions are more than one erosion layer deep — the filter fills noticeably less than documented. Level 5 datasets (1 pass either way) are unaffected.

**Recommendation:** Trust SIMPLNX (post-fix). It implements the documented and originally-intended pass schedule; the same fix is on the 6.5.172 proof branch.

---

## NeighborOrientationCorrelationFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `NeighborOrientationCorrelationFilter-D3` |
| **Filter UUID** | `4625c192-7e46-4333-a294-67a2eb64cb37` |
| **Status** | active |

**Symptom:** When a bad cell's neighbors have unequal similarity counts, 6.5.171 copies from the *last* scanned neighbor that has any similar pair, not the *most* similar neighbor (fixture `F03_argmax_vs_lastwins`: a count-1 neighbor beats a count-3 neighbor because it is scanned later).

**Root cause:** Bug in 6.5.171, present since the filter's 2013 introduction. The best-neighbor selection loop resets `best = 0` inside the per-neighbor loop, degrading the intended arg-max (`if(neighborSimCount[j] > best)` with `bestNeighbor` bookkeeping) to "last neighbor with count > 0 wins" — contradicting both the variable's purpose and the documented "replaced with the best neighbor". SIMPLNX inherited the quirk at port time; fixed during this V&V (ties now resolve to the first neighbor in the fixed −Z,−Y,−X,+X,+Y,+Z scan order, preserving determinism).

**Affected users:** Every 6.5.171 replacement decision where neighbor similarity counts are unequal — common at grain boundaries and noise-cluster edges. In uniform-neighborhood interiors all counts tie and only the tie-break (first vs last neighbor) differs; the copied attributes there are usually indistinguishable.

**Recommendation:** Trust SIMPLNX (post-fix). The 6.5.171 selection ignores the computed similarity ranking; the same fix is on the 6.5.172 proof branch.

---

## NeighborOrientationCorrelationFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `NeighborOrientationCorrelationFilter-D4` |
| **Filter UUID** | `4625c192-7e46-4333-a294-67a2eb64cb37` |
| **Status** | active |

**Symptom:** Latent. A neighbor pair whose misorientation sits within float32 round-off of the tolerance can be classified "similar" by one version and "not similar" by the other, cascading into different replacements.

**Root cause:** Precision. 6.5.171 computes misorientations with `QuatF`/`getMisoQuat` (float32); SIMPLNX uses `ebsdlib::QuatD`/`calculateMisorientation` (float64). Not observable in the V&V fixtures, which keep every pair ≥ 1° away from the 5° tolerance by design; on real EBSD data isolated boundary-straddling pairs may flip.

**Affected users:** Only datasets containing neighbor misorientations within ~10⁻⁵ degrees of the chosen tolerance — rare and physically meaningless differences.

**Recommendation:** Either acceptable. Double precision is not materially more correct at the tolerance boundary; differences are confined to knife-edge pairs.

---

## Shared characterized behaviors (not deviations — identical in both versions)

- **`neighborDiffCount` is dead code**: the count of neighbors *different* from the reference cell is accumulated but never read, in every version since 2013; the documented "at least *Cleanup Level* neighbors must be different than the reference cell" threshold has never been enforced (`currentLevel` never appears in the loop body — the Level parameter only sets the pass count). Documentation corrected during this V&V.
- **`bestNeighbor` persists across passes**: a cell replaced in pass *n* is re-copied from the same neighbor in later passes even if its confidence is now above the threshold. Identical in 6.5.171, 6.5.172-patched, and SIMPLNX; mirrored by the reference oracle.
- **Level ≥ 6 means zero passes**: the filter is a no-op at its default parameter value in both versions (invariant I4 in the unit tests).
