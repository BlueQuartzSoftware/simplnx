# V&V Report: ComputeKernelAvgMisorientationsFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `61cfc9c1-aa0e-452b-b9ef-d3b9e6268035` |
| SIMPLNX Human Name | Compute Kernel Average Misorientations |
| DREAM.3D v6.5.171 equivalent | `FindKernelAvgMisorientations` (OrientationAnalysis plugin, UUID `88d332c1-cf6c-52d3-a38d-22f6eae19fa6`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-07-16; second engineer: Jared Duffey, 2026-07-28 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Minor changes to `FindKernelAvgMisorientations` in DREAM.3D v6.5.171. SIMPLNX corrects the X kernel bound, improves precision and adds an optional same-phase mode across feature boundaries. |
| Oracle (confirmed) | Class 1 analytical averages and Class 4 invariants are encoded in `test/ComputeKernelAvgMisorientationsTest.cpp`. All pass in CI. |
| Code paths enumerated | 28 of 45 exercised. The table identifies gaps in fallback modes, defensive guards, cancellation and transfer failures. |
| Tests today | 17 test cases cover analytical averages, invariants, input bounds, memory planning and SIMPL conversion. All pass in CI. |
| Exemplar archive | Current tests construct their inputs and expected values in C++. The former KAM exemplar in `6_6_stats_test_v2.tar.gz` is retired for this filter. |
| Legacy comparison | Not run on the current algorithm. The DREAM.3D v6.5.171 comparison dated 2026-09-17 predates the current implementation. |
| Bug flags | None. |
| V&V phase | Analytical checks, regression checks and second-engineer review are complete. |

## Summary

`ComputeKernelAvgMisorientationsFilter` averages each valid cell's misorientation to admitted kernel neighbors, including the focal cell, and writes the result in degrees. The default mode admits the same feature; the optional mode admits positive feature IDs in the same phase. Class 1 averages and Class 4 invariants verify both modes; all pass in CI.

## Algorithm Relationship

*Classification:* Minor changes

*Evidence:* `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/OrientationAnalysisLegacyUUIDMapping.hpp` maps the legacy UUID to this filter; both implementations average admitted kernel neighbors, while SIMPLNX corrects the X bound and adds same-phase admission.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant) companions.

*Applied:* Pure Z rotations give cubic misorientations of `|Δφ1|` for differences at most 45°; hand averages include the focal zero, and invariants check range, background zeros and mode equivalence.

*Encoded:* `src/Plugins/OrientationAnalysis/test/ComputeKernelAvgMisorientationsTest.cpp` — six `Class 1` fixtures, `Bounded Cache Full-Depth Kernel`, `Class 4 - Invariants` and `Class 4 - Mode Equivalence on Single Feature`. All pass in CI.

*Second-engineer review:* Jared Duffey, 2026-07-28.

## Bugs found and fixed

All defects in this table are fixed. The fixes will be in the DREAM3D-NX release after v7.4.2. DREAM3D-NX v7.4.2 contains the fix for `ComputeKernelAvgMisorientationsFilter-D1`. DREAM3D-NX v7.0.0 contains the fix for `ComputeKernelAvgMisorientationsFilter-D2`.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `ComputeKernelAvgMisorientationsFilter-D1` | Cubic misorientation loses precision near zero and can inflate the kernel average. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | EbsdLib calculates the reduced angle with explicit quaternion vector components and `2*atan2`. |
| `ComputeKernelAvgMisorientationsFilter-D2` | The legacy X loop uses the Z radius as its upper bound and changes asymmetric kernels. | DREAM.3D v6.5.171 was affected. DREAM3D-NX v7.0.0 through v7.4.2 were not affected. | Each axis uses its own radius and clamps the kernel to the image. |
| `ComputeKernelAvgMisorientationsFilter-D4` | A participating phase or Laue index can access outside its array. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. | The filter rejects an invalid phase with `-67203` and an invalid Laue index with `-67204` before either executor runs. |
| `ComputeKernelAvgMisorientationsFilter-D5` | A radius of `INT32_MAX` overflows the signed loop upper bound. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. | The filter clamps unsigned neighbor bounds to the image before iteration. |

## Code path coverage

28 of 45 paths exercised. Each uncovered row states the missing fixture or the guard that prevents direct execution.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeKernelAvgMisorientations.cpp` (92 lines), `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeKernelAvgMisorientationsDirect.cpp` (251 lines), `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeKernelAvgMisorientationsScanline.cpp` (964 lines), and `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ComputeKernelAvgMisorientationsFilter.cpp` (175 lines).

Test names below omit the `OrientationAnalysis::ComputeKernelAvgMisorientationsFilter:` or `OrientationAnalysis::ComputeKernelAvgMisorientations:` prefix. Rows 1–8 describe Direct and rolling-window calculation; the bounded-cache gaps have separate rows.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Calculate | Positive focal feature and phase enter the kernel | `Class 1 - Uniform 2D Single Feature` |
| 2 | Calculate | Clamp X, Y and Z neighbor bounds to the image | `Class 1 - Uniform 2D Single Feature`; `Class 1 - 1D x-axis Gradient`; `Class 1 - 1D z-axis Gradient (3D path)` |
| 3 | Calculate | Same-feature mode admits a matching feature | `Class 1 - Multi-Feature Multi-Voxel + Background` |
| 4 | Calculate | Same-feature mode rejects a different feature | `Class 1 - Multi-Feature Multi-Voxel + Background` |
| 5 | Calculate | Same-phase mode admits positive features across a feature boundary | `Class 1 - Per-Voxel Mode (use_feature_ids = false)` |
| 6 | Calculate | Same-phase mode rejects feature zero | `Class 1 - Per-Voxel Mode (use_feature_ids = false)` |
| 7 | Calculate | Same-phase mode rejects a different phase | `Class 1 - Per-Voxel Mode Two-Phase Gates` |
| 8 | Calculate | Focal feature zero writes zero, including with a positive phase | `Class 1 - Multi-Feature Multi-Voxel + Background`; `Class 1 - Per-Voxel Mode Two-Phase Gates` |
| 9 | Validation | Participating phase is outside Crystal Structures: error `-67203` | `Phase and Laue Index Bounds` / `Participating Phase returns an error` |
| 10 | Validation | Participating Laue index is outside the operator list: error `-67204` | `Phase and Laue Index Bounds` / `Participating Laue index returns an error` |
| 11 | Validation | Ignore an invalid phase on feature zero | `Phase and Laue Index Bounds` / `Phase on a background Feature is ignored` |
| 12 | Calculate | Positive focal feature has a nonpositive phase | *Not directly tested. Background fixtures also set the feature to zero.* |
| 13 | Calculate | Negative feature ID follows the background rule | *Not directly tested. Fixtures use zero or positive feature IDs.* |
| 14 | Dispatch | Direct calculation on arrays in memory | `Class 1 - 1D x-axis Gradient` — algorithm scenario harness |
| 15 | Dispatch | Scanline rolling-window calculation on arrays in memory | `Class 1 - 1D z-axis Gradient (3D path)` — algorithm scenario harness |
| 16 | Dispatch | Bounded-cache calculation in same-feature mode | `Bounded Cache Full-Depth Kernel` — all values match `[10,7,6,7,10]` by Z slice |
| 17 | Calculate | Bounded-cache calculation in same-phase mode | *Not directly tested. The forced bounded-cache fixture enables same-feature mode.* |
| 18 | Calculate | Bounded cache skips background cells or different-feature neighbors | *Not directly tested. The forced bounded-cache fixture has one positive feature and phase.* |
| 19 | Calculate | Maximum signed radius clamps to the image | `Scanline Handles Maximum Kernel Radius` — Direct and Scanline scenarios |
| 20 | Setup | An empty image returns an empty output | `Scanline Handles Empty Image Geometry` — each axis is zero in turn |
| 21 | Plan | Kernel vector has fewer or more than three values: error `-67202` | `Working Set Input Validation` / `short kernel is rejected before indexing`; `oversized kernel is rejected` |
| 22 | Plan | Negative kernel radius: error `-67202` | `Working Set Input Validation` / `negative kernel radius is rejected as invalid input` |
| 23 | Plan | Live cache use reduces the memory cap | `Working Set Plan` / `live cache use reduces the algorithm cap` |
| 24 | Plan | Rolling window fits the cap, including zero dimensions | `Working Set Plan` / `normal kernel selects rolling window`; `Working Set Budget Boundaries` / `zero dimensions have an empty rolling working set` |
| 25 | Plan | Large window selects bounded blocks and shrinks row-aligned blocks | `Working Set Plan` / `large window selects bounded blocks` |
| 26 | Plan | Zero, exhausted or sub-tuple budget keeps one block and slot | `Working Set Budget Boundaries` |
| 27 | Plan | Slice tuple count overflows: error `-67200` | `Working Set Plan` / `slice tuple overflow reports complete context` |
| 28 | Plan | Total tuple or quaternion component count overflows: error `-67200` | `Working Set Overflow Boundaries` / `total tuple count overflow`; `total quaternion value count overflow` |
| 29 | Plan | Rolling input, output or total bytes overflow: error `-67200` | `Working Set Overflow Boundaries` — three byte-count sections |
| 30 | Plan | Other checked window, block and cache-size arithmetic rejects overflow | *Not directly tested. Earlier tuple and byte checks and bounded radii/block sizes constrain these quantities.* |
| 31 | Cache | Small fully associative cache loads, reuses and replaces blocks; writes a partial final block | `Bounded Cache Full-Depth Kernel` — eight slots, 1,280 tuples per block and 4,096 tuples per slice |
| 32 | Cache | More than 16 slots select a set-associative cache | *Not directly tested. The forced bounded-cache fixture has eight slots.* |
| 33 | Cancel | Direct stops within a row; Scanline stops before a plane or block | *Not directly tested. No fixture injects cancellation.* |
| 34 | I/O | Validation propagates Crystal Structures, FeatureIds or Phases read errors | *Not directly tested. Requires injected store read failures.* |
| 35 | I/O | Rolling execution propagates ensemble or input-slice read errors | *Not directly tested. Requires injected store read failures.* |
| 36 | I/O | Rolling execution propagates output-slice write errors | *Not directly tested. Requires an injected store write failure.* |
| 37 | I/O | Bounded execution propagates ensemble, focal or neighbor block read errors | *Not directly tested. Requires injected store read failures.* |
| 38 | I/O | Bounded execution propagates output-block write errors | *Not directly tested. Requires an injected store write failure.* |
| 39 | Calculate | Scanline zero-neighbor fallback writes zero | *Not directly tested. A valid focal cell always includes itself.* |
| 40 | Validation | Read more than one 65,536-tuple validation chunk | *Not directly tested. The largest execution fixture has 20,480 cells.* |
| 41 | Calculate | Calculate misorientation for a non-cubic Laue class | *Not directly tested. All analytical orientations use cubic symmetry.* |
| 42 | Preflight | Create one float32 output component with the phase array tuple shape | `Class 1 - Uniform 2D Single Feature` — execution and tuple-dimension check |
| 43 | Conversion | Convert legacy paths, output name and UUID | `SIMPL Backwards Compatibility` — both dynamic sections |
| 44 | Setup | Internal rolling and bounded executors return early on empty dimensions | *Not directly tested. The Scanline entry point returns before either executor is called.* |
| 45 | Cache | Rolling window replaces a slice slot after its first full window | *Not directly tested. Rolling fixtures have at most three Z slices; the five-slice fixture forces bounded-cache execution.* |

## Test inventory

All pass in CI.

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::ComputeKernelAvgMisorientations: Working Set Plan` | kept | Checks rolling-window size, live-budget reduction, row-aligned fallback blocks and slice-overflow diagnostics. |
| `OrientationAnalysis::ComputeKernelAvgMisorientations: Working Set Input Validation` | kept | Checks short and oversized vectors, a negative radius, and acceptance of `INT32_MAX` radii by the planner. |
| `OrientationAnalysis::ComputeKernelAvgMisorientations: Working Set Budget Boundaries` | kept | Checks zero, exhausted and sub-tuple budgets, plus zero dimensions. |
| `OrientationAnalysis::ComputeKernelAvgMisorientations: Working Set Overflow Boundaries` | kept | Checks total tuples, quaternion values and three byte-count overflow boundaries; checks full diagnostic text. |
| `OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Scanline Handles Maximum Kernel Radius` | kept | Runs a one-cell image with all radii at `INT32_MAX`; requires one zero output value. Covers D5. |
| `OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Scanline Handles Empty Image Geometry` | kept | Checks zero output tuples in dynamic sections `dimensions = 0 x 1 x 1`, `dimensions = 1 x 0 x 1` and `dimensions = 1 x 1 x 0`. |
| `OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: SIMPL Backwards Compatibility` | kept | Checks UUID, comments, paths and output name in `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` dynamic sections. |
| `OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Class 1 - Uniform 2D Single Feature` | new-for-V&V | Checks nine zero values on a 3×3×1 image within 0.0001° and checks tuple dimensions. |
| `OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Phase and Laue Index Bounds` | kept | Checks errors `-67203` and `-67204`; accepts an invalid phase on background feature zero. Covers D4. |
| `OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Class 1 - 1D x-axis Gradient` | new-for-V&V | Checks all five values against `[2.5,10/3,10/3,10/3,2.5]` within 0.001°. |
| `OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Class 1 - 1D z-axis Gradient (3D path)` | new-for-V&V | Checks all three values against `[5,20/3,5]` within 0.001°. |
| `OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Class 1 - Multi-Feature Multi-Voxel + Background` | new-for-V&V | Checks `[5,5,10,10,0,0]`, including an exact background zero and an isolated focal cell. |
| `OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Class 1 - Per-Voxel Mode (use_feature_ids = false)` | new-for-V&V | Checks `[5,20/3,10,10,0,0]`; the changed second value proves that the mode admits a different feature. |
| `OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Class 1 - Per-Voxel Mode Two-Phase Gates` | new-for-V&V | Checks `[5,5,0,0,0]` for two cubic phases and an invalid focal feature. |
| `OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Class 4 - Mode Equivalence on Single Feature` | new-for-V&V | Compares all 27 values bit-for-bit between modes on a single-feature, single-phase gradient; requires nonzero output. |
| `OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Class 4 - Invariants` | new-for-V&V | Checks uniform zero, exact background zero, nonnegativity, a 62.8° cubic bound and nonzero gradient output in three sections. |
| `OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Bounded Cache Full-Depth Kernel` | kept | Checks all 20,480 values against slice values `[10,7,6,7,10]` within 0.001° under a 1 MiB budget. |

## Exemplar archive

- **Archive:** `6_6_stats_test_v2.tar.gz` — retired for this filter; current fixtures are inline.
- **SHA512:** `e84999dec914d81efce4fc4237c49c9bf32e48381b1e79f58aa4df934f0d7606cd7a948f9a5e7b17a126a7944cc531b531cfdc70756ca3e2207b20734e089723`
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/ComputeKernelAvgMisorientationsFilter.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/ComputeKernelAvgMisorientationsFilter.md` for the root cause, affected users and recommendation for each Part 1 entry.

| Deviation | Observed difference |
|-----------|---------------------|
| `ComputeKernelAvgMisorientationsFilter-D1` | Cubic KAM values can differ because of the corrected angle calculation and double-precision quaternion arithmetic. |
| `ComputeKernelAvgMisorientationsFilter-D2` | SIMPLNX uses the requested X radius on both sides of an asymmetric kernel. DREAM.3D v6.5.171 uses the Z radius on the positive X side. |
| `ComputeKernelAvgMisorientationsFilter-D3` | `use_feature_ids=false` admits same-phase neighbors across feature boundaries. DREAM.3D v6.5.171 has no equivalent mode. |
