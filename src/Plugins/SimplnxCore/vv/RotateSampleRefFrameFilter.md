# V&V Report: RotateSampleRefFrameFilter

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `d2451dc1-a5a1-4ac2-a64d-7991669dcffc` |
| SIMPLNX Human Name | Rotate Sample Reference Frame |
| DREAM.3D v6.5.171 equivalent | `RotateSampleRefFrame` (Sampling plugin, SIMPL UUID `e25d9b4c-2b37-578c-b1de-cf7032b5ef19`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-07-02; second engineer: Nathan Young, 2026-07-08 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Rewrite of `RotateSampleRefFrame` in DREAM.3D v6.5.171. SIMPLNX uses shared affine transformation utilities and restricts rotations to proper signed axis permutations. |
| Oracle (confirmed) | Class 1 analytical permutations and geometry values, with Class 4 value conservation and composition checks. Nine cases in `test/RotateSampleRefFrameTest.cpp` encode these checks; all pass in CI. |
| Code paths enumerated | 15 of 29 exercised. Gaps include in-place output, defensive guards, external storage, cancellation and I/O failures. |
| Tests today | 14 test cases: 9 analytical or invariant cases, 2 rejection cases, 1 conversion case, 1 child-name regression and 1 hidden HDF5 case. |
| Exemplar archive | None. Inline analytical expectations replace the retired `Rotate_Sample_Ref_Frame_Test_v2.tar.gz` and `Rotate_Sample_Ref_Frame_Test_v3.tar.gz` archives. |
| Legacy comparison | Not run on the current algorithm. The saved DREAM.3D v6.5.171 comparison is dated 2026-07-02 and predates algorithm changes. |
| Bug flags | None. |
| V&V phase | Complete. Analytical checks and regressions for the two fixed defects are encoded. |

## Summary

`RotateSampleRefFrameFilter` rotates Image Geometry cell data by nearest-neighbor sampling and supports whole-volume or slice-by-slice operation. Class 1 permutations and geometry values, with Class 4 conservation and composition checks, verify the supported rotations. The V&V records two fixed defects: an incorrect output origin and unwanted changes to copied child names.

## Algorithm Relationship

*Classification:* Rewrite

*Evidence:* `src/Plugins/SimplnxCore/src/SimplnxCore/SimplnxCoreLegacyUUIDMapping.hpp` maps the legacy UUID to this filter; shared Eigen affine transforms and origin-aware cell-center sampling replace the legacy matrix kernel and truncated source indices.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant).

*Applied:* Hand-derived permutations specify voxel order, dimensions, spacing and origin; value conservation and full-circle composition check the remaining positive fixtures.

*Encoded:* `src/Plugins/SimplnxCore/test/RotateSampleRefFrameTest.cpp::SimplnxCore::RotateSampleRefFrame: Class 1 - 180 about Z reverses a slice` and eight companion cases include 18 principal-axis rotation variants and three chirality fixtures. All pass in CI.

*Second-engineer review:* Michael Jackson, 2026-07-16 (technical authority).

## Bugs found and fixed

All defects in this table are fixed. DREAM3D-NX v7.4.2 contains the fix for `RotateSampleRefFrameFilter-D2`. The fix for `RotateSampleRefFrameFilter-D3` will be in the DREAM3D-NX release after v7.4.2.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `RotateSampleRefFrameFilter-D2` | The transform-derived output origin counted the nonzero input origin twice. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | The output origin equals the absolute minimum corner of the transformed bounding box. |
| `RotateSampleRefFrameFilter-D3` | Copying geometry children replaced the geometry name inside child names and parent paths. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected. | Copy actions replace only the source geometry path prefix and preserve child names. |

## Code path coverage

15 of 29 paths exercised. Each uncovered row states the gap.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/RotateSampleRefFrameFilter.cpp` (406 lines); `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/RotateSampleRefFrame.cpp` (108 lines); `src/simplnx/Utilities/ImageRotationUtilities.hpp` (1345 lines); `src/simplnx/Utilities/ImageRotationUtilities.cpp` (288 lines).

Test names below omit the `SimplnxCore::RotateSampleRefFrame: ` prefix. Coverage refers to the mapped in-core tests and the nearest-neighbor code used by this filter.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Preflight | Accept a proper signed axis permutation | `accepts 120-degree rotation about (111)`; `Class 1/4 - principal-90 rotations are lossless permutations` |
| 2 | Preflight | Reject a matrix entry outside the allowed tolerance with `-6850` | `rejects non-principal-90 rotations` — two axis-angle cases and one matrix case |
| 3 | Preflight | Reject a matrix without exactly one signed unit entry in each row and column | *Not directly tested. No fixture supplies a degenerate matrix with only zero and signed unit entries.* |
| 4 | Preflight | Reject a reflection with determinant -1 | *Not directly tested. No fixture supplies a reflection matrix.* |
| 5 | Preflight | Reject slice-by-slice rotation that exchanges Z with another axis with `-6851` | `rejects slice-by-slice with a slice-reordering rotation` |
| 6 | Transform | Construct the matrix from axis-angle parameters | `Class 1 - exact 90-degree permutation pins chirality` — X, Y and Z |
| 7 | Transform | Read the supplied rotation matrix | `Class 1 - 180 about Z reverses a slice`; `Class 1/4 - principal-90 rotations are lossless permutations` — matrix variants |
| 8 | Sampling | Inverse-map cell centers for a whole-volume rotation | `Class 1 - exact 90-degree permutation pins chirality`; `Class 4 - full-circle composition is identity` |
| 9 | Sampling | Keep source Z equal to output Z in slice-by-slice mode | `slice-by-slice 180 about Y is a lossless per-slice flip` — value conservation and different results from whole-volume mode |
| 10 | Geometry | Keep the input origin | `Class 1 - non-zero input origin` — keep-origin section |
| 11 | Geometry | Use the transformed bounding-box origin | `Class 1 - non-zero input origin` — expects (-22,10,0) and exact voxel values |
| 12 | Geometry | Create output at a separate geometry path | `Class 1 - 180 about Z reverses a slice` |
| 13 | Geometry | Replace the input geometry through deferred rename and delete actions | *Not directly tested. Current test executions create separate output geometries.* |
| 14 | Copy | Copy an additional group and declare its descendant paths | `Child names containing the geometry name are kept` — declared paths and copied objects |
| 15 | Copy | Copy an additional non-group child | *Not directly tested. The child-name fixture adds an AttributeMatrix, not a direct array child.* |
| 16 | Copy | No extra geometry children require copying | `Class 1 - 180 about Z reverses a slice` |
| 17 | Preflight | Warn that copied feature AttributeMatrices can contain stale data | *Not directly tested. The child-name regression does not assert the warning.* |
| 18 | Storage | Read source slabs, reuse overlapping slices and write output slices | `Class 1/4 - principal-90 rotations are lossless permutations`; `Class 4 - full-circle composition is identity` |
| 19 | Storage | Use bounded source pages and serialize array tasks | *Not directly tested. The hidden `real HDF5 1MiB-page permutation oracle` requires external stores.* |
| 20 | Sampling | A mapped coordinate is outside the source geometry; retain zero fill | *Not directly tested. Current fixtures map all output cell centers inside the source.* |
| 21 | Sampling | A whole output slice is outside the source; write zeros | *Not directly tested. No fixture produces an exterior output slice.* |
| 22 | Cancellation | Stop before an array task or output slice | *Not directly tested. No fixture injects cancellation.* |
| 23 | Storage | Return a source read, page range or page allocation error | *Not directly tested. No fixture injects these failures.* |
| 24 | Storage | Return a destination slice write error | *Not directly tested. No fixture injects a write failure.* |
| 25 | Dispatch | Rotate a scalar Int32 cell array | `Class 1 - 180 about Z reverses a slice` |
| 26 | Dispatch | Rotate another scalar type or a multi-component array | *Not directly tested. All rotation fixtures use scalar Int32 cell data.* |
| 27 | Geometry | Permute unequal X and Y spacing for 90 degrees about Z | `Class 1 - anisotropic spacing permutes with the axes` — checks (5,2,1) |
| 28 | Geometry | Clamp a computed zero output dimension to one | *Not directly tested. Fixtures have nonzero dimensions and positive spacing.* |
| 29 | Sampling | A valid source Z is outside the cached slab; retain zero fill | *Not directly tested. The calculated slab contains the source cells used by the fixtures.* |

## Test inventory

The 13 ordinary cases run in in-core CI. All pass in CI. The hidden case is listed separately in the inventory.

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::RotateSampleRefFrame: Class 1 - 180 about Z reverses a slice` | new-for-V&V | Checks [6,5,4,3,2,1] on a 3×2×1 input in Axis-Angle and Rotation-Matrix sections. |
| `SimplnxCore::RotateSampleRefFrame: Class 1 - exact 90-degree permutation pins chirality` | new-for-V&V | Three variants check exact dimensions and arrays for 90 degrees about Z, X and Y. The expected arrays distinguish positive from negative rotations. |
| `SimplnxCore::RotateSampleRefFrame: Class 1 - anisotropic spacing permutes with the axes` | new-for-V&V | Checks that 90 degrees about Z changes spacing (2,5,1) to (5,2,1). |
| `SimplnxCore::RotateSampleRefFrame: accepts 120-degree rotation about (111)` | new-for-V&V | Checks all 24 values form a bijection on a 2×3×4 input with unit spacing. |
| `SimplnxCore::RotateSampleRefFrame: Class 1 - non-zero input origin` | new-for-V&V | Checks exact voxel values and origin (-22,10,0), or the preserved input origin (10,20,0). Covers D2. |
| `SimplnxCore::RotateSampleRefFrame: Class 1/4 - principal-90 rotations are lossless permutations` | new-for-V&V | Nine rotations (90, 180 and 270 degrees about X, Y and Z), each in both representations. Checks dimensions, all input values exactly once, and no zero fill. |
| `SimplnxCore::RotateSampleRefFrame: Class 4 - full-circle composition is identity` | new-for-V&V | Four variants: four 90-degree rotations about Z, X or Y, and two 180-degree rotations about Z. Checks the original dimensions and every original value. |
| `SimplnxCore::RotateSampleRefFrame: KeepInputGeometryOrigin controls output origin` | new-for-V&V | Checks (0,0,0) with keep-origin enabled and (-3,0,0) with it disabled. |
| `SimplnxCore::RotateSampleRefFrame: slice-by-slice 180 about Y is a lossless per-slice flip` | new-for-V&V | Checks value conservation in both slice and whole-volume modes and requires their output arrays to differ. |
| `SimplnxCore::RotateSampleRefFrame: rejects non-principal-90 rotations` | new-for-V&V | Checks `-6850` for 45 degrees about Z, 90 degrees about (111), and a 45-degree rotation matrix. |
| `SimplnxCore::RotateSampleRefFrame: rejects slice-by-slice with a slice-reordering rotation` | new-for-V&V | Checks `-6851` for 90 degrees about X with slice-by-slice enabled. |
| `SimplnxCore::RotateSampleRefFrame: real HDF5 1MiB-page permutation oracle` | new-for-V&V | Hidden. Checks HDF5 input and output stores, dispatch counters, and all 264,196 values against a per-plane reversal formula in both slice modes. |
| `SimplnxCore::RotateSampleRefFrameFilter: SIMPL Backwards Compatibility` | kept | Two dynamic variants convert SIMPL 6.4 and 6.5 fixtures and check filter identity, comments and selected arguments. |
| `SimplnxCore::RotateSampleRefFrame: Child names containing the geometry name are kept` | kept | Checks declared descendant paths and copied object names when `Geom` contains `Geom Feature Data/Geom Values`. Covers D3. |

## Exemplar archive

- **Archive:** None. The v2 and v3 rotation archives are retired; the oracle is inline in the test source.
- **SHA512:** Not applicable. `src/Plugins/SimplnxCore/test/CMakeLists.txt` has no active archive registration for this filter.
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/RotateSampleRefFrameFilter.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/RotateSampleRefFrameFilter.md` for the root cause, affected users and recommendation. This table lists only Part 1.

| Deviation | Observed difference |
|-----------|---------------------|
| `RotateSampleRefFrameFilter-D1` | SIMPLNX rejects rotations outside the proper signed axis permutations (`-6850`) and rejects slice-by-slice rotations that exchange Z with another axis (`-6851`). DREAM.3D v6.5.171 has neither guard. |
