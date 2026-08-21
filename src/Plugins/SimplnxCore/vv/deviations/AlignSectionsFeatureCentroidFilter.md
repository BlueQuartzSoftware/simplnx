# Deviations from DREAM3D 6.5.171: AlignSectionsFeatureCentroidFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent (`AlignSectionsFeatureCentroid`, UUID `{886f8b46-51b6-5682-a289-6febd10b7ef0}`).

Entries are referenced by stable ID (`AlignSectionsFeatureCentroidFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

## Headline: A/B performed against 6.5.171, and a matching 6.5.172 alignment patch exists

Five configurations (consecutive; reference anchored three different ways; one fully masked-out section) were run through DREAM3D 6.5.171 `PipelineRunner` and through SIMPLNX `nxrunner` on byte-identical legacy-format inputs, comparing every Cell array element-wise plus the legacy shift CSV against the SIMPLNX shift arrays: 130 comparables, 21 divergences, **all predicted from source before either binary was run**. The same five configurations were then run through a patched 6.5.172 tree (`/Users/mjackson/Workspace9/6.5.172/DREAM3D`, commit `f81973147`), which agrees with SIMPLNX on **every** comparable.

---

## AlignSectionsFeatureCentroidFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `AlignSectionsFeatureCentroidFilter-D1` |
| **Filter UUID** | `b83f9bae-9ccf-4932-96c3-7f2fdb091452` |
| **Status** | active (shared bug **fixed in both code bases during this V&V cycle**) |

**Symptom:** With *Use Reference Slice* enabled, `Reference Slice = k` aligned every section to physical slice `Z-1-k` rather than to physical slice `k`. Only `k = Z-1` (and, for a single-slice-thick request, coincidence) selected the section the user asked for. Nothing warned, and for `k = 0` — the default, and the value in the shipping `(02) Small IN100 Full Reconstruction` pipeline — the filter anchored on the section farthest from the Z origin instead of the one at it.

**Root cause:** Algorithmic choice inherited from a common ancestor, i.e. a shared bug. The per-slice centroid arrays are filled by an iteration that walks the stack from the far end toward the Z origin: `slice = (dims[2] - 1) - iter` (SIMPLNX `Algorithms/AlignSectionsFeatureCentroid.cpp:101`, legacy `AlignSectionsFeatureCentroid.cpp:207`), so `xCentroid[i]` holds the centroid of physical slice `Z-1-i`. The user's value was then used directly as an index into that array (`xCentroid[static_cast<size_t>(m_InputValues->ReferenceSlice)]`, legacy `xCentroid[static_cast<size_t>(m_ReferenceSlice)]`), which reverses the meaning of the parameter. The parameter has always been documented as a slice number.

The fix has two parts, because correcting the index alone is not enough. The shift loop began at index 1, treating the far section as an unmovable anchor, and the shared transfer loop in `src/simplnx/Utilities/AlignSections.cpp` also began at index 1. In reference mode the far section is an ordinary section that must move like any other, so the shift loop now starts at 0 in reference mode and the transfer loop starts at 0 unconditionally, skipping any section whose X and Y shifts are both zero. That skip is what keeps the transfer-loop change a no-op for `AlignSectionsList`, `AlignSectionsMisorientation` and `AlignSectionsMutualInformation`, whose `findShifts` implementations only ever write indices 1 and up.

**Equivalence mapping for legacy parity:** old `Reference Slice = 0` is exactly new `Reference Slice = Z-1`. Both resolve to centroid index 0. Confirmed three ways: by derivation, by the `ab2` A/B configuration (cell arrays byte-identical to 6.5.171), and by reproducing the exemplar archive's DREAM3D 6.6.331 shift CSV on all 59 of its rows.

**Affected users:** Anyone who ran the filter with *Use Reference Slice* enabled and a Reference Slice other than `Z-1`, which includes every user of the default value 0 and of the shipping Small IN100 reconstruction pipeline. Consecutive mode (the parameter default) is unaffected — it never reads Reference Slice.

**Recommendation:** Trust SIMPLNX at or after this cycle, and 6.5.172 at or after `f81973147`. Results from earlier builds that used a non-`Z-1` Reference Slice aligned to the wrong section and should be regenerated. Users who deliberately want the old behavior can request `Reference Slice = Z-1`.

---

## AlignSectionsFeatureCentroidFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `AlignSectionsFeatureCentroidFilter-D2` |
| **Filter UUID** | `b83f9bae-9ccf-4932-96c3-7f2fdb091452` |
| **Status** | active (shared bug **fixed in both code bases during this V&V cycle**) |

**Symptom:** A section in which no Cell is flagged in the mask produced undefined behavior. In consecutive mode it also corrupted the shift of the *following* section, so a single empty section could leave the whole remainder of the stack mis-aligned with no error and only a garbled warning.

**Root cause:** Bug. The centroid is `sum / count` with no guard on `count`, so an empty section gives `0.0f / 0.0f = NaN` (SIMPLNX `alg cpp:101-102` pre-fix, legacy `fc.cpp:221-222`). The relative shift then evaluates `static_cast<int64_t>(NaN)`, which is undefined behavior: arm64 `fcvtzs` yields 0 and x86-64 `cvttss2si` yields `INT64_MIN`. In consecutive mode the next section computes `trunc(centroid - NaN)`, which is also NaN and also cast, so the accumulated shift stops tracking the data. Both code bases only *warned* about the NaN after the cast had already happened, and the legacy warning named the iteration index rather than the slice.

The observed 6.5.171 behavior on the `ab5` fixture (arm64 build) is the `0` branch: the empty section and the section after it both kept cumulative shift 1 where the correct answer for the second is 2, and the CSV recorded `nan,nan` centroids.

**Fix (both code bases):** an empty section contributes a relative shift of 0, so it keeps the shift of the section before it and travels with the stack; a Warning names the physical slice; and the last usable centroid is carried forward so the following sections still align to real data. If the *reference* section is the empty one there is no alignment target at all and the filter fails at execute time (SIMPLNX `-53901`, 6.5.172 `-5557`). No path casts a NaN any more.

**Affected users:** Anyone whose mask leaves an entire section unflagged — common when a threshold is tight, when a section is badly indexed, or at the ends of a scan. In consecutive mode the damage propagates to every section past the empty one. Platform-dependent, so the same data could align differently on Intel and Apple Silicon builds.

**Recommendation:** Trust SIMPLNX at or after this cycle, and 6.5.172 at or after `f81973147`. Any earlier result computed on data with a fully masked-out section should be regenerated, and results produced on different architectures should not be compared.

---

## AlignSectionsFeatureCentroidFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `AlignSectionsFeatureCentroidFilter-D3` |
| **Filter UUID** | `b83f9bae-9ccf-4932-96c3-7f2fdb091452` |
| **Status** | active (legacy-only bug; also fixed in the 6.5.172 patch) |

**Symptom:** The DREAM3D 6.5.171 alignment shift file's "New X Shift" and "New Y Shift" columns are always `0`, whatever the data. SIMPLNX's `Relative Shifts` array holds the real per-section values.

**Root cause:** Bug (legacy). `newxshift` and `newyshift` are declared `size_t newxshift = 0` / `size_t newyshift = 0` (`fc.cpp:187-188`) and are never assigned before being written to the file at `fc.cpp:282`. The columns were presumably meant to carry the per-section relative shift; the variables were left over from an earlier revision. Observed on every A/B run and also visible in the archive's own DREAM3D 6.6.331 CSV, so the bug survived into the 6.6 line.

**Affected users:** Anyone who read columns 3 and 4 of a DREAM3D 6.5.x or 6.6.x alignment shift file and expected the per-section relative shift. Columns 5 and 6 (the cumulative shifts) were always correct.

**Recommendation:** Trust SIMPLNX. In legacy output, derive the relative shifts by differencing the cumulative columns.

---

## AlignSectionsFeatureCentroidFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `AlignSectionsFeatureCentroidFilter-D4` |
| **Filter UUID** | `b83f9bae-9ccf-4932-96c3-7f2fdb091452` |
| **Status** | active (legacy-only bug + SIMPLNX guard gap, both closed) |

**Symptom:** Neither code base rejected every out-of-range Reference Slice. Legacy tested `m_ReferenceSlice > image->getZPoints()` (`fc.cpp:131`), so `Reference Slice == Z` passed the check and then read one past the end of the centroid vector; and the check ran even when *Use Reference Slice* was off, so a value that would never be read could still fail the pipeline. SIMPLNX had no upper bound at all — only `Reference Slice < 0` — so any value at or above the Z dimension read past the end of a `std::vector`.

**Root cause:** Bug (legacy off-by-one) plus a missing guard (SIMPLNX). Both produce an out-of-bounds read rather than a diagnostic.

**Fix:** SIMPLNX preflight now rejects `Reference Slice >= zDim` with `-68071` and `Reference Slice < 0` with `-68064`, both only when *Use Reference Slice* is enabled, and the algorithm carries a defensive execute-time equivalent (`-53905`) for callers that bypass preflight. The 6.5.172 patch applies the same rule.

**Affected users:** Anyone who typed a Reference Slice at or beyond the section count. The read is out of bounds, so the resulting shifts are arbitrary and the run may or may not crash.

**Recommendation:** Trust SIMPLNX at or after this cycle. Re-run anything that used an out-of-range Reference Slice.

---

## AlignSectionsFeatureCentroidFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `AlignSectionsFeatureCentroidFilter-D5` |
| **Filter UUID** | `b83f9bae-9ccf-4932-96c3-7f2fdb091452` |
| **Status** | active (deliberate, PR #1237) |

**Symptom:** DREAM3D 6.5.171 writes the shifts to a CSV text file; SIMPLNX stores them as four Data Arrays in a new Attribute Matrix. The legacy file has `Z-1` rows, one per moved section; the SIMPLNX arrays have `Z` tuples, so tuple 0 has no legacy counterpart.

**Root cause:** Algorithmic choice — a deliberate modernization in PR #1237 ("Alignment Filters Modernization"), which also bumped `parametersVersion` from 1 to 2 and provides the file-name-to-Attribute-Matrix-name conversion for old pipelines.

**Mapping for comparison:** legacy row for iteration index `i` corresponds to SIMPLNX tuple `i`. Legacy columns 1-2 map to `Slice Indices`, 3-4 to `Relative Shifts` (see D3), 5-6 to `Cumulative Shifts`, 7-8 to `Centroids`. Legacy prints centroids with the default six significant digits; SIMPLNX stores float32.

**Tuple 0 contract:** in consecutive mode the far section is the anchor, never moves, and its tuple is a deterministic all-zero row. In reference mode that section is aligned like any other, so tuple 0 carries its real slice pair, relative shift, cumulative shift and centroid.

**Affected users:** Anyone with a DREAM3D 6.x pipeline that consumed the shift file. Writing the Attribute Matrix out with a text writer reproduces the same information.

**Recommendation:** Either is acceptable; they carry the same information apart from D3.

---

## AlignSectionsFeatureCentroidFilter-D6

| Field | Value |
|---|---|
| **Deviation ID** | `AlignSectionsFeatureCentroidFilter-D6` |
| **Filter UUID** | `b83f9bae-9ccf-4932-96c3-7f2fdb091452` |
| **Status** | active (SIMPLNX superset) |

**Symptom:** SIMPLNX accepts a `bool` **or** `uint8` mask, treating any non-zero `uint8` as in-mask. DREAM3D 6.5.171 requires a strict `DataArray<bool>` and refuses anything else at preflight.

**Root cause:** Algorithmic choice. `MaskCompareUtilities` (`MaskCompareUtilities.hpp:121`) provides the non-zero comparison for `uint8`.

**Affected users:** Nobody negatively — this is a strict superset. It matters for A/B work: a legacy comparison fixture must use a bool mask.

**Recommendation:** Trust SIMPLNX. Verified equal to the bool path on identical layouts by the `UInt8 Mask Parity` test.

---

## AlignSectionsFeatureCentroidFilter-D7

| Field | Value |
|---|---|
| **Deviation ID** | `AlignSectionsFeatureCentroidFilter-D7` |
| **Filter UUID** | `b83f9bae-9ccf-4932-96c3-7f2fdb091452` |
| **Status** | active (SIMPLNX-only bug **fixed during this V&V cycle**) |

**Symptom:** Every diagnostic this filter raised was an `Info` message, so a pipeline whose shifts had gone wild completed with a clean Result and nothing recorded. DREAM3D 6.5.171 set warning conditions 100-103 for the same situations. Two of the four SIMPLNX messages had additionally lost their `{}` placeholder, so they printed `Slice=` with no number at all.

**Root cause:** Bug (SIMPLNX), introduced during the messaging sweeps of PRs #1267/#1340. The `Info`-only calls are at pre-fix `alg cpp:143/151/157/163` and `200/208/214/220`; the placeholder-less strings are pre-fix `alg cpp:156/162/213/219`.

**Fix:** the out-of-range diagnostics and the new empty-slice diagnostic are `Warning` entries on the returned `Result` (`-53902`, `-53903`, `-53904`), and `AlignSections::execute` now carries the shift-search result forward instead of discarding it, so those warnings reach the pipeline. The two placeholder-less messages described NaN centroids, which the D2 fix makes unreachable, so they were removed rather than repaired.

**Affected users:** Anyone who relied on a pipeline's warning list to notice a bad alignment on any SIMPLNX build before this cycle.

**Recommendation:** Trust SIMPLNX at or after this cycle.

---

## AlignSectionsFeatureCentroidFilter-D8

| Field | Value |
|---|---|
| **Deviation ID** | `AlignSectionsFeatureCentroidFilter-D8` |
| **Filter UUID** | `b83f9bae-9ccf-4932-96c3-7f2fdb091452` |
| **Status** | active (SIMPLNX-only bug **fixed during this V&V cycle**) |

**Symptom:** Tuple 0 of all four optional shift arrays was uninitialized memory. In practice it usually read as zero because a fresh allocation lands on zeroed OS pages, but nothing guaranteed it, and a long-running session that reuses heap could produce arbitrary slice indices, shifts and centroids in that tuple.

**Root cause:** Bug (SIMPLNX-only; the legacy file simply has no row for that section). The four `CreateArrayAction`s were constructed without a fill value, and `DataStore` allocates with `new value_type[count]` and only fills when an init value is present (`DataStore.hpp:249`, `ArrayCreationUtilities.hpp:91-104`, and `DataIOCollection::createDataStore` passes an empty init value). The shift loop wrote tuples 1 and up only.

**Fix:** all four arrays are created with a `"0"` fill value, and in reference mode the loop now writes tuple 0 with its real values.

**Verification note:** this defect is nondeterminism, not a wrong value, so a unit test cannot observe it directly on a platform where fresh pages read as zero. The assertion is proven to bite by mutation M7, which changes the fill value from `"0"` to `"7"` and fails six tests.

**Affected users:** Anyone reading tuple 0 of the shift arrays on any SIMPLNX build before this cycle, and anyone feeding those arrays into `AlignSectionsList` (which ignores tuple 0, so it was insulated by luck).

**Recommendation:** Trust SIMPLNX at or after this cycle.

---

## AlignSectionsFeatureCentroidFilter-D9

| Field | Value |
|---|---|
| **Deviation ID** | `AlignSectionsFeatureCentroidFilter-D9` |
| **Filter UUID** | `b83f9bae-9ccf-4932-96c3-7f2fdb091452` |
| **Status** | active (SIMPLNX-only guard gaps **closed during this V&V cycle**) |

**Symptom:** Three inputs that DREAM3D 6.5.171 either rejected or tolerated were accepted by SIMPLNX and then went wrong at execute time:

* A geometry with any dimension of 1 or 0. Legacy errored `-3010` (`AlignSections.cpp:217-223`); SIMPLNX silently no-oped a single-slice volume and would happily "align" a planar one.
* A Cell Attribute Matrix holding a `StringArray` or `NeighborList`. Both derive from `IArray` but not `IDataArray` (`StringArray.hpp:10`, `INeighborList.hpp:17`), and the shared transfer step reaches them through `getDataRefAs<IDataArray>` (`AlignSections.cpp:168`), which is a `dynamic_cast` to a reference and therefore throws `std::bad_cast`. `IFilter::execute` has no `try`/`catch`, so the exception escapes the filter. DREAM3D 6.5.171 shifted NeighborLists without complaint because SIMPL's `IDataArray` interface included `copyTuple`.
* An Image Geometry with no Cell Attribute Matrix. `AlignSections::getSelectedDataPaths` dereferences `imageGeom.getCellData()` unconditionally (`AlignSections.cpp:183`), so the run ends in a null dereference.

**Root cause:** Missing guards (SIMPLNX), all three arising from the port dropping legacy's `dataCheck` coverage.

**Fix:** preflight rejects a non-3D geometry with `-68072` (restoring the legacy semantics with the actual dimensions in the message), a non-Data-Array Cell child with `-68073` (naming the offending objects), and a missing Cell Attribute Matrix with `-68074`. Per the ratified scope for this cycle the guards live in this filter's preflight, not in the shared base, so `AlignSectionsList` and `AlignSectionsMutualInformation` remain exposed — logged as a follow-up.

**Affected users:** Anyone pointing the filter at a 2D or single-slice geometry, or at a Cell Attribute Matrix that also holds a string or neighbor-list array. The last case is a hard crash, not a bad answer.

**Recommendation:** Trust SIMPLNX at or after this cycle.

---

## AlignSectionsFeatureCentroidFilter-D10

| Field | Value |
|---|---|
| **Deviation ID** | `AlignSectionsFeatureCentroidFilter-D10` |
| **Filter UUID** | `b83f9bae-9ccf-4932-96c3-7f2fdb091452` |
| **Status** | active (shared design wart, documented, deliberately unchanged) |

**Symptom:** Shifts are truncated toward zero, not rounded to the nearest Cell. A centroid offset of 0.9 Cells produces no shift at all, and an offset of -1.9 Cells produces -1. The SIMPLNX documentation claimed the shifts are "rounded to the nearest multiple of the Cell resolution". In consecutive mode the *truncated* per-section shifts are summed, so the truncation error accumulates down the stack rather than cancelling.

**Root cause:** Algorithmic choice, identical in both code bases: `static_cast<int64_t>` of the centroid difference divided by the spacing.

**Affected users:** Everybody, mildly. Sub-Cell misalignments are never corrected, and a long stack of small same-signed offsets drifts.

**Recommendation:** Either acceptable — but be aware of the behavior. Changing truncation to rounding would alter every existing result for every user, so it was left alone; the documentation was corrected instead, and the `Shifts Truncate Toward Zero` and `Accumulated Shift Beyond The X Dimension Warns` tests pin both the direction and the accumulation.

---

## AlignSectionsFeatureCentroidFilter-D11

| Field | Value |
|---|---|
| **Deviation ID** | `AlignSectionsFeatureCentroidFilter-D11` |
| **Filter UUID** | `b83f9bae-9ccf-4932-96c3-7f2fdb091452` |
| **Status** | active (shared, cosmetic, deliberately unchanged) |

**Symptom:** The `Slice Indices` array (legacy CSV columns 1-2) always records the pair `{slice, slice+1}`, which describes the consecutive-mode pairing. In reference mode the second component is not the section that was actually used as the alignment target. For the far section's tuple the second component is `Z`, one past the last valid slice index.

**Root cause:** Algorithmic choice inherited from legacy, which writes the same pair in both modes.

**Affected users:** Anyone reading the second component of `Slice Indices` as "the section this one was aligned to" while in reference mode. The Reference Slice parameter is the authoritative answer there.

**Recommendation:** Either acceptable. Left identical to legacy so the arrays remain directly comparable with 6.5.x output; changing it would be a gratuitous output change inside a V&V cycle.

---

## AlignSectionsFeatureCentroidFilter-D12

| Field | Value |
|---|---|
| **Deviation ID** | `AlignSectionsFeatureCentroidFilter-D12` |
| **Filter UUID** | `b83f9bae-9ccf-4932-96c3-7f2fdb091452` |
| **Status** | active (SIMPLNX improvement) |

**Symptom:** DREAM3D 6.5.171's empty-section diagnostic named the *iteration* index, not the slice: for the `ab5` fixture, whose empty section is physical slice 1 of a four-slice volume, legacy reported `Slice=2`. SIMPLNX reports the physical slice.

**Root cause:** Bug (legacy). The warning interpolated the loop counter `iter` rather than `slice = dims[2]-1-iter` (`fc.cpp:267,276`), even though the very same loop had already computed `slice` for the shift file.

**Affected users:** Anyone who tried to find the offending section from a DREAM3D 6.5.x or 6.6.x warning message. The reported number is the mirror image of the real one.

**Recommendation:** Trust SIMPLNX. The 6.5.172 patch adopts the physical index as well.

---

## AlignSectionsFeatureCentroidFilter-D13

| Field | Value |
|---|---|
| **Deviation ID** | `AlignSectionsFeatureCentroidFilter-D13` |
| **Filter UUID** | `b83f9bae-9ccf-4932-96c3-7f2fdb091452` |
| **Status** | retired 2026-08-21 (cruft removed, no user-visible effect) |

**Symptom:** None. Four error-code constants `-68001`, `-68002`, `-68003` and `-68004` were defined in the filter's translation unit and never used, and the documentation described a "Linear Background Subtraction" option that this filter has never had (copied in from the Misorientation-family documentation).

**Root cause:** Cruft.

**Affected users:** Nobody. Recorded because a reader comparing the SIMPLNX and DREAM3D error-code tables would find four SIMPLNX codes with no counterpart and no trigger.

**Recommendation:** N/A — both the constants and the phantom paragraph were removed.

---

## Non-deviations (confirmed correct — do not "fix")

* **`xShifts[0]` is zero in consecutive mode.** The far section is the anchor by design; there is no preceding section to align it to. This is not the D1 bug.
* **The `Slice Indices` array is `uint32` while the slice pair for the far section reaches `Z`.** `Z` is a valid `uint32`; see D11 for the semantics.
* **Cross-suppression of the range warnings.** The one-shot `xWarning` flag means only the first out-of-range X shift is reported per run, which matches legacy. Before this cycle the flag also suppressed the NaN-centroid message and vice versa; that interaction disappeared with the NaN path (see D2 and D7), so only same-axis suppression remains, which is the intended "report once" behavior.
* **`ParallelTaskAlgorithm` over arrays rather than over Cells.** One array is always processed by exactly one thread, which is what the project's DataArray thread-safety rule requires. Legacy did the same thing with a raw TBB task group.
* **The mask array is itself shifted.** It is a child of the Cell Attribute Matrix, so it moves with everything else — in both code bases. That is what makes the aligned mask blocks stack up, and the oracle tests assert it explicitly.
