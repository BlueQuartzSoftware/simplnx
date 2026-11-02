# Deviations from DREAM.3D v6.5.171: ComputeAvgOrientationsFilter

Part 1 records the Rodrigues differences from `FindAvgOrientations` and the retired D1 candidate. Part 2 records SIMPLNX defects found and fixed during V&V. Both parts use one stable ID sequence; the Filter UUID is the permanent reference.

Source references use `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeAvgOrientations.cpp` and the DREAM.3D `v6.5.171` file `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindAvgOrientations.cpp`. The provenance sidecar records release-source checks and the 2026-09-17 comparison. That comparison predates the current algorithm.

## Part 1 — Differences from DREAM.3D v6.5.171

### ComputeAvgOrientationsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeAvgOrientationsFilter-D1` |
| **Filter UUID** | `086ddb9a-928f-46ab-bad6-b1498270d71e` |
| **Status** | retired 2026-06-30 |

**Symptom:** No quaternion sign difference was observed on the compared populated features.

**Root cause:** Algorithmic choice. SIMPLNX applies `getPositiveOrientation()` after normalization. DREAM.3D v6.5.171 normalizes without that explicit call.

**Affected users:** No affected user case was found in these fixtures.

**Recommendation:** Either acceptable within tolerance `1e-6` on the compared features. Quaternion signs must also be interpreted with the equivalence of `q` and `-q`.

**Evidence:** The 2026-09-17 comparison reports zero sign flips on all 415 real features across four fixtures. This result applies to those fixtures; it does not prove that every possible input gives the same quaternion representative. See `vv_work:ComputeAvgOrientationsFilter/comparison/comparison_summary.md`.

---

### ComputeAvgOrientationsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeAvgOrientationsFilter-D2` |
| **Filter UUID** | `086ddb9a-928f-46ab-bad6-b1498270d71e` |
| **Status** | active |

**Symptom:** Positive-phase cells with FeatureId zero contribute to the SIMPLNX average. DREAM.3D v6.5.171 skips them and leaves the feature-zero quaternion at `(0,0,0,0)`.

**Root cause:** Algorithmic choice. The legacy accumulation condition requires both FeatureId and phase to be positive. SIMPLNX removes the FeatureId condition so an unlabeled set of orientations can use feature zero. The current Rodrigues implementation retains that rule.

**Affected users:** Users who assign valid positive phases to cells labeled FeatureId zero.

**Recommendation:** Trust SIMPLNX when feature zero represents an orientation set to average. Set the cell phase to zero when those cells must be excluded.

**Evidence:** PR #1577 introduced feature-zero averaging. On 2026-09-17, `case_B_forcing_d2_d3` gives feature zero identity and Rz90 input cells. SIMPLNX outputs Rz45, `(0,0,0.382683,0.923880)`; stock DREAM.3D v6.5.171 outputs the zero quaternion. A local proof version fixes D3 but retains the feature-zero exclusion, so it outputs identity. See `vv_work:ComputeAvgOrientationsFilter/comparison/comparison_summary.md` and `vv_work:ComputeAvgOrientationsFilter/comparison/case_B_forcing_d2_d3_per_feature.csv`.

---

### ComputeAvgOrientationsFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeAvgOrientationsFilter-D3` |
| **Filter UUID** | `086ddb9a-928f-46ab-bad6-b1498270d71e` |
| **Status** | active |

**Symptom:** Empty features give invalid quaternions in DREAM.3D v6.5.171 and affected DREAM3D-NX releases. SIMPLNX now writes identity quaternions and zero Euler angles.

**Root cause:** Bug. The legacy finalize loop starts at feature 1, leaving tuple zero as `(0,0,0,0)`. For a later zero-count feature it sets identity, then divides by zero. DREAM3D-NX release sources through v7.4.1 also omit tuple zero and divide empty features by zero. SIMPLNX starts finalization at zero and adds `continue` after writing identity. The current implementation uses that rule and initializes the Euler buffer to zero.

**Affected users:** Users of DREAM.3D v6.5.171 or DREAM3D-NX v7.0.0 through v7.4.1 with empty feature tuples, including background tuple zero and gaps after feature removal.

**Recommendation:** Trust SIMPLNX. DREAM3D-NX v7.4.2 contains the fix. Identity is a defined placeholder; a zero quaternion or NaN tuple cannot represent an orientation.

**Evidence:** PR #1577 (`41c079dd2`) corrected empty-feature finalization. The 2026-09-17 comparison shows the zero quaternion at legacy tuple zero in all four cases. Empty nonzero tuples in cases B and C1 contain NaNs in both legacy arrays. A local proof version writes identity and zero Euler angles for these tuples and matches SIMPLNX. `Rodrigues Analytical Oracle` independently checks the corrected values. See `vv_work:ComputeAvgOrientationsFilter/comparison/comparison_summary.md`.

---

### ComputeAvgOrientationsFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeAvgOrientationsFilter-D4` |
| **Filter UUID** | `086ddb9a-928f-46ab-bad6-b1498270d71e` |
| **Status** | active |

**Symptom:** Quaternion and Euler components can differ in their final float32 digits.

**Root cause:** Library and precision. DREAM.3D v6.5.171 uses `QuaternionMathF` and `OrientationTransforms::qu2eu`. SIMPLNX uses EbsdLib quaternion arithmetic and `QuaternionFType::toEuler()`. Their intermediate operations differ.

**Affected users:** Users who require bitwise equality across applications.

**Recommendation:** Either acceptable within absolute component tolerance `1e-6` for the compared fixtures. The measurements do not define a universal error bound.

**Evidence:** At SIMPLNX commit `8b80721c2`, the 2026-09-17 Small IN100 comparison has 408 populated real features. Against stock DREAM.3D v6.5.171, maximum differences are `8.94e-8` for quaternions and `4.77e-7` radians for Euler angles. There are 86 Euler components in the `[1e-7,1e-6)` band, and no quaternion sign flips. See `vv_work:ComputeAvgOrientationsFilter/comparison/comparison_summary.md`.

---

## Part 2 — SIMPLNX defects and behavior changes

### ComputeAvgOrientationsFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeAvgOrientationsFilter-D5` |
| **Filter UUID** | `086ddb9a-928f-46ab-bad6-b1498270d71e` |
| **Status** | active |
| **Affected released versions** | None. DREAM3D-NX v7.0.0 through v7.4.2 were not affected. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** A phase-zero cell could change a vMF or Watson average and prevent the single-sample shortcut.

**Root cause:** Bug. The directional counting pass excluded phase-zero cells, but its gather loop selected every cell with the same FeatureId. The filter now applies the positive, in-range phase condition to both passes. The current resident and Scanline implementations retain that condition. DREAM.3D v6.5.171 has no directional-sampling path.

**Affected users:** Users of development builds before DREAM3D-NX v7.4.2 with phase-zero cells inside a populated feature. No released version contained this defect.

**Recommendation:** Trust SIMPLNX. Phase-zero cells no longer contribute to the directional average.

**Evidence:** PR #1577 introduced the sampling mismatch; PR #1664 (`bb0cd6545`) corrected it. `vMF/Watson Ignores Phase-0 Voxels` checks single-sample and multiple-sample cases.

---

### ComputeAvgOrientationsFilter-D6

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeAvgOrientationsFilter-D6` |
| **Filter UUID** | `086ddb9a-928f-46ab-bad6-b1498270d71e` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** An unsupported crystal-structure value could cause an out-of-bounds orientation-operator access.

**Root cause:** Bug. The released Rodrigues code uses the crystal-structure value as an unchecked vector index. DREAM.3D v6.5.171 does the same. The filter now checks this index in Rodrigues and the directional worker. It reports skipped input with warning `-54671`. Rodrigues excludes affected cells; directional methods skip a feature whose selected phase has an unsupported structure.

**Affected users:** Users with invalid or unknown crystal-structure values on participating phases.

**Recommendation:** Trust SIMPLNX. The filter excludes unsupported crystal structures and reports a warning.

**Evidence:** PR #1645 (`4f9bbc378`) added the operator-index check; PR #1664 added warning `-54671`. `Unknown Crystal Structure and Out-Of-Range Phase Guards` checks the warning and output values.

---

### ComputeAvgOrientationsFilter-D7

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeAvgOrientationsFilter-D7` |
| **Filter UUID** | `086ddb9a-928f-46ab-bad6-b1498270d71e` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** A positive phase outside the Crystal Structures array could cause an out-of-bounds read.

**Root cause:** Bug. The earlier code checked only that the phase was positive before indexing Crystal Structures. The filter now checks the upper bound in Rodrigues and directional counting and gathering. The current code excludes these cells and returns warning `-54672`. DREAM.3D v6.5.171 also uses an unchecked positive phase index.

**Affected users:** Users whose cell phases do not identify an existing ensemble tuple.

**Recommendation:** Trust SIMPLNX. Correct the phase values or supply the missing ensemble tuples.

**Evidence:** PR #1664 (`bb0cd6545`) added the phase upper-bound check and warning `-54672`. `Unknown Crystal Structure and Out-Of-Range Phase Guards` checks the warning and output values.

---

### ComputeAvgOrientationsFilter-D8

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeAvgOrientationsFilter-D8` |
| **Filter UUID** | `086ddb9a-928f-46ab-bad6-b1498270d71e` |
| **Status** | active |
| **Affected released versions** | None. DREAM3D-NX v7.0.0 through v7.4.2 were not affected. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** The directional worker could access shared array stores concurrently without a thread-safety guarantee.

**Root cause:** Bug. The directional implementation scheduled `VmfWatsonSamplingImpl` through `ParallelDataAlgorithm` without disabling parallel execution. The worker reads shared input stores and writes shared output stores. The current resident path calls `setParallelizationEnabled(false)`. DREAM.3D v6.5.171 has no such worker.

**Affected users:** Users of development builds before DREAM3D-NX v7.4.2 with parallel execution enabled. No released version contained this defect.

**Recommendation:** Trust SIMPLNX. Serial worker execution removes the concurrent store access.

**Evidence:** PR #1577 introduced the parallel worker; PR #1645 (`4f9bbc378`) disabled parallel execution. The fixtures check numerical outputs; they do not inject a race.

---

### ComputeAvgOrientationsFilter-D9

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeAvgOrientationsFilter-D9` |
| **Filter UUID** | `086ddb9a-928f-46ab-bad6-b1498270d71e` |
| **Status** | active |
| **Affected released versions** | None. DREAM3D-NX v7.0.0 through v7.4.2 were not affected. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Cancellation did not stop the vMF/Watson worker between features.

**Root cause:** Bug. The directional worker had no cancellation check in its feature loop. The current worker checks `m_Filter->getCancel()` before each feature. DREAM.3D v6.5.171 has no directional-sampling path.

**Affected users:** Users who cancelled directional averaging in development builds before DREAM3D-NX v7.4.2. No released version contained this defect.

**Recommendation:** Trust SIMPLNX. The worker stops before the next feature; this change does not interrupt an EbsdLib estimation already in progress.

**Evidence:** PR #1577 introduced the worker without this check; PR #1645 (`4f9bbc378`) added cancellation checks between features. The current test file does not directly inject cancellation in the resident directional worker.
