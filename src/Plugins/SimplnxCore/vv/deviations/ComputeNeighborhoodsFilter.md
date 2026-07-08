# Deviations from DREAM3D 6.5.171: ComputeNeighborhoodsFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent (`FindNeighborhoods`).

Entries are referenced by stable ID (`ComputeNeighborhoodsFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

Comparison fixture: `6_6_stats_test_v2.dream3d` (Small IN100, 620 features), `MultiplesOfAverage = 1.0`, run through DREAM3D 6.5.171 `PipelineRunner` and DREAM3D-NX `nxrunner`.

---

## ComputeNeighborhoodsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeNeighborhoodsFilter-D1` |
| **Filter UUID** | `924c10e3-2f39-4c08-9d7a-7fe029f74f6d` |
| **Status** | active |

**Symptom:** For the same input and `MultiplesOfAverage` value, SIMPLNX reports fewer neighbors per feature than 6.5.171 — on Small IN100 (mult=1) the mean is 5.55 (SIMPLNX) vs 10.93 (6.5.171), i.e. ~52%, with a per-feature correlation of 0.894.

**Root cause:** *Algorithmic choice.* Both implementations scale each feature's search reach by that feature's own Equivalent Sphere Diameter times the multiplier. They differ in the **shape** of the neighborhood region:

- **6.5.171** compares the per-axis integer bin difference against a per-feature `criticalDistance` (`|Δbinₓ| < cd && |Δbin_y| < cd && |Δbin_z| < cd`), which tests membership in an **axis-aligned box** in a space normalized by the average diameter. This over-counts: it accepts features in the box corners that lie beyond the nominal reach.
- **SIMPLNX** computes the true **Euclidean distance** between centroids and compares it against the radius (`distSq ≤ radius²`), i.e. a **sphere**.

A sphere of radius `r` has ~0.52× the volume of a cube of half-width `r`, which quantitatively explains the ~52% ratio observed. The SIMPLNX result is the geometrically correct interpretation of "features within a search radius."

The comparison operator also differs: 6.5.171 used a **strict** `<` on bin differences, while SIMPLNX uses an **inclusive** `distSq ≤ radius²`, so a centroid lying exactly on the sphere surface is counted as a neighbor. This boundary is measure-zero for real data but is an intentional, pinned choice (see the boundary-inclusion assertion in `test/ComputeNeighborhoodsTest.cpp`).

**Affected users:** Anyone migrating a `FindNeighborhoods` pipeline from 6.5.171 and comparing `Neighborhoods` / `NeighborhoodList` values. Absolute counts will be lower under SIMPLNX; relative structure is preserved (corr 0.894).

**Recommendation:** *Trust SIMPLNX.* The Euclidean sphere is the correct region for a centroid-distance neighborhood; the 6.5.171 box test was a bin-scan approximation. Users needing an exact reproduction of a specific physical search size should use the new "Search Radius (microns)" mode with an explicit radius.

> **Note (SIMPLNX-internal regression, fixed in the V&V PR — not a 6.5.171 deviation):** Between the original NX port and this V&V, PR #1485 correctly switched to a Euclidean distance test but also introduced a `÷2` factor and a single **global** radius (`avgDiameter·mult÷2`) in place of the per-feature radius. At the default `mult=1` this found ~37× fewer neighbors than 6.5.171 (mean 0.29 vs 10.93). The V&V restored the per-feature radius `equivalentDiameter[i]·mult`; the numbers above are post-fix.

---

## ComputeNeighborhoodsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeNeighborhoodsFilter-D2` |
| **Filter UUID** | `924c10e3-2f39-4c08-9d7a-7fe029f74f6d` |
| **Status** | active |

**Symptom:** The SIMPLNX filter has no "Feature Phases" input parameter; 6.5.171 `FindNeighborhoods` required one.

**Root cause:** *Algorithmic choice (API).* `FindNeighborhoods` declared `FeaturePhases` as a required array in `dataCheck()` but never referenced it in the neighbor computation (`execute()` / `FindNeighborhoodsImpl`). SIMPLNX removes the unused parameter. Output arrays are unaffected.

**Affected users:** Anyone converting a 6.5.171/SIMPL pipeline — the `FeaturePhasesArrayPath` key is dropped during conversion. No change to computed `Neighborhoods` / `NeighborhoodList` values.

**Recommendation:** *Trust SIMPLNX.* The removed input had no effect on output; dropping it removes a confusing required selection (and it was never needed by the absolute "Search Radius (microns)" mode either).

---

## ComputeNeighborhoodsFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeNeighborhoodsFilter-D3` |
| **Filter UUID** | `924c10e3-2f39-4c08-9d7a-7fe029f74f6d` |
| **Status** | active |

**Symptom:** SIMPLNX rejects non-positive radius parameters at preflight; 6.5.171 accepted any value. A converted pipeline with `MultiplesOfAverage ≤ 0` now fails preflight with error `-5732` (multiples mode) and a non-positive search radius fails with `-5733` (microns mode).

**Root cause:** *Intentional hardening.* A non-positive multiplier/radius produces an empty or degenerate search region — under 6.5.171 it silently produced all-zero `Neighborhoods` output. SIMPLNX surfaces the misconfiguration at preflight instead of silently returning meaningless results.

**Affected users:** Only pipelines carrying a zero or negative `MultiplesOfAverage` value, which produced no useful output in 6.5.171 anyway.

**Recommendation:** *Trust SIMPLNX.* Set a positive multiplier/radius. Both error paths are pinned by `ComputeNeighborhoods_InvalidSearchRadius`.
