# V&V Report: ComputeFeatureFaceMisorientationFilter

|           |                          |
|-----------|--------------------------|
| Plugin    | OrientationAnalysis      |
| SIMPLNX UUID               | `f3473af9-db77-43db-bd25-60df7230ea73`|
| SIMPLNX Human Name         | Compute Feature Face Misorientation (Face)             |
| DREAM3D 6.5.171 equivalent | `GenerateFaceMisorientationColoring` — `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/GenerateFaceMisorientationColoring.{h,cpp}`            |
| Verified commit            | *<filled at SBIR deliverable assembly>*                |
| Status | COMPLETE     |
| Sign-off  | *Nathan Young (algorithm rewrite + initial dataset, 2026-05-19) — Michael Jackson <mike.jackson@bluequartz.net> (hand-built test data, V&V completion, 2026-05-28)*           |

## At a glance

| Aspect                 | Current state            |
|------------------------|--------------------------|
| Algorithm Relationship | **Rewrite** — output 1-component angle (was 3-component axis·angle); Laue classes 2→11; NaN explicit on invalid (was implicit 0); modernized EbsdLib API; raw TBB → `ParallelDataAlgorithm` (parallelization disabled per thread-safety policy). EbsdLib `CubicOps` precision fix surfaced during this cycle (non-observable on V&V fixtures). 6 deltas total — see Algorithm Relationship.       |
| Oracle (confirmed) | **Class 1 (Analytical) primary** — the hand-built fixtures cover all 11 EbsdLib Laue classes. Companion disk-backed verification checks analytical 15° and 30° rotations across validation/computation boundaries and with limited working memory. |
| Code paths enumerated | The mappings below cover resident execution, phase validation, HDF5 boundaries, and admitted lookup routes. The resident per-face cancellation check and corrupt Laue-class guard remain untested. |
| Tests today | Public tests cover analytical output, phase bounds, resident mismatched phases, working-headroom routing, and SIMPL conversion. Companion verification covers disk-backed execution across block boundaries and with limited working memory. |
| Exemplar archive       | **None — data inlined in test source** (`test/ComputeFeatureFaceMisorientationTest.cpp` namespace `curated`). 102 vertices, 34+3 triangles, 41+4 features, 12+1 ensembles all encoded as `std::unique_ptr<…[]>` literals. No tar.gz archive, no download_test_data() entry needed.   |
| Legacy comparison      | **Not run.** Output structure differs by design (3-component axis·angle vs 1-component angle), so direct array comparison with DREAM3D 6.5.171's `GenerateFaceMisorientationColoring` output is not meaningful. The deviations are documented per-design rather than verified per-feature against the legacy output.   |
| Bug flags              | One root-caused precision issue **in EbsdLib** (not in this filter): `CubicOps::calculateMisorientationInternal` lost precision via `(qco.z()+qco.w())/sqrt(2)` followed by `acos(w)` near 1. Patched in EbsdLib to use `2·atan2(|v|, w)` with `|v|` from explicit reduced-quaternion components. Eliminated a ~0.02° residual on cubic boundaries that lie on a 4-fold sym op.         |
| V&V phase | The original COMPLETE status and sign-off are retained. The dated OOC recertification results below are historical. This source-based mapping update adds no new execution result, sign-off, or legacy binary comparison. |

## Summary

`ComputeFeatureFaceMisorientationFilter` computes the symmetry-reduced misorientation angle between the two grains on each surface-mesh face. A Class 1 analytical dataset covers all 11 EbsdLib Laue classes. Companion disk-backed verification checks analytical 15° and 30° outputs at both block boundaries and with limited working memory. The documented DREAM3D 6.5.171 deviations remain reconciled.

## Algorithm Relationship

*Classification:* **Rewrite** ~~| Port | Minor changes | New filter~~

*Evidence:* Deliberate rewrite of legacy `GenerateFaceMisorientationColoring::CalculateFaceMisorientationColorsImpl` (DREAM3D 6.5.171). Same SIMPL UUID retained; SIMPL 6.4/6.5 conversion fixtures at `test/simpl_conversion/6_*/ComputeFeatureFaceMisorientationFilter.json`. Legacy control-flow structure is preserved (parallel per-triangle, two-face phase + Laue-class lookup, fall-through for mismatches) but every interior choice differs — see deltas below.

*Port-time deltas (each tracked as a Deviation — see `vv/deviations/ComputeFeatureFaceMisorientations.md`):*

1. **Output structure: 3-component → 1-component** (Deviation D2). Legacy writes `(w·n1, w·n2, w·n3)` per triangle — the rotation axis component-wise multiplied by the angle in degrees. SIMPLNX writes just the angle in degrees. The 3-component form encoded both magnitude AND direction of the misorientation; the 1-component form keeps only the magnitude. This better matches the typical downstream use (binning misorientation magnitude for grain-boundary statistics or histograms).
2. **Laue class support: 2 classes → 11 classes** (Deviation D1). Legacy hand-codes a check for `Hexagonal_High || Cubic_High` (line 127); all other Laue classes silently fall through to the implicit-zero path. SIMPLNX checks `laueIndex < m_LaueOrientationOps.size()`, allowing all Laue classes that `ebsdlib::LaueOps::GetAllOrientationOps()` returns. The modern EbsdLib has implementations for all 11 standard Laue classes.
3. **Invalid-face handling: implicit 0 → explicit NaN** (Deviation D3). Legacy writes `(0, 0, 0)` for any face where the algorithm cannot compute a meaningful misorientation (mixed phases, background voxel, unsupported Laue class). SIMPLNX writes `(NaN)`. Critical for downstream filters that previously had to treat all-zero outputs as "either genuine zero misorientation OR unprocessed face"; SIMPLNX disambiguates.
4. **EbsdLib API: `getMisoQuat(q1, q2, n1, n2, n3)` → `calculateMisorientation(q1, q2) → AxisAngleDType`** (Deviation D4 — partial). Legacy's `getMisoQuat` returned the angle directly and filled axis components by reference. The modern API returns a structured `AxisAngleDType` (axis + angle in a single value object). The legacy API is no longer present in the current EbsdLib.
5. **Parallelization: raw `tbb::parallel_for` → serial execution**. Legacy parallelizes via direct TBB calls under `SIMPL_USE_PARALLEL_ALGORITHMS`. The resident SIMPLNX path uses `ParallelDataAlgorithm` with `setParallelizationEnabled(false)` because DataArray/DataStore access is not thread-safe, including distinct-index writes. The OOC path uses serial bulk reads and writes around admitted face and lookup buffers. Both paths retain the same per-face angle calculation.
6. **EbsdLib precision fix** (Deviation D4 — full root-cause). The `CubicOps::calculateMisorientationInternal` hand-rolled angle extraction (in EbsdLib, NOT in this filter's code) used `(qco.z()+qco.w())/sqrt(2)` followed by `acos(w)`. When the misorientation lies on a cubic symmetry op (e.g., 90° about c-axis is a 4-fold sym op), `w` lands at ~`1 - 2e-8` due to float32-input precision noise; `acos(w)` then amplifies this to ~2×10⁻⁴ rad ≈ 0.023° residual. Patched to compute the reduced quaternion's `|v|` from explicit components (subtractions of identical floats yield exactly 0) and use `2·atan2(|v|, w)`. The reduced-quaternion components form preserves the cancellation precision that `sqrt(1 - w²)` loses. Eliminates the ~0.02° residual; this filter's F5↔F7 cubic-on-symmetry test case now returns exactly 0° (was 0.0212°).

*Material PRs since baseline (2026-05-19, Nathan's V&V doc commit):*

- **`nathan/enh/issue_1596`** (squashed into this branch 2026-05-28) — Six commits authored by Nathan implementing the filter rewrite (deltas 1–5 above). Squash-merged because the intermediate commits ("filter and algorithm implementation; test pending", "Create new test (failing)", "patch test", etc.) are work-in-progress and the squashed unit is the meaningful change.
- **This V&V cycle (2026-05-28)** — Mike added the hand-built Class 1 test dataset, Mike added Trigonal_High coverage (originally missing — Laue indices 0–9 only; index 10 added to close the gap), Mike + Claude root-caused the F5↔F7 precision residual to EbsdLib `CubicOps::calculateMisorientationInternal`, Claude patched EbsdLib with the `2·atan2(|v|, w)` form, Mike updated test assertion from `0.0212f` → `0.0f`.

## Oracle

*Class:* **1 (Analytical)** primary.

### Applied (Class 1 — Analytical)

Expected misorientation values are derived from the closed-form symmetry-group reduction of the boundary's true rotation. The dataset uses pure φ1-rotations (Bunge Euler angles `(φ1, 0, 0)` with `Φ = φ2 = 0`), so the true misorientation between any two features is simply `|Δφ1|` modulo the c-axis-aligned symmetry operators of the Laue class.

For each Laue class L, four features (one phase, four orientations) are constructed:
- Feature A: `φ1 = 0°`
- Feature B: `φ1 = 45°`
- Feature C: `φ1 = 90°`
- Feature D: `φ1 = 180°`

And three boundary faces are constructed: A↔B, A↔C, A↔D. The symmetry-reduced expected misorientation depends on the Laue class's c-axis n-fold:

| Laue class (idx)          | c-axis n-fold | A↔B (0°↔45°) | A↔C (0°↔90°) | A↔D (0°↔180°) |
|--------------------------------------------|---------------|--------------|--------------|---------------|
| Hexagonal_High m⁻³m (0)   | 6-fold        | 15°          | 30°          | 0°            |
| Cubic_High 6/mmm (1)      | 4-fold        | 45°          | **0°***      | 0°            |
| Hexagonal_Low 6/m (2)     | 6-fold        | 15°          | 30°          | 0°            |
| Cubic_Low m-3 (3)         | 2-fold (face) | 45°          | 90°          | 0°            |
| Triclinic -1 (4)          | 1-fold        | 45°          | 90°          | 180°          |
| Monoclinic 2/m (5)        | 1-fold        | 45°          | 90°          | 180°          |
| OrthoRhombic mmm (6)      | 2-fold        | 45°          | 90°          | 0°            |
| Tetragonal_Low 4/m (7)    | 4-fold        | 45°          | 0°           | 0°            |
| Tetragonal_High 4/mmm (8) | 4-fold        | 45°          | 0°           | 0°            |
| Trigonal_Low -3 (9)       | 3-fold        | 45°          | 30°          | 60°           |
| Trigonal_High -3m (10) — *SIMPLNX addition* | 3-fold        | 45°          | 30°          | 60°           |

*Cubic_High A↔C: 90° about c-axis is a 4-fold cubic symmetry op of m-3m, so the true symmetry-reduced misorientation is exactly 0°. Before the EbsdLib precision fix, this returned ~0.0212° due to `acos(w)` near 1. See Deviation D4 / Algorithm Relationship delta 6.

**Edge cases** (faces 30–33, after the 30 normal cases): all four expected to produce NaN.

| Face | Front label | Back label | Expected | Path exercised |
|------|-------------|------------|----------|--------------------------------------------------|
| 30   | 0           | 1          | NaN      | Background-front (frontFeature == 0)             |
| 31   | 1           | 0          | NaN      | Background-back (backFeature == 0)               |
| 32   | 1           | 5          | NaN      | Different phases (phase 1 Hex_High vs phase 2 Cubic_High) |
| 33   | 5           | 1          | NaN      | Different phases, reversed      |

### Encoded

- **Class 1 (Analytical), public**: `test/ComputeFeatureFaceMisorientationTest.cpp::"OrientationAnalysis::ComputeFeatureFaceMisorientationFilter: Curated Data"` — checks the closed-form values for all 11 Laue classes and the nonparticipating-face NaN cases above.
- **Class 1 (Analytical), companion disk-backed verification**: [BlueQuartzSoftware/SimplnxOoc#4](https://github.com/BlueQuartzSoftware/SimplnxOoc/pull/4) checks analytical 15° and 30° results across the public validation and computation boundaries. It checks the complete output, NaN for nonparticipating faces, and unchanged inputs.
- *(kept)* `test/ComputeFeatureFaceMisorientationTest.cpp::"OrientationAnalysis::ComputeFeatureFaceMisorientationFilter: SIMPL Backwards Compatibility"` — SIMPL 6.4 + 6.5 conversion paths via `DYNAMIC_SECTION`.

Companion verification also checks numerical results with limited working memory, refusal when memory is unavailable, read-failure diagnostics, warnings, and cancellation. These are behavior-based coverage descriptions, not new execution results. The public validation pass still copies the complete Feature Phases array. This coverage does not establish a bound for whole-filter memory or process RSS.

### Second-engineer review

**Signed off by Michael Jackson (technical authority), 2026-05-28.** Review focus: the symmetry-group hand calculations for Trigonal_High and the EbsdLib precision-fix rationale. Note that the Trigonal_Low and Trigonal_High closed-form values are identical (mirror planes containing the c-axis do not reduce pure c-axis rotations further).

## Bugs found and fixed

This branch includes the EbsdLib correction in this table. The correction will be in the DREAM3D-NX release after v7.4.1.

| Deviation | Defect | Affected released versions | Resolution in this branch |
|-----------|--------|----------------------------|---------------------------|
| `ComputeFeatureFaceMisorientations-D4` | Cubic symmetry boundaries could contain a false residual of about 0.02° because angle extraction lost precision near zero. | DREAM3D-NX v7.0.0 through v7.4.1. | EbsdLib now uses stable reduced-quaternion components and `2*atan2` angle extraction. |

## Code path coverage

The table maps current source routes to public tests and companion behavior checks. Companion verification covers cancellation during OOC execution; the resident per-face cancellation check remains untested. The corrupt Laue-class guard and other bulk-I/O failure sites do not gain coverage from those companion checks.

Sources: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureFaceMisorientation.cpp`, its shared `OrientationTupleLookup.hpp`, and `ComputeFeatureFaceMisorientationFilter.cpp`. Validation reads face labels in blocks of 65,536. OOC computation uses at most 262,144 faces per block and reduces the block when the admitted lookup plan requires it. The resident path uses direct array access. Short test names below use the `OrientationAnalysis::ComputeFeatureFaceMisorientationFilter: ` prefix.

| # | Phase           | Path             | Test case            |
|---|-----------------|-------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------|
| 1 | Validation/computation boundaries | Analytical angles remain correct across the 65,536-face validation and 262,144-face computation boundaries | Companion disk-backed verification in [BlueQuartzSoftware/SimplnxOoc#4](https://github.com/BlueQuartzSoftware/SimplnxOoc/pull/4) — complete output and unchanged inputs |
| 2 | Validation      | A participating shared phase is outside the Crystal Structures range → error `-98412` | `Phase Index Bounds` — "Participating Phase returns an error" |
| 3 | Validation | Mismatched phases do not participate in the bounds check | Public `Mismatched Phase resident scenarios`; companion verification with limited working memory |
| 4 | Cancel check | Resident per-triangle cancellation check returns before further computation | *Not directly tested. Companion cancellation checks cover the OOC route instead.* |
| 5 | Per-face        | `frontFeature == 0` → NaN | `Curated Data` — face 30 `(0, 1)` |
| 6 | Per-face        | `backFeature == 0` → NaN | `Curated Data` — face 31 `(1, 0)` |
| 7 | Per-face        | Positive features have different phases → NaN | `Curated Data` — faces 32 and 33 |
| 8 | Per-face        | Shared phase has an unsupported Laue index → NaN | *Not directly tested. This is a low-value corrupt-metadata guard.* |
| 9 | Per-face | Shared phase has a supported Laue index → calculate and write degrees | Public `Curated Data`; companion boundary and low-memory verification with analytical 15° and 30° values |
| 10 | Per-face math  | Cubic symmetry reduction uses the precision-corrected angle extraction | `Curated Data` — F5↔F6, F5↔F7, and F5↔F8 |
| 11 | OOC lookup admission | Lookup routes preserve known angles with limited working memory | Companion low-memory verification in [BlueQuartzSoftware/SimplnxOoc#4](https://github.com/BlueQuartzSoftware/SimplnxOoc/pull/4) |
| 12 | OOC lookup admission | Zero or insufficient headroom returns `-98425`; resident computation does not need that grant | Public `working headroom applies only to OOC computation`; companion verification of insufficient-memory errors |
| 13 | OOC lookup outcomes | Preserve lookup errors/path context and successful warnings; cancellation leaves output defaults and the filter returns `-1` without execution warnings | Companion read-failure, warning, and cancellation checks |

## Test inventory

Public cases are in `test/ComputeFeatureFaceMisorientationTest.cpp`. With OOC enabled, the public `test/CMakeLists.txt` registers `OrientationAnalysisOocStoreContracts`, which selects `[.OocStoreContract]`. Companion coverage is tracked in [BlueQuartzSoftware/SimplnxOoc#4](https://github.com/BlueQuartzSoftware/SimplnxOoc/pull/4). Resident scenarios alone do not prove disk-backed execution.

| Test case        | Status      | Notes           |
|--------------------------------------------------------------------------------------|-------------|----------------------------------------------|
| `OrientationAnalysis::ComputeFeatureFaceMisorientationFilter: Curated Data`          | new-for-V&V | Class 1 hand-built dataset; 30 normal + 4 edge + 3 Trigonal_High asserts. Replaces the legacy `Valid filter execution` test (which used the `6_6_Small_IN100_GBCD.tar.gz` exemplar) — the legacy test was a regression-against-exemplar test, not a closed-form correctness test, and was incompatible with the rewritten 1-component output. |
| `OrientationAnalysis::ComputeFeatureFaceMisorientationFilter: Phase Index Bounds` | current public | Checks participating-phase error `-98412`. The fixture requires HDF5-OOC stores when that backend is available. The mismatched-phase success case is separate. |
| Companion disk-backed boundary verification | companion | Checks analytical angles across both public block boundaries, NaN for nonparticipating faces, and unchanged inputs. See [BlueQuartzSoftware/SimplnxOoc#4](https://github.com/BlueQuartzSoftware/SimplnxOoc/pull/4). |
| `OrientationAnalysis::ComputeFeatureFaceMisorientationFilter: Mismatched Phase resident scenarios` | current public | Runs the selected resident-store algorithm scenarios and checks NaN for nonparticipating mismatched phases. |
| `OrientationAnalysis::ComputeFeatureFaceMisorientationFilter: working headroom applies only to OOC computation` | current public | Exhausts shared working headroom. Checks `-98425` for the OOC algorithm on resident stores and zero degrees for the resident algorithm. |
| Companion low-memory verification | companion | Checks known angles, insufficient-memory errors, diagnostics, warnings, and cancellation with disk-backed storage. See [BlueQuartzSoftware/SimplnxOoc#4](https://github.com/BlueQuartzSoftware/SimplnxOoc/pull/4). |
| `OrientationAnalysis::ComputeFeatureFaceMisorientationFilter: SIMPL Backwards Compatibility` | kept        | Unchanged. `DYNAMIC_SECTION` over SIMPL 6.4 and 6.5 conversion fixtures (`test/simpl_conversion/6_*/ComputeFeatureFaceMisorientationFilter.json`); validates UUID, argument keys, and parameter conversion only.             |
| *(retired)* `OrientationAnalysis::ComputeFeatureFaceMisorientationFilter: Invalid filter execution` | retired     | Removed during Nathan's rewrite. The Class 1 dataset's faces 30–33 cover the same paths via the NaN-on-invalid-face semantics; the explicit-preflight-failure tests are no longer reachable for the new code structure.     |

### Historical execution evidence

The following dated result refers to the former public `genuine HDF5 65536-block tail oracle`. That case was replaced by the companion verification above. The result is retained as historical evidence, not as a current test count or a new run.

OOC recertification, 2026-09-18: serial CTest passed 3/3 in `NX-Com-Qt69-Vtk96-Rel` and 3/3 in `NX-Com-Qt69-Vtk96-OoC-Rel`. The new hidden boundary case passed 25 assertions in the OOC binary and is included in the OOC-only `OrientationAnalysisOocStoreContracts` CTest entry. The original report status and sign-off above are historical and unchanged.

## Exemplar archive

- **Archive:** None — data inlined in `test/ComputeFeatureFaceMisorientationTest.cpp` namespace `curated`.
- **SHA512:** N/A
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/ComputeFeatureFaceMisorientations.md`

Data construction details: 102 vertices laid out in a y-axis-stacked grid (one row of 9 vertices per Laue class block + edge case block), 34 + 3 triangles (3 per Laue class + 4 edge case + 3 Trigonal_High), 41 + 4 features, 12 + 1 ensembles. The unique-vertices-per-triangle layout means each triangle is geometrically independent (no shared edges or vertices between triangles) — this is intentional, since the algorithm only reads `FaceLabels`, not vertex coordinates, so the geometric layout is arbitrary.

## Deviations from DREAM3D 6.5.171

Four documented deviation classes. All are deliberate design changes from the legacy filter (none are bugs in either side). One related EbsdLib precision fix (not a deviation in the strict V&V sense, since it improves both SIMPLNX and any other consumer of EbsdLib's CubicOps).

### ComputeFeatureFaceMisorientations-D1

- Supports all 11 Laue classes; legacy supported only Hex_High and Cubic_High. See `vv/deviations/ComputeFeatureFaceMisorientations.md`.

### ComputeFeatureFaceMisorientations-D2

— Output is a 1-component angle in degrees; legacy was a 3-component axis·angle vector. See `vv/deviations/ComputeFeatureFaceMisorientations.md`.

### ComputeFeatureFaceMisorientations-D3

— Invalid faces (mixed phase, background voxel, unsupported Laue class) write NaN; legacy wrote 0. See `vv/deviations/ComputeFeatureFaceMisorientations.md`.

### ComputeFeatureFaceMisorientations-D4

— Precision improvement on cubic boundaries that lie on a 4-fold symmetry op. Root-caused to EbsdLib `CubicOps::calculateMisorientationInternal`; patched at the EbsdLib level (replaces `acos(w)` with `2·atan2(|v|, w)` using explicit reduced-quaternion components). See `vv/deviations/ComputeFeatureFaceMisorientations.md`.

### Historical downstream impact note

The following paragraph records the original V&V run. It is not a claim that these tests still fail on the current branch:
 
The EbsdLib precision fix in D4 propagates through any filter that consumes cubic misorientations. Eight OrientationAnalysis unit tests now fail against their pre-fix exemplars with diffs in the range `1.4× to 10× epsilon` (epsilons of `1e-4`, observed diffs `1.4e-4` to `1e-3`): 
- `BadDataNeighborOrientationCheckFilter: Case 1.{3,4,5,6}.3`
- `ComputeFeatureReferenceMisorientationsFilter_AverageMisorientation`
- `ComputeFeatureReferenceMisorientationsFilter_EuclideanDistance`
- `ComputeKernelAvgMisorientationsFilter`
- `ComputeFeatureNeighborMisorientationsFilter`. 

These exemplar files were generated against the pre-fix algorithm; the new values are *more* mathematically correct. **The exemplar files will need to be regenerated**.
