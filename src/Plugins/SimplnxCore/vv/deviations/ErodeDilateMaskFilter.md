# Deviations from DREAM.3D v6.5.171: ErodeDilateMaskFilter

This file has two parts. Part 1 records the historical validation difference from DREAM.3D v6.5.171 (D1). Part 2 records the fixed SIMPLNX direction-flag defect (D2). Both fixes are in develop.

Entries use stable IDs (`ErodeDilateMaskFilter-D<N>`) from one sequence. The Filter UUID is the permanent cross-reference anchor. D1 keeps its original ID and retirement date; that date records the V&V decision, not a released fix.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### ErodeDilateMaskFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ErodeDilateMaskFilter-D1` |
| **Filter UUID** | `cab66cd1-f64c-42b4-8f94-18f0835a967f` |
| **Status** | retired 2026-08-19 |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | PR #1717 (`ba246bc7c`), the DREAM3D-NX release after v7.4.2. |

**Symptom:** SIMPLNX accepted a zero or negative iteration count and returned the input mask unchanged; DREAM.3D v6.5.171 rejected the same setting.

**Root cause:** Bug. The port omitted the positive-iteration check from `ErodeDilateMask::dataCheck()`. The algorithm loop did not run when `NumIterations < 1`. Current `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ErodeDilateMaskFilter.cpp:94-107` rejects these values with `-14701` and includes the value and mask path in the message. DREAM.3D v6.5.171 source, tag `v6.5.171`, `Source/Plugins/Processing/ProcessingFilters/ErodeDilateMask.cpp:137-142`, uses `-5555` for the same condition.

**Affected users:** Users whose scripts or saved pipelines supply an iteration count below 1. Earlier SIMPLNX execution silently skipped morphology.

**Recommendation:** Trust current SIMPLNX. Supply at least one iteration; the filter now reports invalid counts before execution. The error code differs from DREAM.3D v6.5.171, but both reject the input.

**Evidence:** `SimplnxCore::ErodeDilateMaskFilter: Invalid Number of Iterations` checks invalid preflight and error `-14701` for 0 and -2. The requester chose the legacy validation behavior on 2026-08-19; no parameter key changed, and `parametersVersion()` remains 1. The zero-iteration comparison is recorded in `vv_work:ErodeDilateMaskFilter/ReadMe.md`, with inputs in `vv_work:ErodeDilateMaskFilter/extra/legacy_numiter0.json` and `vv_work:ErodeDilateMaskFilter/extra/nx_numiter0.d3dpipeline`. The test passes in CI at `01809c236`. All release branches from `upstream/release/v7.0.0` through `upstream/release/v7.4.2` omit the guard in `ErodeDilateMaskFilter.cpp`.

---

## Part 2 — SIMPLNX defects and behavior changes

### ErodeDilateMaskFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ErodeDilateMaskFilter-D2` |
| **Filter UUID** | `cab66cd1-f64c-42b4-8f94-18f0835a967f` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.4.1 through v7.4.2. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | PR #1568 (`4ec21e93f`), the DREAM3D-NX release after v7.4.2; PR #1717 (`ba246bc7c`) moves the gating into the shared helper. |

**Symptom:** Disabling X, Y or Z still allowed growth or erosion along that axis.

**Root cause:** Bug. PR #1523 (`9bfcb01e4`, 2026-02-05) replaced the explicit face checks with neighbour utilities but omitted direction gating. Its parent reads all three flags; the changed algorithm reads none. This was a refactor regression, not an omission in the original port. PR #1568 restores explicit per-axis gates. Current `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ErodeDilateMask.cpp:156` calls `adjustValidNeighbors`; `src/simplnx/Utilities/NeighborUtilities.hpp:329-337` applies each flag to the two matching faces. The DREAM.3D v6.5.171 source gates all six neighbours on the corresponding axis flags.

**Affected users:** Users who disable an axis with valid face neighbours in DREAM3D-NX releases that contain the regression. All-directions-on examples cannot detect this defect.

**Recommendation:** Trust current SIMPLNX. The algorithm now honors each selected axis. The release after v7.4.2 contains the correction.

**Evidence:** `Class 1 dilate honours direction flags` checks X-off, `Class 1 dilate with Y direction off` checks Y-off, and `Class 1 erode honours direction flags` checks Z-off. The current one-dimensional test also checks disabled-axis dilation and erosion on X and Z. These tests are in `src/Plugins/SimplnxCore/test/ErodeDilateMaskTest.cpp`.

**Release-source evidence:** `upstream/release/v7.0.0` through `upstream/release/v7.4.0` gate all six faces in `Filters/Algorithms/ErodeDilateMask.cpp`. `upstream/release/v7.4.1` and `upstream/release/v7.4.2` do not read the direction flags in that algorithm. DREAM.3D tag `v6.5.171`, `Source/Plugins/Processing/ProcessingFilters/ErodeDilateMask.cpp:227-250`, gates all six faces. Both fixes are in develop and will be in the DREAM3D-NX release after v7.4.2.

**Comparison evidence:** Run against DREAM.3D v6.5.171 on 2026-08-18: 28 of 28 mask cases match after the direction fix. See `vv_work:ErodeDilateMaskFilter/ReadMe.md` and `vv_work:ErodeDilateMaskFilter/results_compare.txt`. The direction-flag regression tests pass in CI at `01809c236`.
