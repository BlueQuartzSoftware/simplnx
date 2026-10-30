# Deviations from DREAM.3D v6.5.171: ReadCtfDataFilter

Part 1 lists differences from DREAM.3D v6.5.171. Part 2 lists SIMPLNX defects whose fixes restore the legacy behavior. All entries use one stable ID sequence.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### ReadCtfDataFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ReadCtfDataFilter-D1` |
| **Filter UUID** | `7751923c-afb9-4032-8372-8078325c69a4` |
| **Status** | active |

**Symptom:** Unindexed points have phase 0 in SIMPLNX and phase 1 in DREAM.3D v6.5.171.

**Root cause:** Algorithmic choice. DREAM.3D changes every phase value below 1 to 1 before storing the phase array. SIMPLNX preserves phase zero because it identifies an unindexed point.

**Affected users:** Users who import CTF files with unindexed points and select or mask data by phase.

**Recommendation:** Trust SIMPLNX. Preserve phase zero to distinguish unindexed points from measured phase 1; use an explicit replacement if a workflow requires the old convention.

**Evidence:** The DREAM.3D `v6.5.171` source at `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/ReadCtfData.cpp` contains the remap. PR #937 removes it from SIMPLNX. `Class 1 Analytical Oracle` asserts phases `{1, 2, 0, 1, 2, 0}`. The stock comparison on 2026-07-24 records 116,050 changed phase values in the 220,000-point Cugrid scan: `vv_work:ReadCtfDataFilter/results/compare_ab_2026-07-24.txt`.

---

### ReadCtfDataFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ReadCtfDataFilter-D2` |
| **Filter UUID** | `7751923c-afb9-4032-8372-8078325c69a4` |
| **Status** | active |

**Symptom:** With EDAX alignment enabled and a 6/mmm phase 1, DREAM.3D v6.5.171 adds 30 degrees to unindexed points' phi2; SIMPLNX leaves those values unchanged.

**Root cause:** Algorithmic choice. D1 makes DREAM.3D use phase 1's crystal structure for unindexed points. SIMPLNX uses the unknown-structure slot at phase zero, which does not receive a hexagonal correction.

**Affected users:** Users who retain unindexed Euler angles with EDAX alignment enabled and a 6/mmm phase 1.

**Recommendation:** Trust SIMPLNX. A point without an indexed phase must not inherit the orientation correction for a measured phase.

**Evidence:** `Euler Conversion Combinations` asserts unchanged unindexed values in all four modes. The 2026-07-24 stock comparison records 116,050 differing phi2 values in the Cugrid scan, each approximately 0.5235988 radians apart; the other 543,950 Euler components match exactly. See `vv_work:ReadCtfDataFilter/results/compare_ab_2026-07-24.txt`.

---

### ReadCtfDataFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ReadCtfDataFilter-D3` |
| **Filter UUID** | `7751923c-afb9-4032-8372-8078325c69a4` |
| **Status** | active |

**Symptom:** Malformed CTF input can crash DREAM.3D v6.5.171 or produce incorrect output; SIMPLNX rejects the input with an error.

**Root cause:** Bug. The import paths lacked checks for missing column buffers, empty phase definitions, phase bounds, and inconsistent dimensions. SIMPLNX now checks these conditions before copying or indexing data. It also rejects phase counts that exceed the preflight allocation after a file change. Errors `-19600` through `-19605` identify these conditions; the current reader rejects the zero-Z fixture earlier with `-101105`.

**Affected users:** Users of DREAM3D-NX v7.0.0 through v7.4.1 who import malformed files or change a file during import. DREAM.3D v6.5.171 was affected by the missing-column, empty-phase, and out-of-range-phase defects.

**Recommendation:** Trust SIMPLNX. DREAM3D-NX v7.4.2 contains the guards; correct the reported input error before importing again.

**Evidence:** All release sources from `upstream/release/v7.0.0` through `upstream/release/v7.4.1` lack the guards; v7.4.2 contains them. The tagged legacy source performs unchecked copies and phase lookups. `vv_work:ReadCtfDataFilter/ReadMe.md` records two stock crashes and an out-of-range phase accepted on 2026-07-24. `vv_work:ReadCtfDataFilter/LegacyPatch.md` records a local proof version that rejects all three inputs on 2026-09-17 and preserves 60 datasets across four well-formed cases. Current error tests assert `-19600`, `-19601`, `-19602`, and `-19604`; `-19603` and `-19605` require file-change injection.

---

### ReadCtfDataFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `ReadCtfDataFilter-D5` |
| **Filter UUID** | `7751923c-afb9-4032-8372-8078325c69a4` |
| **Status** | active |

**Symptom:** EDAX alignment omitted the 30-degree phi2 correction for 6/m phases.

**Root cause:** Bug. The condition selected only `Hexagonal_High` (6/mmm). The Oxford/HKL-to-EDAX axis correction applies to the hexagonal lattice in both 6/m and 6/mmm. The current condition accepts both classes and applies the correction before converting to radians.

**Affected users:** Users of DREAM3D-NX v7.0.0 through v7.4.2 who import 6/m data with EDAX alignment enabled. DREAM.3D v6.5.171 was affected.

**Recommendation:** Trust SIMPLNX. The fix will be in the DREAM3D-NX release after v7.4.2; re-import 6/m data with that correction when EDAX alignment is required.

**Evidence:** All ten release sources and the tagged legacy source select only `Hexagonal_High`. PR #1781 (`84c66066a`) introduces `IsHexagonalCrystalStructure()` in `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/utilities/EbsdReaderUtilities.hpp`. `Euler Conversion Combinations` checks both hexagonal classes, cubic points, and phase-zero points in all four modes.

---

## Part 2 — SIMPLNX defects and behavior changes

### ReadCtfDataFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ReadCtfDataFilter-D4` |
| **Filter UUID** | `7751923c-afb9-4032-8372-8078325c69a4` |
| **Status** | retired 2026-07-24 |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** A multi-slice CTF file produced a single-slice geometry without a warning.

**Root cause:** Bug. Preflight used a fixed Z extent and spacing of 1. It ignored the header's ZCells and ZStep, so the copy loop read only the first slice. The current filter uses ZCells and positive ZStep, with spacing 1 when ZStep is not positive.

**Affected users:** Users who imported multi-slice CTF files in DREAM3D-NX v7.0.0 through v7.4.1.

**Recommendation:** Trust SIMPLNX. Re-import these files with DREAM3D-NX v7.4.2 or later to recover all slices.

**Evidence:** Every release source through `upstream/release/v7.4.1` fixes Z to 1; v7.4.2 and the tagged legacy source read the header's Z extent. `vv_work:ReadCtfDataFilter/results/release_matrix.md` records the 2026-09-17 comparison: v7.4.1 gives 2×2×1, while v7.4.2 gives 2×2×2 with Z spacing 0.75. All 24 Euler values match DREAM.3D v6.5.171; two phase values differ as D1 specifies. `3D Multi-Slice CTF` checks both slice-spacing variants.

---

### ReadCtfDataFilter-D6

| Field | Value |
|---|---|
| **Deviation ID** | `ReadCtfDataFilter-D6` |
| **Filter UUID** | `7751923c-afb9-4032-8372-8078325c69a4` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Some converted Euler values differed from the double-intermediate result after rounding to float32.

**Root cause:** Bug. The port used a float32 radians constant and float32 arithmetic. The current filter uses double-precision intermediates for the hexagonal shift and radian conversion, then stores float32 values. DREAM.3D v6.5.171 also used double-precision constants.

**Affected users:** Users who compare converted Euler values exactly or rely on the final float32 rounding.

**Recommendation:** Trust SIMPLNX. DREAM3D-NX v7.4.2 restores the more accurate conversion; re-import data when exact converted values matter.

**Evidence:** All release sources through v7.4.1 use `k_PiOver180F`; v7.4.2 uses `k_PiOver180D` and explicit float64 intermediates. The tagged legacy filter uses `SIMPLib::Constants::k_PiOver180`, declared as double in `Source/SIMPLib/Math/SIMPLibMath.h` at the SIMPL snapshot `0afaa6854` selected by the legacy tag date. `Euler Conversion Combinations` checks 7.125, 3.375, 6.75, and shifted 37.125 degrees with literals that distinguish the two conversion methods.

---

### ReadCtfDataFilter-D7

| Field | Value |
|---|---|
| **Deviation ID** | `ReadCtfDataFilter-D7` |
| **Filter UUID** | `7751923c-afb9-4032-8372-8078325c69a4` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** A reader failure could produce a filter error with code zero instead of its negative error code.

**Root cause:** Bug. The filter read only the reader's error-code member, which some failures leave at zero. The current preflight and execution paths use the negative return code when that member is not negative. DREAM.3D v6.5.171 used the return code directly.

**Affected users:** Users who import files with a zero scan step or other reader failures that set only a return code and message.

**Recommendation:** Trust SIMPLNX. DREAM3D-NX v7.4.2 reports the failure code needed to diagnose these inputs.

**Evidence:** All release sources through v7.4.1 call `MakeErrorResult(reader.getErrorCode(), ...)`; v7.4.2 adds the fallback in both read paths. The tagged legacy `readDataFile()` uses `setErrorCondition(err)`. `EbsdLib Error Passthrough - Zero Step (-102)` checks the execution fallback. The header-read fallback has no direct test.
