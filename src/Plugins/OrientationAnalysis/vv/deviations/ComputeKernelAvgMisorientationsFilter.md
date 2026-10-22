# Deviations from DREAM.3D v6.5.171: ComputeKernelAvgMisorientationsFilter

Part 1 lists differences from DREAM.3D v6.5.171 (D1–D3). Part 2 lists fixed input-safety defects (D4–D5). The stable IDs share one sequence, and the Filter UUID is the permanent cross-reference anchor.

Source, release and comparison evidence is recorded in `../provenance/ComputeKernelAvgMisorientationsFilter.md`.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### ComputeKernelAvgMisorientationsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeKernelAvgMisorientationsFilter-D1` |
| **Filter UUID** | `61cfc9c1-aa0e-452b-b9ef-d3b9e6268035` |
| **Status** | active |

**Symptom:** Cubic kernel averages can differ from DREAM.3D v6.5.171, especially when admitted orientations are identical or nearly identical.

**Root cause:** Precision. The earlier CubicOps calculation recovers the angle from `acos(w)` near one. Rounding in symmetry reduction can produce a nonzero angle for an identity rotation. The corrected EbsdLib calculation uses explicit reduced-quaternion vector components with `2*atan2(|v|, w)`. SIMPLNX also uses double-precision quaternions where DREAM.3D uses float precision. Both self-pairs and nearly identical distinct pairs can contribute to the difference.

**Affected users:** Users who calculate KAM on cubic-phase data in DREAM3D-NX v7.0.0 through v7.4.1 or DREAM.3D v6.5.171, or compare their results with DREAM.3D v6.5.171.

**Recommendation:** Trust SIMPLNX. DREAM3D-NX v7.4.2 contains the stable cubic-angle calculation. Do not treat the difference as a constant offset or apply a universal 0.01° bound.

**Evidence:** The six analytical fixtures derive angles without using filter output. The source and dependency checks in the provenance sidecar identify the affected releases. The 2026-09-17 comparison, recorded in `vv_work:ComputeKernelAvgMisorientationsFilter/comparison/summary.md`, measures a maximum difference of 0.055953° and mean absolute difference of 0.0015421° on Small IN100 with the symmetric kernel. These results describe the implementation used for that run.

---

### ComputeKernelAvgMisorientationsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeKernelAvgMisorientationsFilter-D2` |
| **Filter UUID** | `61cfc9c1-aa0e-452b-b9ef-d3b9e6268035` |
| **Status** | active |

**Symptom:** DREAM.3D v6.5.171 uses the wrong positive X extent when the X and Z kernel radii differ.

**Root cause:** Bug. `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindKernelAvgMisorientations.cpp` uses `l < m_KernelSize.z + 1` for the X loop. For radius `{1,1,2}`, it visits X offsets `-1,0,1,2`: four columns instead of three. For `{2,1,1}`, it visits `-2,-1,0,1`: four columns instead of five. SIMPLNX uses the X radius for both X bounds. Equal X and Z radii hide this defect.

**Affected users:** Users of DREAM.3D v6.5.171 who select different X and Z radii. DREAM3D-NX v7.0.0 through v7.4.2 were not affected.

**Recommendation:** Trust SIMPLNX. DREAM3D-NX v7.0.0 contains the fix. Its kernel has the requested extent on each axis.

**Evidence:** The DREAM.3D v6.5.171 source has the typo; every inspected DREAM3D-NX release uses the correct X radius. `vv_work:ComputeKernelAvgMisorientationsFilter/comparison/legacy_comparison_report.md` records the 2026-09-17 proof for both asymmetric cases on synthetic data and Small IN100. A local proof version corrects the bound and cubic precision; its maximum difference from the tested SIMPLNX implementation is below 0.000001°.

---

### ComputeKernelAvgMisorientationsFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeKernelAvgMisorientationsFilter-D3` |
| **Filter UUID** | `61cfc9c1-aa0e-452b-b9ef-d3b9e6268035` |
| **Status** | active |

**Symptom:** With `use_feature_ids=false`, SIMPLNX averages across feature boundaries within the same phase. DREAM.3D v6.5.171 has only same-feature admission.

**Root cause:** Algorithmic choice. The default value `true` retains the same-feature rule. With `false`, a neighbor contributes when its feature ID is positive and its phase equals the focal phase. In both modes, the focal cell must have a positive feature ID and phase. DREAM3D-NX v7.4.2 adds this optional mode; DREAM3D-NX v7.0.0 through v7.4.1 provide only the default rule.

**Affected users:** Users who disable Use Feature Ids to calculate KAM across grain boundaries.

**Recommendation:** Use the default mode for same-feature KAM. Disable Use Feature Ids when the intended neighborhood crosses feature boundaries but stays within one phase.

**Evidence:** `Class 1 - Per-Voxel Mode (use_feature_ids = false)` requires the diagnostic value `20/3°` where same-feature mode gives `5°`. `Class 1 - Per-Voxel Mode Two-Phase Gates` checks phase and background exclusion. `Class 4 - Mode Equivalence on Single Feature` compares all 27 values when both modes admit the same neighbors.

---

## Part 2 — SIMPLNX defects and behavior changes

### ComputeKernelAvgMisorientationsFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeKernelAvgMisorientationsFilter-D4` |
| **Filter UUID** | `61cfc9c1-aa0e-452b-b9ef-d3b9e6268035` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** A positive phase or invalid crystal-structure value on a participating cell can cause an out-of-bounds read.

**Root cause:** Bug. Earlier execution reads Crystal Structures through the phase index and then reads the orientation-operator list through the Laue index without range checks. `ValidateKernelAvgMisorientationsPhaseIndices()` now rejects an invalid phase with `-67203` and an invalid Laue index with `-67204`. Both executors call this validator. Cells with nonpositive feature IDs or phases do not participate in validation.

**Affected users:** Users whose participating cell phases do not identify ensemble entries, or whose ensemble entries do not identify a supported Laue operator.

**Recommendation:** Trust SIMPLNX. Correct the reported phase or crystal-structure value before calculating KAM. The fix will be in the DREAM3D-NX release after v7.4.2.

**Evidence:** `Phase and Laue Index Bounds` checks both errors and accepts an invalid phase on background feature zero. All inspected release sources and DREAM.3D v6.5.171 have unchecked lookups; the current common validator performs the checks before calculation.

---

### ComputeKernelAvgMisorientationsFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeKernelAvgMisorientationsFilter-D5` |
| **Filter UUID** | `61cfc9c1-aa0e-452b-b9ef-d3b9e6268035` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** A kernel radius of `INT32_MAX` causes signed overflow when earlier implementations form the inclusive loop bound.

**Root cause:** Bug. Earlier loops evaluate a signed radius plus one, such as `kernelSize[0] + 1`. That expression overflows at `INT32_MAX`. Current Direct and Scanline calculations clamp unsigned neighbor coordinates to the image first. The loops stop when each coordinate reaches its clamped maximum.

**Affected users:** Users who supply a maximum signed 32-bit radius through a pipeline or API.

**Recommendation:** Trust SIMPLNX. The fix will be in the DREAM3D-NX release after v7.4.2. In earlier releases, restrict each radius to the extent needed for the image.

**Evidence:** `Scanline Handles Maximum Kernel Radius` exercises Direct and Scanline scenarios with all three radii at `INT32_MAX` and requires one zero output value. `Working Set Input Validation` also accepts those radii. All inspected release sources and DREAM.3D v6.5.171 form signed radius-plus-one bounds.
