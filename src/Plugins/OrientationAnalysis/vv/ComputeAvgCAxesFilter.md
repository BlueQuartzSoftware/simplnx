# V&V Report: ComputeAvgCAxesFilter

|           |                          |
|-----------|--------------------------|
| Plugin    | OrientationAnalysis      |
| SIMPLNX UUID               | `453cdb58-7bbb-4576-ad5e-f75a1c54d348`                |
| SIMPLNX Human Name         | Compute Average C-Axis Orientations  |
| DREAM3D 6.5.171 equivalent | `FindAvgCAxes` (SIMPL UUID `c5a9a96c-7570-5279-b383-cc25ebae0046`) — `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindAvgCAxes.{h,cpp}` in DREAM3D 6.5.171 |
| Verified commit            | *<filled at SBIR deliverable assembly>*               |
| Status | COMPLETE     |
| Sign-off  | Michael Jackson &lt;mike.jackson@bluequartz.net&gt; — 2026-05-28        |

## At a glance

| Aspect                 | Current state            |
|------------------------|--------------------------|
| Algorithm Relationship | **Port with Minor Changes** — float→double accumulation + counter-reorder + error-code re-numbering (PR #1438); EbsdLib 2.0 API (PR #1472); cancel checks (PR #1582); `counter==0`→NaN finalize + per-feature normalize (V&V refactor). 7 deltas total — see Algorithm Relationship.           |
| Oracle (confirmed)     | **Class 1 (Analytical)** — 11-cell hand-built dataset, closed-form `AvgCAxes` per feature, 8 of 12 code paths covered. **Class 4 (Invariant)** — `||AvgCAxes||==1.0` for hex-valid; `NaN` for empty/non-hex. F7 only asserts unit-vector magnitude (direction is precision-sensitive at the antipodal-flip cancellation boundary). Class 3 N/A.       |
| Code paths enumerated  | 12 (from line-by-line scan of `ComputeAvgCAxes.cpp`)|
| Tests today            | 3: 1 valid-execution exemplar (positive), 1 all-non-hex error (negative), 1 SIMPL 6.4+6.5 backwards-compat (DYNAMIC_SECTION)             |
| Exemplar archive       | **`7_2_AvgCAxis.tar.gz` retired** — confirmed legacy-by-reputation oracle: reference values produced by a "special build of DREAM3D 6.6.379 with micro-texture bug fixes," not by an independent oracle. Per policy line 33, not eligible as a correctness oracle. Replaced by `compute_avg_c_axis.tar.gz` (hand-built Class 1 dataset, this V&V cycle).              |
| Legacy comparison      | **Complete (Run — SIMPLNX vs DREAM3D 6.5.171, post-normalize).** Two deviation classes, 4 feature-level differences vs 6.5.171: D1 (`counter==0` → NaN vs `(0,0,1)` rescue) at F0/F5/F6; D2 (precision-sensitive direction at antipodal-flip cancellation boundary + unit-vector vs unnormalized magnitude) at F7. Each root cause was proven by applying the corresponding surgical fixes to a local build of the legacy source, after which the legacy output became bit-identical to SIMPLNX across all 8 features. See `vv/comparisons/ComputeAvgCAxesFilter/results/three_way_comparison.txt`.                |
| Bug flags              | None confirmed. PR #1438's silent semantic changes are deviation candidates, not bugs.|
| V&V phase              | **Phases 1, 2, 3 (Port classification), 4, 5, 6, 7 (algorithm review + fix-up), 8 (test restructure), 9 (legacy comparison), 11 (filter doc review) — complete.** SIMPLNX bit-identically matches the Class 1 oracle on F0-F6 and the Class 4 unit-vector invariant on F7. Legacy A/B confirmed against DREAM3D 6.5.171 (`/Users/mjackson/Applications/DREAM3D.app/Contents/bin/PipelineRunner`); each root cause was proven by applying the corresponding surgical fixes to a local build of the legacy source, after which the legacy output became bit-identical to SIMPLNX across all 8 features. **Outstanding:** Phase 10 (re-tar archive), Phase 12 (archive bundle: `download_test_data` + `TestFileSentinel` defaults), Phase 13 (status promotion DRAFT → COMPLETE).|

## Summary

`ComputeAvgCAxesFilter` computes a per-feature average C-axis direction (unit vector in the sample reference frame) for **hexagonal**-phase grains, by rotating the crystallographic `[001]` of each cell into the sample frame, applying a running-average antipodal-flip rule to keep contributions in a coherent hemisphere, and normalizing the final per-feature sum. Verification used a **Class 1 (Analytical) hand-built 11-cell / 8-feature dataset** with closed-form expected values for F0–F6 plus a **Class 4 unit-vector invariant** (`||AvgCAxes|| == 1.0`) for F7, which was deliberately placed on the antipodal-flip cancellation boundary; results were cross-checked against DREAM3D 6.5.171 (official release). Each root cause was proven by applying the corresponding surgical fixes to a local build of the legacy source, after which **the legacy output became bit-identical to SIMPLNX across all 8 features**, conclusively isolating the four documented differences vs 6.5.171 (D1 — `counter==0 → NaN` vs `(0,0,1)` rescue at F0/F5/F6; D2 — precision-sensitive direction + unit-vector vs unnormalized magnitude at F7) to the SIMPLNX-era design changes (float→double accumulation, Eigen-style math, `counter==0 → NaN` at finalize, and final per-feature normalize).

## Algorithm Relationship


*Classification:* **Port** with Minor Changes

*Evidence:* The SIMPLNX algorithm at `Algorithms/ComputeAvgCAxes.cpp` (181 lines) is a translation of the legacy `FindAvgCAxes::execute()` from `DREAM3D/Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindAvgCAxes.cpp` (DREAM3D 6.5.171). 
- Same SIMPL UUID retained via `OrientationAnalysisLegacyUUIDMapping.hpp` + SIMPL 6.4 and 6.5 conversion fixtures at `test/simpl_conversion/6_*/ComputeAvgCAxesFilter.json`. 
- The control flow is preserved: phase-validity preflight → per-cell accumulation loop (passive-quaternion → orientation matrix → transpose → c-axis · `[0,0,1]`, antipodal-flip aware) → per-feature finalize (divide-by-count or `counter==0` handling). 
- However, PR #1438 ("ENH: Microtexture related filter cleanup") applied intentional silent changes that distinguish this from a pure line-by-line port; 
- PR #1472 applied an EbsdLib API refactor; 
- PR #1582 added cancel checks
- Phase-7 V&V refactor consolidated the no-valid-contributions handling AND added a final per-feature normalize step so the output contract is unit C-axis vectors. All seven changes are listed below.

*Port-time deltas (numerical / semantic Deviations vs legacy — see Phase 9):*

1. **Float → double accumulation** (PR #1438) — All inner-loop accumulation now uses `Eigen::Vector3d`, `OrientationD`, `QuatD`, `Matrix3dR`. Legacy used `float` throughout. Contributes to Deviation D2 (precision-boundary antipodal flip).
2. **`counter[currentFeatureId]++` reordered** (PR #1438) — Moved from *after* the antipodal-flip dot-product test to *before* it. Changes which divisor is used for the first voxel's running-average reference; can produce sign flips on the first voxel of each feature vs. legacy. Contributes to Deviation D2.
3. **Error codes re-numbered + preflight warning removed** (PR #1438) — Legacy `-6402` → SIMPLNX `-76402`; legacy `-6403` → `-76403`; legacy preflight warning `-6401` ("Selected crystal structure phase is not Hexagonal") was *deleted entirely* and replaced with a non-error `preflightUpdatedValues` "Crystal Symmetry Warning": entry. API-surface Deviation candidate (no numerical impact on this V&V dataset).
4. **EbsdLib API refactor** (PR #1472) — `EbsdLib::` → `ebsdlib::` namespace; explicit `OrientationTransformation::qu2om<QuatD,OrientationD>(...)` + `OrientationMatrixToGMatrixTranspose(oMatrix)` helper call replaced by inline `ebsdlib::QuaternionDType(...).toOrientationMatrix()` + `oMatrix.transpose() * cAxis`. Convention parity between the new `toOrientationMatrix()` and the legacy `qu2om` verified on the V&V dataset (the surgically patched local build of the legacy source, still using legacy EbsdLib, bit-matches SIMPLNX, so the conventions are functionally aligned).
5. **Cancel checks added** (PR #1582) — `m_ShouldCancel` guards in the per-cell loop and the per-feature normalization loop. UX-only; no algorithmic effect on completed runs.
6. **`counter==0` → NaN at feature loop, feature loop starts at 0** (Phase-7 V&V refactor) — In the per-cell loop, non-hex cells are simply `continue`d (no in-place NaN write, no counter increment). The per-feature finalize loop now starts at `currentFeatureId = 0` (legacy starts at 1) and writes `(NaN, NaN, NaN)` whenever `counter[feature] == 0`. This consolidates all "no valid contributing voxels" handling at finalize and signals undefined output honestly. Drives Deviation D1 (F0, F5, F6).
7. **Final per-feature normalize** (Phase-7 V&V refactor) — After dividing the accumulated sum by `cellCount`, the result is now normalized to a unit C-axis vector before being stored. Legacy 6.5.171 returned the unnormalized average — magnitude was implicitly a within-feature coherence signal (1.0 for aligned, lower for diverging). The unit-vector contract better matches user expectations and downstream filters (which compute `acos(dot(a,b))` and need unit-magnitude inputs anyway). The antipodal-flip invariant guarantees `|sum| >= sqrt(cellCount)` so the divisor is never near-zero — no NaN guard needed. Widens Deviation D2 to include a magnitude difference (1.0 vs 2/3) at F7 in addition to the precision-sensitive direction.

*Material PRs since baseline (2025-10-01):*

- **#1438** — "ENH: Microtexture related filter cleanup" (merge `e6896714b`, 2025-10-25) — algorithm `+89/-76`, filter `+2/-3`. Float→double, counter-reorder, error re-numbering / preflight-warning removal. *(Normally pruned as a broad refactor, but promoted here per the audit's exception rule because it materially altered this filter's inner loop.)*
- **#1472** — "ENH: Update to EbsdLib 2.0.0 API" (merge `413e6fa46`, 2025-11-24) — algorithm `+9/-12`. EbsdLib namespace + `toOrientationMatrix()` refactor. *(Promoted from broad-refactor exclusion list — this filter delegates orientation math to EbsdLib.)*
- **#1476** — "BUG/ENH: Fix Backward Pipeline Compatibility and Add Testing" (merge `e45bca2a5`, 2026-01-06) — filter `+1/-1`. Single-line `FromSIMPLJson` converter type correction (`DataArrayNameFilterParameterConverter` → `DataArrayCreationToDataObjectNameFilterParameterConverter`) for the `AvgCAxes` output.
- **#1582** — "ENH: Add missing cancel checks to lots of filters" (merge `1a42ec6fb`, 2026-04-08) — algorithm `+10/-0`. Two `m_ShouldCancel` guards.
- **#1588** — "ENH: SIMPL Backwards Compatibility Test Redesign" (merge `f854bb636`, 2026-04-22) — test `+49`, plus two new fixture files `simpl_conversion/6_4/ComputeAvgCAxesFilter.json` and `simpl_conversion/6_5/ComputeAvgCAxesFilter.json`. SIMPL conversion test only.
- *(excluded — doc typo)* #1547 — "DOC: Fix filter documentation and documentation related code bugs" (2026-03-10) — docs `+1/-1` ("Crystallographic" → "Crystallography" subgroup typo). No algorithm or API change.
- *(pruned — broad refactor, no behavioral change to this filter)* #1439 (NeighborList tuple API), #1457 (static-inline cleanup), #1501 (Vec3 unification), #1538 (zlib tar.gz extraction in tests).

## Oracle

*Class:* **1 (Analytical)** primary, **4 (Invariant)** companion.

### Phase 2 exemplar-provenance finding

The pre-existing exemplar `7_2_AvgCAxis.tar.gz` is a regression-style oracle pinned to a custom DREAM3D 6.6.379 "micro-texture special build" (per its inline ReadMe). Per V&V policy line 33, legacy output is never a valid correctness oracle — doubly disqualified here, since the reference build isn't even 6.5.171 baseline. **Retired** in favor of the hand-built Class 1 dataset below.

### Applied (Class 1 — Analytical)

Expected `AvgCAxes` values are derived by hand from input definitions without reference to any DREAM3D implementation. For each hex-feature `f`:

1. Identify the cells assigned to `f`: `cellSet = {i : FeatureIds[i] == f}`.
2. For each cell `i` in `cellSet`: compute the voxel c-axis `c_i = R(q_i).T · [0,0,1]` where `R(q)` is the standard quaternion → passive orientation matrix (standard 2015 Eq. 14 convention; the algorithm's `oMatrix.transpose() * (0,0,1)` is the active-rotation form).
3. Apply the running-average antipodal flip rule: process cells in cell-index order; for each cell after the first, if `c_i · (Σ prior c_j / count) < 0`, flip `c_i := -c_i`.
4. Sum the (possibly-flipped) `c_i`, divide by the cell count, then normalize: `AvgCAxes[f] = normalize((Σ c_i) / count)`. The antipodal-flip invariant guarantees `|Σ c_i| >= sqrt(count)`, so dividing by `|Σ c_i / count|` is always well-defined whenever `count > 0`.

**The output is a unit C-axis direction per feature** — magnitude is exactly 1.0 (within float32 epsilon) for every feature with at least one hex-phase contributing voxel.

Hand-derivation on the 11-cell hand-built fixture (`FeatureIds = [1,2,3,3,3,4,4,6,7,7,7]`, `CellPhases = [1,1,1,1,1,1,1,2,1,1,1]`, hand-picked quaternions detailed in `vv/comparisons/ComputeAvgCAxesFilter/README.md`). **Class 1 fully covers F0-F6**; F7 is handled by the Class 4 unit-vector invariant below because the F7 cells were deliberately designed to land on the *antipodal-flip cancellation boundary* — the algorithm's choice between two genuinely distinct c-axis directions at this boundary is precision-sensitive (see Deviation D2).

| Feature | Expected `AvgCAxes`        | Magnitude | Path           |
|---------|----------------------------|-----------|------------------------------------------------------------------------------------|
| 0       | NaN       | —         | placeholder; no cells reference it, `counter==0` at finalize → NaN|
| 1       | (0, 0, 1) | 1.0       | single voxel, identity          |
| 2       | (0, 0.866025, 0.5)         | 1.0       | single voxel, +60° about X      |
| 3       | (0, 0, 1) | 1.0       | 3 aligned voxels — trivial average               |
| 4       | (0, 0, 1) | 1.0       | antipodal pair + antipodal-flip resolution       |
| 5       | NaN       | —         | no cells reference feature 5, `counter==0` at finalize → NaN      |
| 6       | NaN       | —         | only cell of F6 is non-hex (Cubic_High) → skipped in cell loop, `counter==0` → NaN |
| 7       | *Class 4: magnitude = 1.0* | 1.0       | precision-sensitive boundary case — see below    |

**Feature 7 boundary case detail.** F7 has three cells with c-axes `(0, 0, 1)`, `(0, +√3/2, 0.5)`, `(0, -√3/2, 0.5)`. The Y-components of cells 9 and 10 are exact antipodes. At cell 10 the running-average dot product `c1 · normalize(avg/counter)` evaluates to `-0.433 + 0.433` — exact mathematical cancellation. Whichever side of zero the floating-point result lands on determines whether the antipodal flip fires, and after the final normalize this produces two **genuinely different** c-axis directions (NOT hex c≡-c equivalents — they make a 60° angle in 3D):

- **No flip** (pure-double math-ideal path): pre-normalize `(0, 0, 2/3)` → post-normalize `(0, 0, 1)`
- **Flip fires** (SIMPLNX faithful-float32 Eigen path): pre-normalize `(0, 1/√3, 1/3)` → post-normalize `(0, √3/2, 0.5)`

Both are valid "average c-axes" of the same cell set under different sign-assignment choices for cells with c-axes pointing in opposite hemispheres. Both have magnitude `1.0` after normalization. The deviation is captured in Deviation D2; the unit test asserts only the invariant magnitude `1.0` for F7 (the direction is implementation-dependent).

### Applied (Class 4 — Invariant)

Derivable properties asserted inline in test code (Phase 8 work):

- **For features with `counter[i] > 0`** (i.e., at least one hex-phase contributing voxel): `||AvgCAxes[i]|| == 1.0` (within float32 ε). The Phase-7 finalize-normalize step guarantees every hex-valid feature is a unit C-axis vector.
- **`counter[i] == 0` → `(NaN, NaN, NaN)`** at finalize. Covers all three "no valid contributing voxels" scenarios: placeholder feature 0 (never referenced by any cell), feature with declared tuples but no cells assigned (F5), and feature whose cells are all non-hex (F6).
- **For F7 specifically**: the unit-vector invariant `||AvgCAxes[7]|| == 1.0` still holds; the direction is implementation-dependent at the precision boundary (see Deviation D2).

### Encoded

- **Class 1 (Analytical)**: `test/ComputeAvgCAxesTest.cpp::"Class 1 Oracle (hand-built dataset)"` — exact-value comparisons for F0–F6 per the table above (NaN checks for F0/F5/F6; exact component-wise checks for F1–F4). Encoded as a `DYNAMIC_SECTION` per feature.
- **Class 4 (Invariant)**: same test, with the `magnitude == 1.0` assertion for F7 plus a general invariant `DYNAMIC_SECTION` asserting `||AvgCAxes[i]|| == 1.0` over all hex-valid features.
- *(retained)* `test/ComputeAvgCAxesTest.cpp::"Invalid Filter Execution"` — all-non-hex error path (`-76402`) by mutating `crystalStructs[1] = 1` (Cubic_High).
- *(retained)* `test/ComputeAvgCAxesTest.cpp::"SIMPL Backwards Compatibility"` — SIMPL 6.4 + 6.5 conversion paths via `DYNAMIC_SECTION`.

### Second-engineer review

**Pending**

## Code path coverage

*8 of 12 code paths exercised by unit tests. 4 gaps remain: path 2 (all-hex ensemble), path 4 (background `featureId == 0` cell), and paths 8 & 12 (cancel-signal during execution). Path 4 and the cancel paths are low-value coverage gaps (algorithm-loop guards rather than algorithmic logic); path 2 is the more notable gap but is trivially exercised by every shipping all-hex pipeline.*

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeAvgCAxes.cpp` (181 lines).

The algorithm has three logical phases: (a) phase-validity preflight scan, (b) per-cell accumulation loop, (c) per-feature finalize loop. Each phase contains decision branches enumerated below.

| #  | Phase           | Path| Test case      |
|----|-----------------|-----------|------------------------------------------------------------------------------------|
| 1  | (a) Preflight   | All phases non-Hexagonal → return `-76402` error       | `No_Hex_Phase` (mutates `CrystalStructures[1] = 1` to make all ensemble phases non-hex) |
| 2  | (a) Preflight   | All phases Hexagonal → no error, no warning            | *Not directly tested.* The Class 1 dataset is intentionally mixed-phase to exercise paths 3 and 5. Exercised implicitly by shipping pipelines (e.g., `EBSD_Hexagonal_Data_Analysis`). |
| 3  | (a) Preflight   | Mixed phases (some Hex, some non-Hex) → warning `-76403` pushed, computation proceeds    | `Class 1 Oracle` (ensemble has Hex_High + Cubic_High; warning fires once and the test sees it in the per-section warning log) |
| 4  | (b) Per-cell    | `featureId == 0` (background) → skip cell (`if(currentFeatureId > 0)` guard)             | *Not directly tested.* The Class 1 input has no background voxels (`FeatureIds = [1,2,3,3,3,4,4,6,7,7,7]`). Low-value gap — this is a loop-guard, not algorithmic logic. |
| 5  | (b) Per-cell    | `featureId > 0` + non-Hex crystal struct → `continue` (no in-place NaN write; counter NOT incremented; NaN handled later at finalize)       | `Class 1 Oracle` — F6 (sole cell is Cubic_High) verifies via the F6 = NaN assertion |
| 6  | (b) Per-cell    | `featureId > 0` + Hex crystal struct → normal accumulation (passive→active rotation → unit-vector → running-average + antipodal flip → add) | `Class 1 Oracle` — F1, F2, F3 exact-value checks verify the rotation + accumulation |
| 7  | (b) Per-cell    | Antipodal-flip branch: `CosBetweenVectors(c1, curCAxis) < 0` → `c1 *= -1` before accumulating             | `Class 1 Oracle` — F4 (antipodal-pair → (0,0,1)) exact-value check + F7 magnitude invariant (cancellation-boundary case) |
| 8  | (b) Per-cell    | Cancellation: `m_ShouldCancel` checked at top of per-cell loop → early return `result`   | *Not tested.* Requires injecting a cancel signal mid-execution; not exercised by any current test. Low-value coverage gap. |
| 9  | (c) Per-feature | Per-feature finalize loop starts at `currentFeatureId = 0` (placeholder feature included)| `Class 1 Oracle` — F0 NaN check verifies the loop visits index 0 (legacy 6.5.171 leaves it untouched) |
| 10 | (c) Per-feature | `cellCount[i] == 0` → write `(NaN, NaN, NaN)` to feature's slot. Covers placeholder F0, no-cells features (F5), all-non-hex features (F6).  | `Class 1 Oracle` — F0/F5/F6 NaN checks each cover one of the three sub-scenarios   |
| 11 | (c) Per-feature | `cellCount[i] > 0` → divide x/y/z by `cellCount[i]`, then normalize (Phase-7 refactor)   | `Class 1 Oracle` — F1–F4, F7 magnitude-1.0 invariants verify divide + normalize    |
| 12 | (c) Per-feature | Cancellation: `m_ShouldCancel` checked at top of per-feature loop → early return `result`| *Not tested.* Same rationale as path 8. Low-value coverage gap.   |

## Test inventory

| Test case        | Status      | Notes                |
|--------------------------------------------------------------------------------------|-------------|-----------|
| `OrientationAnalysis::ComputeAvgCAxesFilter: Class 1 Oracle (hand-built dataset)`    | new-for-V&V | Replaces the retired `7_2_AvgCAxis` exemplar test. Encodes Class 1 + Class 4 oracle: exact-value checks for F0–F6 plus magnitude == 1.0 invariant for F7 and a general unit-vector invariant across all hex-valid features. |
| `OrientationAnalysis::ComputeAvgCAxesFilter:No_Hex_Phase`           | kept        | Retained; switched from the legacy `caxis_data` archive to the new hand-built input by mutating `CrystalStructures[1] = 1` (Cubic_High) to trigger `-76402` (all-non-hex error path). |
| `OrientationAnalysis::ComputeAvgCAxesFilter: SIMPL Backwards Compatibility`          | kept        | Unchanged. `DYNAMIC_SECTION` over SIMPL 6.4 and 6.5 conversion fixtures (`test/simpl_conversion/6_*/ComputeAvgCAxesFilter.json`); validates UUID, argument keys, and parameter conversion only. |

## Exemplar archive

- **Archive:** `compute_avg_c_axis.tar.gz`
- **SHA512:** `4ee957b4a5e78e1d75e3585016a33de40985c66a8e8d1036b252e5974eb2b3360f34dacdcb51cd1d1ae25bfef2bb638979912cbc4555ff521eb2f42a167155b0`
- **Provenance:** Captured in the archive's internal `README.md` (no separate provenance file). Authoritative source for the input + scripts + pipelines + 3-way comparison outputs; mirrors the canonical V&V working set under `src/Plugins/OrientationAnalysis/vv/comparisons/ComputeAvgCAxesFilter/`.

## Deviations from DREAM3D 6.5.171

**Two documented deviation classes (four feature-level differences total), all fully isolated** to *precision + matrix-math style + counter==0 NaN at finalize* by a surgically patched local build of the legacy source. The patch applies the SIMPLNX-era design changes (float→double accumulation, Eigen-style math, counter==0 → NaN at feature loop) back into the legacy DREAM3D 6.5 codebase and produces output **bit-identical** to SIMPLNX on the V&V hand-built fixture — conclusively isolating these design changes as the sole sources of the SIMPLNX-vs.-6.5.171 differences.

Comparison fixtures:
- `/Users/mjackson/Workspace6/DREAM3D_Data/TestFiles/compute_avg_c_axis/output_legacy/6_5_171_compute_avg_c_axis.dream3d` (official DREAM3D 6.5.171 release output)
- a second output file alongside it in `output_legacy/` (output of the surgically patched local build of the legacy source)
- Three-way comparison report: `vv/comparisons/ComputeAvgCAxesFilter/results/three_way_comparison.txt`

- `ComputeAvgCAxesFilter-D1` — `counter==0` → NaN vs `(0, 0, 1)` rescue. Fires at **F0** (placeholder feature, no cells), **F5** (no cells reference this featureId), and **F6** (all cells of this feature are non-hex). SIMPLNX writes NaN to signal "no valid contributing voxels"; legacy 6.5.171 falls into the rescue branch and writes a confusing `(0, 0, 1)`. See `vv/deviations/ComputeAvgCAxesFilter.md`.
- `ComputeAvgCAxesFilter-D2` — Per-feature direction may flip to an antipodal-equivalent representative under hex `c ≡ -c` symmetry; magnitude preserved. Fires at **F7** (precision-sensitive antipodal-flip cancellation boundary). Root cause: double-precision and Eigen-style matrix math vs. float-only hand-rolled math. See `vv/deviations/ComputeAvgCAxesFilter.md`.

**Comparison library nuance:** legacy DREAM3D 6.5.171 (and the patched local build of its source) uses a **built-in EbsdLib/OrientationLib** inside the DREAM3D source tree (frozen at the 6.5.171 release point, with the targeted surgical patches applied on top). SIMPLNX uses an **independent, vcpkg-installed EbsdLib** that is actively updated. Both implement standard conventions but the underlying code differs. The patched legacy build reproduces SIMPLNX *functional behavior*, not *identical library code* — sufficient to isolate the design-choice drivers of the deviations.
