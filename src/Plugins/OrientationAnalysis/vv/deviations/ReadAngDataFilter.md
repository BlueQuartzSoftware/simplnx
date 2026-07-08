# Deviations from DREAM3D 6.5.171: ReadAngDataFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

Entries are referenced by stable ID (`ReadAngDataFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

Comparison run 2026-07-07 against the official DREAM3D 6.5.171 release on three fixtures (hand-authored 2-phase toy `.ang`, Small IN100 `Slice_1.ang`, and a non-contiguous-phase-index variant of the toy file). **All numeric outputs — cell arrays, ensemble arrays, and geometry — were bit-identical on the two supported-format fixtures.** The deviations below are the complete list of differences.

---

## ReadAngDataFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ReadAngDataFilter-D1` |
| **Filter UUID** | `5b062816-79ac-47ce-93cb-e7966896bcbd` |
| **Status** | active |

**Symptom:** Material names in the Ensemble Attribute Matrix carry a trailing space in 6.5.171 (`"Nickel "`) but are trimmed in SIMPLNX (`"Nickel"`).

**Root cause:** Algorithmic choice. EbsdLib's `AngPhase::parseMaterialName()` (both legacy and 3.0.0) rejoins the header tokens with a trailing space. SIMPLNX `ReadAngData::loadMaterialInfo()` applies `StringUtilities::trimmed()` before storing the name; 6.5.171 stored the raw string. Demonstrated on both comparison fixtures (2026-07-07).

**Affected users:** Any workflow that string-matches material names against `.dream3d` files produced by 6.5.171 (e.g., scripting against `MaterialName`). Visualization users will not notice.

**Recommendation:** Trust SIMPLNX. The trailing space is a parsing artifact, not information; the trimmed name is the name as written in the `.ang` header.

---

## ReadAngDataFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ReadAngDataFilter-D2` |
| **Filter UUID** | `5b062816-79ac-47ce-93cb-e7966896bcbd` |
| **Status** | active |

**Symptom:** For `.ang` files whose header contains `# TEM data` or `# File Created from ACOM RES results`, 6.5.171 sets the Image Geometry length unit to **Nanometer**; SIMPLNX always sets **Micrometer**.

**Root cause:** Algorithmic choice. Legacy `ReadAngData::readDataFile()` scanned the original header for the two TEM/ACOM marker strings and switched the unit; the SIMPLNX port hard-codes `LengthUnit::Micrometer` (`ReadAngDataFilter.cpp`, preflight geometry action). Deliberately not restored: EDAX retired those TEM/ACOM `.ang` variants more than a decade ago and no longer supports them, so no supported instrument produces such files. Source-level finding; not demonstrable with any supported fixture.

**Affected users:** Only users importing archival TEM/ACOM `.ang` files from retired EDAX software. The unit is display/metadata only — the numeric spacing values are identical.

**Recommendation:** Either acceptable. Users with archival TEM/ACOM files should manually set the geometry units to Nanometer (e.g., Set Image Geometry Units filter) after import.

---

## ReadAngDataFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ReadAngDataFilter-D3` |
| **Filter UUID** | `5b062816-79ac-47ce-93cb-e7966896bcbd` |
| **Status** | active |
| **Bug flag** | **Legacy bug — crash, empirically confirmed** |

**Symptom:** A `.ang` file whose phase sections do not start at index 1 (e.g., only a `# Phase 2` section) **crashes DREAM3D 6.5.171 with a segmentation fault** (PipelineRunner exit code 139, confirmed 2026-07-07). SIMPLNX imports the file correctly.

**Root cause:** Bug in 6.5.171. Legacy `ReadAngData::loadMaterialInfo()` sizes the ensemble arrays to `phases.size() + 1` but writes each phase at `phase->getPhaseIndex()`; when indices are non-contiguous the write is out of bounds (a 2-tuple array written at index 2). SIMPLNX (fixed during this V&V pass) sizes the ensemble arrays to `maxPhaseIndex + 1` in preflight, initializes every slot to the "Invalid Phase" defaults (`CrystalStructures = 999`, `MaterialName = "Invalid Phase"`, zero lattice constants), overwrites the slots that have phase sections, and returns error `-19502` if a phase index falls outside the arrays.

**Affected users:** Anyone importing a `.ang` file with non-contiguous phase indices into 6.5.171 (crash, potential silent memory corruption in earlier writes). Standard EDAX exports number phases contiguously from 1, so typical files are unaffected.

**Recommendation:** Trust SIMPLNX. The 6.5.171 behavior is undefined (out-of-bounds write). Pinned by the `Non-Contiguous Phase Index` unit test in `test/ReadAngDataTest.cpp`.

---

## ReadAngDataFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ReadAngDataFilter-D4` |
| **Filter UUID** | `5b062816-79ac-47ce-93cb-e7966896bcbd` |
| **Status** | active |

**Symptom:** Error codes and messages for rejected files differ: HexGrid files fail with `-1000` in 6.5.171 vs `-19500` in SIMPLNX; a missing `GRID` header key fails with `-300` (from EbsdLib at execute) in 6.5.171 vs `-19501` at preflight in SIMPLNX; missing-file and wrong-extension errors (`-388` / `-997` in legacy `dataCheck()`) are handled by the `FileSystemPathParameter` in SIMPLNX with framework codes.

**Root cause:** Algorithmic choice (framework error-handling differences). No data output is affected — these are rejection paths.

**Affected users:** Only scripts that match on specific error codes/messages from pipeline logs.

**Recommendation:** Either acceptable. Update any error-code matching to the SIMPLNX codes (`-19500` HexGrid, `-19501` missing GRID, `-19502` phase index < 1, `-19503` reader/geometry element-count mismatch, `-19504` phase index above the ensemble count).

---

## ReadAngDataFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `ReadAngDataFilter-D5` |
| **Filter UUID** | `5b062816-79ac-47ce-93cb-e7966896bcbd` |
| **Status** | active |

**Symptom:** A `.ang` file containing a `# Phase 0` section is accepted by DREAM3D 6.5.171 but rejected by SIMPLNX at execute with error `-19502`.

**Root cause:** Algorithmic choice. Legacy `ReadAngData` skipped only *negative* phase indices; a Phase 0 section was written into ensemble slot 0 (the slot SIMPLNX reserves for the "Invalid Phase" defaults) and the import succeeded. TSL `.ang` phase numbering starts at 1, so SIMPLNX treats a phase index `< 1` as malformed and rejects it. (Legacy also had no dedicated code path for it — it happened to not crash.) This is enforced in `loadMaterialInfo` and pinned by the static `Phase 0 rejected (-19502)` unit test, which trips the guard deterministically without any file-mutation injection.

**Affected users:** Only users with nonstandard `.ang` files that declare a Phase 0 section — rare, and such files carry no meaningful phase-0 crystallography in legacy output anyway (slot 0 collided with the invalid-phase slot).

**Recommendation:** Trust SIMPLNX. Renumber the file's phases to start at 1. A Phase 0 section is outside the TSL `.ang` specification.
