# Retroactive V&V: ComputeAvgOrientationsFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `086ddb9a-928f-46ab-bad6-b1498270d71e` |
| SIMPLNX ClassName | `ComputeAvgOrientationsFilter` |
| SIMPLNX Human Name | Compute Feature Average Orientations |
| SIMPL UUID | *(TBD — confirm in legacy SIMPL repo; legacy class was `FindAvgOrientations`)* |
| SIMPL ClassName | `FindAvgOrientations` |
| SIMPL Human Name | Find Feature Average Orientations |
| Plugin | OrientationAnalysis |

### Source files scanned

- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ComputeAvgOrientationsFilter.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeAvgOrientations.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/test/ComputeAvgOrientationsTest.cpp`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_5/ComputeAvgOrientationsFilter.json`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_4/ComputeAvgOrientationsFilter.json`
- `src/Plugins/OrientationAnalysis/docs/ComputeAvgOrientationsFilter.md`
- `src/Plugins/OrientationAnalysis/test/CMakeLists.txt` (download_test_data entry)

## Algorithm Relationship

- **Tentative classification:** **Port + Major Enhancement.**
  - The original Rodrigues running-quaternion-average path (`computeRodriguesAverage()`) is a direct translation of the legacy SIMPL `FindAvgOrientations` filter. The SIMPLNX UUID is `086ddb9a-…`.
  - On top of the legacy port, the SIMPLNX implementation now also offers two alternative averaging methods — **von Mises-Fisher (vMF)** and **Watson** — added in PR #1577. Both new paths delegate the actual statistics to **EbsdLib's `ebsdlib::DirectionalStats`** class (Expectation-Maximization estimation of the mean direction *mu* and concentration *kappa*).
  - The class is therefore best described as a **Port** of one path (Rodrigues) plus a **New filter behavior** stacked on top (vMF, Watson) under the same UUID. `parametersVersion()` returns `2` and the comment in the filter explicitly documents "Version 2 adds the ability to compute the von Mises-Fisher average and the Watson sampling average; Version 2 also adds the option to NOT compute the Eulers/Quats from the original algorithm."
- **Evidence:**
  - PR #1577 introduces the vMF/Watson paths and the `Use*` linkable boolean parameters.
  - PR #1438 ("ComputeAvgOrientations fix incorrect computation") restructures the original Rodrigues path: the previous code divided the running average by count *before* every accumulation step (incorrect); the new code accumulates the nearest-quaternion sum and divides only at the end (correct Cho/Rollett-style running average). This is a **material correctness fix** vs. SIMPL.
- **Action required:** Confirm the SIMPL `FindAvgOrientations` UUID and behavior, then run `compare-legacy-dream3d` (Step 0 e) on the Rodrigues path against DREAM3D 6.5.172.

## PRs inspected (since 2025-10-01)

> Pruned: pure-style/repo-wide refactor PRs and pure test-infrastructure PRs are listed at the bottom of this section but not detailed individually — they did not change behavior of this filter. **#1472 is also pruned here**: per the EbsdLib-bump exception rule, the diff scoped to this filter was inspected and is API-rename only (`QuatF` -> `ebsdlib::QuatF`, `OrientationTransformation::qu2eu<...>(q)` -> `ebsdlib::QuaternionFType(q).toEuler()`) with no change to the underlying math.

### PR #1438 — *"ENH: Microtexture related filter cleanup"* — merged 2025-10-25 (broad refactor, exception flagged because the commit message explicitly says "BUG: ComputeAvgOrientations fix incorrect computation")

- **Files in this filter:** algorithm (.cpp) only — 120 lines touched (62 +, 58 -)
- **Diff size:** Substantial restructure of the running-quaternion-average loop
- **Change nature:** **Material correctness fix.** The previous algorithm computed `curAvgQuat = (avgQuats[fid*4 + k] / count)` *before* finding the nearest equivalent, then added `nearestQuat` to that already-divided value. The new algorithm:
  1. Reads the current accumulator into `finalAvgQuat`,
  2. Divides a *separate* `curAvgQuat` by `count` to use as the reference for `getNearestQuat()`,
  3. If this is the first voxel for the feature, swaps the reference for the identity quaternion (avoids using a zero vector as reference),
  4. Adds the nearest-equivalent voxel quaternion back into the *un-divided* `finalAvgQuat`,
  5. Defers the final `/= count` and `.normalize().getPositiveOrientation()` until the post-loop pass.
  
  Also adds explicit handling for `counts[featureId] == 0` (writes identity quaternion). This is the canonical Cho/Rollett-style running average — the prior code had a divide-then-add bug that gave drifting averages once `count > 1`. **This is exactly the kind of finding that warrants a Deviation entry vs. legacy SIMPL.**
- **V&V content:** **High.** The PR message also says "TEST: Update ComputeAvgOrientations Unit Test. This is really ONLY testing Cubic High Laue group." — i.e., the test was simultaneously rewritten, but the developer flagged that test coverage is still narrow.

### PR #1472 — *"ENH: Update to EbsdLib 2.0.0 API"* — merged 2025-11-24 (broad refactor, exception evaluated and downgraded — see note below)

- **Files in this filter:** algorithm (.cpp) only — 44 lines touched (23 +, 21 -)
- **Diff size:** Small
- **Change nature:** **API renames only.** Inspected the scoped diff line-by-line:
  - `LaueOps::Pointer` -> `ebsdlib::LaueOps::Pointer`
  - `QuatF` -> `ebsdlib::QuatF`, `QuatD` -> `ebsdlib::QuatD`
  - Two debug `operator<<` overloads commented out (they referenced the now-namespaced types)
  - `OrientationTransformation::qu2eu<QuatF, OrientationF>(curAvgQuat)` -> `ebsdlib::QuaternionFType(curAvgQuat).toEuler()`
- **Risk evaluation:** The Euler-conversion replacement is the only line that *could* be a silent precision change. Both old and new are `float`-precision Bunge-convention conversions and dispatch to the same EbsdLib `qu2eu` math under the hood; this is unlikely to be a behavior change, unlike the Batch B finding for `RotateEulerRefFrame`. Listed in the pruned table to keep this section focused, but flagged here for the developer to confirm during the legacy comparison.
- **V&V content:** None expected. Should be invisible in regression.

### PR #1535 — *"ENH: Remove redundant preflight checks that are already done in the parameter"* — merged 2026-02-18 (broad refactor, exception flagged because it removed substantive preflight logic from this filter)

- **Files in this filter:** filter (.cpp) only — 47 lines touched (2 +, 45 -)
- **Diff size:** Large preflight reduction
- **Change nature:** Deleted hand-written preflight component-count checks for the Crystal Structures (1 component), Quats (4 components), Phases (1 component), and FeatureIds (1 component) arrays. These are now enforced by the `ArraySelectionParameter::AllowedComponentShapes{{N}}` constraints declared in `parameters()`. Also replaced an explicit nullptr check on the `AttributeMatrix` with `getDataRefAs<AttributeMatrix>()`, removed the unused `BoolArray`/`GoodVoxelsArrayType` aliases and three `k_IncorrectInputArray` / `k_MissingInputArray` / `k_MissingOrIncorrectGoodVoxelsArray` constants.
- **V&V content:** **No behavior change** if the parameter-level component-shape constraints are enforced correctly by the framework. **Risk:** any pipeline that previously surfaced one of the now-deleted error codes (`-7000`, `-7001`, `-7002`) will now fail with the framework's generic component-shape validation message instead. Worth a Deviation entry for *error-code stability* if any downstream tooling relies on those numeric codes.

### PR #1577 — *"ENH: Compute Avg Orientations now can use von-Mises Fisher or Watson sampling"* — merged 2026-04-12

- **Files in this filter:** all of them — algorithm (.hpp +28, .cpp +355/-43), filter (.hpp +16, .cpp +141/-58), test (.cpp +71), docs (.md +65). Also touched the test/CMakeLists.txt to switch the archive from `7_ComputeAvgOrientation.tar.gz` to **`7_ComputeAvgOrientation_v2.tar.gz`**.
- **Change nature:** **Major feature addition.** Adds two new averaging methods on top of the existing Rodrigues path, each gated by a new linkable boolean (`UseVonMisesFisher`, `UseWatson`):
  - `computeVmfWatsonAverage()` — runs in parallel via `ParallelDataAlgorithm` with a `VmfWatsonSamplingImpl` worker (one feature = one work item). For each feature: collects all voxel quaternions of that feature, reduces each into the fundamental zone using `LaueOps::getFZQuat`, then calls `ebsdlib::DirectionalStats` ("VMF" or "WAT" mode) to run an EM estimation with `NumEMIterations=5`, `NumIterations=10`, `RandomSeed=43514` (all hard-coded — see filter docs). Outputs `mu` (as both quaternion and Euler) and `kappa`.
  - Special-cased: a single-voxel feature uses that quaternion directly without running EM; an empty feature is filled with `NaN`.
  - The original Rodrigues path was renamed `computeRodriguesAverage()` and bumped `parametersVersion()` to 2.
  - Subcommit "BUG: Fix phaseIndex calculation bug" — confirms a bug was uncovered & fixed during this work; the in-tree `computeRodriguesAverage()` now reads the current voxel's phase as `phases[i]` (an integer >= 1 for valid voxels) and indexes `crystalStructures[currentPhase]`, the long-established correct pattern.
  - The unit test (`ComputeAvgOrientationsTest.cpp`) was extended with `UseWatson_Key=true` and `UseVonMisesFisher_Key=true` plus exemplar comparison blocks for both new outputs against fields named `Watson Avg Quats`, `Watson Avg EulerAngles`, `vMF Avg Quats`, `vMF Avg EulerAngles` in the new exemplar archive.
- **V&V content:** **Very high** — this PR adds substantial new math that delegates to EbsdLib. No paper citations are included in the source, but the documentation references *"algorithms from the EMsoft software library."* The Oracle for the vMF/Watson paths is therefore *whatever EbsdLib's `DirectionalStats::EMforDS` produces* — a transitive dependency on EbsdLib's tests. **A separate Oracle Provenance block for the vMF/Watson exemplars in `7_ComputeAvgOrientation_v2.tar.gz` is required.**

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** test (.cpp) +47 lines, plus two new fixture files
  - `test/simpl_conversion/6_4/ComputeAvgOrientationsFilter.json` (~1.4 KB)
  - `test/simpl_conversion/6_5/ComputeAvgOrientationsFilter.json` (~1.5 KB)
- **Change nature:** **Test addition.** Added a per-filter SIMPL→SIMPLNX backwards-compatibility test exercising both SIMPL 6.4 (Filter_Name fallback) and 6.5 (UUID-mapped) pipeline conversion paths via `DYNAMIC_SECTION`. Test name: `"OrientationAnalysis::ComputeAvgOrientationsFilter: SIMPL Backwards Compatibility"`.
- **V&V content:** **Pipeline-conversion correctness only** — verifies that opening a legacy SIMPL pipeline produces a `ComputeAvgOrientationsFilter` instance with the right UUID and the seven expected parameter values populated. Does **not** verify that the filter's *output* matches legacy.

### Pruned PRs (touched the file but not behaviorally relevant to this filter)

| PR | Subject | Why pruned |
|---|---|---|
| #1457 | Clean up 'static inline' from filter headers | Style only (filter .hpp, 14 lines, 7+/7-) |
| #1458 | Update EbsdLib to 1.0.40 | 1-line include adjustment in algorithm |
| #1472 | Update to EbsdLib 2.0.0 API | API renames only after scoped diff inspection (see exception note above); risk flagged for developer follow-up on Euler-conversion call |
| #1476 | Fix Backwards Pipeline Compatibility and Add Testing | 5 lines in filter.cpp — `FromSIMPLJson()` adjustment for AttributeMatrixSelection mapping; behavior is conversion only |
| #1524 | Fixed filter tags to consistently use the full filter name | Test cosmetic |
| #1538 | Replace cmake subprocess tar.gz extraction with zlib | Test infrastructure (1-line change in test cpp) |
| #1543 | Update pipeline references in each of the documentation files | Doc currency only (3 lines) |
| #1547 | Fix filter documentation and documentation related code bugs | Doc currency only (1 line) |

## Test coverage detected

`ComputeAvgOrientationsTest.cpp` contains **2** `TEST_CASE`s:

1. `OrientationAnalysis::ComputeAvgOrientations` — Loads the exemplar `.dream3d` (which already contains the input `FeatureIds`, `Phases`, `Quats`, `CrystalStructures`, plus the legacy-name reference `AvgQuats`, `AvgEulerAngles`, `Watson Avg Quats`, `Watson Avg EulerAngles`, `vMF Avg Quats`, `vMF Avg EulerAngles`). Runs the filter with **all three** methods enabled (`UseRodriguesAverage=true`, `UseWatson=true`, `UseVonMisesFisher=true`). Compares the six computed arrays against the matching exemplars using `CompareFloatArraysWithNans<float32>` with tolerance `5.0E-7f`. Also calls `CheckArraysInheritTupleDims`.
2. `OrientationAnalysis::ComputeAvgOrientationsFilter: SIMPL Backwards Compatibility` — SIMPL 6.4 + 6.5 conversion paths via `DYNAMIC_SECTION` *(added by PR #1588)*. Verifies UUID, comments, and seven parameter values.

**Coverage gaps observed:**
- Per the developer's own comment in PR #1438 ("This is really ONLY testing Cubic High Laue group"), there is no test for hexagonal, trigonal, tetragonal, or other Laue classes. The fundamental-zone reduction `LaueOps::getFZQuat` and the per-Laue-class symmetry path are therefore exercised only for `m-3m`.
- No test for a feature with a single voxel (the explicit `fzQuats.size() == 1` short-circuit in the vMF/Watson path is not covered).
- No test for an "all voxels are phase 0" edge case (the NaN-fill path on the vMF/Watson side, and the identity-quaternion path on the Rodrigues side).
- No test that varies `RandomSeed` / `NumEMIterations` / `NumIterations` (they are hard-coded constants today).
- No test of the linkable parameters individually — only the all-three-true permutation is exercised.

## Exemplar archive

- **Archive name:** `7_ComputeAvgOrientation_v2.tar.gz`
- **SHA512:** `2c2a691f1da301c449c20bafec65512d5134db38384ac7cb4c910880ccd87a260a5f011e905f35b97abff3952309f109c737c63ec3c833708926827a62a92efc`
- **Referenced in:** `src/Plugins/OrientationAnalysis/test/CMakeLists.txt` line 133
- **Provenance:** Bumped from `7_ComputeAvgOrientation.tar.gz` (v1) to the `_v2` archive in PR #1577 when the vMF and Watson exemplars were added. The exemplars for `Watson Avg Quats`, `Watson Avg EulerAngles`, `vMF Avg Quats`, `vMF Avg EulerAngles` were almost certainly produced by the same DREAM3DNX build that introduced the new code paths — i.e., the **oracle is self-referential** for the two new methods. *(TBD — engineer must inspect the archive contents and confirm whether a `ReadMe.md` and a generating `.d3dpipeline` are inside.)*
- **Action required:**
  1. Download the archive locally and confirm whether a `ReadMe.md` / `.d3dpipeline` exists inside, and what the input `.dream3d` was.
  2. For the **Rodrigues** outputs (`AvgQuats`, `AvgEulerAngles`), determine whether the v1 exemplars were inherited from a SIMPL run or were also self-referential. If self-referential, the exemplars are no longer a legacy-equivalence oracle (because PR #1438 was a behavior change vs. SIMPL), only a regression oracle.
  3. Promote everything found into an Oracle Provenance block per Step 0 policy.

## Oracle classification (tentative)

The brief recommends **Class 3 (Paper-based, Rowenhorst et al. 2015 + Glez & Driver 2001 / Cho, Rollett, Oh 2005)** for symmetric quaternion averaging, with a Class-4 invariant companion. After reading the source I propose a **per-method classification**, because this single filter UUID exposes three substantially different algorithms:

| Method | Recommended class | Rationale |
|---|---|---|
| **Rodrigues running average** (`computeRodriguesAverage()`) | **3 (Paper-based)** + **4 (Invariant)** companion | The math is the standard symmetric-quaternion running average (Cho, Rollett & Oh, *Materials Science Forum* 2005, building on Glez & Driver, *J. Applied Crystallography* 2001; see also Rowenhorst et al. 2015 §6 for the modern review). Defending this is straightforward. The Class-4 invariants in the brief hold by inspection of the code (see below). |
| **Von Mises-Fisher (vMF)** (`computeVmfWatsonAverage()` "VMF" mode) | **3 (Paper-based)** + **2 (External-tool comparison)** | Math is delegated to `ebsdlib::DirectionalStats::EMforDS` and is described in the documentation as "based on algorithms from the EMsoft software library." Paper basis is Banerjee, Dhillon, Ghosh & Sra, *J. Machine Learning Research* 2005 ("Clustering on the Unit Hypersphere using von Mises-Fisher Distributions") combined with the EMsoft directional-stats implementation (Singh & De Graef, *Modelling and Simulation in Materials Science and Engineering* 2016). External-tool comparison oracle = run the same input through EMsoft and compare. |
| **Watson** (`computeVmfWatsonAverage()` "WAT" mode) | **3 (Paper-based)** + **2 (External-tool comparison)** | Watson distribution on quaternions: Sra & Karp, *Statistics and Computing* 2013 ("The Multivariate Watson Distribution") and the EMsoft implementation. Same external-tool oracle approach. |

**Class 4 (Invariant) companion checks** that should be hard assertions in the test suite:

1. **Single-voxel feature:** for a feature where exactly one voxel has that featureId, `AvgQuat == that voxel's quaternion` (after the post-pass `.normalize().getPositiveOrientation()`). The vMF/Watson code paths do this trivially (`if(fzQuats.size() == 1) muhat = fzQuats[0]`); the Rodrigues path also satisfies it because of the `counts[featureId] == 1.0f -> curAvgQuat = QuatF::identity()` guard added in PR #1438.
2. **Identical-orientation feature:** for a feature where every voxel has the same orientation Q, `AvgQuat == Q` (modulo sign-flip into the northern hemisphere). True by construction for all three methods.
3. **Unit norm:** `|AvgQuat| == 1.0` for every populated feature. The Rodrigues path explicitly calls `.normalize()`; the vMF/Watson paths return EM-estimated unit quaternions.
4. **Empty feature:** `AvgQuat == identity` for the Rodrigues path; `AvgQuat == NaN` and `AvgEuler == NaN` for the vMF/Watson paths. (These are *different* sentinel choices for the same condition across methods — flag for documentation.)
5. **Northern-hemisphere convention:** `AvgQuat.w() >= 0` for every populated feature (Rodrigues calls `.getPositiveOrientation()`; vMF/Watson call `.positiveOrientation()`).

**Class 1 (Analytical) check from the brief:** a 2-voxel feature with orientations differing by a small angle `dθ` has average ≈ midpoint within `dθ²/8`. This is straightforward to encode for the Rodrigues path; for the EM-based vMF/Watson paths it should hold *only if `kappa` is large* (i.e., the EM estimator collapses to the geometric mean for tightly-clustered samples).

**Action required:** Developer to confirm/replace this per-method classification.

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | **Partial** | PR #1438's commit message does not include an explicit `REV:` line for ComputeAvgOrientations (only for ComputeFeatureNeighbor* and ComputeFeatureReferenceCAxisMisorientations), but PR #1438 *was* the developer-driven correction pass. PR #1577 introduced new code that has not been line-by-line reviewed in any visible PR. |
| Code path coverage (algorithmic) | **Weak** | Only one TEST_CASE covers the algorithmic paths; runs all three methods together against Cubic-High exemplars. No per-method test, no non-cubic Laue-class test, no degenerate-feature test. |
| Code path coverage (SIMPL conversion) | **Good** | PR #1588 added SIMPL 6.4 + 6.5 conversion test. |
| Exemplar data in Data_Archive | **Yes** | `7_ComputeAvgOrientation_v2.tar.gz` referenced in test/CMakeLists.txt; was bumped from v1 in PR #1577. |
| Exemplar provenance documented | **No** | TBD by inspecting archive contents. The vMF/Watson exemplars in v2 are very likely self-referential (no published reference dataset exists). |
| Oracle class recorded | **No** | This document is the first to propose one (per-method). |
| Toy data / independent expected output (Step 0 c) | **No** | No independent script or hand-derived 2-quaternion test on file. |
| Legacy comparison report (Step 0 e) | **No** | `compare-legacy-dream3d` has not been run. The Rodrigues path almost certainly disagrees with SIMPL 6.5.172 because of PR #1438 (drift bug fix) and possibly because of the deleted error-code constants in PR #1535. |
| Deviation entries (`ComputeAvgOrientations-D<N>`) | **None** | Not yet written. PRs #1438, #1535, #1577 are all strong candidates — see "Recommended Deviation entries" below. |
| Documentation currency | **Good** | Documentation rewritten in full by PR #1577 — covers all three methods, the EM hard-coded parameters, the special cases for empty / single-voxel features, and explicitly notes the NaN-vs-identity sentinel difference. PRs #1543 and #1547 made minor pipeline-reference and capitalization fixes. Needs accuracy audit per `review-filter-docs`. |
| Verification archive (OneDrive) | **No** | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

1. **a. Confirm the oracle (per-method).** Promote the per-method table above into the policy doc. Adopt Class 3 + Class 4 for Rodrigues; Class 3 + Class 2 (EMsoft external comparison) for vMF and Watson.
2. **b. Promote the existing implicit invariants into explicit Class-4 assertions.** The exemplar-comparison TEST_CASE today only checks "matches v2 archive within tolerance." Add explicit `REQUIRE`s for the five invariants enumerated above.
3. **c. Hand-derive a 2-quaternion analytical case.** Two voxels with quaternions differing by a small angle `dθ`; compare against `dθ²/8` bound for the Rodrigues path. Record as an additional small TEST_CASE.
4. **d. Add Laue-class coverage.** At minimum hexagonal (HCP), since the surrounding microtexture pipelines (hex C-axis filters) are the historical motivation. PR #1438's commit explicitly flags this gap.
5. **e. Run `compare-legacy-dream3d` against DREAM3D 6.5.172.** Expect at least one Deviation entry on the Rodrigues path (PR #1438 fixed a drifting average that legacy still has). Optionally compare vMF/Watson against EMsoft as the external-tool oracle for Class 2.
6. **Inspect `7_ComputeAvgOrientation_v2.tar.gz`** and document provenance. Determine the input data for the v2 exemplars; record whether the v1 Rodrigues exemplars were inherited from SIMPL or are also self-referential.
7. **Produce the Algorithm Relationship one-liner.** Tentative: *"Port + Major Enhancement — Rodrigues path is a corrected port of SIMPL `FindAvgOrientations` (drift bug fixed in #1438), plus two new EM-based averaging methods (vMF, Watson) added in #1577 that delegate to EbsdLib `DirectionalStats`."*
8. **Archive everything** per `archive-filter-verification` for the OneDrive folder.

## Recommended Deviation entries (proposed, pending legacy comparison)

> **Deviation ID:** `ComputeAvgOrientations-D1`
> **Filter UUID:** `086ddb9a-928f-46ab-bad6-b1498270d71e`
> **Symptom:** SIMPLNX (post-PR-#1438) produces a different average quaternion for any feature with `count > 1` than SIMPL 6.5.172 produces on the same input.
> **Root cause:** Bug in 6.5.172 — the legacy running-average loop divided the accumulator by `count` *before* every nearest-quaternion lookup, then added `nearestQuat` back into the already-divided value, causing the running average to drift downward as `count` increased. SIMPLNX rewrote this loop in PR #1438 to accumulate the un-divided sum and divide only once at the end (the canonical Cho/Rollett-Oh form).
> **Affected users:** Any pipeline that consumes `AvgQuats` / `AvgEulerAngles` for features containing 2+ voxels (i.e., essentially all real datasets). The discrepancy grows with feature size.
> **Recommendation:** Trust SIMPLNX. Legacy was wrong. Engineer to evaluate whether a backport patch to 6.5.172 is needed for users who must reproduce exact legacy values.
> **Status:** Proposed — pending verification by running 6.5.172 on the same input. *Highly likely* given the explicit "BUG: ComputeAvgOrientations fix incorrect computation" line in the PR #1438 commit body.

> **Deviation ID:** `ComputeAvgOrientations-D2`
> **Filter UUID:** `086ddb9a-928f-46ab-bad6-b1498270d71e`
> **Symptom:** SIMPLNX (Version 2) ships two additional averaging methods, von Mises-Fisher and Watson, that do not exist in SIMPL 6.5.172.
> **Root cause:** New feature added in PR #1577. Behavior is gated behind `UseVonMisesFisher` / `UseWatson` boolean parameters which both default to `false`, so default-pipeline behavior is unchanged from legacy. When enabled, the EM estimation is delegated to `ebsdlib::DirectionalStats` (transitive dependency on EbsdLib's tested implementation, itself derived from EMsoft).
> **Affected users:** Anyone enabling the new methods. No effect on default-pipeline outputs.
> **Recommendation:** Document as a SIMPLNX-only feature. The v2 exemplars in `7_ComputeAvgOrientation_v2.tar.gz` for the Watson and vMF outputs are the only available oracle today; an EMsoft cross-check (Class 2 external-tool oracle) is recommended before publishing as a verified result.
> **Status:** Proposed — feature addition rather than a bug, but worth recording as a versioning Deviation.

> **Deviation ID:** `ComputeAvgOrientations-D3`
> **Filter UUID:** `086ddb9a-928f-46ab-bad6-b1498270d71e`
> **Symptom:** Pipelines that previously surfaced numeric error codes `-7000` / `-7001` / `-7002` from this filter's preflight no longer do; they now surface the framework's generic component-shape-mismatch error.
> **Root cause:** PR #1535 deleted the hand-written component-count checks (Crystal Structures must be 1 component, Quats must be 4 components, etc.) on the grounds that `ArraySelectionParameter::AllowedComponentShapes{{N}}` already enforces the same constraint at the parameter level. The constants `k_IncorrectInputArray = -7000`, `k_MissingInputArray = -7001`, `k_MissingOrIncorrectGoodVoxelsArray = -7002` were also removed from the .cpp.
> **Affected users:** Any external tooling that pattern-matches on the numeric error codes when reporting preflight failures.
> **Recommendation:** Document the change in the user-facing changelog; no algorithmic deviation.
> **Status:** Proposed — low severity. Verify that the framework-level check fires *before* the algorithm runs on a malformed input.
