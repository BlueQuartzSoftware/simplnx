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
| Header `ZCells 0` | Not run (legacy sizes the geometry from the same header) | Rejected at execute with `-19603` |

**Root cause:** Bug in 6.5.171 (out-of-bounds access on malformed input) — fixed in SIMPLNX by value-add guards added during this V&V pass. On well-formed files the guards never fire and outputs are unaffected.

**Affected users:** Anyone importing truncated, hand-edited, or non-standard `.ctf` files. No effect on well-formed files.

**Recommendation:** Trust SIMPLNX. A clean error is strictly better than a crash or a heap-dependent output.

---

## ReadCtfDataFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ReadCtfDataFilter-D4` |
| **Filter UUID** | `7751923c-afb9-4032-8372-8078325c69a4` |
| **Status** | retired 2026-07-24 (resolved during this V&V pass — recorded for the migration guide) |

**Symptom:** Before this V&V pass, SIMPLNX imported only slice 0 of a multi-slice (3D) `.ctf` file (`ZCells > 1`) — silently — while 6.5.171 imported the full 3D volume.

**Root cause:** Bug in the SIMPLNX port (the preflight hard-coded a z-extent of 1 and a z-spacing of 1.0, dropping the header's `ZCells`/`ZStep`). Restored during this pass; on the 2×2×2 fixture the SIMPLNX output now matches 6.5.171 bit-for-bit (dims, ZStep-derived spacing, and all nine cell arrays).

**Affected users:** Anyone who imported a multi-slice `.ctf` with an earlier DREAM3D-NX release received a single-slice geometry with slice-0 data and no warning.

**Recommendation:** Trust SIMPLNX (current). Re-import multi-slice `.ctf` files with a build containing this fix.
