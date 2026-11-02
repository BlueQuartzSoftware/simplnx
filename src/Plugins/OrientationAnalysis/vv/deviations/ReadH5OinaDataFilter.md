# Deviations from DREAM.3D v6.5.171: ReadH5OinaDataFilter

DREAM.3D v6.5.171 has no H5OINA importer. Part 2 records SIMPLNX defects and behavior changes; there are no Part 1 entries. IDs D1 through D14 keep their existing meanings. D15 through D18 identify the additional input-validation defects.

## Part 2 — SIMPLNX defects and behavior changes

---

### ReadH5OinaDataFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D1` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Hexagonal alignment added 30 radians instead of 30 degrees to phi2.

**Root cause:** bug. The importer reused the degree-valued literal 30.0 although H5OINA Euler angles are in radians. Adds pi/6 radians with a double-precision intermediate.

**Affected users:** Imports with Hexagonal-High phases and EDAX alignment enabled.

**Recommendation:** Trust SIMPLNX. Re-import affected data and repeat downstream orientation analysis.

**Evidence:** `Class 1 Analytical Oracle`, `Conversion Option Combinations`, and `Multi-Scan Hexagonal Alignment` assert independently rounded phi2 values. Release-source and dependency evidence is in `../provenance/ReadH5OinaDataFilter.md`.

---

### ReadH5OinaDataFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D2` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Later scans overwrote earlier Euler values.

**Root cause:** bug. The Euler copy used an element count of three times the point count but passed a destination offset in tuples. Multiplies the destination tuple offset by three Euler components.

**Affected users:** Imports with two or more selected scans.

**Recommendation:** Trust SIMPLNX. Re-import multi-scan data produced by the affected releases.

**Evidence:** `Multi-Scan Slab Placement` checks all 24 Euler values of two cubic scans. Release-source and dependency evidence is in `../provenance/ReadH5OinaDataFilter.md`.

---

### ReadH5OinaDataFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D3` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Alignment repeatedly modified the first scan and skipped later scans.

**Root cause:** bug. The alignment helper did not receive the scan offset and always started at tuple zero. Applies alignment once to each point in the current scan slab.

**Affected users:** Multi-scan imports with Hexagonal-High phases and EDAX alignment enabled.

**Recommendation:** Trust SIMPLNX. Re-import affected multi-scan data.

**Evidence:** `Multi-Scan Hexagonal Alignment` checks distinct Euler values in both scan slabs. Release-source and dependency evidence is in `../provenance/ReadH5OinaDataFilter.md`.

---

### ReadH5OinaDataFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D4` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Pattern import reported missing data when the reader did not support pattern import.

**Root cause:** bug. The reader returned no pattern buffer or dimensions. Preflight interpreted the zero dimensions as absent file data. The unreachable copy also used a different element type from the output action. Rejects pattern import explicitly with error `-9583` and identifies an alternative.

**Affected users:** Users who enable Import Pattern Data.

**Recommendation:** Trust SIMPLNX. Disable Import Pattern Data; use Read HDF5 Dataset to read pattern datasets separately.

**Evidence:** `Parameter Rejections` includes `Pattern Import Not Supported (-9583)`. `vv_work:ReadH5OinaDataFilter/probe_real_file.txt` shows Processed Patterns in the vendor input. Release-source and dependency evidence is in `../provenance/ReadH5OinaDataFilter.md`.

---

### ReadH5OinaDataFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D5` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** The gamma lattice-angle slot contained beta.

**Root cause:** bug. H5OINAReader assembled the angle slots from indices 0, 1, and 1. EbsdLib 3.1.2 uses indices 0, 1, and 2. Copies the third lattice angle into the gamma slot.

**Affected users:** Imports whose phase gamma differs from beta and consumers of LatticeConstants.

**Recommendation:** Trust SIMPLNX. Re-import phase metadata from affected files.

**Evidence:** `Class 1 Analytical Oracle` expects lattice angles 90, 90, and 120 degrees for its hexagonal phase. Release-source and dependency evidence is in `../provenance/ReadH5OinaDataFilter.md`.

---

### ReadH5OinaDataFilter-D6

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D6` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** H5OINA lattice angles used radians while the shared phase model used degrees.

**Root cause:** bug. H5OINAReader copied the file angles without conversion. EbsdLib 3.1.2 converts the three angles to the same degree units used by the ANG and CTF readers. Converts lattice angles to degrees with a double-precision intermediate.

**Affected users:** All consumers of H5OINA LatticeConstants angle components.

**Recommendation:** Trust SIMPLNX. Stop any additional radians-to-degrees conversion after import. Multiply saved angle components from affected releases by 180/pi when comparing with new imports; Euler angles remain in radians.

**Evidence:** `Class 1 Analytical Oracle` and `Real AZtec File Readback` check degree-valued lattice angles. `vv_work:ReadH5OinaDataFilter/exemplar_selforacle_check.txt` records the old radian exemplar. Release-source and dependency evidence is in `../provenance/ReadH5OinaDataFilter.md`.

---

### ReadH5OinaDataFilter-D7

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D7` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Missing lattice datasets caused invalid access during preflight.

**Root cause:** bug. H5OINAReader ignored failed vector reads and indexed empty lattice vectors. EbsdLib 3.1.2 checks the phase-group open, lattice dimensions, lattice angles, and Laue group with errors `-90030` through `-90033`; Space Group remains optional. Validates required phase data before indexing it.

**Affected users:** Users opening incomplete or malformed phase metadata.

**Recommendation:** Trust SIMPLNX. Repair the file or export it again when preflight reports missing phase data.

**Evidence:** `EbsdLib Error Passthrough - Missing Lattice Angles (-9582)` checks first and later scans. `vv_work:ReadH5OinaDataFilter/red_baseline.log` records the earlier crash. Release-source and dependency evidence is in `../provenance/ReadH5OinaDataFilter.md`.

---

### ReadH5OinaDataFilter-D8

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D8` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Phase groups could exceed ensemble bounds or give different meanings to a shared phase index.

**Root cause:** bug. The ensemble arrays were sized from the first scan and then filled from each scan without checking group indices or compatible definitions. Preflight now reports `-9587`, `-9589`, or `-9590` before those writes. Validates phase indices, counts, and definitions in every selected scan.

**Affected users:** Imports with invalid phase-group indices or differing phase lists between scans.

**Recommendation:** Trust SIMPLNX. Import scans separately when they describe different phase definitions.

**Evidence:** `Phase Index Out Of Range rejected (-9587)`, `Scan Phase Count Mismatch rejected (-9589)`, and `Scan Phase Definition Mismatch rejected (-9590)` cover the guards. Release-source and dependency evidence is in `../provenance/ReadH5OinaDataFilter.md`.

---

### ReadH5OinaDataFilter-D9

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D9` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** The Stacking Order setting had no effect.

**Root cause:** bug. The scan loop always used list order and did not read the stacking-order parameter. Low To High now preserves the list; High To Low puts the last selected scan at Z = 0. Reverses the selected scan list for High To Low stacking.

**Affected users:** Multi-scan imports with High To Low selected.

**Recommendation:** Trust SIMPLNX. Check downstream slice selections when repeating an affected pipeline.

**Evidence:** `Stacking Order` asserts Euler, Band Contrast, and Phase values for both orders. Release-source and dependency evidence is in `../provenance/ReadH5OinaDataFilter.md`.

---

### ReadH5OinaDataFilter-D10

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D10` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Reader failures could have empty explanations or lose the specific inner error.

**Root cause:** bug. Ten H5OINAReader paths composed text in a stream but stored the unchanged input string. readFile also replaced detailed header and data errors. EbsdLib 3.1.2 preserves the composed text and inner context. Stores the composed error text and includes the inner failure in wrapper messages.

**Affected users:** Users who encounter H5OINA reader failures.

**Recommendation:** Trust SIMPLNX. Use the reported file, scan, and dataset context to correct the input.

**Evidence:** `EbsdLib Error Passthrough - Missing Data Column (-8970)` and `EbsdLib Error Passthrough - Missing Lattice Angles (-9582)` check error propagation and context. Release-source and dependency evidence is in `../provenance/ReadH5OinaDataFilter.md`.

---

### ReadH5OinaDataFilter-D11

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D11` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Invalid cell counts could become unsigned dimensions; zero columns returned inconsistent error codes.

**Root cause:** bug. H5OINAReader widened counts before checking them. With positive rows and zero columns, it stored `-90301` but returned `-301`; zero rows instead returned `-200`. EbsdLib 3.1.2 checks both signed counts. Filter preflight independently rejects invalid geometry counts with `-9584`. Validates both signed cell counts before conversion and returns the stored reader error code.

**Affected users:** Users opening files with zero or negative cell counts; direct H5OINAReader callers also receive the corrected error code.

**Recommendation:** Trust SIMPLNX. Supply positive X Cells and Y Cells.

**Evidence:** `Invalid Cell Counts rejected (-9584)` checks zero X, zero Y, and negative X. EbsdLib release source establishes the return-code correction. Release-source and dependency evidence is in `../provenance/ReadH5OinaDataFilter.md`.

---

### ReadH5OinaDataFilter-D12

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D12` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** A non-numeric phase-group name caused an exception after opening the HDF5 group.

**Root cause:** bug. H5OINAReader called std::stoi on the group name after opening it. An exception bypassed cleanup. EbsdLib 3.1.2 returns `-90034` for noncanonical names; the filter forwards the failure as `-9582`. Validates the complete positive integer name before opening the group.

**Affected users:** Users with malformed or manually edited phase-group names.

**Recommendation:** Trust SIMPLNX. Use canonical positive integer phase-group names or export the file again.

**Evidence:** `Invalid Phase Group Name rejected (-9582)` uses the name phase-one and checks the file and group context. Release-source and dependency evidence is in `../provenance/ReadH5OinaDataFilter.md`.

---

### ReadH5OinaDataFilter-D13

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D13` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Non-positive or non-finite spacing could create invalid geometry.

**Root cause:** bug. Preflight tested only whether Z spacing was less than or equal to zero. It accepted NaN Z and unchecked X/Y step values. It now reports `-9580` for Z and `-9591` for scan spacing. Requires finite, positive X, Y, and Z spacing.

**Affected users:** Imports with invalid scan step values or a non-finite Z Spacing parameter.

**Recommendation:** Trust SIMPLNX. Correct the spacing values before import.

**Evidence:** `Parameter Rejections` and `Invalid Scan Spacing rejected (-9591)` check zero, negative, and non-finite values. Release-source and dependency evidence is in `../provenance/ReadH5OinaDataFilter.md`.

---

### ReadH5OinaDataFilter-D14

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D14` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** EDAX alignment skipped Hexagonal-Low phases.

**Root cause:** bug. The alignment condition accepted only Hexagonal-High. The correction depends on the hexagonal lattice and also applies to Hexagonal-Low. The shared IsHexagonalCrystalStructure predicate now accepts both. Applies pi/6 to both Hexagonal-High and Hexagonal-Low phases.

**Affected users:** Imports with Hexagonal-Low phases and EDAX alignment enabled.

**Recommendation:** Trust SIMPLNX. Re-import Hexagonal-Low data when EDAX alignment is required.

**Evidence:** `Conversion Option Combinations` checks both phase types and alignment states. `Multi-Scan Hexagonal Alignment` checks both hexagonal classes in each scan. PR #1781 (`84c66066a`) supplies this correction. Release-source and dependency evidence is in `../provenance/ReadH5OinaDataFilter.md`.

---

### ReadH5OinaDataFilter-D15

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D15` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Different scan grids could cause incomplete copies or reads beyond a scan buffer.

**Root cause:** bug. Output dimensions came from the first scan, but later reader buffers could describe a different grid. Preflight now compares every later scan with the first scan. Rejects incompatible scan dimensions or step sizes with `-9585`.

**Affected users:** Users selecting scans with different dimensions or spacing.

**Recommendation:** Trust SIMPLNX. Import incompatible grids separately.

**Evidence:** `Scan Header Mismatch rejected (-9585)` checks different cell counts and different step sizes. Release-source and dependency evidence is in `../provenance/ReadH5OinaDataFilter.md`.

---

### ReadH5OinaDataFilter-D16

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D16` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** A missing later scan passed preflight and failed after earlier scans had been copied.

**Root cause:** bug. Preflight read only the first selected header. It did not check later names against the file scan list, so failure left partially populated output arrays. Checks every selected scan name before execution and reports `-9586`.

**Affected users:** Users with a missing scan name after a valid first selection.

**Recommendation:** Trust SIMPLNX. Select names that exist in the input file.

**Evidence:** `Missing Scan Name rejected (-9586)` checks missing first and second scan names. Release-source and dependency evidence is in `../provenance/ReadH5OinaDataFilter.md`.

---

### ReadH5OinaDataFilter-D17

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D17` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Dataset lengths inconsistent with the header could cause out-of-bounds reads or discarded values.

**Root cause:** bug. The reader allocated from dataset extents while the filter copied counts from the header. The filter now has an extent guard (`-34971`); the current reader rejects the short-data fixture earlier and the filter reports `-8970`. Rejects dataset extents that differ from the required point and component counts.

**Affected users:** Users with incomplete or inconsistent H5OINA Data datasets.

**Recommendation:** Trust SIMPLNX. Repair the dataset extents or export the file again.

**Evidence:** `Dataset Extent Mismatch rejected (-8970)` asserts reader code `-101100` and the Band Contrast dataset name. `vv_work:ReadH5OinaDataFilter/mutation_table.md` records the earlier filter-guard check. Release-source and dependency evidence is in `../provenance/ReadH5OinaDataFilter.md`.

---

### ReadH5OinaDataFilter-D18

| Field | Value |
|---|---|
| **Deviation ID** | `ReadH5OinaDataFilter-D18` |
| **Filter UUID** | `fad3d47f-f1e1-4429-bc65-5e021be62ba0` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** An out-of-range cell Phase value could index beyond CrystalStructures.

**Root cause:** bug. The alignment loop indexed CrystalStructures directly with an unchecked phase byte. The current copy rejects values outside zero through the declared phase count with `-34972`. Validates every cell Phase value before storing it or using it for alignment.

**Affected users:** Users with cell Phase values above the declared phase count.

**Recommendation:** Trust SIMPLNX. Correct the Phase column or the phase definitions.

**Evidence:** `Out-of-Range Phase Value rejected (-34972)` checks rejection and the file, scan, and point context. Release-source and dependency evidence is in `../provenance/ReadH5OinaDataFilter.md`.
