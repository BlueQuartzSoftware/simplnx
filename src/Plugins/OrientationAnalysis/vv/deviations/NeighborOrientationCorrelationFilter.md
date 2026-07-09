# Deviations from DREAM3D 6.5.171: NeighborOrientationCorrelationFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent (`NeighborOrientationCorrelation`, SIMPL UUID `6427cd5e-0ad2-5a24-8847-29f8e0720f4f`).

Entries are referenced by stable ID (`NeighborOrientationCorrelationFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

**Comparison summary (2026-07-07):** SIMPLNX vs DREAM3D 6.5.171 on the 12 legacy-native V&V fixtures (engineer's V&V working archive, `VV_Work/NeighborOrientationCorrelationFilter/`). 6.5.171 reproduces the diagnostic `legacy` prediction of the reference implementation on all 12; SIMPLNX (post-fix) reproduces the canonical `intended` oracle on all 12. 5 of 12 fixtures show 6.5.171 vs SIMPLNX differences — exactly the fixtures whose outcome depends on a defect (D1: F05; D2: F04, F10; D3: F03, F12) — while the 7 fixtures whose neighborhoods are fully tied produce identical output in both versions because SIMPLNX's argmax resolves ties to the same (last-in-scan-order) neighbor 6.5.171 picked. D4 (precision) is latent by fixture design. Each root cause was proven by applying the corresponding surgical fix to a local build of the legacy 6.5.171 source, after which the legacy output became **bit-identical to SIMPLNX** on all 12 fixtures.

**Production-scale A/B (2026-07-07):** Small IN100 (4,444,713 cells, 36.6% below the 0.2 confidence threshold, phases {0, 1}), MinConfidence 0.2, tolerance 5 deg, Level 2, identical legacy-native input fed to both runners. SIMPLNX modified 1,115,040 cells (25.09%); 6.5.171 modified 1,022,494 (23.01%). **635,114 cells (14.29% of the volume) differ between the two versions**, decomposed as: 115,380 cells filled only by SIMPLNX (the D2 extra passes reaching deeper into bad regions), 22,834 filled only by 6.5.171 (cascade divergence — different pass-1 sources shift later scan decisions in both directions), and 496,900 of the 999,660 cells modified by both receiving a different replacement (D1/D3 selection differences plus their cascades). The same input through the patched local legacy build reproduced SIMPLNX **bit-for-bit: 0 of 4,444,713 cells differ** — which also bounds D4 (float32 vs float64 misorientation precision) at zero observable effect on this dataset.

---

## NeighborOrientationCorrelationFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `NeighborOrientationCorrelationFilter-D1` |
| **Filter UUID** | `4625c192-7e46-4333-a294-67a2eb64cb37` |
| **Status** | active |

**Symptom:** 6.5.171 can replace a low-confidence cell with the attributes of a *different-phase* neighbor (fixture `F05_stale_w_mixed_phase`: a phase-1 cell is overwritten with phase-2 data), and more generally can count a mixed-phase or phase-0 neighbor pair as "similar" when it should never be.

**Root cause:** Bug in 6.5.171 (stale `w`). In legacy `NeighborOrientationCorrelation::execute()` the misorientation `w` is a function-scope variable computed only inside the same-phase conditional, but the `w < misorientationToleranceR` similarity test runs unconditionally — a mixed-phase/phase-0 pair inherits whatever `w` the previous pair produced. SIMPLNX re-initializes the axis-angle to `max()` before every pair (`Algorithms/NeighborOrientationCorrelation.cpp`). Same defect class as `BadDataNeighborOrientationCheckFilter-D2`.

**Affected users:** Anyone running 6.5.171 on multi-phase (or partially-indexed, phase-0-containing) datasets. Cells at phase boundaries can be filled from the wrong phase; single-phase fully-indexed datasets are unaffected. Small IN100 contains phase-0 (unindexed) cells, so D1 contributes to the measured 14.29% production-scale difference (jointly with D3; the fixtures isolate each cause individually).

**Recommendation:** Trust SIMPLNX. The 6.5.171 behavior is mathematically incorrect. The root cause was proven by resetting `w` in a local build of the legacy source, which eliminated the difference.

---

## NeighborOrientationCorrelationFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `NeighborOrientationCorrelationFilter-D2` |
| **Filter UUID** | `4625c192-7e46-4333-a294-67a2eb64cb37` |
| **Status** | active |

**Symptom:** 6.5.171 runs only half the intended cleanup passes — `ceil((6 − Level)/2)` instead of the original design's `6 − Level` — so multi-pass fills stop early (fixture `F04_multipass_cascade_L2`, Level 2: a 3×3×3 bad region that needs 3 passes is fully filled by SIMPLNX's 4 passes but left with an unfilled center after legacy's 2 passes).

**Root cause:** Bug in 6.5.171 (introduced 2014, DREAM3D commit `0bbeb1d49`). Converting the original 2013 `while(currentLevel > m_Level)` loop to a `for(...; currentLevel--)` loop for progress reporting left the trailing `currentLevel = currentLevel - 1;` in place, decrementing twice per pass. The authoritative statement of intent is the original 2013 implementation, whose single-decrement `while` loop runs exactly `6 − Level` passes (levels 6 down to Level+1). Note that the legacy user documentation's own example ("if the user selects a level of 4, then the filter will run with a level of 6, then 5, then 4") describes `7 − Level` passes and never matched *any* shipped implementation — the doc example was wrong from the filter's birth. SIMPLNX inherited the double decrement at port time; fixed during this V&V to the original `6 − Level` schedule.

**Affected users:** Anyone using *Cleanup Level* ≤ 4 on data where bad regions are more than one erosion layer deep — the filter fills noticeably less than intended. Level 5 datasets (1 pass either way) are unaffected. Measured on Small IN100 (Level 2): the restored passes let SIMPLNX fill 115,380 cells (2.6% of the volume) that 6.5.171 leaves bad.

**Recommendation:** Trust SIMPLNX (post-fix). It implements the documented and originally-intended pass schedule; applying the same fix to a local build of the legacy source reproduces SIMPLNX's output exactly.

---

## NeighborOrientationCorrelationFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `NeighborOrientationCorrelationFilter-D3` |
| **Filter UUID** | `4625c192-7e46-4333-a294-67a2eb64cb37` |
| **Status** | active |

**Symptom:** When a bad cell's neighbors have unequal similarity counts, 6.5.171 copies from the *last* scanned neighbor that has any similar pair, not the *most* similar neighbor (fixture `F03_argmax_vs_lastwins`: a count-1 neighbor beats a count-3 neighbor because it is scanned later).

**Root cause:** Bug in 6.5.171, present since the filter's 2013 introduction. The best-neighbor selection loop resets `best = 0` inside the per-neighbor loop, degrading the intended arg-max (`if(neighborSimCount[j] > best)` with `bestNeighbor` bookkeeping) to "last neighbor with count > 0 wins" — contradicting both the variable's purpose and the documented "replaced with the best neighbor". SIMPLNX inherited the quirk at port time; fixed during this V&V. The corrected argmax resolves ties to the **last** neighbor in the fixed −Z,−Y,−X,+X,+Y,+Z scan order (`>=` with a count > 0 guard), deliberately chosen so that fully-tied neighborhoods — the common case in grain interiors — pick the *same* neighbor as 6.5.171. Migration differences therefore concentrate exclusively where the ranking defect genuinely changed the outcome.

**Affected users:** 6.5.171 replacement decisions where the neighbor similarity counts are unequal and the maximum is not the last positive count in scan order — typically at grain boundaries and noise-cluster edges. Fully-tied neighborhoods (grain interiors) are **not** affected: SIMPLNX's last-of-ties argmax picks the identical neighbor there. Measured on Small IN100: 496,900 of the 999,660 cells modified by both versions (49.7%) received a different replacement (jointly with D1 and cascade effects). **This also changes results relative to prior SIMPLNX/DREAM3D-NX releases**, which inherited the legacy quirk at port time: a pipeline re-run after this fix (with `Level < 5`, where a cell can have unequal neighbor counts) can produce different cleanup than the same pipeline produced on an earlier NX build. This should be called out in the release notes for the version that ships this fix, not only as a legacy-migration note.

**Recommendation:** Trust SIMPLNX (post-fix). The 6.5.171 selection ignores the computed similarity ranking; applying the same fix to a local build of the legacy source reproduces SIMPLNX's output exactly.

---

## NeighborOrientationCorrelationFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `NeighborOrientationCorrelationFilter-D4` |
| **Filter UUID** | `4625c192-7e46-4333-a294-67a2eb64cb37` |
| **Status** | active |

**Symptom:** Latent. A neighbor pair whose misorientation sits within float32 round-off of the tolerance can be classified "similar" by one version and "not similar" by the other, cascading into different replacements.

**Root cause:** Precision. 6.5.171 computes misorientations with `QuatF`/`getMisoQuat` (float32); SIMPLNX uses `ebsdlib::QuatD`/`calculateMisorientation` (float64). Not observable in the V&V fixtures, which keep every pair ≥ 1° away from the 5° tolerance by design; on real EBSD data isolated boundary-straddling pairs may flip.

**Affected users:** Only datasets containing neighbor misorientations within ~10⁻⁵ degrees of the chosen tolerance — rare and physically meaningless differences. Measured bound: on Small IN100 (4.44M cells, ~10⁸ pair comparisons across 4 passes), float64 SIMPLNX and the float32 patched legacy build produced **zero** differing cells.

**Recommendation:** Either acceptable. Double precision is not materially more correct at the tolerance boundary; differences are confined to knife-edge pairs.

---

## Shared characterized behaviors (not deviations — identical in both versions)

- **All cell array types are transferred**: the per-pass transfer copies numeric DataArrays *and* NeighborList and String cell arrays into replaced cells, so every attribute of a replaced cell stays consistent. Matches 6.5.171 (which copies all `AttributeMatrix` arrays). The transfer collects arrays via `findAllChildrenOfType<IArray>` and dispatches the tuple copy by concrete type — `IDataArray::copyTuple` / `INeighborList::copyTuple` / `StringArray::operator[]` (`GenerateTransferArrayList` and `CopyArrayTuple` in `Algorithms/NeighborOrientationCorrelation.cpp`). Verified by the `Oracle F13` unit test.
- **`neighborDiffCount` is dead code**: the count of neighbors *different* from the reference cell is accumulated but never read, in every version since 2013; the documented "at least *Cleanup Level* neighbors must be different than the reference cell" threshold has never been enforced (`currentLevel` never appears in the loop body — the Level parameter only sets the pass count). Documentation corrected during this V&V.
- **`bestNeighbor` persists across passes**: a cell replaced in pass *n* is re-copied from the same neighbor in later passes even if its confidence is now above the threshold. Identical in 6.5.171 and SIMPLNX; mirrored by the reference oracle.
- **Level ≥ 6 means zero passes**: the filter is a no-op at its default parameter value in both versions (invariant I4 in the unit tests).
