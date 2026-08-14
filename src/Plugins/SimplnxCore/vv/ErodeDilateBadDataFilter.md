# V&V Report: ErodeDilateBadDataFilter

|                             |                                                                          |
|-----------------------------|--------------------------------------------------------------------------|
| Plugin                      | SimplnxCore                                                              |
| SIMPLNX UUID                | `7f2f7378-580e-4337-8c04-a29e7883db0b`                                   |
| SIMPLNX Human Name          | Erode/Dilate Bad Data                                                    |
| DREAM3D 6.5.171 equivalent  | `ErodeDilateBadData`, SIMPL UUID `3adfe077-c3c9-5cd0-ad74-cf5f8ff3d254` (legacy source located on this machine and diffed directly this pass — see Algorithm Relationship) |
| Verified commit             | Head of branch `vv/ErodeDilateBadData` (PR #1687) as of 2026-08-14, i.e. the re-review fix commit on top of `42934ffcd` "Clang-format unit tests" — `SimplnxCoreUnitTest` built and all 7 tests run locally on that tree |
| Status                      | READY FOR REVIEW                                                         |
| Sign-off                    | *pending*                                                                |

## At a glance

| Aspect                 | Current state |
|------------------------|----------------|
| Algorithm Relationship | **Port, confirmed by direct source diff** — legacy `ErodeDilateBadData.{h,cpp}` located and compared line-by-line against `Algorithms/ErodeDilateBadData.cpp` this pass. Neighbor offsets, boundary-validity checks, vote/tie-break order, and transfer conditions are structurally identical. One divergence found and fixed — see Bug Fixes. |
| Oracle                 | **Class 2 (Reference implementation).** The 28 expected `FeatureIds`/`Misc` arrays (7 direction combinations × 2 operations × 2 iteration counts) were regenerated from genuine DREAM3D 6.5.171 binary output and verified element-wise against SIMPLNX; they are compiled into the test as constants so the comparison re-runs in CI without the legacy binary. The generating A/B run itself is manual/one-time — see deviations doc for a recommendation to formalize it as an archive-based test. |
| Code paths enumerated  | **9 of 9 paths exercised**, all 6 face directions (-Z/-Y/-X/+X/+Y/+Z) confirmed hit by instrumentation. The zero-dimensions preflight path, flagged as uncovered in the prior V&V pass, is now reached and correctly asserts `-14602` — see below. |
| Tests today            | **7 TEST_CASEs, all pass**: `(Erode)`, `(Erode) Expanded`, `(Dilate) Expanded` (GENERATE sweep, 14 valid runs each — **both `FeatureIds` and `Misc` asserted**, previously `Misc` was disabled), `Ignored Path` (both operations via `GENERATE(k_Dilate, k_Erode)`; preflights, executes, then asserts both that `Misc` is untouched *and* that `FeatureIds` changed), `No Direction` (both operations), `No Dimensions` (Dilate only), and `: SIMPL Backwards Compatibility`. |
| Test fixtures          | Inline `CreateTestData()` for the `Expanded` sweep — no exemplar archive for that sweep; the `(Erode)` test is separately archive-based (`6_6_erode_dilate_test.tar.gz`). 32-voxel `ImageGeom` (4×4×2), hand-set `FeatureIds` (5 bad voxels at indices 0, 10, 13, 14, 31; features 1–6 elsewhere) plus a `Misc` int32 array initialized to its own index (`data[i] = i`) so every transferred value traces back to its source voxel unambiguously. Separately, a byte-for-byte HDF5 twin of this fixture (`Test Data/erode_dilate_legacy/erode_dilate_bad_data_base_test.dream3d`) was used for the manual legacy A/B run — confirmed identical dims/FeatureIds/Misc before use. |
| Legacy comparison      | **Performed this pass.** All 28 combinations run through DREAM3D 6.5.171 (`PipelineRunner.exe`) against the `.dream3d` twin fixture; `FeatureIds` and `Misc` diffed element-wise against SIMPLNX's exemplar constants. **28/28 exact matches.** See Oracle section for the run list. |
| Bug flags              | `ErodeDilateBadDataFilter-B1` (direction parameters had no effect) — **confirmed and fixed this pass.** One additional hypothesis (Dilate tie-break order) was investigated, found to be a false lead, and reverted — see deviations doc. |
| V&V phase              | Direction-masking bug fixed, committed (`56b30c923`), and legacy-verified across all 28 combinations. Zero-dimensions preflight path now covered (9/9 paths). Outstanding before promotion: formalize the manual legacy A/B run as an automated Class 2 CI test (see deviations doc). |

## Summary

`ErodeDilateBadDataFilter` either erodes or dilates voxels with `FeatureId == 0` ("bad data") in an `ImageGeometry`. In *dilate* mode, every good voxel face-adjacent to a bad voxel has its data overwritten by the bad voxel's data (the bad region grows by one voxel per iteration). In *erode* mode, each bad voxel is assigned the data of whichever good face-neighbor's feature id occurs most often among its valid neighbors (first-processed wins on a tie). The operation repeats for a configurable number of iterations and can be restricted to any non-empty combination of X, Y, and Z face directions.

**This pass found and fixed a confirmed bug:** the X/Y/Z direction-restriction parameters had no effect on the algorithm at all — `adjustValidNeighbors`, the helper meant to mask face neighbors by direction, was defined but never called. This is exactly what produced the prior V&V pass's observation that all 7 direction-combination fixtures encoded byte-identical expected output — not a weak fixture, a genuinely broken direction parameter. Fixed and verified — see `ErodeDilateBadDataFilter-B1` in the deviations doc.

A second hypothesis — that the Dilate tie-break order (which of several bad neighbors a good voxel copies from) was also wrong — was investigated, a fix was implemented, and it was then **disproven** by running the actual DREAM3D 6.5.171 binary: legacy uses the same last-write-wins behavior the original SIMPLNX code already had. The fix was reverted. See deviations doc, "Dilate tie-break: last-bad-neighbor-wins is correct, not a bug."

Verification is **Class 2 (Reference implementation)**: two `GENERATE`-driven test cases (`(Erode) Expanded`, `(Dilate) Expanded`) sweep all 7 valid direction combinations (all-off is skipped) × 2 iteration counts against expected `FeatureIds`/`Misc` arrays for a small, fully-inspectable 32-voxel dataset. Those expected arrays are real DREAM3D 6.5.171 output for the same fixture, verified element-wise (see Oracle section). All 7 tests pass, **2283 assertions** (55 + 1039 + 1039 + 91 + 25 + 7 + 27, measured by direct local run of the `[ErodeDilateBadDataFilter]` tag), both `FeatureIds` and `Misc` checked in every `Expanded` run (`Misc` was previously commented out — see prior revision of this report).

## Algorithm Relationship

*Classification:* **Port** — confirmed by direct source diff this pass ~~(inferred) | Minor changes | Rewrite | New filter~~

*Evidence available:*
- Legacy source (`Source/Plugins/Processing/ProcessingFilters/ErodeDilateBadData.{h,cpp}`) was located on this machine (`C:\Users\holym\BlueQuartz\Projects\DREAM3D\DREAM3D\...`, a sibling checkout — not committed to this repository) and diffed line-by-line against `Algorithms/ErodeDilateBadData.cpp` this pass, not merely inferred from documentation:
  - Face-neighbor offset arithmetic (`neighpoints[]` vs. `initializeFaceNeighborOffsets`) — identical.
  - Boundary-validity checks per face — identical (`computeValidFaceNeighbors` reproduces the same six boundary conditions as the legacy inline checks).
  - Vote-count tie-break scan order `[-Z,-Y,-X,+X,+Y,+Z]` and comparison logic — identical.
  - Dilate/Erode transfer condition (`copyTuple` gating) — identical.
  - **One divergence found:** legacy ORs the direction flag into the same boundary check for every neighbor (`|| !m_ZDirOn` etc.); SIMPLNX's `adjustValidNeighbors` was supposed to do the equivalent but was never called — see Bug Fixes / deviations doc `ErodeDilateBadDataFilter-B1`.
- `SimplnxCoreLegacyUUIDMapping.hpp` maps legacy SIMPL UUID `3adfe077-c3c9-5cd0-ad74-cf5f8ff3d254` directly to `FilterTraits<ErodeDilateBadDataFilter>`, and `test/simpl_conversion/{6_4,6_5}/ErodeDilateBadDataFilter.json` carry the legacy `Direction`/`NumIterations`/`XDirOn`/`YDirOn`/`ZDirOn`/`FeatureIdsArrayPath`/`IgnoredDataArrayPaths` parameter set unchanged — this is the same filter, not a reimplementation with a different parameter model.

*SIMPLNX implementation:* `Algorithms/ErodeDilateBadData.cpp` (~230 lines) uses `NeighborUtilities::VoxelNeighbors<Image3D>` for face-neighbor offsets and boundary validity, and `ParallelTaskAlgorithm` to transfer non-`FeatureIds` arrays in parallel (with `FeatureIds` itself transferred afterward, serially, since the transfer condition for every other array depends on the *current* `FeatureIds` values).

## Bug Fixes (this pass)

### ErodeDilateBadDataFilter-B1: Direction parameters had no effect — fixed

See deviations doc for full detail. Summary: `adjustValidNeighbors` was dead code (defined, never called); `XDirOn`/`YDirOn`/`ZDirOn` had zero effect on which face neighbors participated. Fixed by retyping the helper to mask the actual per-voxel validity array (`isValidFaceNeighbor`) with correct axis mapping, and calling it at `Algorithms/ErodeDilateBadData.cpp:162-163` for every bad-data voxel. Verified by regenerating all 28 exemplar constants from real DREAM3D 6.5.171 output (they were previously byte-identical across all 7 direction combinations for a given operation/iteration count) and matching them element-wise against SIMPLNX for every combination — see Oracle section.

### Investigated, disproven, reverted: Dilate tie-break "fix"

A plausible-looking bug hypothesis (last-bad-neighbor-wins vs. first-bad-neighbor-wins, for a good voxel with multiple bad neighbors) was implemented as a fix and then falsified by an actual legacy binary run. Reverted in full. Recorded as a confirmed non-deviation in the deviations doc so it isn't relitigated. This is the reason the Oracle section below emphasizes binary-verified results over source-only reasoning — source-level comparison alone did not catch this, since legacy's own source has the identical "unconditional overwrite" line; only running both binaries against the same input and diffing a value that isn't blind to the tie-break (`Misc`, not `FeatureIds`) surfaced the truth.

## Oracle

*Class:* **2 (Reference implementation)** — the expected values are genuine DREAM3D 6.5.171 output. They are compiled as constants in `ErodeDilateBadDataTest.cpp` rather than loaded from an exemplar archive, so the comparison re-runs in CI without needing the legacy binary present.

*Fixture construction:* `CreateTestData()` builds an in-memory 4×4×2 (32-voxel) `ImageGeom` with a hand-authored `FeatureIds` array (features 1–6, with bad voxels at flat indices 0, 10, 13, 14, and 31) and a `Misc` `int32` array initialized so `Misc[i] == i`, making every copied tuple traceable to its source voxel by value alone.

*Expected-output provenance:* the 28 `k_ExemplarFeatureIds*` / `k_ExemplarData*` constant pairs — one per operation (Erode/Dilate) × iteration count (1, 2) × direction combination (XYZ, XY, XZ, YZ, X, Y, Z) — were **regenerated from the legacy 6.5.171 binary** and verified element-wise against SIMPLNX's output, not hand-traced. That is a stronger oracle than a hand derivation and removes any question of the arrays having been fitted to SIMPLNX's own behavior.

*Legacy A/B run (this pass):* Built pipeline JSONs (`DataContainerReader` → `ErodeDilateBadData` → `DataContainerWriter`) and ran them through the actual DREAM3D 6.5.171 binary (`PipelineRunner.exe`, `C:\Users\holym\BlueQuartz\Builds\DREAM3D\DREAM3D-6.5.171-Win64`) against `Test Data/erode_dilate_legacy/erode_dilate_bad_data_base_test.dream3d` — verified byte-for-byte identical to the C++ `CreateTestData()` fixture (dims, `FeatureIds` including which 5 voxels are bad, `Misc`) before use. Ran and diffed (via `h5py`) all **28 combinations**: {Dilate, Erode} × {X, XY, XYZ, XZ, Y, YZ, Z} × {1, 2 iterations}. **28/28 exact matches**, both `FeatureIds` and `Misc`, against the exemplar constants now in `ErodeDilateBadDataTest.cpp`.

*Encoded:* `SimplnxCore::ErodeDilateBadDataFilter(Erode) Expanded` and `(Dilate) Expanded` — each `GENERATE`s `dirX,dirY,dirZ ∈ {true,false}` and `numIterations ∈ {1,2}`, skips the all-directions-off combination (invalid per preflight), and dispatches to the matching exemplar constants. **14 valid parameterized runs each for Erode and Dilate, both `FeatureIds` and `Misc` asserted — all pass** (built + run locally; `Misc` assertion was disabled in the prior pass and is now active for the first time).

*Second-engineer review:* Prior pass's open items — erode/dilate tie-break order, and whether direction combinations produce genuinely different output — are both **resolved this pass** via the legacy binary comparison above, not merely reviewed. Remaining recommendation: formalize the manual A/B run as an automated Class 2 test (see deviations doc) so future changes are caught by CI rather than requiring another manual pass.

## Code path coverage

9 of 9 paths exercised. Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ErodeDilateBadData.cpp`.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Setup | `numFeatures` scan, face-offset/validity initialization, `adjustValidNeighbors` direction masking | All tests. **As of this pass, this path is actually functional** — in the prior revision of this report, `adjustValidNeighbors` was listed here but was dead code (never called); it is now wired in and confirmed exercised for all 6 face directions, see below. |
| 2 | (b) Per-voxel | `featureName != 0` (good voxel) → skip | All tests (majority of the 32 voxels are good) |
| 3 | (b) Per-voxel | `featureName == 0` + Dilate + neighbor `feature > 0` → `neighbors[neighborPoint] = voxelIndex` | `(Dilate) Expanded`, all 6 face directions confirmed hit (see below) |
| 4 | (b) Per-voxel | `featureName == 0` + Erode + neighbor `feature > 0` → vote accumulation, `neighbors[voxelIndex] = neighborPoint` on new-max (ties keep the first-processed neighbor) | `(Erode) Expanded`, all 6 face directions confirmed hit (see below) |
| 5 | (b) Per-voxel | Erode post-vote cleanup — `featureCount[feature] = 0` for each valid neighbor of the bad voxel | `(Erode) Expanded`, all 6 face directions confirmed hit (see below) |
| 6 | (c) Transfer | `neighbor >= 0` + Erode condition (`featureName==0 && featureIds[neighbor]>0`) → `copyTuple` | `(Erode) Expanded` |
| 7 | (c) Transfer | `neighbor >= 0` + Dilate condition (`featureName>0 && featureIds[neighbor]==0`) → `copyTuple` | `(Dilate) Expanded` |
| 8 | (c) Transfer | `neighbor == -1` → skip (voxel untouched this iteration) | Both `Expanded` tests, implicitly (voxels far from bad data are unchanged in every expected array) |
| 9 | Preflight | `dims[0]==0 \|\| dims[1]==0 \|\| dims[2]==0` → error `-14602` (`k_NoGeometryDimensionsError`) | **Covered.** `No Dimensions` test now sets `directions = {true, true, true}` (previously all-off, which tripped the earlier `-14601` check first and masked this path — see prior V&V revision). Run and confirmed locally: 3/3 assertions pass, error code is exactly `-14602`. Also note the boundary condition itself changed from `&&` to `\|\|` (any single dimension being 0 is now sufficient to trigger the error, not just all three) — both the test fix and the condition fix landed together in this branch's `Fixed filter preflight errors` / `Fixing ErodeDilateBadData` commits. |

**Per-direction coverage, confirmed by instrumentation this pass:** `Algorithms/ErodeDilateBadData.cpp` was temporarily instrumented with hit counters per face direction (`-Z/-Y/-X/+X/+Y/+Z`) at (a) the point immediately after the `isValidFaceNeighbor` gate in the vote/mark loop, (b) the point where the Dilate mark / Erode vote condition (`feature > 0`) actually fires, and (c) the equivalent point in the Erode cleanup loop. Running the full `(Erode) Expanded` + `(Dilate) Expanded` sweep (28 GENERATE runs) produced non-zero counts for **every one of the 6 directions at every one of those 3 measurement points** — e.g. vote/mark loop reached counts were `-Z=38 -Y=111 -X=108 +X=106 +Y=64 +Z=97`, and the `feature>0` condition fired for Dilate marking at `-Z=9 -Y=46 -X=37 +X=35 +Y=16 +Z=44` and for Erode voting at `-Z=8 -Y=25 -X=24 +X=24 +Y=8 +Z=32`. The instrumentation was removed after confirming this (not shipped in the reverted-to-clean algorithm file); this row records the empirical result, not a standing code artifact.

Additional confirmed items, not path gaps but worth recording:

- **Cancel path exists but is untested.** Corrected from the prior V&V pass, which found no cancel check present at the time. As of `56b30c923` ("Fixing ErodeDilateBadData"), `operator()` now reads `m_ShouldCancel` once per Z-slice (`Algorithms/ErodeDilateBadData.cpp:144-148`, inside the outer `for(zIdx...)` loop, itself inside the `for(iteration...)` loop) and returns immediately if set. This is a real, functional early-exit — checked on every Z-slice of every iteration, not just once — but no current test sets `m_ShouldCancel` and asserts early termination, so this path is present in the code and reachable, but not exercised by any `TEST_CASE`. Not counted in the 9-path table above (that table scopes to `preflightImpl`/vote-transfer branches); worth considering as a 10th path if the table's scope is later widened. Legacy's equivalent loop has no cancel check at all — SIMPLNX is ahead of legacy here, not behind; not a deviation.
- **Direction masking, fixed.** Previously flagged: "`adjustValidNeighbors` bitwise-ANDs the face-index constants themselves... flagged for second-engineer scrutiny." This is now resolved — see Bug Fixes / `ErodeDilateBadDataFilter-B1` in the deviations doc. The function has been rewritten and is confirmed exercised across all 6 directions (this section, above) and legacy-verified across all 28 combinations (Oracle section).

## Test inventory

| Test case | Notes |
|-----------|-------|
| `SimplnxCore::ErodeDilateBadDataFilter(Erode)` | Exemplar-archive-based smoke test (`6_6_erode_dilate_test.tar.gz`), all directions on, 2 iterations. Passes. |
| `SimplnxCore::ErodeDilateBadDataFilter(Erode) Expanded` | Class 2 oracle (see Oracle). `GENERATE` over 7 valid direction combinations × 2 iteration counts (14 runs). Compares both `FeatureIds` and `Misc` against exemplar arrays. Passes. |
| `SimplnxCore::ErodeDilateBadDataFilter(Dilate) Expanded` | Same sweep, Dilate operation, both arrays asserted. Passes. |
| `SimplnxCore::ErodeDilateBadDataFilter Ignored Path` | Confirms an array listed in `IgnoredDataArrayPaths` (`Misc`) is left untouched. Preflights **and executes** the filter, then asserts both that `Misc` still equals the fixture values and that `FeatureIds` did change — the second assertion is what keeps the first from passing vacuously (in the prior revision this test never called `execute()`, so it could not distinguish "ignored" from "filter never ran"; verified by mutation — emptying the ignore list, or dropping the `execute()` call, each now fails the test). Passes. |
| `SimplnxCore::ErodeDilateBadDataFilter No Dimensions` | Preflight-error test: `ImageGeom` dimensions forced to `{0,0,0}`, directions all **on**. Asserts `preflightResult.outputActions.invalid()` and `errors()[0].code == -14602`. Correctly named and covers the intended zero-dimensions path (previously it also zeroed all direction flags, which tripped the earlier `-14601` check first — now fixed). |
| `SimplnxCore::ErodeDilateBadDataFilter No Direction` | Preflight-error test: all directions off, geometry otherwise valid. Asserts `-14601`. Correctly named and covers the intended path. |
| `SimplnxCore::ErodeDilateBadDataFilter: SIMPL Backwards Compatibility` | `DYNAMIC_SECTION` over `simpl_conversion/6_5/ErodeDilateBadDataFilter.json` (matched by `Filter_Uuid`) and `simpl_conversion/6_4/ErodeDilateBadDataFilter.json` (matched by `Filter_Name`, no UUID field present in that fixture). Loads each legacy pipeline JSON via `Pipeline::FromSIMPLFile`, confirms it resolves to a single `PipelineFilter` with `FilterTraits<ErodeDilateBadDataFilter>::uuid`, and checks the converted arguments: `Operation == k_Dilate` (legacy `Direction: 0` round-trips to SIMPLNX's own `Dilate = 0`), `NumIterations == 5`, `XDirOn/YDirOn/ZDirOn == true`, geometry path `DataPath({"DataContainer"})`, feature-ids path `DataPath({"DataContainer","CellData","TestArray"})`. `IgnoredDataArrayPaths` verified only by successful pipeline load, not by value, matching the pattern used in `FillBadDataTest.cpp`. Passes. |

## Deviations from DREAM3D 6.5.171

See [`deviations/ErodeDilateBadDataFilter.md`](deviations/ErodeDilateBadDataFilter.md) — one confirmed and fixed SIMPLNX-side bug (`ErodeDilateBadDataFilter-B1`, direction parameters had no effect), one investigated-and-disproven hypothesis (Dilate tie-break order — legacy matches SIMPLNX's original behavior), and no confirmed legacy deviations. Legacy binary/pipeline comparison has now been run (28/28 matches) — no longer a gap.
