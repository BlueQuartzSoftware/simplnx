# Deviations from DREAM3D 6.5.171: ComputeEuclideanDistMapFilter

This file lists every documented behavioral difference between `nx::core::ComputeEuclideanDistMapFilter` and DREAM3D 6.5.171 `FindEuclideanDistMap` (SIMPL UUID `933e4b2d-dd61-51c3-98be-00548ba783a3`), plus one semantics note that is shared by both versions.

Entries are referenced by stable ID (`ComputeEuclideanDistMapFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

Evidence classes are labeled throughout: **executed** means a binary was run and its output compared; **source-derived** means the conclusion was read off the source and verified by an independent reimplementation, but no run isolates it on its own.

**ID namespace mapping.** Two ID namespaces are in play and they are not the same: `ED-<N>` are the *pre-identified finding* IDs from the batch plan and are the labels emitted by the A/B comparison tooling (`ww_work/ComputeEuclideanDistMap/results_compare.txt`, `predict_legacy.py`); `D<N>` are the *deviation* IDs defined in this file. The A/B rows cross-reference as **`ED-2` = `D1`** (bad-data cells: legacy `0.0`, SIMPLNX `-1`) and **`ED-6` = `D2`** (legacy zero-initialised `NearestNeighbors`, spurious lower-tier seeds; `ED-6` is a label the comparison tooling coined for this previously unidentified bug, so it has no row in the batch plan's pre-identified table). `ED-1` is recorded here as `D4` and `ED-5` as `D3`; `ED-3` and `ED-4` are SIMPLNX-side gaps with no deviation entry.

---

## ComputeEuclideanDistMapFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeEuclideanDistMapFilter-D1` |
| **Filter UUID** | `ba9ae8f6-443e-41d3-bb45-a08a139325c1` |
| **Status** | active |

**Symptom:** In float32 (non-Manhattan) mode, cells whose *Feature Id* is less than or equal to zero — the *bad data* region — get `0.0` from DREAM3D 6.5.171 and `-1.0` from SIMPLNX. The int32 Manhattan mode agrees on `-1` in both versions.

**Root cause:** Bug in 6.5.171. Its `NearestNeighbors` scratch array is a real 3-component `DataArray<int32_t>` created by `dataCheck()` with initial value **0** (`FindEuclideanDistMap.cpp:445-446`), while SIMPLNX uses a local `std::vector<int64>` initialised to **-1** (`Algorithms/ComputeEuclideanDistMap.cpp:314`). Both versions' seed loop is guarded by `if(feature > 0)` (legacy `:552`, SIMPLNX `:356`), so bad-data cells are never written, and both versions' propagation admits a cell as a seed when its component is `>= 0` (legacy `:123`, SIMPLNX `:91`). Under legacy's zero initialisation every bad-data cell therefore reads back as *its own nearest neighbour*. Its distance element is `-1`, so the Manhattan sweep never touches it and the int32 output stays `-1` — but the euclidean pass is gated only on `nearestneighbor >= 0` (legacy `:245`), so it computes the cell-to-itself distance and writes `0.0`.

The mechanism is directly visible in 6.5.171's own internal state: a run with `SaveNearestNeighbors = true` on the 10x6x1 fixture writes `NearestNeighbors[i] = i` at exactly the six bad-data cells (9, 19, 29, 39, 49, 59) and the correct seed index everywhere else. It is also baked into 6.5.171's own published unit-test expectations (`Source/Plugins/Statistics/Test/FindEuclideanDistMapTest.cpp`): `GBManhattan[9] == -1` but `GBEuclidean[9] == 0.0f`, and the whole `QPEuclidean` array is `-1.0f` except the bad-data column, which is `0.0f`.

**Evidence class:** executed. Confirmed on fixtures A (six bad-data cells) and F (all nine cells bad) through both binaries, in every toggle combination, for GB, TJ and QP.

**Affected users:** Anyone running the filter in float32 mode on data that has a bad-data region — every EBSD scan with an unindexed border or a cropped-out overscan. The affected cells are exactly the bad-data cells; all other cells agree. Downstream code that thresholds "distance < 1" or treats `0` as "on a boundary" silently reclassifies the entire bad-data region under 6.5.171.

**Recommendation:** Trust SIMPLNX. `0.0` claims the cell sits *on* a boundary, which is the opposite of the truth — the cell has no distance at all, because it was excluded from the computation. `-1` is the value both versions already use for the same situation in Manhattan mode, so SIMPLNX is additionally self-consistent across its two output modes where 6.5.171 is not.

---

## ComputeEuclideanDistMapFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeEuclideanDistMapFilter-D2` |
| **Filter UUID** | `ba9ae8f6-443e-41d3-bb45-a08a139325c1` |
| **Status** | active |

**Symptom:** When *Calculate Distance to Triple Lines* or *Calculate Distance to Quadruple Points* is enabled but a lower-tier option is **not**, DREAM3D 6.5.171 produces a substantially different — and wrong — map from SIMPLNX, in **both** distance modes. Concretely, on the 10x6x1 fixture, `TJ only` differs at 28 of 60 cells in Manhattan mode and 54 of 60 in float mode; `QP only` differs at 54 of 60 and 60 of 60 respectively. With all three options enabled, or with *Calculate Distance to Boundaries* enabled alongside, the two versions agree (modulo D1).

**Root cause:** Bug in 6.5.171, same zero-initialised `NearestNeighbors` array as D1. Component 1 (triple lines) is written by the empty-coordination branch, the boundary branch and the triple-line branch; component 2 (quadruple points) by the empty, boundary, triple-line and quadruple-point branches (`FindEuclideanDistMap.cpp:601-651`). When a lower tier's toggle is off, cells that qualify for that lower tier but not for the requested one fall through every branch, keep their initial `0`, and are admitted as **spurious seeds**. A spurious seed's distance element is `-1` rather than `0`, so it does not propagate on the first pass; instead the commit half of pass 1 assigns it distance 1 (`:217-221`) and from pass 2 on it propagates like a genuine seed. The requested map is then a distance field grown from the union of the real seeds and every lower-tier boundary cell.

Enumerating the eight toggle combinations against `:601-651` gives the exact affected set:

| Toggle set | 6.5.171 result |
|---|---|
| Boundaries only | clean (component 0 is always written) |
| Triple lines only | every coordination-size-1 cell is a spurious TJ seed |
| Quadruple points only | every coordination-size-1 **and** size-2 cell is a spurious QP seed |
| Quadruple points + triple lines | every coordination-size-1 cell is a spurious QP seed |
| Boundaries + anything | clean (the boundary branch writes `-1` into components 1 and 2) |
| All three | clean |

SIMPLNX's `-1` initialisation makes each tier's output independent of the other toggles. That independence is asserted directly as invariant I6 of `ComputeEuclideanDistMapTest.cpp`'s Class 4 TEST_CASE.

**Evidence class:** executed. All 56 fixture x toggle-set x mode combinations were run through both binaries; every difference matched the per-combination prediction derived from the legacy source before the run.

**Affected users:** Anyone who ran 6.5.171 with *Calculate Distance to Triple Lines* or *Calculate Distance to Quadruple Points* enabled while leaving *Calculate Distance to Boundaries* off — which is the natural configuration when only the triple-line or quadruple-point map is wanted. Users who left all three enabled (the GUI default) were never affected. This is a much larger divergence than D1: it is not confined to a few cells.

**Recommendation:** Trust SIMPLNX. 6.5.171's single-toggle triple-line and quadruple-point maps are not distance-to-triple-line or distance-to-quadruple-point fields at all; they are distance fields grown from a contaminated seed set. Any 6.5.171 result produced with a lower-tier toggle off should be regenerated.

---

## ComputeEuclideanDistMapFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeEuclideanDistMapFilter-D3` |
| **Filter UUID** | `ba9ae8f6-443e-41d3-bb45-a08a139325c1` |
| **Status** | active |

**Symptom:** With all three category options disabled, DREAM3D 6.5.171 runs to completion, reports success and produces no output array. SIMPLNX rejects the configuration in preflight with error `-12802`.

**Root cause:** Algorithmic choice. 6.5.171 has no such validation: `dataCheck()` simply creates nothing, `findDistanceMap()` dispatches no task, and `execute()` returns cleanly. SIMPLNX added the guard (`ComputeEuclideanDistMapFilter.cpp:133-138`).

**Evidence class:** executed. `PipelineRunner` exits 0 with no distance array in the output file; `nxrunner` exits 146 and writes no output file.

**Affected users:** Anyone who disabled all three options, deliberately or by accident. Under 6.5.171 the pipeline appeared to succeed while producing nothing, so a downstream filter would fail later with a confusing missing-array error.

**Recommendation:** Trust SIMPLNX. Failing at preflight with a message naming the three options is strictly more useful than a silent no-op. Note that the message was itself defective until this V&V pass: it passed the *Feature Ids* path to `fmt::format` with no `{}` placeholder to receive it, so the path was silently dropped from the text. Fixed in this pass and pinned by the `-12802 no output selected` section of the Error Paths TEST_CASE.

---

## ComputeEuclideanDistMapFilter-D4 — shared semantics note (not a version difference)

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeEuclideanDistMapFilter-D4` |
| **Filter UUID** | `ba9ae8f6-443e-41d3-bb45-a08a139325c1` |
| **Status** | active |

**Symptom:** The float32 output is **not** a Euclidean distance transform. It is the straight-line distance from each cell to *the one seed cell the layer-by-layer propagation happened to hand it*, and that seed is selected by a fixed tie-break rather than by proximity. The reported distance is therefore always greater than or equal to the true distance to the nearest seed, and can be strictly greater. Both SIMPLNX and 6.5.171 behave identically here — the two implementations of the propagation loop are line-for-line the same — so this is a shared semantics note recorded here for migration guidance, not a divergence.

**Root cause:** Algorithmic choice, shared. The propagation loop assigns

```
voxel_NearestNeighbor[i] = voxel_NearestNeighbor[neighpoint];
```

*unconditionally* inside a `j = 0..5` loop over the neighbour offsets `{ -nx*ny, -nx, -1, +1, +nx, +nx*ny }` = `{ -Z, -Y, -X, +X, +Y, +Z }` (`Algorithms/ComputeEuclideanDistMap.cpp:185-200`; legacy `FindEuclideanDistMap.cpp:200-210`). The last qualifying neighbour therefore wins, giving the descending priority

```
+Z  >  +Y  >  +X  >  -X  >  -Y  >  -Z
```

The propagation itself is correct: it is a genuine layer-synchronous breadth-first search, so the *Manhattan* output is the exact 6-connected graph distance and the tie-break cannot change any of its values (every neighbour available in a given pass carries the same pass number; only the recorded seed *identity* differs, and that identity is not written out). It is the float32 conversion that inherits the arbitrariness, because it measures to that recorded identity.

The canonical demonstration is the 10x6x1 fixture with spacing `(1, 2, 1)` and features stacked in Y. Cell 0, at grid position `(0, 0, 0)`, has a boundary seed at `(2, 0, 0)` — physical distance `2.0` — but the `+Y` neighbour outranks the `+X` one, so the propagation hands it the seed at `(0, 2, 0)` and the output is `4.0`. Both binaries produce `4.0`, and 6.5.171's own published unit test asserts `GBEuclidean[0] == 4.0f`.

**Evidence class:** executed for the value itself (both binaries, and 6.5.171's `SaveNearestNeighbors` output records seed index 20 = `(0, 2, 0)` for cell 0, not 2 = `(2, 0, 0)`); source-derived for the general tie-break rule, independently reimplemented in the V&V oracle and confirmed against every fixture.

**Affected users:** Anyone using the float32 output as a nearest-boundary distance. On isotropic spacing the error is bounded by the difference between the tie-break's pick and the nearest seed and is often small; on anisotropic spacing it can be a factor of the spacing ratio, as the `4.0` vs `2.0` example shows. The int32 Manhattan output is unaffected.

**Recommendation:** Either acceptable — the two versions agree exactly. The behaviour is now documented in the user-facing filter documentation, which previously described the output as "the distance of each Cell from the nearest Feature boundary" without qualification. Users who need a true nearest-boundary distance should not use this filter's float32 output. Changing the semantics would alter the output of every existing pipeline and is out of scope for a V&V pass; it is recorded here as the basis for a future decision.

---

## Confirmed non-deviations

Investigated this pass and deliberately **not** recorded as deviations, so they are not relitigated.

1. **The reciprocal coordinate decode.** Both versions decode a seed's flat index back to grid coordinates by reciprocal multiplication rather than integer division (`Algorithms/ComputeEuclideanDistMap.cpp:237-239`; legacy `:247-249`):

   ```
   y2idx = (int64)(nn * (1.0 / nx)) % ny
   z2idx = floor(nn * (1.0 / (nx * ny)))
   ```

   A truncation of a value that landed one ULP below an exact integer would silently mis-place the seed by one cell. It does not happen: a numeric sweep of every exact multiple `k*d` for `k` up to 2^31 and every divisor used by these fixtures plus `d` in `{3, 5, 6, 7, 9..17, 20, 24, 30, 50, 60, 100, 189, 201, 1000, 1023, 1025}` found **no** case where the reciprocal form disagrees with integer division. The single multiplication's error is bounded by one ULP, and one ULP of `k` is far smaller than the `1/d` gap to the next integer for any realistic image geometry, so the hazard is latent rather than active. **Evidence class: executed** (numeric sweep, `oracle.py::hazard_scan` plus the standalone divisor scan recorded in the report). Recommendation for a future cleanup: replace both expressions with integer division. Not changed here, because it is a no-op on every reachable input and this pass makes no output changes.

2. **Legacy's mismatched `readFilterParameters` override.** `FindEuclideanDistMap::readFilterParameters(reader, index)` (`:338-352`) reads `"CalcOnlyManhattanDist"`, `"GBEuclideanDistancesArrayName"`, `"TJEuclideanDistancesArrayName"` and `"QPEuclideanDistancesArrayName"` — none of which are the filter's property names. If that override were live, a JSON pipeline could not select float32 output at all. It is dead code: SIMPL's `JsonFilterParametersReader` routes through the `QJsonObject` overload, which iterates the registered `FilterParameter`s and reads them by property name. **Evidence class: executed.** A probe pipeline requesting `CalcManhattanDist = 0` produced a `float32` output array from the 6.5.171 binary, which is only possible if the property name was honoured.

3. **Toggle-independence within SIMPLNX.** Component `k` of the `nearestNeighbors` scratch vector could in principle be written by a higher tier's branch and disturb a lower tier's map. Enumerating all eight toggle combinations against `:400-438` shows every component always ends up with the same value whenever its map is computed, because the higher-tier branches write the same `coordination[0]` into the lower components that the lower-tier branches would. Asserted as invariant I6 rather than left as an argument. (6.5.171 does *not* satisfy this — that is D2.)

4. **Parallel task safety.** The three tiers are dispatched as three concurrent `ParallelTaskAlgorithm` tasks sharing one `std::vector<int64> nearestNeighbors` and both reading the same `FeatureIds` data store. Each task touches only its own component (`a * 3 + MapType`), so the writes are to distinct memory locations and there is no race on the vector. Confirmed empirically by the Parallel Determinism TEST_CASE (fixture A, all three tiers, both modes, three repeats, bit-identical). **Latent out-of-core hazard, recorded not fixed:** the three tasks *do* read the same `AbstractDataStore<int32>` concurrently, and `AbstractDataStore` is documented as not thread-safe for concurrent access. With the in-core `DataStore` this is a plain read of immutable memory and is safe; with a chunked out-of-core store whose reads mutate a shared chunk cache it may not be. Out-of-core build runs were waived for this batch (requester decision 2026-08-19), so this is source-derived only and is carried as an outstanding item in the report.

5. **Termination of the propagation loop when a tier has no seeds.** `while(count > 0 && changed > 0)` runs exactly one pass when no seed exists: `count` becomes the number of positive-`FeatureId` cells and `changed` stays 0, so the condition fails on re-entry. The output stays at the `-1` fill. Pinned by fixtures E (single feature, no boundary at all) and F (all bad data) and by the TJ/QP tiers of fixtures A, B and Dz. **Evidence class: executed.**
