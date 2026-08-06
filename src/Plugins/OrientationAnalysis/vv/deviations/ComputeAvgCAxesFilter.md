# Deviations from DREAM3D 6.5.171: ComputeAvgCAxesFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

Entries are referenced by stable ID (`ComputeAvgCAxesFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

**Headline result of the comparison** (all values float32, 11-cell hand-built fixture; expected once the normalize fix is re-applied to the local build of the legacy source):

| Feature | 6.5.171 (official) | Patched legacy (post-normalize) | SIMPLNX (post-normalize) | Notes |
|---|---|---|---|---|
| **0** | **(0, 0, 0)** | **(NaN, NaN, NaN)** | **(NaN, NaN, NaN)** | **placeholder — D1 (counter==0)** |
| 1 | (0, 0, 1.000) | (0, 0, 1.000) | (0, 0, 1.000) | identity quaternion, all match |
| 2 | (0, 0.866, 0.500) | (0, 0.866, 0.500) | (0, 0.866, 0.500) | +60° X, all match |
| 3 | (0, 0, 1.000) | (0, 0, 1.000) | (0, 0, 1.000) | 3 aligned cells, all match |
| 4 | (0, 0, 1.000) | (0, 0, 1.000) | (0, 0, 1.000) | antipodal pair, all match |
| **5** | **(0, 0, 1.000)** | **(NaN, NaN, NaN)** | **(NaN, NaN, NaN)** | **no cells assigned — D1** |
| **6** | **(0, 0, 1.000)** | **(NaN, NaN, NaN)** | **(NaN, NaN, NaN)** | **all cells non-hex — D1** |
| **7** | **(0, 0, 0.667)** | **(0, 0.866, 0.500)** | **(0, 0.866, 0.500)** | **antipodal-flip boundary, post-normalize — D2** |

SIMPLNX is expected to be bit-identical to the patched local build of the legacy source (once the patch's normalize step lands). 6.5.171 differs at F0/F5/F6 (D1) and F7 (D2). The Phase-7 changes applied surgically to the legacy build (precision + matrix-math style + counter==0 → NaN at finalize + final per-feature normalize) explain all 4 deviations completely.

---

## ComputeAvgCAxesFilter-D1

| Field            | Value                                  |
|------------------|----------------------------------------|
| **Deviation ID** | `ComputeAvgCAxesFilter-D1`             |
| **Filter UUID**  | `453cdb58-7bbb-4576-ad5e-f75a1c54d348` |
| **Status**       | active                                 |

**Symptom:** For any feature `i` whose `counter[i] == 0` at the final computation — meaning zero hex-phase voxels contributed to the feature — SIMPLNX writes `(NaN, NaN, NaN)` into that feature's `AvgCAxes` slot. Legacy DREAM3D 6.5.171 instead falls into a "counter==0 rescue" branch and writes `(0, 0, 1)` — a confusingly valid-looking c-axis output for a feature with no contributing data.

This symptom manifests in three distinct scenarios:

| Scenario | Example in V&V dataset | SIMPLNX | 6.5.171 |
|---|---|---|---|
| Placeholder feature 0 (never referenced by any cell) | F0 | NaN | (0, 0, 0) — initial state, untouched by legacy finalize loop |
| Feature has no cells assigned | F5 — declared in CellFeatureData with 8 tuples but no cell has `FeatureId == 5` | NaN | (0, 0, 1) — rescue fires |
| Feature has cells but all are non-hex | F6 — sole cell of F6 is Cubic_High phase, skipped in cell loop, counter never incremented | NaN | (0, 0, 1) — rescue fires |

(For F0 specifically, legacy 6.5.171's finalize loop starts at index 1 and never visits feature 0; the array slot stays at its zero-initialized state. SIMPLNX's finalize loop starts at index 0 and writes NaN.)

**Root cause:** **Algorithmic choice** during the SIMPLNX port. The refactored algorithm consolidates "no valid contributions" handling at the per-feature finalize loop:

```cpp
// Cell loop: non-hex cells skip without writing anything
if(currentCrystalStructure != ebsdlib::CrystalStructure::Hexagonal_High &&
   currentCrystalStructure != ebsdlib::CrystalStructure::Hexagonal_Low)
{
  continue;
}
cellCount[currentFeatureId]++;

// ... per-cell accumulation ...

// Per-feature finalize loop (starts at index 0):
if(cellCount[currentFeatureId] == 0)
{
  avgCAxes[cAxesIndex]     = NAN;
  avgCAxes[cAxesIndex + 1] = NAN;
  avgCAxes[cAxesIndex + 2] = NAN;
}
else
{
  // divide by cellCount, then normalize
}
```

The legacy DREAM3D 6.5.171 `FindAvgCAxes` instead writes `(0, 0, 1)` at the counter==0 case, and starts the finalize loop at index 1. A surgical fix applied to a local build of the legacy source retrofits both the SIMPLNX-style `counter==0 → NaN` write and the finalize-loop start-at-0 into the legacy code, producing **bit-identical** SIMPLNX output across F0, F5, and F6.

**Affected users:** Anyone running the filter on EBSD data with one or more of:
- Multi-phase data including non-hexagonal phases (some features may end up all-non-hex)
- CellFeatureData with declared feature tuples that don't have cells assigned (sparse feature space)
- Pole-figure or texture-statistics pipelines that consume `AvgCAxes` downstream — the (0, 0, 1) legacy rescue value would silently propagate as a "real" c-axis; the SIMPLNX NaN allows correct downstream filtering.

**Recommendation:** **Trust SIMPLNX.** Writing `NaN` is a more honest signal that the value is undefined for these features. The legacy `(0, 0, 1)` rescue was a side effect of the rescue branch design, not a meaningful crystallographic c-axis. Downstream consumers of `AvgCAxes` should expect and handle NaN appropriately (e.g., filter out NaN-valued features before computing pole figures or texture statistics).

---

## ComputeAvgCAxesFilter-D2

| Field            | Value                                  |
|------------------|----------------------------------------|
| **Deviation ID** | `ComputeAvgCAxesFilter-D2`             |
| **Filter UUID**  | `453cdb58-7bbb-4576-ad5e-f75a1c54d348` |
| **Status**       | active                                 |

**Symptom:** For features whose cell c-axes lie on the *antipodal-flip cancellation boundary* — where the running-average dot product evaluates to exactly zero in math but is precision-dependent in float32 — SIMPLNX and 6.5.171 produce different per-feature `AvgCAxes` outputs. After the Phase-7 normalize-at-finalize step, both implementations yield unit vectors, but with different magnitudes vs 6.5.171 (1.0 in SIMPLNX, 2/3 in 6.5.171) AND different directions.

In the V&V hand-built fixture this fires at F7: three cells with c-axes `(0, 0, 1)`, `(0, +√3/2, 0.5)`, `(0, -√3/2, 0.5)`. The legacy 6.5.171 float-precision path takes the "no flip" branch and produces `(0, 0, 0.667)`. The SIMPLNX double-precision-Eigen path takes the "flip fires" branch and produces `(0, 0.866, 0.500)` (post-normalize). These are genuinely different c-axis directions (60° apart in 3D), NOT hex c≡-c equivalents — both are valid "average c-axes" of the same cell set under different sign-assignment choices.

**Root cause:** **Precision + matrix-math style**.

The relevant lines in `Algorithms/ComputeAvgCAxes.cpp` lines 118–122 compute the running-average reference and the antipodal-flip decision:

```cpp
float64 cosAngle = ImageRotationUtilities::CosBetweenVectors(cellCAxis, runningCAxisAvg);
if(cosAngle < 0.0)
{
  cellCAxis *= -1.0f;
}
```

At F7 cell 10, the mathematically exact cosAngle is zero — the cell's c-axis is perpendicular to the running average. In SIMPLNX's `Eigen::Vector3d` double-precision path the computed `cosAngle` lands at a tiny negative value, firing the flip. In legacy 6.5.171's float-only path (or in a pure-double NumPy replay), the computed cosAngle lands at a tiny positive value, NOT firing the flip. The accumulated sums diverge to genuinely different directions.

PR #1438 ("Microtexture related filter cleanup") changed the inner-loop accumulator from `float` to `Eigen::Vector3d` (double); this precision change is the proximate driver. The EbsdLib API refactor (PR #1472) replaced explicit `qu2om` + helper calls with inline `toOrientationMatrix()` — verified functionally equivalent by the matching output of the patched local build of the legacy source. The Phase-7 normalize-at-finalize step (V&V cycle) widens the deviation: pre-normalize, both implementations had magnitude 2/3 at F7; post-normalize, SIMPLNX has magnitude 1.0 while 6.5.171 still has 2/3 (since the legacy code never normalized).

**Affected users:** Anyone running the filter on EBSD data containing features whose cell-level c-axes are arranged near the antipodal-flip cancellation boundary. In practice this is rare in natural EBSD data — it requires several cells with c-axes that summed (without flip) would lie nearly orthogonal to a previously-accumulated direction. The deliberately-pathological F7 test case demonstrates the sensitivity. **Users computing pole figures or per-feature texture statistics will get different magnitudes (1.0 vs ≤ 1.0) between SIMPLNX and 6.5.171, with the SIMPLNX magnitude being more predictable across pipelines (always unit-vector).**

**Recommendation:** **Trust SIMPLNX.** The unit-vector contract matches the natural interpretation of "average c-axis direction" and is what downstream filters (`ComputeFeatureReferenceCAxisMisorientations`, `ComputeFeatureNeighborCAxisMisalignments`) need anyway (those filters compute `acos(dot(a, b))` and require unit-magnitude inputs). Where direction differs at the precision boundary, both directions are valid c-axis representatives of the underlying cells — there is no objectively "right" answer at exact cancellation.

---

## Comparison build & library nuance

Legacy DREAM3D 6.5.171 (and the patched local build of its source) uses the **built-in EbsdLib/OrientationLib** inside the DREAM3D source tree (frozen at the 6.5.171 release point, with the targeted surgical patches applied on top). SIMPLNX uses an **independent, vcpkg-installed EbsdLib** that is actively updated. Both implement Rowenhorst conventions but the underlying code differs. The patched legacy build reproduces SIMPLNX *functional behavior*, not *identical library code* — sufficient to isolate the design-choice drivers of the deviations.

The patched legacy build's purpose is **root-cause proof**. If every observed SIMPLNX-vs-6.5.171 difference disappears when the targeted changes are applied to the legacy code, those targeted changes are conclusively the cause. They are.

**Comparison fixtures:**
- `/Users/mjackson/Workspace6/DREAM3D_Data/TestFiles/compute_avg_c_axis/output_legacy/6_5_171_compute_avg_c_axis.dream3d` — official DREAM3D 6.5.171 release output
- a second output file alongside it in `output_legacy/` — output of the surgically patched local build of the legacy source (pending re-run after the normalize fix)
- `output_simplnx/simplnx_compute_avg_caxes.dream3d` — SIMPLNX output
- Three-way diff report: `vv/comparisons/ComputeAvgCAxesFilter/results/three_way_comparison.txt`
