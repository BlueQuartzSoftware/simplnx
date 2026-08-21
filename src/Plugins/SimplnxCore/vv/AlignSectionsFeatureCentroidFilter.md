# V&V Report: AlignSectionsFeatureCentroidFilter

|                            |                                                                                                                         |
|----------------------------|-------------------------------------------------------------------------------------------------------------------------|
| Plugin                     | SimplnxCore                                                                                                             |
| SIMPLNX UUID               | `b83f9bae-9ccf-4932-96c3-7f2fdb091452`                                                                                  |
| SIMPLNX Human Name         | Align Sections (Feature Centroid)                                                                                       |
| DREAM3D 6.5.171 equivalent | `AlignSectionsFeatureCentroid` — `Source/Plugins/Reconstruction/ReconstructionFilters/AlignSectionsFeatureCentroid.{h,cpp}` (UUID `{886f8b46-51b6-5682-a289-6febd10b7ef0}`) |
| Verified commit            | *<filled at SBIR deliverable assembly>*                                                                                 |
| Status                     | READY FOR REVIEW                                                                                                        |
| Sign-off                   | *Michael Jackson <mike.jackson@bluequartz.net> (V&V, 2026-08-21) — second-engineer review pending*                       |

## At a glance

| Aspect | Current state |
|---|---|
| Algorithm Relationship | **Port with one deliberate rewrite plus four corrections.** The centroid reduction, the top-down iteration order, the truncating shift conversion, the cumulative composition and the shift-application math are line-for-line ports of DREAM3D 6.5.171. The shift *output* was rewritten from a CSV file to four Data Arrays (PR #1237). This V&V corrected four defects: the Reference Slice semantics (shared with legacy), the all-masked-slice NaN cast (shared with legacy), four missing preflight guards (SIMPLNX-only) and uninitialized shift-array tuple 0 (SIMPLNX-only). |
| Oracle (confirmed) | **Class 1 (Analytical), 12 fixtures.** Every expected centroid, shift, aligned cell value and guard code is hand-derived from the algorithm source before any run (`ww_work/AlignSectionsFeatureCentroid/oracle_derivations.md`). The two centerpiece fixtures enumerate all 75 cells of all three slices. Supplemented by an **external** oracle: the archive's own DREAM3D 6.6.331 shift CSV, reproduced by SIMPLNX bit-for-bit on 59 slices. |
| Code paths enumerated | 14 (centroid reduction, empty slice, empty reference slice, reference vs consecutive, store vs no-store, positive/negative traversal, in-bounds copy, zero fill, range warnings, four preflight guards) |
| Tests today | 14 test cases (was 3): 1 legacy-6.6 exemplar regression, 11 hand-derived oracle cases, 1 SIMPL 6.4+6.5 backwards-compat, and the exemplar case doubles as the reference-slice equivalence pin. The circular "output test" was retired. |
| Exemplar archive | `align_sections_feature_centroids.tar.gz` retained (provenance sidecar written; its `Exemplar Data` container is legacy DREAM3D 6.6.331 output, proven from the file's embedded pipeline). The orphaned `6_6_align_sections_feature_centroids.tar.gz` download was removed. |
| Legacy comparison | **Run.** 5 configurations x 2 legacy builds x 26 comparables = 130 rows. 99 MATCH, 21 DIFFER, 10 informational. **All 21 divergences were predicted from source before either binary ran** (`ww_work/.../ab_predictions.md`). Patched 6.5.172 matches SIMPLNX on **every** comparable (0 DIFFER). |
| Bug flags | 2 shared bugs fixed in both code bases (Reference Slice inversion, all-masked-slice UB), 1 legacy-only bug documented (relative-shift columns always 0), 1 legacy-only off-by-one documented, 4 SIMPLNX-only guard gaps closed, 1 SIMPLNX-only nondeterminism closed. |
| Mutations | 14 applied, 14 killed, each by the intended assertion; all reverts verified clean. |

## Summary

`AlignSectionsFeatureCentroidFilter` rigid-body shifts each Z section of an Image Geometry in X and Y
so that the centroid of the mask-flagged Cells on that section lands on a target centroid — either
the neighboring section's (consecutive mode) or one nominated reference section's. Shifts are whole
Cell counts obtained by dividing the real-unit centroid difference by the Cell size and truncating
toward zero; every array in the Cell Attribute Matrix is then shifted in place, with vacated Cells
zero-filled.

Verification used a **Class 1 (Analytical)** oracle of 12 hand-derived fixtures, written down in full
before any code was changed or run, plus an **external** cross-check against the DREAM3D 6.6.331
shift CSV that ships inside the exemplar archive. Legacy A/B against DREAM3D 6.5.171 produced 21
divergences, all of them predicted from source in advance; a surgical patch to the 6.5.172 tree
removes every one of them, so patched-legacy, fixed-SIMPLNX and the hand-derived oracle now agree on
all five A/B configurations.

Two of the findings are **shared bugs** that DREAM3D 6.5.x and SIMPLNX inherited from the same
ancestor code and that changed user-visible output when fixed:

* **Reference Slice named the wrong section.** The centroid arrays are filled from the section
  farthest from the Z origin down, and the user's Reference Slice value was used directly as an index
  into them, so "Reference Slice = k" selected physical slice `Z-1-k`. Every value other than `Z-1`
  aligned to the wrong section, silently. Reference Slice is now a physical slice index with 0 at the
  Z origin, as the parameter has always been documented.
* **A fully masked-out section produced undefined behavior.** With no in-mask Cells the centroid was
  `0.0f/0.0f = NaN`, and `static_cast<int64_t>(NaN)` is undefined — it yields 0 on arm64 and
  `INT64_MIN` on x86-64. In consecutive mode the NaN also poisoned the *next* section's relative
  shift, so one empty section could de-align the entire remaining stack. That is exactly what the
  legacy run of the `ab5` fixture did: it left the Z-origin section at shift 1 where the correct
  answer is 2.

## Algorithm Relationship

*Classification:* ~~Rewrite~~ | **Port** (with one deliberate output rewrite) | ~~Minor changes~~ | ~~New filter~~

*Evidence:* the SIMPL UUID is retained and SIMPL 6.4/6.5 conversion fixtures exist at
`test/simpl_conversion/6_*/AlignSectionsFeatureCentroidFilter.json`. Line-for-line correspondence
with DREAM3D 6.5.171:

| Aspect | SIMPLNX | DREAM3D 6.5.171 | Verdict |
|---|---|---|---|
| float32 centroid accumulation of `n*spacing` | `Algorithms/AlignSectionsFeatureCentroid.cpp:110-130` | `AlignSectionsFeatureCentroid.cpp:213-222` | identical |
| Top-down iteration `slice = (Z-1) - iter` | alg cpp:101 | fc.cpp:207 | identical |
| Truncating `static_cast<int64_t>` of centroid difference / spacing | alg cpp:186-187 | fc.cpp:233-239 | identical |
| Cumulative composition of truncated relative shifts | alg cpp:210-211 | fc.cpp:238-239 | identical |
| Shift application, reversed traversal, bounds test, zero fill | `src/simplnx/Utilities/AlignSections.cpp:65-99` | `AlignSections.cpp:108-142` (TBB) and `339-380` (serial) | identical |
| Range-warning thresholds and one-shot flags | alg cpp:223-238 | fc.cpp:242-260 | identical logic |

*Port-time deltas (each a Deviation — see `vv/deviations/AlignSectionsFeatureCentroidFilter.md`):*

1. **Shift output: CSV file → four Data Arrays** (D5, structural, PR #1237). Legacy writes
   `#Slice_A,Slice_B,New X Shift,New Y Shift,X Shift, Y Shift, X Centroid, Y Centroid` with `Z-1`
   rows; SIMPLNX writes `Slice Indices`, `Relative Shifts`, `Cumulative Shifts` and `Centroids`, each
   `Z` tuples x 2 components, in a new Attribute Matrix.
2. **Mask type widened** from a strict `DataArray<bool>` (legacy `fc.cpp:80-82`) to `bool` or `uint8`
   with non-zero meaning in-mask (D6). A superset; verified to give identical results to the bool
   path on the same layout.
3. **Progress and threading**: legacy raw `tbb::task_group` per array, SIMPLNX
   `ParallelTaskAlgorithm` per array. Equivalent — one array is always processed by one thread, which
   satisfies the project's DataArray thread-safety rule.
4. **Warning channel**: legacy set warning codes 100-103 on the filter; SIMPLNX emitted `Info`
   messages only, and two of its four messages had lost their `{}` placeholder so they printed
   `Slice=` with no number. Corrected in this cycle (D7).
5. **Ignore list dropped**: legacy carried an `IgnoredDataArrayPaths` property (always empty for this
   filter) that its parallel branch honored and its serial branch did not. SIMPLNX has no such
   concept and shifts every child of the Cell Attribute Matrix.

## Oracle

*Class:* **1 (Analytical)** primary, with a Class-2-style external cross-check (see "Exemplar
archive").

Every expected value was derived by hand from the algorithm source and written to
`ww_work/AlignSectionsFeatureCentroid/oracle_derivations.md` **before** the fixes were written, and
every hand-derived constant in the test source carries its derivation in a comment. Nothing was
back-filled from observed output.

### Derivation basis

For iteration index `i`, physical slice `s = Z-1-i`, with `mean_x(s)` the mean X index of the in-mask
Cells of slice `s`:

* `xCentroid[i] = spacing[0] * mean_x(s(i))` (float32 accumulation then divide).
* Reference mode: `xShift[i] = trunc(mean_x(s(i)) - mean_x(k))` for user Reference Slice `k`, whose
  centroid lives at index `Z-1-k`.
* Consecutive mode: `xShift[0] = 0`; `xShift[i] = xShift[i-1] + trunc(mean_x(s(i)) - mean_x(s(i-1)))`.
* Application: `out(x,y) = in(x + xShift[i], y + yShift[i])` when the source Cell is inside the
  slice, otherwise `out(x,y) = 0`. A **positive** shift therefore moves content toward `-x`/`-y`.

The old-versus-new Reference Slice identity follows directly: the old index for user value `k` was
`k`, the new index is `Z-1-k`, so old `k = 0` and new `k = Z-1` both select index 0. This is why the
legacy-6.6 exemplar test stays valid with `ReferenceSlice = zDim-1`, and it was confirmed by two
independent executed runs (the exemplar test, and the `ab2` A/B configuration whose cell arrays match
6.5.171 exactly).

### Fixture inventory

| Fixture | Geometry | Purpose | Hand-derived expectation |
|---|---|---|---|
| F1 | 5x5x3, spacing 1 | consecutive mode, integer offsets | shifts {0,1,3} / {0,0,2}; all 75 Payload cells and all 75 Mask cells enumerated |
| F2 | 5x5x3 | reference mode anchored on the far slice (`RS=2`) | shifts {0,1,3} / {0,0,2}; discriminates the old semantics, which give {0,-2,0} / {0,-2,0} |
| F2b | 5x5x3 | reference mode anchored on the Z-origin slice (`RS=0`) | shifts {-3,-2,0} / {-2,-2,0}; the only fixture with a nonzero shift at index 0, so it is the probe for the shared-base transfer-loop extension |
| F3 | 6x3x2, 4 sections | truncation direction | deltas +/-0.6 -> 0 and +/-1.5 -> +/-1 (round-to-nearest would give +/-1 and +/-2) |
| F4 | 5x5x2 | full off-edge push | shift (4,4); exactly one surviving cell, 24 of 25 zero-filled, no range warning |
| F5 | = F1 with store on | shift-array contract | Slices/Relative/Cumulative/Centroids per tuple, tuple 0 = deterministic zeros |
| F6 | 5x5x4 | fully masked-out non-reference section | shifts {0,1,1,2} / {0,1,1,2}, exactly one Warning `-53904` naming **physical** slice 1 (iteration index 2, so the message index is discriminated) |
| F6b | 5x5x4 | fully masked-out **reference** section | execute error `-53901` |
| F7 | 5x5x1 / 1x5x5 / 5x1x5 | degenerate geometry | preflight error `-68072` |
| F8 | 5x5x3, 7 cases | Reference Slice bounds | `-68071` at `RS >= Z`, `-68064` at `RS < 0`, and **valid** for both when Use Reference Slice is off |
| F9 | = F1, uint8 mask | mask-type parity | identical to F1 |
| F10 | = F1, spacing (0.5, 2.0, 1.0) | units invariance | identical voxel shifts to F1; Centroids scale to {0.75,1.0} and {1.75,5.0} |
| F11 | 2x2x13 | accumulated out-of-range shift | cumulative X {0,0,0,0,1,1,1,1,2,2,2,2,3} > X dimension 2, exactly one Warning `-53902`; also pins the truncation-accumulation wart |
| F12 | = F1 plus a StringArray in the Cell AM | non-Data-Array child | preflight error `-68073` |

### Reachability note (source-derived)

In reference mode `|shift| = |trunc(mean_i - mean_ref)| <= X-1`, so the "shift exceeds the X
dimension" diagnostic is **unreachable**. In consecutive mode
`cumulative(i) = (mean(i) - mean(0)) - sum(fractional residues)`, so exceeding the dimension requires
banking more than one Cell of truncation residue across several sections. F11 is built from that
identity: four sections per cycle, three of which truncate to 0 while the fourth banks +1.

## Code path coverage

| # | Path | Covered by |
|---|---|---|
| 1 | Centroid reduction, bool mask | F1, F2, F2b, F3, F4, F6, F10, F11, exemplar |
| 2 | Centroid reduction, uint8 mask | F9, exemplar (the Small IN100 mask is uint8) |
| 3 | `count == 0` empty-slice branch + Warning | F6 |
| 4 | Empty reference slice error | F6b |
| 5 | Reference-mode shift, index 0 included | F2, F2b |
| 6 | Consecutive-mode cumulative composition | F1, F3, F6, F11 |
| 7 | Carry-forward of the last valid centroid | F6 (mutation M5 confirms it is load-bearing) |
| 8 | Store branch array writes | F1/F5, F2, F2b, F4, F6, F9, F10, F11 |
| 9 | No-store branch | F6b, F7, F8, F12, exemplar |
| 10 | Positive-shift ascending traversal + zero fill | F1, F4, F6 |
| 11 | Negative-shift descending traversal + zero fill | F2b |
| 12 | X range Warning | F11 |
| 13 | Y range Warning | not reachable without a second pathological fixture; the X path is identical code and the warning-plumbing is proven by F6 and F11 (mutations M11 and M13) |
| 14 | Preflight guards `-68064`, `-68071`, `-68072`, `-68073` | F8, F7, F12 |

Uncovered and stated as such: the `-68070` missing-geometry and `-68074` missing-Cell-Attribute-Matrix
guards are defensive — a `GeometrySelectionParameter` restricted to Image geometries already
validates existence and type, and no test can construct the missing-Cell-AM case through the normal
filter API without hand-building a geometry that no SIMPLNX filter produces. The `-53905` execute-time
Reference Slice range check is likewise shadowed by `-68071` on every path that goes through preflight.

## Test inventory

| Test case | Kind | Notes |
|---|---|---|
| `Algorithm Test` | legacy-6.6 exemplar regression | Small IN100, 189x201x60, uint8 mask. `ReferenceSlice = zDim-1` with an in-test comment recording the equivalence to the archive's `ReferenceSlice = 0`. Also asserts that none of the three diagnostic warnings fire on this data. |
| `Consecutive Mode Integer Offsets` | Class 1 | F1 + F5 |
| `Reference Slice Is A Physical Slice Index` | Class 1 | F2 and F2b as sections |
| `Shifts Truncate Toward Zero` | Class 1 | F3, four `DYNAMIC_SECTION`s |
| `Off Edge Push Zero Fills` | Class 1 | F4 |
| `Fully Masked Out Slice Warns And Does Not Shift` | Class 1 | F6 |
| `Fully Masked Out Reference Slice Is An Error` | Class 1 | F6b |
| `Non 3D Geometry Is Rejected` | Class 1 | F7, three `DYNAMIC_SECTION`s |
| `Reference Slice Bounds` | Class 1 | F8, seven `DYNAMIC_SECTION`s |
| `UInt8 Mask Parity` | Class 1 | F9 |
| `Non Unit Spacing Invariance` | Class 1 | F10 |
| `Accumulated Shift Beyond The X Dimension Warns` | Class 1 | F11 |
| `Non Data Array Cell Child Is Rejected` | Class 1 | F12 |
| `SIMPL Backwards Compatibility` | conversion | untouched |

**Retired:** the `output test` case. It compared the four shift arrays against
`output_align_sections_feature_centroids.dream3d`, a file this filter generated itself — a circular
oracle, and one whose tuple 0 came from the same uninitialized allocation the test was reading. Its
coverage is replaced by the hand-derived shift-array assertions in F5, F2, F2b, F4, F6, F9, F10 and
F11, and by the external 6.6.331 CSV cross-check below.

### Mutation verification

14 mutations, each applied to the pristine branch, built, run against the full AlignSections family
(`ctest -R "AlignSections"`, 26 tests), then reverted. Full transcript:
`ww_work/AlignSectionsFeatureCentroid/logs/mutation_results.json`.

| # | Mutation | Killed by | Killing assertion |
|---|---|---|---|
| M1 | Reference index mapping reverted to the raw user value | Reference Slice Is A Physical…, Algorithm Test, Fully Masked Out Reference Slice… | `payloadRef[i] == expected[i]` [0 == 23] |
| M2 | Shared-base transfer loop back to `i = 1` | Reference Slice Is A Physical… | `payloadRef[i] == expected[i]` [200 == 0] |
| M3 | Zero-shift skip condition `&&` -> `\|\|` | 9 tests across all four AlignSections filters | `payloadRef[i] == expected[i]` [100 == 101] |
| M4 | `count == 0` guard disabled | Fully Masked Out Slice…, Fully Masked Out Reference Slice… | `warnings().size() == 1` [0 == 1] |
| M5 | Carry-forward centroid replaced by `xCentroid[i-1]` | Fully Masked Out Slice… | `payloadRef[i] == expected[i]` [33 == 22] |
| M6 | Truncation replaced by `std::llround` | Shifts Truncate Toward Zero | `cumulativeShiftsRef[2] == expectedXShift` [1 == 0] |
| M7 | Shift-array fill value `"0"` -> `"7"` | 6 tests | `slicesRef[i] == slices[i]` [7 == 0] |
| M8 | `-68071` bound `>=` -> `>` (the legacy off-by-one) | Reference Slice Bounds | `preflightResult.outputActions.invalid()` [false] |
| M9 | 3D guard `<= 1` -> `< 1` | Non 3D Geometry Is Rejected | `preflightResult.outputActions.invalid()` [false] |
| M10 | Non-Data-Array child guard disabled | Non Data Array Cell Child Is Rejected | `preflightResult.outputActions.invalid()` [false] |
| M11 | X range-warning threshold widened | Accumulated Shift Beyond The X Dimension Warns | `warnings().size() == 1` [0 == 1] |
| M12 | Reference-mode `firstIndex` forced to 1 | Reference Slice Is A Physical… | `slicesRef[i] == slices[i]` [0 == 2] |
| M13 | Base no longer propagates findShifts warnings | Fully Masked Out Slice…, Accumulated Shift… | `warnings().size() == 1` [0 == 1] |
| M14 | Empty-slice message prints the iteration index | Fully Masked Out Slice… | `warnings()[0].message.find("Slice=1") != npos` |

Every mutation was killed, and in every case by the fixture designed to discriminate it. Reverts for
M1-M7 were verified by an empty `git diff`; for M8-M14 the driver's global `git diff` check was
polluted by concurrent documentation edits, so the reverts were re-verified per file at the end of the
run (`git diff` limited to the three source files is empty).

### Shared-base no-op proof

The sole edit to `src/simplnx/Utilities/AlignSections.cpp` starts the per-array transfer loop at index
0 instead of 1 and skips any index whose X and Y shifts are both zero.

*Source-derived:* the shift vectors are zero-initialized (`AlignSections.cpp:140-141`) and the four
`findShifts` overrides write only indices >= 1 — `AlignSectionsList.cpp:49,60`,
`AlignSectionsMisorientation.cpp:89,201`, `AlignSectionsMutualInformation.cpp:92,218`, and this
filter's own consecutive-mode branch. So index 0 is `(0,0)` for every consumer except this filter in
reference mode, and the new iteration is skipped. For `i >= 1` the skip is equally a no-op: with both
shifts zero the in-bounds test at `AlignSections.cpp:89-90` is always true, so the body would perform
`copyTuple(p, p)` and never reach `initializeTuple`.

*Executed:* `ctest -R "AlignSections"` — 26/26 pass, including `AlignSectionsListFilter` (2 execution
tests), `AlignSectionsMisorientation` (Small IN100 pipeline + output test),
`AlignSectionsMutualInformation` (4 tests), `PIPELINE::OrientationAnalysis::001_AlignSectionsMutualInformation`
and `PY::OrientationAnalysis_AlignSectionsMutualInformation`. Mutation M3 shows this suite is
genuinely sensitive to the skip's semantics: breaking the condition fails 9 of those tests, four of
them belonging to the other three alignment filters.

## Exemplar archive

`align_sections_feature_centroids.tar.gz`, SHA512 `06a4f57…3ca9b`, retained. Provenance sidecar:
`vv/provenance/align_sections_feature_centroids.md`.

The dossier's open question — whether the `6_6_` prefix really means legacy provenance — is answered
from the file itself rather than from the naming convention. The `.dream3d` file's embedded `Pipeline`
group contains the eleven-filter DREAM3D 6.6 pipeline that produced it; filter `08` is
`AlignSectionsFeatureCentroid`, `FilterVersion 6.6.331`, `UseReferenceSlice = 1`,
`ReferenceSlice = 0`, `WriteAlignmentShifts = 1`, writing
`6_6_align_sections_feature_centroids.txt` — which is also in the archive. The `Exemplar Data`
container is therefore genuine legacy 6.6.331 output and is **not** circular.

That CSV makes an independent oracle possible for the real dataset. Running post-fix SIMPLNX on the
archive's unaligned `DataContainer` with `ReferenceSlice = 59` (= `zDim-1`) reproduces the 6.6.331
file exactly: 59 of 59 rows agree on the cumulative shifts and on the slice pairs, and on the
centroids to the CSV's six-significant-digit precision. Script:
`ww_work/AlignSectionsFeatureCentroid/scripts/archive_check.py`. This is a third implementation
(6.6.331) agreeing with SIMPLNX and with the hand derivation of the Reference Slice identity, and it
is the strongest available evidence that the semantics fix did not perturb the legacy-parity path.

The orphaned `6_6_align_sections_feature_centroids.tar.gz` download was removed from
`src/Plugins/SimplnxCore/test/CMakeLists.txt`: nothing in the repository referenced it, and the
exemplar of that name lives inside the retained archive.

## Legacy comparison (A/B)

*Binaries:* DREAM3D 6.5.171 `~/Applications/DREAM3D.app/Contents/Bin/PipelineRunner`; the 6.5.172
tree at `/Users/mjackson/Workspace9/6.5.172/DREAM3D` before and after the alignment patch; SIMPLNX
`nxrunner` from `NX-Com-Qt69-Vtk96-Rel`.

*Protocol:* two legacy-format (`FileVersion 7.0`) input files were written with h5py and read by
**both** runners, so the inputs are byte-identical by construction. Each configuration compares every
Cell array element-wise plus the legacy CSV row for iteration index `i` against the SIMPLNX shift
tuple `i`. All predictions were written to `ab_predictions.md` before either binary was run.

*Sanity check:* the unpatched 6.5.172 tree reproduces 6.5.171 byte-for-byte on all four applicable
configurations, so the 6.5.172 differences that follow are attributable to the patch alone.

| Run | Configuration | Predicted | Observed |
|---|---|---|---|
| ab1 | consecutive | cell arrays identical; relative-shift columns differ (legacy always 0) | as predicted |
| ab2 | reference, legacy `RS=0` vs SIMPLNX `RS=2` | cell arrays identical (the equivalence identity) | as predicted |
| ab3 | reference, both `RS=0` | cell arrays diverge; SIMPLNX shifts {-3,-2,0}/{-2,-2,0}, legacy {0,1,3}/{0,0,2} | as predicted, 24 Mask and 48 Payload cells differ |
| ab4 | reference, both `RS=2` | cell arrays diverge; legacy shifts {0,-2,0}/{0,-2,0} | as predicted, 16 Mask and 48 Payload cells differ |
| ab5 | consecutive, one empty section | legacy undefined (NaN cast); SIMPLNX {0,1,1,2}/{0,1,1,2} | legacy left the Z-origin section at cumulative 1 instead of 2 and wrote `nan` centroids, i.e. the arm64 `fcvtzs -> 0` branch of the UB |

**Score: 130 comparables, 99 MATCH, 21 DIFFER, 10 informational. Zero unpredicted divergences.**
Every DIFFER row belongs to one of the pre-documented classes: the legacy always-zero relative-shift
columns (9 rows), the Reference Slice semantics fix (8 rows across ab3 and ab4), or the empty-slice
fix (4 rows in ab5). Raw table: `ww_work/AlignSectionsFeatureCentroid/ab_results.csv`.

### 6.5.172 alignment patch

Commit `f81973147` in `/Users/mjackson/Workspace9/6.5.172/DREAM3D`
("BUG: AlignSectionsFeatureCentroid — anchor on a physical reference slice and survive empty slices")
applies the same five corrections to the legacy tree: the physical reference-slice mapping, reference
mode covering the far slice, both transfer loops starting at index 0 with a zero-shift skip, the
empty-slice policy with warning 104 and error -5557, the Reference Slice bounds check (fixing the `>`
off-by-one and only validating when the value is used), and the never-assigned relative-shift columns.

Re-running all five configurations against the patched binary with the **SIMPLNX** Reference Slice
values gives **0 DIFFER out of 66 comparables** — patched-legacy, fixed-SIMPLNX and the hand-derived
oracle agree on every Cell array, slice pair, relative shift, cumulative shift and centroid, including
the previously undefined empty-slice case.

## Deviations

Full entries: `src/Plugins/SimplnxCore/vv/deviations/AlignSectionsFeatureCentroidFilter.md`.

| ID | Deviation | Class | Disposition |
|---|---|---|---|
| D1 | Reference Slice selected physical slice `Z-1-k` instead of `k` | shared bug | **Fixed in both.** SIMPLNX on this branch; 6.5.172 in `f81973147`. Behavior change for any pipeline using a Reference Slice other than `Z-1`. |
| D2 | A fully masked-out section produced a NaN centroid and an undefined integer cast, poisoning the following section in consecutive mode | shared bug | **Fixed in both.** Zero relative shift + Warning; empty reference section is an error. |
| D3 | Legacy's "New X Shift"/"New Y Shift" CSV columns are always 0 | legacy-only bug | Documented; also fixed in the 6.5.172 patch. SIMPLNX has always written real values. |
| D4 | Legacy's Reference Slice guard used `>` so `RS == Z` passed and then read out of bounds; it also ran when the value was unused | legacy-only bug | Documented; fixed in the 6.5.172 patch. SIMPLNX now rejects the whole invalid range (`-68071`) and only when the value is used. |
| D5 | Shift output: CSV file -> four Data Arrays, `Z` tuples with tuple 0 present | structural, PR #1237 | Documented. Tuple 0 is a deterministic zero anchor row in consecutive mode and a real data row in reference mode. |
| D6 | Mask accepts `uint8` as well as `bool` | SIMPLNX superset | Documented; parity verified (F9). |
| D7 | Diagnostics were `Info` messages with no Result warnings, and two of them had lost their format placeholder | SIMPLNX-only bug | **Fixed.** Warnings are on the Result again; the placeholder-less NaN messages were removed with the NaN path. |
| D8 | Tuple 0 of the four shift arrays was uninitialized memory | SIMPLNX-only bug | **Fixed** by a `"0"` fill value. Not observable as a test failure on this platform because fresh pages read as zero — the mutation `"0" -> "7"` is what proves the assertion bites. |
| D9 | Four missing preflight guards: Reference Slice above the Z dimension, non-3D geometry, non-Data-Array Cell child, missing Cell Attribute Matrix | SIMPLNX-only guard gaps | **Fixed** (`-68071`, `-68072`, `-68073`, `-68074`). The third was an uncaught `std::bad_cast` in the shared base and the fourth a null dereference. |
| D10 | Shifts truncate toward zero, and consecutive mode sums the *truncated* relative shifts so truncation error accumulates | shared design wart | **Documented, not changed.** Changing it would alter every existing result. The filter documentation now states truncation instead of rounding, and F3/F11 pin the behavior. |
| D11 | `Slice Indices` records `{slice, slice+1}` even in reference mode, where the second component is not the section that was actually used as the target | shared, cosmetic | **Documented, not changed.** Kept identical to legacy. Tuple 0's second component is consequently `Z`, one past the last slice. |
| D12 | Legacy's empty-slice diagnostic named the iteration index (`Slice=2` for physical slice 1); SIMPLNX names the physical slice | SIMPLNX improvement | Documented; the 6.5.172 patch adopts the physical index too. |
| D13 | Dead error-code constants `-68001..-68004` | SIMPLNX cruft | **Removed.** |

## Test-suite results

| Command | Result |
|---|---|
| `ctest -R "AlignSectionsFeatureCentroid"` | 14/14 pass |
| `ctest -R "AlignSections"` (all four alignment filters + PIPELINE + PY) | 26/26 pass |
| `ctest -R "SimplnxCore::"` | 996/996 pass |
| `ctest -R "OrientationAnalysis::"` | 293/293 pass, including `PIPELINE::OrientationAnalysis::002_...` (the `(02) Small IN100 Full Reconstruction` pipeline, which uses `reference_slice = 0` and therefore now anchors the section at the Z origin) and every `PY::` chained example pipeline |

## Follow-ups for the human engineers

1. **Guard parity.** `AlignSectionsListFilter` and `AlignSectionsMutualInformationFilter` have the
   same three preflight gaps this cycle closed for FeatureCentroid (non-3D geometry, non-Data-Array
   Cell children, missing Cell Attribute Matrix). Per the ratified scope, no guard was added to the
   shared base, so those filters are still exposed. `AlignSectionsMisorientation` is covered by the
   parallel task in this batch.
2. **`findShifts` duplication.** The store and no-store branches of this filter's shift loop remain
   near-duplicates (about 40 lines). Deliberately not refactored in a V&V change; worth a separate
   enhancement.
3. **Reference-slice semantics are a user-visible change.** Any saved pipeline that uses
   `use_reference_slice = true` with a value other than `zDim-1` will now align to a different
   section. The shipping `(02) Small IN100 Full Reconstruction.d3dpipeline` uses `reference_slice = 0`,
   which under the new semantics anchors the section at the Z origin rather than the far section; the
   pipeline was deliberately left unchanged because 0 is now exactly what the parameter documents.
   This belongs in the release notes.
