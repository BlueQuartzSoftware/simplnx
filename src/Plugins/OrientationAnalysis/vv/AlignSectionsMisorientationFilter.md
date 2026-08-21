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
| Oracle (confirmed)     | **Class 1 (Analytical) primary, Class 4 (Invariant) companion** — 16 hand-built analytical fixture configurations across 9 TEST_CASEs, expected shifts and expected aligned volumes derived in closed form from the algorithm source before any run. All pass. The key structural result: the six shift-asserting oracle cases passed against **unmodified** code on first execution, independently confirming the re-derived sign convention. |
| Code paths enumerated  | **36 of 43 exercised.** The 7 uncovered are 2 cancel-injection paths, 1 runtime-only mask error, 1 candidate-bounds rejection that is only reachable inside a documented out-of-bounds regime, and 3 pre-existing preflight errors made unreachable by parameter validation. Each is listed as its own row with a reason.                                                                                                       |
| Tests today            | **11 TEST_CASEs / 51,803 assertions.** 9 new-for-V&V (51,565 assertions): 7 Class 1 oracle cases + 2 guard cases. 1 kept legacy-parity exemplar (209). 1 untouched SIMPL 6.4/6.5 backwards-compat (29). 1 retired (circular). Mutation-verified: 19 mutation runs, 14 killed a test, 5 survived — every survival is accounted for below (1 equivalent mutant, 1 expected non-observable, 2 runs of one explained stride limitation, 1 stated assertion-strength gap). |
| Exemplar archive        | **`align_sections_misorientation.tar.gz` retained unchanged** (SHA512 verified against CMakeLists). Its `6_6_` file is still the legacy-parity pin; its `output_*.dream3d` file is no longer read by any test. A second archive, `align_sections.tar.gz`, was **retired** — download and sentinel both removed; nothing had read it since #1237. Provenance: `vv/provenance/align_sections_misorientation.md`. |
| Legacy comparison      | **Run — no output deviations.** 4 fixtures, 8 binary runs (PipelineRunner 6.5.171 vs `nxrunner`), 96 element-wise checks, 0 failures. Every cell array bit-identical (Quats to 0.0 max abs diff over 12,288 elements); every shift value identical, including negative relative and cumulative shifts and the 3-pass multi-hop search. All divergences were predicted from source before the runs; **zero unpredicted divergences**. |
| Bug flags              | **D5** (a restored 3D guard plus four new guards, converting silent garbage and two out-of-bounds reads into diagnostics — the out-of-bounds phase read was shared with legacy) and **D7** (memoization array read before its bounds check — shared with legacy, documented, deliberately not fixed). **D8** is a legacy-only defect with no SIMPLNX exposure. |
| V&V phase              | Oracle design, RED-first implementation, mutation verification, legacy A/B, documentation and archive retirement are **complete**. **Outstanding:** second-engineer review of the oracle design and the deviation narrative; status promotion to COMPLETE. |

## Summary

`AlignSectionsMisorientationFilter` aligns the sections of a 3D EBSD volume perpendicular to Z
by hill-climbing a 7x7 candidate window of in-plane shifts, choosing for each section the shift
that minimises the fraction of subsampled cell pairs whose misorientation exceeds a
user-supplied tolerance. Verification used a **Class 1 analytical oracle**: 16 hand-built
fixture configurations whose expected shifts, shift arrays and aligned volumes were derived in
closed form from the algorithm source before anything was run, backed by a **legacy A/B against
DREAM3D 6.5.171** on four fixtures. Headline result: **all 11 TEST_CASEs pass and the A/B is
bit-identical with zero unpredicted divergences.** The pass also added five new input checks —
four errors and one warning — one each for: two out-of-bounds read paths reachable from
ordinary user input (-68006, -68008), a 3D-geometry guard that legacy had and the port had lost
(-68005), a parameter value that silently produced meaningless output (-68007), and a crystal
structure that was silently ignored (-68009).

## Algorithm Relationship

*Classification:* **Port (faithful)** of the shift search, with a **minor rewrite** of the I/O
and numeric paths. ~~Rewrite | New filter~~

*Evidence:* SIMPLNX retains a distinct UUID but the same human name, and ships SIMPL 6.4/6.5
conversion fixtures at `test/simpl_conversion/6_*/AlignSectionsMisorientationFilter.json`. The
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

*Encoded:* `test/AlignSectionsMisorientationTest.cpp`, `namespace AnalyticalFixtures` —
**16 fixture configurations across 9 TEST_CASEs**, 51,565 assertions, all pass.

*Independent confirmation:* the six shift-asserting oracle cases were written against
unmodified code and **passed on their first execution**. Since the expected shifts, including
the sign convention the research dossier had flagged as unverified, were derived from the
source rather than observed, this is a genuine independent check of the derivation and not a
fit to observed output. The A/B then produced the same values from a *third* independent
implementation (DREAM3D 6.5.171).

*Second-engineer review:* **pending** — to be performed at PR review.

## Code path coverage

**36 of 43 paths exercised.**

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/AlignSectionsMisorientation.cpp`
(368 lines), plus the filter's `preflightImpl`
(`AlignSectionsMisorientationFilter.cpp`, 337 lines) and the shift application in the shared
base `src/simplnx/Utilities/AlignSections.cpp` (182 lines).

Phases: **(a)** pre-search validation in `operator()`; **(b)** `findShifts` setup; **(c)** the
candidate scan, which exists in **two duplicated copies** (recording / non-recording);
**(d)** accumulation and shift recording; **(e)** shift application in the shared base;
**(f)** preflight.

| #  | Phase | Path | Test case |
|----|-------|------|-----------|
| 1  | (a) | cancel flag already set on entry -> return empty | *Not directly tested. Requires cancel-signal injection.* |
| 2  | (a) | max phase >= crystal-structure tuple count -> error -68008 | `Execute Guards` |
| 3  | (a) | indexed phase with an unknown Laue class -> warning -68009, run continues | `Execute Guards` |
| 4  | (a) | all phases valid -> no diagnostic | every oracle case |
| 5  | (b) | UseMask true and mask instantiable -> MaskCompare built | `Class 1 Oracle Mask Semantics` |
| 6  | (b) | UseMask true, mask path invalid at runtime -> error -53900 | *Not directly tested. Preflight makes it unreachable through the IFilter API.* |
| 7  | (b) | UseMask false -> mask branches skipped entirely | `Class 1 Oracle Shift Accumulation And Shift Arrays` |
| 8  | (c) | per-section-pair cancel check -> return | *Not directly tested. Requires cancel-signal injection.* |
| 9  | (c) | candidate already memoized -> skipped | `Class 1 Oracle Multi Hop Convergence` (passes 2 and 3 depend on it) |
| 10 | (c) | candidate outside the halfDim window -> skipped | *Not directly tested. Requires a shift near half the slice width, which is inside deviation D7's out-of-bounds-read regime; a test there would assert undefined behaviour.* |
| 11 | (c) | sampled pair out of bounds -> skipped, not counted | every oracle case with a nonzero shift (drops an edge column) |
| 12 | (c) | sampled pair in bounds -> counted | every oracle case |
| 13 | (c) | mask both-true -> orientation comparison performed | `Class 1 Oracle Mask Semantics` |
| 14 | (c) | mask exclusive-or -> +1 mismatch | `Class 1 Oracle Mask Semantics` |
| 15 | (c) | mask both-false -> counted, no mismatch contribution | `Class 1 Oracle Mask Semantics` (both sections) |
| 16 | (c) | either phase == 0 -> angle stays FLT_MAX -> mismatch | every orientation-carrier oracle case (the y-pin rows) |
| 17 | (c) | same Laue class and in range -> EbsdLib misorientation computed | all oracle cases |
| 18 | (c) | different Laue class -> FLT_MAX -> mismatch | `Class 1 Oracle Multi Phase And Cross Laue Class` |
| 19 | (c) | Laue class out of ops range (999) -> FLT_MAX -> mismatch | `Execute Guards` (unknown-structure section) |
| 20 | (c) | angle > tolerance -> mismatch | `Class 1 Oracle Misorientation Tolerance Bracket` (29 deg) |
| 21 | (c) | angle <= tolerance -> no mismatch | `Class 1 Oracle Misorientation Tolerance Bracket` (31 deg) |
| 22 | (c) | candidate strictly better -> accepted | all oracle cases |
| 23 | (c) | candidate ties and wins the asymmetric OR tie-break | `Misorientation Tolerance Bracket` (31 deg), `Mask Semantics` (all-false) |
| 24 | (c) | candidate worse -> rejected | all oracle cases |
| 25 | (c) | re-centring loop takes another pass | `Class 1 Oracle Multi Hop Convergence` (3 passes) |
| 26 | (c) | re-centring loop converges -> exit | all oracle cases |
| 27 | (d) | cumulative accumulation `xShifts[iter-1] + newxshift` | `Class 1 Oracle Shift Accumulation And Shift Arrays` |
| 28 | (d) | recording branch writes all three arrays at tuple `iter` | `Class 1 Oracle Shift Accumulation And Shift Arrays` |
| 29 | (d) | non-recording branch: same search, no arrays created | `Class 1 Oracle Shift Application Without Shift Arrays` |
| 30 | (d) | shift-array tuple 0 unwritten -> deterministic zeros via fill value | `Class 1 Oracle Shift Accumulation And Shift Arrays` |
| 31 | (e) | non-negative shift -> forward index remap | `Class 1 Oracle Shift Accumulation And Shift Arrays` (section 1, shift (2,0)) |
| 32 | (e) | negative shift -> reversed index remap | `Class 1 Oracle Shift Accumulation And Shift Arrays` (section 0, shift (-1,2)) |
| 33 | (e) | source voxel in bounds -> copyTuple | all volume-asserting oracle cases |
| 34 | (e) | source voxel off-slice -> zero fill | `Shift Accumulation` (both x edges + y edge), `Multi Hop Convergence`, `Hexagonal Laue Class Path` |
| 35 | (e) | top section never modified | `Class 1 Oracle Shift Accumulation And Shift Arrays` |
| 36 | (f) | negative tolerance -> -68007 | `Preflight Guards` |
| 37 | (f) | Quats component count != 4 -> -68004 | *Not directly tested. The parameter's allowed component shape makes it unreachable from a pipeline or the GUI.* |
| 38 | (f) | selected cell arrays disagree with each other -> -68063 | *Not directly tested. Pre-existing path, unchanged by this pass.* |
| 39 | (f) | geometry missing, or has no cell data -> -68001 | *Not directly tested. The geometry selection parameter validates existence.* |
| 40 | (f) | geometry not 3D -> -68005 | `Preflight Guards` (2 sections: Z == 1 and X == 1) |
| 41 | (f) | cell array tuple count != geometry cell count -> -68006 | `Preflight Guards` |
| 42 | (f) | StoreAlignmentShifts true -> create AM + 3 arrays with fill value | `Class 1 Oracle Shift Accumulation And Shift Arrays` |
| 43 | (f) | StoreAlignmentShifts false -> no arrays created | `Class 1 Oracle Shift Application Without Shift Arrays` (asserts the AM is absent) |

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
| `AlignSectionsMisorientationFilter: output test` | **retired** | Compared the three shift arrays against `output_align_sections_misorientation.dream3d`, a file captured from this filter's own output — a circular oracle that could only confirm whatever the filter did at capture time. Replaced by hand-derived Class 1 assertions on the same three arrays. |

All non-retired tests pass at the branch head. **OOC runs waived** per the standing program
decision of 2026-08-19.

## Mutation verification

19 mutation runs. Each: apply exactly one textual edit -> rebuild -> run the blind suite
`ctest -R "OrientationAnalysis::AlignSections"` -> revert -> **prove `git diff` is empty**.
All 19 recorded a clean revert. Evidence: `ww_work/AlignSectionsMisorientation/mutations/`.

The blind suite is **15 tests**: this filter's 11 plus the 4 `AlignSectionsMutualInformation`
tests, which is the other *OrientationAnalysis* consumer of the shared base. It does **not**
include `AlignSectionsList` or `AlignSectionsFeatureCentroid` — those live in SimplnxCore and
are covered instead by the `ctest -R "SimplnxCore::AlignSections"` family no-regression run
reported under Test inventory.

The two duplicated copies of the candidate scan were targeted independently (by occurrence
index) so that each copy's coverage could be established separately.

| ID | Mutation | Outcome |
|---|---|---|
| M1 / M16 | accumulation sign `+` -> `-`, **recording copy** | Killed the 6 shift-asserting oracle cases. Correctly did **not** kill the legacy-parity test or the no-store oracle case, both of which run the other copy. |
| M17 | accumulation sign `+` -> `-`, **non-recording copy** | Killed the legacy-parity test **and** the no-store oracle case — the latter proving that case earns its place. |
| M2 / M2b | y sampling stride 4 -> 2, recording copy | **Survived.** Explained above: exact-translation fixtures are stride-invariant by construction. |
| M3 / M3b | y sampling stride 4 -> 2, non-recording copy | Killed the legacy-parity test only. Same explanation for the oracle cases. |
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
  (verified equal to `test/CMakeLists.txt:134`).
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/align_sections_misorientation.md`
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

**96 element-wise checks, 0 failures.** Quats bit-identical (max absolute difference 0.0 over
12,288 elements on the 3-section fixtures); Phases and Mask identical element for element;
every shift value identical, with the legacy text row `r` mapping to SIMPLNX tuple `r+1` and
SIMPLNX tuple 0 all zeros. Every predicted value was **frozen in writing before the runs**
(`ww_work/AlignSectionsMisorientation/ab_predictions.md`) and every one was confirmed;
**zero unpredicted divergences**.

Two warnings appear in the `nxrunner` logs and are **harness artefacts, not filter behaviour**:
an optional-python-plugin `ModuleNotFoundError`, and `-5432` reporting that the *writer* filter's
optional `compression_level` key was absent from the hand-written pipeline JSON.

Deviations, all detailed in `vv/deviations/AlignSectionsMisorientationFilter.md`:

- `AlignSectionsMisorientationFilter-D1` — shift output is DataArrays with a zero tuple 0, not a text file (the only difference actually observed in the A/B)
- `AlignSectionsMisorientationFilter-D2` — misorientation angle computed in double via `atan2` rather than float `acos` (precision-bound, no observed effect)
- `AlignSectionsMisorientationFilter-D3` — one-ULP difference in the degrees-to-radians tolerance constant (deliberately not asserted)
- `AlignSectionsMisorientationFilter-D4` — mask type widened to `bool | uint8` (superset)
- `AlignSectionsMisorientationFilter-D5` — five added guards, including a restored 3D-geometry guard and detection of a phase read that was out of bounds in both implementations
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
6. **Cross-file error-code cruft** — the same copy-pasted constant block, including the
   duplicate `-68063`, also appears in `ComputeMisorientationsFilter.cpp` and
   `AlignSectionsFeatureCentroidFilter.cpp`. Fixed here only; the codes are per-file so there
   is no live collision. `-68001` is still used for two distinct geometry errors in this file
   (both messages are self-identifying); splitting it was outside the ratified scope.
