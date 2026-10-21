# Deviations from DREAM3D 6.5.171: ComputeFeatureFaceMisorientationFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent (`GenerateFaceMisorientationColoring`, source at `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/GenerateFaceMisorientationColoring.{h,cpp}` in DREAM3D 6.5.171).

Entries are referenced by stable ID (`ComputeFeatureFaceMisorientations-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

All four entries below are **deliberate design changes** made during the rewrite (Nathan Young, 2026-05-19), not bugs in either implementation. D4 additionally documents a related precision fix made in EbsdLib during this V&V cycle (2026-05-28).

---

## ComputeFeatureFaceMisorientations-D1

| Field            | Value                                  |
|------------------|----------------------------------------|
| **Deviation ID** | `ComputeFeatureFaceMisorientations-D1` |
| **Filter UUID**  | `f3473af9-db77-43db-bd25-60df7230ea73` |
| **Status**       | active                                 |

**Symptom:** For faces between two features whose shared phase has a Laue class other than `Hexagonal_High` (m-3m) or `Cubic_High` (6/mmm), SIMPLNX computes a real misorientation value; legacy DREAM3D 6.5.171 wrote `0` (implicit, via the fall-through to the else branch). The nine "new" Laue classes that SIMPLNX now handles are: `Hexagonal_Low` (6/m), `Cubic_Low` (m-3), `Triclinic` (-1), `Monoclinic` (2/m), `OrthoRhombic` (mmm), `Tetragonal_Low` (4/m), `Tetragonal_High` (4/mmm), `Trigonal_Low` (-3), and `Trigonal_High` (-3m).

**Root cause:** **Library** + **algorithmic choice**. Legacy `GenerateFaceMisorientationColoring.cpp` line 127 explicitly checks `if((m_CrystalStructures[phase1] == Ebsd::CrystalStructure::Hexagonal_High) || (m_CrystalStructures[phase1] == Ebsd::CrystalStructure::Cubic_High))` and only computes the misorientation under that guard. The legacy `OrientationLib` of that era did not have `calculateMisorientation` (or its predecessor `getMisoQuat`) implementations for the other nine Laue classes — they would have returned undefined behavior had the guard been removed. The modern EbsdLib (`vcpkg-installed/.../EbsdLib`) has `calculateMisorientation` implementations for every Laue class, so SIMPLNX dispatches by `laueIndex < m_LaueOrientationOps.size()` instead of hard-coding the two-class enumeration.

**Affected users:** Any user with surface-mesh face data spanning grain boundaries between non-hexagonal, non-cubic-high phases. Common cases: orthorhombic systems (alpha-uranium, many minerals), monoclinic systems (gypsum, many metals at low symmetry phases), tetragonal systems (TiO2, ZrO2 below transition), trigonal systems (quartz, alpha-corundum). Users running the legacy filter on these systems received silent zeros for every triangle between matched-phase non-hex-high/non-cubic-high features.

**Recommendation:** **Trust SIMPLNX.** The legacy filter's silent zero for unsupported Laue classes was indistinguishable from "genuine zero misorientation" (a real possibility for aligned grains) — a serious correctness ambiguity that the new explicit handling resolves. Combined with D3 (NaN for invalid faces), users get unambiguous values for every triangle.

---

## ComputeFeatureFaceMisorientations-D2

| Field            | Value                                  |
|------------------|----------------------------------------|
| **Deviation ID** | `ComputeFeatureFaceMisorientations-D2` |
| **Filter UUID**  | `f3473af9-db77-43db-bd25-60df7230ea73` |
| **Status**       | active                                 |

**Symptom:** SIMPLNX output is a 1-component `float32` array — the misorientation angle in degrees per triangle. Legacy DREAM3D 6.5.171 wrote a 3-component `float32` array — the rotation axis (3 components) multiplied componentwise by the angle in degrees, i.e., `(w·n1, w·n2, w·n3)`. The 3-component form encodes both the rotation magnitude AND the axis direction; the 1-component form keeps only the magnitude.

**Root cause:** **Algorithmic choice** during the rewrite. Decision attributed to Michael Jackson: "bring the output in line with other Misorientation filters in `simplnx`". Most modern misorientation filters in simplnx (KAM, Reference Misorientations, Feature Neighbor Misorientations) return a 1-component magnitude; the legacy 3-component axis·angle form was the outlier. The new form also halves the output memory footprint and simplifies most downstream uses (binning, histogram, threshold).

**Affected users:** Any user porting a 6.5.171 pipeline that consumed the 3-component output. Common downstream uses:
- **Magnitude-only consumers** (binning for histograms, comparison against a threshold): trivial migration — read component 0 (which used to be `w·n1`) → must change to read the new 1-component (now just `w`). Many pipelines did this by extracting component 0 anyway, which would have been mathematically incorrect for the old format.
- **Axis-and-angle consumers** (visualization with the axis direction encoded in color, downstream filters that need the misorientation rotation axis): not a clean migration — they need the axis explicitly. Workaround: regenerate the axis-angle by running `LaueOps::calculateMisorientation` directly in a custom filter; or back-out the axis from the (legacy) 3-component output by dividing by the angle magnitude.

**Recommendation:** **Either acceptable per use case.** Neither output is wrong, just different representations of the same calculation. For users needing the rotation axis: open an issue requesting a separate axis output array (could be added as a v3 of the filter via the version-bump mechanism).

---

## ComputeFeatureFaceMisorientations-D3

| Field            | Value                                  |
|------------------|----------------------------------------|
| **Deviation ID** | `ComputeFeatureFaceMisorientations-D3` |
| **Filter UUID**  | `f3473af9-db77-43db-bd25-60df7230ea73` |
| **Status**       | active                                 |

**Symptom:** For faces where the algorithm cannot compute a meaningful misorientation — `frontFeature == 0` (background voxel on the front side), `backFeature == 0` (background on the back side), `frontPhase != backPhase` (mixed-phase boundary), or the shared phase's Laue class is out of EbsdLib's supported range — SIMPLNX writes **`NaN`**. Legacy DREAM3D 6.5.171 wrote `0` for all these cases (in the legacy code: explicit zeros for the mismatched-phase case at lines 143–145; implicit zeros for the unsupported-Laue case via fall-through).

**Root cause:** **Bug in 6.5.171** (loose categorization), fixed by **algorithmic choice** in SIMPLNX. In 6.5.171 there is no way to differentiate between a true zero misorientation (two grains in perfect crystallographic alignment, mathematically possible) and an unprocessed face. SIMPLNX's explicit NaN makes the distinction unambiguous:

| Output value | Meaning                                                                  |
|--------------|--------------------------------------------------------------------------|
| `0.0`        | Genuine zero misorientation — the two features are crystallographically aligned (or the misorientation lies on a symmetry op of the shared Laue class) |
| Positive     | Computed misorientation angle in degrees                                 |
| `NaN`        | The algorithm did not compute a value for this face (any of the four reasons above) |

**Affected users:** Anyone running pipelines that aggregated misorientation values across all faces (mean, median, histogram). In the legacy filter, the implicit zeros from unprocessed faces silently dragged the aggregate toward zero. SIMPLNX's NaN propagates correctly under standard aggregation rules (NaN-aware aggregations skip NaN; non-aware aggregations propagate to NaN).

**Recommendation:** **Trust SIMPLNX.** The clear distinction between genuine zero and "no value" is a significant correctness improvement. Downstream consumers should use NaN-aware aggregation (`std::isnan`, `numpy.nanmean`, etc.) or pre-filter faces with NaN before aggregating.

---

## ComputeFeatureFaceMisorientations-D4

| Field            | Value                                  |
|------------------|----------------------------------------|
| **Deviation ID** | `ComputeFeatureFaceMisorientations-D4` |
| **Filter UUID**  | `f3473af9-db77-43db-bd25-60df7230ea73` |
| **Status**       | active                                 |

**Symptom:** For cubic-phase boundaries whose true misorientation lies exactly on a cubic symmetry operator (e.g., 90° rotation about the c-axis is a 4-fold sym op of m-3m), legacy DREAM3D 6.5.171 and pre-fix SIMPLNX returned a small residual misorientation (~0.02°) instead of exactly 0°. Post-fix SIMPLNX returns exactly 0° for these cases.

The hand-built V&V dataset exposes this at F5↔F7 (Cubic_High features with φ1=0° and φ1=90° about c): expected exactly 0° by symmetry; pre-fix observed 0.0212°; post-fix observed 0.0°.

**Root cause:** **Precision** in EbsdLib's `CubicOps::calculateMisorientationInternal` (NOT in this filter's code). The algorithm computed `cos(half-angle)` candidates as:

```cpp
double w_candidate_2 = (qco.z() + qco.w()) / sqrt(2);   // type-2 sym op
```

then extracted the angle via `acos(w_candidate)`. When `AvgQuats` are stored as **float32** in the input data (the standard SIMPLNX/EbsdLib convention) and promoted to double inside the calculation:

1. The float32 truncation of `sqrt(2)/2` is `0.7071068f` ≈ `0.70710676908...` as double (off from true `sqrt(2)/2` by ~6e-8).
2. After the symmetry-op reduction, `qco.z` and `qco.w` end up at this float32 value.
3. `(qco.z + qco.w) / sqrt(2)` (where the divisor is the precise double `sqrt(2)`) computes to `1.0 − ~1.71e-8`, NOT exactly 1.0.
4. `acos(1.0 − 1.71e-8)` is in the catastrophic-cancellation regime: the derivative `−1/sqrt(1−x²)` is unbounded as `x → 1`, so a 1.71e-8 perturbation amplifies to ~`1.85e-4` rad in the result.
5. Doubled to full angle and converted to degrees: `~0.0212°` ≈ the observed residual.

**Fix:** Patch in EbsdLib (`Source/EbsdLib/LaueOps/CubicOps.cpp`). Compute the explicit reduced-quaternion vector components for each of the three sym-op candidates ("type 1/2/3"); pick the candidate with the largest `w`; extract the angle as `2 · atan2(|v|, w)` using `|v|` computed from the explicit reduced-quaternion components, NOT from `sqrt(1 − w²)`. The cancellation that loses precision in the legacy form is *recovered* in the new form because the explicit `v` components include subtractions of identical floats (e.g., `qco.z − qco.w` when `qco.z == qco.w`), which yield exactly 0 in IEEE 754 — regardless of the underlying float32 precision.

The fix is mathematically equivalent for non-sym-op-aligned misorientations (both forms compute the same angle within ULP for inputs far from the cancellation boundary). It strictly improves precision for inputs on or near a sym op.

**Affected users:** Anyone who computes misorientations on cubic phases where some grain boundaries land on or near a 4-fold (90° about c), 3-fold (120° about [111]), 2-fold (180° about face), or other cubic symmetry op. These are not pathological — many real-world cubic textures have these as systematic features (e.g., {100} fiber textures align all grains' [001] directions, so 90° about [001] is a frequent boundary). The 0.02° pre-fix residual would have caused:
- Misorientation histograms to have a spurious peak at ~0.02° that should be at 0°.
- Threshold-based grain boundary classification (e.g., "low-angle boundaries < 5°") to silently misclassify some sym-op-aligned boundaries.
- Downstream KAM and GOS calculations to include the residual.

**Recommendation:** **Trust the fixed SIMPLNX (post-2026-05-28).** The fix is a strict improvement; all 306 EbsdLib unit tests pass post-fix; 181/189 OrientationAnalysis unit tests pass post-fix (the 8 failures are exemplar-based regression tests against pre-fix-generated exemplars, with diffs in the `1e-4` to `1e-3` range — see the V&V doc's "Downstream impact note"). Exemplar files for the 8 affected downstream tests will be regenerated at the engineer's discretion to lock in the post-fix values as the new reference.

---

## Comparison build & library nuance

This filter's V&V did NOT run a direct A/B comparison against legacy DREAM3D 6.5.171's `GenerateFaceMisorientationColoring`. The output structure is incompatible by design (D2: 3-component axis·angle vs 1-component angle), making a per-array comparison meaningless. The Class 1 oracle (hand-derived from symmetry-group analysis) serves as the verification floor; the deviation entries above document the design choices that distinguish the new filter from the legacy.
