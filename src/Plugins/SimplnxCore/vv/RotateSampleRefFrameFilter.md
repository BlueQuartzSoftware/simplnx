# V&V Report: RotateSampleRefFrameFilter

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `d2451dc1-a5a1-4ac2-a64d-7991669dcffc` |
| SIMPLNX Human Name | Rotate Sample Reference Frame |
| DREAM3D 6.5.171 equivalent | `RotateSampleRefFrame` (SIMPL UUID `{e25d9b4c-2b37-578c-b1de-cf7032b5ef19}`) — `Source/Plugins/Sampling/SamplingFilters/RotateSampleRefFrame.{h,cpp}` |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | READY FOR REVIEW |
| Sign-off | *<engineer(s), date>* |

## At a glance

| Aspect                 | Current state                                                                                                                                                                                                                                                                                                              |
|------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Algorithm Relationship | **Rewrite** — generalized Eigen 4×4-affine reimplementation (via shared `ImageRotationUtilities`) of the legacy hand-rolled `RotateSampleRefFrame`; different UUID (SIMPL→NX mapped), different NN rule (cell-center `computeCellIndex` vs truncation), new-geometry vs in-place, plus new `KeepInputGeometryOrigin` + Rotation-Matrix representation. |
| Oracle (confirmed)     | **Class 1 (Analytical)** — principal-90 rotations are exact voxel permutations (explicit 180@Z slice reversal + deterministic permuted output dims). **Class 4 (Invariant)** — value-multiset conservation, zero background, full-circle composition = identity. Encoded in `test/RotateSampleRefFrameTest.cpp` (7 cases, all pass). |
| Code paths enumerated  | 11 (filter preflight guard + geometry setup, and the NN-resample execute). 9 of 11 exercised.                                                                                                                                                                                                                              |
| Tests today            | 7 test cases: 6 new-for-V&V (inline Class 1/4 oracle + 2 guard error paths) + 1 kept SIMPL backward-compat. Parameter coverage spans 9 principal-90 rotations × 2 representations, slice-by-slice, origin handling, and the error paths.                                                                                    |
| Exemplar archive       | **`Rotate_Sample_Ref_Frame_Test_v2/v3.tar.gz` retired** — golden-file (regression) oracle replaced by an inlined Class 1 analytical oracle. No archive is downloaded. See `vv/provenance/RotateSampleRefFrameFilter.md`.                                                                                                    |
| Legacy comparison      | **Run** (SIMPLNX vs 6.5.171) on four principal-90 fixtures (90@Z, 180@Z, 90@X, 180@Y): **bit-identical** — same dims and same voxel values. 1 deviation, on the *unsupported* arbitrary-rotation domain only.                                                                                                              |
| Bug flags              | None. The single deviation (D1) is an intentional guard, not a bug.                                                                                                                                                                                                                                                        |
| V&V phase              | Discovery, oracle design + reconciliation, algorithm review (2 fixed, 1 deferred = shared-utility `std::cout` cleanup), test rework, full-build validation (52/52 affected tests inc. ReadH5Ebsd/ITK), legacy A/B, docs — **complete**. Outstanding: OOC dual-build run, second-engineer oracle review, status → COMPLETE. |

## Summary

`RotateSampleRefFrame` rotates the reference frame of an **Image Geometry** by nearest-neighbor resampling. V&V established that the operation is a *lossless voxel permutation* — the only correct meaning of a reference-frame rotation — **if and only if** the rotation is a 90/180/270-degree rotation about a principal (X/Y/Z) axis; for any other rotation it silently produces a lossy, background-padded resample (empirically demonstrated: 45@Z turns 24 voxels into 50 with 24 introduced zeros). A preflight **guard** was added to enforce the supported domain (error `-6850`, plus `-6851` for a slice-reordering slice-by-slice combination), the golden-file exemplar was retired in favor of an inlined **Class 1 analytical-permutation** oracle (+ Class 4 invariants), and a legacy A/B on four principal-90 fixtures showed SIMPLNX is **bit-identical** to DREAM3D 6.5.171 on the entire supported domain.

## Algorithm Relationship

*Classification:* **Rewrite** (same mapped UUID — a claim of functional equivalence, defended by the deviations file).

*Evidence:* The legacy `RotateSampleRefFrame::execute()` is a hand-rolled TBB `blocked_range3d` kernel using `float rotMat[3][3]`, `MatrixMath`, in-place mutation of the DataContainer, and a truncation source-index rule (`colOld = (int64)(coord/xRes)`, old-origin assumed 0). SIMPLNX reimplements this atop the shared `ImageRotationUtilities` as a generalized Eigen `Matrix4fR` 4×4 affine transform, resamples via origin-aware `ImageGeom::computeCellIndex` on each output **cell center**, creates a **new** geometry (or a rename/delete/rename dance for in-place), parallelizes across arrays (`ParallelTaskAlgorithm`), and adds two features with no legacy equivalent: `KeepInputGeometryOrigin` (PR #1355) and an explicit **Rotation Matrix** representation. The SIMPL UUID is preserved via `SimplnxCoreLegacyUUIDMapping.hpp` + 6.4/6.5 conversion fixtures.

*Material change this V&V cycle:* a preflight guard restricting the accepted rotations to the well-posed principal-90 domain (see Deviation D1). No other behavioral change; the review-driven edits (dead-code removal, a variable-name fix) are non-functional.

## Oracle

*Class:* **1 (Analytical)** primary, **4 (Invariant)** companion. Class 2/3 N/A.

*Applied:* For a 90/180/270-degree rotation about a principal axis, nearest-neighbor resampling maps the voxel grid exactly onto itself, so the output is a closed-form permutation of the input computable by hand: a 180@Z rotation of a single Z-slice is a full reversal (`1..6 → 6..1`), and the output dimensions are a deterministic axis permutation of the input dimensions. The invariant companion asserts that a principal-90 rotation is a bijection on the voxel set (output value-multiset equals input, no `0` background introduced) and that applying the rotation until it sums to 360 degrees returns the original array and dimensions exactly.

*Encoded:* `test/RotateSampleRefFrameTest.cpp` — `Class 1 - 180 about Z reverses a slice` (explicit expected array, both representations); `Class 1/4 - principal-90 rotations are lossless permutations` (9 rotations × 2 representations: exact dims + value bijection); `Class 4 - full-circle composition is identity`; `KeepInputGeometryOrigin controls output origin`; `slice-by-slice 180 about Y is a lossless per-slice flip`. 7 test cases, 233 assertions, all pass (in-core).

*Second-engineer review:* **Pending.**

## Code path coverage

*9 of 11 paths exercised. The 2 gaps are a defensive out-of-bounds branch (unreachable on the guarded principal-90 domain, where every output cell maps in-bounds by construction) and the cancel-signal branches (require mid-execution cancel injection). Neither is algorithmic logic.*

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/RotateSampleRefFrameFilter.cpp` (preflight guard + geometry setup) and `src/SimplnxCore/Filters/Algorithms/RotateSampleRefFrame.cpp` (108 lines) delegating to `src/simplnx/Utilities/ImageRotationUtilities.{hpp,cpp}` (nearest-neighbor path). Logical phases: (a) preflight — validate rotation + build output geometry/actions; (b) execute — per-array nearest-neighbor resample.

| #  | Phase        | Path                                                                                                        | Test case                                                                        |
|----|--------------|-------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------|
| 1  | (a) Preflight| Rotation is a proper principal-90 signed-permutation → proceed                                              | all valid tests (`Class 1`, `Class 1/4`, composition, origin, slice-by-slice)    |
| 2  | (a) Preflight| Rotation is not principal-90 → error `-6850`                                                                | `rejects non-principal-90 rotations` (45@Z, axis (1,1,1), arbitrary matrix)       |
| 3  | (a) Preflight| slice-by-slice + rotation does not preserve Z (\|R(2,2)\|≠1) → error `-6851`                                | `rejects slice-by-slice with a slice-reordering rotation` (90@X + slice-by-slice) |
| 4  | (a) Preflight| Representation = Axis-Angle → `GenerateRotationTransformationMatrix`                                         | all Axis-Angle sections                                                          |
| 5  | (a) Preflight| Representation = Rotation Matrix → `GenerateManualTransformationMatrix`                                      | `Class 1` + `Class 1/4` Rotation-Matrix sections                                 |
| 6  | (b) Execute  | Nearest-neighbor resample, slice-by-slice = false (3D rotation)                                             | `Class 1`, `Class 1/4`, `full-circle composition`                                |
| 7  | (b) Execute  | Nearest-neighbor resample, slice-by-slice = true, Z-preserving → per-slice flip                             | `slice-by-slice 180 about Y is a lossless per-slice flip`                        |
| 8  | (b) Execute  | Inverse-mapped coordinate out of bounds → `fillTuple(destIndex, 0)`                                         | *Not directly tested. Unreachable on the guarded principal-90 domain (exact permutation maps every cell in-bounds); defensive branch.* |
| 9  | (a) Preflight| `RemoveOriginalGeometry = true` → rename/delete/rename in-place                                             | legacy A/B pipelines (nxrunner, `remove_original_geometry=true`) + shipping pipelines |
| 10 | (b) Execute  | `KeepInputGeometryOrigin` true (keep src origin) vs false (transform-derived origin)                        | `KeepInputGeometryOrigin controls output origin` (both branches, exact values)   |
| 11 | (b) Execute  | Cancel checks (`m_ShouldCancel` in the array loop; `getCancel()` per output slice)                          | *Not tested. Requires cancel-signal injection; low-value.*                       |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::RotateSampleRefFrame: Class 1 - 180 about Z reverses a slice` | new-for-V&V | Explicit hand-derived expected array `[6,5,4,3,2,1]` on a 3×2×1 volume; Axis-Angle + Rotation-Matrix sections (the matrix section incidentally cross-checks `GenerateRotationTransformationMatrix` against an Eigen-built matrix). |
| `SimplnxCore::RotateSampleRefFrame: Class 1/4 - principal-90 rotations are lossless permutations` | new-for-V&V | 9 rotations (90/180/270 × X/Y/Z) × 2 representations = 18 DYNAMIC_SECTIONs; asserts exact permuted output dims + value-multiset bijection + zero background. |
| `SimplnxCore::RotateSampleRefFrame: Class 4 - full-circle composition is identity` | new-for-V&V | 4×90@Z, 2×180@Z, 4×90@X, 4×90@Y each return the original array + dims exactly. |
| `SimplnxCore::RotateSampleRefFrame: KeepInputGeometryOrigin controls output origin` | new-for-V&V | 90@Z on a 4×3×2 at origin (0,0,0): keep → (0,0,0); transform-derived → (-3,0,0). |
| `SimplnxCore::RotateSampleRefFrame: slice-by-slice 180 about Y is a lossless per-slice flip` | new-for-V&V | Value bijection for the slice-by-slice flip, and asserts it differs from the true 3D rotation (slice order preserved vs reversed). |
| `SimplnxCore::RotateSampleRefFrame: rejects non-principal-90 rotations` | new-for-V&V | 45@Z, 90@(1,1,1), and an arbitrary 45° Rotation Matrix each return preflight error `-6850`. |
| `SimplnxCore::RotateSampleRefFrame: rejects slice-by-slice with a slice-reordering rotation` | new-for-V&V | 90@X + slice-by-slice returns preflight error `-6851`. |
| `SimplnxCore::RotateSampleRefFrameFilter: SIMPL Backwards Compatibility` | kept | Unchanged. DYNAMIC_SECTION over SIMPL 6.4 + 6.5 conversion fixtures; validates UUID + argument keys. |

All non-retired tests pass in-core (233 assertions). Full `NX-Com-Qt69-Vtk96-Rel` build: 52/52 affected tests pass, including `ReadH5Ebsd`, all EBSD readers, `ITKImportImageStack`, and this filter's SIMPL compat test. OOC dual-build run outstanding (guard is preflight-only; negligible OOC risk).

## Exemplar archive

- **Archive:** None. The prior golden-file exemplars `Rotate_Sample_Ref_Frame_Test_v2.tar.gz` / `_v3.tar.gz` were **retired** this V&V cycle (a regression-on-prior-output oracle) and replaced by an inlined Class 1 analytical oracle. The `download_test_data()` entries were removed from `test/CMakeLists.txt`.
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/RotateSampleRefFrameFilter.md` (documents the retirement, the inline oracle, and the legacy A/B inputs).

## Deviations from DREAM3D 6.5.171

Comparison run on four principal-90 fixtures (90@Z, 180@Z, 90@X, 180@Y) — see `vv/comparisons/RotateSampleRefFrameFilter/results.md`. On the supported domain, **no deviations** (bit-identical dims and voxel values). One deviation on the unsupported domain:

- `RotateSampleRefFrameFilter-D1` — legacy silently runs arbitrary (non-principal-90) rotations as a lossy resample; SIMPLNX rejects them in preflight (`-6850`). Root cause: algorithmic choice (intentional; not a bug). See `vv/deviations/RotateSampleRefFrameFilter.md`.
