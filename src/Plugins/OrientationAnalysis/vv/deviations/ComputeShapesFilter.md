# Deviations from DREAM3D 6.5.171: ComputeShapesFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent, `FindShapes` (SIMPL UUID `3b0ababf-9c8d-538d-96af-e40775c4f0ab`).

Entries are referenced by stable ID (`ComputeShapesFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

## Headline: legacy A/B comparison performed — every shape output differs, and the reason is fully accounted for

Nine fixtures were run through the DREAM3D 6.5.171 `PipelineRunner` (`FindFeatureCentroids` → `FindShapes`) and through `nxrunner` (`ComputeFeatureCentroids` → `ComputeShapes`) and diffed array by array. `Centroids` is bit-exact on all nine — the two centroid filters are genuinely the same algorithm, which is the sanity anchor that validates the harness. `Volumes` is bit-exact on seven of nine. **Every one of `Omega3s`, `AxisLengths`, `AspectRatios` and `AxisEulerAngles` differs on every fixture.**

Three defects account for the whole of it: `-D1` and `-D2` in the two-dimensional path (present in both codebases, fixed in SIMPLNX this pass) and `-D3` in the three-dimensional path (present only in 6.5.171). A surgical patch to a local build of the legacy source implementing exactly those three corrections and nothing else makes the legacy build reproduce this filter's output on all nine fixtures — bit-exact on seven of nine, and to within `4e-6` relative on the remaining two (F1 and F7), for float32 reasons named in the V&V report. That is the proof that nothing else is going on. The patched build is internal proof tooling, not a shipping comparison target.

`-D4` is a latent issue shared by both codebases and is documented, not changed.

---

## ComputeShapesFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeShapesFilter-D1` |
| **Filter UUID** | `036b17d5-23bb-4a24-9187-c4a8dd918792` |
| **Status** | active (shared bug **fixed in SIMPLNX during this V&V cycle**; documented for users of prior SIMPLNX releases and for anyone comparing against 6.5.171) |

**Symptom:** On any `ImageGeometry` with one dimension of a single cell, every shape output — `AxisLengths`, `AspectRatios` and `AxisEulerAngles` — was wrong for every **Feature**. On a 4x8x1 slab of isotropic spacing 0.5 fully labelled as one feature, `AxisLengths` was `(2.278937, 1.2001256, 0)` where the correct answer is `(2.2840717, 1.1352981, 0)`, `AspectRatios[0]` was `0.5266163` instead of `0.4970501`, and the axis Euler angle was `1.5086188` rad instead of `pi/2`. The error is small on large **Features** and grows without bound as a **Feature** shrinks toward a single cell.

**Root cause:** Bug, shared. `findMoments2D` sampled the voxel **corner**:

```
float x = static_cast<float>(xPoint * modXRes) + (origin[0] * static_cast<float>(m_ScaleFactor));
```

which is `origin + index * spacing`, while the `Centroids` array whose values it subtracts comes from `ComputeFeatureCentroids`, which averages voxel **centers** (`Algorithms/ComputeFeatureCentroids.cpp:63`, `ImageGeom::getCoords`). Every offset therefore carried a systematic `-spacing/2` bias. Legacy `FindShapes.cpp:508-509` is the same two lines, and legacy `FindFeatureCentroids.cpp:175` likewise uses `getCoords`, so the defect is identical in both. The three-dimensional branch had already been corrected in SIMPLNX (issue #1124, `imageGeom.getCoordsf(k, j, i)`); the fix was never applied to the two-dimensional branch.

The bias has two distinct effects, and the second is the one that is easy to miss:

1. Each diagonal second moment is inflated by `A * E_i^2 / 4`. For a box of `N` cells along an axis the per-cell moment factor becomes `(N^2-1)/12 + 1/4 + 1/16` instead of `(N^2-1)/12 + 1/16`.
2. The first moments no longer vanish, so `sum_cells X*Y = N * (mod_x/2) * (mod_y/2)`, which is **strictly non-zero** for every non-empty two-dimensional **Feature**. The off-diagonal moment `Ixy` is therefore never zero, which tilts the reported principal axis away from the coordinate axes for an axis-aligned **Feature** and — a side effect worth recording — makes the `Ixy == 0` degenerate branch of `findAxisEulers2D` unreachable. That branch is where SIMPLNX had mis-transcribed legacy's `k_PiOver2` as `pi/180`; the transcription error was invisible precisely because this defect kept the line from ever running. Evidence class: **analytically derived, then executed** — fixture F5's pre-fix axis Euler angle was `1.5086188`, exactly the general-branch value predicted for `Ixy = -8`.

**Fix:** `Algorithms/ComputeShapes.cpp`, `findMoments2D` now samples the cell center, through the remapped in-plane axis indices introduced by `-D2`:

```
float x = (static_cast<float>(xPoint) * spacing[inPlaneAxis0] + origin[inPlaneAxis0] + 0.5f * spacing[inPlaneAxis0]) * static_cast<float>(m_ScaleFactor);
```

**Verification:**

- RED first: with the pristine algorithm, `OrientationAnalysis::ComputeShapesFilter: 2D Oracle` failed on F5 with `AxisLengths[0] = 2.27894` against the expected `2.2840716`, matching the value predicted from the legacy source to seven digits.
- Mutation-verified: reverting only this fix (`ww_work/ComputeShapes/mutation_transcript.txt`, mutation M6) fails only `2D Oracle` — the pre-existing production-scale exemplar test, the 3D oracle test and the rotated-feature test all still pass, which is the direct proof that the existing suite was blind to this defect.
- The patched legacy build reproduces the fixed SIMPLNX output bit-exactly on all three 2D fixtures.

**Affected users:** anyone who ran `Find Feature Shapes` / `Compute Feature Shapes` on a single-slice `ImageGeometry` in DREAM3D 6.5.171, or in SIMPLNX before this fix. Three-dimensional runs are unaffected by this entry (see `-D3` for those).

**Recommendation:** trust SIMPLNX at or after this fix. Two-dimensional shape statistics produced by 6.5.171 or by earlier SIMPLNX builds should be regenerated.

---

## ComputeShapesFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeShapesFilter-D2` |
| **Filter UUID** | `036b17d5-23bb-4a24-9187-c4a8dd918792` |
| **Status** | active (shared bug **fixed in SIMPLNX during this V&V cycle**) |

**Symptom:** For a slab whose flat axis is X or Y — as opposed to Z — every output including `Volumes` was wrong, and wrong by large factors rather than by a bias. On a 1x2x8 X-normal slab of spacing `(0.5, 0.5, 0.25)`, `Volumes` was `4.0` instead of `2.0`, `AxisLengths` was `(3.0762079, 0.5057254, 0)` instead of `(1.1490221, 0.5573576, 0)`, and `AspectRatios[0]` was `0.1643990` instead of `0.4850713` — a factor of nearly three on the major semi-axis. Z-normal slabs, which is what almost every real dataset produces, were unaffected by this entry.

**Root cause:** Bug, shared. `findMoments2D` correctly picks its two loop extents from whichever dimension is 1:

```
if(imageGeom.getNumXCells() == 1) { xPoints = getNumYCells(); yPoints = getNumZCells(); }
```

but then read every other in-plane quantity as if the in-plane axes were always X and Y: `modXRes`/`modYRes` from `spacing[0]`/`spacing[1]`, the origin from `origin[0]`/`origin[1]`, the `Centroids` from components `0` and `1`, and the area constant as `konst2 = spacing[0] * spacing[1]`. For an X-normal slab the moments were therefore taken about a point unrelated to the **Feature** centroid, and the reported area was off by the ratio of two spacings. `findAxes2D` repeated the same assumption in its degenerate-moment fallback (`tempScale1`/`tempScale2` from `spacing[0]`/`spacing[1]`). Legacy `FindShapes.cpp:490-491, 508-521, 532` and `:639-646` are the same code; note in particular that legacy's three `if` blocks all assign the identical `imageGeom->getSpacing()`, which is the visible fossil of a remap that was never written.

This is the only cause of a `Volumes` divergence anywhere in the comparison. `Volumes` is otherwise bit-exact, which is what makes it a useful sanity anchor.

**Fix:** `Algorithms/ComputeShapes.cpp` — `findMoments2D` and `findAxes2D` now each derive `inPlaneAxis0` and `inPlaneAxis1` with the same sequential-`if` structure that selects `xPoints`/`yPoints` (so that the "last match wins" behaviour is preserved for geometries flat along more than one axis), and read spacing, origin, `Centroids` components and `konst2` through those indices. Note that `findAxes2D`'s remapped `tempScale1`/`tempScale2` fallback lives in the `r2 <= 0` branch, which the new empty-feature guard at `:597` (`volumes[i] == 0.0f`, added this pass — see Bug Fixes in the V&V report) now intercepts before it is reached: `r2 <= 0` is otherwise unreachable for any feature with cells, since the moment matrix of a positive-volume feature is positive definite (coverage row 22). The remap there is delivered as a defensive fix but is not exercised by any current test.

**Mutation caveat:** because the `findAxes2D` fallback remap is unreachable for real features, mutation M7 (reverting the axis remap) can only discriminate `findMoments2D`'s remap in practice — it cannot, on its own, prove the `findAxes2D` fallback remap correct or even reached.

**Verification:**

- RED first: `2D Oracle` failed on F6a and F6b on `Volumes` (`4.0` against the expected `2.0`) before anything else, then on all three shape arrays, with every observed value matching the source-derived prediction in `ww_work/ComputeShapes/predictions.txt`.
- Mutation-verified: reverting only the axis remap (mutation M7) fails only `2D Oracle`.
- The patched legacy build reproduces the fixed SIMPLNX output bit-exactly on F6a and F6b, `Volumes` included.

**Affected users:** anyone who ran the filter on an X-normal or Y-normal single-slice geometry. This is rare in practice — EBSD montages and sectioned datasets are almost always Z-normal — which is why the defect survived in both codebases.

**Recommendation:** trust SIMPLNX at or after this fix.

---

## ComputeShapesFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeShapesFilter-D3` |
| **Filter UUID** | `036b17d5-23bb-4a24-9187-c4a8dd918792` |
| **Status** | active (defect present only in DREAM3D 6.5.171; SIMPLNX has been correct since issue #1124) |

**Symptom:** `Omega3s`, `AxisLengths`, `AspectRatios` and `AxisEulerAngles` differ between 6.5.171 and SIMPLNX for **every** three-dimensional **Feature**, with no exceptions and no tolerance under which they agree. The difference is a fraction of a percent on large **Features** and very large on small ones. Measured on the V&V fixtures:

| Fixture | 6.5.171 | SIMPLNX | max rel. difference |
|---|---|---|---|
| F1 (8x4x2, spacing 0.75/0.5/0.25) `Omega3s` | 0.42064863 | 0.85708857 | 0.509 |
| F1 `AxisLengths` | (3.6295013, 1.2804039, 0.36650914) | (3.8062587, 1.2612679, 0.30771723) | 0.160 |
| F1 `AspectRatios` | (0.3527768, 0.10098057) | (0.3313668, 0.080845065) | 0.199 |
| F1 short-axis direction | — | — | \|dot\| = 0.996 |
| F3 (single voxel) `Omega3s` | 0.14365785 | 1.0 (clamped) | 0.856 |
| F3 `AspectRatios` | (0.2550313, 0.1178054) | (0.5, 0.25) | 0.529 |
| F8 feature 1 (2x4x2) `AxisLengths` | (0.77814364, 0.336905, 0.28480598) | (0.61917776, 0.31723422, 0.30958888) | 0.275 |
| F8 feature 1 `Omega3s` | 0.32634357 | 0.91065395 | 0.810 |
| F8 feature 1 short-axis direction | — | — | \|dot\| = 0.486 |
| 256x128x64, spacing 0.75/0.5/0.25 `Omega3s` | 0.787180 | 0.787937 | 9.6e-4 |

All rows except the last are **executed** values from `ww_work/ComputeShapes/results_compare.txt`, and every one of them was predicted from the two sources to the same digits before the run (`predictions.txt`). The last row is **source-derived**, evaluated from the closed form rather than run, and is the case 6.5.171's own unit test asserts (`FindShapesTest.cpp:288`, `0.78715 +/- 0.0001`); it is included to show that on a large **Feature** the defect is small enough to have passed unnoticed for years — the same closed form gives `AxisLengths` of `(121.0075, 40.3384, 10.0871)` for the corner convention against `(121.0163, 40.3385, 10.0844)` for the center convention, well inside that test's `+/- 1.5`.

The axis **directions** also differ, which is the part that is easy to overlook. Because the bias makes the off-diagonal moments non-zero, 6.5.171 reports principal axes that are *tilted* relative to the coordinate axes even for a perfectly axis-aligned box. On F8 feature 1 the shortest-axis direction reported by 6.5.171 has `|dot| = 0.486` with the true shortest axis — better than 60 degrees off.

**Root cause:** Bug, in 6.5.171. `find_moments` (`FindShapes.cpp:313-315`) samples the voxel corner, `origin + k * spacing`, while `FindFeatureCentroids` (`:175`) averages voxel centers. Identical mechanism to `-D1`, applied to the three-dimensional branch. SIMPLNX corrects it by calling `imageGeom.getCoordsf(k, j, i)` (`Algorithms/ComputeShapes.cpp:218`).

**Verification:** three independent lines of evidence.

1. The analytic closed form derived for this V&V (see the report's Oracle section) reproduces all fifteen numbers asserted by 6.5.171's own unit test when evaluated with the **corner** convention, and reproduces SIMPLNX's output when evaluated with the **center** convention. That is a two-sided proof that the sample-point convention is the sole difference.
2. The `6_6_stats_test_v2` exemplar archive was generated by a **later** legacy build (`FilterVersion 6.6.373` for its `FindShapes` step) on an 80x80x117 Small IN100 crop with 620 features. Its `Omega3s`, `AxisLengths`, `AspectRatios` and `Shape Volumes` are **bit-identical** to SIMPLNX's output across all 620 features, and its `AxisEulerAngles` agree to `4.8e-7`. Since the corner convention differs from the center convention by 5% to 50% relative on features that size, the 6.6.373 build must already sample centers — so this defect is specific to the 6.5.x line and the later legacy line agrees with SIMPLNX. Evidence class: **executed** (`ww_work/ComputeShapes/results_archive_check.txt`); the 6.6.373 source was not available for inspection.
3. The patched-legacy alignment run below. The patched legacy build — which changes only the sample point in `find_moments`, plus the two-dimensional corrections of `-D1`/`-D2` — reproduces SIMPLNX's output on all six three-dimensional fixtures, bit-exact on four (F2, F3, F4, F8) and to within `3.9e-6` relative on F1 and `2.3e-7` on F7.

**Affected users:** every 6.5.171 user of `Find Feature Shapes` on a three-dimensional volume. Users of the later 6.6 legacy line are **not** affected — see verification item 2. Practically, morphology statistics fed into synthetic-microstructure generation carry a systematic bias toward larger, rounder **Features**; the effect is negligible for **Features** spanning tens of cells and dominant for **Features** spanning one or two.

**Recommendation:** **trust SIMPLNX.** The corner convention is not a defensible modelling choice — it measures each cell's contribution about a point that is not the **Feature** centroid, which violates the definition of a central moment and is why the off-diagonal terms fail to vanish for a symmetric shape.

---

## ComputeShapesFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeShapesFilter-D4` |
| **Filter UUID** | `036b17d5-23bb-4a24-9187-c4a8dd918792` |
| **Status** | active, **documented and deliberately not changed** (shared latent issue; any change alters reported values) |

**Symptom:** The `AxisEulerAngles` reported for a three-dimensional **Feature** are not guaranteed to describe a right-handed frame, and the sign of each principal axis is arbitrary. Two builds of the same code, or the same build on two platforms, may report Euler triples that differ by pi in one or more components for the same **Feature** while describing the same set of axis *directions*. Consumers that compare `AxisEulerAngles` component by component will see spurious differences.

**Root cause:** Shared latent bug. Both codebases assemble a matrix from the three eigenvectors and then attempt a handedness correction:

```
ebsdlib::ResultType result = g.isValid();
if(result.result == 0)
{
  g[6] *= -1.0f; g[7] *= -1.0f; g[8] *= -1.0f;
}
```

`OrientationMatrix::isValid()` sets `res.result = 1` and returns `-1`, `-2` or `-3` on failure; it has no path that returns `0` (`EbsdLib/Orientation/OrientationMatrix.hpp:166-192`). Legacy's `FOrientTransformsType::om_check` is the same function with the same three failure codes (`OrientationTransforms.hpp:505-556`). The correction is therefore dead code in both, and `Eigen::EigenSolver` gives no guarantee about eigenvector sign, so the assembled matrix has determinant `+1` or `-1` with no control. Evidence class: **source-derived**, from the return statements of both routines.

A partial mitigation is inherent in the conversion: `om2eu` reads only `om[2]`, `om[5]` and `om[6..8]`, so the **third row** of the matrix — the direction of the shortest semi-axis — always survives the round trip through the Euler angles, even for an improper matrix. Rows 0 and 1, the long and intermediate axes, do not.

**Why it is not fixed:** repairing the handedness test would change the reported `AxisEulerAngles` for some fraction of **Features** in every existing dataset, including the shipping Small IN100 exemplars. That is a value-changing behavioural change with no correctness argument attached to any particular choice of sign convention, so it belongs in its own scoped change with its own regeneration of exemplars, not folded into this V&V pass.

**Affected users:** anyone treating `AxisEulerAngles` as a stable, comparable orientation rather than as an axis frame defined up to sign. The `Axis Lengths` and `Aspect Ratios` are unaffected — they depend only on the eigenvalues.

**Recommendation:** either acceptable within tolerance, with a usage constraint: compare axis **directions**, for example via the absolute value of a dot product between reconstructed axis vectors, and prefer the shortest-axis direction (the third row of the reconstructed matrix) because it is the one that round-trips reliably. This constraint is now stated in the user documentation. The V&V test suite follows it: `OrientationAnalysis::ComputeShapesFilter: Rotated Feature` asserts `|dot(row2, expected_short_axis)| == 1` and asserts nothing about the Euler components.

---

## Confirmed non-deviations

### The `pi/2` in the two-dimensional degenerate axis-Euler branch is correct in 6.5.171

`find_axiseulers2D` writes `SIMPLib::Constants::k_PiOver2` when `Ixy == 0 && Ixx > Iyy`. That is geometrically right: the in-plane long axis is the local Y axis, ninety degrees from the local X axis. SIMPLNX had `nx::core::numbers::pi / 180.0F` — a ninety-fold mis-transcription that is dimensionally a degree-to-radian factor, not an angle. This is an **NX-only port regression**, fixed this pass, and therefore not a deviation from 6.5.171; it is recorded in the V&V report's Bug Fixes section. The legacy line is left alone in the alignment patch.

Fixture F6b happens to be the one case where stock 6.5.171 and stock SIMPLNX both reach this branch — the wrong `Centroids` component that `-D2` causes 6.5.171 to read lands, by coincidence, on the local-x index midpoint, so its `Ixy` cancels to `-0.0`. Stock 6.5.171 emitted `1.5707964` there and stock SIMPLNX emitted `0.0174533`. That is the executed evidence for the regression.

### Empty feature ids: SIMPLNX now writes zeros where 6.5.171 writes NaN

For a feature id that no cell references, `find_axes` computes `pow((A*A*A*A)/(B*C), 0.1)` with `A == B == C == 0`, a `0/0`. 6.5.171 leaves `NaN` in all three `AxisLengths` components, and `find_axiseulers` reports the arbitrary eigenvector basis of the zero matrix as a real orientation. Observed on fixture F2 feature 3: 6.5.171 gave `AxisLengths = (nan, nan, nan)` and `AxisEulerAngles = (3.1415925, 1.5707964, 1.5707964)`; in the two-dimensional path (fixture F5 feature 2) it gave `AspectRatios = (1.0, 0.0)`, the ratio of two spacings.

SIMPLNX now writes zeros for all axis outputs of a feature with no cells (requester decision, 2026-08-20), consistent with the zero-initialized state of the output arrays. This is a strict improvement rather than a behavioural difference users would migrate across, so it is not given a deviation ID; it is documented in the user-facing filter documentation and it is deliberately **excluded** from the alignment patch, which is why the two empty features are the only rows on which the patched legacy build and SIMPLNX still differ.

### `Volumes` and `Centroids` agree

`Centroids` is bit-exact on all nine fixtures: `ComputeFeatureCentroids` and `FindFeatureCentroids` both average voxel centers with Kahan compensated summation and are genuinely the same algorithm. `Volumes` is bit-exact on all nine except the two X-/Y-normal slabs, where the divergence is entirely `-D2`. These two arrays are the harness sanity anchors: had either differed unexpectedly, the comparison itself would have been suspect.

### `Omega3s` in two dimensions is zero in both codebases

`findMoments2D` and `find_moments2D` never write `omega3s`, so the array keeps whatever the output-array creation left in it. In SIMPLNX that is zero, verified two ways: source-derived, `Generic/CoreDataIOManager.cpp` passes `0.0f` as the `DataStore<float32>` initialization value for every `float32` array regardless of the `CreateArrayAction` fill string, which is empty here; and executed, the 2D oracle test asserts `Omega3s == 0` to `1e-12` absolute for every feature of F5, F6a and F6b. Legacy's `createNonPrereqArrayFromPath` likewise zero-initializes. No deviation; the user documentation now states that Omega3 is not computed in two dimensions rather than leaving a silent zero.
