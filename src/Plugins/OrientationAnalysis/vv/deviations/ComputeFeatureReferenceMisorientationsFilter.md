# Deviations from DREAM.3D v6.5.171: ComputeFeatureReferenceMisorientationsFilter

Part 1 lists differences from DREAM.3D v6.5.171 (D1, D2 and D7). Part 2 lists SIMPLNX defects (D3 through D6). Both parts use one stable ID sequence; the Filter UUID is the permanent cross-reference anchor.

The source and comparison evidence is in `../provenance/ComputeFeatureReferenceMisorientationsFilter.md`. The recorded comparison predates the current block-read algorithm.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### ComputeFeatureReferenceMisorientationsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureReferenceMisorientationsFilter-D1` |
| **Filter UUID** | `24b54daf-3bf5-4331-93f6-03a49f719bf1` |
| **Status** | active |

**Symptom:** Cell reference angles and feature means differ from DREAM.3D v6.5.171 because orientation precision and accumulation order differ.

**Root cause:** Precision and order of operations. DREAM.3D v6.5.171 uses `QuatF` and `getMisoQuat`; SIMPLNX uses `QuatD` and `calculateMisorientation`. Both accumulate float32 feature sums. DREAM.3D visits column, row, then plane; SIMPLNX visits increasing linear indices. Floating-point addition depends on this order. An arithmetic mean does not amplify a uniform per-cell error by the cell count.

**Affected users:** Users who compare floating-point angles or feature means across applications, including cubic-phase EBSD data.

**Recommendation:** Trust SIMPLNX. The analytical 5° fixtures support its angle values; compare feature means with a tolerance that accounts for float32 accumulation.

**Evidence:** The comparison uses SIMPLNX `8b80721c2` and predates the block-read algorithm in `4ec21e93f`. The 2026-09-17 run gives maximum Small IN100 Mode 0 differences of 0.07380° per cell and 0.01761° per feature between SIMPLNX and DREAM.3D v6.5.171. Fixture B differs by 3.052e-5° and fixture C by 1.864e-4°. A local proof version reduces the Small IN100 per-cell difference to 1.907e-6°; the feature difference is 1.669e-5°. Recomputing each feature sum in its application's traversal order reproduces its stored mean exactly. See `vv_work:ComputeFeatureReferenceMisorientationsFilter/comparison/results.md` and `vv_work:ComputeFeatureReferenceMisorientationsFilter/algorithm_review/source_comparison.md`.

---

### ComputeFeatureReferenceMisorientationsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureReferenceMisorientationsFilter-D2` |
| **Filter UUID** | `24b54daf-3bf5-4331-93f6-03a49f719bf1` |
| **Status** | active |

**Symptom:** Direct Mode 1 can fail before calculating misorientations because it accesses the average-quaternion input that only Mode 0 requires.

**Root cause:** Bug. DREAM.3D v6.5.171 initializes `m_AvgQuatsPtr` only in Mode 0, then dereferences it for `totalFeatures` in either mode. Released SIMPLNX also required the AvgQuats array unconditionally. SIMPLNX replaces that required lookup with an optional pointer and obtains the feature count from the selected feature matrix when AvgQuats is absent.

**Affected users:** Users of DREAM.3D v6.5.171 or DREAM3D-NX v7.0.0 through v7.4.0 who select the farthest-boundary orientation without supplying the Mode 0 average-quaternion input.

**Recommendation:** Trust SIMPLNX. DREAM3D-NX v7.4.1 contains the fix. Mode 1 can use the selected feature matrix independently of AvgQuats.

**Evidence:** PR #1504 (`f7fec9819`) fixes the required AvgQuats lookup in SIMPLNX. On 2026-09-17, all four direct Mode 1 cases crashed with exit 139 in DREAM.3D v6.5.171. A local proof version obtains the count from the always-created feature-average output; all four cases complete. Its three analytical cases match SIMPLNX exactly. See `vv_work:ComputeFeatureReferenceMisorientationsFilter/ReadMe.md`, `vv_work:ComputeFeatureReferenceMisorientationsFilter/comparison/run_exit_codes.txt` and `vv_work:ComputeFeatureReferenceMisorientationsFilter/comparison/results.md`. Current unit fixtures create AvgQuats in both modes and do not isolate its absence.

---

### ComputeFeatureReferenceMisorientationsFilter-D7

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureReferenceMisorientationsFilter-D7` |
| **Filter UUID** | `24b54daf-3bf5-4331-93f6-03a49f719bf1` |
| **Status** | active |

**Symptom:** SIMPLNX writes feature reference-cell coordinates in Mode 1; DREAM.3D v6.5.171 has no equivalent output array.

**Root cause:** Algorithmic choice. SIMPLNX adds the Euclidean Centers output. The algorithm converts the selected linear cell index to the physical cell-center position. This extra output does not change the reference quaternion.

**Affected users:** Users who need to locate or display the reference cell for each feature.

**Recommendation:** Trust SIMPLNX. The analytical Mode 1 fixtures check literal 2D and 3D coordinates.

**Evidence:** PR #1504 adds the Euclidean Centers output. The DREAM.3D v6.5.171 source creates only the two misorientation outputs. The 2026-09-17 results also record the absent legacy coordinate array in `vv_work:ComputeFeatureReferenceMisorientationsFilter/comparison/results.md`.

---

## Part 2 — SIMPLNX defects and behavior changes

### ComputeFeatureReferenceMisorientationsFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureReferenceMisorientationsFilter-D3` |
| **Filter UUID** | `24b54daf-3bf5-4331-93f6-03a49f719bf1` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.0. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.1 |

**Symptom:** Mode 1 can calculate wrong angles or access outside the feature-average quaternion array.

**Root cause:** Bug. The old SIMPLNX code uses `avgQuats[centerGNum * 4 + ...]`, although `centerGNum` is a cell index. SIMPLNX now reads the selected cell quaternion. The current algorithm caches that quaternion in `centerQuats`. DREAM.3D v6.5.171 uses `quats[m_Centers[gnum]]` for this operation; its separate Mode 1 crash is D2.

**Affected users:** Users who select the farthest-boundary reference in the affected releases.

**Recommendation:** Trust SIMPLNX. The reference quaternion must come from the selected cell.

**Evidence:** PR #1504 (`f7fec9819`) fixes the reference quaternion lookup. `Class 1 - Mode 1 KnownCenter`, `Class 1 - Mode 1 MultiGrain CenterIsolation` and `Class 1 - Mode 1 3D Volume` check cell angles and feature means from literal expectations.

---

### ComputeFeatureReferenceMisorientationsFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureReferenceMisorientationsFilter-D4` |
| **Filter UUID** | `24b54daf-3bf5-4331-93f6-03a49f719bf1` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** An imported SIMPL Mode 1 pipeline selects the cell matrix where it requires the feature matrix.

**Root cause:** Bug. `FromSIMPLJson()` derived `k_CellFeatureAttributeMatrixPath_Key` from `FeatureIdsArrayPath`. The conversion now derives it from `AvgQuatsArrayPath`, which identifies the feature matrix. DREAM.3D v6.5.171 does not use the SIMPLNX conversion path.

**Affected users:** Users who import legacy pipelines with distinct cell and feature matrices and select Mode 1.

**Recommendation:** Trust SIMPLNX. The conversion now uses the feature-level path.

**Evidence:** PR #1682 (`5eb69f359`) fixes the feature-matrix source path. The current conversion test checks arguments, but both fixtures put AvgQuats and FeatureIds under the same parent. It does not distinguish the corrected source path from the old one.

---

### ComputeFeatureReferenceMisorientationsFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureReferenceMisorientationsFilter-D5` |
| **Filter UUID** | `24b54daf-3bf5-4331-93f6-03a49f719bf1` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** A positive participating phase outside the Crystal Structures array can cause an out-of-bounds read.

**Root cause:** Bug. Released SIMPLNX reads `crystalStructures[cellPhases[voxelIdx]]` without a phase-range check. DREAM.3D v6.5.171 also performs an unchecked lookup. The current algorithm checks the positive phase before reading the ensemble cache and returns `-34901` on failure.

**Affected users:** Users whose positive phase values do not identify a Crystal Structures tuple on a participating cell.

**Recommendation:** Trust SIMPLNX. Correct the phase or supply the corresponding ensemble tuple before execution.

**Evidence:** PR #1568 (`4ec21e93f`) adds the phase-range check. `Phase and Laue Index Bounds` checks phase 2 against a two-tuple ensemble in both modes. It also checks that an invalid phase on feature zero is ignored.

---

### ComputeFeatureReferenceMisorientationsFilter-D6

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureReferenceMisorientationsFilter-D6` |
| **Filter UUID** | `24b54daf-3bf5-4331-93f6-03a49f719bf1` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** A participating crystal-structure value outside the Laue-operator range can cause an invalid operator access.

**Root cause:** Bug. Released SIMPLNX passes the Crystal Structures value directly to the operator vector. DREAM.3D v6.5.171 does the same. The current algorithm checks the value against `orientationOps.size()` and returns `-34902` before the lookup.

**Affected users:** Users whose participating phase has an invalid Crystal Structures value.

**Recommendation:** Trust SIMPLNX. Supply a valid Laue index for each participating phase.

**Evidence:** PR #1568 (`4ec21e93f`) adds the Laue-index check. `Phase and Laue Index Bounds` checks an invalid Laue value in both reference modes.
