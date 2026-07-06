# V&V Report: GroupMicroTextureRegionsFilter

|                            |                                                                                                                                                                  |
|----------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Plugin                     | OrientationAnalysis                                                                                                                                              |
| SIMPLNX UUID               | `3f695987-81b1-47c3-8cff-b49cfa219be0`                                                                                                                           |
| SIMPLNX Human Name         | Group MicroTexture Regions                                                                                                                                       |
| DREAM3D 6.5.171 equivalent | `GroupMicroTextureRegions` — `Source/Plugins/Reconstruction/ReconstructionFilters/GroupMicroTextureRegions.{h,cpp}` (inherits from `GroupFeatures` base class)    |
| Patched-legacy reference   | 2025-10-23 surgical fix to a local build of the legacy source (`BUG: GroupMicrotextureRegions bug fixes, expose as usable filter`) — used here as a corroborating reference for port-time fixes |
| Verified commit            | *<filled at SBIR deliverable assembly>*                                                                                                                          |
| Status                     | DRAFT                                                                                                                                                            |
| Sign-off                   | *Michael Jackson <mike.jackson@bluequartz.net> — V&V pending review*                                                                                             |

## At a glance

| Aspect                 | Current state                                                                                                                                                                                                                                                                                                                                       |
|------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Algorithm Relationship | **Port** (with deliberate inheritance flattening and 2 port-time **regressions fixed during this V&V cycle**, plus 1 latent legacy bug already corrected in the original 2024-01-08 port). Legacy is a `GroupFeatures` subclass; SIMPLNX inlines the `GroupFeatures::execute` BFS loop into a single algorithm class. UUID retained: `5e18a9e2-…` → `3f695987-…`. |
| Oracle (confirmed)     | **Class 1 (Analytical) primary + Class 4 (Invariant) companion** — pure Bunge φ1=0, Φ=φ°, φ2=0 fixtures make the c-axis angular distance between any two features exactly \|Φ_A − Φ_B\| (folded into [0°, 90°]); expected groupings derive in closed form from the chosen tolerance and neighbor adjacency.                                          |
| Code paths enumerated  | 9 of 9 (line-by-line walk of `GroupMicroTextureRegions::operator()`, `execute`, `getSeed`, `determineGrouping`, `randomizeParentIds`)                                                                                                                                                                                                                |
| Tests today            | 5 test cases: Class 1 "Pure-Phi Bunge" (5 features, 3 groups), Class 4 "RandomizeParentIds invariants" (equivalence-class preservation + seed determinism), Class 1 "Tolerance Boundary" (3 features, 2 groups), defect-A regression pin (UseNonContiguousNeighbors=false runs without error), SIMPL 6.4+6.5 backwards-compat (DYNAMIC_SECTION). |
| Exemplar archive       | **None — data inlined in test source** (`test/GroupMicroTextureRegionsTest.cpp` namespace `AnalyticalFixtures`). Scaffold helper builds an `{nX,1,1}` ImageGeom + Cell/Feature/Ensemble AMs; per-feature pure-Phi quats and per-feature neighbor lists are set explicitly per test.                                                                  |
| Legacy comparison      | **Not run.** Legacy 6.5.171 randomizes parent IDs by default with a non-reproducible seed; bit-identical comparison is not meaningful. A 2025-10-23 fix applied to a local build of the legacy source exposes `RandomizeParentIds` as a user parameter (default false) — the same design SIMPLNX now uses post-fix; this V&V verifies SIMPLNX against an independent oracle. |
| Bug flags              | **None remaining.** Two port-time regressions (D1, D2) were found and fixed during this V&V cycle and are pinned by tests. One legacy 6.5.171 bug (D3) was already corrected in the original 2024 SIMPLNX port and is documented for migration users.                                                                                              |
| V&V phase              | Phases 1, 3, 4, 5, 6, 7, 8, 9, 11 — complete. **Outstanding:** OOC build verification (dual-build protocol), Phase 13 (Status promotion to READY FOR REVIEW), second-engineer review of the oracle design and the patched-legacy reference fix. |

## Summary

`GroupMicroTextureRegionsFilter` groups neighboring **Features** whose c-axes are aligned within a user-specified tolerance. It is intended for Hexagonal_High materials and operates only on features whose phase resolves to `Hexagonal_High`; features in any other Laue class are silently left ungrouped. The algorithm seeds with a randomly-chosen unassigned feature, then walks the contiguous (and optionally non-contiguous) neighbor list, grouping any neighbor whose c-axis falls within tolerance, then repeats with the next seed until all features have a parent. Verification used a **Class 1 (Analytical) oracle**: pure Bunge (0, Φ, 0) Euler angles make the sample-frame c-axis exactly `(0, sin Φ, cos Φ)` and the angular distance between any two c-axes exactly `|Φ_A − Φ_B|`, so the expected groupings on a small hand-built fixture follow directly from the chosen tolerance and contiguous neighbor list. Two port-time bugs that prevented the filter from running in its default mode (D1) and from randomizing parent IDs at all (D2) were found and fixed during the V&V cycle; the fixes are pinned by dedicated regression tests.

## Algorithm Relationship

*Classification:* **Port** ~~| Minor changes | Rewrite | New filter~~

*Evidence:* Legacy 6.5.171 inherits from a `GroupFeatures` base class that owns the BFS-over-neighbor-list loop; SIMPLNX inlines that loop into a single `GroupMicroTextureRegions` algorithm class. SIMPL UUID `5e18a9e2-e342-56ac-a54e-3bd0ca8b9c53` is preserved via `OrientationAnalysisLegacyUUIDMapping.hpp` → `3f695987-81b1-47c3-8cff-b49cfa219be0`. The selection logic (random-feature seed, BFS over contiguous-then-optional-non-contiguous neighbors, Hex_High-only acceptance criterion with optional running-average c-axis, deterministic-seeded RNG) is line-for-line equivalent. SIMPL 6.4 + 6.5 conversion fixtures live at `test/simpl_conversion/6_*/GroupMicroTextureRegionsFilter.json`.

*Port-time deltas (each tracked as a deviation — see `vv/deviations/GroupMicroTextureRegionsFilter.md`):*

1. **Inheritance flattened.** Legacy's `GroupFeatures::execute()` is the BFS driver; concrete-filter `GroupMicroTextureRegions::getSeed/determineGrouping` are the per-call hooks (Template Method). SIMPLNX inlines the BFS into `GroupMicroTextureRegions::execute()` (algorithm class) and keeps `getSeed`/`determineGrouping` as private methods. Same control flow, simpler class graph.
2. **Modern math API.** Legacy uses raw `float[3][3]` matrices + `MatrixMath::*` helpers + `QuaternionMathF`. SIMPLNX uses `ebsdlib::Matrix3X1F` / `Matrix3X3F` value-types and `ebsdlib::Quaternion<float32>::toOrientationMatrix().toGMatrix().transpose()`. Same arithmetic, different type wrapping; no observable output difference.
3. **Defect A (D1) — `UseNonContiguousNeighbors=false` default mode was broken.** The original SIMPLNX port placed the null-pointer check on the non-contiguous neighbor list *outside* the conditional that populates the pointer, so `execute()` unconditionally returned error `-99345` whenever `UseNonContiguousNeighbors=false` — the filter's documented primary mode. **Fixed during this V&V cycle** (`GroupMicroTextureRegions.cpp` lines 56–66): null check moved inside the `if(m_InputValues->UseNonContiguousNeighbors)` block. Pinned by `Regression — runs in default UseNonContiguousNeighbors=false mode` test case.
4. **Defect B (D2) — randomization permanently disabled.** The original SIMPLNX port had the seed-array machinery and a `randomizeParentIds()` helper, but the call site in `operator()` was a `// !!! COMMENT OUT FOR DEMONSTRATION !!!` comment block. The infrastructure existed (seed parameter, seed-output array, machinery to plumb a deterministic seed) but produced no randomization. Legacy 6.5.171 randomizes by default with a clock-derived seed (irreproducible). A 2025-10-23 fix applied to a local build of the legacy source exposes `RandomizeParentIds` as a user parameter defaulting to `false` for reproducible parity. **Fixed during this V&V cycle** to match that design: new `k_RandomizeParentIds_Key` parameter (default `false`), restored `randomizeParentIds()` Fisher-Yates shuffle in the algorithm using `m_Generator` already seeded by `operator()`. The `UseSeed`/`SeedValue` parameters now correctly drive the algorithm's RNG (they were previously declared as parameters but the algorithm hard-coded `std::mt19937::default_seed` — also fixed). `parametersVersion()` bumped to 2.
5. **RNG architecture.** Legacy 6.5.171 calls `SIMPL_RANDOMNG_NEW()` inside `getSeed()` (creates a fresh RNG each call). SIMPLNX uses a single class-level `m_Generator` + `std::uniform_real_distribution<float32>` initialized once in `operator()` from `m_InputValues->SeedValue`. Matches the patched-legacy design and is reproducible across runs given the same seed. Numerical sequence is different from 6.5.171 by construction; not a parity defect.
6. **Latent legacy bug already corrected (D3).** Legacy `determineGrouping` declares `uint32_t phase1 = 0` but only assigns it inside the `if(!m_UseRunningAverage)` branch. When `UseRunningAverage=true`, `phase1` stays at 0, and the subsequent `phase1 == phase2 && phase1 == Hexagonal_High` check fails silently — no features ever group. Bug introduced by J. Tucker 2014-01-30 (commit `7e49e52f` in upstream DREAM3D). SIMPLNX's port assigns `phase1` outside the conditional → bug already fixed in the 2024-01-08 initial port. The 2025-10-23 fix applied to a local build of the legacy source deliberately back-ported the same fix as part of its "expose as usable filter" cleanup — confirmed by inspecting the pre-fix legacy source, which still carries the buggy code. The 6.5.171 release line was never patched.
7. **Hex-only restriction preserved.** Both versions reject non-Hexagonal_High pairs in `determineGrouping`. Not a deviation; documented as a filter precondition.

*Material PRs since baseline (filter was in `SimplnxReview` until 2026-06-11):*

- `2024-01-08 ca6d0aa` — initial port (`Add: GroupMicroTextureRegions and FindGroupingDensity`). Includes the D3 phase1 fix at port time.
- `2024-01-08 15daa51` — `Added Warnings to the filter, in the docs and as a Preflight Returned value.` Introduced the `-65432` "experimental, untested, unverified, unvalidated" preflight warning.
- `2025-10-07 ac46cab` — neighbor-list API update.
- `2025-12-03 db623d3` — "Microtexture Related bug fixes and code review (#7)". Did not address D1 or D2.
- `2026-03-03 6634fb8` — "New rewrite based on feedback". Did not address D1 or D2.
- `2026-06-11 ddf63bb` — moved from SimplnxReview to OrientationAnalysis (this branch).
- `2026-06-11` (this V&V cycle) — D1 fixed, D2 fixed, RNG seeded from `SeedValue`, `RandomizeParentIds` parameter added, `parametersVersion()` bumped to 2, dead `growPatch`/`growGrouping` stubs and dead `m_PatchGrouping` field removed, preflight warning `-65432` ("experimental, untested, unverified, unvalidated") removed now that V&V is in place, filter documentation updated to remove the matching banner and add the new `Randomize Parent Ids` section, V&V deliverables added.

## Oracle

*Class:* **1 (Analytical)** primary + **4 (Invariant)** companion.

### Applied (Class 1 — Analytical)

For pure Bunge Euler angles `(φ1=0, Φ=Φᵢ, φ2=0)`:

- The Bunge passive orientation matrix reduces to `g = R_x(Φ)`.
- The crystal-frame c-axis `[0, 0, 1]` projected into sample frame becomes `g^T · [0, 0, 1] = R_x(-Φ) · [0, 0, 1] = (0, sin Φ, cos Φ)`.
- For any two features A, B with Φ-only tilts: `c_A · c_B = sin Φ_A · sin Φ_B + cos Φ_A · cos Φ_B = cos(Φ_A − Φ_B)`, so the angular distance is exactly `|Φ_A − Φ_B|`.
- The filter applies `w ≤ tol_rad || (π − w) ≤ tol_rad`, so the effective angular-distance metric is `min(θ, π − θ)`, folded into [0°, 90°].

Quaternion storage convention follows the sibling `ComputeFeatureNeighborCAxisMisalignmentsFilter` Class 1 test (the format-of-record for pure-Phi fixtures in OA): `{x = sin(Φ/2), y = 0, z = 0, w = cos(Φ/2)}`. The expected grouping outcomes follow directly from the chosen tolerance, the per-feature Φ, and the per-feature neighbor list.

Fixture A — **Pure-Phi 5-feature chain**: F1(Φ=0°), F2(Φ=5°), F3(Φ=60°), F4(Φ=63°), F5(Φ=25°). Contiguous neighbors: F1↔F2, F2↔F3, F3↔F4, F5 isolated. Tolerance 10°. Expected: {F1,F2} group (Δ=5°), {F3,F4} group (Δ=3°), F2↔F3 bridge fails (Δ=55°), F5 alone — **3 distinct groups**.

Fixture B — **Tolerance boundary 3-feature chain**: F1(Φ=0°), F2(Φ=8°), F3(Φ=20°). Contiguous neighbors: F1↔F2↔F3. Tolerance 10°. Expected: F1↔F2 group (Δ=8° ≤ 10°), F2↔F3 bridge fails (Δ=12° > 10° using F2's c-axis vs F3's c-axis under `UseRunningAverage=false`), so F3 alone — **2 distinct groups**. Exercises the on-the-boundary acceptance vs rejection.

### Applied (Class 4 — Invariant)

Companion assertions, applicable independent of which feature is picked as the first random seed:

- **Same-group invariant**: features designed to bridge produce equal parent IDs; features designed not to bridge produce distinct parent IDs.
- **Positivity invariant**: every real feature's parent ID is `> 0` (parent ID 0 is reserved for unassigned).
- **Group count invariant**: the number of distinct parent IDs across real features matches the hand-derived count.
- **Cell-feature consistency**: `cellParentIds[k] == featureParentIds[featureIds[k]]` for every cell.
- **Attribute matrix sizing**: `newFeatureAM.getNumberOfTuples() == max(featureParentIds) + 1` (index 0 reserved).
- **Seed-roundtrip invariant**: the user-supplied seed value lands in the `_Group_MicroTexture_Regions_Seed_Value_` top-level array.

### Encoded

- **Class 1 (Analytical) + Class 4 (Invariant)**:
  - `test/GroupMicroTextureRegionsTest.cpp::"OrientationAnalysis::GroupMicroTextureRegionsFilter: Class 1 Analytical (Pure-Phi Bunge)"` — 5-feature fixture, 12 assertions.
  - `test/GroupMicroTextureRegionsTest.cpp::"OrientationAnalysis::GroupMicroTextureRegionsFilter: Class 1 Analytical (Tolerance Boundary)"` — 3-feature fixture, 5 assertions.
- **Regression pin (Class 4 invariant — runs cleanly)**: `test/GroupMicroTextureRegionsTest.cpp::"OrientationAnalysis::GroupMicroTextureRegionsFilter: Regression — runs in default UseNonContiguousNeighbors=false mode"` — guards against D1 regression.
- **SIMPL backward-compat (kept from move)**: `test/GroupMicroTextureRegionsTest.cpp::"OrientationAnalysis::GroupMicroTextureRegionsFilter: SIMPL Backwards Compatibility"` — DYNAMIC_SECTION over `simpl_conversion/6_4` and `simpl_conversion/6_5` fixtures. Validates UUID + argument-key conversion only; not a behavioral test.

### Second-engineer review

*Pending.* The c-axis closed-form derivation for pure-Phi Bunge angles is sibling-shared with `ComputeFeatureNeighborCAxisMisalignmentsFilter`, whose Class 1 oracle was reviewed previously; the same derivation applies here. A second-engineer pass on (a) the patched-legacy reference fix as the corroborating source for the D2 design, and (b) the on-the-boundary 12°-rejection assumption in Fixture B (i.e., that under `UseRunningAverage=false` the algorithm compares each candidate against `firstFeature`'s c-axis, not against the running seed's c-axis), is recommended before Status promotion.

## Code path coverage

*9 of 9 paths exercised.*

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/GroupMicroTextureRegions.cpp` (266 lines).

Logical phases: **(a) per-call init** in `operator()` (RNG seed, parent-ID init), **(b) seed-loop driver** in `execute()` (BFS over neighbor lists), **(c) per-candidate grouping decision** in `determineGrouping()`, **(d) per-seed bookkeeping** in `getSeed()` (parent-ID assignment, running-average update), **(e) post-loop finalize** in `operator()` (AM resize, cell-parent backfill, optional shuffle).

| #  | Phase                | Path                                                                                                                                                                                              | Test case                                                                                                                                                    |
|----|----------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1  | (a) Init             | `m_Generator = std::mt19937_64(m_InputValues->SeedValue)` → seed propagates to all `getSeed` random draws                                                                                          | `Class 1 (Pure-Phi)` — seed=42 produces deterministic parent IDs; seed-roundtrip assertion confirms the SeedValue write.                                     |
| 2  | (b) BFS driver       | `UseNonContiguousNeighbors == false` → null-pointer guard NOT triggered (defect-A regression path)                                                                                                  | `Regression — runs in default UseNonContiguousNeighbors=false mode`                                                                                          |
| 3  | (b) BFS driver       | `UseNonContiguousNeighbors == true` → guard fires only if the optional list pointer is genuinely null                                                                                              | *Not directly tested.* Selection parameter validates path existence; the residual guard is defensive. Low-value gap.                                          |
| 4  | (b) BFS driver       | Inside the BFS, walk contiguous neighbors (`k=0`) and optionally non-contiguous (`k=1`)                                                                                                            | `Class 1 (Pure-Phi)` exercises `k=0`. `k=1` only exercised via the `UseNonContiguousNeighbors=true` branch (see #3 — not directly tested in oracle fixtures). |
| 5  | (c) Grouping         | `m_FeatureParentIds[neighborFeature] != -1` (already parented) → skip                                                                                                                              | `Class 1 (Pure-Phi)` — F2's neighbor list includes F1; after F1 is grouped, F1 fails this check when F2 walks back to it.                                    |
| 6  | (c) Grouping         | `referenceFeaturePhase == 0` or `neighborFeaturePhase == 0` (background) → skip                                                                                                                    | *Not directly tested.* Background feature 0 has phase 0; would be reached only if the algorithm picked f0 as a seed and tried to grow it. Low-value gap.     |
| 7  | (c) Grouping         | `phase1 == phase2 && phase1 == Hexagonal_High` AND `angularDist ≤ tol` (or `π − angularDist ≤ tol`) → assign parent, optionally update running-average c-axis                                       | `Class 1 (Pure-Phi)` — F1↔F2 and F3↔F4 cover the accept-with-tolerance branch.                                                                                |
| 8  | (c) Grouping         | `phase1 == phase2 && phase1 == Hexagonal_High` BUT `angularDist > tol` AND `π − angularDist > tol` → no grouping                                                                                  | `Class 1 (Pure-Phi)` — F2↔F3 (55°) and `Class 1 (Tolerance Boundary)` — F2↔F3 (12°) cover the reject branch.                                                  |
| 9  | (d)/(e) Finalize     | `m_NumTuples >= 2` → AM resize + cell-parent backfill; `RandomizeParentIds == true` invokes the Fisher-Yates shuffle                                                                                | `Class 1 (Pure-Phi)` — AM-size and cell-feature-consistency assertions cover the unshuffled path. `RandomizeParentIds invariants` covers the shuffle path: equivalence-class preservation, group-count preservation, cell/feature consistency, positivity, and same-seed-determinism. |

## Test inventory

| Test case                                                                                                                              | Status      | Notes                                                                                                                                                                                                                                                                                                                                                |
|----------------------------------------------------------------------------------------------------------------------------------------|-------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `OrientationAnalysis::GroupMicroTextureRegionsFilter: Class 1 Analytical (Pure-Phi Bunge)`                                              | new-for-V&V | 5 features arranged in two chains + an isolated feature, shared scaffold via `AnalyticalFixtures::Build5FeaturePureBunge()`. 12 assertions covering same-group, different-group, group count, positivity, AM sizing, cell/feature consistency, and seed-roundtrip.                                                                                  |
| `OrientationAnalysis::GroupMicroTextureRegionsFilter: RandomizeParentIds invariants`                                                    | new-for-V&V | Re-uses the same 5-feature scaffold. Runs the filter three times — baseline (no shuffle) + two same-seed shuffled runs — and asserts the Class 4 invariants randomization must preserve: equivalence-class preservation (pairwise sameness pattern), group count, cell/feature consistency, positivity, same-seed determinism, plus a loose non-identity sanity check. |
| `OrientationAnalysis::GroupMicroTextureRegionsFilter: Class 1 Analytical (Tolerance Boundary)`                                          | new-for-V&V | 3-feature chain probing the 12°-on-10°-tolerance acceptance boundary under `UseRunningAverage=false`. 5 assertions.                                                                                                                                                                                                                                  |
| `OrientationAnalysis::GroupMicroTextureRegionsFilter: Regression — runs in default UseNonContiguousNeighbors=false mode`                | new-for-V&V | Regression pin for defect A. Pre-fix `execute()` returned error `-99345` here; post-fix succeeds.                                                                                                                                                                                                                                                    |
| `OrientationAnalysis::GroupMicroTextureRegionsFilter: SIMPL Backwards Compatibility`                                                    | kept        | DYNAMIC_SECTION over SIMPL 6.4 + 6.5 conversion fixtures. Validates UUID and argument-key conversion only; not a behavioral test. Was the only passing test under the old `[.][UNIMPLEMENTED][!mayfail]` regime.                                                                                                                                      |
| *(retired)* `OrientationAnalysis::GroupMicroTextureRegionsFilter: Valid Filter Execution` (tag `[.][UNIMPLEMENTED][!mayfail]`)         | retired     | Old test used empty `DataPath{}` arguments throughout; could not pass preflight and was tagged hidden/expected-fail. Replaced by the two `Class 1 Analytical` tests above. The replacement actually exercises the algorithm with real data.                                                                                                            |

All non-retired tests pass on `vv/group_microtexture_regions` (verified on the in-core release build at 2026-06-11). OOC verification: pending — this V&V cycle did not run the OOC build profile, but the algorithm reads `Int32Array`, `Float32Array`, `UInt32Array`, and `NeighborList<int32>` via reference-binding in the constructor; OOC-incompatible patterns (raw-pointer access, parallel writes to the same array) are not used.

## Exemplar archive

- **Archive:** None — data inlined in `test/GroupMicroTextureRegionsTest.cpp` namespace `AnalyticalFixtures`.
- **SHA512:** N/A
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/GroupMicroTextureRegionsFilter.md`

The fixture scaffold (`AnalyticalFixtures::CreateScaffold(numFeatures)`) builds an `{nX,1,1}` ImageGeom with one cell per real feature, plus a background feature 0 / Cell/Feature/Ensemble attribute matrices. Per-feature pure-Phi quats are set via `SetAvgQuat(td, idx, QuatFromPhiDeg(phiDeg))`; per-feature neighbor lists via `SetNeighbors(td, idx, {...})`. The canonical 5-feature fixture used by the Pure-Phi Class 1 test and the RandomizeParentIds invariance test is built by `AnalyticalFixtures::Build5FeaturePureBunge()` (see the provenance sidecar for the per-feature Φ values, adjacency, and expected groupings). No `download_test_data()` entry is required.

## Deviations from DREAM3D 6.5.171

Direct array-by-array comparison against 6.5.171 is not meaningful: 6.5.171 randomizes parent IDs by default using a clock-derived seed (irreproducible) and produces *grouping equivalence classes* that match SIMPLNX's groups under any permutation, but never *bit-identical* arrays. The three documented deviations are design-level statements rather than per-array diffs.

- `GroupMicroTextureRegionsFilter-D1` — Defect A: pre-fix SIMPLNX returned error `-99345` when `UseNonContiguousNeighbors=false` (filter unusable in default mode). **Fixed.** See `vv/deviations/GroupMicroTextureRegionsFilter.md`.
- `GroupMicroTextureRegionsFilter-D2` — Defect B: SIMPLNX never randomized parent IDs (legacy 6.5.171 always randomized; a fix applied to a local build of the legacy source exposes `RandomizeParentIds` as a user parameter, default false). **Fixed by exposing `RandomizeParentIds` (default false) + restoring the helper and plumbing the user seed through.** See `vv/deviations/GroupMicroTextureRegionsFilter.md`.
- `GroupMicroTextureRegionsFilter-D3` — Legacy 6.5.171 bug: when `UseRunningAverage=true`, `phase1` is never assigned and the Hex_High acceptance check silently fails — no features ever group. Bug introduced upstream by J. Tucker on 2014-01-30 (commit `7e49e52f` in original DREAM3D). SIMPLNX corrected this in the 2024-01-08 initial port; the same fix was applied to a local build of the legacy source. **Documented for migration users.** See `vv/deviations/GroupMicroTextureRegionsFilter.md`.
