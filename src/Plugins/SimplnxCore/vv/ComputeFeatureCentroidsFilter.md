# V&V Working Document: ComputeFeatureCentroidsFilter

*Status:* **DRAFT — Phases 1–11 complete (oracle verified, 1 bug fixed + pinned, tests/deviations/provenance/docs done). Outstanding: OOC dual-build (deferred), Phase 12 OneDrive archive, Phase 13 tracking promotion, second-engineer oracle review (reviewer to be named).**

This is the engineer's working V&V doc for `ComputeFeatureCentroidsFilter`, produced by the `bluequartz-skills:vv-filter` workflow. It follows the oracle-first V&V policy at [`docs/vv_templates/vv_policy.md`](../../../../docs/vv_templates/vv_policy.md). The document is filled in phase-by-phase and IS the deliverable; when all phases pass their gates the Status line is promoted DRAFT → READY FOR REVIEW → COMPLETE.

**Source-of-truth references** (paths relative to this doc at `src/Plugins/SimplnxCore/vv/`):
- Policy: [`../../../../docs/vv_templates/vv_policy.md`](../../../../docs/vv_templates/vv_policy.md)
- Oracle classes: [`../../../../docs/vv_templates/oracle_classes.md`](../../../../docs/vv_templates/oracle_classes.md)
- Section gates: [`../../../../docs/vv_templates/report_gates.md`](../../../../docs/vv_templates/report_gates.md)
- Format-of-record report: [`../../OrientationAnalysis/vv/ComputeFeatureReferenceMisorientationsFilter.md`](../../OrientationAnalysis/vv/ComputeFeatureReferenceMisorientationsFilter.md)
- Retroactive DRAFT report: `/Users/mjackson/Desktop/vv_retroactive_reports/ComputeFeatureCentroidsFilter.md`

---

## Header table

|                            |                                                                                                                        |
|----------------------------|------------------------------------------------------------------------------------------------------------------------|
| Plugin                     | SimplnxCore                                                                                                             |
| SIMPLNX UUID               | `c6875ac7-8bdd-4f69-b6ce-82ac09bd3421`                                                                                  |
| SIMPLNX Human Name         | Compute Feature Centroids                                                                                               |
| DREAM3D 6.5.171 equivalent | `FindFeatureCentroids` (SIMPL UUID `6f8ca36f-2995-5bd3-8672-6b0b80d5b2ca`) — `Source/Plugins/Generic/GenericFilters/FindFeatureCentroids.{h,cpp}` |
| Verified commit            | *<filled at SBIR deliverable assembly>*                                                                                 |
| Status                     | DRAFT                                                                                                                   |
| Sign-off                   | *<named engineer + date at sign-off>*                                                                                   |

## At a glance

| Aspect                 | Current state |
|------------------------|---------------|
| Algorithm Relationship | **Port** — SIMPLNX `ComputeFeatureCentroids::operator()` non-periodic path is a near line-by-line translation of legacy `FindFeatureCentroids::find_centroids()`, **including the identical Kahan compensated summation and identical `double` accumulators**. Port-time additions: (1) SIMPLNX-only `Is Periodic` option, (2) disabled `ParallelDataAlgorithm` scaffolding, (3) `getCoords` returns `double` (`Point3Dd`) vs legacy `float[3]`, (4) `ValidateFeatureIdsToFeatureAttributeMatrixIndexing` guard. |
| Oracle (confirmed)     | **Class 1 (Analytical) primary + Class 4 (Invariant) companion** (confirmed 2026-07-07). 5 toy fixtures A–E hand-derived (centroid = mean of voxel centers). Second-engineer review routed (reviewer to be named). |
| Code paths enumerated  | **8 of 8 exercised** (see Phase 8 table): range filter, range tracking, Kahan accumulate, `count>0` finalize, `count==0` skip, periodic-fires, periodic-not-fires, validation-error. |
| Tests today            | **4 TEST_CASEs (5 ctest entries), all pass**: Class 1 Analytical (Fixtures A/B/C, 3 SECTIONs), Class 1/4 Periodic (D/E), validation-error (-5351), SIMPL 6.4/6.5 backwards-compat (`DYNAMIC_SECTION`). Retired the circular consistency test. |
| Exemplar archive       | **None — inline analytical fixtures** (provenance sidecar written). Shared `6_6_stats_test_v2.tar.gz` no longer consumed by this test (kept for 5 other tests). `6_6_find_feature_centroids.tar.gz` **kept** (used by ExtractComponentAsArray / WriteAbaqusHexahedron — not an orphan). |
| Legacy comparison      | **Source-inspection three-way** (6.5.171 / 6.5.172 / SIMPLNX). Non-periodic path is an exact Kahan port; 2 deviations documented (D1 precision float32→float64 voxel center; D2 periodic feature addition). Empirical binary A/B available if bit-confirmation needed. |
| Bug flags              | **2 SIMPLNX bugs found & FIXED.** (Phase 6) The spacing sub-bug: `AdjustCentroidsForPeriodicFaces` added `(dim−1)/2` in cell-index units to a physical-coordinate centroid, ignoring spacing. **(Follow-up, 2026-07, issue #1665)** The spacing fix was necessary but NOT sufficient — the underlying **constant half-domain offset model is itself wrong** (correct only for mass symmetric about the seam; can place the centroid outside the domain for asymmetric wrapped features). The periodic path was reimplemented as a **minimum-image circular mean** accumulated during the cell sweep; the range-based `ImageGeom` overload of `AdjustCentroidsForPeriodicFaces` was **removed**. See the Follow-up section below. |
| V&V phase              | **Phases 1–11 complete.** Oracle verified, 1 bug fixed + pinned, algorithm review applied, 8/8 code paths covered, deviations + provenance written, docs updated. Outstanding: **OOC dual-build deferred by developer**; Phase 12 (OneDrive archive); Phase 13 (status promotion + retroactive-report/INDEX update); second-engineer oracle review. |

## Follow-up (2026-07, issue #1665) — periodic model reimplemented

The original V&V (#1658) fixed a **spacing** sub-bug in the periodic path but validated the periodic result against the **constant half-domain offset model itself** (Fixture D's expected value was "naive + (dim−1)/2", a circular oracle — the test was designed around the code). An adversarial review (#1665) showed that model is fundamentally wrong: a constant offset is correct only when a wrapped feature's mass is symmetric about the seam, and for asymmetric features it places the centroid in the empty middle of the feature or outside the domain entirely.

This follow-up (on top of merged #1658) corrects it:

- **Periodic path reimplemented as a minimum-image circular mean.** During the existing cell sweep, per feature and axis, the unit vectors of each cell center's angular position around the domain are accumulated (`sumCos`, `sumSin`). At finalize, for any axis on which the feature spans the full extent, the centroid component becomes `atan2(sumSin, sumCos)` mapped back into the domain. A near-zero resultant (domain-filling feature) falls back to the arithmetic mean. O(numFeatures) extra memory; trig only when `Is Periodic` is on.
- **The range-based `ImageGeom` overload of `AdjustCentroidsForPeriodicFaces` was removed** — it received only per-feature index ranges, which are mathematically insufficient to compute a wrapped centroid (the whole point of #1665). Nothing else called it.
- **Fixtures D and E re-derived against an independent minimum-image oracle** (no longer circular): D → x = 3.5 (was 3.667), E (now asymmetric, non-unit spacing) → x = 11.0 (was 17.0). New Fixture F pins the domain-filling arithmetic-mean fallback. All pass in-core and out-of-core.
- The earlier "sibling BoundingBox overload audited — sound, left unchanged" conclusion is **superseded**: that overload carried the same constant-offset defect and was fixed under issue #1665 in the same branch as this follow-up.

The non-periodic path is unchanged (still the verified Kahan port). Deviation `ComputeFeatureCentroidsFilter-D2` is updated accordingly.

## Summary

`ComputeFeatureCentroidsFilter` computes the centroid of each feature as the average X/Y/Z position of all cells belonging to that feature, using Kahan compensated summation for numerical precision. An optional `Is Periodic` mode wraps features that span the image boundary using a minimum-image circular mean (see the Follow-up section for the 2026-07 reimplementation). Verification approach (planned): **Class 1 (Analytical) toy fixtures** — the centroid is the arithmetic mean of voxel-center coordinates, hand-derivable — paired with **Class 4 (Invariant)** bounding-box checks, then a legacy diff against DREAM3D 6.5.171 that is expected to reduce to a single float32-vs-float64 precision non-deviation plus the SIMPLNX-only periodic feature.

---

## Phase 1 — Discovery ✅ COMPLETE

**Goal**: Locate every artifact for the filter, identify the legacy equivalent, and pull forward the retroactive report's tentative classifications as starting points.

### Source files

| Kind | Path |
|---|---|
| Filter | `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ComputeFeatureCentroidsFilter.{hpp,cpp}` |
| Algorithm | `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeFeatureCentroids.{hpp,cpp}` |
| Unit test | `src/Plugins/SimplnxCore/test/ComputeFeatureCentroidsTest.cpp` |
| Docs | `src/Plugins/SimplnxCore/docs/ComputeFeatureCentroidsFilter.md` |
| SIMPL conversion | `test/simpl_conversion/6_4/ComputeFeatureCentroidsFilter.json`, `test/simpl_conversion/6_5/ComputeFeatureCentroidsFilter.json` |
| CMakeLists archives | `test/CMakeLists.txt:236` (`6_6_stats_test_v2.tar.gz`, used) and `:227` (`6_6_find_feature_centroids.tar.gz`, **orphan — downloaded but unreferenced**) |
| Legacy (6.5.172 tree) | `/Users/mjackson/Workspace1/6.5.172/DREAM3D/Source/Plugins/Generic/GenericFilters/FindFeatureCentroids.{h,cpp}` |
| Pipelines referencing it | `pipelines/ComputeBiasedFeatures.d3dpipeline`; docs cite "(03) Small IN100 Morphological Statistics", "InsertTransformationPhase", "(06) SmallIN100 Synthetic" |

### Algorithm summary

- **SIMPLNX** (`Algorithms/ComputeFeatureCentroids.cpp`, 220 lines): triple-nested voxel sweep accumulating per-feature X/Y/Z sums via **Kahan compensated summation** into `float64` stores (`m_Sum` = running sum, `m_Center` = *compensation term* — confusingly named, it is NOT the centroid), `uint64` `m_Count`. Per-feature finalize divides `sum/count`, casts to `float32`, stores in `Centroids`. Tracks per-feature min/max X/Y/Z index ranges for the optional periodic adjustment. `ParallelDataAlgorithm` is present but **`setParallelizationEnabled(false)`** (comment: thread spin-up outweighs the work). Periodic branch calls `GeometryHelpers::Topology::AdjustCentroidsForPeriodicFaces`.
- **Legacy** (`FindFeatureCentroids::find_centroids()`, lines 144–224): **identical** triple-nested sweep with the **same Kahan summation** (`sum`/`c`/`count` `std::vector`s, `double`/`double`/`size_t`), same `count>0` finalize dividing `sum/count` into a `float*`. **No periodic option, no range tracking, no validation guard.**
- **Key precision delta**: legacy fetches `voxel_center` into a `std::array<float,3>` (`getCoords(k,j,i,float*)`), then promotes to `double` inside the Kahan step; SIMPLNX fetches `Point3Dd` (`double`) directly. Both store the final result as `float32`. → the only numeric difference on the non-periodic path.

### TEST_CASE inventory (as-found)

| Test case | Kind | What it verifies |
|---|---|---|
| `SimplnxCore::ComputeFeatureCentroidsFilter` | consistency-with-self | Loads `6_6_stats_test_v2.dream3d`, runs filter → `Centroids NX`, `CompareDataArrays<float>` against the sibling `Centroids` array already in the file. `IsPeriodic == false` only. **Not an independent oracle** (the reference array is prior DREAM3D output). |
| `SimplnxCore::ComputeFeatureCentroidsFilter: SIMPL Backwards Compatibility` | conversion | `DYNAMIC_SECTION` over SIMPL 6.5 (UUID) + 6.4 (Filter_Name); validates UUID + 3 argument values. Conversion only — no output check. |

### Test exemplar archive

- **`6_6_stats_test_v2.tar.gz`** — SHA512 `e84999dec914d81efce4fc4237c49c9bf32e48381b1e79f58aa4df934f0d7606cd7a948f9a5e7b17a126a7944cc531b531cfdc70756ca3e2207b20734e089723` (`test/CMakeLists.txt:236`). Shared "stats" archive (Centroids / Sizes / Phases / Neighbors). Cross-cutting finding #6: its **SSA values are confirmed bad-from-legacy** — irrelevant to centroids (which don't use SSA) but noted.
- **`6_6_find_feature_centroids.tar.gz`** — SHA512 `1eecd4ed…9c28` (`test/CMakeLists.txt:227`). **Orphan**: downloaded by the test target but not referenced by `ComputeFeatureCentroidsTest.cpp`. Resolve in Phase 2/10.

### Material PRs since baseline (2025-10-01)

Per retroactive report — none change the algorithm:
- **#1547** (DOC) — added the `Is Periodic` description to the docs. Doc-only.
- **#1543** (DOC) — pipeline reference rename. Doc-only.
- **#1588** (ENH) — added the SIMPL 6.4/6.5 backwards-compat test + 2 fixtures. Test-only.
- Pruned: #1439 (tuple API), #1457 (static-inline), #1538 (zlib extraction) — no behavior change.

### Discovery deltas vs the retroactive DRAFT report

1. ⚠️ **Retroactive D1 (Kahan-vs-naive precision) is WRONG.** The report hypothesized legacy uses a naive running sum; legacy source lines 177–199 use the **exact same Kahan summation**. → D1 must be re-cast as a **float32-vs-float64 voxel-center** precision delta (see Phase 9 starting point), or retracted if unobservable.
2. ✅ **Retroactive D2 (periodic is SIMPLNX-only) CONFIRMED.** Legacy has no `IsPeriodic` parameter or wrap logic.
3. ✅ Legacy plugin/group confirmed: **Generic / Morphological**, SIMPL UUID `6f8ca36f-2995-5bd3-8672-6b0b80d5b2ca`, human name "Find Feature Centroids".

### Exit criteria — MET

- ✅ All source/test/doc/pipeline/conversion files located
- ✅ Legacy equivalent identified and source read
- ✅ Retroactive report cross-checked; one hypothesis corrected, one confirmed

---

## Phase 2 — Promote existing work product

**Goal**: Decide promote / augment / replace for each existing artifact before designing new oracles.

### Promotion decisions (developer-confirmed 2026-07-07)

| Artifact | Decision | Rationale |
|---|---|---|
| `SimplnxCore::ComputeFeatureCentroidsFilter` consistency test (Centroids NX vs sibling Centroids) | **RETIRE entirely** | Circular oracle (consistency-with-self); replaced by Class 1 + Class 4 toy fixtures A–E. |
| SIMPL 6.4/6.5 conversion fixtures + backwards-compat test | **PROMOTE as-is** | Conversion coverage is sound; not a correctness oracle. |
| `6_6_stats_test_v2.tar.gz` (shared) | **AUGMENT/DECOUPLE** | Centroids no longer consumes it for correctness; if no other retained centroids test needs it, drop the centroids test's `TestFileSentinel`. Provenance sidecar still written in Phase 10 for any array this filter's tests touch. |
| `6_6_find_feature_centroids.tar.gz` (`:227`) | **KEEP** | ⚠️ Retroactive report called this an orphan — **incorrect**. It is unreferenced by the *centroids* test but actively used by `ExtractComponentAsArrayTest.cpp` and `WriteAbaqusHexahedronTest.cpp`. Grep confirmed 2026-07-07. Do NOT remove. |

### Tasks
- [ ] **Grep** the repo to confirm `6_6_find_feature_centroids.tar.gz` and its sentinel `.dream3d` have no consumer, then queue removal for Phase 10.
- [ ] **Inspect** `6_6_stats_test_v2.tar.gz` for an inner ReadMe / provenance (feeds Phase 10 sidecar).

### Exit criteria — table complete; orphan disposition pending grep in Phase 10.

---

## Phase 3 — Algorithm Relationship classification ✅ CONFIRMED

**Goal**: Confirm the opening classification line of the eventual deviations report.

*Classification:* **Port.**

> *Algorithm Relationship: **Port** — `ComputeFeatureCentroids::operator()` is a near line-by-line translation of legacy `FindFeatureCentroids::find_centroids()` (6.5.172 `Generic/GenericFilters/FindFeatureCentroids.cpp:144–224`), preserving the identical Kahan compensated summation, the `float64`/`float64`/`uint64` sum/compensation/count triplet, and the `count>0` divide-and-store finalize.*

**Port-time deltas** (each with output impact):
1. **`Is Periodic` option** (SIMPLNX-only) + per-feature index-range tracking + `GeometryHelpers::Topology::AdjustCentroidsForPeriodicFaces`. → **Changes output when enabled**; default `false` reproduces legacy. Legacy has no equivalent. Drives D2.
2. **Voxel center precision**: SIMPLNX `getCoords → Point3Dd` (float64); legacy `getCoords(…, float*)` (float32) then promoted to double in the Kahan step. → Sub-ULP-at-float32 numeric delta; both store float32. Drives D1.
3. **`ParallelDataAlgorithm` scaffolding present but `setParallelizationEnabled(false)`**. → No output impact (runs serially, same iteration order as legacy).
4. **`ValidateFeatureIdsToFeatureAttributeMatrixIndexing` guard** added before the sweep. → No output impact on valid data; adds an error path legacy lacked.

*Material PRs since baseline:* none change the algorithm (see Phase 1 §Material PRs).

### Exit criteria — MET
- ✅ One classification (Port) with evidence; port-time deltas enumerated with per-delta output-impact note

---

## Phase 4 — Oracle classification ✅ CONFIRMED

**Goal**: Pick the oracle (before any 6.5.171 run — the one ordering rule).

*Class:* **1 (Analytical) primary + 4 (Invariant) companion.** (Confirmed by developer 2026-07-07.)

- **Class 1 (Analytical)** — centroid component = `Σ(voxel-center coord) / N` over cells in the feature, where `voxel-center = origin + (index + 0.5)·spacing`. Closed-form on any toy grid.
- **Class 4 (Invariant)** —
  - Non-periodic: each centroid component lies within `[origin + (idxMin+0.5)·spacing, origin + (idxMax+0.5)·spacing]` (the feature's voxel-center bounding box).
  - Untouched feature ID (count==0) stays `(0,0,0)` via the `count>0` guard.
  - Periodic adjustment fires **iff** the feature's index range spans the full extent on that axis (`min==0 && max==dim-1`); when it fires it offsets that component by exactly `(dim-1)/2` relative to the non-periodic value.
- **Class 3 N/A** — no published paper for "mean position of cells in a feature".
- **Second-engineer oracle review**: developer selected "Yes — someone else." *Reviewer to be named* (recommend a SimplnxCore-domain engineer); review at the Phase 5 checkpoint. Record name + date here on completion.

### Exit criteria — MET
- ✅ Class(es) named (1 + 4); second-engineer review routed (reviewer to be named)

---

## Phase 5 — Toy data design + independent expected output ✅ DESIGNED (pending confirmation)

**Goal**: Minimum-size fixtures covering each code path with hand-derived expected centroids, derived **independently of both codebases** (voxel-center = `origin + (index+0.5)·spacing`; centroid = mean over the feature's cells). **No DREAM3D run yet.**

All fixtures below use spacing `(1,1,1)` and origin `(0,0,0)` unless noted. FeatureIds are given in row-major order; the Feature Attribute Matrix tuple count sets `totalFeatures`.

### Fixture A — single multi-cell feature (basic mean + count==0 background)
- **Grid** 3×1×1; **FeatureIds** `[1, 1, 1]`; **AM tuples** 2 (ids 0,1).
- x-centers 0.5, 1.5, 2.5 → **centroid[1] = (1.5, 0.5, 0.5)**; **centroid[0] = (0, 0, 0)** (count==0).

### Fixture B — multi-feature 2D (averaging + single-cell + empty id)
- **Grid** 4×2×1; **FeatureIds** row y0 `[1,1,2,2]`, row y1 `[1,3,2,2]`; **AM tuples** 4 (ids 0–3).
- **centroid[0] = (0, 0, 0)** (count==0 background).
- **centroid[1]** — cells (0,0),(1,0),(0,1): x̄=(0.5+1.5+0.5)/3=0.8333̅, ȳ=(0.5+0.5+1.5)/3=0.8333̅ → **(0.83333, 0.83333, 0.5)**.
- **centroid[2]** — cells (2,0),(3,0),(2,1),(3,1): x̄=(2.5+3.5+2.5+3.5)/4=3.0, ȳ=(0.5·2+1.5·2)/4=1.0 → **(3.0, 1.0, 0.5)**.
- **centroid[3]** — cell (1,1): → **(1.5, 1.5, 0.5)** (single-cell).

### Fixture C — 3D volume (z-stride correctness)
- **Grid** 2×2×2; **FeatureIds** z0 plane all `1`, z1 plane all `2`; **AM tuples** 3 (ids 0–2).
- **centroid[0] = (0,0,0)**; **centroid[1] = (1.0, 1.0, 0.5)** (z-index 0); **centroid[2] = (1.0, 1.0, 1.5)** (z-index 1). Confirms the z-stride separates planes.

### Fixture D — periodic wrap, unit spacing (`IsPeriodic == true`)
- **Grid** 4×1×1; **FeatureIds** `[1, 2, 2, 1]`; **AM tuples** 3.
- fid 1 occupies x=0 and x=3 → spans full X extent (min idx 0, max idx 3 = dim−1). fid 2 occupies x=1,2 (does not span).
- **Non-periodic** centroid[1].x = (0.5+3.5)/2 = 2.0.
- **Periodic** adds `(dim−1)/2 = 3/2 = 1.5` → **centroid[1] = (3.5, 0.5, 0.5)**; **centroid[2] = (1.5, 0.5, 0.5)** (unchanged, no span).
- *Class-1 note:* the periodic value is analytical **given the DREAM3D periodic-shift model** (`naive + (dim−1)/2` on a spanned axis). This fixture verifies the branch fires on the right condition and applies the documented offset.

### Fixture E — periodic wrap, non-unit spacing (**latent-bug probe**) ⚠️
- **Grid** 4×1×1, **spacing (2,1,1)**, **origin (10,0,0)**; **FeatureIds** `[1, 2, 2, 1]`; **AM tuples** 3.
- x-centers: idx0 → 10+(0.5)·2 = 11.0; idx3 → 10+(3.5)·2 = 17.0. Non-periodic centroid[1].x = 14.0.
- **Code computes** periodic offset `(dim−1)/2 = 1.5` (**cell-index units**) → 14.0 + 1.5 = **15.5**.
- **Physically-correct** wrap offset should scale with spacing (half the domain physical width `dim·spacing/2 = 4` → 18.0, or half the feature physical extent `(17−11)/2 = 3` → 17.0). **15.5 matches none.**
- **Hypothesis:** `AdjustCentroidsForPeriodicFaces` adds `(dim−1)/2` in cell units to a physical-coordinate centroid, ignoring spacing and origin. Correct only when spacing==1. **This fixture is expected to expose the bug in Phase 6.** If confirmed, it becomes a SIMPLNX bug (not a legacy deviation — legacy has no periodic path), fixed under Phase 6/8 with this fixture as the regression pin.

### Class 4 invariants (predicates, all fixtures)
- `count==0 ⇒ centroid == (0,0,0)`.
- Non-periodic: `centroid[c] ∈ [origin[c]+(idxMin+0.5)·spacing[c], origin[c]+(idxMax+0.5)·spacing[c]]`.
- Periodic adjustment fires iff the feature spans the full extent on axis c.

### Tasks
- [ ] **Confirm** fixtures A–E with the second-engineer reviewer (esp. the Fixture E bug hypothesis).
- [ ] **Save** this derivation section as the archive's oracle notes (Phase 12).

### Exit criteria
- ✅ Fixtures cover all code paths incl. count==0, 3D z-stride, periodic-fires and periodic-not-fires, and the spacing-sensitivity probe; expected values hand-derived independently. *(Awaiting second-engineer confirm.)*

---

## Phase 6 — SIMPLNX vs oracle reconciliation ⚠️ RECONCILED — 1 confirmed bug pending fix decision

**Goal**: Run SIMPLNX on the toy data; reconcile against the oracle; fix any SIMPLNX bug now.

**Encoded**: `test/ComputeFeatureCentroidsTest.cpp` — `namespace CentroidToy` + TEST_CASEs `Class 1 - Analytical Centroids` (Fixtures A/B/C) and `Class 1/4 - Periodic Boundary` (Fixtures D/E). Built + run in `NX-Com-Qt69-Vtk96-Rel` on 2026-07-07: **3/3 pass**.

**Results**:
- ✅ **Non-periodic path VERIFIED** — Fixtures A, B, C match the Class 1 analytical values exactly (float32 margin 1e-4). SIMPLNX centroid computation is correct.
- ✅ **Periodic branch fires correctly** — Fixture D: adjustment applies to the spanning feature, leaves the non-spanning feature unchanged.
- ⚠️ **BUG CONFIRMED (Fixture E)** — with spacing (2,1,1), the periodic offset applied is **1.5** (= `(dim−1)/2` in cell-index units), giving centroid.x = 15.5 on top of the analytical non-periodic 14.0. The offset does **not** scale with spacing. `GeometryHelpers::Topology::AdjustCentroidsForPeriodicFaces` (ImageGeom overload) adds `xPoints/2.0f` (cell units) to a physical-coordinate centroid → dimensionally inconsistent; correct only when spacing == 1.

**Blast radius**: the buggy ImageGeom overload is called **only** by `ComputeFeatureCentroids.cpp:213`. The BoundingBox overload (used by `ComputeTriangleGeomCentroids`) works in physical bounding-box space and is **not** affected. Legacy 6.5.172 has no periodic centroid path → this is a **SIMPLNX-internal bug, not a legacy deviation** (so it lives here in Phase 6, not as a Phase 9 D-entry).

**FIX APPLIED** (developer-approved 2026-07-07, "minimal spacing fix"): `src/simplnx/Utilities/GeometryHelpers.cpp:245` ImageGeom overload now adds `(static_cast<float32>(points_c) * spacing[c]) / 2.0f`. Unit-spacing behavior identical (Fixture D unaffected); Fixture E now asserts the corrected **17.0** (was 15.5) as the regression pin. Rebuilt + reran: **3/3 pass**.

**Sibling-overload audit** (developer-approved "audit both overloads"): the BoundingBox overload (`GeometryHelpers.cpp:220`, called by `ComputeTriangleGeomCentroids.cpp:98`) adds `abs(maxPoint−minPoint)/2` on **physical** bounding-box coordinates → dimensionally sound, **no spacing bug**. Left unchanged. *Observation (not a defect):* it uses a **feature-bounding-box** shift model whereas the ImageGeom overload uses a **whole-domain** shift model — a semantic inconsistency worth a future note, out of scope here. `ComputeTriangleGeomCentroids` test re-run post-fix: **passes** (no regression from the core-lib relink).

### Tasks
- [ ] **Encode** fixtures A–E as a new in-code TEST_CASE (build the DataStructure inline, à la the OA format-of-record `ToyFixtures`).
- [ ] **Compare** Class 1 values (float32 tolerance, `Catch::Approx`) + Class 4 predicates.
- [ ] **Confirm/deny the Fixture E periodic units-bug hypothesis.** If confirmed: fix `AdjustCentroidsForPeriodicFaces` to scale the offset by spacing (and account for origin), add Fixture E as the regression pin, and re-derive Fixture D's expected value if the fix changes it (D uses spacing 1 so it should be unaffected). If denied (behavior is intended/documented): downgrade to a documented limitation and record the reasoning.
- [ ] **Resolve** any other discrepancy in SIMPLNX before Phase 7.

### Exit criteria
- ✅ SIMPLNX matches the oracle on every fixture within tolerance (periodic path either fixed or its limitation documented)

---

## Phase 7 — Algorithm Review ✅ COMPLETE

**Goal**: Quality pass on verified-correct code.

### Findings + actions (all output-neutral; rebuilt + 4/4 tests pass after each)
- ✅ **Fixed** — `m_Center`/`center`/`centerPtr` renamed to `m_Compensation`/`compensation`/`compensationPtr` with a class-level comment explaining it is the Kahan compensation term, not a centroid (it read as a bug). `Algorithms/ComputeFeatureCentroids.cpp`.
- ✅ **Fixed** — `if(static_cast<float>(count[...]) > 0.0f)` → `if(count[...] > 0)` (uint64 compared as uint64) in the finalize loop.
- ✅ **Documented** — added a comment that `count` carries the same voxel count in all 3 components and that a zero-cell feature keeps its (0,0,0) default.
- ⏸ **Deferred (rationale)** — no per-cell cancel check: legacy had none, and parallelization is disabled so the sweep is a single serial pass; adding a cancel probe per voxel is low value. Recorded, not blocking.
- ⏸ **Out of scope** — the periodic shift-model inconsistency between the two `AdjustCentroidsForPeriodicFaces` overloads (whole-domain vs feature-bbox) noted in Phase 6; a future cleanup, not a defect.

### Exit criteria — MET
- ✅ All actionable findings fixed; deferrals justified; behavior unchanged (tests green)

---

## Phase 8 — Unit Test Review & Implementation ✅ COMPLETE (OOC dual-build deferred)

**Goal**: Encode the oracle as permanent tests; fill coverage gaps.

### Done
- ✅ Class 1 fixtures A/B/C + Class 1/4 periodic D/E + validation-error test implemented inline in `test/ComputeFeatureCentroidsTest.cpp`.
- ✅ Retired the circular consistency test and its `TestFileSentinel`/archive dependency (the shared archive is kept for other tests).
- ✅ In-core build: **4 TEST_CASEs / 5 ctest entries, 100% pass** in `NX-Com-Qt69-Vtk96-Rel`.
- ⏸ **OOC dual-build deferred by developer** (2026-07-07) — to run before final sign-off.

### Code path coverage — 8 of 8 exercised

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeFeatureCentroids.cpp` (~222 lines). Phases: (a) per-cell sweep (accumulate + range-track), (b) per-feature finalize, (c) optional periodic adjust.

| # | Phase | Path | Test |
|---|---|---|---|
| 1 | (a) | Range filter `featureId ∈ [min,max)` (parallel off → full range) | All fixtures (valid ids) |
| 2 | (a) | Per-feature min/max X/Y/Z index range tracking | Fixtures D, E (drive the periodic condition) |
| 3 | (a) | Kahan accumulate X/Y/Z + count inc | All fixtures |
| 4 | (b) | `count>0` → centroid = sum/count | A/B/C/D/E (non-empty features) |
| 5 | (b) | `count==0` → centroid stays (0,0,0) | A (fid0), B (fid0), C (fid0) |
| 6 | (c) | `IsPeriodic` + feature spans full extent → offset applied (spacing-aware) | D (spacing 1), E (spacing 2, regression pin) |
| 7 | (c) | `IsPeriodic` + feature does not span → unchanged | D (feature 2), all non-periodic runs |
| 8 | pre | `ValidateFeatureIdsToFeatureAttributeMatrixIndexing` failure (`maxId ≥ numFeatures`, -5351) | `Error - FeatureId exceeds Feature AM` |

*Preflight AM-null path (-12700) is guarded by the AttributeMatrixSelectionParameter (cannot be triggered with valid args) — not separately tested.*

### Exit criteria — MET (in-core); OOC pending
- ✅ Every code path maps to ≥1 named test; all pass in-core
- ⏸ OOC build pass deferred

---

## Phase 9 — Legacy DREAM3D Comparison (diff explanation) ✅ COMPLETE

**Goal**: Diff SIMPLNX vs 6.5.171 (and 6.5.172 backport); write Deviation entries. Oracle already established, so this is diff-explanation only.

### Result ✅ COMPLETE (source-inspection three-way)

Comparison basis: source inspection of 6.5.171/6.5.172 `FindFeatureCentroids.cpp` vs SIMPLNX, backed by the Class 1 analytical fixtures verifying SIMPLNX independently. Non-periodic path is an exact Kahan port. Full entries in [`deviations/ComputeFeatureCentroidsFilter.md`](deviations/ComputeFeatureCentroidsFilter.md):

- **D1 (precision)** — SIMPLNX fetches voxel centers as `float64`; legacy as `float32` then promotes. Both store `float32`. ≤1 float32 ULP; trust SIMPLNX.
- **D2 (feature addition)** — `Is Periodic` is SIMPLNX-only; default `false` reproduces legacy. Includes the note that the periodic spacing bug (D2 note) was fixed this cycle (SIMPLNX-internal, not a legacy diff).
- **Retracted** — the DRAFT "Kahan-vs-naive" candidate: legacy already uses Kahan.

*Optional strengthening:* an empirical binary A/B (6.5.171 PipelineRunner + 6.5.172 backport + nxrunner on a shared input) can bit-confirm D1 if the SBIR deliverable requires it. Not required for a verified port.

### Exit criteria — MET
- ✅ Comparison done; each difference has a fleshed-out entry in the deviations file

---

## Phase 10 — Exemplar Validation & Publishing ✅ COMPLETE

**Goal**: Validate/publish exemplars; write provenance sidecars; kill circular oracles.

### Done
- ✅ Circular consistency test **retired**; replaced by inline analytical fixtures (no archive needed).
- ✅ Provenance sidecar written: [`provenance/ComputeFeatureCentroidsFilter.md`](provenance/ComputeFeatureCentroidsFilter.md) — documents the "no archive / inline oracle" state, the retirement, and both shared-archive dispositions.
- ✅ **No CMakeLists archive changes**: `6_6_stats_test_v2.tar.gz` kept (5 other consumers); `6_6_find_feature_centroids.tar.gz` kept (used by ExtractComponentAsArray / WriteAbaqusHexahedron — retroactive "orphan" claim corrected).

### Exit criteria — MET
- ✅ Provenance sidecar exists; circular oracle removed; no dangling/incorrect archive edits

---

## Phase 11 — Documentation Review ✅ COMPLETE

**Goal**: `review-filter-docs` accuracy pass.

### Done (`docs/ComputeFeatureCentroidsFilter.md`)
- ✅ Group/Subgroup corrected "Generic (Misc)" → "Generic (Morphological)" (matches `defaultTags()` + legacy subgroup).
- ✅ Description clarified: physical-coordinate mean, Kahan summation noted, `Is Periodic` reworded to match the fixed spacing-aware behavior ("half the physical distance between the first and last cell centers"), and the count==0 → (0,0,0) contract added.

### Not done (optional)
- Before/after figure — not added (low value for a centroid filter; can add later).

### Exit criteria — MET
- ✅ Docs accurate against implementation (incl. the D2 fix); parameter table auto-generated marker intact

---

## Phase 12 — Archive

**Goal**: `archive-filter-verification` bundle for OneDrive.

### Exit criteria
- ✅ ReadMe captures Algorithm Relationship, Oracle class + rationale, provenance, second-engineer review (or skip), promoted-vs-new artifacts, reproduction instructions

---

## Phase 13 — Update tracking artifacts

**Goal**: Promote status + update trackers.

### Tasks
- [ ] **Set** this doc's Status line to `COMPLETE — V&V finished YYYY-MM-DD` (file stays put).
- [ ] **Update** the retroactive report `/Users/mjackson/Desktop/vv_retroactive_reports/ComputeFeatureCentroidsFilter.md` (DRAFT → confirmed; correct the D1 retraction; confirm D2).
- [ ] **Update** `INDEX.md` row.
- [ ] **Note** completion in the SBIR deliverable tracker.

### Exit criteria
- ✅ Status promoted; trackers updated
