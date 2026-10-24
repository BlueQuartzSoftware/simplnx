# Deviations from DREAM.3D v6.5.171: ComputeFeatureNeighborMisorientationsFilter

Part 1 records the divisor and precision differences from DREAM.3D v6.5.171 (D1 and D2). Part 2 records the phase-index defect fixed in SIMPLNX (D3). Both parts use one stable ID sequence. The Filter UUID is the permanent cross-reference anchor.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### ComputeFeatureNeighborMisorientationsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureNeighborMisorientationsFilter-D1` |
| **Filter UUID** | `0b68fe25-b5ef-4805-ae32-20acb8d4e823` |
| **Status** | active |

**Symptom:** With averaging enabled, excluded neighbors can lower the average and make it depend on neighbor order.

**Root cause:** bug. DREAM.3D v6.5.171 `FindMisorientations::execute()` resets `tempMisoList` to the list length inside each neighbor iteration. The next iteration therefore discards the preceding decrement. For a nonempty list of length N, the final divisor is N when the last neighbor is compatible, or N−1 when it is incompatible. SIMPLNX inherited this code. DREAM3D-NX v7.4.2 initializes the divisor once per feature, before the neighbor loop, and preserves all decrements. Compatibility means equal, valid crystal-structure indices. It does not require equal phase IDs. A list with one incompatible neighbor at the end can produce the correct divisor by coincidence. Multiple incompatible neighbors can still produce a wrong divisor when the last neighbor is incompatible. An all-incompatible list with more than one neighbor can produce zero instead of NaN.

**Affected users:** Users of DREAM.3D v6.5.171 or DREAM3D-NX v7.0.0 through v7.4.1 who enable average misorientation on features with incompatible neighbors. Single-phase data with valid crystal structures does not trigger this divisor defect.

**Recommendation:** trust SIMPLNX. The corrected average divides the sum by the number of compatible neighbors.

**Evidence:** PR #1631 (`ec22ac375`) contains the divisor fix. Test sensitivity verification: `Class 1 - Mixed Phase Neighbors (exposes divisor bug)` uses neighbors `[2, 4, 3]` and expects `[5°, NaN, 10°]` with mean 7.5°. Resetting the divisor inside the loop instead gives 5°. `Class 1 - Mismatch Last Order` uses `[2, 3, 4]` as the control. `Class 4 - Invariants` compares the mean with the sum and count of finite list entries. All pass in CI. The run against DREAM.3D v6.5.171 on 2026-09-17 gives 4.9999804° for the middle-mismatch mean and 7.5° in SIMPLNX. In the mixed-phase Small IN100 case, 508 legacy feature averages differ from the correct formula applied to the legacy lists. SIMPLNX and a local proof version satisfy that formula for every feature. See `vv_work:ComputeFeatureNeighborMisorientationsFilter/comparison/case2_mixed_phase_mismatch_middle.txt`, `vv_work:ComputeFeatureNeighborMisorientationsFilter/comparison/case5_small_in100_mixed_phase.txt`, and the provenance sidecar for source changes after the comparison. Every release from `upstream/release/v7.0.0` through `upstream/release/v7.4.1` resets the divisor inside the neighbor loop. The `upstream/release/v7.4.2` source initializes it before that loop. The algorithm path is `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeMisorientations.cpp` in v7.0.0 through v7.0.3 and `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureNeighborMisorientations.cpp` in later releases. The DREAM.3D `v6.5.171` tag contains the defect in `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindMisorientations.cpp`.

---

### ComputeFeatureNeighborMisorientationsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureNeighborMisorientationsFilter-D2` |
| **Filter UUID** | `0b68fe25-b5ef-4805-ae32-20acb8d4e823` |
| **Status** | active |

**Symptom:** Neighbor angles and feature averages can differ slightly from DREAM.3D v6.5.171 even when the averaging divisor is correct.

**Root cause:** precision. DREAM.3D v6.5.171 uses `QuatF`, `getMisoQuat()`, and a float angle. SIMPLNX promotes stored float32 quaternions to `QuatD`, calls `calculateMisorientation()`, and converts the double angle to degrees before float32 storage. EbsdLib also replaces the cubic `acos` angle calculation with `2*atan2(norm(v), w)` using explicit reduced-quaternion vector components. These differences affect individual angles and the means formed from them.

**Affected users:** Users who compare floating-point neighbor angles or feature means with DREAM.3D v6.5.171. The archived measurements include cubic analytical inputs and cubic and mixed cubic/hexagonal Small IN100 inputs.

**Recommendation:** trust SIMPLNX. The analytical tests use a `1e-3°` tolerance and independent expected values. The precision difference does not change the definition of the average.

**Evidence:** EbsdLib commit `5c8c993` contains the cubic angle-calculation change. On 2026-09-17, the three analytical cases differ from DREAM.3D v6.5.171 by up to `3.05176e-5°` per neighbor. Cubic Small IN100 differs by up to `0.000337124°` per neighbor and `7.24792e-5°` per feature mean. The local proof version matches all five cases within `rtol=1e-6, atol=1e-6`, with paired NaNs; the production cases retain rounding differences. See `vv_work:ComputeFeatureNeighborMisorientationsFilter/comparison/summary.md` and `vv_work:ComputeFeatureNeighborMisorientationsFilter/notes/residual_rounding_analysis.md`.

---

## Part 2 — SIMPLNX defects and behavior changes

### ComputeFeatureNeighborMisorientationsFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureNeighborMisorientationsFilter-D3` |
| **Filter UUID** | `0b68fe25-b5ef-4805-ae32-20acb8d4e823` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** A negative or out-of-range feature phase can cause an out-of-bounds read from Crystal Structures.

**Root cause:** bug. Both the current-feature and neighbor-feature lookups used phase values directly as ensemble indices. Preflight validates array shapes and tuple counts, not the phase values. The filter now checks phase bounds before both lookups in `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureNeighborMisorientations.cpp`. Invalid phases now return error `-34502`. Background feature zero is skipped by the outer loop.

**Affected users:** Users whose participating features contain phase indices outside `[0, number of Crystal Structures tuples)`.

**Recommendation:** trust SIMPLNX. Correct the reported phase value or supply the corresponding Crystal Structures entry before execution.

**Evidence:** PR #1568 (`4ec21e93f`) contains the phase-bounds fix. `Phase Index Bounds` supplies -1 and 2 with a two-tuple ensemble array. It checks both current and neighbor errors and successful execution when only the background phase is invalid. All pass in CI. Every release source from `upstream/release/v7.0.0` through `upstream/release/v7.4.2` has both unchecked lookups, using the algorithm paths listed under D1. DREAM.3D tag `v6.5.171`, `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindMisorientations.cpp`, also has both unchecked lookups.
