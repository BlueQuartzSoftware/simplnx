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

*Updated 2026-10-29 for Clear Feature Attribute Matrix on `topic/correct_feature_am_updates`. Historical sign-off and OOC evidence below apply to the earlier rotation implementation; the new Feature AM contract is validated separately in this note.*

*Updated 2026-11-03: added one test for a Feature AM that is also linked inside another child group of the geometry (`Feature AM nested alias regression`).*

## At a glance

| Aspect                 | Current state            |
|------------------------|--------------------------|
| Algorithm Relationship | **Rewrite** — generalized Eigen 4×4-affine reimplementation (via shared `ImageRotationUtilities`) of the legacy hand-rolled `RotateSampleRefFrame`; different UUID (SIMPL→NX mapped), different NN rule (cell-center `computeCellIndex` vs truncation), new-geometry vs in-place, plus new `KeepInputGeometryOrigin` + Rotation-Matrix representation. |
| Oracle (confirmed)     | **Class 1 (Analytical)** — exact voxel permutations: an explicit 180@Z slice reversal, **hand-derived 90-degree permutations about X/Y/Z that pin the rotation chirality** (a +90 and a -90 give different arrays, so an inverse-transform regression is caught), non-zero-origin placement, and anisotropic-spacing permutation. **Class 4 (Invariant)** — value-multiset conservation, zero background, full-circle composition = identity, and acceptance of the full octahedral group (120@(111)). Encoded in `test/RotateSampleRefFrameTest.cpp`; the new Feature AM tests add hand-computed structural and cell-value expectations. |
| Code paths enumerated  | 19 (preflight, geometry setup, rotation, and Feature AM handling). 17 of 19 exercised.          |
| Tests today | 22 TEST_CASEs: 14 modified and 8 new. 21/21 registered cases pass in the rebuilt in-core configuration (2026-11-03); one hidden HDF5 boundary case was not rerun. |
| Exemplar archive       | **`Rotate_Sample_Ref_Frame_Test_v2/v3.tar.gz` retired** — golden-file (regression) oracle replaced by an inlined Class 1 analytical oracle. No archive is downloaded. See `vv/provenance/RotateSampleRefFrameFilter.md`.                                               |
| Legacy comparison      | **Run** (SIMPLNX vs 6.5.171) on four principal-90 fixtures (90@Z, 180@Z, 90@X, 180@Y): **bit-identical** — same dims and same voxel values. D1 covers unsupported arbitrary rotations; D2 covers the native Clear default. Converted 6.x pipelines keep Clear off.    |
| Bug flags              | None. D1 is an intentional rotation guard; D2 is the intentional Feature AM default.                                    |
| V&V phase | Historical status and sign-off retained. Historical real-HDF5 verification is retained. The 2026-10-29 change adds Feature AM tests and conversion checks. |

## Summary

`RotateSampleRefFrame` rotates the reference frame of an **Image Geometry** by nearest-neighbor resampling. V&V established that the operation is a *lossless voxel permutation* — the only correct meaning of a reference-frame rotation — **if and only if** the rotation maps the cubic voxel grid onto itself, i.e. it is a member of the octahedral rotation group (the 90/180/270-degree rotations about X/Y/Z, plus 180 about a face-diagonal and 120/240 about a body-diagonal); for any other rotation it silently produces a lossy, background-padded resample (empirically demonstrated: 45@Z turns 24 voxels into 50 with 24 introduced zeros). A preflight **guard** was added to enforce the supported domain (error `-6850`, plus `-6851` for a slice-reordering slice-by-slice combination), the golden-file exemplar was retired in favor of an inlined **Class 1 analytical-permutation** oracle (+ Class 4 invariants), and a legacy A/B on four principal-90 fixtures showed SIMPLNX is **bit-identical** to DREAM3D 6.5.171 on the standard EBSD sample transforms.

## Algorithm Relationship

*Classification:* **Rewrite** (same mapped UUID — a claim of functional equivalence, defended by the deviations file).

*Evidence:* The legacy `RotateSampleRefFrame::execute()` is a hand-rolled TBB `blocked_range3d` kernel using `float rotMat[3][3]`, `MatrixMath`, in-place mutation of the DataContainer, and a truncation source-index rule (`colOld = (int64)(coord/xRes)`, old-origin assumed 0). SIMPLNX reimplements this atop the shared `ImageRotationUtilities` as a generalized Eigen `Matrix4fR` 4×4 affine transform, resamples via origin-aware `ImageGeom::computeCellIndex` on each output **cell center**, creates a **new** geometry (or a rename/delete/rename dance for in-place), parallelizes across arrays (`ParallelTaskAlgorithm`), and adds two features with no legacy equivalent: `KeepInputGeometryOrigin` (PR #1355) and an explicit **Rotation Matrix** representation. The SIMPL UUID is preserved via `SimplnxCoreLegacyUUIDMapping.hpp` + 6.4/6.5 conversion fixtures.

*Material changes:* (1) a preflight guard restricting the accepted rotations to the well-posed lossless-grid (octahedral-group) domain (see Deviation D1); (2) a fix to the transform-derived output origin — preflight previously added the input origin to the already-absolute transformed-bounding-box min corner, double-counting it for any non-zero input origin; the persisted output origin now equals the `RotateArgs::TransformedOrigin` the resample worker samples against (no change for the common origin-(0,0,0) case). (3) Clear Feature Attribute Matrix, default on in native pipelines, with converted 6.x pipelines starting with Clear off (D2). The remaining review-driven edits (dead-code removal, the `IsPrincipalAxis90Rotation`→`IsLosslessGridRotation` rename, error-message wording) are non-functional.

## Oracle

*Class:* **1 (Analytical)** primary, **4 (Invariant)** companion. Class 2/3 N/A.

*Applied:* For a 90/180/270-degree rotation about a principal axis, nearest-neighbor resampling maps the voxel grid exactly onto itself, so the output is a closed-form permutation of the input computable by hand: a 180@Z rotation of a single Z-slice is a full reversal (`1..6 → 6..1`), and the output dimensions are a deterministic axis permutation of the input dimensions. The invariant companion asserts that a principal-90 rotation is a bijection on the voxel set (output value-multiset equals input, no `0` background introduced) and that applying the rotation until it sums to 360 degrees returns the original array and dimensions exactly.

*Encoded:* `test/RotateSampleRefFrameTest.cpp` — `Class 1 - 180 about Z reverses a slice` (explicit expected array, both representations); `Class 1 - exact 90-degree permutation pins chirality` (hand-derived arrays for 90@X/Y/Z, distinguishing +90 from -90); `Class 1 - non-zero input origin` (transform-derived origin = absolute transformed min corner, plus keep-origin); `Class 1 - anisotropic spacing permutes with the axes`; `Class 1/4 - principal-90 rotations are lossless permutations` (9 rotations × 2 representations: exact dims + value bijection); `Class 4 - full-circle composition is identity`; `accepts 120-degree rotation about (111)`; `KeepInputGeometryOrigin controls output origin`; `slice-by-slice 180 about Y is a lossless per-slice flip`. 11 test cases, all pass (in-core).

*Feature Attribute Matrix oracle (2026-10-29):* Class 1, hand-computed. A 2×2×1 image has Feature Ids {1,1,3,3}. A 180-degree Z rotation reverses that slice to {3,3,1,1}. Features 1 and 3 keep two cells each; Id 2 remains unused. The Feature matrix has source shape {2,2}, four tuples, and three children: counts {0,2,0,2}, neighbor lists {{},{3},{},{1}}, and names {Background,One,Unused,Three}. Clear on must keep shape {2,2} and four tuples with zero children. Clear off must copy all three arrays exactly. Ensemble shape {2} and CrystalStructures {999,1} must be unchanged in both modes. A second parent must resolve to the same source matrix without copying it into the output. In-place rotation must remove temporary geometries and leave the empty matrix at the original geometry path. The 6.4 and 6.5 fixtures must convert with Clear off and preflight with only a Cell AM. No production rotation helper computes these expectations. New execution cases witness both algorithm paths on in-memory stores; they do not claim real OOC storage coverage.

*Second-engineer review:* **Signed off by Michael Jackson (technical authority), 2026-07-16.**

## Bugs found and fixed

No new defect was found during this storage recertification. Existing fixes and deviation dispositions remain documented above and in the sidecar.

## Code path coverage

*17 of 19 paths exercised. The 2 gaps are a defensive out-of-bounds branch (unreachable on the guarded lossless-rotation domain, where every output cell maps in-bounds by construction) and the cancel-signal branches (require mid-execution cancel injection). Neither is algorithmic logic.*

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/RotateSampleRefFrameFilter.cpp` (preflight guard + geometry setup) and `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/RotateSampleRefFrame.cpp` delegating to `src/simplnx/Utilities/ImageRotationUtilities.{hpp,cpp}` (nearest-neighbor path). Logical phases: (a) preflight — validate rotation + build output geometry/actions; (b) execute — per-array nearest-neighbor resample; (c) Feature Attribute Matrix handling.

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
| 12 | OOC boundary | 1MiB-page permutation oracle | `real HDF5 1MiB-page permutation oracle` — independent expected outputs on actual HDF5 stores |
| 13 | Feature AM | Clear OFF copies non-Cell matrices unchanged and reports "Feature Data Not Cleared"; selection is ignored | `Clear OFF copies Feature AM unchanged` |
| 14 | Feature AM | Clear ON recreates source shape with zero children and reports all cleared paths; Ensemble unchanged | `Clear ON preflight`, `Clear ON execute` |
| 15 | Feature AM | Matrix outside or nested below geometry is not a direct child → `-67441` | `Clear ON requires a direct child Feature AM` (both sections assert code and corrective hint) |
| 16 | Feature AM | Selected matrix is the Cell AM → `-67442` | `Clear ON rejects the Cell AM` (code asserted) |
| 17 | Feature AM | Selection through a second parent is accepted; all paths excluded from generic copy, also a link inside another child group | `Clear ON accepts shared parent paths`, `Feature AM nested alias regression` |
| 18 | Feature AM | Clear ON with deferred in-place rename/delete keeps empty matrix at source path | `Clear ON in place` |
| 19 | Conversion | SIMPL 6.x conversion sets Clear off and preflights without a Feature AM | `SIMPL Backwards Compatibility` (6.4 and 6.5) |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::RotateSampleRefFrame: Class 1 - 180 about Z reverses a slice` | modified | Explicit hand-derived expected array `[6,5,4,3,2,1]` on a 3×2×1 volume; Axis-Angle + Rotation-Matrix sections (the matrix section incidentally cross-checks `GenerateRotationTransformationMatrix` against an Eigen-built matrix). Explicit Clear=false through MakeAxisAngleArgs. |
| `SimplnxCore::RotateSampleRefFrame: Class 1 - exact 90-degree permutation pins chirality` | modified | Hand-derived exact output arrays for 90@Z (`[4,1,5,2,6,3]`), 90@X, 90@Y. Distinguishes +90 from -90, so a regression that dropped the inverse-transform (forward matrix instead of its inverse) is caught. Derivation method validated by reproducing the 180@Z result. Explicit Clear=false through MakeAxisAngleArgs. |
| `SimplnxCore::RotateSampleRefFrame: Class 1 - anisotropic spacing permutes with the axes` | modified | Input spacing (2,5,1), 90@Z → output spacing (5,2,1); pins the spacing permutation that isotropic fixtures cannot detect. Explicit Clear=false through MakeAxisAngleArgs. |
| `SimplnxCore::RotateSampleRefFrame: accepts 120-degree rotation about (111)` | modified | Confirms the guard accepts the full octahedral group (not just principal-90): 120@(111) is a lossless value bijection. Documents that the enforced domain is broader than "principal-axis 90s". Explicit Clear=false through MakeAxisAngleArgs. |
| `SimplnxCore::RotateSampleRefFrame: Class 1 - non-zero input origin` | modified | Input origin (10,20,0), 90@Z → transform-derived origin (-22,10,0) (absolute transformed min corner), not the double-counted (-12,30,0); keep-origin branch preserves the input origin. Both branches also assert the value permutation is origin-invariant. Explicit Clear=false through MakeAxisAngleArgs. |
| `SimplnxCore::RotateSampleRefFrame: Class 1/4 - principal-90 rotations are lossless permutations` | modified | 9 rotations (90/180/270 × X/Y/Z) × 2 representations = 18 DYNAMIC_SECTIONs; asserts exact permuted output dims + value-multiset bijection + zero background. Explicit Clear=false through MakeAxisAngleArgs. |
| `SimplnxCore::RotateSampleRefFrame: Class 4 - full-circle composition is identity` | modified | 4×90@Z, 2×180@Z, 4×90@X, 4×90@Y each return the original array + dims exactly. Explicit Clear=false through MakeAxisAngleArgs. |
| `SimplnxCore::RotateSampleRefFrame: KeepInputGeometryOrigin controls output origin` | modified | 90@Z on a 4×3×2 at origin (0,0,0): keep → (0,0,0); transform-derived → (-3,0,0). Explicit Clear=false through MakeAxisAngleArgs. |
| `SimplnxCore::RotateSampleRefFrame: slice-by-slice 180 about Y is a lossless per-slice flip` | modified | Value bijection for the slice-by-slice flip, and asserts it differs from the true 3D rotation (slice order preserved vs reversed). Explicit Clear=false through MakeAxisAngleArgs. |
| `SimplnxCore::RotateSampleRefFrame: rejects non-principal-90 rotations` | modified | 45@Z, 90@(1,1,1), and an arbitrary 45° Rotation Matrix each return preflight error `-6850`. Explicit Clear=false through MakeAxisAngleArgs. |
| `SimplnxCore::RotateSampleRefFrame: rejects slice-by-slice with a slice-reordering rotation` | modified | 90@X + slice-by-slice returns preflight error `-6851`. Explicit Clear=false through MakeAxisAngleArgs. |
| `SimplnxCore::RotateSampleRefFrame: real HDF5 1MiB-page permutation oracle` | modified | 528,425 assertions; The existing sequential-image builder creates 257×257×4 values. A half turn about Z must reverse each XY plane without changing Z order; every output value is checked. The 264,196 int32 values cross a full 262,144-element source-cache page and a partial second page. Both whole-volume and slice modes use HDF5 input/output and target-only OOC dispatch witnesses. Explicit Clear=false through MakeAxisAngleArgs. Hidden; not run in the 2026-10-29 in-core validation. |
| `SimplnxCore::RotateSampleRefFrameFilter: SIMPL Backwards Compatibility` | modified | Converts both 6.4 and 6.5 fixtures; asserts Clear=false and preflights the converted pipeline on a geometry without a Feature AM. |
| `SimplnxCore::RotateSampleRefFrame: Child names containing the geometry name are kept` | modified | Preserves declared recursive paths and copied child names. Explicit Clear=false. |
| `SimplnxCore::RotateSampleRefFrame: Clear ON preflight` | new | Version 2 and default true; information lists numeric, string, and neighbor-list children, excludes Ensemble and renumber text; actions preserve shape {2,2}, four tuples, zero children. |
| `SimplnxCore::RotateSampleRefFrame: Clear OFF copies Feature AM unchanged` | new | Missing disabled selection is ignored; information value is present without warnings; every Feature child and Ensemble value is copied exactly; rotated Ids {3,3,1,1}. |
| `SimplnxCore::RotateSampleRefFrame: Clear ON execute` | new | Empty Feature matrix with source shape and count; source children retained; rotated Ids {3,3,1,1}; Ensemble {999,1}. |
| `SimplnxCore::RotateSampleRefFrame: Clear ON requires a direct child Feature AM` | new | Outside and nested matrices each return exactly -67441 and the corrective hint to select a direct child matrix or turn off Clear. |
| `SimplnxCore::RotateSampleRefFrame: Clear ON rejects the Cell AM` | new | The geometry Cell AM returns exactly -67442. |
| `SimplnxCore::RotateSampleRefFrame: Clear ON accepts shared parent paths` | new | Select via Aliases/Cell Feature Data; empty output matrix, no duplicate copy; both source paths retain their children; cell and Ensemble oracle unchanged. |
| `SimplnxCore::RotateSampleRefFrame: Clear ON in place` | new | Empty matrix at Input; dims {2,2,1}; exact Ids and Ensemble values; no Output, .Input, or .rotated_image_geometry remains. |
| `SimplnxCore::RotateSampleRefFrameFilter: Feature AM nested alias regression` | new | The Feature AM is also linked in `Input/Misc`, and `Misc` is also linked in `Outside`. Clear and the selected path (direct or nested) are each on and off. With Clear on, the output matrix has no children and `Misc` has no copy of the matrix; with Clear off, it has 3 children and the nested copy keeps the original counts. `Misc/Keep` is 123 in every case. The source matrix keeps its ID, parents, shape, 3 children, counts and the `Outside` link. |

There are 22 TEST_CASEs, including 21 registered cases and one hidden HDF5 case. The former duplicate per-slice inventory row is removed; the child-name regression is now listed. Status is relative to this change: 14 modified, 8 new, 0 unchanged.

Current validation (2026-10-29): rebuilt `NX-Com-Qt69-Vtk96-Rel`, `SIMPLNX_TEST_ALGORITHM_PATH=0`. Serial CTest passed 27/27 entries in the requested Rotate/FilterValidation/serialization selection, including all 20 registered Rotate cases. The requested pipeline/Python selection passed 43/43 (9 pipelines, 10 Python examples, 24 related unit cases). Internal-caller regressions passed 33/33 (29 WriteOnScale, 4 ReadH5Ebsd). The Clear branch mutation failed the six intended Clear-on tests; after byte-for-byte restoration and rebuild all seven new cases passed. No new real-OOC or legacy-binary comparison is claimed. The run logs and the full file and test list were kept with the local review notes. They are not part of the repository.

Historical validation before the Feature AM change: all non-retired tests passed in-core. Full `NX-Com-Qt69-Vtk96-Rel` build: affected tests pass, including `ReadH5Ebsd`, all EBSD readers, `ITKImportImageStack`, and this filter's SIMPL compat test. OOC dual-build run outstanding (guard is preflight-only; negligible OOC risk).

OOC recertification (2026-09-21): serial CTest passed 12/12 target entries in each DREAM3DNX build. The hidden `real HDF5 1MiB-page permutation oracle` passed 528,425 assertions in the OOC binary. The existing sequential-image builder creates 257×257×4 values. A half turn about Z must reverse each XY plane without changing Z order; every output value is checked. The 264,196 int32 values cross a full 262,144-element source-cache page and a partial second page. Both whole-volume and slice modes use HDF5 input/output and target-only OOC dispatch witnesses. The new case is included in the plugin's OOC store-contract CTest group. Upstream oracle assertions and tolerances remain intact. No new legacy binary comparison is claimed.

## Test sensitivity verification

On 2026-10-29, the Clear parameters and legacy conversion remained enabled while the production clearing branch was temporarily changed to `if(false && clearFeatureAttributeMatrix)`. The mutated source was rebuilt. Six tests failed as expected: `Clear ON preflight`, `Clear ON execute`, `Clear ON requires a direct child Feature AM`, `Clear ON rejects the Cell AM`, `Clear ON accepts shared parent paths`, and `Clear ON in place`. `Clear OFF copies Feature AM unchanged` and `SIMPL Backwards Compatibility` passed. These failures detect missing information, copied children, and bypassed ownership checks. The source was restored from a byte backup, verified with `cmp`, touched, and rebuilt before final validation.

The first attempt also found a test-fixture shape mismatch: bulk NeighborList setup flattened {2,2} to {4}. Setting the two nonempty lists individually preserves the source shape. The mutation cycle above was repeated after that fixture correction; only the six intended failures remained.

Evidence: the mutation build and CTest logs were kept with the local review notes. They are not part of the repository. After restoration, all 20 registered Rotate cases passed, including all seven new cases; the full 27-entry target selection passed.

## Exemplar archive

- **Archive:** None. The prior golden-file exemplars `Rotate_Sample_Ref_Frame_Test_v2.tar.gz` / `_v3.tar.gz` were **retired** this V&V cycle (a regression-on-prior-output oracle) and replaced by an inlined Class 1 analytical oracle. The `download_test_data()` entries were removed from `test/CMakeLists.txt`.
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/RotateSampleRefFrameFilter.md` (documents the retirement, the inline oracle, and the legacy A/B inputs).

## Deviations from DREAM3D 6.5.171

Comparison run on four principal-90 fixtures (90@Z, 180@Z, 90@X, 180@Y) — see `vv/comparisons/RotateSampleRefFrameFilter/results.md`. On the supported domain, cell data and dimensions were bit-identical. D2 changes the native Feature AM default; no new legacy binary comparison is claimed.

- `RotateSampleRefFrameFilter-D1` — legacy silently runs arbitrary (non-principal-90) rotations as a lossy resample; SIMPLNX rejects them in preflight (`-6850`). Root cause: algorithmic choice (intentional; not a bug). See `vv/deviations/RotateSampleRefFrameFilter.md`.

- `RotateSampleRefFrameFilter-D2` — native pipelines clear the Feature Attribute Matrix by default; converted 6.x pipelines start with Clear off and still match legacy for Feature data. The cleared matrix keeps the source tuple shape. The rotation does not change Feature membership or the real Feature sizes, but clearing removes every stored Feature array, including size arrays.
