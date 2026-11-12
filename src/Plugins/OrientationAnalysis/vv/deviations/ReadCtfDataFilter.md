# Deviations from DREAM3D 6.5.171: ReadCtfDataFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

Entries are referenced by stable ID (`ReadCtfDataFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

Comparison run 2026-07-24 against the official DREAM3D 6.5.171 release (`PipelineRunner`): four runs over three byte-identical input files — a hand-authored 3×2 toy `.ctf` (two conversion-option combinations), the 550×400 production scan `Cugrid_after 2nd_15kv_2kx_2.ctf` (two phases — Cu3Ga hexagonal + Copper cubic — with 116,050 unindexed points), and a 2×2×2 multi-slice toy `.ctf`. On every run, all numeric outputs are **bit-identical** except the unindexed-point (Phase 0) family documented as D1/D2 (on the production scan: 543,950 of 660,000 Euler values match; the 116,050 differing values are all unindexed points' φ2). Three additional malformed-input fixtures were run through 6.5.171 to document its failure behavior (D3).

---

## ReadCtfDataFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ReadCtfDataFilter-D1` |
| **Filter UUID** | `7751923c-afb9-4032-8372-8078325c69a4` |
| **Status** | active |

**Symptom:** Unindexed scan points ("zero solutions") carry `Phases == 0` in SIMPLNX but `Phases == 1` in 6.5.171. On the Cugrid production fixture this affects 116,050 of 220,000 points.

**Root cause:** Algorithmic choice. 6.5.171's `ReadCtfData::copyRawEbsdData()` remaps every phase value `< 1` to `1` before storing the Phases array (a convention inherited from `H5CtfVolumeReader`: "the lowest value is One (1)"). SIMPLNX deliberately removed this remap in PR #937 (May 2024): a phase value of 0 is meaningful in `.ctf` files — it marks a point the indexing software could not solve — and rewriting it to 1 silently assigns those points to a real phase.

**Affected users:** Anyone importing `.ctf` files that contain unindexed points — which is nearly every real-world Oxford/HKL scan. Downstream workflows that segmented or masked using `Phases == 0` under legacy DREAM3D never saw phase 0 and instead thresholded on `Error > 0`; that workflow continues to work identically in SIMPLNX (`Error` is copied verbatim, bit-identical).

**Recommendation:** Trust SIMPLNX. Preserving the measured phase value is strictly more information; the legacy remap destroyed the unindexed marker and mislabeled unindexed points as phase 1. Users who need legacy-parity masks can threshold on `Error = 0` (recommended in the filter documentation) or replace phase-0 values explicitly.

---

## ReadCtfDataFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ReadCtfDataFilter-D2` |
| **Filter UUID** | `7751923c-afb9-4032-8372-8078325c69a4` |
| **Status** | active |

**Symptom:** When "Convert Hexagonal X-Axis to EDAX Standard" is enabled and the file's phase 1 is hexagonal (Laue group 9), the φ2 Euler angle of **unindexed** points differs by exactly 30° (0.5235988 rad): 6.5.171 applies the +30° shift to them, SIMPLNX does not. On the Cugrid fixture, all 116,050 unindexed points show legacy φ2 = 0.5235988 vs SIMPLNX φ2 = 0.0. Euler angles of every indexed point are bit-identical.

**Root cause:** Algorithmic choice (consequence of D1). Both versions decide the +30° hexagonal-alignment shift by looking up `CrystalStructures[Phases[i]]`. Legacy has already remapped unindexed points to phase 1, so they inherit phase 1's hexagonal classification and receive the shift; SIMPLNX looks up ensemble slot 0 (`UnknownCrystalStructure`, 999), which is never hexagonal, so unindexed points keep their file values.

**Affected users:** Only workflows that read Euler angles of unindexed points under the EDAX-alignment option — those angles are meaningless by definition (the point was never indexed; the file stores 0.0 there).

**Recommendation:** Trust SIMPLNX. Applying a crystallographic correction to points with no crystallographic solution was an artifact of the D1 remap, not a deliberate feature.

---

## ReadCtfDataFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ReadCtfDataFilter-D3` |
| **Filter UUID** | `7751923c-afb9-4032-8372-8078325c69a4` |
| **Status** | active |

**Symptom:** Malformed `.ctf` files that crash or silently corrupt 6.5.171 are rejected by SIMPLNX with descriptive errors. Empirically demonstrated on three fixtures against the official 6.5.171 release:

| Malformed input | DREAM3D 6.5.171 (demonstrated) | SIMPLNX |
|---|---|---|
| Data section missing a standard column (no `BS`) | **Segfault** (exit 139; `memcpy` from null buffer) | Error `-19601` naming the missing column |
| `Phases 0` header (no phase definitions) | **Segfault** (exit 139; reads a zero-tuple ensemble array out of bounds) | Error `-19600` |
| Data row phase value out of range (`Phase 7` with 2 phases) | **Silent success** (exit 0) — the +30° decision reads `CrystalStructures[7]` out of bounds, so the output depends on heap contents | Error `-19602` naming the point and valid range |
| Header `XCells`/`YCells` missing or 0 | Error at execute after a zero-sized preflight | Rejected at preflight with `-19604` |
| Header `ZCells 0` | Not run (legacy sizes the geometry from the same header) | Rejected at preflight with `-67601` (D7) |

**Root cause:** Bug in 6.5.171 (out-of-bounds access on malformed input) — fixed in SIMPLNX by value-add guards added during this V&V pass. On well-formed files the guards never fire and outputs are unaffected.

**Affected users:** Anyone importing truncated, hand-edited, or non-standard `.ctf` files. No effect on well-formed files.

**Recommendation:** Trust SIMPLNX. A clean error is strictly better than a crash or a heap-dependent output.

---

## ReadCtfDataFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ReadCtfDataFilter-D4` |
| **Filter UUID** | `7751923c-afb9-4032-8372-8078325c69a4` |
| **Status** | retired 2026-07-24 (resolved during this V&V pass — recorded for the migration guide) — fixed in DREAM3D-NX **7.4.2** |

**Symptom:** Before this V&V pass, SIMPLNX imported only slice 0 of a multi-slice (3D) `.ctf` file (`ZCells > 1`) — silently — while 6.5.171 imported the full 3D volume.

**Root cause:** Bug in the SIMPLNX port (the preflight hard-coded a z-extent of 1 and a z-spacing of 1.0, dropping the header's `ZCells`/`ZStep`). Restored during this pass; on the 2×2×2 fixture the SIMPLNX output now matches 6.5.171 bit-for-bit (dims, ZStep-derived spacing, and all nine cell arrays).

**Affected users:** Anyone who imported a multi-slice `.ctf` with an earlier DREAM3D-NX release received a single-slice geometry with slice-0 data and no warning.

**Recommendation:** Trust SIMPLNX (current). Re-import multi-slice `.ctf` files with a build containing this fix.

---

## ReadCtfDataFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `ReadCtfDataFilter-D5` |
| **Filter UUID** | `7751923c-afb9-4032-8372-8078325c69a4` |
| **Affected releases** | none through 7.4.2 — those releases matched 6.5.171 (6/mmm only) |
| **Status** | active — intentional correction introduced by PR #1781 (2026-10-14) |

**Symptom:** With EDAX hexagonal alignment enabled, simplnx adds 30 degrees to phi2 for both 6/m and 6/mmm phases; legacy DREAM3D 6.5 shifted only 6/mmm phases.

**Root cause:** The Oxford/HKL versus EDAX/TSL x-axis convention depends on the hexagonal lattice, so the correction also applies to `Hexagonal_Low` (6/m).

**Affected users:** Users importing 6/m phases with EDAX hexagonal alignment enabled.

**Correct behavior:** Both hexagonal Laue classes receive the same shift, before optional conversion to radians. Cubic and unindexed points remain unchanged; trigonal phases are outside this correction. Covered by `test/ReadCtfDataTest.cpp::"Euler Conversion Combinations"`.

**Recommendation:** Re-import 6/m data when EDAX alignment is required.


---

## ReadCtfDataFilter-D6

| Field | Value |
|---|---|
| **Deviation ID** | `ReadCtfDataFilter-D6` |
| **Filter UUID** | `7751923c-afb9-4032-8372-8078325c69a4` |
| **Status** | resolved on this branch |
| **Bug flag** | Header validation defect, reproduced with EbsdLib 3.1.3 |

**Symptom:** An unsupported Laue Group causes a segmentation fault in the preflight preview. Negative or non-finite XStep and YStep values reach the Image Geometry, including in multi-slice files.

**Root cause:** The preview uses a file-derived orientation-operations index without a bounds check. Geometry creation also requires validation of header dimensions and steps before unsigned conversion or spacing assignment.

**Affected users:** Users importing malformed or edited EBSD files. Valid input files retain their symmetry mappings and geometry.

**Correct behavior:** Preflight rejects unsupported Laue Group with error `-67600` and invalid steps with `-67602`. The messages identify the file, header key and value; the symmetry error also identifies the phase. Public execute also rejects the invalid header through its preflight check.

**Test sensitivity verification:** Synthetic files in `test/ReadCtfDataTest.cpp` reproduce the failure before the fix. The regression cases are `OrientationAnalysis::ReadCtfData: HeaderValidation symmetry` and `ReadCtfData: HeaderValidation steps`. The oracle is the supported symmetry domain and the finite, positive grid contract. The legacy executable was not run for these new malformed inputs.

**Recommendation:** Correct the named header value or export the file again from the acquisition software.


---

## ReadCtfDataFilter-D7

| Field | Value |
|---|---|
| **Deviation ID** | `ReadCtfDataFilter-D7` |
| **Filter UUID** | `7751923c-afb9-4032-8372-8078325c69a4` |
| **Status** | resolved on this branch |
| **Bug flag** | Preflight validation defect |

**Symptom:** An explicit ZCells 0 header passed preflight and failed only during data reading.

**Root cause:** The preflight clamped zero to one, although CtfReader retains explicit zero and initializes an absent ZCells key to one.

**Affected users:** Users importing malformed CTF files with an explicit zero slice count.

**Correct behavior:** Preflight rejects explicit zero with the shared dimension error `-67601`, naming the file, ZCells and value. The absent-key default remains one slice, and negative ZCells retains error `-19604`.

**Test sensitivity verification:** `Zero ZCells preflight rejected (-67601)` failed on the unchanged parent production code because preflight succeeded. The existing analytical oracle verifies the absent-key one-slice import; `Invalid Cells Preflight Error (-19604)` retains negative coverage. The oracle is the positive dimension contract. No new legacy comparison was run.

**Recommendation:** Correct ZCells to a positive slice count, or omit it for a 2D file.
