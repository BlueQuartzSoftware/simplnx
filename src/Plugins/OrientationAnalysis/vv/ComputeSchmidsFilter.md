# V&V Report: ComputeSchmidsFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `b4681855-0a3d-4237-97f2-5aec509115c4` |
| DREAM3D 6.5.171 equivalent | `FindSchmids` (SIMPL UUID `e67ca06a-176f-58fc-a676-d6ee5553511a`) - `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindSchmids.{h,cpp}` |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | READY FOR REVIEW |
| Sign-off | Delegated to the PR reviewer (requester decision, 2026-08-18). Second-engineer oracle review is the PR review. |

## At a glance

| Aspect | Current state |
|---|---|
| Algorithm Relationship | Port with one port-time regression, and that one latent. The SIMPL/EbsdLib API changed and the arithmetic moved from `float` to `double`, but the control flow is line-for-line the legacy filter. What was lost in the port is the legacy `initValue 0` on the created arrays (SC-1) - real as a source-level regression, but never observable in-core because the store factory hard-codes a zero `initValue` of its own; see *Mutation verification*. A second defect, the unbounded phase-id index, is shared with legacy (SC-4). |
| Oracle (confirmed) | Class 1 (Analytical) - **12 value-asserting fixtures**: 5 auto-path loading directions, 4 exactly-representable quaternions, 3 override-path cases. Plus Class 4 (Invariant) checks for the physical bound `0 <= m <= 0.5`, `m == Phis * Lambdas`, `Poles == trunc(100 * crystalLoading)`, scale invariance and cubic-symmetry invariance, and 5 further non-value fixtures (the `StoreAngleComponents` toggle, the `LaueGroupEnd` skip row, the two phase-id guards and the non-normal slip-direction rejection). Cross-checked at 80 significant digits by `ww_work/ComputeSchmids/oracle.py`, except the two override-path `acos` values, which are transcendental and were instead recomputed to 60 significant digits and rounded to the nearest `double`. |
| Code paths enumerated | 11 of 12 paths exercised - only the cancel path is untested. |
| Tests today | 6 test cases (**601 assertions**, measured): 1 archive-consuming exemplar test, 3 new Class 1 / Class 4 test cases, 1 new options-and-guards test case, 1 SIMPL backwards-compatibility test. |
| Exemplar archive | `6_6_stats_test_v2.tar.gz` (unchanged, not regenerated). Its three float Schmid arrays were generated with EbsdLib <= 3.1.0 and are now stale by a known factor; the test asserts that exact relationship instead of equality. See *Exemplar archive*. |
| Legacy comparison | Run 2026-08-20 against DREAM3D 6.5.171 `PipelineRunner` on 10 hand-built fixtures (9 executing + 1 negative). 93 predicted relationships confirmed, 0 unexplained differences, 1 prediction amended after adjudication (D5). |
| Bug flags | SC-1 (`ComputeSchmidsFilter-D2`, fixed defensively - latent in-core, see *Mutation verification*), SC-2 (`-D1`, fixed in EbsdLib), SC-4 (fixed, no deviation - shared gap now guarded), SC-5 (`-D3`, fixed in EbsdLib + mitigated in NX; twelve further SC-2-class hexagonal divisors recorded there as **known-open**). |
| V&V phase | Phases 1-13 complete. Outstanding: OOC build run (waived, requester decision 2026-08-18); second-engineer sign-off (delegated to the PR reviewer, requester decision 2026-08-18). **This PR is merge-blocked until EbsdLib 3.1.1 is released** - see *Merge dependency*. |

## Summary

`ComputeSchmidsFilter` computes, for each Feature, the largest Schmid factor over the slip systems of that Feature's crystal structure, given a sample-frame loading direction and the Feature's average orientation. Verification used a Class 1 analytical oracle: the Schmid factor, the two direction cosines, the winning slip-system index and the fixed-point `Poles` encoding were all derived by hand from the algorithm source for identity and three exactly-representable rotated quaternions, then cross-checked in exact arithmetic. The cycle found and fixed four defects — two of them inside EbsdLib — and documented five deviations from DREAM3D 6.5.171, of which four were predicted from source before the comparison was run.

## Merge dependency

This branch bumps `vcpkg.json`'s `ebsdlib` constraint from `"version>=": "3.1.0"` to `"3.1.1"` because two of the four fixes live in EbsdLib (`topic/3_1_1_staging`, commits `4a56725` and `2c84f2a`) and the Class 1 oracle asserts the post-fix values to `1e-6`. Against EbsdLib 3.1.0 the new fixtures fail by roughly `7.4e-5`, which is the whole point of the fix.

**Consequence, accepted by the requester on 2026-08-20:** the PR cannot pass stock CI until EbsdLib 3.1.1 is published to the vcpkg registry. Until then `vcpkg` rejects the manifest with `no version database entry for ebsdlib at 3.1.1` at *configure* time, in every preset — including the local `SIMPLNX_USE_LOCAL_EBSD_LIB=ON` preset, because the manifest's version constraints are resolved regardless of which features are enabled. All of the verification in this report was therefore performed with the bump **not** applied, against the local EbsdLib source through `NX-Com-Qt69-Vtk96-Rel-EbsdLib`, and the bump was applied last as a manifest-only change. Do not reconfigure any build directory on this branch until 3.1.1 exists.

## Algorithm Relationship

**Port**, with one port-time regression, and that one latent.

`ComputeSchmids::operator()` is a statement-for-statement transcription of `FindSchmids::execute()`. Port-time deltas:

1. **`float` to `double`.** Legacy holds the orientation matrix, the crystal-frame loading direction, the Schmid factor and the angle components in `float`; NX computes all of them in `double` and narrows only on store. Output arrays are `float32` in both. This tightens accuracy and does not change any decision — confirmed empirically, see D1.
2. **`qu2om` and the quaternion layout are unchanged.** EbsdLib's `Quaternion<T>::toOrientationMatrix()` is line-identical to legacy's `qu2om`, the component order is `(x, y, z, w)` in both, and `epsijk == +1` (`OrientationFwd.hpp` defines `DREAM3D_PASSIVE_ROTATION`), so neither applies a transpose. The A/B run confirms this: `Poles`, which is a direct function of the orientation matrix, is bit-identical on every fixture.
3. **The `initValue` was dropped.** Legacy creates `Schmids`, `SlipSystems` and `Poles` with `createNonPrereqArrayFromPath(..., 0, ...)`. NX's `CreateArrayAction` calls passed no fill value. **This is SC-1, a port regression**, fixed here — but the exposure it carried turned out to be nil in-core: the store factory supplies its own zero `initValue` and `DataStore`'s constructor always applies it, so the rows read zero either way. Established by experiment, not assumed; see *Mutation verification*.
4. **The skip condition was inverted in form but not in meaning.** Legacy wraps the body in `if(xtal < LaueGroupEnd)`; NX uses `if(laueClass >= LaueGroupEnd) continue;`. Equivalent.
5. **Loop-scoped versus hoisted locals.** Both hoist `schmid`, `angleComps` and `ss` outside the Feature loop. Combined with EbsdLib Laue ops that do not write every output (SC-5), this leaks the previous Feature's angle components into a skipped Feature's row. NX now declares them inside the loop.
6. **Error code renumbered.** The non-normal slip plane/direction rejection is `-1001` in legacy and `-13500` in NX. Same condition, same message text, both at preflight. See D4.

**Material PRs since baseline:** none identified for this filter.

## Oracle

**Class 1 (Analytical)** with **Class 4 (Invariant)** support, and Class 3 as corroboration only.

The oracle was derived by transcribing three sources by hand — `ComputeSchmids.cpp`, EbsdLib's `Quaternion<T>::toOrientationMatrix()`, and both `CubicOps::getSchmidFactorAndSS` overloads — into closed-form expressions, evaluating them exactly, and writing the results into the test as inline expected values with the derivation in a comment above each. The derivation is reproduced independently at 80 significant digits by `ww_work/ComputeSchmids/oracle.py`, which parses EbsdLib's own 24-element symmetry table out of the source rather than re-typing it.

Identity quaternion, `Cubic_High`, auto slip-system path:

| Loading | m | cos phi (Phis) | cos lambda (Lambdas) | SlipSystems | Poles |
|---|---|---|---|---|---|
| `[0,0,1]` | `1/sqrt(6)` = 0.4082482904638630 | `1/sqrt(3)` = 0.5773502691896258 | `1/sqrt(2)` = 0.7071067811865476 | 0 | (0, 0, 100) |
| `[1,1,1]` | `2/(3 sqrt(6))` = 0.2721655269759087 | `1/3` = 0.3333333333333333 | `2/sqrt(6)` = 0.8164965809277260 | 4 | (57, 57, 57) |
| `[0,1,1]` | `1/sqrt(6)` = 0.4082482904638630 | `2/sqrt(6)` = 0.8164965809277260 | `1/2` = 0.5 | 1 | (0, 70, 70) |
| `[1,2,3]` | `8/sqrt(294)` = 0.4665694748158435 | `4/sqrt(42)` = 0.6172133998483701 | `4/sqrt(28)` = 0.7559289460184544 | 10 | (26, 53, 80) |

`[0,0,1]` and `[1,1,1]` and `[0,1,1]` all carry ties at the maximum and pin the strict-`>` tie-break to the lowest index. `[1,2,3]` has a unique maximum and pins the enumeration order. `[0,1,1]` is the row that separates `Phis` from `Lambdas` (0.8165 versus 0.5) — on the other three rows a swapped pair would go unnoticed. `[3,6,9]` repeats `[1,2,3]` to check that the loading direction is normalized.

Rotated quaternions, loading `[1,2,3]`. Each is exactly representable in `float32` and produces an orientation matrix whose entries are all `0` or `+/-1`, so there is no floating-point slack in the expected `Poles`. All four are cubic symmetry operations, so `m` and both angle components must be identical across them while `SlipSystems` and `Poles` move — a Class 4 symmetry invariance riding on the Class 1 values:

| Quaternion (x,y,z,w) | Orientation matrix | crystalLoading | SlipSystems | Poles |
|---|---|---|---|---|
| `(0,0,0,1)` identity | `I` | `(1,2,3)/sqrt(14)` | 10 | (26, 53, 80) |
| `(0,0,1,0)` 180 deg about Z | `diag(-1,-1,1)` | `(-1,-2,3)/sqrt(14)` | 8 | (-26, -53, 80) |
| `(0.5,0.5,0.5,0.5)` 120 deg about [111] | `[[0,0,1],[1,0,0],[0,1,0]]` | `(3,1,2)/sqrt(14)` | 6 | (80, 26, 53) |
| `(1,0,0,0)` 180 deg about X | `diag(1,-1,-1)` | `(1,-2,-3)/sqrt(14)` | 1 | (26, -53, -80) |

Override slip-system path (`(001)[100]`, identity quaternion) — a different EbsdLib overload with different semantics for both the index and the angle components:

| Loading | m | symmetry-operator index | Phis (radians) | Lambdas (radians) |
|---|---|---|---|---|
| `[1,2,3]` | `3/7` = 0.4285714285714286 | 3 | 0.6405223126794245 | 1.0068536854342678 |
| `[1,1,1]` | `1/3` = 0.3333333333333333 | 0 | 0.9553166181245093 | 0.9553166181245093 |
| `[0,0,1]` | 0 | 0 | 0 | 0 |

The three angle columns on this path are `acos` values, i.e. transcendental rather than surds, so they are not covered by `oracle.py`'s exact-arithmetic table. They were recomputed independently to 60 significant digits — `acos(3/sqrt(14)) = atan(sqrt(5)/3) = 0.640522312679424574143…`, `acos(2/sqrt(14)) = atan(sqrt(10)/2) = 1.006853685434267776537…`, `acos(1/sqrt(3)) = atan(sqrt(2)) = 0.955316618124509278163…` — and rounded to the nearest `double`. The asserted tolerance is `1e-6`, far looser than that rounding.

Class 4 invariants asserted alongside the Class 1 values:

- `0 <= m <= 0.5` — the physical bound for a cubic crystal. **This became assertable only after the EbsdLib fix**: EbsdLib <= 3.1.0 reached 0.500090176 at the maximizing direction. Verified over a 400 x 400 direction sweep in `oracle.py`.
- `m == Phis * Lambdas` on the auto path (the stored values are cosines) and `m == cos(Phis) * cos(Lambdas)` on the override path (the stored values are angles). The pair of assertions is what proves the SC-3 unit flip is real behaviour rather than a documentation misreading.
- `Poles[k] == trunc(100 * crystalLoading[k])`, including negative components, which is what distinguishes truncation from rounding.
- Scale invariance: `[1,2,3]` and `[3,6,9]` agree on all five outputs.
- Cubic-symmetry invariance: all four quaternions give the same `m`, `Phis` and `Lambdas`.

**Class 3 corroboration (not the oracle):** the three canonical FCC anchors fall out of the table — `m = 0.408` for `<001>` and `<011>` loading and `m = 0.272` for `<111>` loading, with `phi = 54.74 deg` / `lambda = 45 deg` and `phi = 70.53 deg` / `lambda = 35.26 deg` respectively (Dieter, *Mechanical Metallurgy* 3rd ed. section 4-3; Schmid & Boas, *Kristallplastizität*). These are quoted as a sanity check on the Class 1 derivation, not as its source.

**Encoded test references** (all greppable in `src/Plugins/OrientationAnalysis/test/ComputeSchmidsTest.cpp`):

- `"OrientationAnalysis::ComputeSchmidsFilter: Class 1 analytical oracle, auto slip system"`
- `"OrientationAnalysis::ComputeSchmidsFilter: orientation-matrix convention and equivariance"`
- `"OrientationAnalysis::ComputeSchmidsFilter: override slip system path"`
- `"OrientationAnalysis::ComputeSchmidsFilter: options, skip path and phase guards"`

## Bugs found and fixed

| ID | Where | Defect | Fix |
|---|---|---|---|
| SC-1 | simplnx, latent port regression | All five output arrays created with `CreateArrayAction`'s default empty fill value where legacy allocated with `initValue 0`. Feature 0 and any Feature whose Laue class has no slip systems are never written by the algorithm, so those rows depend entirely on how the store was initialized — and the filter was stating no requirement about that. Measured exposure in-core: **none**, because `CoreDataIOManager` hands every in-core `DataStore<T>` a zero `initValue` of its own. | Explicit `"0"` fill on all five `CreateArrayAction` calls, plus an explicit Feature-0 row write. Defensive: it makes the filter's requirement explicit instead of inherited. |
| SC-2 | EbsdLib (Verdict C) | `CubicOps::getSchmidFactorAndSS` normalized with the `float` literals `1.732f` and `1.414f` inside an otherwise all-`double` computation. Both are smaller than the constants they approximate, inflating every Schmid factor by `sqrt(6)/(1.732f*1.414f) = 1.00018035284` and letting `m` exceed the physical maximum of 0.5. | `ebsdlib::constants::k_Sqrt3D` / `k_Sqrt2D`. EbsdLib `4a56725`. |
| SC-4 | shared with legacy | `crystalStructures[featurePhases[i]]` was unbounded — a phase id at or beyond the ensemble count, or a negative one, read outside the array and dispatched on whatever that read produced. | Bounds guard in the algorithm: `-13501` for too-large, `-13502` for negative, both naming the offending value and the ensemble count. |
| SC-5 | EbsdLib (Verdict C) + simplnx mitigation | `HexagonalLowOps::getSchmidFactorAndSS` **read** `schmidfactor` uninitialized and never initialized `slipsys`; `HexagonalOps` left `slipsys` and `angleComps` undefined; the seven Laue classes with no enumerated slip systems set `schmidfactor` and `slipsys` but left `angleComps` untouched, so a caller with a hoisted buffer got the previous Feature's angles. | All four outputs seeded in every affected overload (EbsdLib `2c84f2a`), plus NX-side per-iteration reinitialization of the three locals so the filter is correct against any EbsdLib. |

## Code path coverage

`Source: src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeSchmids.cpp (137 lines).`

The algorithm is flat: a parameter setup block, a sentinel-row write, then one pass over Features.

11 of 12 paths exercised.

| # | Phase | Path | Test case |
|---|---|---|---|
| 1 | *Preflight* | Create the three unconditional output arrays with a zero fill | all four V&V test cases |
| 2 | *Preflight* | Create `Phis`/`Lambdas` when `StoreAngleComponents` is true | "Class 1 analytical oracle, auto slip system" |
| 3 | *Preflight* | Skip `Phis`/`Lambdas` when `StoreAngleComponents` is false | "options, skip path and phase guards" / "StoreAngleComponents == false does not create Phis/Lambdas" |
| 4 | *Preflight* | Reject a slip direction not lying in the slip plane (`-13500`) | "override slip system path" / "preflight rejects a slip direction that is not in the slip plane" |
| 5 | *Execute* | Zero the Feature-0 sentinel row | "Class 1 analytical oracle, auto slip system" |
| 6 | *Execute* | Normalize the loading direction | "Class 1 analytical oracle, auto slip system" / loading `[3,6,9]` |
| 7 | *Execute* | Normalize the user slip plane and direction (`OverrideSystem` true) | "override slip system path" |
| 8 | *Execute - per-Feature* | Negative phase id guard (`-13502`) | "options, skip path and phase guards" / "a negative phase id is an error" |
| 9 | *Execute - per-Feature* | Out-of-range phase id guard (`-13501`) | "options, skip path and phase guards" / "a phase id beyond the ensemble count is an error" |
| 10 | *Execute - per-Feature* | Skip a Feature whose Laue class is at or beyond `LaueGroupEnd` | "options, skip path and phase guards" / "features whose Laue class is beyond LaueGroupEnd get defined zeros" |
| 11 | *Execute - per-Feature* | Auto slip-system path (`getSchmidFactorAndSS(load, ...)`) | "Class 1 analytical oracle, auto slip system"; "orientation-matrix convention and equivariance" |
| 12 | *Execute - per-Feature* | `m_ShouldCancel` early return | *Not directly tested. No cancel-signal injection infrastructure exists for algorithm classes; same gap as the rest of this plugin.* |

## Test inventory

| Test case | Status | Notes |
|---|---|---|
| `"OrientationAnalysis::ComputeSchmidsFilter"` | kept, modified | Consumes `6_6_stats_test_v2.tar.gz`. The two integer arrays (`SlipSystems`, `Poles`) still assert exact equality with the archive — both are provably invariant under the SC-2 fix. The three float arrays now assert the archive equals the freshly computed value times its predicted per-component bias factor, because the archive predates EbsdLib 3.1.1. 75 assertions. See *Exemplar archive*. |
| `"OrientationAnalysis::ComputeSchmidsFilter: Class 1 analytical oracle, auto slip system"` | new-for-V&V | 5 `DYNAMIC_SECTION` loadings x (5 outputs + 6 Class 4 invariants + the Feature-0 sentinel row). 221 assertions. |
| `"OrientationAnalysis::ComputeSchmidsFilter: orientation-matrix convention and equivariance"` | new-for-V&V | 4 `DYNAMIC_SECTION` quaternions pinning the `qu2om` convention, the no-transpose property, negative `Poles` truncation and cubic-symmetry invariance. 105 assertions. |
| `"OrientationAnalysis::ComputeSchmidsFilter: override slip system path"` | new-for-V&V | 3 `DYNAMIC_SECTION` loadings on the second EbsdLib overload, including the degenerate zero-Schmid case, plus the radians-versus-cosines assertion (SC-3) and the `-13500` preflight rejection. 84 assertions. |
| `"OrientationAnalysis::ComputeSchmidsFilter: options, skip path and phase guards"` | new-for-V&V | `StoreAngleComponents == false`; the `LaueGroupEnd` skip row asserting zeros over a 20 000-Feature dirty-heap fixture (SC-1); the `-13501` and `-13502` guards (SC-4). 83 assertions. |
| `"OrientationAnalysis::ComputeSchmidsFilter: SIMPL Backwards Compatibility"` | kept, untouched | 2 `DYNAMIC_SECTION` conversion fixtures (SIMPL 6.5 UUID, SIMPL 6.4 Filter_Name). 33 assertions. |

**601 assertions total** (75 + 221 + 105 + 84 + 83 + 33), taken from a `ctest --verbose` run rather than tallied by hand. All 6 pass at the verified commit in the in-core build. The out-of-core build run is **waived** (requester decision, 2026-08-18).

Full `OrientationAnalysis::` regression: 269 of 269 unit tests pass. 28 `PIPELINE::` / `PY::` tests fail in this build directory for an environment reason, not a code reason — the `NX-Com-Qt69-Vtk96-Rel-EbsdLib` directory was built with only the `SimplnxCore` and `OrientationAnalysis` plugin targets and no Python bindings (2 of 6 plugins, no `simplnx.cpython-312-darwin.so`), so the first pipeline in each chain fails with `ModuleNotFoundError: No module named 'simplnx'` and every downstream pipeline then fails with `Input file does not exist`.

## Mutation verification

Six mutations, each applied to the fixed tree, rebuilt, run, and reverted; the transcript is `ww_work/ComputeSchmids/mutation_transcript.txt` and the driver is `mutate.sh`. Both repositories were proven byte-identical to the pre-mutation snapshot afterwards, and the baseline re-ran green. The tree the matrix ran against is byte-identical to the committed tree except for the single `vcpkg.json` manifest line bumping `ebsdlib` from `3.1.0` to `3.1.1`, which was applied last (see *Merge dependency*) and which affects only dependency resolution, no compiled code.

The `Killed by` column gives `ctest` ordinals as they stood in `NX-Com-Qt69-Vtk96-Rel-EbsdLib` at the time of the run: 163 = the archive-consuming test, 164 = Class 1 auto, 165 = orientation-matrix convention, 166 = override path, 167 = options/guards, 168 = SIMPL BC. Ordinals are not stable — later building `EbsdLibUnitTest` into the same directory shifted them to 570-575. The test **names** are the stable identifiers.

| # | Mutation | Killed by | Verdict |
|---|---|---|---|
| M1 | Transpose the orientation matrix before rotating the loading direction | tests 163, 165 | killed |
| M2 | Swap quaternion components x and y on read | tests 163, 165 | killed |
| M3 | Round the `Poles` scaling instead of truncating | tests 163, 164, 165, 166, 167 | killed |
| M4 | Slip-system tie-break `>` becomes `>=` (EbsdLib `CubicOps`, all 11 comparisons) | test 164 | killed |
| M5 | Revert the SC-1 explicit zero fill | *nothing* | **SURVIVOR** — behaviour-preserving in-core; see below |
| M6 | Revert both SC-4 phase-id guards | test 167 | killed |

M2 initially survived: the first three fixture quaternions all have `x == y`, so swapping them is a no-op on every one of them, and only the archive-consuming test caught it. A fourth quaternion, `(1,0,0,0)`, was added specifically to close that gap — swapping its x and y turns a 180 deg rotation about X into one about Y, moving `SlipSystems` from 1 to 4 and `Poles` from `(26,-53,-80)` to `(-26,53,-80)`. M2 is killed by the hand fixtures after that addition.

**M5 survives, and the reason is now established rather than presumed.** The original write-up said only that no deterministic RED was obtainable on this platform, having tried two things that both produced nothing: poisoning the heap across six malloc size classes, and re-running under `MallocNanoZone=0 MallocPreScribble=1 MallocScribble=1`. Two null results with no mechanism is not a finding, so the decisive variant was run.

**Experiment.** The `LaueGroupEnd` skip fixture was enlarged from 3 Features to **20 000**, so every output array is 78 KB or more rather than 8-36 bytes, with same-size blocks poisoned to `0xAB` and freed immediately before the run; the explicit `"0"` fill was then removed from all five `CreateArrayAction` calls and the case re-run. **Result: still all zeros — 0 non-zero values across all five arrays and all 19 999 uncomputed rows.** A standalone probe over the same size ladder confirms the allocator is not the reason: `new float[n]` after a same-size dirty free returns `0xABABABAB` for every `n` at or above 4096 elements (16 KB) and zeros below it, which both explains why the earlier 8-36-byte attempts saw nothing and rules out "libmalloc was kind" as the explanation at 20 000.

**Mechanism.** The zeros are written by SIMPLNX. `CoreDataIOManager::addDataStoreFnc()` constructs every in-core store as `std::make_unique<Float32DataStore>(tupleShape, componentShape, 0.0f)` — a hard-coded zero `initValue`, independent of the action's `fillValue` string — and `DataStore`'s constructor unconditionally `std::fill_n`s the buffer with it (`DataStore.hpp:66-69`). `CreateArrayAction`'s `fillValue` only adds a second, redundant `store->fill()`. So SC-1's premise, that an empty `fillValue` leaves the buffer default-initialized, is **false for the in-core store**, and M5 cannot be killed because there is nothing to kill: the mutation is behaviour-preserving in-core.

**What that changes.** SC-1 is reclassified from an active defect to a **latent** one: the dropped `initValue` is a genuine source-level port regression, but no in-core release produced indeterminate output because of it. The explicit fill is kept as a defensive, self-documenting statement of the filter's requirement and as the only guarantee for a store implementation whose factory does not hard-code a zero. The enlarged fixture is kept because it makes the assertion non-vacuous: at 20 000 Features it would catch a store that stopped filling, which the 3-Feature version provably could not. *Evidence class: executed (the negative result and the probe), plus source for the mechanism.*

The SC-2 mutation needs no separate entry — the entire pre-fix baseline run is it. Against EbsdLib 3.1.0 the new fixtures failed with exactly the predicted pre-fix values (`m = 0.4083219171` against an expected `0.4082482905`, `Phis = 0.5773671865` against `0.5773502692`, `Lambdas = 0.7072135806` against `0.7071067812`), recorded in `ww_work/ComputeSchmids/red_transcript.txt`.

## Exemplar archive

`6_6_stats_test_v2.tar.gz`, SHA512 `e84999dec914d81efce4fc4237c49c9bf32e48381b1e79f58aa4df934f0d7606cd7a948f9a5e7b17a126a7944cc531b531cfdc70756ca3e2207b20734e089723`, matching `src/Plugins/OrientationAnalysis/test/CMakeLists.txt:130`. **Neither the archive nor its `download_test_data()` line was changed.** Provenance sidecar: `vv/provenance/6_6_stats_test_v2.md`.

The archive's `Schmids`, `Schmid_Phis` and `Schmid_Lambdas` arrays were generated with EbsdLib <= 3.1.0 and are therefore stale by a known amount. Rather than retire the comparison — which would discard 214 real regression checks — or loosen it into meaninglessness, the test now asserts the **relationship** between archive and fresh output:

| Array | archived / freshly computed | Source of the factor |
|---|---|---|
| `Schmid_Phis` | 1.0000293384785181 | `sqrt(3) / 1.732f` |
| `Schmid_Lambdas` | 1.0001510099261918 | `sqrt(2) / 1.414f` |
| `Schmids` | 1.0001803528351113 | `sqrt(6) / (1.732f * 1.414f)` |

That is strictly stronger than the equality it replaces: it pins both the new values and the exact size and shape of the change, and it fails if the fix perturbed anything the bias analysis did not account for. The predicted `Schmid_Lambdas` factor `1.00015100993` was met by the observed `1.00015102083` — agreement to `1.1e-8` relative, across the whole Small IN100 Feature set. Regenerating the archive against EbsdLib >= 3.1.1 would set all three factors to 1.0; that is deliberately left for a future cycle so this PR does not also churn shared test data.

## Deviations from DREAM3D 6.5.171

Run 2026-08-20. Legacy binary `~/Applications/DREAM3D.app/Contents/Bin/PipelineRunner` (6.5.171); NX binary `DREAM3D-Build/NX-Com-Qt69-Vtk96-Rel-EbsdLib/Bin/nxrunner`, built against the fixed local EbsdLib. One shared hand-built legacy-format input carrying six Features — a sentinel, the four oracle quaternions, and one Feature whose phase has no slip systems — driven through 10 pipeline pairs (5 auto-path loadings, one `StoreAngleComponents = false`, 3 override-path loadings, 1 negative case). Predictions were written to `ww_work/ComputeSchmids/predictions.txt` **before** `compare.py` was run.

Result: **93 predicted relationships confirmed, 0 unexplained differences, 1 prediction amended.**

`SlipSystems` and `Poles` were bit-identical on all five auto-path fixtures. Every auto-path float matched its predicted bias factor to better than `1e-7` relative. On the override path, where neither implementation uses the truncated literals, the floats agreed to `1.2e-7` relative with no systematic bias — the control that proves the D1 divergence is the normalizer and nothing else.

**No 6.5.172 surgical patch was produced for D1, deliberately.** Legacy's Schmid arithmetic lives in `OrientationLib/LaueOps/CubicOps.cpp`, not in the filter, so patching the filter could not bring the two into alignment and patching `OrientationLib` would be a library change rather than the "smallest possible diff, one filter per patch" the protocol asks for. The alignment proof here is instead that **fixed-EbsdLib NX reproduces the exact-arithmetic oracle** to `1e-6` on all 12 value-asserting Class 1 fixtures, which the unit tests establish directly.

- `ComputeSchmidsFilter-D1` — Schmid factor and angle components inflated by a uniform +0.018% in 6.5.171 (and in NX before EbsdLib 3.1.1). Trust SIMPLNX.
- `ComputeSchmidsFilter-D2` — skipped Features: `Phis`/`Lambdas` read `-301` in 6.5.171 and `0` in NX; pre-fix NX read `0` by way of the in-core store factory's hard-coded zero rather than the filter's own intent (latent port regression — see the D2 entry). Trust SIMPLNX.
- `ComputeSchmidsFilter-D3` — EbsdLib Laue ops left `angleComps` and, for `HexagonalLowOps`, `schmidfactor`/`slipsys` undefined. Fixed in EbsdLib 3.1.1-staging plus an NX-side mitigation. D3 also records **twelve known-open SC-2-class divisors** in the hexagonal plane-normal geometry (`0.8164` for `2/sqrt(6)`, `1.154` for `2/sqrt(3)`), deferred because they can move the argmax and so need a hexagonal oracle of their own, and the new hexagonal `slipsys = 0` sentinel.
- `ComputeSchmidsFilter-D4` — `Phis`/`Lambdas` change units with `OverrideSystem` (cosines versus radians), and the default array names differ between versions. Documented, not changed.
- `ComputeSchmidsFilter-D5` — on the override path `SlipSystems` is a table-relative symmetry-operator index; the two libraries order the same 24-element group differently and six operators tie at the maximum, so 6.5.171 reports 8 where NX reports 3 for an identical physical answer. Either acceptable; documented.

## Documentation changes

`docs/ComputeSchmidsFilter.md` gained: the loading-direction normalization and sample-frame semantics; the Feature-0 and skipped-Feature zero behaviour; the tie-break rule and that it is enumeration-order rather than physics; the two incompatible `SlipSystems` numbering schemes and the table-relative, tie-arbitrary nature of the override index (D5); the `Phis`/`Lambdas` unit flip (D4); and a correction of the claim implicit in the name `Poles` — it is a fixed-point encoding of the crystal-frame unit loading direction, truncated toward zero, not a Miller index. The *Example Pipelines* citation was corrected from `(05) SmallIN100 Crystallographic Statistics` to the actual file name, `(04) Small IN100 Crystallographic Statistics`. The `SlipSystems` section additionally now states the cubic-0 versus hexagonal-1 numbering base and the two situations in which a reported `0` means "no slip system found" rather than slip system number 0, including the new hexagonal case introduced by the D3 fix.

**Title convention.** The filter's `humanName()` is **"Compute Schmid Factors"**, and the user-facing doc is titled that way. This report, the deviations file and the commit title use the short form **"Compute Schmids"**, matching the class name `ComputeSchmidsFilter` and the short-form convention used across this V&V batch. Both names refer to the same filter, UUID `b4681855-0a3d-4237-97f2-5aec509115c4`.
