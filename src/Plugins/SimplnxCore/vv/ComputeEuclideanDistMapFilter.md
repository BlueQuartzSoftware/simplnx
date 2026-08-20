# V&V Report: ComputeEuclideanDistMapFilter

|                            |                                                                                     |
|----------------------------|-------------------------------------------------------------------------------------|
| Plugin                     | SimplnxCore                                                                         |
| SIMPLNX UUID               | `ba9ae8f6-443e-41d3-bb45-a08a139325c1`                                              |
| SIMPLNX Human Name         | Compute Euclidean Distance Map                                                      |
| DREAM3D 6.5.171 equivalent | `FindEuclideanDistMap` — SIMPL UUID `933e4b2d-dd61-51c3-98be-00548ba783a3`          |
| Verified commit            | *<filled at SBIR deliverable assembly>*                                             |
| Status                     | READY FOR REVIEW                                                                    |
| Sign-off                   | Authored by Michael Jackson <mike.jackson@bluequartz.net>, 2026-08-20. Second-engineer sign-off **delegated to the PR reviewer** (requester decision, 2026-08-19). |

## At a glance

| Aspect                 | Current state |
|------------------------|----------------|
| Algorithm Relationship | **Port.** `SimplnxCoreLegacyUUIDMapping.hpp:178` maps the legacy SIMPL UUID straight onto this filter. Legacy `FindEuclideanDistMap.cpp` was diffed line-by-line against `Algorithms/ComputeEuclideanDistMap.cpp` this pass: the seed classification, the propagation loop, the tie-break, the coordinate decode and the write-back are structurally identical. **One structural difference exists, and it is the root of both output deviations** — legacy's nearest-neighbour scratch array is zero-initialised, SIMPLNX's is `-1`-initialised. Four SIMPLNX-side additions (two preflight guards, cancel checks in both the propagation sweep and the seed scan, honouring the `-1` sentinel consistently across modes) — see *Port-time deltas*. |
| Oracle (confirmed)     | **Class 1 (Analytical)** primary, **Class 4 (Invariant)** companion, **Class 2 (Reference implementation)** at production scale. The propagation is a hand-derivable layer-synchronous BFS and the float conversion is a closed-form distance, so every expected value is derivable without reference to any implementation. Confirmed — 7 fixtures × 4 toggle sets × 2 modes = 56 `DYNAMIC_SECTION`s in `test/ComputeEuclideanDistMapTest.cpp`, all pass, on the **unmodified** algorithm. Six Class 4 invariants ride along, including the ED-1 inequality `euclidean >= trueEDT` and toggle independence. |
| Code paths enumerated  | **21 of 24 exercised.** Gaps: the two per-Z-slab cancel early returns added this pass (one in the propagation sweep, one in the seed scan; both need cancel-signal injection) and the `-12801` non-int32 preflight branch (unreachable through the parameter system, which already type-checks). |
| Tests today            | **6 TEST_CASEs / 6 ctest entries, 6366 assertions, all pass.** 1 Class 1 analytical case carrying 56 sections (2221) + 1 Class 4 invariant case carrying 7 fixtures (2780) + 1 parallel-determinism case (1129) + 1 three-section error-path case (24) + 1 retained production-scale archive case (181) + 1 SIMPL backwards-compatibility case (31). Full `SimplnxCore::` suite green at 983/983 and `PIPELINE::` green at 33/33 after the new preflight guard. |
| Exemplar archive       | `6_6_stats_test_v2.tar.gz`, retained and **untouched** (many other consumers). Its circularity was broken this pass: all three archived Manhattan arrays *and* the archived internal `NearestNeighbors` array were re-derived from `FeatureIds` alone by an independent vectorised oracle and match exactly on **748,800 cells**. New provenance sidecar records that, plus four measured coverage caveats. |
| Legacy comparison      | **Run.** 56 fixture × toggle × mode combinations through the 6.5.171 `PipelineRunner`, plus three probe tags. **All 87 array comparisons (84 from the 56 tags — a single-tier toggle set emits one array, the all-three set emits three — plus 3 from the `SaveNearestNeighbors` probe tag): SIMPLNX matches the oracle. All 87: 6.5.171 matches the prediction made from the legacy source before the run — zero unpredicted differences.** Legacy differs from SIMPLNX in exactly two places, both traced to its zero-initialised nearest-neighbour array. |
| Bug flags              | **No SIMPLNX output bug found — the algorithm is correct as shipped and was not changed.** Three SIMPLNX-side defects fixed, none affecting computed values: a missing FeatureIds/geometry preflight cross-check (out-of-bounds read, new error `-12803`), an unreadable `-12802` message (its `fmt::format` argument had no `{}` to land in), and a stored-but-never-read `m_ShouldCancel`. `-D1` and `-D2` are legacy-only bugs where SIMPLNX is already correct; `-D3` is a guard SIMPLNX has and legacy lacks; `-D4` is a shared semantics note plus a user-doc rewrite. |
| V&V phase              | Oracle designed, frozen and RED-run before any legacy run; a previously unidentified legacy bug (D2) found by source reading and confirmed by binary run; two preflight defects and one cancel gap closed RED-first; nine-mutation verification sweep passed with zero survivors; production-scale archive de-circularised; user documentation rewritten. Outstanding before promotion to COMPLETE: PR-reviewer sign-off (see header), the uncovered cancel path, the source-derived-only out-of-core concurrent-read concern, and **out-of-core build runs, waived by requester decision 2026-08-19**. |

## Summary

`ComputeEuclideanDistMapFilter` measures, for every Cell of an Image Geometry, how far it is from the nearest Feature boundary, triple line and/or quadruple point, emitting either an int32 6-connected graph distance or a float32 straight-line distance. Verification is **Class 1 (Analytical)**: the propagation is a layer-synchronous breadth-first search whose distances are hand-countable and whose float conversion is a closed form, so every expected value was derived independently — first by hand on the legacy 10×6×1 fixture, then by a Python reimplementation of the seed rule, propagation, tie-break and coordinate decode written and frozen before either binary was invoked. **No output bug was found: the algorithm matched the oracle on all 56 fixture × toggle × mode combinations without modification.** The headline finding is a *documentation* one — the float32 output is not a Euclidean distance transform but the distance to a tie-break-selected seed, which the user doc previously described as "the distance of each Cell from the nearest Feature boundary" — and the headline legacy finding is a previously unidentified 6.5.171 bug (`-D2`) that corrupts the triple-line and quadruple-point maps whenever a lower-tier toggle is left off. Three SIMPLNX-side defects that do not affect computed values were closed: a missing preflight cross-check that permitted an out-of-bounds read, an error message that silently dropped its argument, and a cancel flag that was stored and never read.

## Algorithm Relationship

*Classification:* **Port** ~~| Minor changes | Rewrite | New filter~~

*Evidence:* `SimplnxCoreLegacyUUIDMapping.hpp:178` maps legacy SIMPL UUID `933e4b2d-dd61-51c3-98be-00548ba783a3` directly to `FilterTraits<ComputeEuclideanDistMapFilter>`, and `test/simpl_conversion/{6_4,6_5}/ComputeEuclideanDistMapFilter.json` carry the legacy `CalcManhattanDist` / `DoBoundaries` / `DoTripleLines` / `DoQuadPoints` / `FeatureIdsArrayPath` / `{GB,TJ,QP}DistancesArrayName` parameter set unchanged. Legacy source (`Source/Plugins/Statistics/StatisticsFilters/FindEuclideanDistMap.cpp`, from a sibling `DREAM3D` checkout on the authoring engineer's machine, not committed to this repository) was diffed line-by-line against `Algorithms/ComputeEuclideanDistMap.cpp` this pass rather than inferred from documentation. The propagation loop is line-for-line identical: legacy `:152-223` against SIMPLNX `:122-215` (SIMPLNX's range is the wider of the two only because of the cancel-check block added this pass), including the unconditional assignment inside the `j = 0..5` neighbour loop that produces the tie-break, and the second full pass that commits distances and makes the sweep layer-synchronous. The euclidean pass (legacy `:227-257` / SIMPLNX `:218-247`) is identical down to the reciprocal-multiplication coordinate decode.

*Port-time deltas:*

1. **Nearest-neighbour scratch storage — the only structural difference, and the source of both output deviations.** Legacy allocates a real 3-component `DataArray<int32_t>` in `dataCheck()` with initial value **0** (`FindEuclideanDistMap.cpp:445-446`) and optionally writes it out (`SaveNearestNeighbors`). SIMPLNX uses a local `std::vector<int64>` initialised to **-1** (`:314`) and has no equivalent output. Both versions admit a cell as a seed when its component is `>= 0`, so any component the seed loop does not explicitly write is read as a seed under legacy and as a non-seed under SIMPLNX. That is `-D1` (bad-data cells) and `-D2` (spurious lower-tier seeds). **SIMPLNX is correct in both.** Dropping the output array is a deliberate NX-side simplification; the array was internal bookkeeping that legacy exposed.
2. **Face-neighbour offsets and boundary validity in the seed loop** — legacy computed `neighbors[]` and six inline `good = false` conditionals; SIMPLNX calls `initializeFaceNeighborOffsets(dims)` and `computeValidFaceNeighbors(x, y, z, dims)` from `NeighborUtilities`. The `Image3D` specialisation returns the same six offsets in the same `[-Z,-Y,-X,+X,+Y,+Z]` order and the same six conditions; verified by reading `NeighborUtilities.hpp:244-269` and `:280-313`. No output change. (Note the *propagation* loop still uses its own inline `neighbors[]`/`mask[]`, untouched by that refactor — so the two halves of the algorithm compute neighbour validity by different code that happens to agree.)
3. **Parallel dispatch** — legacy used a raw `tbb::task_group` under `#ifdef SIMPL_USE_PARALLEL_ALGORITHMS` with a serial fallback; SIMPLNX uses `ParallelTaskAlgorithm`. Same three tasks, same shared scratch vector, each task touching only its own component. No output change; confirmed by the Parallel Determinism TEST_CASE.
4. **Cancel check** — SIMPLNX reads the cancel flag once per Z-slab in the propagation sweep *and* once per Z-slab in the seed scan; legacy has no cancel check at all. **Both added this pass** — the flag was previously stored and never read. Additive; no output change on a run to completion.
5. **Preflight validation** — SIMPLNX rejects all-three-toggles-off with `-12802` (legacy silently no-ops; deviation `-D3`) and, **as of this pass**, rejects a FeatureIds tuple count that disagrees with the selected geometry's cell count with `-12803`. Legacy has neither guard, but its `AttributeMatrix` model made the second situation harder to construct.
6. **Progress reporting** — neither version emits progress messages from this algorithm. Not addressed this pass; see *Code path coverage*.

*Material PRs since baseline:* seven commits have touched `Algorithms/ComputeEuclideanDistMap.cpp`; three carry behavioural content and account for deltas 2–3:

- **#1523** — factored the 6-face-neighbour code out into `NeighborUtilities` (delta 2).
- **#1590** — standardized 2D image handling (`VoxelNeighbors<Image3D>` specialization).
- **#1506** — "Compute Euclidean Dist Map now does not create extra arrays": dropped the `NearestNeighbors` output array (delta 1's NX side).

Earlier commits (#1017 store-API migration, #956 Find→Compute rename, #801 complex→simplnx rename, #390 the original port) are mechanical.

## Findings and fixes (this pass)

### No output bug. The algorithm was not changed.

This is the load-bearing negative result. The Class 1 TEST_CASE — 56 fixture × toggle-set × mode sections, 2221 assertions — **passed on the unmodified algorithm at the first run** (`ww_work/ComputeEuclideanDistMap/logs/red_class1_green.log`). The expected values were frozen in `oracle.py` and emitted into the test's constant table by `gen_expected.py` before the filter was built, so this is a genuine oracle-first pass, not a fitted one. All three fixes below are in preflight or in cancel handling; none touches a computed value.

### Fix 1 — missing FeatureIds/geometry cross-check (out-of-bounds read), new error `-12803`

`findDistanceMap` sizes the `nearestNeighbors` scratch vector from the **FeatureIds tuple count** (`Algorithms/ComputeEuclideanDistMap.cpp:290, 314`) while `ComputeDistanceMapImpl::operator()` iterates the **selected geometry's cell count** (`:60`). Nothing forced the two to agree: `GeometrySelectionParameter` and `ArraySelectionParameter` each validate their own object's existence but not their mutual consistency, and a SIMPLNX `DataStructure` permits an `AttributeMatrix` whose tuple shape differs from its parent geometry's dimensions. Selecting a FeatureIds array with fewer tuples than the geometry has cells therefore read and wrote past the end of the vector, and past the end of the output data stores.

Fixed at `ComputeEuclideanDistMapFilter.cpp:140-152` — preflight now rejects the mismatch with `-12803`, naming both actual counts and the geometry's dimensions so the user can fix it from the message alone. RED-first: the `-12803 FeatureIds tuple count does not match the geometry cell count` section was written and observed failing (`REQUIRE(preflightResult.outputActions.invalid())` → `false`) before the guard existed.

### Fix 2 — the `-12802` message silently dropped its argument

`fmt::format` was called with `pFeatureIdsArrayPathValue.toString()` as an argument but **no `{}` placeholder to receive it**, so the path never reached the user. `fmt` permits unused arguments, so this compiled and shipped silently. Rewritten at `:133-138` to name both the offending path and the three options. RED-first: the assertion `message.find(k_TestFeatureIdsPath.toString()) != npos` was observed failing against the shipped message.

### Fix 3 — `m_ShouldCancel` stored, never read

The algorithm held a `const std::atomic_bool&` and exposed `getCancel()`, but nothing ever read it: a run on a large volume could not be interrupted. The propagation dominates — its pass count grows with the largest distance in the volume (27 on the 748,800-cell archive dataset), and each pass is a full raster sweep — but it is not the only long phase: the seed scan visits every cell once and probes six face neighbours per cell, so on a production volume it costs about as much as one propagation pass. **Two** checks were therefore added, both at the Z-slab level and both matching the `ErodeDilateBadData.cpp:144-148` idiom: one inside the propagation sweep (`:130-139`) and one in `findDistanceMap`'s seed scan (`:344-353`, where the per-cell cost is a single integer compare against a running slab boundary rather than a modulo). A cancel in the seed scan returns before any propagation runs, so the outputs keep the `-1` fill plus whatever `0` seed markers were written. **A fired cancel leaves the output arrays partially written — the `-1` fill plus whatever distances earlier passes committed — and the filter still reports success**, which is the standing behaviour across SimplnxCore rather than something specific to this filter.

### Fix 4 — the user documentation described the wrong quantity

`docs/ComputeEuclideanDistMapFilter.md` opened with "calculates the distance of each **Cell** from the nearest **Feature** boundary" and never mentioned the tie-break, so a reader would reasonably take the float32 output for a Euclidean distance transform. It is not (see `-D4`). Rewritten this pass to state the seed rule with its thresholds and the `>= 0` id-0 admission, the layer-by-layer growth, the explicit tie-break order with the worked `4.0` vs `2.0` example and an explicit "do not use this output as a nearest-boundary distance" warning, the two distinct meanings of the `-1` fill, and toggle independence. The prior text's threshold wording ("at least 2 / 3 / 4 different neighbors") was checked against `:410`/`:421`/`:432` and is *correct* when read as counting distinct ids including the cell's own; it was reworded for unambiguity, not because it was wrong.

## Oracle

*Class:* **1 (Analytical)** primary, **4 (Invariant)** companion, **2 (Reference implementation)** at production scale.

### Why not a library EDT

The float32 output is **not** a Euclidean distance transform, so `scipy.ndimage.distance_transform_edt` is not an oracle for it — it disagrees by design. Finding `-D4`: the propagation records, for each cell, *the one seed cell it happened to be handed*, chosen by the tie-break `+Z > +Y > +X > -X > -Y > -Z`, and the float conversion measures to that seed. The oracle therefore reimplements the propagation. A library EDT is still computed, as the Class 4 *floor* (`euclidean >= trueEDT` pointwise), never as the expected value.

*Applied:* Three independent derivations, agreeing with each other before any SIMPLNX run.

- **By hand.** Fixture A (the legacy 10×6×1 fixture, spacing `(1,2,1)`) was derived cell by cell: the coordination count for all 60 cells, the four GB propagation passes, the four TJ passes, the tie-break pick for every non-seed, and the reciprocal decode of every recorded seed index. Recorded in full in the working folder.
- **By independent reimplementation.** `ww_work/ComputeEuclideanDistMap/oracle.py` encodes four rules — seed classification (R1), toggle independence (R2), propagation and tie-break (R3), coordinate decode (R4) — each with the cited line range of the NX source it was derived from, and each written before the filter was built against these fixtures. Its output reproduces the hand derivation exactly on all 60 cells of fixture A and supplies the expected values for the other six fixtures. The test's expected-value table is *generated* from it by `gen_expected.py`, so hand-transcription cannot introduce drift.
- **By legacy's own published expectations.** DREAM3D 6.5.171 ships its own unit test for this filter (`Source/Plugins/Statistics/Test/FindEuclideanDistMapTest.cpp:139-215`) with hard-coded `GBManhattan`, `TJManhattan`, `QPManhattan`, `GBEuclidean`, `TJEuclidean` and `QPEuclidean` arrays for the same 10×6×1 fixture. The hand derivation and `oracle.py` reproduce **all six** of those arrays exactly, except at the six bad-data cells of the three float arrays — which is deviation `-D1`, predicted from the legacy source before the comparison. Three independent routes to the same numbers is the strongest corroboration available for this filter.

*Encoded:*

- `SimplnxCore::ComputeEuclideanDistMapFilter: Class 1 - Manhattan and BFS-Seed Distance Maps` — 7 fixtures × 4 toggle sets × 2 modes as `DYNAMIC_SECTION`s, asserting every element of every enabled output and the absence of every disabled one. 2221 assertions. Rational expectations (including the `-1` fill and the `0.0` seed marker) are asserted **exactly**; the five irrational ones (√2, √5, √8, √17, √20) use a `1e-6` relative tolerance whose only cause is the float64 → float32 narrowing at `:254-262`.
- `SimplnxCore::ComputeEuclideanDistMapFilter: Class 4 - Invariants` — six invariants per fixture, 2780 assertions. Detailed below.
- `SimplnxCore::ComputeEuclideanDistMapFilter: Parallel Determinism` — fixture A, all three tiers, both modes, three repeats, bit-identical. 1129 assertions.
- `SimplnxCore::ComputeEuclideanDistMap` — the retained production-scale Class 2 case (see *Exemplar archive*). 181 assertions.

*The ED-1 pin.* Fixture A's `gbEuclidean[0] == 4.0` is asserted **exactly**, with a loud comment, even though the nearest boundary seed is `2.0` away. It is the single most discriminating assertion in the suite: reversing the tie-break direction (mutation M6) changes it to `2.0` and kills nothing else in Class 1. The Class 4 case additionally asserts that fixture A's GB map has exactly **three** cells where the tie-break handed out a farther seed than the nearest (cells 0, 11 and 17), so both an over- and an under-count of tie-break victims fails.

*Class 4 invariants.* Each is computed from filter output plus geometry alone — the `TrueEdt` helper brute-forces the nearest-seed distance from the *filter's own* zero-set, so I5 does not consult the oracle table.

| # | Invariant | Why it is derivable |
|---|---|---|
| I1 | `{QP == 0}` ⊆ `{TJ == 0}` ⊆ `{GB == 0}` | The coordination thresholds nest: 3 ≥ 2 ≥ 1 |
| I2 | `featureId <= 0` ⟹ every output element is `-1`; and where a tier has ≥1 seed the converse also holds on these fixtures, whose positive-id regions are face-connected; where a tier has no seed at all, every element is `-1` | The seed loop is guarded by `feature > 0` and the propagation only travels through positive-id cells |
| I3 | Face-adjacent cells' Manhattan distances differ by at most 1 | Defining property of a breadth-first graph distance. This is the invariant that fails if the distance commit moves inside the raster sweep (mutation M7) |
| I4 | `{manhattan == 0}` == `{euclidean == 0}` | Both mean "the cell is its own nearest neighbour" |
| I5 | `euclidean >= trueEDT` pointwise, strictly greater exactly where the tie-break picked a farther seed | `-D4` restated as a checkable inequality |
| I6 | A tier's output is identical whether that tier is the only one enabled or all three are | SIMPLNX's `-1`-initialised scratch vector. **6.5.171 violates this — that is `-D2`** |

*Second-engineer review:* **delegated to the PR reviewer** (requester decision, 2026-08-19). The reviewer's highest-value target is the oracle's R3, since every expected float value depends on the tie-break direction being read off `:185-200` correctly. Three independent confirmations are on record: the hand derivation, legacy's published `GBEuclidean[0] == 4.0f`, and the production-scale match of the archive's own internal `NearestNeighbors` array on 748,800 cells.

## Code path coverage

21 of 24 paths exercised. The algorithm has four logical phases: (a) preflight validation, (b) a per-cell seed classification scan, (c) a layer-synchronous propagation per enabled tier, and (d) the optional float conversion plus write-back.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeEuclideanDistMap.cpp` (504 lines) and `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ComputeEuclideanDistMapFilter.cpp` (238 lines).

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | (a) Preflight | FeatureIds resolves to `Int32Array`, output arrays created with the FeatureIds tuple shape and int32 or float32 per `CalcManhattanDist` | All tests; both output types covered by every Class 1 section pair |
| 2 | (a) Preflight | `!doBoundaries && !doTripleLines && !doQuadPoints` → error `-12802` | Error Paths `-12802 no output selected`; also asserts the message names the FeatureIds path (Fix 2) |
| 3 | (a) Preflight | `numFeatureIdsTuples != numGeometryCells` → error `-12803` | Error Paths `-12803 FeatureIds tuple count does not match the geometry cell count` |
| 4 | (a) Preflight | counts agree → valid | Error Paths `-12803 does not fire when the counts agree`, and every other test |
| 5 | (a) Preflight | `getDataAs<Int32Array>` returns null → error `-12801` | *Not directly tested. Unreachable through the parameter system: `ArraySelectionParameter` is declared with `AllowedTypes{DataType::int32}` and already rejects a non-int32 or absent selection before `preflightImpl` runs. Retained as defence in depth.* |
| 6 | (b) Seed scan | `featureId <= 0` → cell skipped entirely, all three components stay `-1` | Fixtures A (six bad-data cells) and F (all nine); Class 4 I2 |
| 7 | (b) Seed scan | boundary-invalid face neighbour skipped | Fixtures B (1D: four of six faces always invalid), Dz (3D interior and both z faces), and every fixture's edge cells |
| 8 | (b) Seed scan | neighbour id `!= feature` and `>= 0`, not already in `coordination` → pushed | All fixtures |
| 9 | (b) Seed scan | neighbour id `== 0` admitted as a distinct feature (the `>= 0` rule) | Fixture A column x=8; killed by mutation M4 |
| 10 | (b) Seed scan | neighbour id already in `coordination` → not pushed | Fixture Dz (all four in-plane neighbours share one id) and fixture A's interiors |
| 11 | (b) Seed scan | `coordination.empty()` → all three components `-1` | Fixtures A (interior cells), E (every cell); Class 4 I2 |
| 12 | (b) Seed scan | `size >= 1` and `DoBoundaries` → GB seed | Every fixture's GB sections; killed by M1 |
| 13 | (b) Seed scan | `size >= 2` and `DoTripleLines` → TJ seed | Fixtures A, C, D; killed by M2 |
| 14 | (b) Seed scan | `size > 2` and `DoQuadPoints` → QP seed | Fixture D (2D pinwheel, coordination size 4); killed by M3 |
| 15 | (c) Propagation | init: component `>= 0` → cell is its own nearest neighbour; else `-1` | All tests |
| 16 | (c) Propagation | raster sweep assigns from the last qualifying neighbour (the tie-break) | Fixture A `gbEuclidean[0] == 4.0`; Class 4 I5's strict count; killed by M6 |
| 17 | (c) Propagation | commit pass assigns `Distance` to newly reached cells | All fixtures with distance > 0; Class 4 I3; killed by M7 |
| 18 | (c) Propagation | loop terminates with `changed == 0` when a tier has no seed | Fixtures E and F, and the TJ/QP tiers of A, B and Dz |
| 19 | (c) Propagation | loop terminates with `count == 0` when every positive-id cell is reached | Fixtures A, B, C, D, Dz |
| 20 | (c) Propagation | `m_ShouldCancel` read once per Z-slab → early return, partial output, success | *Not directly tested. Requires cancel-signal injection; no test sets `m_ShouldCancel` and asserts early termination. Added this pass; legacy has no cancel check at all, so SIMPLNX is ahead of legacy here — not a deviation.* |
| 24 | (b) Seed scan | `shouldCancel` read once per Z-slab boundary → early return before any propagation, `-1` fill plus partial `0` seed markers, success | *Not directly tested — same cancel-signal-injection gap as path 20. Added this pass.* |
| 21 | (d) Float pass | `constexpr` branch skipped for `T == int32` | Every `manhattan int32` section (the maps stay integral) |
| 22 | (d) Float pass | `nearestNeighbor >= 0` → reciprocal decode + `sqrt`; `< 0` → element keeps the `-1` fill | Every `euclidean float32` section. The x, y and z halves of the decode are separately isolated: fixture B (x only), fixture A (x and y with `spacing[1] = 2`), fixture Dz (z with `spacing[2] = 4`) |
| 23 | (d) Write-back | per-tier `static_cast<T>` into the selected output store | All tests; killed by M5 (fill value) |

**Progress reporting.** Neither version emits progress messages from this algorithm, and none were added. The propagation is O(cells × max-distance) and on the 748,800-cell archive dataset runs 27 passes for the QP tier, so it is a legitimate candidate; deferred rather than bundled into a V&V pass whose premise is no behavioural change beyond the guards.

**Mutation verification.** Nine mutations, each with its killing fixture predicted before the run; **zero survivors**, and every kill set matched the prediction. Transcript at `ww_work/ComputeEuclideanDistMap/mutation_transcript.md`. Restore verified: the two mutated files are byte-identical (MD5) before and after the sweep and the gate is green at 6/6.

| Mutation | Kill set (measured) |
|---|---|
| M1 GB seed threshold `>= 1` → `>= 2` | Class 1 (20 sections, from `fixture A / GB only / manhattan`), Class 4 (fixture A), archive test |
| M2 TJ seed threshold `>= 2` → `>= 3` | Class 1 (20 sections, from `fixture A / TJ only / manhattan`), archive test |
| M3 QP seed threshold `> 2` → `> 1` | Class 1 (20 sections, from `fixture A / QP only / manhattan`), archive test |
| M4 neighbour admission `>= 0` → `> 0` | Class 1 (**exactly 6** fixture-A sections), Class 4 (fixture A) — archive test **blind** |
| M5 output fill `-1` → `0` | Class 1 (20 sections), Class 4 (A, B, C, Dz, F), archive test |
| M6 tie-break reversed | Class 1 (**exactly 2** sections, `fixture A / GB only / euclidean float32` and `fixture A / all three / euclidean float32`), Class 4 (fixture A) — archive test **blind** |
| M7 distance commit moved inside the sweep | Class 1 (18 sections), Class 4 (A, B, D), archive test |
| M8 `-12803` guard reverted | Error Paths, section `-12803 FeatureIds tuple count does not match the geometry cell count` |
| M9 `-12802` `{}` placeholder reverted | Error Paths, section `-12802 no output selected` |

M4 and M6 are the two narrow kills, and they are the reason the fixture set looks the way it does. M4 fires only on fixture A, the only fixture with a bad-data region adjacent to a positive-id feature. M6 fires on **two sections in the whole suite** — both fixture-A float sections — because the tie-break provably cannot change a Manhattan value, so `gbEuclidean[0] == 4.0` is the only assertion in the suite that sees it. That is why the ED-1 pin is asserted exactly and commented loudly rather than left to a tolerance.

**Blind-suite result for the retained archive test.** Measured, not argued. The production-scale archive test catches M5 and M7 but is **blind to M4 and M6**:

- Blind to **M4** because the archive's `FeatureIds` has *no cell with id ≤ 0* (min 1, max 619), so admitting or rejecting id 0 as a neighbour is a no-op there.
- Blind to **M6** because the archive stores Manhattan arrays only, and the tie-break provably cannot change a Manhattan value — every neighbour available in a given propagation pass carries the same pass number, so only the recorded seed *identity* differs and that identity is not written out.

Both gaps are closed by the inline fixtures. This is why the archive test is retained as a scale-and-realism regression guard rather than promoted to the primary oracle.

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::ComputeEuclideanDistMapFilter: Class 1 - Manhattan and BFS-Seed Distance Maps` | new-for-V&V | Primary Class 1 oracle. 7 fixtures × 4 toggle sets × 2 distance modes = 56 `DYNAMIC_SECTION`s, 2221 assertions. Asserts every element of every enabled output array and the *absence* of every disabled one. Expected values generated from `oracle.py` by `gen_expected.py`; the whole test file is assembled by `assemble_test.py` from four parts so the generated table cannot drift. Exact comparison for rational expectations, `1e-6` relative for the five irrational ones. |
| `SimplnxCore::ComputeEuclideanDistMapFilter: Class 4 - Invariants` | new-for-V&V | Six invariants (I1–I6) per fixture, 2780 assertions. I5 brute-forces the true EDT from the filter's own zero-set, so it is independent of the oracle table. I6 is toggle independence, which is the invariant DREAM3D 6.5.171 violates (`-D2`). Includes the explicit ED-1 pin: fixture A's GB map has exactly three tie-break victims and `euclidean[0] == 4.0` against a true EDT of `2.0`. |
| `SimplnxCore::ComputeEuclideanDistMapFilter: Parallel Determinism` | new-for-V&V | Fixture A, all three tiers, both modes, three repeats, 1129 assertions. The three tiers run as concurrent `ParallelTaskAlgorithm` tasks over one shared scratch vector; this asserts the output is bit-identical run to run. |
| `SimplnxCore::ComputeEuclideanDistMapFilter: Error Paths` | new-for-V&V | Three sections, 24 assertions. `-12802` (all toggles off) additionally asserts the message names the FeatureIds path, which is Fix 2's regression pin. `-12803` asserts that exactly one error is raised and that both actual counts (`has 8 tuples`, `has 9 cells (3x3x1)`) and both offending paths appear in the message as whole phrases, and **only preflights** — executing that configuration is the out-of-bounds access itself. A third section proves the guard does not fire when the counts agree. |
| `SimplnxCore::ComputeEuclideanDistMap` | kept | Retained production-scale Class 2 oracle, 181 assertions, 748,800 cells, 619 features, distances to 27. `GENERATE`s the three single-toggle configurations, checks that exactly the expected array exists, and compares it element-wise against the archive's sibling array. **Unmodified this pass.** Its circularity was resolved externally (see *Exemplar archive*) rather than by rewriting it. Blind to M4 and M6 — see the blind-suite result above. |
| `SimplnxCore::ComputeEuclideanDistMapFilter: SIMPL Backwards Compatibility` | kept | **Untouched this pass**, byte-identical to the pre-V&V version. `DYNAMIC_SECTION` over `simpl_conversion/6_5/` (matched by `Filter_Uuid`) and `simpl_conversion/6_4/` (matched by `Filter_Name`); loads each legacy pipeline via `Pipeline::FromSIMPLFile`, confirms one `PipelineFilter` with the right UUID, and checks all nine converted arguments; 31 assertions. Not an oracle test. |

All 6 tests pass (6366 assertions total) in the in-core build `NX-Com-Qt69-Vtk96-Rel`. **Out-of-core build runs are waived by requester decision 2026-08-19** and are recorded as outstanding below.

## Exemplar archive

- **Archive:** `6_6_stats_test_v2.tar.gz`
- **SHA512:** `e84999dec914d81efce4fc4237c49c9bf32e48381b1e79f58aa4df934f0d7606cd7a948f9a5e7b17a126a7944cc531b531cfdc70756ca3e2207b20734e089723`
- **Provenance:** [`provenance/6_6_stats_test_v2.md`](provenance/6_6_stats_test_v2.md) — new this pass
- **`download_test_data()` line in `test/CMakeLists.txt`:** **untouched.** The archive has many other SimplnxCore consumers.

The archive's `FindEuclideanDistMap` step (embedded pipeline step 35, `FilterVersion` **6.6.373**, which is *later* than the 6.5.171 V&V baseline) ran in Manhattan mode with all three toggles on and `SaveNearestNeighbors = 1`. So it is genuine legacy output but **not itself a 6.5.171 comparison** — the 6.5.171 comparison of record is the 56-combination `PipelineRunner` A/B run below. Both documented deviations are structurally out of reach of that configuration, which is why it agrees with SIMPLNX.

**The circularity was broken.** Comparing the filter's output against a sibling array produced by the same algorithm proves nothing on its own. All three archived Manhattan maps **and the archived internal `NearestNeighbors` array** were therefore re-derived from `FeatureIds` alone by a vectorised port of the independent oracle (`ww_work/ComputeEuclideanDistMap/archive_oracle.py`): **exact match on all 748,800 cells for all four arrays.** The `NearestNeighbors` match is the stronger result — it confirms the tie-break rule cell by cell at production scale, not just on hand fixtures. Four measured coverage caveats (no bad-data region, Manhattan only, all toggles on, isotropic spacing) are recorded in the sidecar with the mutation evidence for two of them.

## Deviations from DREAM3D 6.5.171

The comparison was run on all seven fixtures × four toggle sets × two distance modes (56 tags), plus three probe tags, through the 6.5.171 `PipelineRunner` and `nxrunner`. The 56 tags yield **84** array comparisons — a single-tier toggle set emits one output array and the all-three set emits three, so 7 × 2 × (1 + 1 + 1 + 3) = 84 — and the `A_all_flt_nn` probe tag adds 3 more (`A_none_man` and `A_probe_keys` produce no array to compare), for **87 array comparisons in total**, matching the 87 `A/B:` verdict rows in the results file. **SIMPLNX matched the oracle on 87 of 87. DREAM3D 6.5.171 matched the prediction derived from its own source before the run on 87 of 87 — zero unpredicted differences.** Full matrix in `ww_work/ComputeEuclideanDistMap/results_compare.txt`.

Note on ID namespaces: the comparison tooling labels its rows with the *pre-identified finding* IDs (`ED-2`, `ED-6`), while this report and the deviations file use *deviation* IDs. `ED-2` = `-D1` and `ED-6` = `-D2`; the mapping for all six `ED-` findings is recorded at the top of the deviations file.

Every difference reduces to one legacy defect: its `NearestNeighbors` `DataArray` is created with initial value `0` where SIMPLNX's scratch vector is initialised to `-1`, and the seed test in both versions is `>= 0`.

| Toggle set | Manhattan mode | Float mode |
|---|---|---|
| Boundaries only | match | `-D1` at bad-data cells only |
| All three | match | `-D1` at bad-data cells only |
| Triple lines only | **`-D2`** | **`-D2`** |
| Quadruple points only | **`-D2`** | **`-D2`** |

Four entries, all in [`deviations/ComputeEuclideanDistMapFilter.md`](deviations/ComputeEuclideanDistMapFilter.md):

- `ComputeEuclideanDistMapFilter-D1` — float mode writes `0.0` at bad-data cells in 6.5.171, `-1.0` in SIMPLNX. Legacy bug; **trust SIMPLNX** (`0.0` claims the cell is *on* a boundary, and `-1` is what legacy itself writes there in Manhattan mode).
- `ComputeEuclideanDistMapFilter-D2` — **previously unidentified.** With a lower-tier toggle off, 6.5.171's triple-line and quadruple-point maps are grown from a contaminated seed set, wrong in both modes and at most cells (28–60 of 60 on fixture A). Legacy bug; **trust SIMPLNX**, and regenerate any 6.5.171 result produced with *Calculate Distance to Boundaries* off.
- `ComputeEuclideanDistMapFilter-D3` — all-toggles-off: 6.5.171 silently no-ops, SIMPLNX errors `-12802`. **Trust SIMPLNX**; the guard is better.
- `ComputeEuclideanDistMapFilter-D4` — shared semantics note, **not a version difference**: the float32 output is the distance to a tie-break-selected seed, not a Euclidean distance transform. Both versions behave identically. Documented in the user doc this pass; *either acceptable*.

Five confirmed **non**-deviations are recorded in the same file so they are not relitigated: the reciprocal coordinate decode (numerically proven latent through flat index 2^31 — recommended cleanup, deliberately not changed), legacy's mismatched `readFilterParameters` override (proven dead code by a probe run), SIMPLNX's toggle independence (now asserted as I6), parallel task safety (with a latent out-of-core concurrent-read concern recorded), and the propagation loop's termination when a tier has no seeds.

**No 6.5.172 surgical patch was created for this filter.** The protocol calls for one only when a *shared* output bug is fixed in SIMPLNX; here both output deviations are legacy-only and SIMPLNX needed no output change, so there is nothing to bring back into alignment.

## Outstanding before promotion to COMPLETE

1. **PR-reviewer sign-off** (see header) — second-engineer review delegated by requester decision 2026-08-19.
2. **Out-of-core build runs: waived** by requester decision 2026-08-19. Not run for this filter.
3. **Path 20, the cancel early-return, is not directly tested.** It needs cancel-signal injection.
4. **Latent out-of-core concurrent-read concern, source-derived only.** The three tier tasks read the same `AbstractDataStore<int32>` concurrently. With the in-core `DataStore` that is a plain read of immutable memory and is safe; with a chunked out-of-core store whose reads mutate a shared chunk cache it may not be. Because item 2 waives OOC runs, this is unverified either way. Recorded in the deviations file's non-deviation 4.
5. **Recommended cleanup, deliberately not done:** replace the reciprocal coordinate decode with integer division (`nn / nx` and `nn / (nx * ny)`). Proven a no-op on every reachable input, so changing it in a pass whose premise is "no output changes" would add risk without benefit.
6. **Progress messages** are absent from an algorithm whose cost is O(cells × max-distance). A candidate for a follow-up.
