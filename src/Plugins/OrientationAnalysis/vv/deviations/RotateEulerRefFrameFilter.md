# Deviations from DREAM.3D v6.5.171: RotateEulerRefFrameFilter

Part 1 lists the invalid-axis handling difference. Part 2 lists the SIMPLNX store-access defect. Both parts use the stable `RotateEulerRefFrameFilter-D<N>` sequence.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### RotateEulerRefFrameFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `RotateEulerRefFrameFilter-D1` |
| **Filter UUID** | `0458edcd-3655-4465-adc8-b036d76138b5` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** A zero rotation axis passed validation although it has no defined direction. SIMPLNX now rejects it before changing the Euler array.

**Root cause:** Bug. The filter did not validate the axis magnitude. DREAM3D-NX v7.4.1 divided the zero vector by its magnitude and could write NaN values. DREAM.3D v6.5.171 also accepted the invalid axis, but its float `MatrixMath::Normalize3x1()` returned without division when the magnitude was zero. That helper leaves the zero components unchanged. Current `RotateEulerRefFrameFilter.cpp` rejects magnitudes below float32 epsilon with `-96200`; `Algorithms/RotateEulerRefFrame.cpp` repeats the check with `-67050` for direct calls.

**Affected users:** Users whose pipelines supply a zero or very small rotation axis. The current guard also rejects nonzero magnitudes below float32 epsilon.

**Recommendation:** Trust SIMPLNX. Supply a nonzero axis of sufficient magnitude; the filter normalizes it before rotation.

**Evidence:** PR #1654 (`11a7acdae`) added both checks. Source at every `upstream/release/v7.0.0` through `upstream/release/v7.4.1` omits them; `upstream/release/v7.4.2` contains them. DREAM.3D tag `v6.5.171`, `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/RotateEulerRefFrame.cpp`, omits axis validation. SIMPL `0afaa6854`, selected at or before that tag date, has the zero-denominator return in `Source/SIMPLib/Math/MatrixMath.cpp`. `Zero-Length Axis Fails Preflight` asserts rejection without checking the error code. The earlier informational label `RotateEulerRefFrameFilter-N3` refers to this same input-validation difference.

---

## Part 2 — SIMPLNX defects and behavior changes

### RotateEulerRefFrameFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `RotateEulerRefFrameFilter-D2` |
| **Filter UUID** | `0458edcd-3655-4465-adc8-b036d76138b5` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Parallel Euler rotation could access a nonresident store from multiple workers without the required residency check.

**Root cause:** Bug. `ParallelDataAlgorithm` dispatched workers that read and wrote `Float32Array::operator[]` without first requiring the Euler array to be resident. DREAM3D-NX v7.4.2 adds `requireArraysInMemory()` before dispatch. The current algorithm uses a serial read-modify-write loop with buffers of at most 65,536 tuples. DREAM.3D v6.5.171 uses a resident raw float array and does not have the nonresident store path.

**Affected users:** Users who supply nonresident Euler arrays to the affected SIMPLNX implementation with parallel execution enabled.

**Recommendation:** Trust SIMPLNX. DREAM3D-NX v7.4.2 applies the residency safeguard; the current buffered implementation avoids concurrent store access.

**Evidence:** PR #1654 (`11a7acdae`) records the unsafe worker access and adds `requireArraysInMemory()`. All inspected release sources from `upstream/release/v7.0.0` through `upstream/release/v7.4.1` lack that call; `upstream/release/v7.4.2` contains it. PR #1568 (`4ec21e93f`) replaces the workers with serial buffered traversal. The tagged DREAM.3D source passes a raw `float*` into disjoint worker ranges. Current tests check rotation values; they do not reproduce the earlier concurrent-access failure.
