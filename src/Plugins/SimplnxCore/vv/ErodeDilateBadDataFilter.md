# V&V Report: ErodeDilateBadDataFilter

|                             |                                                                          |
|-----------------------------|--------------------------------------------------------------------------|
| Plugin                      | SimplnxCore                                                              |
| SIMPLNX UUID                | `7f2f7378-580e-4337-8c04-a29e7883db0b`                                   |
| SIMPLNX Human Name          | Erode/Dilate Bad Data                                                    |
| DREAM3D 6.5.171 equivalent  | `ErodeDilateBadData`, SIMPL UUID `3adfe077-c3c9-5cd0-ad74-cf5f8ff3d254` (legacy source not present in this repository — see Algorithm Relationship) |
| Verified commit             | `3cd0f6cbd` (branch `vv/ErodeDialateBadData`) plus SIMPL-backwards-compatibility `TEST_CASE` added to `test/ErodeDilateBadDataTest.cpp` this pass — `SimplnxCoreUnitTest.exe` (Debug) built and run locally 2026-07-23 |
| Status                      | READY FOR REVIEW                                                         |
| Sign-off                    | *pending*                                                                |

## At a glance

| Aspect                 | Current state |
|------------------------|----------------|
| Algorithm Relationship | **Port** (inferred) — filter markdown description and legacy SIMPL UUID mapping confirm this replaces legacy `ErodeDilateBadData`; no line-level diff against legacy source was possible (source not in this repo). |
| Oracle (confirmed)     | **Class 1 (Analytical)** — expected outputs are hand-traced against a fixed 32-voxel (4×4×2) synthetic `FeatureIds` dataset with 5 bad-data voxels, for both operations, both iteration counts, and all 7 valid face-direction combinations. |
| Code paths enumerated  | 8 of 9 paths exercised. 1 confirmed gap: the zero-dimensions preflight error is never reached by any test (see below). |
| Tests today            | **5 TEST_CASEs, all pass** (built + run locally, 1883 assertions): `(Erode) Expanded`, `(Dilate) Expanded` (GENERATE sweep, 14 valid runs each), `(Dilate) No Dimensions`, `(Dilate) No Direction`, and `: SIMPL Backwards Compatibility` (new this pass — 2 `DYNAMIC_SECTION`s, 6.4 and 6.5, 27 assertions, both pass). |
| Test fixtures          | Inline `CreateTestData()` — no exemplar archive. 32-voxel `ImageGeom` (4×4×2), hand-set `FeatureIds` (5 bad voxels at indices 0, 10, 13, 14, 31; features 1–6 elsewhere) plus a `Misc` int32 array initialized to its own index (`data[i] = i`) so every transferred value traces back to its source voxel unambiguously. |
| Legacy comparison      | **Not performed.** Oracle is analytical only; no DREAM3D 6.5.171 pipeline/binary comparison was run for this V&V pass, and legacy source is not available in this repository to diff against. |
| Bug flags              | None confirmed. One implementation detail (`adjustValidNeighbors`) and one observed fixture characteristic are flagged below for second-engineer attention — see Code path coverage. |
| V&V phase              | Tests pass as written, including the newly added SIMPL backwards-compatibility test. Outstanding before promotion: (1) add a fixture that actually distinguishes direction combinations (see finding below); (2) add a dedicated zero-dimensions preflight test that doesn't also trip the no-direction check; (3) second-engineer review of the `adjustValidNeighbors` direction-masking implementation; (4) commit the new test case (currently uncommitted on this branch). |

## Summary

`ErodeDilateBadDataFilter` either erodes or dilates voxels with `FeatureId == 0` ("bad data") in an `ImageGeometry`. In *dilate* mode, every good voxel face-adjacent to a bad voxel has its data overwritten by the bad voxel's data (the bad region grows by one voxel per iteration). In *erode* mode, each bad voxel is assigned the data of whichever good face-neighbor's feature id occurs most often among its valid neighbors (first-processed wins on a tie). The operation repeats for a configurable number of iterations and can be restricted to any non-empty combination of X, Y, and Z face directions.

Verification is via a **Class 1 (Analytical) oracle**: two `GENERATE`-driven test cases (`(Erode) Expanded`, `(Dilate) Expanded`) sweep all 7 valid direction combinations (all-off is skipped) × 2 iteration counts against hand-traced expected `FeatureIds`/`Misc` arrays for a small, fully-inspectable 32-voxel dataset. Both tests pass, as do two preflight-error tests. A `SIMPL Backwards Compatibility` test (both the SIMPL 6.4 and 6.5 legacy pipeline JSON fixtures) was added this pass and also passes. Built and executed locally against the current branch head: **5/5 test cases pass, 1883 assertions**.

A concrete, verified gap: for this specific fixture, the 7 per-direction-combination expected-value functions (`CheckDataErode1XYZ`, `CheckDataErode1XY`, `CheckDataErode1XZ`, `CheckDataErode1X`, `CheckDataErode1YZ`, `CheckDataErode1Y`, `CheckDataErode1Z`, and their `Erode2`/`Dilate1`/`Dilate2` counterparts) all encode byte-identical expected arrays. The fixture therefore validates the core neighbor-voting/marking logic thoroughly, but does not actually discriminate "direction flag correctly restricts which neighbors participate" from "direction flag has no effect" for this dataset — see Code path coverage for detail and a recommended fixture change.

## Algorithm Relationship

*Classification:* **Port** (inferred) ~~| Minor changes | Rewrite | New filter~~

*Evidence available:*
- The SIMPLNX filter markdown (`docs/ErodeDilateBadDataFilter.md`) describes the same semantics as legacy DREAM3D — erode assigns the majority neighbor feature id ("if there is a tie... one... chosen randomly" — legacy phrasing retained), dilate grows the bad region by overwriting good neighbors. This text reads as carried over from the legacy filter's own documentation.
- `SimplnxCoreLegacyUUIDMapping.hpp` maps legacy SIMPL UUID `3adfe077-c3c9-5cd0-ad74-cf5f8ff3d254` directly to `FilterTraits<ErodeDilateBadDataFilter>`, and `test/simpl_conversion/{6_4,6_5}/ErodeDilateBadDataFilter.json` carry the legacy `Direction`/`NumIterations`/`XDirOn`/`YDirOn`/`ZDirOn`/`FeatureIdsArrayPath`/`IgnoredDataArrayPaths` parameter set unchanged — this is the same filter, not a reimplementation with a different parameter model.
- **What could not be verified:** the legacy DREAM3D 6.5.171 C++ source (`Source/Plugins/Processing/ProcessingFilters/ErodeDilateBadData.{h,cpp}`) is not present in this repository, so no line-level comparison of the vote/tie-break/boundary-handling logic against the legacy implementation was possible in this pass. The "Port" classification should be treated as inferred from documentation and UUID/parameter continuity, not confirmed by source diff.

*SIMPLNX implementation:* `Algorithms/ErodeDilateBadData.cpp` (~226 lines) uses `NeighborUtilities::VoxelNeighbors<Image3D>` for face-neighbor offsets and boundary validity, and `ParallelTaskAlgorithm` to transfer non-`FeatureIds` arrays in parallel (with `FeatureIds` itself transferred afterward, serially, since the transfer condition for every other array depends on the *current* `FeatureIds` values).

## Oracle

*Class:* **1 (Analytical)** — confirmed 2026-07-23.

*Applied:* `CreateTestData()` builds an in-memory 4×4×2 (32-voxel) `ImageGeom` with a hand-authored `FeatureIds` array (features 1–6, with bad voxels at flat indices 0, 10, 13, 14, and 31) and a `Misc` `int32` array initialized so `Misc[i] == i`, making every copied tuple traceable to its source voxel by value alone. Expected output arrays (`exemplarData`/`exemplarFeatures`, despite the "exemplar" naming these are hand-derived, not legacy-sourced) are provided per operation (Erode/Dilate), per iteration count (1, 2), and per direction combination (XYZ, XY, XZ, YZ, X, Y, Z) via 28 dedicated `CheckData*` functions in `test/ErodeDilateBadDataTest.cpp`.

*Encoded:* `SimplnxCore::ErodeDilateBadDataFilter(Erode) Expanded` and `(Dilate) Expanded` — each `GENERATE`s `dirX,dirY,dirZ ∈ {true,false}` and `numIterations ∈ {1,2}`, skips the all-directions-off combination (invalid per preflight), and dispatches to the matching `CheckData{Erode,Dilate}{1,2}{XYZ,XY,XZ,YZ,X,Y,Z}` function. **14 valid parameterized runs each for Erode and Dilate — all pass** (verified by local build+run, not just static review).

*Second-engineer review:* *Pending.* Recommend focused review of: (1) the erode tie-break order (first-processed-neighbor-wins, per `faceNeighborInternalIdx` iteration order `[-Z,-Y,-X,+X,+Y,+Z]`) against the intended/legacy semantics; (2) whether the fixture should be extended so that direction combinations produce genuinely different expected output (see below).

## Code path coverage

8 of 9 paths exercised. Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ErodeDilateBadData.cpp`.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Setup | `numFeatures` scan, face-offset/validity initialization, `adjustValidNeighbors` direction masking | All tests |
| 2 | (b) Per-voxel | `featureName != 0` (good voxel) → skip | All tests (majority of the 32 voxels are good) |
| 3 | (b) Per-voxel | `featureName == 0` + Dilate + neighbor `feature > 0` → `neighbors[neighborPoint] = voxelIndex` | `(Dilate) Expanded` |
| 4 | (b) Per-voxel | `featureName == 0` + Erode + neighbor `feature > 0` → vote accumulation, `neighbors[voxelIndex] = neighborPoint` on new-max (ties keep the first-processed neighbor) | `(Erode) Expanded` |
| 5 | (b) Per-voxel | Erode post-vote cleanup — `featureCount[feature] = 0` for each valid neighbor of the bad voxel | `(Erode) Expanded` (implicitly, via correct 2-iteration results) |
| 6 | (c) Transfer | `neighbor >= 0` + Erode condition (`featureName==0 && featureIds[neighbor]>0`) → `copyTuple` | `(Erode) Expanded` |
| 7 | (c) Transfer | `neighbor >= 0` + Dilate condition (`featureName>0 && featureIds[neighbor]==0`) → `copyTuple` | `(Dilate) Expanded` |
| 8 | (c) Transfer | `neighbor == -1` → skip (voxel untouched this iteration) | Both `Expanded` tests, implicitly (voxels far from bad data are unchanged in every expected array) |
| 9 | Preflight | `dims[0]==0 && dims[1]==0 && dims[2]==0` → error `-14602` (`k_NoGeometryDimensions`) | **Not covered.** The only test that zeroes the geometry dimensions (`(Dilate) No Dimensions`) *also* sets all three direction flags off, so the earlier `-14601` (`k_NoDirections_Error`) check fires first and the zero-dimensions branch is never reached. Confirmed by running the test locally: its assertion message is `-14601`, not `-14602`, despite the test's name. |

Additional confirmed items, not path gaps but worth recording:

- **No cancel path exists.** `m_ShouldCancel` is passed into `ErodeDilateBadData` and exposed via `getCancel()`, but `operator()` never reads it. The erode/dilate loop runs to completion regardless of a cancellation request — this is a behavior characteristic of the current implementation, not merely an untested path.
- **Direction masking is implemented unusually.** `adjustValidNeighbors` bitwise-ANDs the *face-index constants themselves* (`faceNeighborInternalIdx`, values 0–5) against the direction booleans, rather than gating a separate boolean-validity array. Combined with the observation that all 7 direction-combination fixtures for a given operation/iteration-count produce byte-identical expected output (see Summary and Oracle), this is flagged for second-engineer scrutiny — not as a confirmed defect (the current fixture cannot distinguish correct per-direction gating from a no-op direction gate), but as an area where an independent reviewer should hand-trace at least one single-axis-only case (e.g. Erode, `X` only, on a voxel whose good neighbors differ between the X-only and XYZ neighbor sets) to positively confirm the direction restriction behaves as documented.

## Test inventory

| Test case | Notes |
|-----------|-------|
| `SimplnxCore::ErodeDilateBadDataFilter(Erode) Expanded` | Class 1 oracle. `GENERATE` over 7 valid direction combinations × 2 iteration counts (14 runs). Compares `FeatureIds` and `Misc` against hand-traced expected arrays. Passes. |
| `SimplnxCore::ErodeDilateBadDataFilter(Dilate) Expanded` | Same sweep, Dilate operation. Passes. |
| `SimplnxCore::ErodeDilateBadDataFilter(Dilate) No Dimensions` | Preflight-error test: `ImageGeom` dimensions forced to `{0,0,0}`, directions also all off. Asserts `preflightResult.outputActions.invalid()`. **Misleading name** — actually exercises the no-direction path (`-14601`), not the zero-dimensions path (`-14602`), because directions are also off and that check runs first. |
| `SimplnxCore::ErodeDilateBadDataFilter(Dilate) No Direction` | Preflight-error test: all directions off, geometry otherwise valid. Asserts `-14601`. Correctly named and covers the intended path. |
| `SimplnxCore::ErodeDilateBadDataFilter: SIMPL Backwards Compatibility` | **New this pass.** `DYNAMIC_SECTION` over `simpl_conversion/6_5/ErodeDilateBadDataFilter.json` (matched by `Filter_Uuid`) and `simpl_conversion/6_4/ErodeDilateBadDataFilter.json` (matched by `Filter_Name`, no UUID field present in that fixture). Loads each legacy pipeline JSON via `Pipeline::FromSIMPLFile`, confirms it resolves to a single `PipelineFilter` with `FilterTraits<ErodeDilateBadDataFilter>::uuid`, and checks the converted arguments: `Operation == k_Dilate` (legacy `Direction: 0` round-trips to SIMPLNX's own `Dilate = 0`), `NumIterations == 5`, `XDirOn/YDirOn/ZDirOn == true`, geometry path `DataPath({"DataContainer"})`, feature-ids path `DataPath({"DataContainer","CellData","TestArray"})`. `IgnoredDataArrayPaths` (a `MultiDataArraySelectionFilterParameterConverter`) is verified only by successful pipeline load, not by value, matching the pattern used in `FillBadDataTest.cpp`. **27 assertions, both fixtures pass.** |

Both `simpl_conversion/6_4/ErodeDilateBadDataFilter.json` and `simpl_conversion/6_5/ErodeDilateBadDataFilter.json` were already present on disk (as they are for sibling filters such as `FillBadDataFilter`) but were unused until this pass — the gap noted in the previous revision of this report is now closed.

## Deviations from DREAM3D 6.5.171

Not evaluated in this pass — see [`deviations/ErodeDilateBadDataFilter.md`](deviations/ErodeDilateBadDataFilter.md). No legacy binary/pipeline comparison has been run for this filter; the oracle is Class 1 (Analytical) only, and legacy source is not present in this repository to support a source-level diff.
