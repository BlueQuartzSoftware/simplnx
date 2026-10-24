# Deviations from DREAM.3D v6.5.171: ErodeDilateMaskFilter

This file has two parts. Part 1 records the validation difference from DREAM.3D v6.5.171 (D1). Part 2 records the fixed SIMPLNX direction-flag defect (D2). Both fixes will be in the DREAM3D-NX release after v7.4.2.

Entries use stable IDs (`ErodeDilateMaskFilter-D<N>`) from one sequence. The Filter UUID is the permanent cross-reference anchor. D1 keeps its original ID and retirement date; that date records the V&V decision, not a released fix.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### ErodeDilateMaskFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ErodeDilateMaskFilter-D1` |
| **Filter UUID** | `cab66cd1-f64c-42b4-8f94-18f0835a967f` |
| **Status** | retired 2026-08-19 |

**Symptom:** SIMPLNX accepted a zero or negative iteration count and returned the input mask unchanged; DREAM.3D v6.5.171 rejected the same setting.

**Root cause:** Bug. The port omitted the positive-iteration check from `ErodeDilateMask::dataCheck()`. The algorithm loop did not run when `NumIterations < 1`. Current `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ErodeDilateMaskFilter.cpp:94-107` rejects these values with `-14701` and includes the value and mask path in the message. DREAM.3D v6.5.171 source, `Source/Plugins/Processing/ProcessingFilters/ErodeDilateMask.cpp:137-142`, uses `-5555` for the same condition.

**Affected users:** Users of DREAM3D-NX v7.0.0 through v7.4.2 whose scripts or saved pipelines supply an iteration count below 1. These releases silently skipped morphology. DREAM.3D v6.5.171 was not affected.

**Recommendation:** Trust current SIMPLNX. Supply at least one iteration; the filter now reports invalid counts before execution. The error code differs from DREAM.3D v6.5.171, but both reject the input. The fix will be in the DREAM3D-NX release after v7.4.2.

**Evidence:** PR #1717 (`ba246bc7c`) adds the positive-iteration check. `SimplnxCore::ErodeDilateMaskFilter: Invalid Number of Iterations` checks invalid preflight and error `-14701` for 0 and -2. The requester chose the legacy validation behavior on 2026-08-19; no parameter key changed, and `parametersVersion()` remains 1. The zero-iteration comparison is recorded in `vv_work:ErodeDilateMaskFilter/ReadMe.md`, with inputs in `vv_work:ErodeDilateMaskFilter/extra/legacy_numiter0.json` and `vv_work:ErodeDilateMaskFilter/extra/nx_numiter0.d3dpipeline`. The test passes in CI at `01809c236`. All release branches from `upstream/release/v7.0.0` through `upstream/release/v7.4.2` omit the guard in `ErodeDilateMaskFilter.cpp`.

---

## Part 2 — SIMPLNX defects and behavior changes

### ErodeDilateMaskFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ErodeDilateMaskFilter-D2` |
| **Filter UUID** | `cab66cd1-f64c-42b4-8f94-18f0835a967f` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.4.1 through v7.4.2. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** Disabling X, Y or Z still allowed growth or erosion along that axis.

**Root cause:** Bug. A change from explicit face checks to neighbour utilities omitted the direction checks. The filter now applies each direction flag through `adjustValidNeighbors` in `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ErodeDilateMask.cpp`. The helper in `src/simplnx/Utilities/NeighborUtilities.hpp` applies each flag to the two matching faces. DREAM.3D v6.5.171 also checks the corresponding axis flag for each of the six neighbours.

**Affected users:** Users who disable an axis with valid face neighbours in DREAM3D-NX v7.4.1 through v7.4.2. All-directions-on examples cannot detect this defect.

**Recommendation:** Trust current SIMPLNX. The algorithm now honors each selected axis. The DREAM3D-NX release after v7.4.2 will contain the correction.

**Evidence:** PR #1523 (`9bfcb01e4`) removed direction checks; PR #1568 (`4ec21e93f`) restored them, and PR #1717 (`ba246bc7c`) moved them into the shared helper. In `src/Plugins/SimplnxCore/test/ErodeDilateMaskTest.cpp`, the Class 1 direction tests check X-off, Y-off and Z-off; the one-dimensional test checks disabled-axis dilation and erosion on X and Z. These tests pass in CI at `01809c236`. The algorithm in `upstream/release/v7.0.0` through `upstream/release/v7.4.0` checks all six faces; `upstream/release/v7.4.1` and `upstream/release/v7.4.2` do not read the direction flags. DREAM.3D v6.5.171 checks all six faces in `Source/Plugins/Processing/ProcessingFilters/ErodeDilateMask.cpp:227-250`. The comparison recorded on 2026-08-18 has 28 of 28 matches; see `vv_work:ErodeDilateMaskFilter/ReadMe.md` and `vv_work:ErodeDilateMaskFilter/results_compare.txt`. That run predates the change to rolling slice buffers and does not verify the current algorithm.
