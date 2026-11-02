# Deviations from DREAM.3D v6.5.171: ConvertOrientationsFilter

Part 1 lists the differences from DREAM.3D v6.5.171. D1 is retired; D2 through D5 are active. The shared ID sequence is `ConvertOrientationsFilter-D<N>`, and the Filter UUID is the permanent cross-reference anchor.

The filter dispatch is a rewrite. The shared representations retain their conversion roles; D2 through D5 define the input, capability, validation, and Euler-normalization differences. The test source checks all 56 cross-type pairs against EbsdLib reference values and independently checks the stereographic formula.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### ConvertOrientationsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ConvertOrientationsFilter-D1` |
| **Filter UUID** | `501e54e6-a66f-4eeb-ae37-00e649c00d4b` |
| **Status** | retired 2026-09-17 |

**Symptom:** The claimed maximum difference of `1.78e-6` did not compare against DREAM.3D v6.5.171. The legacy output came from a local proof version.

**Root cause:** Library. The comparison used the wrong legacy application. The official DREAM.3D v6.5.171 comparison on 2026-09-17 gives identical components for all six shared Euler-input conversions on the original 45°/30°/60° tuple and on 1,288 interior tuples. Both DREAM3D-NX v7.4.1 and v7.4.2 match. These results do not establish a library precision difference.

**Affected users:** None for the claimed precision difference on these fixtures. The Euler-boundary defect is documented separately in D5.

**Recommendation:** Either application gives identical results on the tested interior fixtures. Do not use the withdrawn `1.78e-6` value as a tolerance bound for other orientations or representation pairs.

**Evidence:** `vv_work:ConvertOrientationsFilter/ReadMe.md` identifies the official `PipelineRunner 1.2.828` and withdraws the earlier result. `vv_work:ConvertOrientationsFilter/results/proof_matrix.json` and `vv_work:ConvertOrientationsFilter/results/proof_matrix.md` record zero differing components for all six interior conversions. The algorithm later changed in `4ec21e93f` (2026-09-20, PR #1568) and `a33c370ca` (2026-09-25, PR #1697); the saved run compares the named releases, not the current algorithm.

---

### ConvertOrientationsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ConvertOrientationsFilter-D2` |
| **Filter UUID** | `501e54e6-a66f-4eeb-ae37-00e649c00d4b` |
| **Status** | active |

**Symptom:** SIMPLNX rejects float64 orientation arrays that DREAM.3D v6.5.171 accepts.

**Root cause:** Algorithmic choice. DREAM.3D selects `generateRepresentation<float>` or `generateRepresentation<double>` from the input type. SIMPLNX restricts `ArraySelectionParameter` to `DataType::float32` and converts float32 tuples.

**Affected users:** Users whose input orientation arrays use float64.

**Recommendation:** Trust SIMPLNX for float32 input. Convert float64 input to float32 only when that precision meets the application requirements; DREAM.3D v6.5.171 retains float64 conversion support.

**Evidence:** The DREAM.3D v6.5.171 source `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/ConvertOrientations.cpp` contains both conversion types. `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ConvertOrientationsFilter.cpp` accepts only float32. Every inspected DREAM3D-NX release from v7.0.0 through v7.4.2 has that restriction.

---

### ConvertOrientationsFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ConvertOrientationsFilter-D3` |
| **Filter UUID** | `501e54e6-a66f-4eeb-ae37-00e649c00d4b` |
| **Status** | active |

**Symptom:** SIMPLNX offers Stereographic as input and output type 7; DREAM.3D v6.5.171 does not.

**Root cause:** Algorithmic choice. DREAM.3D creates seven converter classes. SIMPLNX dispatches eight representations, including Stereographic.

**Affected users:** Users who select Stereographic. DREAM.3D v6.5.171 has no equivalent conversion path.

**Recommendation:** Trust SIMPLNX. `Stereographic closed form (Class 1)` checks Quaternion-to-Stereographic values from `(qx,qy,qz)/(1+qw)`; the dispatch matrix checks the other Stereographic pairs against EbsdLib reference values.

**Evidence:** The seven legacy converters are in `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/ConvertOrientations.cpp`. The eight current dispatch groups are in `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ConvertOrientations.cpp`; the checks are in `src/Plugins/OrientationAnalysis/test/ConvertOrientationsTest.cpp`.

---

### ConvertOrientationsFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ConvertOrientationsFilter-D4` |
| **Filter UUID** | `501e54e6-a66f-4eeb-ae37-00e649c00d4b` |
| **Status** | active |

**Symptom:** Invalid inputs return different error codes in SIMPLNX and DREAM.3D v6.5.171.

**Root cause:** Algorithmic choice. SIMPLNX returns `-67005` for equal representation types and `-67004` for an incorrect component count. DREAM.3D uses `-1000` and `-1006`. SIMPLNX delegates out-of-range type indices to `ChoicesParameter`; DREAM.3D uses `-1001` and `-1002` for input and output indices. SIMPLNX also rejects multidimensional component shapes with `-67003`.

**Affected users:** Scripts and tests that match numeric error codes from DREAM.3D v6.5.171.

**Recommendation:** Trust SIMPLNX. Update numeric error checks to the SIMPLNX codes; both applications reject equal types, out-of-range indices, and incorrect component counts.

**Evidence:** Compare DREAM.3D v6.5.171 `ConvertOrientations::dataCheck()` with current `ConvertOrientationsFilter::preflightImpl()`. `Invalid preflight` checks the index and component-count errors; `Equal Representations` checks all eight equal-type choices.

---

### ConvertOrientationsFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `ConvertOrientationsFilter-D5` |
| **Filter UUID** | `501e54e6-a66f-4eeb-ae37-00e649c00d4b` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.0 changed the converted orientation through Euler normalization. DREAM.3D v6.5.171 was affected and also changed the stored input. |
| **Fixed in** | DREAM3D-NX v7.4.1 |

**Symptom:** Euler normalization can change the represented rotation for negative angles, out-of-range `Phi`, or float32 `Phi = pi`. DREAM.3D v6.5.171 also overwrites the stored Euler tuple.

**Root cause:** Bug. DREAM.3D calls `EulerConverter::sanityCheckInputData()` before conversion. It applies modulo `2*pi`, `pi`, and `2*pi`, then changes negative values to positive values. These operations do not preserve every rotation. The float32 value of `pi` exceeds the double modulo constant, so the middle angle can wrap to nearly zero. DREAM3D-NX v7.0.0 through v7.4.0 apply the same normalization to a local tuple, without changing the stored array. DREAM3D-NX v7.4.1 and later omit this normalization and convert the supplied tuple.

**Affected users:** Users with these Euler inputs in the affected releases. Changed rotations can alter downstream orientation analysis. On the comparison matrix, DREAM.3D v6.5.171 changed 143 of 1,288 stored tuples at the float32 `Phi = pi` boundary.

**Recommendation:** Trust SIMPLNX. Use DREAM3D-NX v7.4.1 or later, which preserves the input and omits the lossy normalization. Recalculate outputs produced from affected Euler tuples in earlier releases.

**Evidence:** The DREAM.3D v6.5.171 source `Source/OrientationLib/OrientationMath/OrientationConverter.hpp` applies `EulerSanityCheck` to the stored input. Source on every `upstream/release/v7.0.0` through `upstream/release/v7.4.0` calls `m_CheckFunc(input.data())` after copying a tuple in `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ConvertOrientationsFilter.cpp`. The v7.4.1 and v7.4.2 algorithm sources call `inputInstance.to##TO_REP()` without that check. The 2026-09-17 artifacts `vv_work:ConvertOrientationsFilter/results/proof_matrix.json`, `vv_work:ConvertOrientationsFilter/results/proof_matrix.md`, and `vv_work:ConvertOrientationsFilter/LegacyPatch.md` isolate the legacy normalization with a local proof version. For `(-0.75, 4.0, 7.0)`, removing the normalization preserves the input and brings all six outputs within `1.2e-7` of DREAM3D-NX v7.4.2. On the boundary matrix, corrected orientation matrices agree within `5.96e-8` and quaternions are identical; raw Rodrigues and Cubochoric representations still differ at the 180-degree singularity. The current filter tests do not include negative-angle or `Phi = pi` regression fixtures.
