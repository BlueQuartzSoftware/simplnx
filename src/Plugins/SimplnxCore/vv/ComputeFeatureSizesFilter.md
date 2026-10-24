# V&V Report: ComputeFeatureSizesFilter

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `c666ee17-ca58-4969-80d0-819986c72485` |
| SIMPLNX Human Name | Compute Feature Sizes |
| DREAM.3D v6.5.171 equivalent | `FindSizes` (Statistics plugin, UUID `656f144c-a120-5c3b-bee5-06deab438588`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Nathan Young, 2026-06-10; second engineer: Michael Jackson, 2026-07-28 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Port of `FindSizes` in DREAM.3D v6.5.171, with changes to precision, parallel execution, and 2D size calculation. |
| Oracle (confirmed) | Class 1 analytical counts, areas, volumes, and equivalent diameters. Three base fixtures and three output-block variants encode expectations in `test/ComputeFeatureSizesTest.cpp`. All pass in CI. The flat-spacing case is an expected failure (`ComputeFeatureSizes-D3`). |
| Code paths enumerated | 25 of 45 paths exercised with test assertions. Gaps include input read errors, count overflow, and several cancellation paths. |
| Tests today | 17 test cases: 15 visible and 2 hidden OOC cases. Coverage includes analytical values, output writes, invalid inputs, cell tuple counts, archive regression, and SIMPL conversion. |
| Exemplar archive | `6_6_stats_test_v2.tar.gz` supplies the Small IN100 input and three regression arrays. Analytical fixtures are constructed in C++. |
| Legacy comparison | Not run — runtime comparison artifacts are not available. Source comparison with DREAM.3D v6.5.171 identifies D1–D3. |
| Bug flags | `ComputeFeatureSizes-D3` |
| V&V phase | Analytical fixtures, source comparison, and test inventory are complete. Runtime comparison artifacts and a separate oracle-design review record are required. `ComputeFeatureSizes-D3` remains open with an expected-failure test. |

## Summary

`ComputeFeatureSizesFilter` calculates feature voxel counts, areas or volumes, and equivalent diameters for Image and RectGrid geometries. Class 1 fixtures check the formulas, and the Small IN100 archive supplies a regression comparison. The V&V fixed two implementation defects; the 2D flat-spacing defect is not fixed.

## Algorithm Relationship

*Classification:* Port

*Evidence:* `SimplnxCoreLegacyUUIDMapping.hpp` maps `FindSizes` to this filter; both retain cell counting and circle/sphere formulas, with precision and execution changes documented in the sidecars.

## Oracle

*Class:* 1 (Analytical).

*Applied:* Count cells per feature, multiply by image cell size or sum RectGrid cell volumes, then use `2*sqrt(A/pi)` or `2*cbrt(3*V/(4*pi))`.

*Encoded:* `src/Plugins/SimplnxCore/test/ComputeFeatureSizesTest.cpp::SimplnxCore::ComputeFeatureSizes: Valid: Image 2D` and five companion `Valid:` cases use three base fixtures; `SimplnxCore::ComputeFeatureSizesScanline: bounded output writes` adds three variants. All pass in CI. The flat-spacing case is an expected failure (`[!shouldfail]`) for `ComputeFeatureSizes-D3`.

*Second-engineer review:* The report records Michael Jackson's sign-off on 2026-07-28. A separate record of the oracle-design review is required.

## Bugs found and fixed

All defects in this table are fixed. The fixes will be in the DREAM3D-NX release after v7.4.2. DREAM3D-NX v7.4.2 contains the fix for `ComputeFeatureSizes-D4`. DREAM3D-NX v7.4.2 contains the fix for `ComputeFeatureSizes-D5`.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `ComputeFeatureSizes-D4` | RectGrid reduction applied Kahan compensation to the accumulated total instead of the incoming partial sum. | None. DREAM3D-NX v7.0.0 through v7.4.2 were not affected. DREAM.3D v6.5.171 was not affected. | The filter subtracts the compensation from the incoming partial sum before adding it to the accumulated total. |
| `ComputeFeatureSizes-D5` | Parallel workers could read nonresident FeatureIds without a store-residency scheduling guard. | None. DREAM3D-NX v7.0.0 through v7.4.2 were not affected. DREAM.3D v6.5.171 was not affected. | Direct execution checks FeatureIds residency before parallel scheduling. |

## Code path coverage

25 of 45 paths exercised with test assertions. Gaps include input read errors, count overflow, and cancellation during input processing.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeFeatureSizes.cpp` (25 lines); `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeFeatureSizesDirect.cpp` (481 lines); `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeFeatureSizesScanline.cpp` (449 lines); `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ComputeFeatureSizesFilter.cpp` (206 lines).

Test names below omit the `SimplnxCore::ComputeFeatureSizes: ` prefix, except where `ComputeFeatureSizesScanline` is stated. The scenario harness selects Direct or Scanline according to the test configuration.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Preflight | Two or more Image dimensions are less than 2 → error. | `Invalid: Preflight Failure` — four dimension combinations. |
| 2 | Preflight | Valid Image dimensions → output actions. | `Valid: Image 2D`; `Valid: Image Stack 3D`. |
| 3 | Direct validation | FeatureId ≥ feature tuple count → error. | `Invalid: Execution Failure` — ID 10 with four feature tuples. |
| 4 | Direct validation | Valid FeatureIds → geometry dispatch. | `Valid: Image Stack 3D`; `Valid: Rectilinear Grid`. |
| 5 | Direct dispatch | Image geometry. | `Valid: Image 2D`; `Valid: Image Stack 3D`. |
| 6 | Direct dispatch | RectGrid geometry. | `Valid: Rectilinear Grid`. |
| 7 | Direct dispatch | Other geometry → no operation. | *Not directly tested. The selection parameter accepts only Image and RectGrid.* |
| 8 | Direct Image | One unit dimension → area and circular diameter. | `Valid: Image 2D` — flat spacing is 1. |
| 9 | Direct Image | All dimensions greater than 1 → volume and spherical diameter. | `Valid: Image Stack 3D`. |
| 10 | Direct Image | Feature count exceeds INT32_MAX in 2D or 3D → error. | *Not directly tested. No fixture supplies more than INT32_MAX cells in one feature.* |
| 11 | Direct Image | SaveElementSizes=false → no new element-size array. | `Valid: Image 2D`; `Valid: Image Stack 3D`. |
| 12 | Direct Image | SaveElementSizes=true → generate element sizes. | `Valid: Image 2D with Element Sizes`; `Valid: Image Stack 3D with Element Size`. |
| 13 | Direct Image | Cancel during counting, reduction, or output → return. | *Not directly tested. No test injects cancellation into Direct Image execution.* |
| 14 | Direct RectGrid | Generate cell volumes. | `Valid: Rectilinear Grid with Element Size`. |
| 15 | Direct RectGrid | Count cells and combine float64 compensated sums. | `Valid: Rectilinear Grid` — asserts three feature volumes and diameters. |
| 16 | Direct RectGrid | Feature count exceeds INT32_MAX → error. | *Not directly tested. No fixture supplies more than INT32_MAX cells in one feature.* |
| 17 | Direct RectGrid | SaveElementSizes=false → delete element sizes. | `Valid: Rectilinear Grid`. |
| 18 | Direct RectGrid | SaveElementSizes=true → retain element sizes. | `Valid: Rectilinear Grid with Element Size`. |
| 19 | Direct RectGrid | Cancel during counting, reduction, or output → return. | *Not directly tested. No test injects cancellation into Direct RectGrid execution.* |
| 20 | Direct validation | Negative FeatureId → error. | *Not directly tested. The invalid-ID fixture uses a positive ID above the limit.* |
| 21 | Direct scheduling | Nonresident FeatureIds → serial scheduling. | *Not directly tested. The real OOC fixtures require Scanline execution.* |
| 22 | Direct output | Background feature or unused feature → zero output. | *Not directly tested. The Direct analytical fixtures do not assert these entries.* |
| 23 | Scanline Image | 2D area and circular diameter; SaveElementSizes=false. | `SimplnxCore::ComputeFeatureSizesScanline: bounded output writes` — Image 2D. |
| 24 | Scanline Image | 3D volume and spherical diameter; SaveElementSizes=false. | `SimplnxCore::ComputeFeatureSizesScanline: bounded output writes` — Image 3D. |
| 25 | Scanline RectGrid | Cell-volume sums and spherical diameter; SaveElementSizes=false. | `SimplnxCore::ComputeFeatureSizesScanline: bounded output writes` — RectGrid. |
| 26 | Scanline element sizes | SaveElementSizes=true for 2D Image, 3D Image, and RectGrid. | `Valid: Image 2D with Element Sizes`; `Valid: Image Stack 3D with Element Size`; `Valid: Rectilinear Grid with Element Size`. |
| 27 | Scanline validation | Image FeatureId ≥ feature tuple count → error. | `Invalid: Execution Failure`. |
| 28 | Scanline validation | RectGrid FeatureId ≥ feature tuple count → error. | *Not directly tested. The invalid-ID fixture uses Image geometry only.* |
| 29 | Scanline validation | Negative Image or RectGrid FeatureId → error. | *Not directly tested. No negative-ID fixture exists in this test file.* |
| 30 | Scanline input | Image FeatureId bulk read fails → error. | *Not directly tested. The test stores inject output errors only.* |
| 31 | Scanline input | RectGrid FeatureId or element-size bulk read fails → error. | *Not directly tested. The test stores inject output errors only.* |
| 32 | Scanline input | Full 262,144-cell read followed by another input chunk. | *Not directly tested. The analytical fixtures expand feature outputs, not input cell counts.* |
| 33 | Scanline input | Cancel during Image or RectGrid input scan → return. | *Not directly tested. Cancellation is injected after an output block.* |
| 34 | Scanline output | Write a full block and a two-feature tail. | `SimplnxCore::ComputeFeatureSizesScanline: bounded output writes` — all three geometries. |
| 35 | Scanline output | NumElements, Volumes, or EquivalentDiameters write fails → stop. | `SimplnxCore::ComputeFeatureSizesScanline: output write failures stop later writes` — one variant per array. |
| 36 | Scanline output | Cancel after a completed block; empty flush → success. | `SimplnxCore::ComputeFeatureSizesScanline: cancellation preserves completed output blocks`. |
| 37 | Scanline output | Cancel with a partially filled block → flush completed features. | *Not directly tested. The cancellation fixture stops at a block boundary.* |
| 38 | Scanline output | Count exceeds INT32_MAX → flush then error. | *Not directly tested. No count-overflow fixture exists.* |
| 39 | Scanline output | Background and unused features stay zero. | `SimplnxCore::ComputeFeatureSizesScanline: bounded output writes` — background arrays and unused-feature counts and sizes. |
| 40 | Scanline stores | Real HDF5-OOC input and outputs; short and multiple output blocks. | `real OOC output writes preserve analytical values`; `real OOC multi-block output writes` — hidden cases. |
| 41 | Scanline dispatch | Other geometry → no operation. | *Not directly tested. The selection parameter accepts only Image and RectGrid.* |
| 42 | Element sizes | Image or RectGrid generation fails → return the error. | *Not directly tested. All geometry fixtures have valid element-size inputs.* |
| 43 | Element sizes | Reuse an existing element-size array. | *Not directly tested. No fixture explicitly checks reuse of an existing array.* |
| 44 | Output limits | No feature after background → empty output loop. | *Not directly tested. Every fixture has at least one non-background feature.* |
| 45 | Preflight | FeatureIds tuple count differs from the selected geometry cell count → error `-74771`. | `Geometry cell tuple count` — Image and RectGrid geometries with 12 cells reject 0, 11, and 13 tuples. |

## Test inventory

All names are in `src/Plugins/SimplnxCore/test/ComputeFeatureSizesTest.cpp`. Dynamic variants are listed in Notes.

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::ComputeFeatureSizesScanline: bounded output writes` | kept | Variants: Image 2D, Image 3D, RectGrid. Checks analytical values, one 262,144-feature block plus a two-feature tail, and no scalar writes. The 2D variant pins the current slab formula, not true area. |
| `SimplnxCore::ComputeFeatureSizesScanline: output write failures stop later writes` | kept | Variants: NumElements, Volumes, EquivalentDiameters. Injects each output write error and checks that later writes stop. |
| `SimplnxCore::ComputeFeatureSizesScanline: cancellation preserves completed output blocks` | kept | Cancels after the first complete output block. Checks completed values and zero values in the unwritten tail. |
| `SimplnxCore::ComputeFeatureSizes: real OOC output writes preserve analytical values` | kept | Hidden OOC case. Checks seven feature tuples on real HDF5-OOC stores; skips the body in an in-core build without that store. |
| `SimplnxCore::ComputeFeatureSizes: real OOC multi-block output writes` | kept | Hidden OOC case. Requires HDF5-OOC and checks values across an output-block boundary. |
| `SimplnxCore::ComputeFeatureSizes: Valid: Image 2D` | kept | Checks counts, areas, and circular diameters for three features; confirms that element sizes are absent. Float comparisons now use relative tolerance `1e-6` for platform and reduction-order variation. |
| `SimplnxCore::ComputeFeatureSizes: 2D area excludes the flat-dimension spacing` | new-for-V&V | Expected failure (`[!shouldfail]`). Variants: flat Z, flat X, flat Y. Expects area 24 with flat spacing 5; both algorithms calculate 120 (`ComputeFeatureSizes-D3`). |
| `SimplnxCore::ComputeFeatureSizes: Valid: Image 2D with Element Sizes` | kept | Checks the same 2D values and the presence of the element-size array. Float comparisons now use relative tolerance `1e-6` for platform and reduction-order variation. |
| `SimplnxCore::ComputeFeatureSizes: Valid: Image Stack 3D` | kept | Checks counts, volumes, and spherical diameters for three features; confirms that element sizes are absent. Float comparisons now use relative tolerance `1e-6` for platform and reduction-order variation. |
| `SimplnxCore::ComputeFeatureSizes: Valid: Image Stack 3D with Element Size` | kept | Checks the same 3D values and the presence of the element-size array. Float comparisons now use relative tolerance `1e-6` for platform and reduction-order variation. |
| `SimplnxCore::ComputeFeatureSizes: Valid: Rectilinear Grid` | kept | Checks counts, volumes, and spherical diameters on a nonuniform grid; confirms deletion of element sizes. Float comparisons now use relative tolerance `1e-6` for platform and reduction-order variation. |
| `SimplnxCore::ComputeFeatureSizes: Valid: Rectilinear Grid with Element Size` | kept | Checks the same RectGrid values and retention of element sizes. Float comparisons now use relative tolerance `1e-6` for platform and reduction-order variation. |
| `SimplnxCore::ComputeFeatureSizes: Invalid: Execution Failure` | kept | Uses FeatureId 10 with four feature tuples. Expects an execution error. |
| `SimplnxCore::ComputeFeatureSizes: Invalid: Preflight Failure` | kept | Checks rejection of dimensions (5,1,1), (1,5,1), (1,1,5), and (1,1,1). |
| `SimplnxCore::ComputeFeatureSizes: Legacy: Small IN100 Test` | kept | Loads `6_6_stats_test_v2.tar.gz` and compares all three generated arrays with archived arrays. This is a regression check, not an independent oracle. |
| `SimplnxCore::ComputeFeatureSizesFilter: SIMPL Backwards Compatibility` | kept | Variants: `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)`. Checks filter identity, empty comments, and seven converted parameter values. |
| `SimplnxCore::ComputeFeatureSizes: Geometry cell tuple count` | new-for-V&V | Image and RectGrid geometries have 12 cells. Rejects FeatureIds with 0, 11, or 13 tuples with error `-74771`; checks the array path and both counts in the message. Accepts 12 tuples in another Attribute Matrix with a different tuple shape. |

## Exemplar archive

- **Archive:** `6_6_stats_test_v2.tar.gz`
- **SHA512:** `e84999dec914d81efce4fc4237c49c9bf32e48381b1e79f58aa4df934f0d7606cd7a948f9a5e7b17a126a7944cc531b531cfdc70756ca3e2207b20734e089723`
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/ComputeFeatureSizesFilter.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/ComputeFeatureSizesFilter.md` for the root cause, affected users, and recommendation for each deviation. Part 1 contains the following entries.

| Deviation | Observed difference |
|-----------|---------------------|
| `ComputeFeatureSizes-D1` | RectGrid feature volumes use float64 compensated sums instead of naive float32 sums. |
| `ComputeFeatureSizes-D2` | Equivalent diameters use float64 roots and unrounded feature sizes instead of the stored float32 sizes. |
| `ComputeFeatureSizes-D3` | A 2D image includes the flat spacing in its area value and then uses the circle diameter formula. DREAM.3D v6.5.171 excludes that spacing. |
