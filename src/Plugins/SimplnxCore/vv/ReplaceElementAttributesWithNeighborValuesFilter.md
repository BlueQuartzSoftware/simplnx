# V&V Report: ReplaceElementAttributesWithNeighborValuesFilter

|           |                  |
|-----------|------------------|
| Plugin    | SimplnxCore      |
| SIMPLNX UUID                | `65128c53-d3be-4a69-a559-32a48d603884`       |
| SIMPLNX Human Name          | Replace Element Attributes with Neighbor (Threshold)                                             |
| DREAM3D 6.5.171 equivalent  | `ReplaceElementAttributesWithNeighborValues` — SIMPL UUID `17410178-4e5f-58b9-900e-8194c69200ab` |
| Verified commit             | *<filled at SBIR deliverable assembly>*      |
| Status                      | **COMPLETE**                                 |
| Sign-off                    | Nathan Young, 07-21-2026                                    |

## At a glance

| Aspect                 | Current state            |
|------------------------|--------------------------|
| Algorithm Relationship | **Port** — direct translation of SIMPL `ReplaceElementAttributesWithNeighborValues`; same two-pass (scan → copy) + optional-loop structure. Legacy UUID cited in filter header. |
| Oracle                | **Class 1 (Analytical)** — synthetic 3×3×3 grid, 4 SECTIONs with closed-form expected values. **Class 4 (Invariant)** companion — post-loop threshold saturation and multi-array copy. |
| Code paths            | 16 of 17 directly tested; cancel path untestable (no cancel-signal injection in test framework). |
| Tests                 | 2 TEST_CASEs: synthetic (4 SECTIONs, Class 1 + Class 4 oracle) + SIMPL backwards-compat (2 DYNAMIC_SECTIONs). |
| Exemplar archive      | `6_6_replace_element_attributes_with_neighbor.tar.gz` — unreferenced; consuming circular-oracle test removed. See `vv/provenance/ReplaceElementAttributesWithNeighborValues.md`. |
| Legacy comparison     | **Complete (2026-07-21, re-run 2026-07-21 post-refactor).** Bit-identical on LessThan and GreaterThan vs 6.5.171; LessThan vs 6.5.172. No deviations. See `vv/deviations/ReplaceElementAttributesWithNeighborValuesFilter.md`. |
| Bug flags             | None. |

## Summary

`ReplaceElementAttributesWithNeighborValuesFilter` replaces each "bad" cell (one that fails a user-supplied threshold comparison) by copying all AttributeMatrix arrays from the best-passing face neighbor. Algorithm: (1) scan pass — for each bad voxel, check 6 face neighbors and record the one with the most-extreme passing value; (2) copy pass — apply recorded neighbor copies to all cell arrays. An optional loop repeats until no bad voxels remain.

Verification used a Class 1 (Analytical) synthetic 3×3×3 fixture with closed-form expected values across four modes (LessThan, GreaterThan, loop=false, compare1=false), paired with Class 4 (Invariant) post-loop threshold saturation and multi-array copy assertions. A/B comparison against DREAM3D 6.5.171 and 6.5.172 confirmed bit-identical output on both operators. The comparison was re-run after the algorithm was refactored and remained bit-identical. No deviations.

## Algorithm Relationship

*Classification:* **Port**

Near line-by-line translation of SIMPL `ReplaceElementAttributesWithNeighborValues`. Legacy UUID `17410178-4e5f-58b9-900e-8194c69200ab` cited explicitly in `ReplaceElementAttributesWithNeighborValuesFilter.hpp`. SIMPL conversion fixtures present at `test/simpl_conversion/6_5/` and `6_4/ReplaceElementAttributesWithNeighborValuesFilter.json`. Two-pass architecture, 6-face-neighbor scan with individual boundary guards, and `IComparisonFunctor<T>` dispatch pattern are direct ports.

*Port-time deltas (none behavioral):*

1. **Algorithm class extraction** — core logic moved to `Algorithms/ReplaceElementAttributesWithNeighborValues.cpp` per SimplnxCore convention.
2. **Geometry access** — uses `NeighborUtilities::initializeFaceNeighborOffsets(dims)`; equivalent to legacy's hard-coded `int64 neighpoints[6]`.
3. **Array copy pass** — iterates `AttributeMatrix` with structured binding + `copyTuple`; legacy iterated SIMPL `AttributeMatrix` equivalently.
4. **Type dispatch** — `ExecuteDataFunction(ExecuteTemplate{}, ...)` vs SIMPL's `EXECUTE_FUNCTION_TEMPLATE`. UX/framework only.
5. **Progress reporting** — `ProgressMessage` via `messageHandler` vs `notifyStatusMessage`. UX only.

## Oracle

*Class:* **1 (Analytical)** primary + **4 (Invariant)** companion. Classes 2, 3, 5 N/A — no external library implements this heuristic; no published algorithm.

### Class 1 — Analytical derivations

3×3×3 grid, float32 `Confidence Index`, int32 `Marker` (initialized to linear index):

- **LessThan** (`compare = val < threshold`), bad={0,13,26}, good=0.9, bad=0.1, threshold=0.5, loop=true: bad voxels fail (0.1 < 0.5). Each has ≥1 face neighbor at 0.9. `compare1(0.9 ≥ 0.5)` = true; `compare2(0.9 > 0.1)` = true on first good neighbor; remaining equal neighbors fail `compare2(0.9 > 0.9)`. After copy pass: all three become 0.9. Second iteration: count=0, exit.
- **GreaterThan** (`compare = val > threshold`): symmetric with good=0.1, bad=0.9. All three become 0.1. All values ≤ 0.5 after one iteration.
- **loop=false**: single bad voxel at center (index 13), 6 good neighbors, one pass fills it. Deterministic.
- **compare1=false**: bad cluster = center (13) + all 6 face neighbors {4,10,12,14,16,22}. In the scan pass every `compare1` call for voxel 13 returns false → `bestNeighbor[13]` stays -1 → voxel 13 unchanged. Ring voxels each have ≥1 good face neighbor outside the cluster and are replaced.

Multi-array copy verified via `Marker`: after replacement `markerStore[13] != 13`, confirming the copy pass covers all AttributeMatrix arrays, not just the comparison array.

### Class 4 — Invariants

- Post-loop threshold saturation: after `loop=true` convergence, every value passes the threshold.
- Multi-array copy: `markerStore[13] != 13` after bad-center-voxel replacement.

### Encoded

`test/ReplaceElementAttributesWithNeighborValuesTest.cpp` — TEST_CASE `"Synthetic neighbor replacement"` — 4 SECTIONs with explicit `REQUIRE` checks. All pass.

*Second-engineer review:* Skipped — no second engineer available at time of V&V.

## Code path coverage

*16 of 17 paths directly tested; 1 untestable gap.*

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ReplaceElementAttributesWithNeighborValues.cpp`

| #  | Phase        | Path                                          | Test coverage                                    |
|----|--------------|----------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------|
| 1  | Loop         | Cancel check at top of `while(keepGoing)`     | **Untestable** — no cancel-signal injection available.                                                |
| 2  | (a) Scan     | Good voxel → skip; `count` not incremented    | All 24 good voxels in each synthetic SECTION.    |
| 3  | (a) Scan     | Bad voxel → enter 6-neighbor search; `count++`| LessThan/GreaterThan: voxels {0,13,26}; loop=false: voxel 13.                                        |
| 4  | (a) Scan     | -Z boundary guard (`plane == 0`) → skip       | Voxel 0 at (0,0,0).                              |
| 5  | (a) Scan     | -Y boundary guard (`row == 0`) → skip         | Voxel 0 at (0,0,0).                              |
| 6  | (a) Scan     | -X boundary guard (`column == 0`) → skip      | Voxel 0 at (0,0,0).                              |
| 7  | (a) Scan     | +X boundary guard (`column == dims[0]-1`) → skip                                                   | Voxel 26 at (2,2,2).                             |
| 8  | (a) Scan     | +Y boundary guard (`row == dims[1]-1`) → skip | Voxel 26 at (2,2,2).                             |
| 9  | (a) Scan     | +Z boundary guard (`plane == dims[2]-1`) → skip                                                    | Voxel 26 at (2,2,2).                             |
| 10 | (a) Scan     | `compare1=true` AND `compare2=true` → update `best` + `bestNeighbor[i]`                           | First good face neighbor of each bad voxel.      |
| 11 | (a) Scan     | `compare1=false` → skip neighbor              | compare1=false SECTION: cluster bad={4,10,12,13,14,16,22}; all neighbors of voxel 13 fail compare1; `bestNeighbor[13]` stays -1. |
| 12 | (a) Scan     | `compare1=true` AND `compare2=false` → skip neighbor                                               | Voxel 13: first good neighbor sets `best=0.9`; remaining 5 at 0.9 fail `compare2(0.9 > 0.9)`.       |
| 13 | (b) Copy     | `bestNeighbor[i] != -1` → `copyTuple` across all AttributeMatrix arrays                           | LessThan: voxels {0,13,26} replaced; `markerStore[13] != 13` confirms non-comparison array copied.   |
| 14 | (b) Copy     | `bestNeighbor[i] == -1` → skip                | All 24 good voxels; also voxel 13 in the compare1=false SECTION.                                     |
| 15 | Loop control | `loopUntilDone && count > 0` → `keepGoing = true`                                                  | LessThan: iteration 1 has count=3; sets keepGoing=true.                                               |
| 16 | Loop control | `count == 0` → exit while                     | LessThan: second iteration has count=0.          |
| 17 | Loop control | `loop=false` → single pass only               | loop=false SECTION.                              |

## Test inventory

| Test case (SECTION)                          | Oracle        | Parameters         |
|----------------------------------------------|---------------|-------------------------------------------------------------------------|
| Synthetic — LessThan                         | Class 1 + 4   | bad={0,13,26}, good=0.9, threshold=0.5, loop=true. All 27 ≥ threshold; `Marker[13]` copied. |
| Synthetic — GreaterThan                      | Class 1 + 4   | bad={0,13,26}, good=0.1, threshold=0.5, loop=true. All 27 ≤ threshold. |
| Synthetic — loop=false                       | Class 1       | bad={13}, good=0.9, threshold=0.5, loop=false. Single pass fills center. |
| Synthetic — compare1=false skip              | Class 1       | bad cluster={4,10,12,13,14,16,22}, loop=false. Center stays bad; ring replaced. Covers path 11. |
| SIMPL Backwards Compatibility (6.4 + 6.5)   | N/A           | UUID + arg-key + value round-trip via `Pipeline::FromSIMPLFile`. Not an oracle test. |

## Exemplar archive

- **Archive:** `6_6_replace_element_attributes_with_neighbor.tar.gz`
- **SHA512:** `319ebdf08b83ce5ec915afda8ee2af1e0952a3ce26b3e65a4171eb3125c7bc6613c3994610bf526f70413c00da9061c3e6c2d867643220d62faa8c3cd79a96cd`
- **Status:** Unreferenced — the circular-oracle test that consumed it was removed. Archive may be retired.
- **Provenance:** `vv/provenance/ReplaceElementAttributesWithNeighborValues.md`

## Deviations from DREAM3D 6.5.171

No deviations. A/B comparison (2026-07-21) confirmed bit-identical output on LessThan and GreaterThan vs 6.5.171, and LessThan vs 6.5.172.

Full record: `vv/deviations/ReplaceElementAttributesWithNeighborValuesFilter.md`
