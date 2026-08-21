# V&V Report: AlignSectionsMisorientationFilter

|                            |                                                                                                 |
|----------------------------|-------------------------------------------------------------------------------------------------|
| Plugin                     | OrientationAnalysis                                                                             |
| SIMPLNX UUID               | `8df2135c-7079-49f4-9756-4f3c028a5ced`                                                          |
| SIMPLNX Human Name         | Align Sections (Misorientation)                                                                 |
| DREAM3D 6.5.171 equivalent | `AlignSectionsMisorientation` — `Source/Plugins/Reconstruction/ReconstructionFilters/AlignSectionsMisorientation.{h,cpp}`, legacy UUID `{4fb2b9de-3124-534b-b914-dbbbdbc14604}` |
| Verified commit            | *<filled at SBIR deliverable assembly>*                                                         |
| Status                     | READY FOR REVIEW                                                                                |
| Sign-off                   | *Michael Jackson <mike.jackson@bluequartz.net> (V&V, 2026-08-21) — second engineer pending*      |

## At a glance

| Aspect                 | Current state                                                                                                                                                                                                                                                                                                                                                                                                                                    |
|------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Algorithm Relationship | **Port (faithful) of the shift search + minor rewrite of the I/O and numeric paths.** The candidate scan, memoization, stride, tie-break and accumulation are line-for-line equivalent to 6.5.171. Three material port-time deltas: shift output moved from a text file to DataArrays (#1237), the misorientation angle moved from float `acos` to double `atan2` via EbsdLib 3.1.0, and the mask type widened from `bool` to `bool \| uint8`. |
| Oracle (confirmed)     | **Class 1 (Analytical) primary, Class 4 (Invariant) companion** — 16 hand-built fixture configurations across 9 TEST_CASEs (7 Class 1 oracle cases plus 2 guard cases), encoded in `src/Plugins/OrientationAnalysis/test/AlignSectionsMisorientationTest.cpp`, `namespace AnalyticalFixtures`; 10 of the 16 assert a hand-derived shift, shift array or aligned volume. All pass. The six oracle cases written in the first oracle commit passed against **unmodified** code on first execution, independently confirming the re-derived sign convention. |
| Code paths enumerated  | **36 of 48 exercised.** The 12 uncovered are 5 cancel-injection paths, 1 runtime-only mask error, 1 candidate-bounds rejection inside deviation D7's out-of-bounds regime, 1 message-text variant of -68008, and 4 pre-existing preflight errors — 2 genuinely unreachable (-68004 and -68001's geometry-not-found branch) and 2 reachable but untested (-68063 and -68001's cell-data branch). Each is its own row with a reason, and `M` is a floor at the stated granularity — see Code path coverage. |
| Tests today            | **11 TEST_CASEs / 51,812 assertions** (per-case counts in Test inventory): 9 new-for-V&V (51,574), 1 kept legacy-parity exemplar (209), 1 untouched SIMPL 6.4/6.5 backwards-compat (29); 1 circular test retired. Coverage shape: 7 Class 1 oracle cases, 2 guard cases, 1 legacy-parity pin, 1 argument-conversion case. Mutation-verified — 19 runs over 16 distinct mutants, 14 killed a test, 5 survived, every survival accounted for under Mutation verification. |
| Exemplar archive        | **`align_sections_misorientation.tar.gz` retained unchanged** (SHA512 verified against CMakeLists). Its `6_6_` file is still the legacy-parity pin and its `output_*.dream3d` file is **still a golden input for two `SimplnxCore::AlignSectionsListFilter` tests** — 3 consumers in total, so the circular-oracle retirement here is scoped to this filter's own tests (follow-up 7). A second archive, `align_sections.tar.gz`, was **retired**: download and sentinel both removed, nothing having read it since #1237. |
| Legacy comparison      | **Run — no output *value* deviations; D1 is the one structural difference observed** (the shift output is DataArrays with a zero tuple 0 rather than a text file). 4 fixtures, 8 binary runs (PipelineRunner 6.5.171 vs `nxrunner`), **95 checks, 0 failures** (log: `ww_work/AlignSectionsMisorientation/ab/ab_comparison_results.txt`), 18 of them element-wise array comparisons that matched element for element with no tolerance of any kind. Every divergence was predicted from source before the runs; **zero unpredicted divergences**. |
| Bug flags              | **D5, D7, D8** — the three deviations whose root cause is classified `bug`. D5 closes an out-of-bounds phase read shared with legacy (-68008, one of the five guards this pass added); D7 is a memoization read before its bounds check, also shared with legacy and deliberately not fixed; D8 is legacy-only, with no SIMPLNX exposure and nothing to fix in SIMPLNX. |
| V&V phase              | Oracle design, RED-first implementation, mutation verification, legacy A/B, documentation and archive retirement are **complete**. **Outstanding:** second-engineer review of the oracle design and the deviation narrative; status promotion to COMPLETE. |

## Summary

`AlignSectionsMisorientationFilter` aligns the sections of a 3D EBSD volume perpendicular to Z
by hill-climbing a 7x7 candidate window of in-plane shifts, choosing for each section the shift
that minimises the fraction of subsampled cell pairs whose misorientation exceeds a
user-supplied tolerance. Verification used a **Class 1 analytical oracle** — expected shifts and
aligned volumes derived in closed form from the algorithm source before anything was run — backed by
a **legacy A/B against DREAM3D 6.5.171** on four fixtures. Headline result: **all 11 TEST_CASEs
pass, every array the A/B compared is element-for-element identical, and the pass added five new
input checks** (errors -68005, -68006, -68007 and -68008, plus the -68009 warning), each detailed
in deviation D5.

## Algorithm Relationship

*Classification:* **Port (faithful)** of the shift search, with a **minor rewrite** of the I/O
and numeric paths.

*Evidence:* SIMPLNX retains a distinct UUID but the same human name, and ships SIMPL 6.4/6.5
conversion fixtures at `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_*/AlignSectionsMisorientationFilter.json`. The
search body is equivalent expression-for-expression: the slice pairing
(`slice = (dims[2]-1) - iter`, reference is the upper section), the `refposition`/`curposition`
index arithmetic, the stride-4 sampling, the `j`-outer/`k`-inner 7x7 scan, the memoization
index, the asymmetric-OR tie-break, and the cumulative accumulation all match legacy
line-for-line. Verified against the 6.5.171 source directly, claim by claim.

*Port-time deltas (each tracked as a Deviation — see `vv/deviations/AlignSectionsMisorientationFilter.md`):*

1. **Shift output: text file -> DataArrays** (D1). Legacy wrote one tab-separated row per
   section pair. SIMPLNX writes three 2-component DataArrays in a new Attribute Matrix, with a
   zero-filled tuple 0 that has no legacy counterpart. Introduced by PR #1237, which also
   bumped parameters version 1 -> 2. Values map one-to-one, legacy row `r` <-> tuple `r+1`;
   verified as such in the A/B.
2. **Misorientation angle: float `2*acos(w)` -> double `2*atan2(|v|, w)`** (D2). Arrived via
   the EbsdLib 2.0.0 API migration (PR #1472) plus the EbsdLib `CubicOps` precision fix that
   ships in the pinned EbsdLib (verified: manifest floor `>=3.1.0`, actually resolved and
   compiled against **3.1.0**). Cubic only; non-cubic Laue classes still use `2*acos(w)` on
   both sides. Affects only the boolean `angle > tolerance`, so it cannot change a shift unless
   a sampled pair sits within ~1e-3 rad of the tolerance.
3. **Mask type: `bool` -> `bool | uint8`** (D4). Strict superset; legacy hard-typed
   `DataArray<bool>` in both the parameter requirement and `dataCheck`.
4. **`IgnoredDataArrayPaths` not ported** (D6). Legacy's member had no `FilterParameter`, so it
   was unreachable from a JSON pipeline and always empty. Both implementations shift every cell
   array in practice — which is why the A/B's array sets matched.
5. **3D-geometry guard lost in the port, restored by this pass** (D5). Legacy rejected any
   dimension <= 1 with `-3010`; the port had no equivalent. Now `-68005`, and unlike legacy it
   returns early rather than letting a later check overwrite the reported code.
6. **Progress/cancel plumbing**: legacy notified per section; SIMPLNX uses a
   `ThrottledMessenger`. No output effect.

*Material PRs since baseline:* `daab6e42c` (#1237, the shift-output rewrite — prime suspect for
output-shape deviations, confirmed as D1), `413e6fa46` (#1472, the EbsdLib 2.0.0 API migration —
origin of D2), `e6896714b` (#1438). **Correction to the research dossier:** #1515 does *not*
touch this filter (it is "Fill Bad Data Out-of-Core Optimization").

## Oracle

*Class:* **1 (Analytical)** primary, **4 (Invariant)** companion.

*Applied:* Each fixture slice carries the same analytic pattern displaced by a per-slice offset
`D_z`. Because the displacement is an exact translation, the candidate shift that makes every
subsampled pair agree is exactly `D_lower - D_upper`, so the true answer scores a mismatch
fraction of **exactly zero** and is provably the unique global minimum. The pattern is built so
that this minimum is also unique and reachable:

- **Sub-voxel resolution.** A plain vertical feature boundary is invisible to a stride-4
  sampling — it aliases and produces 4-way ties. The fixture instead uses a boundary whose x
  position steps one voxel every 4 rows, so across the 8 sampled rows it covers each residue
  mod 4 exactly twice. That yields the closed form `mismatch(ex, 0) = 2 * |ex|` **exactly**,
  which resolves single-voxel shifts.
- **Zigzag, not a monotone staircase.** A monotone staircase satisfies `T(v-4) = T(v) - 1`
  exactly, which makes the candidate at error `(-1, -4)` score identically to the true answer —
  an exact tie. Reversing the step at the midpoint destroys that degeneracy; the report's
  companion derivation shows the tying candidate then scores 2 instead of 0.
- **Y pinned by unindexed cells.** Rows off the sampling lattice are phase 0. A phase-0 cell
  fails the `> 0` test, so its angle stays at `FLT_MAX` and the pair is an unconditional
  mismatch **at any tolerance**. Any candidate with a nonzero y error therefore scores 1.0, the
  maximum. This is tolerance-independent, which is what lets the same fixture be reused for the
  tolerance bracket.

Expected values were derived by hand from the algorithm source before any execution, and the
derivation is reproduced as comments beside each assertion.

The Class 4 companion assertions — properties that hold regardless of the specific expected
values — are: the three shift arrays each have one tuple per section; no alignment attribute
matrix exists when Store Alignment Shifts is off; an all-false mask yields a score of 0 rather
than NaN; the top section is bit-unchanged by the run; and
`UnitTest::CheckArraysInheritTupleDims` holds over the whole DataStructure in every case.

*Encoded:* `src/Plugins/OrientationAnalysis/test/AlignSectionsMisorientationTest.cpp`,
`namespace AnalyticalFixtures` —
**16 fixture configurations across 9 TEST_CASEs** (7 Class 1 oracle cases and 2 guard cases),
**51,574 assertions**, all pass.

**The fixture population this suite runs on**, written down once so that every "fixture" claim in
this report and in `vv/deviations/AlignSectionsMisorientationFilter.md` can be scoped against it:

1. **14 `BuildFixture` configurations** — synthetic, 32x32 in plane, 2 or 3 sections.
2. **2 hand-rolled degenerate-geometry fixtures** — the `Preflight Guards` "degenerate X dimension"
   (1x32x3) and "cell array tuple count" sections; default-initialised arrays, rejected at
   preflight.
3. **1 real-data legacy-parity test** — `AlignSectionsMisorientation Small IN100 Pipeline`, on the
   **189 x 201 x 117** Small IN100 volume at a **5-degree** tolerance.

Items 1 and 2 are the **16 Class 1 fixture configurations**; item 3 is a retained regression pin,
not part of the Class 1 oracle. Unqualified claims about "fixtures" below mean the 16.

The 16 configurations cut two different ways, and both cuts matter:

- **By construction — 14 + 2.** 14 are built by `AnalyticalFixtures::BuildFixture` and carry the
  analytic pattern. The remaining 2 — the `Preflight Guards` "degenerate X dimension" and "cell
  array tuple count" sections — are hand-rolled degenerate geometries with default-initialised
  arrays and no pattern at all, since those guards fire before any pattern is read.
- **By what they assert — 10 + 6.** Only **10** assert a hand-derived shift, shift array or
  aligned volume, and those 10 are the actual Class 1 oracle. The other **6** are diagnostic
  fixtures whose expected value is an error or warning code: the 4 `Preflight Guards`
  sections (-68005 twice, -68006, -68007) and the 2 `Execute Guards` sections (-68008, -68009).
  Four of those 6 are pattern-carrying `BuildFixture` fixtures, which is why the two cuts do not
  line up — they are counting different things, not disagreeing.

*Independent confirmation:* the **six oracle cases added in the first oracle commit**
(`5552cb79c` — every Class 1 oracle case except `Shift Application Without Shift Arrays`, which
`81c0bc7b6` added later) were written against
unmodified code and **passed on their first execution**. Since the expected shifts, including
the sign convention the research dossier had flagged as unverified, were derived from the
source rather than observed, this is a genuine independent check of the derivation and not a
fit to observed output. The A/B then produced the same values from a *third* independent
implementation (DREAM3D 6.5.171).

*Second-engineer review:* **not yet performed — deliberately deferred to PR review**, with the
reason recorded in `vv/provenance/align_sections_misorientation.md` (§Second-engineer oracle
review). This gate is open, which is why Status reads `READY FOR REVIEW` rather than `COMPLETE`.

## Code path coverage

**36 of 48 paths exercised.**

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/AlignSectionsMisorientation.cpp`
(377 lines), plus the filter's `preflightImpl`
(`AlignSectionsMisorientationFilter.cpp`, 337 lines) and the shift application in the shared
base `src/simplnx/Utilities/AlignSections.cpp` (182 lines).

Phases: **(a)** pre-search validation in `operator()`; **(b)** `findShifts` setup; **(c)** the
candidate scan, which exists in **two duplicated copies** (recording / non-recording);
**(d)** accumulation and shift recording; **(e)** shift application in the shared base;
**(f)** preflight.

| #  | Phase | Path | Test case |
|----|-------|------|-----------|
| 1  | (a) | cancel flag already set on entry -> return empty (`:104`) | *Not directly tested. Requires cancel-signal injection.* |
| 2  | (a) | max phase >= crystal-structure tuple count -> error -68008 | `Execute Guards` |
| 3  | (a) | same guard with an **empty** crystal-structure array -> -68008 with the "there are no valid phase values" message text instead of a valid range | *Not directly tested. Requires a zero-tuple CrystalStructures array alongside a positive phase; the message string is the only difference from path 2, which is asserted.* |
| 4  | (a) | indexed phase with an unknown Laue class -> warning -68009, run continues | `Execute Guards` |
| 5  | (a) | all phases valid -> no diagnostic | every oracle case |
| 6  | (b) | UseMask true and mask instantiable -> MaskCompare built | `Class 1 Oracle Mask Semantics` |
| 7  | (b) | UseMask true, mask path invalid at runtime -> error -53900 | *Not directly tested. Preflight makes it unreachable through the IFilter API.* |
| 8  | (b) | UseMask false -> mask branches skipped entirely | `Class 1 Oracle Shift Accumulation And Shift Arrays` |
| 9  | (c) | per-section-pair cancel check -> return (`:173` / `:281`; one row, both copies) | *Not directly tested. Requires cancel-signal injection.* |
| 10 | (c) | candidate already memoized -> skipped | `Class 1 Oracle Multi Hop Convergence` (passes 2 and 3 depend on it) |
| 11 | (c) | candidate outside the halfDim window -> skipped | *Not directly tested. Requires a shift near half the slice width, which is inside deviation D7's out-of-bounds-read regime; a test there would assert undefined behaviour.* |
| 12 | (c) | sampled pair out of bounds -> skipped, not counted | every oracle case with a nonzero shift (drops an edge column) |
| 13 | (c) | sampled pair in bounds -> counted | every oracle case |
| 14 | (c) | mask both-true -> orientation comparison performed | `Class 1 Oracle Mask Semantics` |
| 15 | (c) | mask exclusive-or -> +1 mismatch | `Class 1 Oracle Mask Semantics` |
| 16 | (c) | mask both-false -> counted, no mismatch contribution | `Class 1 Oracle Mask Semantics` (both sections) |
| 17 | (c) | either phase == 0 -> angle stays FLT_MAX -> mismatch | every orientation-carrier oracle case (the y-pin rows) |
| 18 | (c) | same Laue class and in range -> EbsdLib misorientation computed | every orientation-carrier oracle case (never reached in the all-false-mask config, where `bothTrue` never holds) |
| 19 | (c) | different Laue class -> FLT_MAX -> mismatch | `Class 1 Oracle Multi Phase And Cross Laue Class` — **partial**: the case proves cross-Laue pairs do not perturb the argmin, but does **not** assert that they are *counted* as mismatches (mutation M8 survives). See Mutation verification and follow-up 5. |
| 20 | (c) | Laue class out of ops range (999) -> FLT_MAX -> mismatch | `Execute Guards` (unknown-structure section) |
| 21 | (c) | angle > tolerance -> mismatch | `Class 1 Oracle Misorientation Tolerance Bracket` (29 deg) |
| 22 | (c) | angle <= tolerance -> no mismatch | `Class 1 Oracle Misorientation Tolerance Bracket` (31 deg) |
| 23 | (c) | candidate strictly better -> accepted | all oracle cases |
| 24 | (c) | candidate ties and wins the asymmetric OR tie-break | `Misorientation Tolerance Bracket` (31 deg), `Mask Semantics` (all-false) |
| 25 | (c) | candidate strictly worse -> rejected | every orientation-carrier oracle case (never reached in the all-false-mask config, where every candidate scores 0 and none is strictly worse) |
| 26 | (c) | re-centring loop takes another pass | `Class 1 Oracle Multi Hop Convergence` (3 passes) |
| 27 | (c) | re-centring loop converges -> exit | all oracle cases |
| 28 | (d) | cumulative accumulation `xShifts[iter-1] + newxshift` | `Class 1 Oracle Shift Accumulation And Shift Arrays` |
| 29 | (d) | recording branch writes all three arrays at tuple `iter` | `Class 1 Oracle Shift Accumulation And Shift Arrays` |
| 30 | (d) | non-recording branch: same search, no arrays created | `Class 1 Oracle Shift Application Without Shift Arrays` |
| 31 | (d) | shift-array tuple 0 unwritten -> deterministic zeros via fill value | `Class 1 Oracle Shift Accumulation And Shift Arrays` |
| 32 | (e) | non-negative shift -> forward index remap | `Class 1 Oracle Shift Accumulation And Shift Arrays` (section 1, shift (2,0)) |
| 33 | (e) | negative shift -> reversed index remap | `Class 1 Oracle Shift Accumulation And Shift Arrays` (section 0, shift (-1,2)) |
| 34 | (e) | source voxel in bounds -> copyTuple | all volume-asserting oracle cases |
| 35 | (e) | source voxel off-slice -> zero fill | `Shift Accumulation` (both x edges + y edge), `Multi Hop Convergence`, `Hexagonal Laue Class Path` |
| 36 | (e) | top section never modified | `Class 1 Oracle Shift Accumulation And Shift Arrays` |
| 37 | (e) | cancel check inside the per-array transfer loop (`AlignSections.cpp`, `AlignSectionsTransferDataImpl::operator()`, the per-slice `m_Filter->getCancel()` guard at `:52`) | *Not directly tested. Requires cancel-signal injection.* |
| 38 | (e) | cancel check between `findShifts` and the transfer (`AlignSections::execute`, the `getCancel()` guard immediately after the `findShifts` result check) | *Not directly tested. Requires cancel-signal injection.* |
| 39 | (e) | cancel check at the top of each selected-array iteration (`AlignSections::execute`, the `m_ShouldCancel` guard inside the `selectedCellArrays` loop) | *Not directly tested. Requires cancel-signal injection.* |
| 40 | (f) | negative tolerance -> -68007 | `Preflight Guards` |
| 41 | (f) | Quats component count != 4 -> -68004 | *Not directly tested. The parameter's allowed component shape makes it unreachable from a pipeline or the GUI.* |
| 42 | (f) | selected cell arrays disagree with each other -> -68063 | *Not directly tested, and **reachable** — three independently selected cell arrays can disagree. The check is a single shared `DataStructure::validateNumberOfTuples` call used identically by dozens of filters; coverage is deferred to that utility rather than duplicated per filter.* |
| 43 | (f) | selected geometry not found, or not an Image geometry -> -68001 (the `inputGeom == nullptr` branch, `AlignSectionsMisorientationFilter.cpp:198-201`) | *Not directly tested. Genuinely **unreachable**: `GeometrySelectionParameter` (`:89-90`, `AllowedTypes{IGeometry::Type::Image}`) validates both existence and type before `preflightImpl` is entered.* |
| 44 | (f) | selected geometry has no cell-data Attribute Matrix -> -68001 (the `getCellData() == nullptr` branch, `AlignSectionsMisorientationFilter.cpp:203-206`) | *Not directly tested, and **reachable**. `GeometrySelectionParameter` validates existence and type but **not** cell-data presence, and an `ImageGeom` has no cell Attribute Matrix until `setCellData()` is called — a separate step from creating the geometry, as this test file's own fixtures show (`test:191-192` in `BuildFixture`; `test:867-871` in the hand-rolled degenerate-X fixture, where `ImageGeom::Create` and `setCellData` are four statements apart). A pipeline that creates a geometry and never populates its cell AM reaches this branch.* |
| 45 | (f) | geometry not 3D -> -68005 | `Preflight Guards` (2 sections: Z == 1 and X == 1) |
| 46 | (f) | cell array tuple count != geometry cell count -> -68006 | `Preflight Guards` |
| 47 | (f) | StoreAlignmentShifts true -> create AM + 3 arrays with fill value | `Class 1 Oracle Shift Accumulation And Shift Arrays` |
| 48 | (f) | StoreAlignmentShifts false -> no arrays created | `Class 1 Oracle Shift Application Without Shift Arrays` (asserts the AM is absent) |

Line citations in this report and in the test file's comments are to the **head of this branch**.
Where `findShifts` duplicates a construct between its recording and non-recording copies, both
line numbers are given.

**What `M = 48` counts, and what it does not.** Rows are cut at the granularity of a distinct
diagnostic or a distinct control-flow decision that changes the algorithm's output. `M` is
therefore a **floor**, not an exhaustive branch enumeration: finer sub-branches exist inside rows
and are not given their own rows. Two known examples, both inside row 2's `ValidatePhaseData`
scan: the `!sawUnknownStructure` short-circuit at `AlignSectionsMisorientation.cpp:67`, which stops
looking up crystal structures once the first unknown one is recorded (exercised by
`Execute Guards`, unknown-structure section), and the `maxPhase > 0` test at `:79`, which means a
volume whose phases are all zero or negative skips the ensemble-bounds error entirely (not
exercised — no fixture that **reaches `ValidatePhaseData`** has a non-positive maximum phase; the
two hand-rolled guard fixtures do, since `CreateTestDataArray` zero-fills, but preflight rejects
them with -68005/-68006 before the scan runs). Splitting those out would raise `M`
without changing which *diagnostics* and *output-affecting decisions* are pinned by a test, which
is what this table exists to answer.

### Known coverage limitation: the sampling stride

Mutating the sampling stride from 4 to 2 (in either duplicated copy of the scan) is **not**
detected by any Class 1 oracle case. This is a structural property of the oracle design, not an
oversight: because each fixture slice is an *exact translation* of the others, at the true shift
every pair agrees **at any stride**, so the true shift remains the argmin however the lattice is
subsampled. Discriminating the stride would require a fixture that is deliberately *not* a
translation — a fine-scale component voting for one shift and a coarse-scale component voting
for another. The stride is currently pinned only by the retained legacy-parity exemplar, and
only for the non-recording copy of the scan (mutation M3/M3b). Logged as a follow-up.

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `AlignSectionsMisorientation Small IN100 Pipeline` | kept | Legacy-parity pin against the `6_6_` (DREAM3D 6.6-generated) exemplar; 209 assertions over the whole cell attribute matrix. Not an independent oracle. Retained for regression value; the vestigial `align_sections.tar.gz` sentinel was removed from it. Mutation-verified as a real pin: killed by 7 of the 19 mutation runs (M3, M3b, M4, M5, M6, M9, M17) — all of them mutations that reach the non-recording copy of the search, which is the copy this test exercises. |
| `AlignSectionsMisorientationFilter: Class 1 Oracle Shift Accumulation And Shift Arrays` | new-for-V&V | 3-section fixture. Asserts all three shift arrays tuple-by-tuple (including the zero tuple 0) and the entire aligned volume element-by-element. Exercises both index-remap directions and both x zero-fill edges. |
| `... Class 1 Oracle Shift Application Without Shift Arrays` | new-for-V&V | Same fixture with the shift arrays off, to cover the second, duplicated copy of the search. Added *because* mutation testing showed the first copy's coverage did not extend to it. Also asserts no alignment attribute matrix is created. |
| `... Class 1 Oracle Multi Hop Convergence` | new-for-V&V | True shift (4,0) lies outside the first 7x7 window, so the re-centring loop must run 3 passes. Full per-candidate score table derived in the test comment. A single-pass search reports (3,0) and is killed. |
| `... Class 1 Oracle Misorientation Tolerance Bracket` | new-for-V&V | 2 DYNAMIC_SECTIONs at 29 and 31 degrees around a 30-degree fixture disorientation. 29 -> shift (2,0); 31 -> the pattern becomes invisible and the tie-break walk converges to (0,0), traced candidate by candidate in the comment. Kills a degrees/radians error. 30 degrees is deliberately never asserted. |
| `... Class 1 Oracle Mask Semantics` | new-for-V&V | 2 SECTIONs. Structured mask (identity quaternions everywhere, so only the mask can drive the result) and an all-false mask (asserts 0/count == 0, not NaN, and the tie-break walk to (0,0)). |
| `... Class 1 Oracle Multi Phase And Cross Laue Class` | new-for-V&V | 2 SECTIONs. A co-moving cross-Laue-class stripe must not perturb the argmin; a same-Laue-class stripe under a *different phase index* must still be compared, proving dispatch keys on crystal structure rather than phase index. |
| `... Class 1 Oracle Hexagonal Laue Class Path` | new-for-V&V | Hexagonal_High routes through the generic `2*acos(w)` path that the CubicOps precision fix does not touch. 20 degrees about [0001], 15 degrees of margin. |
| `... Preflight Guards` | new-for-V&V | 4 SECTIONs: -68005 twice (Z == 1, X == 1), -68006, -68007. Each asserts exactly one error of the expected code. |
| `... Execute Guards` | new-for-V&V | 2 SECTIONs: -68008 error, -68009 warning with a successful run. |
| `AlignSectionsMisorientationFilter: SIMPL Backwards Compatibility` | kept, untouched | SIMPL 6.4 + 6.5 argument-mapping conversion, 2 DYNAMIC_SECTIONs, 29 assertions. |
| `AlignSectionsMisorientationFilter: output test` | **retired** | Compared the three shift arrays against `output_align_sections_misorientation.dream3d`, a file captured from this filter's own output — a circular oracle that could only confirm whatever the filter did at capture time. Replaced by hand-derived Class 1 assertions on the same three arrays. **Note:** retiring this TEST_CASE closes the circular oracle *for this filter's own tests only*; the same file is still a golden input for two `SimplnxCore::AlignSectionsListFilter` tests (see Exemplar archive and follow-up 7). |

Per-case assertion counts, recounted from `ctest -V -R "OrientationAnalysis::AlignSectionsMisorientation"`
at the branch head (11/11 passed): 209 (Small IN100 legacy-parity), 15,400 (Shift Accumulation),
15,373 (Shift Application Without Shift Arrays), 10,274 (Multi Hop), 63 (Tolerance Bracket),
63 (Mask Semantics), 63 (Multi Phase And Cross Laue), 10,274 (Hexagonal), 41 (Preflight Guards),
23 (Execute Guards), 29 (SIMPL Backwards Compatibility). **Total 51,812; the 9 new-for-V&V cases
account for 51,574.**

All non-retired tests pass at the branch head:

- `ctest -R "OrientationAnalysis::"` -> **301/301 passed, 0 failed** (303 s). This includes the
  `PIPELINE::` and `PY::` chained example-pipeline tests, all of which passed.
- `ctest -R "SimplnxCore::AlignSections"` -> **6/6 passed, 0 failed** (the family
  no-regression run for the SimplnxCore consumers of the shared base).

Both suite runs were executed from `ctest` in the shared build directory after rebuilding
`OrientationAnalysisUnitTest` and `SimplnxCoreUnitTest` at the branch head; the console
transcripts are not archived, so the reproduction command is the evidence pointer. The
per-case counts above are reproducible with the single `ctest -V` invocation named.

**OOC runs waived** per the standing program decision of 2026-08-19.

## Mutation verification

19 mutation runs over **16 distinct mutants**. Each run: apply exactly one textual edit -> rebuild
-> run the blind suite `ctest -R "OrientationAnalysis::AlignSections"` -> revert -> **prove
`git diff` is empty**. All 19 recorded a clean revert. Evidence:
`ww_work/AlignSectionsMisorientation/mutations/` (per-mutation logs plus `COMBINED_SUMMARY.txt`).

Three qualifications on those headline numbers, all verifiable from the logs:

1. **The 19 runs cover 16 distinct mutants.** M2b's edit is byte-identical to M2's, M3b's to M3's,
   and M1's to M16's; each pair was re-run to confirm reproducibility, not to test a new mutant.
   The pairings are visible in the table below.
2. **Run M1 is the one exception to "rebuild, 15 tests".** M1 was produced by the
   first-generation `mutate.sh` harness: its log's build section is **empty** (the harness did not
   record a build result) and it ran **14 tests**, because it predates the
   `Shift Application Without Shift Arrays` case. Every one of the other 18 runs recorded
   `build OK` and ran the full 15-test suite. The M1/M16 row below therefore scopes its no-store
   claim to **M16**: at the time M1 ran, the no-store oracle case did not exist to be spared.
3. **The blind suite's effective detector set is 11 tests, not 15.** All 19 edits were made in
   `AlignSectionsMisorientation.cpp` or `AlignSectionsMisorientationFilter.cpp`, so the 4
   `AlignSectionsMutualInformation` tests in the suite are structurally incapable of detecting
   any of them; they are present as a no-regression check on the shared base, not as detectors.

The blind suite is **15 tests** (14 for run M1, per qualification 2): this filter's 11 plus the 4
`AlignSectionsMutualInformation` tests, which is the other *OrientationAnalysis* consumer of the
shared base. It does **not** include `AlignSectionsList` or `AlignSectionsFeatureCentroid` — those
live in SimplnxCore and are covered instead by the `ctest -R "SimplnxCore::AlignSections"` family
no-regression run reported under Test inventory.

The two duplicated copies of the candidate scan were targeted independently (by occurrence
index) so that each copy's coverage could be established separately.

*Harness caveat, recorded for honesty:* both harnesses infer build success from the absence of
`": error"` in the compiler output rather than from the build's exit status, and prove the revert
with `git diff --stat` (which sees unstaged tracked files only) while printing "working tree
byte-identical to HEAD". The two mutations independently re-run by the review gate used a real
build plus `git status --short` and were clean, so this is harness rigor rather than a result in
doubt.

| ID | Mutation | Outcome |
|---|---|---|
| M1 / M16 | accumulation sign `+` -> `-`, **recording copy** (one edit, run twice) | Killed all 6 recording-copy oracle cases in both runs. Correctly did **not** kill the legacy-parity test, which runs the other copy. **M16 only:** also correctly did not kill the no-store oracle case (also the other copy) — that case did not exist when M1 ran. |
| M17 | accumulation sign `+` -> `-`, **non-recording copy** | Killed the legacy-parity test **and** the no-store oracle case — the latter proving that case earns its place. |
| M2 / M2b | y sampling stride 4 -> 2, recording copy (one edit, run twice) | **Survived.** Explained above: exact-translation fixtures are stride-invariant by construction. |
| M3 / M3b | y sampling stride 4 -> 2, non-recording copy (one edit, run twice) | Killed the legacy-parity test only. Same explanation for the oracle cases. |
| M4 | drop the degrees-to-radians conversion | Killed 7 of 15: the legacy-parity test plus 6 oracle cases. `Mask Semantics` correctly survives — its fixture carries the identity quaternion everywhere, so no angle is ever compared against the tolerance and the conversion is irrelevant to it. |
| M5 | tie-break `||` -> `&&` | Killed the tolerance bracket and the legacy-parity test. |
| M6 | collapse the re-centring loop to a single pass | Killed `Multi Hop Convergence` and the legacy-parity test. |
| M7 | phase-0 skip `> 0` -> `>= 0` | **Survived — equivalent mutant.** Ensemble tuple 0 is 999 by convention, so a phase-0 cell's Laue class fails the ops-range test and the pair is a mismatch either way. Identical outcome by construction, not a coverage gap. |
| M8 | drop the `laueClass1 == laueClass2` half of the dispatch guard | **Survived.** In the cross-Laue fixture the stripe carries the same quaternion as its surroundings, so removing the guard makes those pairs compute 0 degrees and match rather than mismatch — which lowers the score of misaligned candidates without displacing the zero-scoring true answer. The fixture therefore proves cross-Laue pairs *do not perturb the argmin*, but does **not** prove they are counted as mismatches. Stated as a limitation, logged as a follow-up. |
| M9 | remove the mask exclusive-or penalty | Killed `Mask Semantics` and the legacy-parity test. |
| M10 | guard -68005 `<= 1` -> `< 1` | Killed `Preflight Guards`. |
| M11 | guard -68006 `!=` -> `>` | Killed `Preflight Guards`. |
| M12 | guard -68007 threshold -> -1000 | Killed `Preflight Guards`. |
| M13 | guard -68008 max-phase tracking inverted | Killed `Execute Guards`. |
| M14 | guard -68009 flag never set | Killed `Execute Guards`. |
| M15 | shift-array fill value `"0"` -> `""` | **Survived.** Confirms what was expected: tuple 0 was already zero via the data store's default initialization on this platform. The fill value is therefore a **determinism hardening, not an observable bug fix**, and this report makes no claim that it changed behaviour. It removes a dependence on unspecified default-initialization that an out-of-core store need not share. |

**5 of 19 runs survived**, in four groups: **M7** is an equivalent mutant (identical outcome by
construction); **M15** confirms an expected non-observable (the fill value is a hardening, not a
fix); **M2 and M2b** are the two runs of the stride limitation explained above, whose sibling
runs M3/M3b did kill the legacy-parity test; and **M8** is a genuine, stated
assertion-strength limitation. Only M8 represents coverage that could be strengthened without
changing the oracle's design.

### Correction to an earlier inference

An intermediate reading of M1 was that the legacy-parity exemplar test must have all-zero
cumulative X shifts, since flipping the X accumulation sign did not kill it. **That inference
was wrong and is recorded here so it is not repeated.** Direct inspection of the Small IN100
shift arrays shows **103 of 117** cumulative X values are non-zero (range -6 to +2). The actual
reason M1 spared that test is that M1's anchor text was unique to the *recording* copy of the
scan, while the legacy-parity test runs with the shift arrays off and therefore executes the
*other* copy. M17 confirms this: the same mutation applied to the non-recording copy does kill
it.

## Exemplar archive

- **Archive:** `align_sections_misorientation.tar.gz` — retained unchanged, not re-cut.
- **SHA512:** `8a186b2e96dd94a8583eacaec768c252885d89c8f5734b6511d573235beae075971e6e81b42bb517b7cd617fc478ed394abf8ea4fe3188f50d340f90573013f4`
  (verified equal to `src/Plugins/OrientationAnalysis/test/CMakeLists.txt:134`).
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/align_sections_misorientation.md`
- **Three consumers, not one.** Paths below are repo-root relative throughout. The archive is read by:
  1. `OrientationAnalysis::AlignSectionsMisorientation Small IN100 Pipeline`
     (`src/Plugins/OrientationAnalysis/test/AlignSectionsMisorientationTest.cpp:379,386`) — reads
     `6_6_align_sections_misorientation.dream3d`, the legacy-parity pin.
  2. `SimplnxCore::AlignSectionsListFilter: Relative Shifts execution`
     (`src/Plugins/SimplnxCore/test/AlignSectionsListTest.cpp:41,50`) — reads
     `output_align_sections_misorientation.dream3d` and feeds its `Relative Shifts` array into
     `AlignSectionsListFilter`.
  3. `SimplnxCore::AlignSectionsListFilter: Cumulative Shifts execution`
     (`src/Plugins/SimplnxCore/test/AlignSectionsListTest.cpp:109,118`) — same file, its
     `Cumulative Shifts` array.
- **The circular-oracle retirement is scoped to this filter.** Retiring this filter's "output test"
  removed the last place where `AlignSectionsMisorientationFilter` was validated against a capture
  of its own output. It does **not** close the problem repo-wide:
  `output_align_sections_misorientation.dream3d` was captured from this filter's output and remains
  the **golden input** for consumers 2 and 3, so `AlignSectionsListFilter` is still exercised
  against data this filter produced. That is out of this batch's ratified scope
  (`AlignSectionsList` is a separate filter with its own V&V) and is recorded as follow-up 7.
- **Cross-plugin coupling: `src/Plugins/SimplnxCore/test/CMakeLists.txt` has no
  `download_test_data` entry for this archive.** Consumers 2 and 3 free-ride on the
  OrientationAnalysis declaration at
  `src/Plugins/OrientationAnalysis/test/CMakeLists.txt:134`, which is the only place the archive is
  fetched. Two consequences: the archive **cannot** be dropped or re-cut without breaking
  SimplnxCore tests, and a SimplnxCore-only build/test configuration would not fetch it. Recorded
  as follow-up 7.
- **Retired:** `align_sections.tar.gz` — download and in-test sentinel both removed. It was
  downloaded only to satisfy a sentinel expecting a legacy shift *text* file; no test had read
  its contents since #1237 replaced the file output with DataArrays. Confirmed by
  repository-wide grep that the CMake line and the sentinel were its only two references, and
  that `AlignSectionsMutualInformationTest.cpp` does not share it.
- The Class 1 oracle needs **no archive**: all 16 fixture configurations are built in-test.

## Deviations from DREAM3D 6.5.171

**Comparison run.** Legacy `~/Applications/DREAM3D.app/Contents/Bin/PipelineRunner` (6.5.171,
SIMPLib 1.2.828) against `nxrunner` from this branch, over four fixtures written as
legacy-layout `.dream3d` files (`Quats` float32x4 in (x,y,z,w) order, `Phases` int32, `Mask`
`DataArray<bool>`, `CrystalStructures` uint32 = [999, 1]):

| Fixture | Geometry | What it adds | Result |
|---|---|---|---|
| AB1 | 32x32x3 | two chained section pairs, mask off | identical |
| AB2 | 32x32x3 | same input, all-true mask on | identical, and a proven no-op vs AB1 on both sides |
| AB3 | 32x32x2 | true shift (4,0) — forces the 3-pass search | identical; legacy also reports (4,0), not (3,0) |
| AB4 | 32x32x3 | negative relative **and** cumulative shift | identical, including the reversed index-remap branch |

**95 checks, 0 failures**, of which **18 are element-wise array comparisons** (the 3 cell arrays
on each of the 4 fixtures, plus 6 in the AB1-vs-AB2 no-op section). The other 77 are 32
array-presence checks, 12 element-count checks and 33 shift-value/structure checks. Full log:
`ww_work/AlignSectionsMisorientation/ab/ab_comparison_results.txt` (the file also carries a
`RESULT: ALL CHECKS PASSED` banner line, so a naive `grep -c PASS` returns 96).

The comparison predicate (`ab/compare_ab.py`) uses **no tolerance of any kind** — no `allclose`,
no `atol`, no `rtol`. Floating-point arrays are compared as
`abs(a.astype(float64) - b.astype(float64)).max() == 0.0`; integer and boolean arrays as
`(a != b).sum() == 0`. Quats therefore matched with a maximum absolute difference of exactly 0.0
over 12,288 elements on the 3-section fixtures, and Phases and Mask with 0 differing elements.
This is **exact equality on value**; it is deliberately not described as bit-identical, because a
zero max-abs-difference does not distinguish `+0.0` from `-0.0` (immaterial here, since both sides
zero-fill through `T(0)`). Every shift value was identical, with the legacy text row `r` mapping to
SIMPLNX tuple `r+1` and SIMPLNX tuple 0 all zeros. Every predicted value was **frozen in writing
before the runs** (`ww_work/AlignSectionsMisorientation/ab_predictions.md`) and every one was
confirmed; **zero unpredicted divergences**.

Two warnings appear in the `nxrunner` logs and are **harness artefacts, not filter behaviour**:
an optional-python-plugin `ModuleNotFoundError`, and `-5432` reporting that the *writer* filter's
optional `compression_level` key was absent from the hand-written pipeline JSON.

**How strong the pre-registration evidence is.** `ab_predictions.md` is not under version control,
so only mtimes prove ordering. Round 2 (P7, P8, for AB4) is timestamp-proven: the file's mtime is
16:48:57 and the AB4 legacy run log is 16:49:04. Round 1 (P1–P6) is **not** — the same mtime is
about six minutes *after* the AB1–AB3 logs at 16:42:57–58, because appending round 2 rewrote the
whole file, so round 1's ordering is attested rather than proven. What *is* provable for AB1–AB3 is
that their expected shift values were committed to git in
`src/Plugins/OrientationAnalysis/test/AlignSectionsMisorientationTest.cpp` at 16:28:28 and 16:30:59,
ahead of those runs. AB4's expected values were committed 35 s *after* its runs (`2e28f72b9`,
16:49:40), so its ordering rests on the mtime-proven round-2 pre-registration above, not on git.

All mutation and A/B evidence lives under `ww_work/`, outside this repository and this branch's
commit range, so a reviewer working from the commits alone cannot independently verify the 19
mutation runs or the 95 A/B checks; the reproduction commands and file paths are given throughout.

Deviations, all detailed in `vv/deviations/AlignSectionsMisorientationFilter.md`:

- `AlignSectionsMisorientationFilter-D1` — shift output is DataArrays with a zero tuple 0, not a text file (the only difference actually observed in the A/B)
- `AlignSectionsMisorientationFilter-D2` — misorientation angle computed in double via `atan2` rather than float `acos` (precision-bound, no observed effect)
- `AlignSectionsMisorientationFilter-D3` — one-ULP difference in the degrees-to-radians tolerance constant (deliberately not asserted)
- `AlignSectionsMisorientationFilter-D4` — mask type widened to `bool | uint8` (superset)
- `AlignSectionsMisorientationFilter-D5` — five added guards (four errors, one warning), including a restored 3D-geometry guard and the closure of two distinct out-of-bounds read paths; only the phase read (-68008) is classified `bug`, because only it was out of bounds in both implementations
- `AlignSectionsMisorientationFilter-D6` — `IgnoredDataArrayPaths` not ported (unreachable from pipelines in legacy; both shift every cell array)
- `AlignSectionsMisorientationFilter-D7` — memoization array read before its bounds check; shared with legacy, documented, deliberately not fixed
- `AlignSectionsMisorientationFilter-D8` — legacy leaves the shift file truncated when cancelled; no SIMPLNX exposure

No 6.5.172-style patched-legacy build was needed: no shared **output** defect was found, so
there was nothing whose root cause required proving by patching the legacy source.

## Follow-ups for the engineering queue

1. **`findShifts` duplication** — roughly 100 lines are duplicated between the recording and
   non-recording branches. Not refactored here (behaviour-preserving refactors ride separately),
   but this pass demonstrated the hazard concretely: a mutation in one copy is invisible to
   tests that exercise the other, and the first attempt to place a stride mutation aborted
   because the anchor text matched twice.
2. **Guard parity for the sibling filters** — `AlignSectionsList` and
   `AlignSectionsMutualInformation` have the same missing 3D-dimension and
   tuple-count-versus-geometry guards. Per the ratified scope decision, guards were added
   per-filter only and the shared base was left untouched.
3. **Deviation D7** — reorder the memoization read after its bounds check.
4. **Sampling-stride discrimination** — add a deliberately non-translational fixture, as
   described under Code path coverage.
5. **Cross-Laue assertion strength** — mutation M8 survives; give the stripe a distinct
   orientation so that dropping the `laueClass1 == laueClass2` guard changes the argmin.
6. **Cross-file error-code cruft** — the same copy-pasted constant block appears in two other
   filters, but only one of them carries the duplicate. `-68063` is declared **twice** in
   `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ComputeMisorientationsFilter.cpp:33-34`
   (as `k_InconsistentTupleCount` and `k_OutputFilePathEmpty`), which is the same in-file collision
   this pass removed here.
   `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/AlignSectionsFeatureCentroidFilter.cpp:30`
   carries a **single** `-68063` and has no collision — it is listed only because it shares the
   copy-pasted block. Fixed here only; the codes are per-file so there is no live collision across
   files. `-68001` is still used for two distinct geometry errors in this file — geometry not found
   (unreachable) and cell data missing (reachable), coverage rows 43 and 44; both messages are
   self-identifying, and splitting the code was outside the ratified scope.
7. **The circular oracle survives outside this filter, and the archive is cross-plugin coupled.**
   `output_align_sections_misorientation.dream3d` — a capture of *this* filter's output — is still
   the golden input for `SimplnxCore::AlignSectionsListFilter: Relative Shifts execution` and
   `: Cumulative Shifts execution` (`src/Plugins/SimplnxCore/test/AlignSectionsListTest.cpp:41,50`
   and `:109,118`). Retiring it there needs a hand-derived oracle for `AlignSectionsListFilter`,
   which is that filter's own V&V, not this one's. Separately,
   `src/Plugins/SimplnxCore/test/CMakeLists.txt` declares no `download_test_data` for this
   archive; those two tests free-ride on
   `src/Plugins/OrientationAnalysis/test/CMakeLists.txt:134`. Either declare the archive in
   SimplnxCore as well, or fold it into that filter's own archive when its V&V re-cuts one.
8. **AM-12 — the shared base discards a valid-but-warning `findShifts` result.**
   `AlignSections::execute` propagates only an `invalid()` result from `findShifts`
   (`src/simplnx/Utilities/AlignSections.cpp:136-140`) and otherwise returns `{}` (`:167`), so a
   warning raised inside `findShifts` never reaches the user. This pass worked around it for this
   filter by raising the `-68009` warning from `operator()` instead of from `findShifts`; the base
   itself is unchanged. It affects **all four AlignSections consumers**
   (`AlignSectionsMisorientation`, `AlignSectionsMutualInformation`, `AlignSectionsList`,
   `AlignSectionsFeatureCentroid`). Fixing it means touching the shared base, which is outside this
   batch's ratified scope; routed here so it reaches whoever owns the base next.
