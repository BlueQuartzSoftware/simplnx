# Retroactive V&V: ComputeFeatureSizesFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `c666ee17-ca58-4969-80d0-819986c72485` |
| SIMPLNX ClassName | `ComputeFeatureSizesFilter` |
| SIMPLNX Human Name | Compute Feature Sizes |
| SIMPL UUID | *(TBD — confirm in legacy SIMPL repo)* |
| SIMPL ClassName | *(TBD — likely `FindSizes` based on the SIMPL parameter key `FeatureAttributeMatrixName`/`SaveElementSizes`)* |
| SIMPL Human Name | *(TBD — historically "Find Feature Sizes")* |
| Plugin | SimplnxCore |

### Source files scanned

- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ComputeFeatureSizesFilter.{hpp,cpp}`
- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeFeatureSizes.{hpp,cpp}`
- `src/Plugins/SimplnxCore/test/ComputeFeatureSizesTest.cpp`
- `src/Plugins/SimplnxCore/test/simpl_conversion/6_5/ComputeFeatureSizesFilter.json`
- `src/Plugins/SimplnxCore/test/simpl_conversion/6_4/ComputeFeatureSizesFilter.json`
- `src/Plugins/SimplnxCore/docs/ComputeFeatureSizesFilter.md`

## Algorithm Relationship

- **Tentative classification:** **Port with substantive enhancements.** The filter started life in SIMPLNX as a direct translation of the legacy SIMPL `FindSizes` filter (UUID preserved). PR #1540 then layered on a meaningful set of correctness/feature changes: RectGrid geometry support, Kahan summation for RectGrid volume accumulation, `Result<>`-based geometry error handling, integer-overflow guards on per-feature voxel counts, and a hard preflight rule that the input ImageGeom must have at least 2 non-degenerate dimensions.
- **Evidence:** PR #1301 introduced an Algorithm class (extraction-only refactor, no algorithm change). PR #1540 then ~doubled the algorithm file (124 → 394 lines) and added the RectGrid execution branch. PR #1590 trimmed dead 2D-handling code as part of a wider 2D-image standardization sweep.
- **Action required:** Confirm by reading the legacy SIMPL `FindSizes` source to (a) verify the SIMPLNX 3D ImageGeom path is byte-equivalent, (b) confirm the SIMPL legacy did *not* use Kahan summation for RectGrid (this is a candidate Deviation), and (c) confirm whether the integer-overflow check existed in legacy.

## PRs inspected (since 2025-10-01)

> Pruned: pure-style/repo-wide refactor PRs (#1457 static-inline, #1439 multi-dim StringArray, #1524 tag fix, #1538 zlib extraction) are listed at the bottom of this section but not detailed individually — they did not change behavior of this filter.

### PR #1301 — *"ENH: Add missing algorithm classes to some filters"* — merged 2026-01-08

- **Files in this filter:** filter (.cpp), algorithm (.hpp, .cpp) — algorithm files newly created
- **Diff size:** 3 files, +257 / -154 lines (algorithm extraction)
- **Change nature:** **Refactor — algorithm extraction.** Moved the filter's `executeImpl` body out of `ComputeFeatureSizesFilter.cpp` into a dedicated `ComputeFeatureSizes` algorithm class under `Algorithms/`. This is the architectural style change called out in the project CLAUDE.md (issue #1284). The filter shrank by 154 lines while the new algorithm class added 246. No algorithmic change is intended in this PR — it is a relocation refactor.
- **V&V content:** None directly. Sets up the canonical Filter/Algorithm split that subsequent PRs (notably #1540) build on.

### PR #1535 — *"ENH: Remove redundant preflight checks that are already done in the parameter"* — merged 2026-02-18

- **Files in this filter:** filter (.cpp) only
- **Diff size:** 1 file, +4 / -28 lines
- **Change nature:** Cleanup. Stripped explicit existence checks for parameters that are already validated by their selection-parameter type (consistent with project CLAUDE.md guidance: don't null-check what `GeometrySelectionParameter`/`ArraySelectionParameter` already validates). No behavior change.
- **V&V content:** None.

### PR #1540 — *"REV/ENH: Compute Feature Sizes and Geometry Error Handling"* — merged 2026-02-20

- **Files in this filter:** docs (.md), algorithm (.cpp), filter (.cpp), test (.cpp)
- **Diff size (scoped to this filter):** 4 files, **+973 / -150 lines** — algorithm `+270/-124`, test `+639/-17`, filter `+48/-8`, docs `+16/-1`
- **Change nature:** **Major revision.** This is the largest PR in this filter's history and touches almost every dimension of behavior:
  1. **Adds RectGrid geometry support** — previously ImageGeom-only; now dispatches by geometry type to a separate `ProcessRectGridGeom` path that uses the rect grid's per-cell `elementSizes`.
  2. **Kahan summation** in the RectGrid volume accumulation to reduce float drift across many cells.
  3. **Integer-overflow guard** — emits error `k_BadFeatureCount = -78231` if any feature has more voxels than `int32` max.
  4. **Preflight tightening** — error `k_InvalidInputDimensions = -74770` when an ImageGeom has more than one degenerate (size-1) dimension; the algorithm comment block explains the orientation-ambiguity reason.
  5. **2D vs 3D selection** — explicitly checks for one degenerate ImageGeom dimension and switches to area / Equivalent Circular Diameter instead of volume / ESD.
  6. **Throttled progress messaging + cancel checks** added to both processing paths.
  7. **Documentation expanded** to describe the 2D/3D selection rule and the orientation-ambiguity caveat.
  8. **Restricted FeatureIds typing** to `int32` with single component in the parameter spec.
- **V&V content:** **High.** Test file expanded by +639 lines: 2D-Image, 2D-Image-with-element-sizes, 3D-Image-Stack, 3D-Image-Stack-with-element-sizes, RectGrid, RectGrid-with-element-sizes, plus invalid-execution and invalid-preflight cases were added. Validation values are hand-derived (in test source comments) including expected `numElements`, `volumes`/`areas`, and `equivalentDiameters` to multiple decimal places — this functionally serves as an inline analytical oracle. The Small IN100 legacy comparison test is also present in this PR's history (post-condition).
- **Note:** Several Deviation candidates live in this PR — Kahan summation, the +1 overflow guard, and the new preflight error all are SIMPLNX-only behaviors that the legacy SIMPL filter likely lacks.

### PR #1543 — *"DOC: Update pipeline references in each of the documentation files"* — merged 2026-02-24

- **Files in this filter:** docs (.md) only, +2 / -3 lines
- **Change nature:** Doc hygiene — updated the "Example Pipelines" list. Not algorithmic.
- **V&V content:** None.

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** test (.cpp) +49 lines, plus two new fixture files
  - `test/simpl_conversion/6_4/ComputeFeatureSizesFilter.json` (25 lines)
  - `test/simpl_conversion/6_5/ComputeFeatureSizesFilter.json` (26 lines)
- **Change nature:** **Test addition.** Added a per-filter SIMPL→SIMPLNX backwards-compatibility test that exercises both SIMPL 6.4 (Filter_Name fallback) and 6.5 (UUID-mapped) pipeline conversion paths via `DYNAMIC_SECTION`. Test name: `"SimplnxCore::ComputeFeatureSizesFilter: SIMPL Backwards Compatibility"`. Verifies that the seven parameter keys (`save_element_sizes`, `input_image_geometry_path`, `feature_ids_path`, `feature_attribute_matrix_path`, `equivalent_diameters_name`, `num_elements_name`, `volumes_name`) all round-trip correctly from the legacy JSON.
- **V&V content:** **Pipeline-conversion correctness only** — the test verifies that opening a legacy SIMPL pipeline in DREAM3DNX produces a filter instance with the right parameter values. It does **not** verify that the filter's *output* matches legacy. That latter step is still missing.

### PR #1590 — *"ENH: Standardize 2D Image Handling"* — merged 2026-04-23

- **Files in this filter:** algorithm (.cpp) +1 / -8, test (.cpp) +1 / -1
- **Change nature:** Cleanup riding on a wider 2D-image-handling standardization that introduced shared `NeighborUtilities` for dimensionality-aware neighbor traversal. The algorithm trimmed 7 lines in the 2D handling block and the test had a one-line touch-up. The 2D code path semantics established in PR #1540 were preserved.
- **V&V content:** Low. The companion changes to `ImageGeom` and `IdentifySample` in this PR carried a 2D bug fix; the FeatureSizes touch is incidental. Worth noting because the core `ImageGeom::findElementSizes()` semantics were locked in by tests in this PR — and `findElementSizes` is what `ComputeFeatureSizes` depends on for the "Generate Missing Element Sizes" option.

### Pruned PRs (touched the file but not behaviorally relevant to this filter)

| PR | Subject | Why pruned |
|---|---|---|
| #1439 | Multi-Dimensional Tuple Support for StringArray and NeighborList | API change, no per-filter behavior change |
| #1457 | Clean up 'static inline' from filter headers | Style |
| #1524 | Fixed filter tags to consistently use the full filter name | Test cosmetic |
| #1538 | Replace cmake subprocess tar.gz extraction with zlib | Test infrastructure |

## Test coverage detected

`ComputeFeatureSizesTest.cpp` contains 9 `TEST_CASE`s:

1. `SimplnxCore::ComputeFeatureSizes: Valid: Image 2D` — 2D ImageGeom (5×5×1), three features, hand-derived expected `numElements`, areas, and ECDs.
2. `SimplnxCore::ComputeFeatureSizes: Valid: Image 2D with Element Sizes` — same as #1 but verifies that `SaveElementSizes=true` causes the geometry to retain its element-sizes array.
3. `SimplnxCore::ComputeFeatureSizes: Valid: Image Stack 3D` — 3D ImageGeom (5×5×5), hand-derived expected `numElements`, volumes, ESDs.
4. `SimplnxCore::ComputeFeatureSizes: Valid: Image Stack 3D with Element Size` — as #3 plus the element-sizes retention check.
5. `SimplnxCore::ComputeFeatureSizes: Valid: Rectilinear Grid` — RectGridGeom (4×4×4) with non-uniform x/y/z bounds; exercises the Kahan-summation path with hand-derived expected values.
6. `SimplnxCore::ComputeFeatureSizes: Valid: Rectilinear Grid with Element Size` — as #5 plus element-sizes retention.
7. `SimplnxCore::ComputeFeatureSizes: Invalid: Execution Failure` — sets a feature ID outside the feature-attribute-matrix bounds and expects an invalid execute result.
8. `SimplnxCore::ComputeFeatureSizes: Invalid: Preflight Failure` — exercises all four "two or more degenerate dimensions" cases (5×1×1, 1×5×1, 1×1×5, 1×1×1) and expects preflight failure.
9. `SimplnxCore::ComputeFeatureSizes: Legacy: Small IN100 Test` — uses `6_6_stats_test_v2.dream3d` exemplar and compares the filter's `Volumes`, `EquivalentDiameters`, and `NumElements` against named exemplar arrays in the file.
10. `SimplnxCore::ComputeFeatureSizesFilter: SIMPL Backwards Compatibility` — SIMPL 6.4 + 6.5 conversion paths via DYNAMIC_SECTION *(added by PR #1588)*

Tests 1–6 are essentially analytical-oracle tests (hand-computed expected values directly in the test source). Test 9 is the closest existing legacy-vs-NX comparison. Test 10 is conversion-only.

## Exemplar archive

- **Archive name:** `6_6_stats_test_v2.tar.gz`
- **SHA512:** `e84999dec914d81efce4fc4237c49c9bf32e48381b1e79f58aa4df934f0d7606cd7a948f9a5e7b17a126a7944cc531b531cfdc70756ca3e2207b20734e089723`
- **Referenced in:** `src/Plugins/SimplnxCore/test/CMakeLists.txt`
- **Provenance:** *(TBD — engineer must inspect the archive to determine how the exemplar was generated and whether an Oracle Provenance block exists in any ReadMe inside it.)* The `6_6_` prefix indicates a legacy DREAM3D-6.6 origin; the "stats_test" portion suggests this archive is shared across multiple statistics filters (FindSizes, FindNeighbors, FindShapes, etc.) so its provenance should be documented once and referenced by each filter that uses it.
- **Action required:** Download the archive locally and inspect for: an inner `ReadMe.md`, the source `.dream3d` files used to generate the exemplars, the SIMPL pipeline files that produced the exemplar `Volumes` / `EquivalentDiameters` / `NumElements` arrays, and any provenance notes. Promote this content into the verification archive ReadMe per Step 0's Oracle Provenance policy.

## Oracle classification (tentative)

- **Recommended class:** **1 (Analytical).**
- **Rationale:** Every output produced by this filter is closed-form from voxel counts and cell sizes:
  - `NumElements[f]` = count of voxels with FeatureId = f. Trivially analytical.
  - For ImageGeom: `Volume[f] = NumElements[f] · sx · sy · sz`; `Area[f]` for the 2D case = `NumElements[f] · sx · sy · sz` (with the degenerate-dim spacing acting as a unit factor — see the algorithm comment block for the orientation caveat).
  - `ESD[f] = 2 · cbrt(Volume[f] · 3 / (4π))`; `ECD[f] = 2 · sqrt(Area[f] / π)`. Both are textbook geometry.
  - For RectGridGeom: `Volume[f] = Σ elementSize[c]` over voxels c with FeatureId = f, with Kahan compensated summation. Same closed form, just over heterogeneous cell sizes.
- The hand-derived expected values already in tests 1–6 are themselves the analytical oracle — they should be lifted out of the test bodies into the report's verification block to make the oracle explicit.
- **Optional Class 4 (Invariant) overlay:** `sum(NumElements[1..n]) ≤ totalVoxels`, `Volume[f] = NumElements[f] · voxelVolume` (ImageGeom), `ESD/ECD ≥ 0`. Already implicitly checked.
- **No paper reference is needed** — equivalent spherical/circular diameter formulas are standard. (If desired, ASTM E112 grain-size methodology is the closest external citation.)
- **Action required:** Developer to defend or replace the Class-1 recommendation, and to decide whether to lift the in-test hand-derived oracle values into a formal verification document.

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | Not visible from PR history | No PR explicitly performs the line-by-line review. PR #1540's commit message lists "Extensive Code Documentation" which is the closest signal. |
| Code path coverage (algorithmic) | Good | Tests 1–6 cover ImageGeom-2D, ImageGeom-3D, RectGrid, each with and without `SaveElementSizes`. |
| Code path coverage (error handling) | Good | Test 7 hits the per-feature integer-overflow / index-out-of-range error; test 8 hits all 4 preflight degenerate-dimension cases. |
| Code path coverage (SIMPL conversion) | Good | PR #1588 added SIMPL 6.4 + 6.5 conversion test. |
| Exemplar data in Data_Archive | **Yes** | `6_6_stats_test_v2.tar.gz` referenced in test/CMakeLists.txt. Shared archive across statistics filters. |
| Exemplar provenance documented | Unknown | TBD by inspecting archive contents. The `6_6_` prefix implies a DREAM3D 6.6 origin which would itself be Oracle Class 5 (legacy) — this needs to be made explicit. |
| Oracle class recorded | **No** | This document is the first to propose one (Class 1). |
| Toy data / independent expected output (Step 0 c) | Partial | The hand-derived expected values in tests 1–6 are an inline analytical oracle but are not extracted into a standalone document/script. |
| Legacy comparison report (Step 0 e) | Partial | Test 9 (`Legacy: Small IN100 Test`) uses a legacy-derived exemplar but is not a formal `compare-legacy-dream3d` run with a written diff report. |
| Deviation entries (`ComputeFeatureSizes-D<N>`) | None | Not yet written. PR #1540 introduced at least three behaviors that may differ from legacy (Kahan summation, overflow guard, degenerate-dimension preflight). |
| Documentation currency | Probably current | Substantially updated by PR #1540 (2D/3D rule + orientation-ambiguity explanation) and PR #1543 (pipeline list). Has not yet been through `review-filter-docs`. |
| Verification archive (OneDrive) | No | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

1. **Confirm the oracle.** Class 1 (Analytical) is the recommended starting point. Lift the hand-derived expected values currently embedded in tests 1–6 into a standalone verification doc and reference it from this report.
2. **Promote the inline analytical assertions into named oracle assertions.** The current per-feature `REQUIRE(numElements.getValue(1) == 11)` style is already the right shape; rename / wrap them so a reader can identify them as Class-1 oracle checks rather than ordinary regression checks.
3. **Inspect `6_6_stats_test_v2.tar.gz` and document provenance.** This archive is shared across multiple Find*/Compute* statistics filters — provenance written once will benefit several reports. Determine which legacy SIMPL pipeline produced the `Volumes` / `EquivalentDiameters` / `NumElements` arrays used as exemplars in test 9.
4. **Run the legacy comparison.** Use `compare-legacy-dream3d` to diff SIMPLNX vs. DREAM3D 6.5.172 on the same toy 2D image, 3D stack, and RectGrid datasets used in tests 1, 3, 5. Expected outcome: at minimum two Deviation entries (`-D1` Kahan summation difference on RectGrid; `-D2` SIMPLNX-only overflow guard; possibly `-D3` SIMPLNX-only degenerate-dimension preflight).
5. **Confirm legacy SIMPL UUID and ClassName.** Fill in the metadata table.
6. **Produce the Algorithm Relationship one-liner.** Tentative: *"Port + substantive enhancements — original ImageGeom path is a translation of SIMPL `FindSizes`; PR #1540 added RectGrid support, Kahan summation, integer-overflow guard, and a degenerate-dimension preflight rule."*
7. **Archive everything** per `archive-filter-verification` for the OneDrive folder.

## Recommended Deviation entries (proposed, pending legacy comparison)

> **Deviation ID:** `ComputeFeatureSizes-D1`
> **Filter UUID:** `c666ee17-ca58-4969-80d0-819986c72485`
> **Symptom:** On RectGridGeom inputs with many small cells of varying size, SIMPLNX `Volumes` differ from SIMPL 6.5.172 in the low-order bits.
> **Root cause:** SIMPLNX uses Kahan compensated summation when accumulating per-cell `elementSize` into per-feature volume (introduced PR #1540). Legacy SIMPL uses a naive running sum, which accumulates rounding error proportional to feature voxel count.
> **Affected users:** Anyone comparing per-feature volumes across SIMPL and SIMPLNX on RectGrid inputs. Image-stack inputs are unaffected (closed form, no summation).
> **Recommendation:** Trust SIMPLNX. The Kahan path is numerically more accurate. Difference is expected to be sub-ULP for typical feature sizes.
> **Status:** Proposed — pending verification that 6.5.172 actually exhibits the divergence (run the comparison).

> **Deviation ID:** `ComputeFeatureSizes-D2`
> **Filter UUID:** `c666ee17-ca58-4969-80d0-819986c72485`
> **Symptom:** On a synthetic input with a single feature larger than 2,147,483,647 voxels, SIMPLNX errors out (`-78231 "Feature N contains more voxels than the 32-bit integer limit"`); SIMPL 6.5.172 silently overflows the `NumElements` int32 store and produces a negative voxel count, which then propagates as a garbage volume / negative-radius cube root.
> **Root cause:** SIMPLNX added an explicit overflow guard against the int32 max in PR #1540 (`k_MaxVoxelCount = std::numeric_limits<int32>::max()`). Legacy did not.
> **Affected users:** Synthetic-microstructure or full-wafer datasets where a single feature can exceed ~2.1B voxels. Real-world EBSD users essentially never hit this.
> **Recommendation:** Trust SIMPLNX. Legacy was silently wrong. Either patch legacy to also error, or document this as a SIMPLNX-only correctness improvement.
> **Status:** Proposed — pending verification that 6.5.172 actually exhibits the silent overflow (build a synthetic test).

> **Deviation ID:** `ComputeFeatureSizes-D3`
> **Filter UUID:** `c666ee17-ca58-4969-80d0-819986c72485`
> **Symptom:** On an ImageGeom with two or three degenerate (size-1) dimensions (e.g., 5×1×1 or 1×1×5), SIMPLNX preflight fails with error `-74770 "This filter requires at least 2 valid dimensions in the image geom..."`; SIMPL 6.5.172 either runs and produces orientation-ambiguous area values or runs and produces nonsense.
> **Root cause:** SIMPLNX added a preflight check (PR #1540) refusing to compute area when there is no unambiguous way to choose which two dimensions span the area. The algorithm comment block explains the rationale at length.
> **Affected users:** Anyone whose pipeline accidentally feeds a degenerate-dimension geometry. Practically rare.
> **Recommendation:** Trust SIMPLNX. The preflight failure is a correctness improvement — the legacy result was undefined. Document the change in the user-facing release notes.
> **Status:** Proposed — pending verification of legacy behavior.
