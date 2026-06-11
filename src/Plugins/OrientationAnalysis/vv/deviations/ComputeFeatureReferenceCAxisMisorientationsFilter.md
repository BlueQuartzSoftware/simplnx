# Deviations from DREAM3D 6.5.171: ComputeFeatureReferenceCAxisMisorientationsFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent (`FindFeatureReferenceCAxisMisorientations`, source at `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindFeatureReferenceCAxisMisorientations.{h,cpp}` in DREAM3D 6.5.171).

Entries are referenced by stable ID (`ComputeFeatureReferenceCAxisMisorientationsFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

## Comparison summary

The legacy A/B comparison was performed **empirically** on 2026-06-10 against three binaries:

- **A:** DREAM3D 6.5.171 (official release, `/Users/mjackson/Applications/DREAM3D.app/Contents/bin/PipelineRunner`).
- **B:** DREAM3D 6.5.172 (A non-public branch with local fixes to prove deviation from legacy simplnx).
- **C:** SIMPLNX.

All three binaries were run on the same hand-built `.dream3d` input file containing the realistic-microstructure fixture (5×5×1 ImageGeom, 5 features incl. one all-cubic F3, pure-Φ Bunge ZXZ rotations matching the SIMPLNX `AnalyticalFixtures::BuildRealisticMicrostructure()` helper). A/B test workspace and artifacts at `/Users/mjackson/Desktop/FRCAM_AB_Test/`.

**Result summary:**

- **6.5.172 ≡ SIMPLNX byte-for-byte** across `FeatureReferenceCAxisMisorientations` (25 cells), `FeatureAvgCAxisMisorientations` (6 features), and `FeatureStdevCAxisMisorientations` (6 features). The two pre-existing 6.5.172 backport commits (`d4b5509aa ENH: FindFeatureReferenceCAxisMisorientations - Update to use Eigen` + `4435d1997 BUG: ... StdDev double precision`) were sufficient.
- **6.5.171 produces materially-different output** in two distinct ways — D1 (non-hex feature handling) and D4 (EbsdLib precision) — both documented below.

This filter is the c-axis analog of `ComputeFeatureReferenceMisorientationsFilter`. Important distinction: this filter does NOT route through `LaueOps::calculateMisorientation` — it uses Eigen for the c-axis vector math (orientation matrix → c-axis rotation → `arccos(c1 · avgCAxis)` folded to `[0°, 90°]`). The EbsdLib 2.4.1+ `calculateMisorientation` precision improvement that surfaced in F#1/F#2/F#4/F#5 of this V&V cycle does **not** apply here. The precision drift documented under D4 below has a different mechanism: hand-rolled `MatrixMath` + float32 stddev accumulation on the legacy side vs Eigen + double accumulation on the SIMPLNX side.

---

## ComputeFeatureReferenceCAxisMisorientationsFilter-D1

| Field            | Value                                                                                                                                |
|------------------|--------------------------------------------------------------------------------------------------------------------------------------|
| **Deviation ID** | `ComputeFeatureReferenceCAxisMisorientationsFilter-D1`                                                                               |
| **Filter UUID**  | `16c487d2-8f99-4fb5-a4df-d3f70a8e6b25`                                                                                               |
| **Status**       | active (SIMPLNX fixed pre-V&V via PR #1438; legacy 6.5.171 still has the bug — backported to `v6_5_172` branch in commit `d4b5509aa`) |

**Symptom:** Per-cell and per-feature outputs for non-hex features differ between SIMPLNX and DREAM3D 6.5.171. Legacy 6.5.171s c-axis projection treats every cubic cell's quaternion as if it were hex. For a feature whose cells are *all* non-hex, the legacy filter produces `featAvg = 0.0 or whatever-the-projection-yields` instead of the analytically-correct `NaN`. The issue exists in the per-feature population stddev calculation as well.

**Root cause:** **Bug** in legacy DREAM3D 6.5.171.

The legacy code at `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindFeatureReferenceCAxisMisorientations.cpp:281` gates the per-cell math on `if(m_FeatureIds[point] > 0 && m_CellPhases[point] > 0)` — no crystal-structure check. Cubic cells fall through to lines 290-308 which compute `c = R^T · [0,0,1]` (the cell-quat-derived c-axis) and then `arccos(c · avgCAxes[fid])` (against whatever `AvgCAxes` was pre-populated for the feature). For a non-hex feature, the upstream `FindAvgCAxes` would either skip the feature (leaving `AvgCAxes[F] = 0` initialized) or produce arbitrary content; either way the resulting cell-level miso is not meaningful.

SIMPLNX adds the `isHex` gate at `Algorithms/ComputeFeatureReferenceCAxisMisorientations.cpp:124`:
```cpp
const bool isHex = crystalStructureType == ebsdlib::CrystalStructure::Hexagonal_High || crystalStructureType == ebsdlib::CrystalStructure::Hexagonal_Low;
...
if(isHex && cellFeatureId > 0 && cellPhase > 0)
{
  // c-axis projection
}
else
{
  cellRefCAxisMis.setValue(cellIdx, 0.0f);  // explicit zero, skip accumulation
}
```

For a feature with all non-hex cells, `counts[featureId] = 0` after the per-cell loop, the per-feature avg becomes `0.0f / 0 = NaN` (IEEE 754) at line 176, and the per-feature stddev becomes `sqrt(NaN / 0) = NaN` at line 202.

The fix was introduced in SIMPLNX via PR #1438 ("Microtexture related filter cleanup") and backported to `v6_5_172` in commit `d4b5509aa ENH: FindFeatureReferenceCAxisMisorientations - Update to use Eigen` (October 2025).

**Empirical confirmation (2026-06-10 A/B):**

Realistic-microstructure fixture, F3 = all-cubic (5 cells):

| Output                                                  | 6.5.171 (A)  | 6.5.172 (B)  | SIMPLNX (C)  |
|---------------------------------------------------------|--------------|--------------|--------------|
| `FeatureReferenceCAxisMisorientations[cells of F3]`     | 9.999988 each cell  | 0.0 each cell  | 0.0 each cell  |
| `FeatureAvgCAxisMisorientations[F3]`                    | 9.999988             | **NaN**       | **NaN**       |
| `FeatureStdevCAxisMisorientations[F3]`                  | 0.0                  | **NaN**       | **NaN**       |

The 6.5.171 `9.999988°` value is the projection of the cubic cell's identity-Quat-derived c-axis `[0,0,1]` against the test fixture's `AvgCAxes[F3] = [0, sin(10°), cos(10°)]` — i.e., the test fixture happened to pre-populate F3's avg-c-axis at a 10° tilt, and 6.5.171 produces ~10° as the "miso" for each cubic cell. On a real dataset, that value would be whatever-random-content `FindAvgCAxes` left in `AvgCAxes[F3]`.

**Affected users:** Any workflow that runs `FindFeatureReferenceCAxisMisorientations` on mixed-phase datasets in DREAM3D 6.5.171. Single-phase hex-only datasets are unaffected. The garbage values propagate into downstream statistics (per-feature average + stddev rows for non-hex features) but cell-level visualization may not surface the issue immediately.

**Recommendation:** **Trust SIMPLNX (or 6.5.172).** The legacy 6.5.171 output for non-hex features is mathematically meaningless — the c-axis is a hex-specific concept and asking "how far is this cubic cell's c-axis from the feature's avg c-axis" has no defined answer. The `NaN` produced by SIMPLNX is the correct signal that the question is ill-posed for the feature.

---

## ComputeFeatureReferenceCAxisMisorientationsFilter-D2

| Field            | Value                                                                                                                                |
|------------------|--------------------------------------------------------------------------------------------------------------------------------------|
| **Deviation ID** | `ComputeFeatureReferenceCAxisMisorientationsFilter-D2`                                                                               |
| **Filter UUID**  | `16c487d2-8f99-4fb5-a4df-d3f70a8e6b25`                                                                                               |
| **Status**       | active (SIMPLNX uses Eigen + double; legacy 6.5.171 uses hand-rolled MatrixMath + float32 — backported to `v6_5_172` in commits `d4b5509aa` + `4435d1997`) |

**Symptom:** Per-cell `FeatureReferenceCAxisMisorientations` and per-feature `FeatureStdevCAxisMisorientations` values drift between SIMPLNX and DREAM3D 6.5.171 by `~1e-4°` to `~1e-6°`, with the larger magnitude appearing on per-cell values for cells whose Φ differs from the feature's avg Φ. The drift is precision-class, not algorithmic; both implementations compute the same analytical reduction.

**Root cause:** **Precision + library.** Two stacked contributions:

1. **Orientation-matrix math.** Legacy uses `MatrixMath::Transpose3x3` + `MatrixMath::Multiply3x3with3x1` + `MatrixMath::Normalize3x1` (all float32). SIMPLNX uses Eigen's `Eigen::Matrix3d.transpose()` + `Eigen::Vector3d` arithmetic (double precision throughout). The hand-rolled matrix math accumulates intermediate float32 round-off that the Eigen path does not.
2. **Standard-deviation accumulation.** Legacy accumulates `(diff * diff)` in a `std::vector<float32>` and divides by `static_cast<float32>(counts)`. SIMPLNX uses `std::vector<double>` and `static_cast<double>(counts)`, then casts the final result to float32 only at the output array write. The float32 accumulator on the legacy side lossily rounds each per-cell contribution.

The SIMPLNX-side fix has been present since the initial port (no SIMPLNX-side code change required for this V&V cycle). The legacy fix is split across two backport commits on `v6_5_172`:

- `d4b5509aa ENH: FindFeatureReferenceCAxisMisorientations - Update to use Eigen` (cherry-picked October 2025) — replaces `MatrixMath::*` with Eigen for the per-cell orientation-matrix → c-axis pipeline.
- `4435d1997 BUG: FindFeatureReferenceCAxisOrientation - Use doubles to accumulate the StdDev values` (cherry-picked October 2025) — promotes the stddev accumulator from `std::vector<float32>` to `std::vector<double>` and uses `double` for `diff` and the final division.

With both backports applied, 6.5.172 produces byte-identical output to SIMPLNX across all 3 output arrays on the FRCAM A/B fixture.

**Empirical confirmation (2026-06-10 A/B):**

Per-cell `FeatureReferenceCAxisMisorientations`, sample of hex-feature cells (F2 row, cell Φ = 8°, 9°, 10°, 11°, 12°, avg Φ = 10°):

| Cell | Expected | 6.5.171 (A)  | 6.5.172 (B)  | SIMPLNX (C)  |
|------|----------|--------------|--------------|--------------|
| F2[0] | 2.0  | 1.999782   | 2.000001   | 2.000001   |
| F2[1] | 1.0  | 0.999755   | 1.000000   | 1.000000   |
| F2[2] | 0.0  | 0.000000   | 0.000001   | 0.000001   |
| F2[3] | 1.0  | 0.999755   | 1.000000   | 1.000000   |
| F2[4] | 2.0  | 1.999880   | 2.000000   | 2.000000   |

Per-feature `FeatureAvgCAxisMisorientations`:

| Feature | Expected  | 6.5.171 (A)  | 6.5.172 (B) ≡ SIMPLNX (C) |
|---------|-----------|--------------|---------------------------|
| F1      | 0.0       | 0.000000     | 0.000000                  |
| F2      | 1.2       | 1.199834     | 1.200000                  |
| F4      | 0.0       | 0.000000     | 0.000000                  |
| F5      | 2.8       | 2.800004     | 2.800000                  |

Per-feature `FeatureStdevCAxisMisorientations`:

| Feature | Expected     | 6.5.171 (A)  | 6.5.172 (B) ≡ SIMPLNX (C) |
|---------|--------------|--------------|---------------------------|
| F2      | √0.56 ≈ 0.7483 | 0.748285   | 0.748331                  |
| F5      | √3.76 ≈ 1.9391 | 1.939061   | 1.939072                  |

**Affected users:** Any workflow that runs `FindFeatureReferenceCAxisMisorientations` on 6.5.171 and compares its output against SIMPLNX or 6.5.172. The drift magnitude (~1e-4° per cell, ~1e-5° per feature avg) is sub-perceptual for visualization but visible in numerical analysis or unit-test comparisons. Real-world EBSD datasets with many cells per feature will see the per-feature avg drift become more stable (the float32-noise per-cell contributions average toward the analytical mean), but the per-cell values themselves still show the precision-class drift.

**Recommendation:** **Trust SIMPLNX (or 6.5.172).** The SIMPLNX value is closer to the analytical oracle. The 6.5.171 value differs by precision-class round-off, not by an algorithmic choice.
