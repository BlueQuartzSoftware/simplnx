# V&V Report: ComputeBiasedFeaturesFilter

|                             |                                                                              |
|-----------------------------|------------------------------------------------------------------------------|
| Plugin                      | SimplnxCore                                                                  |
| SIMPLNX UUID                | `d46f2fd7-dc68-4b57-bca3-693016512b2f`                                       |
| SIMPLNX Human Name          | Compute Biased Features (Bounding Box)                                       |
| DREAM3D 6.5.171 equivalent  | `FindBoundingBoxFeatures` — SIMPL UUID `450c2f00-9ddf-56e1-b4c1-0e74e7ad2349` |
| Verified commit             | *<filled at SBIR deliverable assembly>*                                      |
| Status                      | READY FOR REVIEW                                                             |
| Sign-off                    | *delegated to the PR reviewer (requester decision, 2026-08-19)*               |

## At a glance

| Aspect                 | Current state |
|------------------------|----------------|
| Algorithm Relationship | **Port** — legacy `FindBoundingBoxFeatures.{h,cpp}` (Plugins/Generic/GenericFilters, *not* Statistics) was diffed line-by-line against `Algorithms/ComputeBiasedFeatures.cpp` this pass. The greedy box shrink, the nearest-face selection with its first-wins tie-break, the inclusive classification and the per-phase reset are structurally identical. Three divergences found on the 2D path — one SIMPLNX-only, one shared, one legacy-only — plus one SIMPLNX-only output-initialization gap and one SIMPLNX-only empty-range dereference. See Bug Fixes and Deviations. |
| Oracle (confirmed)     | **Class 1 (Analytical) + Class 4 (Invariant).** **Twelve** self-contained fixtures across **eleven** `TEST_CASE`s (A, B1, B2, B3, C, D, E, E0, F, G, H, I), whose expected `BiasedFeatures` vectors were hand-derived from the algorithm source (full per-feature shrink and classify traces are frozen in comments above each `TEST_CASE`) before any code was run, using dyadic geometry values so the inclusive face comparisons are exact. **Eleven of the twelve were carried through the legacy A/B — fixture I has no legacy counterpart** (see Deviations). Three invariants (I1 feature-0 untouched, I2 every classified surface feature biased, I3 box never grows) are asserted on top. Confirmed — 14 `TEST_CASE`s, 14/14 pass. |
| Code paths enumerated  | **26 of 28 exercised.** Two gaps, each with its own row in the table below: the four `m_ShouldCancel` early-returns (no cancel-signal injection) and the `std::out_of_range` guard around `InstantiateMaskCompare` (unreachable through the `IFilter` API). |
| Tests today            | **14 `TEST_CASE`s, all pass** (in-core; OOC runs waived — see Outstanding). Twelve analytical/invariant fixtures across eleven `TEST_CASE`s (3D baseline with a bool/uint8 mask-type sweep, 3D no-shrink, the greedy-order pair B2/B3 in one `TEST_CASE`, non-zero origin + anisotropic spacing, phase-by-phase, 2D Z-normal with origin, 2D Z-normal at origin ignoring phases, 2D X-normal, 2D Y-normal, box-floor, degenerate geometry), plus two negative `TEST_CASE`s (a two-section preflight-error case and the zero-tuple guard case) and the untouched SIMPL backwards-compatibility case. |
| Exemplar archive       | **`6_6_find_biased_features.tar.gz` — RETIRED this pass.** It was the sole consumer of that archive (confirmed by repo-wide grep before and after removal) and it was a circular oracle: the "exemplar" `BiasedFeatures` array was legacy output, so the test asserted agreement with DREAM3D 6.5.171 rather than with an independent expected value — and it could not have caught D1, because its 2D container was a Z-normal slab at the origin, the one 2D configuration where all three 2D defects are inert. The `download_test_data()` block is removed from `test/CMakeLists.txt`; the filter now has no archive dependency. |
| Legacy comparison      | **Run.** Eleven of the twelve unit-test fixtures were re-emitted as legacy-format `.dream3d` inputs and driven through both DREAM3D 6.5.171 `PipelineRunner` and `nxrunner`; fixture I is excluded because the legacy `DataContainerReader` cannot round-trip a zero-dimension `ImageGeom`. **8 of 11 identical; 3 diverge, all three predicted from the legacy source read before the run.** SIMPLNX matched the hand-derived oracle on 11 of 11. Root cause for both shared/legacy defects was then proven by surgically patching a local build of the legacy source: the patched build reproduces the fixed SIMPLNX output and the oracle on all 11 fixtures. |
| Bug flags              | `ComputeBiasedFeaturesFilter-D1` (2D classification used the wrong axes — **shared with 6.5.171**, fixed in SIMPLNX this pass) and `ComputeBiasedFeaturesFilter-D2` (2D origin ignored — **6.5.171 only**, SIMPLNX already correct). `ComputeBiasedFeaturesFilter-D3` (phase-count scan includes the unassigned slot) is documented as a non-output difference, not a bug flag. Three additional SIMPLNX-only defects were found and fixed and are **not** deviations because 6.5.171 was correct on all three — see Bug Fixes. |
| V&V phase              | Oracle chosen, hand-derived and confirmed; all 28 code paths enumerated; legacy comparison run and every divergence adjudicated; three SIMPLNX-only bugs and one shared bug fixed; alignment with the patched legacy build proven; exemplar archive retired; user documentation corrected; 10-mutation verification run. Outstanding before promotion to COMPLETE: reviewer sign-off, the OOC build run (waived), the uncovered cancel paths, and the D3 phase-count scan recommendation. |

## Summary

`ComputeBiasedFeaturesFilter` flags **Features** whose *centroid* lies on or outside a bounding box that has been greedily shrunk away from the *centroids* of the surface **Features**, so that size-correlated edge effects can be excluded from downstream statistics. Verification is **Class 1** — twelve fixtures across eleven `TEST_CASE`s, with per-feature shrink/classify traces hand-derived from the algorithm source before execution — reinforced by three **Class 4** invariants, and then compared against DREAM3D 6.5.171 on eleven of those twelve fixtures (fixture I has no legacy counterpart).

Four bugs were found and fixed this pass. Three are SIMPLNX-only port regressions or gaps with no deviation entry, because DREAM3D 6.5.171 was correct on all three: the 2D bounding box was built from `spacing[0]`/`spacing[1]` regardless of which axis was flat (legacy remapped the spacing per axis); the created `BiasedFeatures` array carried no fill value, so on a degenerate geometry that satisfies neither the 3D nor the 2D dispatch test the user received an uninitialized boolean mask (legacy passed an explicit `false` init value); and zero-tuple Feature data passed preflight and then dereferenced `std::max_element` over an empty range (legacy's phase scan was an index loop and could not run off the end). The fourth — the 2D classification comparing the raw X/Y centroid components against an axis-shifted box — is **shared with 6.5.171** and is written up as `ComputeBiasedFeaturesFilter-D1`. All 14 tests pass, and the legacy comparison lands exactly on the three predicted divergences with no surprises.

## Algorithm Relationship

*Classification:* **Port** ~~| Minor changes | Rewrite | New filter~~

*Evidence:* `SimplnxCoreLegacyUUIDMapping.hpp:176` maps legacy SIMPL UUID `450c2f00-9ddf-56e1-b4c1-0e74e7ad2349` directly to `FilterTraits<ComputeBiasedFeaturesFilter>`, and `test/simpl_conversion/{6_4,6_5}/ComputeBiasedFeaturesFilter.json` carry the legacy `CalcByPhase` / `CentroidsArrayPath` / `SurfaceFeaturesArrayPath` / `PhasesArrayPath` / `BiasedFeaturesArrayName` parameter set unchanged. Legacy source (`Source/Plugins/Generic/GenericFilters/FindBoundingBoxFeatures.{h,cpp}` from a sibling `DREAM3D` checkout on the authoring engineer's machine, not committed to this repository) was diffed line-by-line against `Algorithms/ComputeBiasedFeatures.cpp` this pass rather than inferred from documentation. Note the legacy filter lives in the **Generic** plugin and is named `FindBoundingBoxFeatures`, not `FindBiasedFeatures` and not in Statistics.

*Port-time deltas:*

1. **3D bounding box source** — legacy calls `imageGeom->getBoundingBox(float*)`, which fills `{xMin, xMax, yMin, yMax, zMin, zMax}`; SIMPLNX calls `getBoundingBoxf()` and unpacks `getMinPoint()`/`getMaxPoint()` into the same six slots. Both evaluate to `origin` and `origin + dims * spacing` per axis (`SIMPL/.../ImageGeom.cpp:460-468` vs `src/simplnx/DataStructure/Geometry/ImageGeom.cpp:129-134`). No output change — confirmed by the 3D A/B matching on all seven 3D fixtures.
2. **Surface-feature access** — legacy reads a raw `bool*` from a `DataArray<bool>`; SIMPLNX goes through `MaskCompareUtilities::InstantiateMaskCompare`, which additionally accepts `uint8`. Additive: the `bool` behaviour is unchanged (fixture A asserts both types produce identical output), and the widened parameter type is a usability improvement over legacy's `DataArray<bool>`-only requirement.
3. **Phase count derivation** — legacy loops from index 1; SIMPLNX uses `std::max_element` over the whole array. No output change; this is `ComputeBiasedFeaturesFilter-D3`.
4. **2D in-plane spacing remap** — legacy remaps per axis via `std::tie(zRes, xRes, yRes) = getResolution()` and friends; SIMPLNX read `spacing[0]`/`spacing[1]` unconditionally. **This one did change output** and is the first bug fixed below.
5. **2D in-plane origin** — legacy leaves `xOrigin`/`yOrigin` at 0 and never assigns them; SIMPLNX reads the geometry origin. Changes output in SIMPLNX's favour — `ComputeBiasedFeaturesFilter-D2`.
6. **Output array initialization** — legacy's `dataCheck()` created `BiasedFeatures` via `createNonPrereqArrayFromPath<DataArray<bool>, AbstractFilter, bool>(this, tempPath, false, cDims)`, an explicit `false` init value; SIMPLNX's `CreateArrayAction` was constructed with the default empty fill value. Both algorithm branches immediately `fill(false)`, so this only mattered on the dispatch fall-through — the third bug fixed below.
7. **Cancel checks** — SIMPLNX checks `m_ShouldCancel` at four points; legacy has none. Additive; no output change on a run to completion.
8. **Progress messaging** — SIMPLNX emits a per-phase `Working on Phase N of M` info message through the `MessageHandler`; legacy used `notifyStatusMessage`. No output change.

*Material PRs since baseline:* `(none identified for this filter)` — `git log` on `Algorithms/ComputeBiasedFeatures.cpp` shows only rename/store-API/warning churn between the original port and this branch; no commit changed the 2D branch's arithmetic.

*SIMPLNX implementation:* `Algorithms/ComputeBiasedFeatures.cpp` (362 lines), two sibling routines `findBoundingBoxFeatures()` (3D) and `findBoundingBoxFeatures2D()`, dispatched on the geometry dimensions.

## Bug Fixes (this pass)

### Fix 1: the 2D bounding box ignored which axis was flat (SIMPLNX-only port regression — no deviation entry)

`findBoundingBoxFeatures2D()` correctly remapped the point counts, the origin and the centroid component selection for each of the three flat-axis cases, but built the box with `spacing[0]` and `spacing[1]` in all three:

```cpp
boundBox = {xOrigin, xOrigin + xPoints * spacing[0], yOrigin, yOrigin + yPoints * spacing[1], 0, 0};
```

For an X-normal slab the in-plane axes are Y and Z, so the box needed `spacing[1]` and `spacing[2]`. On V&V fixture F (dims (1, 5, 6), spacing (1, 2, 3)) this produced a box of `[0, 5] × [0, 12]` where the geometry is `[0, 10] × [0, 18]` — an undersized box that pushed two of the three surface features outside it, suppressing their shrink entirely. DREAM3D 6.5.171 does the remap correctly (`std::tie(zRes, xRes, yRes) = imageGeom->getResolution()` for the X-flat case), so this is a SIMPLNX-only regression introduced during the port and is reported here rather than as a deviation.

Fixed by resolving `xSpacing`/`ySpacing` in each of the three flat-axis blocks alongside the origin and point count. The three blocks remain independent `if`s rather than `else if`, deliberately, so that on a degenerate geometry with more than one flat axis **the winning axis is unchanged** — the last matching `if` still decides, exactly as before. The fix changes only *which spacing values* that winning axis contributes (previously always `spacing[0]`/`spacing[1]`, now the two in-plane spacings), so no axis-selection behaviour is smuggled in alongside it. No fixture pins a multi-flat-axis geometry such as dims (1, 1, 6); the claim above is source-derived from the unchanged `if` structure rather than executed, and a fixture for it was judged unnecessary because the selection logic is byte-identical pre- and post-fix.

*Evidence:* fixtures F and G failed against the pre-fix code (F at index 4, `true == false`; G at index 1, `false == true`), both at the exact index the hand derivation predicted; both pass after the fix. Mutation M6 (revert to `spacing[0]`/`spacing[1]`) is killed by F and G and by nothing else.

### Fix 2: the 2D classification compared the wrong axes (shared with 6.5.171 — `ComputeBiasedFeaturesFilter-D1`)

The 2D classify loop tested `centroids[3j]` and `centroids[3j + 1]` — always X and Y — against a box defined on the two in-plane axes. Correct only when the flat axis is Z. Full write-up in `vv/deviations/ComputeBiasedFeaturesFilter.md`; fixed by classifying `centroidShift0`/`centroidShift1`, the same components the shrink loop uses.

*Evidence:* mutation M7 (revert to the raw components) is killed by fixtures F and G and by nothing else. On fixture G the pre-fix code additionally violated invariant I2 — the invariant is the automatic detector for this class of defect. The two outcomes on fixture G must be kept apart, because they belong to different defects:

- **Pre-fix SIMPLNX** (this defect *and* Fix 1's SIMPLNX-only spacing regression, both live on the same code path) produced no biased features at all, leaving **three** genuine surface features (indices 1, 2, 3) unbiased.
- **Stock DREAM3D 6.5.171** (this defect only — legacy's spacing remap was already correct) produced `[0,1,0,1,0,0]`, leaving **one** genuine surface feature (index 2) unbiased.

The "three unbiased" figure is therefore the combined pre-fix SIMPLNX outcome, not the deviation's own symptom; `vv/deviations/ComputeBiasedFeaturesFilter.md` records the legacy symptom as one.

### Fix 3: the created output array had no fill value (SIMPLNX-only port regression — no deviation entry)

`preflightImpl()` constructed its `CreateArrayAction` without a fill value. `ArrayCreationUtilities::CreateArray` only fills when `fillValue` is non-empty, and the store factory passes `{}` (an empty `std::optional`) as the init value (`DataIOCollection.cpp:54,59`), so `DataStore::resizeTuples` allocates with `new value_type[newSize]` and leaves the contents indeterminate. Both algorithm branches open with `biasedFeaturesStore.fill(false)`, so this is invisible on every normal run — but `operator()` dispatches to the 3D path only when all three dimensions are `> 1` and to the 2D path only when some dimension `== 1`, so a dimension of **0** satisfies neither test, both branches are skipped, and the filter returns success handing back an uninitialized boolean mask. Legacy has the same dispatch fall-through but its output array was explicitly initialized to `false`, so its result was well defined.

Fixed by passing `"false"` as the `CreateArrayAction` fill value, restoring the legacy guarantee on every path.

*Evidence class: source-derived, not executed.* Fixture I pins the post-fix guarantee, but **no deterministic RED could be produced on this platform**, and the reason is now measured rather than assumed. Three allocator experiments were run against the pre-fix code:

1. Fixture I as shipped (a 4-tuple `bool` array) under `MallocNanoZone=0 MallocPreScribble=1 MallocScribble=1` — passed on three consecutive runs.
2. Fixture I temporarily widened to 4096 features, so the allocation is well clear of the macOS nano zone's 256-byte ceiling and the scribble settings unambiguously apply, under the same environment — passed on two consecutive runs (4111 assertions each).
3. A standalone probe compiled with the same toolchain, to establish what the allocator actually does. This is what explains the two passes: **macOS poisons with `0xAA`, and clang lowers a `bool` load to a byte load masked with `1`, so `0xAA` reads back as `false`.** The probe confirms the poison is present (`firstByte=0xaa`) while every `bool` element still reads `false`; the byte-level checks `0xAA → false`, `0x55 → true`, `0xFF → true`, `0x02 → false` show the mechanism is the low bit, not an inactive setting. A second probe dirtied a same-size block with `0xFF`, freed it, and immediately re-allocated it — reuse confirmed at sizes 4 through 1024 — and every element still read `false`, i.e. this libmalloc zeroes freed blocks.

So the earlier claim that "no deterministic RED was obtainable" stands, but the passing runs were never the evidence for it: **`MallocScribble`/`MallocPreScribble` cannot flip a `bool` on this toolchain at this optimization level by construction**, because the poison byte's low bit is clear, and the free path zeroes. An allocator that poisons with an odd byte (or ASan / a debug `operator new`) would be required. Correspondingly, mutation M10 (drop the fill value again) **survives** — recorded as such in the mutation transcript rather than papered over. The defect is established by reading the allocation path; fixture I is a regression lock, not a reproduction.

### Fix 4: zero-tuple Feature data dereferenced an empty-range `std::max_element` (SIMPLNX-only — no deviation entry)

`dataStructure.validateNumberOfTuples()` in `preflightImpl()` checks only that the selected Feature arrays *agree* on their tuple count, so a Feature `AttributeMatrix` with **zero** tuples satisfies it — *Centroids*, *Surface Features* and *Phases* all report 0 and the check passes. With *Apply Phase by Phase* on, `findBoundingBoxFeatures()` then evaluates

```cpp
numPhases = *std::max_element(phasesStorePtr->begin(), phasesStorePtr->end());
```

`std::max_element` returns `end()` for an empty range, so this dereferences one past the array — undefined behaviour, with the phase-loop bound taken from whatever that read produced. Zero-tuple Feature data is malformed input in its own right on both the 2D and 3D paths, since a Feature array always carries at least the index-0 "unassigned" Feature. Note the guard is a deliberate strictness increase on one previously-succeeding path: with *Apply Phase by Phase* off, a zero-tuple input was not UB (both loops no-op'd) and completed as a no-op; it now fails preflight by design. DREAM3D 6.5.171 was not exposed: its phase scan is an index loop from 1 (`FindBoundingBoxFeatures.cpp:218-224`) that simply does not execute when `size == 0`, leaving `numPhases` at its initial value. So this is SIMPLNX-only and carries no deviation entry.

Fixed by a preflight guard (`Filters/ComputeBiasedFeaturesFilter.cpp`, error `-7461`) that rejects a *Centroids* selection with zero tuples and names the array and the offending count in the message. Added under the standing preflight-guard decision of 2026-08-19.

*Evidence: executed RED → GREEN.* The guard's `TEST_CASE` (`Zero-tuple Feature arrays are rejected in preflight`, two `SECTION`s covering *Apply Phase by Phase* on and off) was written first and run against the pre-guard build, where it failed at both `SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)` sites with `false` — preflight accepted the input — while the other 13 `TEST_CASE`s passed. With the guard in place the suite is 14/14.

## Oracle

*Class:* **1 (Analytical)**, reinforced by **4 (Invariant-based)**

*Applied:* For each fixture the box's initial extent was computed from `origin` and `origin + dims * spacing`; then, walking the surface features in ascending index order exactly as the source does, the six (or four, in 2D) face distances were evaluated by hand, the winning face selected under the source's strict-`<` first-wins tie-break, and the box updated — or left alone where the centroid sat on or outside a face and `move` was forced to 0. Each feature's *centroid* was then compared against the final box under the source's inclusive `<=` / `>=` tests. The complete trace for every fixture is frozen in a comment block immediately above its `TEST_CASE`, so a reviewer can re-check the arithmetic without re-deriving it. Every geometry value, origin, spacing and centroid component is dyadic (`0.5`, `0.25`, `3.75`, `10.25`, `20.5`, …) so that a centroid lying exactly on a face is an exact float32 equality rather than a near-miss. Where the brief's sketch used `3.8`, the fixture uses `3.75` instead — same trace, exactly representable.

The Class 4 layer asserts, for every valid fixture: **I1** `Biased[0] == false` (feature 0 is the unassigned bucket; fixtures A and C plant deliberately hostile data there to prove both loops skip it); **I2** every classified surface feature is biased, which follows from the shrink either pulling a face onto that feature's own centroid or the feature already sitting on/outside one, and which is the automatic detector for D1 on non-Z-normal slabs; **I3** the box never grows, pinned by fixture H placing a probe exactly on each of the six full-geometry faces when no feature is a surface feature.

*Encoded:* `src/Plugins/SimplnxCore/test/ComputeBiasedFeaturesTest.cpp` — twelve fixtures (A, B1, B2, B3, C, D, E, E0, F, G, H, I) across 11 `TEST_CASE`s, 14 `TEST_CASE`s including the two error cases and the SIMPL-conversion case, all pass. Eleven of the twelve fixtures were carried through the legacy A/B; fixture I has no legacy counterpart.

*Mutation verification:* ten mutations were applied one at a time to a pristine snapshot, each with a full rebuild and `ctest` run; the transcript records the failing tests and the first failing fixture index for each. Nine were killed, and the four the brief called for are each killed by their named fixture and by nothing else:

| Mutation | Killed by |
|---|---|
| M1 3D classify comparison flip (`<=` → `<` on min-X) | A, B1, B2/B3, C, D, H |
| M2 3D no-shrink (drop the nearest-face pull) | A, B2/B3, C, D |
| M3 3D phase-gate drop | **D only** (index 5 — the phase-0 surface feature) |
| M4 3D classify loop starts at feature 0 | B2/B3, C (index 0) |
| M5 3D shrink loop starts at feature 0 | **A only** (index 5 — the sentinel mirroring feature 0's centroid) |
| M6 revert Fix 1 (2D box from `spacing[0]`/`spacing[1]`) | **F, G only** |
| M7 revert Fix 2 (2D classify raw X/Y) | **F, G only** |
| M8 2D no-shrink | E, E0, F, G |
| M9 2D origin forced to 0 (emulating 6.5.171) | **E only** (index 4) |
| M10 revert Fix 3 (drop the output fill value) | **survived** — see Fix 3; the platform's poison byte is even and the free path zeroes, so the indeterminate read cannot flip a `bool` here |

The pristine sources were restored afterwards and the clean rebuild returned 13/13 at the time of the mutation sweep; the gate is 14 as of the final commit (the -7461 preflight case was added after the sweep and cannot mask an algorithm-body kill).

*Second-engineer review:* delegated to the PR reviewer (requester decision, 2026-08-19).

## Code path coverage

**26 of 28 paths exercised.**

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeBiasedFeatures.cpp` (362 lines), plus the preflight in `Filters/ComputeBiasedFeaturesFilter.cpp`.

The filter has five logical phases: (a) preflight, (b) dimensional dispatch, (c) phase-loop setup, (d) the greedy box shrink, (e) the inclusive classification. The 3D and 2D routines each implement (d) and (e) separately, which is why they appear as parallel rows.

| #  | Phase              | Path                                                                                  | Test case |
|----|--------------------|---------------------------------------------------------------------------------------|-----------|
| 1  | (a) Preflight      | tuple counts disagree across Centroids/SurfaceFeatures/Phases → error `-7460`          | `Invalid filter execution` / "Mismatched tuple counts between Centroids and Surface Features" |
| 2  | (a) Preflight      | selected Feature arrays carry zero tuples → error `-7461` (guards the empty-range `std::max_element` dereference) | `Zero-tuple Feature arrays are rejected in preflight` (both `SECTION`s) |
| 3  | (a) Preflight      | Surface Features array of a type outside {bool, uint8} → parameter validation rejects   | `Invalid filter execution` / "Surface Features array of a disallowed type" |
| 4  | (a) Preflight      | valid inputs → `CreateArrayAction` creates the boolean output, fill value `"false"`     | every valid fixture; the fill value itself by `Fixture I` |
| 5  | (b) Dispatch       | all three dimensions `> 1` → 3D routine                                                | `Fixture A`, `B1`, `B2/B3`, `C`, `D`, `H` |
| 6  | (b) Dispatch       | some dimension `== 1` → 2D routine                                                     | `Fixture E`, `E0`, `F`, `G` |
| 7  | (b) Dispatch       | a dimension of 0 → neither branch; success with no writes                               | `Fixture I` |
| 8  | (c) Phase setup    | Apply Phase by Phase off → `numPhases = 1`, no phase gating                             | `Fixture A`, `B1`, `B2/B3`, `C`, `H` |
| 9  | (c) Phase setup    | Apply Phase by Phase on → `numPhases` from `std::max_element` over the whole array      | `Fixture D` (`phases[0] = 5`, real phases 1–2) |
| 10 | (c) Phase setup    | Apply Phase by Phase on, but a 2D geometry → phases never read at all                   | `Fixture E0` (toggle on, phases 1/2/3 scattered, output identical to the phase-free derivation) |
| 11 | (d) 3D shrink      | surface feature, phase matches, strictly inside → nearest **min** face pulled           | `Fixture A` (i=1, xMin), `Fixture C` (i=2, xMin) |
| 12 | (d) 3D shrink      | nearest **max** face pulled                                                             | `Fixture A` (i=3, xMax), `Fixture C` (i=1, yMax) |
| 13 | (d) 3D shrink      | centroid on/outside a face → `move = 0`, box unchanged                                  | `Fixture B1` |
| 14 | (d) 3D shrink      | non-surface feature skipped                                                             | `Fixture A` (i=2,4,5), `Fixture H` (all) |
| 15 | (d) 3D shrink      | phase mismatch skipped                                                                  | `Fixture D` (i=3 is phase 2, does not shrink phase 1's box) |
| 16 | (d) 3D shrink      | an earlier pull changes which face is nearest for a later feature (order dependence)    | `Fixtures B2/B3` — same two features, both orders, different output |
| 17 | (e) 3D classify    | inclusive **min**-face hit on X / Y / Z                                                 | `Fixture A` (1, X), `Fixture H` (1 X, 3 Y, 5 Z) |
| 18 | (e) 3D classify    | inclusive **max**-face hit on X / Y / Z                                                 | `Fixture A` (3, X), `Fixture H` (2 X, 4 Y, 6 Z) |
| 19 | (e) 3D classify    | strictly interior → false                                                               | `Fixture A` (2, 4, 5), `Fixture H` (7) |
| 20 | (e) 3D classify    | phase mismatch → feature never classified, stays false                                  | `Fixture D` (i=5, phase 0, is a surface feature yet stays unbiased) |
| 21 | (f) 2D axis remap  | X flat → in-plane (Y, Z), spacing (`[1]`, `[2]`), origin (`[1]`, `[2]`)                 | `Fixture F` |
| 22 | (f) 2D axis remap  | Y flat → in-plane (X, Z), spacing (`[0]`, `[2]`), origin (`[0]`, `[2]`)                 | `Fixture G` |
| 23 | (f) 2D axis remap  | Z flat → in-plane (X, Y), spacing (`[0]`, `[1]`), origin (`[0]`, `[1]`)                 | `Fixture E`, `Fixture E0` |
| 24 | (g) 2D shrink      | nearest min or max face pulled                                                          | `Fixture E` (all three of min-X, max-X, min-Y), `F`, `G` |
| 25 | (g) 2D shrink      | centroid on/outside a face → `move = 0`, box unchanged                                  | `Fixture E0` (i=5, sits outside the already-shrunk xMin face) |
| 26 | (h) 2D classify    | inclusive hit on either in-plane axis, min or max; and strictly interior → false        | `Fixture E`, `E0`, `F` (index 2/6 second-axis max), `G` (index 2/5 second-axis max), interior in all four |
| 27 | (i) Guards         | `InstantiateMaskCompare` throws `std::out_of_range` → error `-54900` (both routines)     | *Not directly tested. Unreachable through the `IFilter` API: the Surface Features parameter both validates existence and restricts the type to bool/uint8, so the catch is defensive cover for direct algorithm callers.* |
| 28 | (j) Cancel         | `m_ShouldCancel` early return — 4 sites (3D shrink, 3D classify, 3D post-phase, 2D)     | *Not directly tested. Requires cancel-signal injection; each site is a bare early `return {}` that cannot alter the result of a run that completes.* |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `Fixture A - 3D baseline` | new-for-V&V | Class 1. 6 features, Apply Phase by Phase off. Two `SECTION`s sweep the bool and uint8 Surface Features types (delta 2) and must agree. Feature 0 carries hostile data (flagged as a surface feature, centroid strictly inside) and feature 5 mirrors its centroid, so the pair is the exclusive killer for a shrink loop that starts at index 0 (mutation M5). |
| `Fixture B1 - 3D no-shrink surface feature` | new-for-V&V | Class 1. A surface feature sitting exactly on the xMin face: `move = 0`, box unchanged, yet the feature is still biased by the inclusive comparison. |
| `Fixtures B2/B3 - 3D greedy order dependence` | new-for-V&V | Class 1. Two `SECTION`s run the same two surface features in both index orders and assert *different* outputs (final box `{2, 4, 0, 4, 0, 4}` vs `{0, 4, 2, 4, 0, 4}`); a non-surface probe flips between them. This is the executed evidence for the order-dependence caveat now in the user documentation. |
| `Fixture C - 3D origin and anisotropic spacing` | new-for-V&V | Class 1. dims (2, 3, 4), origin (10, 20, 30), spacing (2, 0.5, 4) → box `[10,14] × [20,21.5] × [30,46]`. Exercises a max-face pull, a min-face pull, and a non-surface feature biased by a *shrunk* face. Feature 0's centroid at (0,0,0) makes it the killer for a classify loop that starts at index 0 (M4). |
| `Fixture D - 3D apply phase by phase` | new-for-V&V | Class 1. Two real phases plus a phase-0 surface feature and `phases[0] = 5`. Proves the per-phase box reset, proves the `max_element` quirk (D3) is output-benign, and is the exclusive killer for dropping the phase gate (M3). |
| `Fixture E - 2D Z-normal with origin` | new-for-V&V | Class 1. dims (4, 5, 1), origin (4, 8, 0), spacing (2, 0.5, 8). The exclusive discriminator for the 2D origin handling — mutation M9 (origin forced to 0, i.e. legacy behaviour) is killed by this fixture and no other. Also the A/B fixture that exhibits `ComputeBiasedFeaturesFilter-D2`. |
| `Fixture E0 - 2D Z-normal at origin ignores phases` | new-for-V&V | Class 1. Same geometry at the origin with Apply Phase by Phase **on** and phases deliberately scattered, asserting the documented "no effect on 2D" behaviour. Also covers the 2D `move = 0` path (feature 5). This is the clean 2D A/B match case — Z-normal, zero origin, so all three 2D defects are inert. |
| `Fixture F - 2D X-normal anisotropic` | new-for-V&V | Class 1. dims (1, 5, 6), spacing (1, 2, 3). RED evidence for Fix 1 and Fix 2: failed at index 4 pre-fix. Two probes sit on the shrunk yMax and zMax faces so the second in-plane axis is actually asserted. |
| `Fixture G - 2D Y-normal anisotropic` | new-for-V&V | Class 1 + Class 4. dims (5, 1, 6), spacing (2, 1, 3). RED at index 1 pre-fix, where pre-fix SIMPLNX (carrying both Fix 1 and Fix 2's defects) returned *no* biased features at all and so violated invariant I2 for three genuine surface features. Stock DREAM3D 6.5.171, which has only Fix 2's defect, leaves **one** genuine surface feature (index 2) unbiased — see Fix 2. |
| `Fixture H - box floor with no surface features` | new-for-V&V | Class 4 (invariant I3). No surface features, so the box must be exactly the geometry bounds; six probes sit one on each face plus one at the centre, pinning all six faces at once. |
| `Fixture I - degenerate geometry yields an initialized false output` | new-for-V&V | Regression lock for Fix 3. dims (0, 4, 4) satisfies neither dispatch test, so neither `fill(false)` runs and the output must have been initialized by the preflight action. Invariant I2 is explicitly waived here (documented in the fixture struct) because no box is ever built. |
| `Invalid filter execution` | modified | Was archive-based; now self-contained. Two `SECTION`s: mismatched tuple counts across two AttributeMatrices (error `-7460`) and a float32 array offered as Surface Features (parameter-level rejection). Replaces the retired archive test's single negative case with two. |
| `Zero-tuple Feature arrays are rejected in preflight` | new-for-V&V | Regression lock and executed RED for Fix 4. A Feature `AttributeMatrix` with zero tuples satisfies `validateNumberOfTuples()` (all three arrays agree at 0) and pre-guard reached `*std::max_element` over an empty range. Two `SECTION`s cover *Apply Phase by Phase* on (where the dereference is) and off (where zero-tuple Feature data is still malformed); both assert preflight `-7461` and an invalid execute. |
| `SIMPL Backwards Compatibility` | kept | **Untouched.** Two `DYNAMIC_SECTION`s over the 6.4 (`Filter_Name`) and 6.5 (UUID) conversion fixtures, checking all six converted arguments. |
| ~~`Valid filter execution`~~ | retired | Removed with the exemplar archive. It compared SIMPLNX against a legacy-generated `BiasedFeatures` array — a circular oracle, not an independent expected value — and its 2D container was a Z-normal slab at the origin, the one 2D configuration in which all three 2D defects are inert, so it could not have caught `ComputeBiasedFeaturesFilter-D1`. Its coverage (3D + 2D, Apply Phase by Phase on) is subsumed by fixtures A–I with hand-derived expectations. |

## Exemplar archive

- **Archive:** `6_6_find_biased_features.tar.gz` — **retired this pass.**
- **SHA512:** *(was `5a9df4c5a660768b19973b4ee3c6a59e1a997ea63823ef5931327a4857cc179d4f5dbf346130543d7ec13bee020c2caca483f1d4a730ee10a92b247cf3b0fe86`)*
- **Provenance:** n/a — no archive is consumed any more.

The `download_test_data()` block was removed from `src/Plugins/SimplnxCore/test/CMakeLists.txt`. A repo-wide grep before removal confirmed `ComputeBiasedFeaturesTest.cpp` was the archive's sole consumer, and the same grep after removal returns no remaining references outside historical review diffs. Retirement rationale is in the Test inventory row above: the archive supplied a legacy-derived "exemplar" that made the test circular, and its 2D fixture was in the one configuration blind to the filter's actual 2D defects. All twelve replacement fixtures are constructed in-test, so the filter now has no external data dependency and the whole suite runs in about one second.

## Deviations from DREAM3D 6.5.171

Comparison run on eleven of the twelve V&V fixtures (A, B1, B2, B3, C, D, E, E0, F, G, H — fixture I is excluded because the legacy `DataContainerReader` cannot round-trip a zero-dimension `ImageGeom`), re-emitted as legacy-format `.dream3d` inputs and driven through both DREAM3D 6.5.171 `PipelineRunner` and `nxrunner`. **8 of 11 byte-identical; 3 diverge.** All three divergences were predicted from the legacy source read *before* the run, and SIMPLNX matched the independently hand-derived oracle on 11 of 11 — there were no unpredicted differences to adjudicate.

| Fixture | Oracle / SIMPLNX | DREAM3D 6.5.171 | Result |
|---|---|---|---|
| A, B1, B2, B3, C, D, H (3D) | as derived | identical | match |
| E0 (2D Z-normal, origin 0) | as derived | identical | match |
| E (2D Z-normal, origin (4, 8, 0)) | `[0,1,1,1,0]` | `[0,1,1,1,1]` | differs at index 4 — D2 |
| F (2D X-normal) | `[0,1,1,1,0,1,1]` | `[0,1,1,1,1,1,1]` | differs at index 4 — D1 |
| G (2D Y-normal) | `[0,1,1,1,0,1]` | `[0,1,0,1,0,0]` | differs at indices 2 and 5 — D1 |

- `ComputeBiasedFeaturesFilter-D1` — 2D classification compares the raw X/Y centroid components against an axis-shifted box, so non-Z-normal slabs are classified on the wrong axes. **Shared** defect; fixed in SIMPLNX this pass. See `vv/deviations/ComputeBiasedFeaturesFilter.md`.
- `ComputeBiasedFeaturesFilter-D2` — 6.5.171 anchors the 2D box at (0, 0) instead of the geometry origin. **Legacy-only**; SIMPLNX was already correct. See `vv/deviations/ComputeBiasedFeaturesFilter.md`.
- `ComputeBiasedFeaturesFilter-D3` — phase-count scan includes the unassigned slot at index 0. **No output difference** (proven on fixture D); runtime-only. See `vv/deviations/ComputeBiasedFeaturesFilter.md`.

**Alignment validation.** After fixing D1 in SIMPLNX, the same two corrections (D1's classification remap and D2's origin remap) were applied as the smallest possible diff to a **surgically patched local build of the legacy source**, built and driven over the same eleven fixtures. The patched legacy build reproduces the fixed SIMPLNX output *and* the hand-derived oracle on **all eleven** fixtures, where the stock legacy binary differed on three. This is the evidence that the SIMPLNX fixes bring the two codebases into output alignment rather than merely changing SIMPLNX's answer. The patched build is internal proof tooling and is not a shipping comparison target.

## Outstanding

1. **Reviewer sign-off.** Second-engineer review of the oracle design is delegated to the PR reviewer (requester decision, 2026-08-19).
2. **Out-of-core build runs waived** (requester decision, 2026-08-19). All 14 tests pass in the in-core build; the OOC configuration was not exercised this pass. The filter reads through `AbstractDataStore` accessors and adds no raw-pointer access, so no OOC-specific risk was introduced, but this is reasoned rather than measured.
3. **Cancel paths uncovered** (path 28). The four `m_ShouldCancel` early-returns are untested; covering them needs cancel-signal injection in the test harness.
4. **`ComputeBiasedFeaturesFilter-D3` recommendation not applied.** Starting the `max_element` scan at index 1 would match 6.5.171 and stop a garbage value in the unassigned slot from driving the phase-loop bound. Left unchanged this pass because it is not an output difference.
5. **Mutation M10 survives.** Fix 3's defect (uninitialized output on the degenerate-dispatch path) is source-derived; no deterministic failing test could be produced for it, so fixture I locks the fixed behaviour without ever having demonstrated the broken one. The reason is now measured rather than assumed: macOS poisons with `0xAA`, whose low bit is clear, and clang masks `bool` loads with `1`, so the scribble environment variables cannot flip a `bool` on this platform, and the free path zeroes as well. Full experiment record in Fix 3. Demonstrating the broken behaviour needs an allocator that poisons with an odd byte — ASan or a debug `operator new`.
6. **A silent no-op on degenerate geometry.** A geometry with a zero *dimension* now yields a well-defined all-false output, but the filter still reports success without warning the user that nothing was computed. Fix 4's new guard does **not** cover this: it rejects zero-*tuple* Feature data, which is a different malformed input. A preflight guard or warning on the zero-dimension geometry itself would be a better user experience; it was left out of scope this pass because, unlike the zero-tuple case, it is no longer undefined behaviour.
