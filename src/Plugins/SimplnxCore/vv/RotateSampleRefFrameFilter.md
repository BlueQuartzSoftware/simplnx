# V&V Report: RotateSampleRefFrameFilter

|           |                  |
|-----------|------------------|
| Plugin    | SimplnxCore      |
| SIMPLNX UUID | `d2451dc1-a5a1-4ac2-a64d-7991669dcffc` |
| SIMPLNX Human Name | Rotate Sample Reference Frame |
| DREAM3D 6.5.171 equivalent | `RotateSampleRefFrame` (SIMPL UUID `{e25d9b4c-2b37-578c-b1de-cf7032b5ef19}`) — `Source/Plugins/Sampling/SamplingFilters/RotateSampleRefFrame.{h,cpp}` |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE — 2026-07-16 |
| Sign-off | Michael Jackson <mike.jackson@bluequartz.net> — 2026-07-16 |

## At a glance

| Aspect                 | Current state            |
|------------------------|--------------------------|
| Algorithm Relationship | **Rewrite** — generalized Eigen 4×4-affine reimplementation (via shared `ImageRotationUtilities`) of the legacy hand-rolled `RotateSampleRefFrame`; different UUID (SIMPL→NX mapped), different NN rule (cell-center `computeCellIndex` vs truncation), new-geometry vs in-place, plus new `KeepInputGeometryOrigin` + Rotation-Matrix representation. |
| Oracle (confirmed)     | **Class 1 (Analytical)** — exact voxel permutations: an explicit 180@Z slice reversal, **hand-derived 90-degree permutations about X/Y/Z that pin the rotation chirality** (a +90 and a -90 give different arrays, so an inverse-transform regression is caught), non-zero-origin placement, and anisotropic-spacing permutation. **Class 4 (Invariant)** — value-multiset conservation, zero background, full-circle composition = identity, and acceptance of the full octahedral group (120@(111)). Encoded in `test/RotateSampleRefFrameTest.cpp` (11 cases, all pass). |
| Code paths enumerated  | 11 (filter preflight guard + geometry setup, and the NN-resample execute). 9 of 11 exercised.          |
| Tests today            | 11 test cases: 10 new-for-V&V (inline Class 1/4 oracle + 2 guard error paths) + 1 kept SIMPL backward-compat. Parameter coverage spans 9 principal-90 rotations × 2 representations, exact chirality-pinning 90s, non-zero origin, anisotropic spacing, 120@(111) acceptance, slice-by-slice, origin handling, and the error paths. Every case calls `CheckArraysInheritTupleDims`. |
| Exemplar archive       | **`Rotate_Sample_Ref_Frame_Test_v2/v3.tar.gz` retired** — golden-file (regression) oracle replaced by an inlined Class 1 analytical oracle. No archive is downloaded. See `vv/provenance/RotateSampleRefFrameFilter.md`.                                               |
| Legacy comparison      | **Run** (SIMPLNX vs 6.5.171) on four principal-90 fixtures (90@Z, 180@Z, 90@X, 180@Y): **bit-identical** — same dims and same voxel values. 1 deviation, on the *unsupported* arbitrary-rotation domain only.    |
| Bug flags              | None. The single deviation (D1) is an intentional guard, not a bug.                                    |
| V&V phase              | Discovery, oracle design + reconciliation, algorithm review (2 fixed, 1 deferred = shared-utility `std::cout` cleanup), test rework, full-build validation (52/52 affected tests inc. ReadH5Ebsd/ITK), legacy A/B, docs — **complete**. **V&V complete and signed off by Michael Jackson (technical authority) 2026-07-16.** Outstanding: OOC dual-build run. |

## Summary

`RotateSampleRefFrame` rotates the reference frame of an **Image Geometry** by nearest-neighbor resampling. V&V established that the operation is a *lossless voxel permutation* — the only correct meaning of a reference-frame rotation — **if and only if** the rotation maps the cubic voxel grid onto itself, i.e. it is a member of the octahedral rotation group (the 90/180/270-degree rotations about X/Y/Z, plus 180 about a face-diagonal and 120/240 about a body-diagonal); for any other rotation it silently produces a lossy, background-padded resample (empirically demonstrated: 45@Z turns 24 voxels into 50 with 24 introduced zeros). A preflight **guard** was added to enforce the supported domain (error `-6850`, plus `-6851` for a slice-reordering slice-by-slice combination), the golden-file exemplar was retired in favor of an inlined **Class 1 analytical-permutation** oracle (+ Class 4 invariants), and a legacy A/B on four principal-90 fixtures showed SIMPLNX is **bit-identical** to DREAM3D 6.5.171 on the standard EBSD sample transforms.

## Algorithm Relationship

*Classification:* **Rewrite** (same mapped UUID — a claim of functional equivalence, defended by the deviations file).

*Evidence:* The legacy `RotateSampleRefFrame::execute()` is a hand-rolled TBB `blocked_range3d` kernel using `float rotMat[3][3]`, `MatrixMath`, in-place mutation of the DataContainer, and a truncation source-index rule (`colOld = (int64)(coord/xRes)`, old-origin assumed 0). SIMPLNX reimplements this atop the shared `ImageRotationUtilities` as a generalized Eigen `Matrix4fR` 4×4 affine transform, resamples via origin-aware `ImageGeom::computeCellIndex` on each output **cell center**, creates a **new** geometry (or a rename/delete/rename dance for in-place), parallelizes across arrays (`ParallelTaskAlgorithm`), and adds two features with no legacy equivalent: `KeepInputGeometryOrigin` (PR #1355) and an explicit **Rotation Matrix** representation. The SIMPL UUID is preserved via `SimplnxCoreLegacyUUIDMapping.hpp` + 6.4/6.5 conversion fixtures.

*Material changes:* (1) a preflight guard restricting the accepted rotations to the well-posed lossless-grid (octahedral-group) domain (see Deviation D1); (2) a fix to the transform-derived output origin — preflight previously added the input origin to the already-absolute transformed-bounding-box min corner, double-counting it for any non-zero input origin; the persisted output origin now equals the `RotateArgs::TransformedOrigin` the resample worker samples against (no change for the common origin-(0,0,0) case). The remaining review-driven edits (dead-code removal, the `IsPrincipalAxis90Rotation`→`IsLosslessGridRotation` rename, error-message wording) are non-functional.

## Oracle

*Class:* **1 (Analytical)** primary, **4 (Invariant)** companion. Class 2/3 N/A.

*Applied:* For a 90/180/270-degree rotation about a principal axis, nearest-neighbor resampling maps the voxel grid exactly onto itself, so the output is a closed-form permutation of the input computable by hand: a 180@Z rotation of a single Z-slice is a full reversal (`1..6 → 6..1`), and the output dimensions are a deterministic axis permutation of the input dimensions. The invariant companion asserts that a principal-90 rotation is a bijection on the voxel set (output value-multiset equals input, no `0` background introduced) and that applying the rotation until it sums to 360 degrees returns the original array and dimensions exactly.

*Encoded:* `test/RotateSampleRefFrameTest.cpp` — `Class 1 - 180 about Z reverses a slice` (explicit expected array, both representations); `Class 1 - exact 90-degree permutation pins chirality` (hand-derived arrays for 90@X/Y/Z, distinguishing +90 from -90); `Class 1 - non-zero input origin` (transform-derived origin = absolute transformed min corner, plus keep-origin); `Class 1 - anisotropic spacing permutes with the axes`; `Class 1/4 - principal-90 rotations are lossless permutations` (9 rotations × 2 representations: exact dims + value bijection); `Class 4 - full-circle composition is identity`; `accepts 120-degree rotation about (111)`; `KeepInputGeometryOrigin controls output origin`; `slice-by-slice 180 about Y is a lossless per-slice flip`. 11 test cases, all pass (in-core).

*Second-engineer review:* **Signed off by Michael Jackson (technical authority), 2026-07-16.**

## Code path coverage

*9 of 11 paths exercised. The 2 gaps are a defensive out-of-bounds branch (unreachable on the guarded lossless-rotation domain, where every output cell maps in-bounds by construction) and the cancel-signal branches (require mid-execution cancel injection). Neither is algorithmic logic.*

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/RotateSampleRefFrameFilter.cpp` (preflight guard + geometry setup) and `src/SimplnxCore/Filters/Algorithms/RotateSampleRefFrame.cpp` (108 lines) delegating to `src/simplnx/Utilities/ImageRotationUtilities.{hpp,cpp}` (nearest-neighbor path). Logical phases: (a) preflight — validate rotation + build output geometry/actions; (b) execute — per-array nearest-neighbor resample.

| #  | Phase        | Path                                                   | Test case                   |
|----|--------------|-------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------|
| 1  | (a) Preflight| Rotation is a proper signed-permutation (octahedral group) → proceed                                        | all valid tests (`Class 1`, `Class 1/4`, composition, origin, spacing, 120@(111)) |
| 2  | (a) Preflight| Rotation is not a lossless grid rotation → error `-6850`                                                    | `rejects non-principal-90 rotations` (45@Z, 90@(1,1,1), arbitrary matrix)         |
| 3  | (a) Preflight| slice-by-slice + rotation does not preserve Z (\|R(2,2)\|≠1) → error `-6851`                                | `rejects slice-by-slice with a slice-reordering rotation` (90@X + slice-by-slice) |
| 4  | (a) Preflight| Representation = Axis-Angle → `GenerateRotationTransformationMatrix`                                         | all Axis-Angle sections     |
| 5  | (a) Preflight| Representation = Rotation Matrix → `GenerateManualTransformationMatrix`                                      | `Class 1` + `Class 1/4` Rotation-Matrix sections                                 |
| 6  | (b) Execute  | Nearest-neighbor resample, slice-by-slice = false (3D rotation)                                             | `Class 1`, `Class 1/4`, `full-circle composition`                                |
| 7  | (b) Execute  | Nearest-neighbor resample, slice-by-slice = true, Z-preserving → per-slice flip                             | `slice-by-slice 180 about Y is a lossless per-slice flip`                        |
| 8  | (b) Execute  | Inverse-mapped coordinate out of bounds → `fillTuple(destIndex, 0)`                                         | *Not directly tested. Unreachable on the guarded lossless-rotation domain (exact permutation maps every cell in-bounds); defensive branch.* |
| 9  | (a) Preflight| `RemoveOriginalGeometry = true` → rename/delete/rename in-place                                             | legacy A/B pipelines (nxrunner, `remove_original_geometry=true`) + shipping pipelines |
| 10 | (b) Execute  | `KeepInputGeometryOrigin` true (keep src origin) vs false (transform-derived origin)                        | `KeepInputGeometryOrigin controls output origin` (both branches, exact values)   |
| 11 | (b) Execute  | Cancel checks (`m_ShouldCancel` in the array loop; `getCancel()` per output slice)                          | *Not tested. Requires cancel-signal injection; low-value.*                       |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::RotateSampleRefFrame: Class 1 - 180 about Z reverses a slice` | new-for-V&V | Explicit hand-derived expected array `[6,5,4,3,2,1]` on a 3×2×1 volume; Axis-Angle + Rotation-Matrix sections (the matrix section incidentally cross-checks `GenerateRotationTransformationMatrix` against an Eigen-built matrix). |
| `SimplnxCore::RotateSampleRefFrame: Class 1 - exact 90-degree permutation pins chirality` | new-for-V&V | Hand-derived exact output arrays for 90@Z (`[4,1,5,2,6,3]`), 90@X, 90@Y. Distinguishes +90 from -90, so a regression that dropped the inverse-transform (forward matrix instead of its inverse) is caught. Derivation method validated by reproducing the 180@Z result. |
| `SimplnxCore::RotateSampleRefFrame: Class 1 - non-zero input origin` | new-for-V&V | Input origin (10,20,0), 90@Z → transform-derived origin (-22,10,0) (absolute transformed min corner), not the double-counted (-12,30,0); keep-origin branch preserves the input origin. Both branches also assert the value permutation is origin-invariant. |
| `SimplnxCore::RotateSampleRefFrame: Class 1 - anisotropic spacing permutes with the axes` | new-for-V&V | Input spacing (2,5,1), 90@Z → output spacing (5,2,1); pins the spacing permutation that isotropic fixtures cannot detect. |
| `SimplnxCore::RotateSampleRefFrame: accepts 120-degree rotation about (111)` | new-for-V&V | Confirms the guard accepts the full octahedral group (not just principal-90): 120@(111) is a lossless value bijection. Documents that the enforced domain is broader than "principal-axis 90s". |
| `SimplnxCore::RotateSampleRefFrame: Class 1/4 - principal-90 rotations are lossless permutations` | new-for-V&V | 9 rotations (90/180/270 × X/Y/Z) × 2 representations = 18 DYNAMIC_SECTIONs; asserts exact permuted output dims + value-multiset bijection + zero background. |
| `SimplnxCore::RotateSampleRefFrame: Class 4 - full-circle composition is identity` | new-for-V&V | 4×90@Z, 2×180@Z, 4×90@X, 4×90@Y each return the original array + dims exactly. |
| `SimplnxCore::RotateSampleRefFrame: KeepInputGeometryOrigin controls output origin` | new-for-V&V | 90@Z on a 4×3×2 at origin (0,0,0): keep → (0,0,0); transform-derived → (-3,0,0). |
| `SimplnxCore::RotateSampleRefFrame: slice-by-slice 180 about Y is a lossless per-slice flip` | new-for-V&V | Value bijection for the slice-by-slice flip, and asserts it differs from the true 3D rotation (slice order preserved vs reversed). |
| `SimplnxCore::RotateSampleRefFrame: rejects non-principal-90 rotations` | new-for-V&V | 45@Z, 90@(1,1,1), and an arbitrary 45° Rotation Matrix each return preflight error `-6850`. |
| `SimplnxCore::RotateSampleRefFrame: rejects slice-by-slice with a slice-reordering rotation` | new-for-V&V | 90@X + slice-by-slice returns preflight error `-6851`. |
| `SimplnxCore::RotateSampleRefFrameFilter: SIMPL Backwards Compatibility` | kept | Unchanged. DYNAMIC_SECTION over SIMPL 6.4 + 6.5 conversion fixtures; validates UUID + argument keys. |

All non-retired tests pass in-core. Full `NX-Com-Qt69-Vtk96-Rel` build: affected tests pass, including `ReadH5Ebsd`, all EBSD readers, `ITKImportImageStack`, and this filter's SIMPL compat test. OOC dual-build run outstanding (guard is preflight-only; negligible OOC risk).

## Exemplar archive

- **Archive:** None. The prior golden-file exemplars `Rotate_Sample_Ref_Frame_Test_v2.tar.gz` / `_v3.tar.gz` were **retired** this V&V cycle (a regression-on-prior-output oracle) and replaced by an inlined Class 1 analytical oracle. The `download_test_data()` entries were removed from `test/CMakeLists.txt`.
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/RotateSampleRefFrameFilter.md` (documents the retirement, the inline oracle, and the legacy A/B inputs).

## Deviations from DREAM3D 6.5.171

Comparison run on four principal-90 fixtures (90@Z, 180@Z, 90@X, 180@Y) — see `vv/comparisons/RotateSampleRefFrameFilter/results.md`. On the supported domain, **no deviations** (bit-identical dims and voxel values). One deviation on the unsupported domain:

- `RotateSampleRefFrameFilter-D1` — legacy silently runs arbitrary (non-principal-90) rotations as a lossy resample; SIMPLNX rejects them in preflight (`-6850`). Root cause: algorithmic choice (intentional; not a bug). See `vv/deviations/RotateSampleRefFrameFilter.md`.
