# V&V Report: ErodeDilateBadDataFilter

|           |                  |
|-----------|------------------|
| Plugin    | SimplnxCore      |
| SIMPLNX UUID                | `7f2f7378-580e-4337-8c04-a29e7883db0b`                                   |
| SIMPLNX Human Name          | Erode/Dilate Bad Data                                                    |
| DREAM3D 6.5.171 equivalent  | `ErodeDilateBadData` — SIMPL UUID `3adfe077-c3c9-5cd0-ad74-cf5f8ff3d254` |
| Verified commit             | *<filled at SBIR deliverable assembly>*                                  |
| Status                     | COMPLETE                                 |
| Sign-off                   | *Matthew Marine, 8/7/2026*  second engineer: *Michael Jackson &lt;mike.jackson@bluequartz.net&gt;, 08-17-2026* |

## At a glance

| Aspect                 | Current state            |
|------------------------|--------------------------|
| Algorithm Relationship | **Port** — legacy `ErodeDilateBadData.{h,cpp}` was diffed line-by-line against `Algorithms/ErodeDilateBadData.cpp` this pass. Neighbor offsets, boundary-validity checks, vote/tie-break order, and transfer conditions are structurally identical. One divergence, since resolved — see Resolved Defects. |
| Oracle (confirmed)     | **Class 2 (Reference implementation).** The 28 expected `FeatureIds`/`Misc` arrays (7 direction combinations × 2 operations × 2 iteration counts) are genuine DREAM3D 6.5.171 output, matched element-wise against SIMPLNX and compiled into `ErodeDilateBadDataTest.cpp` as constants so the comparison re-runs in CI without the legacy binary. Confirmed — `(Erode) Expanded` and `(Dilate) Expanded` pass, 28/28 combinations. |
| Code paths enumerated  | **10 of 11 exercised.** All 6 face directions (-Z/-Y/-X/+X/+Y/+Z) confirmed hit by instrumentation. Only gap: the `m_ShouldCancel` early-exit (Path 11) — no test injects a cancel signal. |
| Tests today            | **7 TEST_CASEs, all pass in both in-core and OOC builds** (2283 assertions, identical in each): 1 production-scale exemplar-archive comparison + 2 `GENERATE` parameter sweeps (14 valid runs each over direction × iteration count) + 1 ignored-path test + 2 preflight-error tests + 1 SIMPL backwards-compat. |
| Exemplar archive       | `6_6_erode_dilate_test.tar.gz` — provides `Input Data` plus legacy-generated `Exemplar Bad Data Erode` / `Exemplar Bad Data Dilate` containers on a 189×201×20 Small IN100 slice. Consumed by the `(Erode)` test here and shared with `ErodeDilateMaskTest` and `ErodeDilateCoordinationNumberTest`. SHA512 verified against `test/CMakeLists.txt`. |
| Legacy comparison      | **Run.** Two independent comparisons: (1) all 28 parameter combinations run through DREAM3D 6.5.171 `PipelineRunner` against an HDF5 twin of the inline fixture and diffed element-wise — **28/28 exact matches** on both `FeatureIds` and `Misc`; (2) the `(Erode)` test compares SIMPLNX against a legacy-generated exemplar at production scale (759,780 cells, 6 cell arrays). |
| Bug flags              | `ErodeDilateBadDataFilter-D1` (X/Y/Z direction parameters had no effect) — **confirmed and resolved.** One additional hypothesis (Dilate tie-break order) was investigated, found to be a false lead, and reverted — see deviations doc. |
| V&V phase              | Oracle chosen and confirmed; legacy comparison run; direction-masking bug fixed; zero-dimensions preflight path now covered. Outstanding before promotion to COMPLETE: second-engineer sign-off, the uncovered cancel path (Path 11), and formalizing the manual 28-combination A/B run as an automated archive-based test (see deviations doc). |

## Summary

`ErodeDilateBadDataFilter` erodes or dilates voxels with `FeatureId == 0` ("bad data") in an `ImageGeometry`, optionally restricted to any non-empty combination of X, Y, and Z face directions. Verification is **Class 2**: the 28 expected output arrays compiled into `(Erode) Expanded` / `(Dilate) Expanded` are genuine DREAM3D 6.5.171 output for the same fixture, matched element-wise across every operation × direction × iteration combination, and the `(Erode)` test additionally compares against a legacy-generated exemplar archive at production scale. One SIMPLNX-side bug was found and resolved (`ErodeDilateBadDataFilter-D1` — the direction parameters had no effect at all); all 7 tests pass in both in-core and OOC builds with 2283 assertions.

## Algorithm Relationship

*Classification:* **Port** ~~| Minor changes | Rewrite | New filter~~

*Evidence:* `SimplnxCoreLegacyUUIDMapping.hpp` maps legacy SIMPL UUID `3adfe077-c3c9-5cd0-ad74-cf5f8ff3d254` directly to `FilterTraits<ErodeDilateBadDataFilter>`, and `test/simpl_conversion/{6_4,6_5}/ErodeDilateBadDataFilter.json` carry the legacy `Direction`/`NumIterations`/`XDirOn`/`YDirOn`/`ZDirOn`/`FeatureIdsArrayPath`/`IgnoredDataArrayPaths` parameter set unchanged — the same filter with the same parameter model, not a reimplementation. Legacy source (`Source/Plugins/Processing/ProcessingFilters/ErodeDilateBadData.{h,cpp}`, from a sibling `DREAM3D` checkout on the authoring engineer's machine, not committed to this repository) was diffed line-by-line against `Algorithms/ErodeDilateBadData.cpp` this pass rather than inferred from documentation.

*Port-time deltas:*

1. **Face-neighbor offsets** — legacy computed `neighpoints[]` inline; SIMPLNX calls `initializeFaceNeighborOffsets(dims)` from `NeighborUtilities`. Same six offsets in the same `[-Z,-Y,-X,+X,+Y,+Z]` order; no output change.
2. **Boundary-validity checks** — legacy tested each face boundary with inline conditionals; SIMPLNX calls `computeValidFaceNeighbors(x, y, z, dims)`. Reproduces the same six conditions; no output change.
3. **Direction gating** — legacy ORs the direction flag into each per-face boundary check (`|| !m_ZDirOn`); SIMPLNX masks the per-voxel `isValidFaceNeighbor` array in `adjustValidNeighbors`. Equivalent once wired in — but it was *not* wired in, which is `ErodeDilateBadDataFilter-D1`. Now fixed and legacy-verified.
4. **Parallel array transfer** — legacy transferred all cell arrays in one serial pass interleaved with `FeatureIds`; SIMPLNX uses `ParallelTaskAlgorithm` for the non-`FeatureIds` arrays and transfers `FeatureIds` afterward, serially. Proven equivalent: erode only maps 0→>0 and dilate only >0→0, `neighbors[]` always points at a voxel whose relevant polarity is preserved, and each index is written at most once per pass, so no transfer predicate can observe a changed value. No output change.
5. **Cancel check** — SIMPLNX reads `m_ShouldCancel` once per Z-slice; legacy has no cancel check at all. Additive; no output change on a run to completion.
6. **Progress reporting** — SIMPLNX reports per-array progress through `MessageHelper`/`ThrottledMessenger`; legacy used `notifyStatusMessage`. No output change.

*Material PRs since baseline:* four PRs have touched `Algorithms/ErodeDilateBadData.cpp` and account for the port-time deltas above:

- **#1523** — factored the 6-face-neighbor code out into `NeighborUtilities` (deltas 1–2, and the `isValidFaceNeighbor` array that delta 3 masks).
- **#1590** — standardized 2D image handling (`VoxelNeighbors<Image3D>` specialization).
- **#1340** — thread-safe messaging rework (delta 6).
- **#1687** — this branch: fixes D1, adds the `-14601`/`-14602` preflight guards, and rebuilds the test suite.

Earlier commits (#1249, #1017, #1013, #801) are compiler-warning, store-API, and rename churn with no behavioral content.

*SIMPLNX implementation:* `Algorithms/ErodeDilateBadData.cpp` (231 lines) uses `NeighborUtilities::VoxelNeighbors<Image3D>` for face-neighbor offsets and boundary validity, and `ParallelTaskAlgorithm` to transfer non-`FeatureIds` arrays in parallel.

## Resolved Defects

### ErodeDilateBadDataFilter-D1: Direction parameters had no effect — fixed

See deviations doc for full detail. Summary: `adjustValidNeighbors` was dead code (defined, never called); `XDirOn`/`YDirOn`/`ZDirOn` had zero effect on which face neighbors participated. Fixed by retyping the helper to mask the actual per-voxel validity array (`isValidFaceNeighbor`) with correct axis mapping, and calling it at `Algorithms/ErodeDilateBadData.cpp:162-163` for every bad-data voxel. Verified by regenerating all 28 exemplar constants from real DREAM3D 6.5.171 output (they were previously byte-identical across all 7 direction combinations for a given operation/iteration count) and matching them element-wise against SIMPLNX for every combination — see Oracle section.

### Investigated, disproven, reverted: Dilate tie-break "fix"

A plausible-looking bug hypothesis (last-bad-neighbor-wins vs. first-bad-neighbor-wins, for a good voxel with multiple bad neighbors) was implemented as a fix and then falsified by an actual legacy binary run. Reverted in full. Recorded as a confirmed non-deviation in the deviations doc so it isn't relitigated. This is why the Oracle section emphasizes binary-verified results over source-only reasoning: source comparison alone did not catch it, since legacy's own source has the identical "unconditional overwrite" line. Only running both binaries against the same input and diffing a value that is not blind to the tie-break (`Misc`, not `FeatureIds`) surfaced the truth.

## Oracle

*Class:* **2 (Reference implementation)** — expected values are genuine DREAM3D 6.5.171 output, at two different scales.

*Applied:* Two oracles, both sourced from the legacy filter and neither derived from SIMPLNX output.

- **Small-scale (28 combinations).** `CreateTestData()` builds an in-memory 4×4×2 (32-voxel) `ImageGeom` with a hand-authored `FeatureIds` array (features 1–6; bad voxels at flat indices 0, 10, 13, 14, 31) and a `Misc` `int32` array initialized so `Misc[i] == i`, making every copied tuple traceable to its source voxel by value alone. Pipeline JSONs (`DataContainerReader` → `ErodeDilateBadData` → `DataContainerWriter`) were run through the DREAM3D 6.5.171 `PipelineRunner` against an HDF5 twin of that fixture — verified byte-for-byte identical in dims, `FeatureIds`, and `Misc` before use — for all **28 combinations** of {Dilate, Erode} × {X, XY, XYZ, XZ, Y, YZ, Z} × {1, 2 iterations}. Outputs were diffed with `h5py` against SIMPLNX: **28/28 exact matches** on both arrays. Those legacy arrays are what the `k_ExemplarFeatureIds*` / `k_ExemplarData*` constants now hold — they were regenerated from the legacy binary, not hand-traced, which removes any question of their having been fitted to SIMPLNX's own behavior.
- **Production-scale (1 combination).** `6_6_erode_dilate_test.tar.gz` carries `Exemplar Bad Data Erode` and `Exemplar Bad Data Dilate` containers generated by the legacy SIMPL `ErodeDilateBadData` filter (UUID `3adfe077-…`, Erode and Dilate, `NumIterations=2`, all three directions on) on a 189×201×20 Small IN100 slice. The archive's embedded pipeline records that legacy build as `FilterVersion 6.6.338` — later than the 6.5.171 baseline — so this exemplar is genuine legacy output but is **not itself a 6.5.171 comparison**; the 6.5.171 comparison of record is the 28-combination run above. See `## Exemplar archive` and its provenance sidecar for both caveats.

*Coverage split:* the archive exemplar was generated with all three directions enabled, so it cannot discriminate direction gating — a build with D1 still present passes it. That is exactly the gap the 28-combination sweep closes, and why both oracles are needed. Conversely the 32-voxel fixture is single-typed and single-component, so only the archive test exercises `copyTuple` across mixed types and component counts (`EulerAngles` float32×3, `IPFColor` uint8×3, `Mask` uint8, `Phases` int32, `Confidence Index`/`Image Quality` float32).

*Encoded:*

- `SimplnxCore::ErodeDilateBadDataFilter(Erode) Expanded` and `(Dilate) Expanded` — each `GENERATE`s `dirX,dirY,dirZ ∈ {true,false}` and `numIterations ∈ {1,2}`, reports the invalid all-directions-off combination with `SUCCEED`, and looks the expected arrays up in the 28-row `k_Exemplars` table. 14 valid parameterized runs each, both `FeatureIds` and `Misc` asserted, 1039 assertions each — all pass.
- `SimplnxCore::ErodeDilateBadDataFilter(Erode)` — `UnitTest::CompareExemplarToGeneratedData` against `6_6_erode_dilate_bad_data.dream3d`, 55 assertions. Passes.

*Second-engineer review:* **pending.** The previously open items — erode/dilate tie-break order, and whether direction combinations produce genuinely different output — were both resolved this pass by the legacy binary comparison above rather than by review alone. A named second-engineer sign-off on the oracle design is still required before Status can move to COMPLETE.

## Code path coverage

10 of 11 paths exercised. The algorithm has three logical phases: (a) preflight validation, (b) a per-voxel vote/mark scan over face neighbors, and (c) a per-array transfer pass.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ErodeDilateBadData.cpp` (231 lines).

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | (a) Setup | `numFeatures` scan, face-offset/validity initialization, `adjustValidNeighbors` direction masking | All tests. Functional as of this pass — in the prior revision `adjustValidNeighbors` was dead code; it is now wired in and confirmed exercised for all 6 face directions (see below). |
| 2 | (b) Per-voxel | `featureName != 0` (good voxel) → skip | All tests (most of the 32 voxels are good) |
| 3 | (b) Per-voxel | `featureName == 0` + Dilate + neighbor `feature > 0` → `neighbors[neighborPoint] = voxelIndex` | `(Dilate) Expanded`, all 6 face directions confirmed hit |
| 4 | (b) Per-voxel | `featureName == 0` + Erode + neighbor `feature > 0` → vote accumulation, `neighbors[voxelIndex] = neighborPoint` on new max (ties keep the first-processed neighbor) | `(Erode) Expanded`, all 6 face directions confirmed hit |
| 5 | (b) Per-voxel | Erode post-vote cleanup — `featureCount[feature] = 0` for each valid neighbor of the bad voxel | `(Erode) Expanded`, all 6 face directions confirmed hit |
| 6 | (c) Transfer | `neighbor >= 0` + Erode condition (`featureName == 0 && featureIds[neighbor] > 0`) → `copyTuple` | `(Erode) Expanded`, `(Erode)` |
| 7 | (c) Transfer | `neighbor >= 0` + Dilate condition (`featureName > 0 && featureIds[neighbor] == 0`) → `copyTuple` | `(Dilate) Expanded` |
| 8 | (c) Transfer | `neighbor == -1` → skip (voxel untouched this iteration) | Both `Expanded` tests, implicitly (voxels far from bad data are unchanged in every expected array) |
| 9 | (a) Preflight | `!xDirOn && !yDirOn && !zDirOn` → error `-14601` (`k_NoDirectionsError`) | `No Direction` — asserts `invalid()` and `errors()[0].code == -14601` for both operations |
| 10 | (a) Preflight | `dims[0] == 0 \|\| dims[1] == 0 \|\| dims[2] == 0` → error `-14602` (`k_NoGeometryDimensionsError`) | `No Dimensions` — directions all **on** so the `-14601` check does not mask this path (the prior revision zeroed them and never reached here); asserts error code is exactly `-14602` |
| 11 | (b) Cancel | `m_ShouldCancel` read once per Z-slice inside the iteration loop → early return | *Not directly tested. Requires cancel-signal injection; no test sets `m_ShouldCancel` and asserts early termination.* Legacy has no cancel check at all, so SIMPLNX is ahead of legacy here — not a deviation. |

**Per-direction coverage, confirmed by instrumentation:** `Algorithms/ErodeDilateBadData.cpp` was temporarily instrumented with per-face-direction hit counters at (a) the point immediately after the `isValidFaceNeighbor` gate in the vote/mark loop, (b) the point where the Dilate mark / Erode vote condition (`feature > 0`) fires, and (c) the equivalent point in the Erode cleanup loop. Running the full `(Erode) Expanded` + `(Dilate) Expanded` sweep produced non-zero counts for **every one of the 6 directions at all 3 measurement points** — vote/mark loop reached counts `-Z=38 -Y=111 -X=108 +X=106 +Y=64 +Z=97`; Dilate marking fired at `-Z=9 -Y=46 -X=37 +X=35 +Y=16 +Z=44`; Erode voting at `-Z=8 -Y=25 -X=24 +X=24 +Y=8 +Z=32`. The instrumentation was removed afterward; this records the empirical result, not a standing code artifact.

Confirmed correct and deliberately not counted as deviations:

- **Direction masking of the Erode `featureCount` reset loop.** SIMPLNX reuses the direction-masked `isValidFaceNeighbor` in the reset loop; legacy resets over *boundary-valid* neighbors and ignores the direction flags there. Simulating both variants across all 28 combinations gives identical output, and the equivalence is general: the reset set is a superset of the increment set in both variants, so `featureCount` returns to all-zeros after every bad voxel either way.
- **`neighbors` is intentionally not reset between iterations** — matches legacy. The deviations doc records the investigation that established this.
- **`MessageHelper` shared across parallel tasks.** Each task gets its own `ThrottledMessenger` (independent timing state) over a shared `std::shared_ptr<Messenger>`; `trySendMessage` is the documented cross-thread path.

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::ErodeDilateBadDataFilter(Erode)` | kept | Class 2 production-scale oracle. `6_6_erode_dilate_test.tar.gz`, Erode, all directions on, 2 iterations, 189×201×20. `CompareExemplarToGeneratedData` over the 6 cell arrays present in both containers (`Confidence Index`, `EulerAngles`, `FeatureIds`, `Image Quality`, `Mask`, `Phases`; the exemplar's `IPFColor` has no counterpart in `Input Data` and is skipped); 55 assertions. The only test that exercises `copyTuple` across mixed types and component counts. Restored this pass after an earlier revision dropped it. |
| `SimplnxCore::ErodeDilateBadDataFilter(Erode) Expanded` | new-for-V&V | Class 2 small-scale oracle. `GENERATE` over 7 valid direction combinations × 2 iteration counts (14 runs); all-off reported via `SUCCEED`. Both `FeatureIds` and `Misc` asserted against the `k_Exemplars` table; 1039 assertions. Modified this pass: expected arrays regenerated from legacy output (previously byte-identical across direction combinations, per D1), the `Misc` assertion enabled for the first time, and the two duplicated dispatch helpers collapsed into one table lookup. |
| `SimplnxCore::ErodeDilateBadDataFilter(Dilate) Expanded` | new-for-V&V | Same sweep and same table, Dilate operation; 1039 assertions. Same modifications as the Erode sweep. |
| `SimplnxCore::ErodeDilateBadDataFilter Ignored Path` | new-for-V&V | Confirms an array listed in `IgnoredDataArrayPaths` (`Misc`) is left untouched, for both operations via `GENERATE(k_Dilate, k_Erode)`; 91 assertions. Modified this pass: the prior revision only preflighted, so comparing the DataStructure against a fresh fixture could not fail. It now executes the filter and additionally asserts that `FeatureIds` *did* change, which is what keeps the ignored-path check from passing vacuously. Verified by mutation — emptying the ignore list, or dropping the `execute()` call, each now fails the test. |
| `SimplnxCore::ErodeDilateBadDataFilter No Direction` | new-for-V&V | Preflight-error test: all directions off, geometry otherwise valid, both operations × 2 iteration counts. Asserts `invalid()` and `errors()[0].code == -14601`; 25 assertions. Covers Path 9. |
| `SimplnxCore::ErodeDilateBadDataFilter No Dimensions` | new-for-V&V | Preflight-error test: `ImageGeom` dimensions forced to `{0,0,0}`, directions all on, Dilate. Asserts `invalid()` and `errors()[0].code == -14602`; 7 assertions. Covers Path 10. Modified this pass: previously it also zeroed all direction flags, which tripped `-14601` first and left Path 10 unreached. |
| `SimplnxCore::ErodeDilateBadDataFilter: SIMPL Backwards Compatibility` | kept | `DYNAMIC_SECTION` over `simpl_conversion/6_5/ErodeDilateBadDataFilter.json` (matched by `Filter_Uuid`) and `simpl_conversion/6_4/ErodeDilateBadDataFilter.json` (matched by `Filter_Name`; that fixture has no UUID field). Loads each legacy pipeline via `Pipeline::FromSIMPLFile`, confirms a single `PipelineFilter` with the right UUID, and checks the converted arguments: `Operation == k_Dilate`, `NumIterations == 5`, `XDirOn/YDirOn/ZDirOn == true`, geometry path `DataPath({"DataContainer"})`, feature-ids path `DataPath({"DataContainer","CellData","TestArray"})`; 27 assertions. `IgnoredDataArrayPaths` verified only by successful pipeline load, matching the pattern in `FillBadDataTest.cpp`. Not an oracle test. |

All 7 tests pass in both the in-core build (`NX-Com-Qt69-Vtk96-Rel`) and the out-of-core build (`simplnx-ooc-Rel`), with identical assertion counts in each — 2283 total (55 + 1039 + 1039 + 91 + 25 + 7 + 27).

## Exemplar archive

- **Archive:** `6_6_erode_dilate_test.tar.gz`
- **SHA512:** `5f0773e5d296936effbb2239965f5847e7c18533b0a2c3ec6a1d6a83b03417e5b459cce29808c8e0273613b3b6fa032c675e84926eb35d8da8a6ddc0641a0ef5`
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/6_6_erode_dilate_test.md`

The archive is shared: `6_6_erode_dilate_bad_data.dream3d` serves this filter, `6_6_erode_dilate_mask.dream3d` serves `ErodeDilateMaskTest`, and `6_6_erode_dilate_coordination_number.dream3d` serves `ErodeDilateCoordinationNumberTest`. Any regeneration must account for all three consumers.

## Deviations from DREAM3D 6.5.171

No confirmed legacy deviations. The comparison was run at both scales described in the Oracle section — 28/28 exact matches on the 32-voxel fixture, and an element-wise match against the legacy-generated exemplar on the 189×201×20 Small IN100 slice.

One SIMPLNX-side defect, since resolved:

- `ErodeDilateBadDataFilter-D1` — the `XDirOn`/`YDirOn`/`ZDirOn` parameters had no effect on which face neighbors participated — see [`deviations/ErodeDilateBadDataFilter.md`](deviations/ErodeDilateBadDataFilter.md)

One hypothesis was investigated and disproven (Dilate tie-break order: legacy matches SIMPLNX's original last-write-wins behavior), and is recorded in the same file as a confirmed non-deviation so it is not relitigated.
**Fixed in DREAM3D-NX 7.4.2:** `ErodeDilateBadDataFilter-D1`.

