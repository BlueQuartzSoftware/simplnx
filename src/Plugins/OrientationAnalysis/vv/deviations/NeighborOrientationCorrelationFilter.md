# Deviations from DREAM.3D v6.5.171: NeighborOrientationCorrelationFilter

Part 1 lists differences from DREAM.3D v6.5.171 (D1–D4 and D8). Part 2 lists SIMPLNX test and implementation defects (D5–D7). All entries use one stable ID sequence and the filter UUID as the permanent reference.

## Part 1 — Differences from DREAM.3D v6.5.171

### NeighborOrientationCorrelationFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `NeighborOrientationCorrelationFilter-D1` |
| **Filter UUID** | `4625c192-7e46-4333-a294-67a2eb64cb37` |
| **Status** | active |

**Symptom:** A low-confidence cell can receive a different-phase tuple because an unrelated neighbor pair counted as similar.

**Root cause:** bug. DREAM.3D v6.5.171 stores the misorientation in a function-scope variable. The same-phase condition updates that variable, but the similarity test runs outside the condition. A different-phase or phase-zero pair can reuse an earlier angle. SIMPLNX counts similarities only inside the same-positive-phase condition.

**Affected users:** Users of DREAM.3D v6.5.171 with mixed-phase or unindexed neighbors. DREAM.3D v6.5.171 was affected. DREAM3D-NX v7.0.0 through v7.4.2 were not affected.

**Recommendation:** Trust SIMPLNX. Only a pair with the same positive phase contributes to the similarity count; the selected pair need not have the center cell's phase.

**Evidence:** `Oracle F05 - mixed-phase pair never similar (D1 regression)` checks the mixed-phase fixture. The tagged legacy source has the stale-angle path; all ten release sources initialize the pair angle before testing it. See `vv_work:NeighborOrientationCorrelationFilter/notes/ComparisonResults.md` and the source audit in `../provenance/neighbor_orientation_correlation_v2.md`.

---

### NeighborOrientationCorrelationFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `NeighborOrientationCorrelationFilter-D2` |
| **Filter UUID** | `4625c192-7e46-4333-a294-67a2eb64cb37` |
| **Status** | active |

**Symptom:** DREAM.3D and earlier DREAM3D-NX releases stop cleanup before the intended number of passes.

**Root cause:** bug. The loop decrements the level both in its header and at the end of its body. It runs `ceil((6 - Level)/2)` passes. SIMPLNX removes the second decrement and runs `6 - Level` passes. Level controls the pass count, not a required number of dissimilar neighbors.

**Affected users:** Users who set Level below 5 and need repeated propagation. DREAM3D-NX v7.0.0 through v7.4.1 were affected. DREAM.3D v6.5.171 was affected. DREAM3D-NX v7.4.2 contains the fix.

**Recommendation:** Trust SIMPLNX. The corrected schedule gives four passes at Level 2 and two at Level 4.

**Evidence:** `Oracle F04 - pass schedule Level 2 (D2 regression)` and `Oracle F10 - pass schedule Level 4 (D2 regression)` check the resulting fill. Direct inspection of all release sources finds the second decrement through v7.4.1 and no second decrement in v7.4.2. PR #1655 (`71526a1f5`) contains the correction.

---

### NeighborOrientationCorrelationFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `NeighborOrientationCorrelationFilter-D3` |
| **Filter UUID** | `4625c192-7e46-4333-a294-67a2eb64cb37` |
| **Status** | active |

**Symptom:** A later neighbor with a smaller positive similarity count can replace a neighbor with a larger count.

**Root cause:** bug. DREAM.3D resets `best` inside the selection loop. Any positive count therefore replaces the previous selection. SIMPLNX retains `best` across the loop and selects the greatest positive count. Equal maxima select the last neighbor in −Z, −Y, −X, +X, +Y, +Z order.

**Affected users:** Users with unequal neighbor similarity counts. DREAM3D-NX v7.0.0 through v7.4.1 were affected. DREAM.3D v6.5.171 was affected. DREAM3D-NX v7.4.2 contains the fix.

**Recommendation:** Trust SIMPLNX. The selected neighbor has the greatest similarity count; fully tied neighborhoods retain the legacy choice.

**Evidence:** `Oracle F03 - argmax selection (D3 regression)` and `Oracle F12 - anisotropic dims` distinguish the rules. All release sources through v7.4.1 reset `best` per neighbor. The v7.4.2 source contains the maximum-count rule from PR #1655 (`71526a1f5`).

---

### NeighborOrientationCorrelationFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `NeighborOrientationCorrelationFilter-D4` |
| **Filter UUID** | `4625c192-7e46-4333-a294-67a2eb64cb37` |
| **Status** | active |

**Symptom:** A pair close to the angle tolerance can receive a different similarity decision because the orientation calculations use different precision.

**Root cause:** precision. DREAM.3D v6.5.171 uses `QuatF` and `getMisoQuat`. SIMPLNX converts the stored float32 quaternion components to `ebsdlib::QuatD` and calls `calculateMisorientation`. The angle result is double precision; conversion of the tolerance to radians still uses float32.

**Affected users:** Users whose neighbor-pair angles are close to the selected tolerance. This is a numerical difference, not a confirmed defect.

**Recommendation:** Trust SIMPLNX. Avoid requiring identical classification across implementations for a pair at the tolerance boundary.

**Evidence:** The legacy and current algorithm sources establish the arithmetic types. The 2026-07-07 comparison found no cell difference between that SIMPLNX version and a local proof version on Small IN100. This measurement does not bound other inputs or the current slice-transfer implementation; see `vv_work:NeighborOrientationCorrelationFilter/notes/ComparisonResults.md`.

---

### NeighborOrientationCorrelationFilter-D8

| Field | Value |
|---|---|
| **Deviation ID** | `NeighborOrientationCorrelationFilter-D8` |
| **Filter UUID** | `4625c192-7e46-4333-a294-67a2eb64cb37` |
| **Status** | active |

**Symptom:** Replacement chains can copy different tuples from DREAM.3D when source cells are also replaced.

**Root cause:** order of operations. DREAM.3D copies tuples in ascending index order and retains the source map across passes. Current SIMPLNX copies numeric tuples from the store state at the start of each slice transfer, then clears that slice's source marks. NeighborList and String arrays still use ascending in-place copies. DREAM3D-NX v7.0.0 through v7.4.2 retain the earlier transfer method; the changed method will be in the DREAM3D-NX release after v7.4.2.

**Affected users:** Users with adjacent low-confidence cells whose selected sources also change. Numeric tuples do not chain within one slice transfer. The selection window and the tuple-transfer buffers serve different purposes.

**Recommendation:** Use the current slice-transfer rules when interpreting replacement chains. The isolated-cell analytical fixtures check source selection but do not establish whole-volume parity with DREAM.3D.

**Evidence:** PR #1568 (`4ec21e93f`) replaces the volume source map with `sliceBestNeighbor`, adds `SliceBufferedTransferOneZ`, and clears marks after each slice. `src/simplnx/Utilities/SliceBufferedTransfer.hpp::SliceTransferOneZFunctor` reads source slices before writing the destination. The 2026-07-07 comparison predates this change.

---

## Part 2 — SIMPLNX defects and behavior changes

### NeighborOrientationCorrelationFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `NeighborOrientationCorrelationFilter-D5` |
| **Filter UUID** | `4625c192-7e46-4333-a294-67a2eb64cb37` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1 test sources. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** The production test can pass without comparing any filter output with its exemplar.

**Root cause:** bug. The test substitutes `Exemplar Data` for the container name, but the retired output file uses `DataContainer`. Missing arrays trigger `continue`. The corrected test snapshots the input cell arrays and checks transfer invariants; analytical fixtures independently check exact tuples.

**Affected users:** Developers relying on the earlier exemplar test as correctness evidence. This is a test defect, not a separate output transformation.

**Recommendation:** Trust the current analytical and invariant tests. Do not use the retired exemplar comparison as evidence of correctness.

**Evidence:** All ten release test sources were inspected. Sources through v7.4.1 use the missing-array skip; v7.4.2 uses snapshots and requires at least one changed cell. `vv_work:NeighborOrientationCorrelationFilter/notes/ComparisonResults.md` records the archive container mismatch. PR #1655 (`71526a1f5`) removes the output archive registration.

---

### NeighborOrientationCorrelationFilter-D6

| Field | Value |
|---|---|
| **Deviation ID** | `NeighborOrientationCorrelationFilter-D6` |
| **Filter UUID** | `4625c192-7e46-4333-a294-67a2eb64cb37` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** A replaced cell keeps its old NeighborList and String values while its numeric values change.

**Root cause:** bug. The earlier transfer list collects only `IDataArray` objects. NeighborList and String arrays are separate types. The corrected list collects `IArray` objects and copies each supported type. DREAM.3D collects every array in the cell AttributeMatrix.

**Affected users:** Users with NeighborList or String arrays beside the confidence array.

**Recommendation:** Trust SIMPLNX. The transfer includes these arrays, and F13 checks their source tuples and unchanged cells.

**Evidence:** PR #1655 (`71526a1f5`) adds `GenerateTransferArrayList`, the concrete copy dispatch and `Oracle F13 - NeighborList and String cell arrays transferred`. All release sources through v7.4.1 use `GenerateDataArrayList`; v7.4.2 includes all three types. The tagged DREAM.3D source loops over all cell arrays.

---

### NeighborOrientationCorrelationFilter-D7

| Field | Value |
|---|---|
| **Deviation ID** | `NeighborOrientationCorrelationFilter-D7` |
| **Filter UUID** | `4625c192-7e46-4333-a294-67a2eb64cb37` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** An invalid participating phase or crystal-structure value can cause an out-of-bounds array access.

**Root cause:** bug. Earlier sources index Crystal Structures through the cell phase and index the orientation operators through that value without range checks. SIMPLNX now checks each participating positive phase before either lookup. It returns -580096 for an invalid phase and -580097 for an invalid Laue index. Nonpositive phases do not participate.

**Affected users:** Users with a same-positive-phase neighbor pair whose phase exceeds the ensemble bounds or whose crystal structure is not a valid Laue index.

**Recommendation:** Trust SIMPLNX. Correct the phase or crystal-structure value reported in the error before running cleanup again.

**Evidence:** `Phase and Laue Index Bounds` asserts both errors and successful execution with phase-zero neighbors. All release sources through v7.4.2 and DREAM.3D tag `v6.5.171` lack these bounds checks. PR #1568 (`4ec21e93f`) adds the current guards.
