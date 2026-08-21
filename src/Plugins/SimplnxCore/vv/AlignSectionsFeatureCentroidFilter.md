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
| Algorithm Relationship | **Port of DREAM3D 6.5.171 `AlignSectionsFeatureCentroid`, with one deliberate output rewrite plus six corrections.** The centroid reduction, the top-down iteration order, the truncating shift conversion, the cumulative composition and the shift-application math are line-for-line ports. The shift *output* was rewritten from a CSV file to four Data Arrays (PR #1237). This V&V corrected six defects: the Reference Slice semantics (D1, shared with legacy), the all-masked-slice NaN cast (D2, shared with legacy), the Reference Slice bounds guard (D4), four further missing preflight guards (D9, SIMPLNX-only), uninitialized shift-array tuple 0 (D8, SIMPLNX-only) and the demoted/broken diagnostics (D7, SIMPLNX-only); D13's dead constants and phantom documentation paragraph were additionally removed as cleanup. |
| Oracle (confirmed) | **Class 1 (Analytical), 15 fixtures** encoded in `src/Plugins/SimplnxCore/test/AlignSectionsFeatureCentroidTest.cpp` (13 hand-derived `TEST_CASE`s). Every expected centroid, shift, aligned cell value and guard code is hand-derived from the algorithm source before any run (`ww_work/AlignSectionsFeatureCentroid/oracle_derivations.md`). The two centerpiece fixtures enumerate all 75 cells of all three slices. Supplemented by an **external** oracle: the archive's own DREAM3D 6.6.331 shift CSV, whose slice-pair and cumulative-shift columns SIMPLNX reproduces exactly on all 59 rows (centroids to the file's six-significant-digit text precision). |
| Code paths enumerated | **18 of 24 exercised** (centroid reduction over both mask types, empty slice, empty reference slice, reference vs consecutive, store vs no-store, positive/negative traversal, zero-shift skip, zero fill, the X range warning and five of the seven preflight guard codes). The six uncovered paths — including the cancel checks, the Y range warning and the two defensive guards `-68070`/`-68074` — are listed as rows in `## Code path coverage`. |
| Tests today | 15 test cases (was 3): 1 legacy-6.6 exemplar regression (kept, re-anchored), 13 new hand-derived Class 1 oracle cases spanning both modes, the truncation direction, empty slices, non-unit spacing, mask-type parity and the five preflight guards a test can construct, and 1 SIMPL 6.4+6.5 backwards-compat case (kept). The circular "output test" was retired. |
| Exemplar archive | `align_sections_feature_centroids.tar.gz` retained (provenance sidecar written; its `Exemplar Data` container is legacy DREAM3D 6.6.331 output, proven from the file's embedded pipeline). The orphaned `6_6_align_sections_feature_centroids.tar.gz` download was removed. |
| Legacy comparison | **Run.** 5 configurations against 2 legacy builds = **130 compared items** (59 against DREAM3D 6.5.171, 71 against the patched 6.5.172 proof build; 11 or 15 items per configuration depending on how many arrays and CSV columns that configuration produces). Against 6.5.171: 33 MATCH, 21 DIFFER, 5 informational. **All 21 divergences were predicted from source before either binary ran** (`ww_work/.../ab_predictions.md`). The patched build matches SIMPLNX on **every** comparable (66 MATCH, 5 informational, 0 DIFFER). |
| Bug flags | **D1, D2, D3, D4, D7, D8, D9, D12** — every deviation whose root cause is `bug`. Shared with legacy: D1, D2, D4. Legacy-only: D3, D12. SIMPLNX-only: D7, D8, D9. (D13 is dead code plus a documentation error with no computational effect.) |
| V&V phase | Complete: oracle selection and hand derivation, RED-first implementation of every fix, mutation verification, legacy A/B against 6.5.171 plus the patched-build root-cause proof, and the three source-tree deliverables (report, deviations, provenance). Outstanding: second-engineer review of the report, of the deviation narratives and of the oracle design, and a release note for the Reference Slice behavior change. That is what holds `Status` at `READY FOR REVIEW`. |
| Mutations | 14 applied, 14 killed, each by the intended assertion; all reverts verified clean. |

## Summary

`AlignSectionsFeatureCentroidFilter` rigid-body shifts each Z section of an Image Geometry in X and Y
so that the centroid of its mask-flagged Cells lands on a target centroid — the neighboring section's
in consecutive mode, or one nominated reference section's — using whole-Cell shifts obtained by
truncating the real-unit centroid difference divided by the Cell size. Verification used a **Class 1
(Analytical)** oracle of 15 hand-derived fixtures written down in full before any code was changed,
plus an external cross-check against the DREAM3D 6.6.331 shift CSV that ships inside the exemplar
archive. Result: 13 deviations from DREAM3D 6.5.171, of which two are shared bugs fixed in both code
bases (the Reference Slice inversion, D1, and the all-masked-section undefined cast, D2); all 21 A/B
divergences were predicted from source before either binary ran, and all 15 test cases pass.

## Algorithm Relationship

*Classification:* ~~Rewrite~~ | **Port** | ~~Minor changes~~ | ~~New filter~~ — the shift *output* was
rewritten from a CSV file to four Data Arrays, which is recorded as port-time delta 1 / D5 rather
than as a change of classification.

*Evidence:* the SIMPL UUID is retained and SIMPL 6.4/6.5 conversion fixtures exist at
`test/simpl_conversion/6_*/AlignSectionsFeatureCentroidFilter.json`. Line-for-line correspondence
with DREAM3D 6.5.171:

| Aspect | SIMPLNX | DREAM3D 6.5.171 | Verdict |
|---|---|---|---|
| float32 centroid accumulation of `n*spacing` | `Algorithms/AlignSectionsFeatureCentroid.cpp:110-131` | `AlignSectionsFeatureCentroid.cpp:213-222` | identical |
| Top-down iteration `slice = (Z-1) - iter` | alg cpp:101 | fc.cpp:207 | identical |
| Truncating `static_cast<int64_t>` of centroid difference / spacing | alg cpp:182-183 (reference mode), :189-190 (consecutive mode) | fc.cpp:233-239 | identical |
| Cumulative composition of truncated relative shifts | alg cpp:222-223 (store branch), :267-268 (no-store branch) | fc.cpp:238-239 | identical |
| Shift application, reversed traversal, bounds test, zero fill | `src/simplnx/Utilities/AlignSections.cpp:65-99` | `AlignSections.cpp:108-142` (TBB) and `339-380` (serial) | identical |
| Range-warning thresholds and one-shot flags | alg cpp:226-239 (store branch), :271-284 (no-store branch) | fc.cpp:242-260 | identical logic; SIMPLNX names the physical slice where legacy names the iteration index (D12) |

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

*Encoded at:* `src/Plugins/SimplnxCore/test/AlignSectionsFeatureCentroidTest.cpp` — 15 fixtures across
13 hand-derived `TEST_CASE`s, from
`SimplnxCore::AlignSectionsFeatureCentroidFilter: Consecutive Mode Integer Offsets` through
`SimplnxCore::AlignSectionsFeatureCentroidFilter: Mask Tuple Count Must Match The Cell Count`. Every
one passes at the verified commit; the per-fixture mapping is in the Test inventory below.

*How the oracle was applied:* the semantics were extracted from the algorithm source first, then each
fixture's centroids, per-section relative and cumulative shifts, every aligned Cell value and every
expected error or warning code were computed by hand and written to
`ww_work/AlignSectionsFeatureCentroid/oracle_derivations.md` **before** the fixes were written. Every
hand-derived constant in the test source carries its derivation in a comment. Nothing was back-filled
from observed output.

*Second-engineer review of the oracle design:* **skipped**, same reason as the archive provenance
sidecar records — no second engineer was realistically available for this cycle. The mitigation is
that the oracle is fully analytic and re-derivable from the report alone, and that the Reference Slice
identity it turns on is independently confirmed by the archive's DREAM3D 6.6.331 shift CSV. Listed as
outstanding in the `V&V phase` row.

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
| F3 | 6x3x2 (2 sections), 4 parameterized cases | truncation direction | deltas +/-0.6 -> 0 and +/-1.5 -> +/-1 (round-to-nearest would give +/-1 and +/-2) |
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
| F13 | = F1 plus a 50-tuple and a 100-tuple bool mask in a sibling Attribute Matrix | mask tuple count vs geometry Cell count | preflight error `-68075` in both directions (the geometry has 75 Cells) |

### Reachability note (source-derived)

In reference mode `|shift| = |trunc(mean_i - mean_ref)| <= X-1`, so the "shift exceeds the X
dimension" diagnostic is **unreachable**. In consecutive mode
`cumulative(i) = (mean(i) - mean(0)) - sum(fractional residues)`, so exceeding the dimension requires
banking more than one Cell of truncation residue across several sections. F11 is built from that
identity: four sections per cycle, three of which truncate to 0 while the fourth banks +1.

## Code path coverage

**18 of 24 paths exercised.**

*Source:* `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/AlignSectionsFeatureCentroid.cpp`
(289 lines), plus the shared shift application in `src/simplnx/Utilities/AlignSections.cpp` (191 lines)
and the preflight guards in
`src/Plugins/SimplnxCore/src/SimplnxCore/Filters/AlignSectionsFeatureCentroidFilter.cpp` (313 lines).

The algorithm has three phases: **(a)** a per-section centroid reduction over the mask, **(b)** a
per-section shift solve (reference or consecutive) that optionally writes the four shift arrays, and
**(c)** the shared per-array Cell transfer that applies the shifts. Phase (p) below is the filter's
preflight, which runs before all of them.

| # | Phase | Path | Test case |
|---|---|---|---|
| 1 | a | Centroid reduction, bool mask | F1, F2, F2b, F3, F4, F6, F10, F11 |
| 2 | a | Centroid reduction, uint8 mask | `UInt8 Mask Parity` (F9), `Algorithm Test` (the Small IN100 mask is uint8) |
| 3 | a | Mask instantiation failure `-53900` | *Not directly tested. Defensive: the `ArraySelectionParameter` validates the path and restricts the type to bool/uint8, so this is only reachable by driving the algorithm class outside the `IFilter` preflight/execute API.* |
| 4 | a | `count == 0` empty-section branch + Warning `-53904` | `Fully Masked Out Slice Warns And Does Not Shift` (F6) |
| 5 | a | Cancel check between sections | *Not directly tested. Requires cancel-signal injection; the two `m_ShouldCancel` early returns simply return an empty `Result<>`.* |
| 6 | b | Empty reference section error `-53901` | `Fully Masked Out Reference Slice Is An Error` (F6b) |
| 7 | b | Defensive execute-time Reference Slice range `-53905` | *Not directly tested. Shadowed by preflight `-68071` on every path that goes through preflight.* |
| 8 | b | Reference-mode shift, index 0 included | `Reference Slice Is A Physical Slice Index` (F2, F2b) |
| 9 | b | Consecutive-mode cumulative composition | F1, F3, F6, F11 |
| 10 | b | Carry-forward of the last valid centroid | F6 (mutation M5 confirms it is load-bearing) |
| 11 | b | Store branch array writes | F1/F5, F2, F2b, F4, F6, F9, F10, F11 |
| 12 | b | No-store branch | F6b, F7, F8, F12, F13, `Algorithm Test` |
| 13 | b | X range Warning `-53902` | `Accumulated Shift Beyond The X Dimension Warns` (F11) |
| 14 | b | Y range Warning `-53903` | *Not directly tested. the same block with the axis swapped (alg cpp:233-239 vs :226-232); the warning plumbing is proven by F6 and F11, and mutations M11 and M13 show both the threshold and the propagation are load-bearing. A second 13-section pathological fixture was judged a poor trade.* |
| 15 | c | Positive-shift ascending traversal + zero fill | F1, F4, F6 |
| 16 | c | Negative-shift descending traversal + zero fill | F2b |
| 17 | c | Zero-shift skip in the shared transfer loop | F1 (tuple 0) and every other `AlignSections` filter's tests; mutation M3 fails 9 of the 26 `AlignSections` tests that existed at mutation time |
| 18 | p | `-68072` non-3D geometry | `Non 3D Geometry Is Rejected` (F7) |
| 19 | p | `-68064` / `-68071` Reference Slice bounds, both conditional on *Use Reference Slice* | `Reference Slice Bounds` (F8, all 7 cases) |
| 20 | p | `-68073` non-Data-Array Cell child | `Non Data Array Cell Child Is Rejected` (F12) |
| 21 | p | `-68075` Mask tuple count vs geometry Cell count | `Mask Tuple Count Must Match The Cell Count` (F13, both directions) |
| 22 | p | `-68070` missing Image Geometry | *Not directly tested. Defensive: a `GeometrySelectionParameter` restricted to Image geometries already validates existence and type before preflight runs.* |
| 23 | p | `-68074` missing Cell Attribute Matrix | *Not directly tested. No test can construct it through the normal filter API without hand-building an Image Geometry with no Cell Attribute Matrix, which no SIMPLNX filter produces.* |
| 24 | p | Store-shifts output actions: the Attribute Matrix plus four arrays created with the `"0"` fill value | F5; mutation M7 (`"0"` -> `"7"`) fails 6 tests |

## Test inventory

All 15 live cases are in `src/Plugins/SimplnxCore/test/AlignSectionsFeatureCentroidTest.cpp`; the one
retired case is listed last and no longer exists in the file. Oracle class is Class 1 (Analytical) for
every case marked `new-for-V&V`.

| Test case | Status | Notes |
|---|---|---|
| `Algorithm Test` | kept (modified) | Legacy-6.6 exemplar regression: Small IN100, 189x201x60, uint8 mask, all four Cell arrays compared element-wise against the archive's `Exemplar Data` container. **Modified for this cycle:** `ReferenceSlice` moved from 0 to `zDim-1` per the D1 equivalence identity, with an in-test comment recording why, and an added assertion that none of the three diagnostic warnings fire on this data. |
| `Consecutive Mode Integer Offsets` | new-for-V&V | F1 + F5: all 75 Payload cells, all 75 Mask cells and all four shift arrays (6 values each) asserted against hand-derived values. |
| `Reference Slice Is A Physical Slice Index` | new-for-V&V | F2 and F2b as two `SECTION`s: 150 Payload cells, 150 Mask cells and both shift-array sets. The killing test for D1 (mutations M1, M2, M12). |
| `Shifts Truncate Toward Zero` | new-for-V&V | F3 in four `DYNAMIC_SECTION`s: pins truncation against round-to-nearest in both signs (mutation M6). |
| `Off Edge Push Zero Fills` | new-for-V&V | F4: 50 Payload cells; 24 of 25 zero-filled on the moved section, and no range warning at shift 4 on a 5-wide geometry. |
| `Fully Masked Out Slice Warns And Does Not Shift` | new-for-V&V | F6: 100 Payload cells, exactly one warning, and the warning text asserted to name physical slice 1 and not iteration index 2 (D12; mutations M4, M5, M13, M14). |
| `Fully Masked Out Reference Slice Is An Error` | new-for-V&V | F6b: execute fails with `-53901`. |
| `Non 3D Geometry Is Rejected` | new-for-V&V | F7 in three `DYNAMIC_SECTION`s: preflight `-68072` for Z, X and Y equal to 1 (mutation M9). |
| `Reference Slice Bounds` | new-for-V&V | F8 in seven `DYNAMIC_SECTION`s: `-68071`, `-68064`, and valid preflights when *Use Reference Slice* is off (mutation M8). |
| `UInt8 Mask Parity` | new-for-V&V | F9: byte-identical result to F1 with the mask stored as uint8 (D6). |
| `Non Unit Spacing Invariance` | new-for-V&V | F10: identical voxel shifts to F1 with the stored Centroids scaled by the spacing. |
| `Accumulated Shift Beyond The X Dimension Warns` | new-for-V&V | F11: the 13-tuple cumulative X series plus exactly one `-53902` warning (mutations M11, M13). |
| `Non Data Array Cell Child Is Rejected` | new-for-V&V | F12: preflight `-68073` for a `StringArray` in the Cell Attribute Matrix (mutation M10). |
| `Mask Tuple Count Must Match The Cell Count` | new-for-V&V | F13 in two `DYNAMIC_SECTION`s: preflight `-68075` for a mask with 50 and with 100 tuples against a 75-Cell geometry. Added in fix round 1; RED-first evidence in `ww_work/.../logs/red_run_fix1_mask_tuple_guard.txt`. |
| `SIMPL Backwards Compatibility` | kept | Untouched, byte-identical to the base commit: SIMPL 6.4 and 6.5 conversion fixtures in two `DYNAMIC_SECTION`s, checking the UUID and five converted arguments. Owns no `DataStructure`, so it correctly carries no `CheckArraysInheritTupleDims`. |
| `output test` | retired | **Circular oracle.** It compared the four shift arrays against `output_align_sections_feature_centroids.dream3d`, a file this filter generated itself — and whose tuple 0 came from the same uninitialized allocation the test was reading (D8). Its coverage is replaced by the hand-derived shift-array assertions in F5, F2, F2b, F4, F6, F9, F10 and F11, and by the external 6.6.331 CSV cross-check below. |

Every case that owns a `DataStructure` ends with `UnitTest::CheckArraysInheritTupleDims`, every
`getDataRefAs` is preceded by a `REQUIRE_NOTHROW` of the same call, and every loop assertion carries a
`CAPTURE` of its index.

### Mutation verification

14 mutations, each applied to the pristine branch, built, run against the full AlignSections family
(`ctest -R "AlignSections"`, 26 tests at the time; 27 after the fix-round guard test was added), then
reverted. Full transcript:
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

There are **two** edits to `src/simplnx/Utilities/AlignSections.cpp`, and both have to be shown to be
no-ops for the other three alignment filters.

**Edit 1 — the per-array transfer loop starts at index 0 instead of 1, and skips any index whose X and
Y shifts are both zero.**

*Source-derived:* the shift vectors are zero-initialized (`AlignSections.cpp:140-141`) and the four
`findShifts` overrides write only indices >= 1 — `AlignSectionsList.cpp:49,60`,
`AlignSectionsMisorientation.cpp:89,201`, `AlignSectionsMutualInformation.cpp:92,218`, and this
filter's own consecutive-mode branch. So index 0 is `(0,0)` for every consumer except this filter in
reference mode, and the new iteration is skipped. For `i >= 1` the skip is equally a no-op: with both
shifts zero the in-bounds test at `AlignSections.cpp:89-90` is always true, so the body would perform
`copyTuple(p, p)` and never reach `initializeTuple` — and `DataArray::copyTuple` itself opens with
`if(from == to) { return; }`, so the skip is a no-op for *any* zero-shift index rather than only for
index 0.

**Edit 2 — `AlignSections::execute` returns the `findShifts` result instead of `return {}`**, so a
Warning raised by `findShifts` reaches the pipeline (the AFC-9 / D7 fix).

*Source-derived:* this can only surface a warning that was previously being discarded. Grepping
`warnings()`, `MakeWarning` and `Warning{` across `AlignSectionsList.cpp`,
`AlignSectionsMisorientation.cpp` and `AlignSectionsMutualInformation.cpp` returns **zero hits** —
none of the other three `findShifts` implementations raises a warning at all, and their *errors*
already propagated through the pre-existing `if(foundShiftsResults.invalid())` early return. So no
previously-swallowed diagnostic newly appears for them.

*Executed:* `ctest -R "AlignSections"` — 27/27 pass, including `AlignSectionsListFilter` (2 execution
tests), `AlignSectionsMisorientation` (Small IN100 pipeline + output test),
`AlignSectionsMutualInformation` (4 tests), `PIPELINE::OrientationAnalysis::001_AlignSectionsMutualInformation`
and `PY::OrientationAnalysis_AlignSectionsMutualInformation`. Mutation M3 shows this suite is
genuinely sensitive to the skip's semantics: breaking the condition failed 9 of the 26 tests that
existed when the mutations were run, four of
them belonging to the other three alignment filters.

## Exemplar archive

`align_sections_feature_centroids.tar.gz`, retained. Provenance sidecar:
`vv/provenance/align_sections_feature_centroids.md`. SHA512 as it appears in
`src/Plugins/SimplnxCore/test/CMakeLists.txt:247`:

```
06a4f576108e96fff49241ed39bdcaa90b96ba9b6fa91f420bde9d797a21fc58880b52f4d070c0efbf5b193e7c9f0c0383c140db404c5e74c3ff8ce3b4c3ca9b
```

The dossier's open question — whether the `6_6_` prefix really means legacy provenance — is answered
from the file itself rather than from the naming convention. The `.dream3d` file's embedded `Pipeline`
group contains the eleven-filter DREAM3D 6.6 pipeline that produced it; filter `08` is
`AlignSectionsFeatureCentroid`, `FilterVersion 6.6.331`, `UseReferenceSlice = 1`,
`ReferenceSlice = 0`, `WriteAlignmentShifts = 1`, writing
`6_6_align_sections_feature_centroids.txt` — which is also in the archive. The `Exemplar Data`
container is therefore genuine legacy 6.6.331 output and is **not** circular.

That CSV makes an independent oracle possible for the real dataset. Running post-fix SIMPLNX on the
archive's unaligned `DataContainer` with `ReferenceSlice = 59` (= `zDim-1`) reproduces it on all 59
rows. What "reproduces" means exactly, because the comparison is deliberately not a whole-file one
(`ww_work/AlignSectionsFeatureCentroid/scripts/archive_check.py`, run captured at
`logs/archive_check.log`):

| CSV columns | Compared how | Result |
|---|---|---|
| 1-2 `Slice_A,Slice_B` | exact integer equality against `Slice Indices` | 59 of 59 agree |
| 3-4 `New X Shift,New Y Shift` | **not compared** — legacy never assigns these, so all 59 legacy rows carry `0,0` while SIMPLNX writes the real relative shifts. This is D3, a known legacy bug, not a match. | excluded by design |
| 5-6 `X Shift,Y Shift` | exact integer equality against `Cumulative Shifts` | 59 of 59 agree |
| 7-8 `X Centroid,Y Centroid` | `np.allclose(rtol=0, atol=6e-4)` against `Centroids` — a text-precision bound, because the CSV carries six significant digits and SIMPLNX stores float32 | 59 of 59 within tolerance |

So of the eight columns, four are bit-exact, two agree to the file's own printed precision, and two
are excluded because legacy is known to write a constant there. That is a third implementation
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

*Attribution of the patched-build differences:* the patch is provably the only thing that separates
the patched build from 6.5.171 on this filter's code. The pre-patch blobs of all four files the
alignment code lives in — `AlignSections.{h,cpp}` and `AlignSectionsFeatureCentroid.{h,cpp}` under
`Source/Plugins/Reconstruction/ReconstructionFilters/` — are **byte-identical** to the corresponding
6.5.171 sources (SHA-256 compared blob by blob), so every difference the patched build shows against
6.5.171 comes from the patch. A weaker check was also run: the unpatched build was executed on four of
the five configurations (`ab1`, `ab3`, `ab4`, `ab5`; `ab2` was prepared but never run) and produced
textually identical shift files to 6.5.171. Note that the `.dream3d` outputs are *not* byte-identical
between the two builds and were never expected to be — HDF5 output embeds the pipeline text and
timestamps — and no cell-array comparison of the unpatched build was performed, so the blob-identity
argument above is the one that carries the attribution.

| Run | Configuration | Predicted | Observed |
|---|---|---|---|
| ab1 | consecutive | cell arrays identical; relative-shift columns differ (legacy always 0) | as predicted |
| ab2 | reference, legacy `RS=0` vs SIMPLNX `RS=2` | cell arrays identical (the equivalence identity) | as predicted |
| ab3 | reference, both `RS=0` | cell arrays diverge; SIMPLNX shifts {-3,-2,0}/{-2,-2,0}, legacy {0,1,3}/{0,0,2} | as predicted, 24 Mask and 73 Payload cells differ (of 75) |
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
values gives **0 DIFFER out of 66 comparables** (plus 5 informational rows) — patched-legacy,
fixed-SIMPLNX and the hand-derived oracle agree on every Cell array, slice pair, relative shift,
cumulative shift and centroid, including the previously undefined empty-slice case.

A follow-up commit `f6e20f7e3` on the same tree carries the mode-accurate empty-section wording into
the patched build's warning 104 and makes its `dataCheck` range bound signed. It changes **message
text only** — no array, shift or shift-file value — so the A/B result above stands without a re-run.
That commit also records what `f81973147`'s message did not: the patch deletes warning conditions 102
and 103 (the NaN-centroid detectors, 18 lines), which are unreachable once an empty section can no
longer produce a NaN centroid and which SIMPLNX has no equivalent of.

## Deviations from DREAM3D 6.5.171

Full entries: `src/Plugins/SimplnxCore/vv/deviations/AlignSectionsFeatureCentroidFilter.md`.
The IDs below are the short form of the stable IDs used there and in migration guidance:
`D<N>` == `AlignSectionsFeatureCentroidFilter-D<N>`.

| ID | Deviation | Root cause | Disposition |
|---|---|---|---|
| D1 | Reference Slice selected physical slice `Z-1-k` instead of `k` | `bug` (shared with legacy) | **Fixed in both.** SIMPLNX on this branch; 6.5.172 in `f81973147`. Behavior change for any pipeline using a Reference Slice other than `Z-1`. |
| D2 | A fully masked-out section produced a NaN centroid and an undefined integer cast, poisoning the following section in consecutive mode | `bug` (shared with legacy) | **Fixed in both.** Zero relative shift + Warning; empty reference section is an error. |
| D3 | Legacy's "New X Shift"/"New Y Shift" CSV columns are always 0 | `bug` (legacy-only) | Documented; also fixed in the 6.5.172 patch. SIMPLNX has always written real values. |
| D4 | Legacy's Reference Slice guard used `>` so `RS == Z` passed and then read out of bounds; it also ran when the value was unused, and SIMPLNX had no upper bound at all | `bug` (legacy off-by-one) + missing guard (SIMPLNX) | **Fixed in both.** SIMPLNX preflight rejects the whole invalid range with `-68071` and only when *Use Reference Slice* is on; `-68064` covers negatives on the same condition. **D4 owns `-68071`**; the guards in D9 are the other four. |
| D5 | Shift output: CSV file -> four Data Arrays, `Z` tuples with tuple 0 present | `algorithmic choice` (deliberate, PR #1237) | Documented. Tuple 0 is a deterministic zero anchor row in consecutive mode and a real data row in reference mode. |
| D6 | Mask accepts `uint8` as well as `bool` | `algorithmic choice` (SIMPLNX superset) | Documented; parity verified (F9). |
| D7 | Diagnostics were `Info` messages with no Result warnings, and two of them had lost their format placeholder | `bug` (SIMPLNX-only) | **Fixed.** Warnings are on the Result again; the placeholder-less NaN messages were removed with the NaN path. |
| D8 | Tuple 0 of the four shift arrays was uninitialized memory | `bug` (SIMPLNX-only) | **Fixed** by a `"0"` fill value. Not observable as a test failure on this platform because fresh pages read as zero — the mutation `"0" -> "7"` is what proves the assertion bites. |
| D9 | Four missing preflight guards: non-3D geometry, non-Data-Array Cell child, missing Cell Attribute Matrix, and a Mask Array whose tuple count does not match the geometry's Cell count | `bug` (missing guards, SIMPLNX-only) | **Fixed** (`-68072`, `-68073`, `-68074`, `-68075`). The second was an uncaught `std::bad_cast` in the shared base, the third a null dereference and the fourth an uncaught `std::out_of_range` from `DataStore::at`. The Reference Slice bound `-68071` belongs to D4, not here. |
| D10 | Shifts truncate toward zero, and consecutive mode sums the *truncated* relative shifts so truncation error accumulates | `algorithmic choice` (shared design wart) | **Documented, not changed.** Changing it would alter every existing result. The filter documentation now states truncation instead of rounding, and F3/F11 pin the behavior. |
| D11 | `Slice Indices` records `{slice, slice+1}` even in reference mode, where the second component is not the section that was actually used as the target | `algorithmic choice` (shared, cosmetic) | **Documented, not changed.** Kept identical to legacy. Tuple 0's second component is consequently `Z`, one past the last slice. |
| D12 | Legacy's diagnostics named the iteration index, not the physical slice (`Slice=2` for physical slice 1). SIMPLNX names the physical slice in the empty-section warning **and** in both range warnings, where the pre-V&V SIMPLNX code also printed the iteration counter | `bug` (legacy), plus a SIMPLNX diagnostic improvement | Documented; the 6.5.172 patch adopts the physical index for its new warning 104. Its pre-existing warnings 100/101 still print `iter`, an internal inconsistency noted in the patch's follow-ups. |
| D13 | Dead error-code constants `-68001..-68004`, plus a documentation paragraph describing an option this filter has never had | `cleanup` (dead code and a documentation error; no computational effect — excluded from the bug-flag roll-up) | **Removed.** Fix round 1 additionally removed the `-68063` tuple-count guard, which called `validateNumberOfTuples` with a single path and therefore could never fail; the real check is now `-68075` (D9). |

## Test-suite results

All runs from `/Users/mjackson/Workspace9/DREAM3D-Build/NX-Com-Qt69-Vtk96-Rel` via `ctest`; no test
binary was invoked directly.

| Command | Result |
|---|---|
| `ctest -R "SimplnxCore::AlignSectionsFeatureCentroid"` | 15/15 pass |
| `ctest -R "AlignSections"` (all four alignment filters + PIPELINE + PY) | 27/27 pass |
| `ctest -R "SimplnxCore::"` | 997/997 pass |
| `ctest -R "OrientationAnalysis::"` | 293/293 pass, including `PIPELINE::OrientationAnalysis::002_...` (the `(02) Small IN100 Full Reconstruction` pipeline, which uses `reference_slice = 0` and therefore now anchors the section at the Z origin) and every `PY::` chained example pipeline |

*Out-of-core builds:* not run. The batch operates under the standing V&V decision of 2026-08-19 that
waives OOC test runs, so `report_gates.md`'s "both in-core and OOC builds" requirement is satisfied by
waiver, not by execution. Nothing in this cycle's changes is storage-backend specific: the new guards
are preflight metadata checks, and the algorithm's data access is unchanged apart from one extra
`std::vector<bool>`.

## Follow-ups for the human engineers

1. **Guard parity — needs a filed ticket, not a report bullet.** `AlignSectionsListFilter` and
   `AlignSectionsMutualInformationFilter` have the same four preflight gaps this cycle closed for
   FeatureCentroid (non-3D geometry, non-Data-Array Cell children, missing Cell Attribute Matrix, and
   mask/geometry tuple-count mismatch). Per the ratified scope no guard was added to the shared base,
   so those two filters remain exposed — and the `std::bad_cast` and `std::out_of_range` cases are
   *hard crashes in shared code that this cycle proved reachable*, not merely bad answers. This should
   be tracked in the issue queue rather than left here. `AlignSectionsMisorientation` is covered by the
   parallel task in this batch.
2. **`findShifts` duplication.** The store and no-store branches of this filter's shift loop remain
   near-duplicates (about 40 lines), including the two range-warning blocks, which the fix round
   deliberately left duplicated per plan decision 5 (behavior-preserving refactors ride separately).
   Worth a separate enhancement.
3. **Reference-slice semantics are a user-visible change.** Any saved pipeline that uses
   `use_reference_slice = true` with a value other than `zDim-1` will now align to a different
   section. The shipping `(02) Small IN100 Full Reconstruction.d3dpipeline` uses `reference_slice = 0`,
   which under the new semantics anchors the section at the Z origin rather than the far section; the
   pipeline was deliberately left unchanged because 0 is now exactly what the parameter documents.
   Note explicitly that **no test anywhere pins that pipeline's post-fix output** — the
   `PIPELINE::` and `PY::` cases assert only that the expected files are produced, not their contents,
   so the changed reconstruction result is unverified by the suite. This belongs in the release notes.
4. **6.5.172 patch internal inconsistency.** The patch's new empty-section warning 104 prints the
   physical slice while the adjacent pre-existing warnings 100 and 101 still print the iteration
   index. Harmless but confusing in a single run's log; worth a one-line follow-up on that tree.
