# V&V Report: RotateEulerRefFrameFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `0458edcd-3655-4465-adc8-b036d76138b5` |
| SIMPLNX Human Name | Rotate Euler Reference Frame |
| DREAM.3D v6.5.171 equivalent | `RotateEulerRefFrame` |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-07-05; second engineer: Nathan Young, 2026-07-10 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Port of `RotateEulerRefFrame` in DREAM.3D v6.5.171. SIMPLNX retains the matrix composition and uses double-precision transforms with serial buffered traversal. |
| Oracle (confirmed) | Class 1 analytical expectations and Class 4 invariants are encoded in `test/RotateEulerRefFrameTest.cpp`. All pass in CI. |
| Code paths enumerated | 9 of 15 exercised. Cancellation, direct algorithm rejection, empty input and I/O failures have no direct fixtures. |
| Tests today | 6 test cases: an archive regression, SIMPL conversion, eight analytical fixtures, axis rejection, three invariant sections and a hidden storage-boundary case. |
| Exemplar archive | `ASCIIData.tar.gz` supplies 480,000 input Euler tuples and comparison values. Independent expectations are defined in the test source. |
| Legacy comparison | Not run for the current algorithm against DREAM.3D v6.5.171. The 2026-09-17 comparison used DREAM3D-NX v7.4.1 and v7.4.2, before the buffered traversal change. |
| Bug flags | None. |
| V&V phase | COMPLETE. |

## Summary

`RotateEulerRefFrameFilter` rotates the sample reference frame and replaces a three-component float32 Euler-angle array in place. Eight analytical fixtures and three invariant sections check the rotation, and an archive test checks 480,000 tuples. All pass in CI.

## Algorithm Relationship

*Classification:* Port

*Evidence:* `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/OrientationAnalysisLegacyUUIDMapping.hpp` maps the legacy UUID to this filter; both implementations compose orientation and rotation matrices, normalize columns and convert back to Euler angles.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant) companions.

*Applied:* Independent expectations use `g′ = g · R_active(n,w)`; invariants check output ranges, inverse rotation and composition of two 45° rotations.

*Encoded:* `src/Plugins/OrientationAnalysis/test/RotateEulerRefFrameTest.cpp::OrientationAnalysis::RotateEulerRefFrameFilter: Class 1 Analytical Fixtures` contains eight fixtures; `Class 4 Invariants` contains three sections over six orientations. All pass in CI.

*Second-engineer review:* Michael Jackson, 2026-07-16, as recorded in the oracle review.

## Bugs found and fixed

All defects in this table are fixed. DREAM3D-NX v7.4.2 contains both fixes.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `RotateEulerRefFrameFilter-D1` | A zero rotation axis passed validation and entered a calculation with no defined rotation direction. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | Preflight rejects axis magnitudes below float32 epsilon with `-96200`; direct algorithm calls return `-67050`. |
| `RotateEulerRefFrameFilter-D2` | Parallel workers accessed the Euler store without a residency check, permitting unsafe access to nonresident stores. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | The current algorithm reads, rotates and writes bounded buffers serially. |

## Code path coverage

9 of 15 paths exercised. Each uncovered path has a reason below.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/RotateEulerRefFrame.cpp` (99 lines) and `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/RotateEulerRefFrameFilter.cpp` (142 lines).

Test names below omit the `OrientationAnalysis::RotateEulerRefFrameFilter:` prefix, except for the archive test `OrientationAnalysis::RotateEulerRefFrame`.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Entry | Cancel before setup | *Not directly tested. Requires cancellation injection before execution.* |
| 2 | Preflight | Reject an axis magnitude below float32 epsilon | `Zero-Length Axis Fails Preflight` checks rejection of `(0,0,0)`. |
| 3 | Entry | Reject an invalid axis in a direct algorithm call with `-67050` | *Not directly tested. Existing fixtures call the filter and reject the axis in preflight.* |
| 4 | Entry | Normalize a non-unit axis | `Class 1 Analytical Fixtures` — `F5 unnormalized axis x90`. |
| 5 | Entry | Convert degrees and form the rotation matrix | `Class 1 Analytical Fixtures` — X, Y, Z and `(1,1,1)` rotations, including zero angle. |
| 6 | Rotation | Compose matrices, normalize columns and convert a general orientation | `Class 1 Analytical Fixtures` — F2, F3 and F8; `Class 4 Invariants`. |
| 7 | Rotation | Convert an orientation at the Euler singularity | `Class 1 Analytical Fixtures` — F1 has output Phi equal to zero. |
| 8 | Traversal | Process a full 65,536-tuple buffer | `OrientationAnalysis::RotateEulerRefFrame` checks all values across seven full buffers. |
| 9 | Traversal | Process a partial final buffer | `OrientationAnalysis::RotateEulerRefFrame` checks the 21,248-tuple tail; `Class 1 Analytical Fixtures` checks one-tuple inputs. |
| 10 | Traversal | Cancel before processing a buffer | *Not directly tested. Requires cancellation injection during traversal.* |
| 11 | I/O | Return a buffer-read error | *Not directly tested. No fixture injects a store read failure.* |
| 12 | I/O | Return a buffer-write error | *Not directly tested. No fixture injects a store write failure.* |
| 13 | I/O | Write rotated values back into the same array | `Class 1 Analytical Fixtures`; `OrientationAnalysis::RotateEulerRefFrame`. |
| 14 | Progress | Update progress after a successful buffer write | `OrientationAnalysis::RotateEulerRefFrame` traverses multiple buffers; message contents are not asserted. |
| 15 | Traversal | Skip the buffer loop for zero tuples | *Not directly tested. All fixtures contain at least one tuple.* |

## Test inventory

The five ordinary cases pass on Linux, macOS and Windows. All pass in CI.

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::RotateEulerRefFrame` | kept | Imports 480,000 Euler tuples, rotates 30° about `(1,1,1)` and checks all 1,440,000 values against `EulersRotated.csv` at absolute tolerance `1e-4`. This is an archive regression. |
| `OrientationAnalysis::RotateEulerRefFrameFilter: SIMPL Backwards Compatibility` | kept | `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` sections check successful loading, filter identity, empty comments and the Euler array path. |
| `OrientationAnalysis::RotateEulerRefFrameFilter: Class 1 Analytical Fixtures` | new-for-V&V | Eight dynamic sections check three components each at `1e-5` rad: identity, general orientation, wraparound, X rotation, axis normalization, zero angle, `(1,1,1)` rotation and Y rotation. |
| `OrientationAnalysis::RotateEulerRefFrameFilter: real HDF5 65536-block rotation oracle` | new-for-V&V | Hidden case. Checks six Euler values at tuples 65,535 and 65,536 against independent expectations at `1e-5` rad, and checks the store format before and after execution. |
| `OrientationAnalysis::RotateEulerRefFrameFilter: Zero-Length Axis Fails Preflight` | new-for-V&V | Requires invalid preflight for axis `(0,0,0)`; it does not assert the error code. Covers `RotateEulerRefFrameFilter-D1`. |
| `OrientationAnalysis::RotateEulerRefFrameFilter: Class 4 Invariants` | new-for-V&V | Three sections check inclusive float32 output ranges, inverse-rotation recovery and 45°+45° equivalence to 90° over six orientations; the latter two use `1e-4` tolerance. |

## Exemplar archive

- **Archive:** `ASCIIData.tar.gz`
- **SHA512:** `70388864301ca1ea7fce7b1666d3abf682eee68c7d8b7a9bf532df7aff11e7ea9de7dc2dc80e33f0e363cbad023b663bff97df4be362f0312d311e9d5bedf370`
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/RotateEulerRefFrameFilter.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/RotateEulerRefFrameFilter.md` for the root cause, affected users and recommendation. Only Part 1 entries appear here.

| Deviation | Observed difference |
|-----------|---------------------|
| `RotateEulerRefFrameFilter-D1` | SIMPLNX rejects a zero rotation axis before changing the Euler array; DREAM.3D v6.5.171 accepts it. |
