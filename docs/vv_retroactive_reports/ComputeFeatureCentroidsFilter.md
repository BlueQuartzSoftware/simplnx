# Retroactive V&V: ComputeFeatureCentroidsFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `c6875ac7-8bdd-4f69-b6ce-82ac09bd3421` |
| SIMPLNX ClassName | `ComputeFeatureCentroidsFilter` |
| SIMPLNX Human Name | Compute Feature Centroids |
| SIMPL UUID | `6f8ca36f-2995-5bd3-8672-6b0b80d5b2ca` (from `test/simpl_conversion/6_5/ComputeFeatureCentroidsFilter.json`) |
| SIMPL ClassName | `FindFeatureCentroids` (from SIMPL conversion fixtures) |
| SIMPL Human Name | Find Feature Centroids *(inferred from class name; confirm in legacy SIMPL repo)* |
| Plugin | SimplnxCore |

### Source files scanned

- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ComputeFeatureCentroidsFilter.{hpp,cpp}`
- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeFeatureCentroids.{hpp,cpp}`
- `src/Plugins/SimplnxCore/test/ComputeFeatureCentroidsTest.cpp`
- `src/Plugins/SimplnxCore/test/simpl_conversion/6_4/ComputeFeatureCentroidsFilter.json`
- `src/Plugins/SimplnxCore/test/simpl_conversion/6_5/ComputeFeatureCentroidsFilter.json`
- `src/Plugins/SimplnxCore/docs/ComputeFeatureCentroidsFilter.md`
- `src/Plugins/SimplnxCore/test/CMakeLists.txt` (for archive resolution)

## Algorithm Relationship

- **Tentative classification:** **Port with one feature addition** — the SIMPLNX filter is a direct translation of the legacy SIMPL `FindFeatureCentroids` filter (renamed to `ComputeFeatureCentroidsFilter`). The legacy SIMPL UUID is preserved in the 6.5 conversion fixture and a `FromSIMPLJson` converter is in place.
- **Notable additions over a pure port:**
  - Use of **Kahan compensated summation** in the X/Y/Z accumulators (algorithm `ComputeFeatureCentroids.cpp`, lines ~61–83). This is a numerical-precision improvement over a naive running sum and may not be present in legacy SIMPL.
  - The `Is Periodic` option (delegated to `GeometryHelpers::Topology::AdjustCentroidsForPeriodicFaces`). The doc was updated in PR #1547 to describe this option, suggesting it was either added in SIMPLNX or only recently documented.
- **Evidence:** No rewrite signal in PR history. The algorithm is small (~220 lines) and structurally simple (single triple-nested voxel sweep + per-feature divide). UUID inheritance from SIMPL via the conversion fixtures is the strongest evidence of "port".
- **Action required:** Confirm by reading the SIMPL `FindFeatureCentroids` source. Specifically: (a) does legacy use Kahan summation or a naive sum? (b) does legacy support periodic boundaries? Either answer affects the legacy comparison Deviation entry.

## PRs inspected (since 2025-10-01)

> Pruned: pure-style/repo-wide refactor PRs (#1457 static-inline, #1438/#1439 multi-dim tuple API, #1538 zlib extraction) are listed at the bottom of this section but not detailed individually — they did not change behavior of this filter.

### PR #1547 — *"DOC: Fix filter documentation and documentation related code bugs"* — merged 2026-03-11

- **Files in this filter:** docs (.md), 1 line changed
- **Diff size:** 1 file, +1 / -1 lines
- **Change nature:** **Documentation correction.** Rewrote the description paragraph to add an explanation of the `Is Periodic` option (its enabled/disabled behavior with respect to features that extend beyond the image boundary). Promoted from a generic "no periodic support" statement to a full description of both modes.
- **V&V content:** Documentation accuracy. The behavior described matches the algorithm path that calls `GeometryHelpers::Topology::AdjustCentroidsForPeriodicFaces` when `IsPeriodic == true`. Doc currency improvement; not algorithmic.

### PR #1543 — *"DOC: Update pipeline references in each of the documentation files"* — merged 2026-02-24

- **Files in this filter:** docs (.md), 1 line changed
- **Diff size:** 1 file, +1 / -1 lines
- **Change nature:** Pipeline reference cosmetic — renamed the example pipeline reference from "(01) SmallIN100 Morphological Statistics" to "(03) Small IN100 Morphological Statistics" to match the actual current pipeline filename. Doc hygiene only.
- **V&V content:** None.

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** test (.cpp) +45 lines, plus two new fixture files
  - `test/simpl_conversion/6_4/ComputeFeatureCentroidsFilter.json` (~22 lines, uses `Filter_Name`-only fallback)
  - `test/simpl_conversion/6_5/ComputeFeatureCentroidsFilter.json` (~23 lines, includes `Filter_Uuid`)
- **Diff size:** 3 files, +90 lines
- **Change nature:** **Test addition.** Added a per-filter SIMPL→SIMPLNX backwards-compatibility test that exercises both SIMPL 6.4 (Filter_Name fallback) and 6.5 (UUID-mapped) pipeline conversion paths via `DYNAMIC_SECTION`. Test name: `"SimplnxCore::ComputeFeatureCentroidsFilter: SIMPL Backwards Compatibility"`.
- **V&V content:** **Pipeline-conversion correctness only** — the test verifies that opening a legacy SIMPL pipeline in DREAM3DNX produces a `ComputeFeatureCentroidsFilter` instance with the right parameter values (FeatureIds path, geometry path, and Centroids array name). It does **not** verify that the filter's *output* matches legacy. That latter step is still missing.
- **Notable in fixtures:** Both fixtures only specify the legacy `FeatureIdsArrayPath` and `CentroidsArrayPath` keys — there is no legacy parameter for `IsPeriodic`. The conversion silently uses the SIMPLNX default (`false`), which is consistent with the hypothesis that periodic support is a SIMPLNX addition.

### Pruned PRs (touched the file but not behaviorally relevant to this filter)

| PR | Subject | Why pruned |
|---|---|---|
| #1439 | Multi-Dimensional Tuple Support for StringArray and NeighborList | API plumbing; +3/-3 across 2 files. No algorithmic change to centroid computation. |
| #1457 | Clean up 'static inline' from filter headers | Style — 5 line changes in the .hpp constexpr declarations. No behavior. |
| #1538 | Replace cmake subprocess tar.gz extraction with zlib | Test infrastructure — 1-line update of how the test fetches its `.tar.gz` exemplar. |

## Test coverage detected

`ComputeFeatureCentroidsTest.cpp` contains **2** `TEST_CASE`s:

1. `SimplnxCore::ComputeFeatureCentroidsFilter` — Loads `6_6_stats_test_v2.dream3d`, runs the filter to produce a `Centroids NX` array in the cell-feature attribute matrix, then compares it tuple-by-tuple against the pre-existing `Centroids` array in the same exemplar via `CompareDataArrays<float>`. This exercises the **default code path with `IsPeriodic == false`**.
2. `SimplnxCore::ComputeFeatureCentroidsFilter: SIMPL Backwards Compatibility` — SIMPL 6.4 + 6.5 conversion paths via `DYNAMIC_SECTION` *(added by PR #1588)*. Conversion-only.

**Coverage gaps observed:**

- **No test of `IsPeriodic == true`.** The periodic branch in `ComputeFeatureCentroids::operator()` (which calls `AdjustCentroidsForPeriodicFaces` and may emit the "Non-Contiguous Features" info message) is currently untested.
- **No toy-data / hand-derivation test.** Centroids on a known geometry with known feature membership would let the test stand on its own oracle (Class 1) instead of relying on a comparison against another array in the same exemplar file.
- **No test of the `ValidateFeatureIdsToFeatureAttributeMatrixIndexing` failure path** (mismatched feature-AM tuple count vs. max featureId).

## Exemplar archive

- **Archive name:** `6_6_stats_test_v2.tar.gz`
- **SHA512:** `e84999dec914d81efce4fc4237c49c9bf32e48381b1e79f58aa4df934f0d7606cd7a948f9a5e7b17a126a7944cc531b531cfdc70756ca3e2207b20734e089723`
- **Referenced in:** `src/Plugins/SimplnxCore/test/CMakeLists.txt` (line 233)
- **Sentinel file:** `6_6_stats_test_v2.dream3d`
- **Note on archive name:** This is a **shared exemplar archive** used by multiple "stats" filter tests (ComputeFeatureCentroids, plus likely ComputeNeighborhoods, ComputeFeatureClustering, etc.). The `6_6_` prefix is the legacy DREAM3D-version prefix; the `_v2` suffix indicates this is a versioned re-upload of an earlier archive. This filter does **not** have a dedicated per-filter exemplar.
- **Related but unused archive:** `6_6_find_feature_centroids.tar.gz` (line 224 of `test/CMakeLists.txt`) is downloaded by the test target but is **not** referenced by `ComputeFeatureCentroidsTest.cpp` — it appears to be an orphan or used by a different (unfound) test. Engineer should investigate whether this archive can be retired or whether it was intended to be the centroids-specific exemplar.
- **Provenance:** *(TBD — engineer must inspect the archive to determine how the exemplar was generated and whether an Oracle Provenance block exists in any ReadMe inside it.)*
- **Action required:** Download `6_6_stats_test_v2.tar.gz` locally and inspect for: an inner `ReadMe.md`, the input `.dream3d` files used to generate the exemplars, the pipeline files that produced the exemplars, and any provenance notes. Also inspect `6_6_find_feature_centroids.tar.gz` and confirm whether it is dead weight or intended as a per-filter oracle.

## Oracle classification (tentative)

- **Recommended class:** **1 (Analytical)**, secondary **4 (Invariant-based)**.
- **Rationale:** The centroid of a feature is mathematically defined as the arithmetic mean of the voxel-center positions of every cell belonging to that feature. For any toy dataset with N voxels of known coordinates, the expected centroid is a hand-computable number: `centroid_x = (sum of voxel_center_x for cells in feature) / N`. This is the textbook example of a **Class 1** analytical oracle — there is no ambiguity, no algorithmic choice that affects the answer (modulo float roundoff), and the expected output can be hand-derived on a 3-voxel feature.
  - Worked toy example: feature 1 occupies cells (0,0,0), (1,0,0), (2,0,0) on a unit-spacing image with origin (0,0,0). Voxel centers are (0.5, 0.5, 0.5), (1.5, 0.5, 0.5), (2.5, 0.5, 0.5). Expected centroid = (1.5, 0.5, 0.5). The current implementation should produce exactly this (within `float32` roundoff).
- **Class 4 (Invariant) supplements:** Even with an analytical oracle, useful invariants exist: every centroid component must lie within the bounding box of the feature; the centroid for an empty feature ID stays at zero (the algorithm's `count > 0` guard); under the periodic option, centroids of wrapping features should fall outside the literal voxel bounding box.
- **Class 3 (Paper-based) is NOT recommended** — there is no paper to cite for "average position of the cells in a feature".
- **Action required:** Developer to confirm Class 1 and write a 5–10 line toy-data test (`SimplnxCore::ComputeFeatureCentroids:Analytical`) that hand-derives one or two centroids and asserts the expected float32 values. This test would *be* the Class-1 oracle of record.

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | Not visible from PR history | No PR explicitly performs the line-by-line review. The Kahan-summation block is uncommented as such and could use a comment justifying the precision choice. |
| Code path coverage (algorithmic) | **Partial** | Only `IsPeriodic == false` is exercised. Periodic branch and validation-failure branch are untested. |
| Code path coverage (SIMPL conversion) | Good | PR #1588 added SIMPL 6.4 + 6.5 conversion test. |
| Exemplar data in Data_Archive | **Yes (shared)** | Test consumes `6_6_stats_test_v2.tar.gz`; no per-filter archive. `6_6_find_feature_centroids.tar.gz` exists in CMakeLists but is not referenced by this test. |
| Exemplar provenance documented | Unknown | TBD by inspecting archive contents. |
| Oracle class recorded | **No** | This document is the first to propose one. Class 1 (Analytical) is the recommendation. |
| Toy data / independent expected output (Step 0 c) | **No** | The current test compares `Centroids NX` against a sibling `Centroids` array inside the same exemplar — that is *consistency with self*, not an independent oracle. A small hand-derived toy test would close this gap easily. |
| Legacy comparison report (Step 0 e) | No | `compare-legacy-dream3d` has not been run. |
| Deviation entries (`ComputeFeatureCentroids-D<N>`) | None | Not yet written. Likely candidate: float32-precision differences between legacy naive sum and SIMPLNX Kahan sum on large features (see proposed D1 below). |
| Documentation currency | Probably current | Updated by PRs #1543 (pipeline reference) and #1547 (Is Periodic description). The auto-generated parameter table is in place. Needs accuracy audit per `review-filter-docs`. |
| Verification archive (OneDrive) | No | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

1. **Confirm the oracle.** Class 1 (analytical) is the recommended starting point. The arithmetic mean of voxel-center coordinates is the definition of the centroid; this is the textbook hand-derivation case.
2. **Add a Class-1 toy test.** Construct a small DataStructure (e.g., 4×1×1 image, two features) in code, run the filter, and `REQUIRE(centroids[i] == hand_derived_value)` with appropriate `Approx`/`Catch::Approx` tolerance. This becomes the oracle of record and removes the dependency on the consistency-with-self comparison in the current test.
3. **Add a periodic-mode test.** Construct a feature that wraps the X edge (cells at x=0 and x=N-1 belonging to the same feature) and confirm the periodic-adjusted centroid is the wrap-aware value, not the literal mean. This will exercise the currently-uncovered `AdjustCentroidsForPeriodicFaces` path.
4. **Inspect `6_6_stats_test_v2.tar.gz` and document provenance.** Determine how the exemplar `Centroids` array was generated (which DREAM3D version produced the file, what pipeline) and write an Oracle Provenance block. Also resolve the status of the orphan `6_6_find_feature_centroids.tar.gz`.
5. **Run the legacy comparison.** Use `compare-legacy-dream3d` to diff SIMPLNX vs. DREAM3D 6.5.172 on a shared toy dataset. Pay specific attention to (a) numerical differences caused by the Kahan-vs-naive summation choice, and (b) whether legacy supports the periodic boundary option at all (the conversion fixtures suggest it does not).
6. **Produce the Algorithm Relationship one-liner.** Tentative: *"Port — direct translation of the SIMPL `FindFeatureCentroids` filter; SIMPLNX adds Kahan compensated summation for numerical precision and an explicit `Is Periodic` option that the legacy filter does not appear to expose."*
7. **Archive everything** per `archive-filter-verification` for the OneDrive folder.

## Recommended Deviation entries (proposed, pending legacy comparison)

> **Deviation ID:** `ComputeFeatureCentroids-D1`
> **Filter UUID:** `c6875ac7-8bdd-4f69-b6ce-82ac09bd3421`
> **Symptom:** *Hypothesized.* For features with very large voxel counts and/or coordinate magnitudes, SIMPLNX centroid values may differ from SIMPL 6.5.172 in the lowest few bits of the `float32` result. SIMPLNX accumulates voxel-center coordinates with Kahan compensated summation (see `ComputeFeatureCentroids.cpp` lines ~61–83); SIMPL is suspected to use a naive running sum.
> **Root cause:** Numerical-precision improvement intentionally introduced in SIMPLNX. The Kahan algorithm reduces the cumulative roundoff error of the running sum; the naive algorithm is correct in expectation but loses precision proportional to N.
> **Affected users:** Anyone whose downstream analysis is sensitive to bit-exact reproducibility of centroid values across DREAM3D versions; anyone whose feature has on the order of millions of voxels or whose image has very large coordinate values.
> **Recommendation:** Trust SIMPLNX. The Kahan sum is more accurate. Legacy is not "wrong" in the algorithmic sense — it computes the same definition with more roundoff. Document the difference; do not patch legacy.
> **Status:** **Proposed and unverified** — this deviation is hypothesized from a code read of the SIMPLNX algorithm. Engineer must confirm by (a) reading the SIMPL `FindFeatureCentroids` source to verify it uses naive summation, and (b) running `compare-legacy-dream3d` to measure whether the predicted bit-level difference actually appears on real data. If both confirm, promote D1 from Proposed to Confirmed. If the SIMPL source already uses Kahan, retract D1.

> **Deviation ID:** `ComputeFeatureCentroids-D2` *(placeholder — likely)*
> **Filter UUID:** `c6875ac7-8bdd-4f69-b6ce-82ac09bd3421`
> **Symptom:** *Hypothesized.* When `IsPeriodic == true`, SIMPLNX produces wrap-aware centroids; SIMPL likely produces simple (non-wrap-aware) centroids because the option does not exist in the legacy filter.
> **Root cause:** Feature addition in SIMPLNX (the `IsPeriodic` BoolParameter and the call to `GeometryHelpers::Topology::AdjustCentroidsForPeriodicFaces`).
> **Affected users:** Anyone running periodic-boundary morphological analysis from a legacy SIMPL pipeline that has been auto-converted — the auto-converted pipeline will use the SIMPLNX default (`IsPeriodic == false`), matching legacy behavior, so the deviation only manifests when the SIMPLNX user explicitly opts in.
> **Recommendation:** Trust SIMPLNX. The new option is opt-in and the default preserves legacy behavior. No legacy patch needed.
> **Status:** **Proposed and unverified** — engineer must confirm the SIMPL filter does not expose this option.
