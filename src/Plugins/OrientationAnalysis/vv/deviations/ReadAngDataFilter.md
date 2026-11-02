# Deviations from DREAM.3D v6.5.171: ReadAngDataFilter

Part 1 lists differences from DREAM.3D v6.5.171 (D1–D5). Part 2 records the shared reader-buffer defect fixed in SIMPLNX (D6). The entries share one stable ID sequence.

## Part 1 — Differences from DREAM.3D v6.5.171

### ReadAngDataFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ReadAngDataFilter-D1` |
| **Filter UUID** | `5b062816-79ac-47ce-93cb-e7966896bcbd` |
| **Status** | active |

**Symptom:** DREAM.3D v6.5.171 stores material names such as `"Nickel "`; SIMPLNX stores `"Nickel"`.

**Root cause:** Algorithmic choice. `ReadAngData::loadMaterialInfo()` trims the name before storing it. DREAM.3D v6.5.171 stores the reader's name without trimming.

**Affected users:** Users who compare material-name strings with DREAM.3D v6.5.171 output.

**Recommendation:** Trust SIMPLNX. Trim surrounding whitespace when comparing material names across applications.

**Evidence:** The current algorithm and the tagged DREAM.3D v6.5.171 `ReadAngData.cpp` show the two assignments. `vv_work:ReadAngDataFilter/ComparisonSummary.md` records the trailing-space difference on the toy and Small IN100 fixtures on 2026-07-07. The analytical oracle checks both trimmed names.

---

### ReadAngDataFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ReadAngDataFilter-D2` |
| **Filter UUID** | `5b062816-79ac-47ce-93cb-e7966896bcbd` |
| **Status** | active |

**Symptom:** Headers containing `# TEM data` or `# File Created from ACOM RES results` receive Nanometer geometry units in DREAM.3D v6.5.171 and Micrometer units in SIMPLNX.

**Root cause:** Algorithmic choice. DREAM.3D v6.5.171 detects these markers in `readDataFile()`. SIMPLNX always creates the Image Geometry with `LengthUnit::Micrometer`; both use the header's numeric spacing values.

**Affected users:** Users who import files with either marker and use geometry units to interpret physical dimensions.

**Recommendation:** Set the imported geometry units to Nanometer when those units describe the source scan. The unchanged spacing numbers do not compensate for a different unit label.

**Evidence:** Tagged DREAM.3D v6.5.171 `ReadAngData.cpp` contains both marker checks and unit assignments. Current `ReadAngDataFilter.cpp` supplies Micrometer to `CreateImageGeometryAction`. No current fixture contains either marker.

---

### ReadAngDataFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ReadAngDataFilter-D3` |
| **Filter UUID** | `5b062816-79ac-47ce-93cb-e7966896bcbd` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** A file that declares only phase 2 can cause an out-of-bounds ensemble write and a crash. SIMPLNX now imports that file with three ensemble tuples.

**Root cause:** Bug. The affected versions allocate `phaseCount + 1` tuples but write each phase at its declared index. SIMPLNX now allocates `max(maxPhaseIndex, phaseCount) + 1` tuples and initializes every slot to invalid-phase defaults. It returns `-19502` for an index below 1 and `-19504` for an index at or above the allocated count.

**Affected users:** Users with sparse phase indices. A file change after preflight can also put a newly declared phase outside the allocated arrays.

**Recommendation:** Trust SIMPLNX in DREAM3D-NX v7.4.2 or later. Keep the input file unchanged during import.

**Evidence:** Every release source from `upstream/release/v7.0.0` through `upstream/release/v7.4.1` has count-based allocation and unchecked writes. The v7.4.2 source contains the PR #1657 correction (`49ea8b09c`). `Non-Contiguous Phase Index` asserts three tuples and all unused-slot defaults. `vv_work:ReadAngDataFilter/ComparisonSummary.md` records the stock DREAM.3D v6.5.171 crash on 2026-07-07; `vv_work:ReadAngDataFilter/ReadMe.md` and `vv_work:ReadAngDataFilter/LegacyPatch.md` record the successful local proof version on 2026-09-17.

---

### ReadAngDataFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ReadAngDataFilter-D4` |
| **Filter UUID** | `5b062816-79ac-47ce-93cb-e7966896bcbd` |
| **Status** | active |

**Symptom:** HexGrid gives `-1000` in DREAM.3D v6.5.171 and `-19500` in SIMPLNX. A missing GRID key gives reader error `-300` during legacy execution and `-19501` during SIMPLNX preflight.

**Root cause:** Algorithmic choice. SIMPLNX adds a missing-GRID check to preflight and uses its own filter error codes. `FileSystemPathParameter` performs file-path validation.

**Affected users:** Scripts that match error codes or depend on the stage at which an invalid input fails.

**Recommendation:** Either rejection is acceptable. Update error-code matching to the SIMPLNX codes.

**Evidence:** The tagged legacy filter and `Source/EbsdLib/TSL/AngReader.cpp` contain `-1000` and `-300`. Current preflight and the `HexGrid Preflight Error (-19500)` and `Missing GRID Preflight Error (-19501)` cases establish the SIMPLNX behavior.

---

### ReadAngDataFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `ReadAngDataFilter-D5` |
| **Filter UUID** | `5b062816-79ac-47ce-93cb-e7966896bcbd` |
| **Status** | active |

**Symptom:** DREAM.3D v6.5.171 writes a `# Phase 0` section into ensemble slot 0. SIMPLNX rejects that section with `-19502`.

**Root cause:** Algorithmic choice. DREAM.3D v6.5.171 skips negative header indices but accepts zero. SIMPLNX reserves slot 0 for invalid-phase defaults and rejects every header index below 1. Cell phase values below 1 still map to 1 during copying.

**Affected users:** Users whose ANG headers declare phase 0 or a negative phase index.

**Recommendation:** Trust SIMPLNX. Use positive phase indices in the header and update the corresponding indexed cell values.

**Evidence:** Tagged DREAM.3D v6.5.171 `loadMaterialInfo()` checks `phaseID < 0`; current SIMPLNX checks `phaseID < 1`. `Phase 0 rejected (-19502)` checks this static input. The analytical oracle separately checks the cell-phase remap.

---

## Part 2 — SIMPLNX defects and behavior changes

### ReadAngDataFilter-D6

| Field | Value |
|---|---|
| **Deviation ID** | `ReadAngDataFilter-D6` |
| **Filter UUID** | `5b062816-79ac-47ce-93cb-e7966896bcbd` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected by inconsistent column counts. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Import can read outside the reader buffers when the geometry contains more cells than the reader supplies.

**Root cause:** Bug. The copy loops use the geometry cell count without checking the reader count. For a square grid, `getXDimension()` uses `NCOLS_EVEN`, while reader allocation uses a positive `NCOLS_ODD` first. A malformed header with `NCOLS_EVEN > NCOLS_ODD` can therefore make the copies exceed the buffers. In SIMPLNX, a smaller file read after preflight can cause the same mismatch. The filter now returns `-19503` before accessing those buffers.

**Affected users:** Users with inconsistent square-grid column counts, or an input file that shrinks between SIMPLNX preflight and execution.

**Recommendation:** Trust SIMPLNX in DREAM3D-NX v7.4.2 or later. Correct inconsistent column counts and keep the input file unchanged during import.

**Evidence:** All release sources through `upstream/release/v7.4.1` lack the reader-count guard; v7.4.2 contains the PR #1657 fix (`49ea8b09c`). Tagged DREAM.3D v6.5.171 `ReadAngData::copyRawEbsdData()` also copies the geometry count without a bounds check; tagged `Source/EbsdLib/TSL/AngReader.cpp` confirms the even-column geometry and odd-column allocation. No current test directly triggers `-19503`.
