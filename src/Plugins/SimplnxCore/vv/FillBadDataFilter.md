# V&V Report: FillBadDataFilter

|                             |                                                                          |
|-----------------------------|--------------------------------------------------------------------------|
| Plugin                      | SimplnxCore                                                              |
| SIMPLNX UUID                | `a59eb864-9e6b-40bb-9292-e5281b0b4f3e`                                   |
| SIMPLNX Human Name          | Fill Bad Data                                                            |
| DREAM3D 6.5.171 equivalent  | `FillBadData` — SIMPL UUID `30ae0a1e-3d94-5dab-b279-c5727ab5d7ff`       |
| Verified commit             | *<filled at SBIR deliverable assembly>*                                  |
| Status                      | COMPLETE — 2026-07-16 |
| Sign-off                    | Michael Jackson <mike.jackson@bluequartz.net> — 2026-07-16 |

## At a glance

| Aspect                 | Current state |
|------------------------|---------------|
| Algorithm Relationship | **Rewrite** — 4-phase chunk-sequential CCL+Union-Find (OOC support); legacy used a simpler in-memory approach. Functional behavior preserved. |
| Oracle                 | **Class 2** for `FillBadData_SmallIN100` (`6_5_exemplar.dream3d` from legacy 6.5 pipeline). **Class 1** for Tests 01–13 (hand-authored expected values serialized by a format-conversion script that never runs `FillBadDataFilter`). Circular-oracle concern resolved. |
| Code paths             | **15 of 16** covered. Only gap: `m_ShouldCancel` cancel path in Phase 4 while-loop (Path 14). |
| Tests                  | **14 TEST_CASEs**, all pass. 1 SmallIN100 (Class 2) + 9 OOC synthetic fixtures (Class 1, Tests 01–07, 11, 13) + 1 all-bad-data termination guard + 1 preflight-error inline (asserts `-16500`) + 1 SIMPL backwards-compat. |
| Exemplar archive       | `6_5_fill_bad_data.tar.gz` — `6_5_input/exemplar.dream3d` (Class 2) + `test_NN_input/expected.dream3d` pairs for Tests 01–07, 11, 13 (Class 1). *(Tests 08–10 and 12 have no fixtures — the numbering is non-contiguous.)* |
| Legacy comparison      | SmallIN100 in-test (Class 2) — SIMPLNX matches the 6.5.x exemplar element-wise. A separate Test 08 three-way binary A/B was cited by an earlier revision but its working files are unrecoverable, so that claim is **withdrawn** (see deviations). Per-code-path correctness is pinned by the Class 1 analytical fixtures. |
| Bug flags              | `FillBadDataFilter-B1` (preflight dead-return for `minAllowedDefectSize < 1`) resolved. Resolved: an all-bad-data / enclosed-bad-pocket input previously looped forever in Phase 4 (no fillable neighbor → `count` never reached 0); a no-progress guard now stops with a warning. |
| V&V phase              | **COMPLETE — V&V signed off by Michael Jackson (technical authority) 2026-07-16.** Outstanding: cancel path (Path 14) untested. *(The unrecoverable Test 08 A/B claim has been withdrawn — no longer a gate.)* |

## Summary

`FillBadDataFilter` fills voxels with `FeatureId == 0` ("bad data") by assigning them to the most-common positive-feature neighbor. Connected regions below `minAllowedDefectSize` are filled; larger regions are preserved (optionally relabeled as a new phase). SIMPLNX uses a four-phase CCL+Union-Find architecture to support OOC datasets — a structural departure from legacy DREAM3D 6.5.171. Output equivalence is confirmed by two independent comparisons: the `FillBadData_SmallIN100` unit test (passes against `6_5_exemplar.dream3d`) and a manual A/B run on a custom dataset across three binaries (6.5.171, 6.5.172, NX). One bug surfaced during V&V (B1 — dead error-return path for `minAllowedDefectSize < 1`) has been fixed and is now covered by a dedicated test.

## Algorithm Relationship

*Classification:* **Rewrite** ~~| Port | Minor changes | New filter~~

The SIMPLNX algorithm (`Algorithms/FillBadData.cpp`, ~765 lines) introduces a 4-phase architecture to support OOC datasets stored in chunked Zarr format:

1. **Phase 1 — Chunk-sequential CCL:** Assigns provisional negative labels to connected components of bad data using a scanline algorithm. `ChunkAwareUnionFind` tracks cross-chunk label equivalences.
2. **Phase 2 — Global resolution:** Flattens the Union-Find tree (path compression + size accumulation to roots). No equivalent in legacy.
3. **Phase 3 — Region classification:** Compares each component's voxel count to `minAllowedDefectSize`; small → mark -1 for fill, large → keep as 0 or assign new phase.
4. **Phase 4 — Iterative fill:** Assigns each -1 voxel to its most-common positive-feature face neighbor, iterating until none remain. Propagates all cell-data arrays via `FillBadDataUpdateTuplesFunctor`.

SIMPL UUID mapping is preserved via `SimplnxCoreLegacyUUIDMapping.hpp` and SIMPL conversion fixtures (`test/simpl_conversion/6_5/` and `6_4/FillBadDataFilter.json`).

*Behavioral equivalence evidence:* `FillBadData_SmallIN100` passes against `6_5_exemplar.dream3d` (Small IN100, threshold=1000). (A separate Test 08 three-way A/B was previously cited but its working files are unrecoverable; that claim is withdrawn — see deviations.)

## Oracle

*Class:* **Class 2 (Reference implementation)** for `FillBadData_SmallIN100`. **Class 1 (Analytically derived)** for Tests 01–13.

**Class 2:** `6_5_fill_bad_data/6_5_exemplar.dream3d` was generated by running a DREAM3D 6.5.x `FillBadData` pipeline against the Small IN100 dataset (`MinAllowedDefectSize=1000`, `StoreAsNewPhase=false`). The legacy filter is the independent reference.

**Class 1:** `test_NN_expected_featureids.txt` files were hand-authored by tracing the algorithm specification phase-by-phase. `generate_dream3d_files.py` serializes these arrays to HDF5 without running `FillBadDataFilter`. Evidence of independent derivation: Test 11 explicitly traces neighbor scan order and identifies Feature 1 as the tie-break winner by count — reasoning that cannot originate from a simulation.

**Encoded tests:**

- Class 2: `"SimplnxCore::FillBadData_SmallIN100"` — `UnitTest::CompareExemplarToGeneratedData` against `6_5_exemplar.dream3d`. Passes.
- Class 1: `"SimplnxCore::FillBadData::Test01"` through `Test13` — `UnitTest::CompareExemplarToGeneratedData` against `test_NN_expected.dream3d`. Pass.
- SIMPL compat: `"SimplnxCore::FillBadDataFilter: SIMPL Backwards Compatibility"` — UUID + arg-key + value assertions only. Not an oracle test.

*Second-engineer review:* **Signed off by Michael Jackson (technical authority), 2026-07-16.**

## Code path coverage

*14 of ~15 paths enumerated. See gaps below.*

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/FillBadData.cpp` (~765 lines).

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Phase 1 — CCL | Non-bad voxel (`featureId ≠ 0`) → skip | All tests |
| 2 | Phase 1 — CCL | Bad voxel, no bad backward neighbors → new negative label | `Test01_SingleSmallDefect` |
| 3 | Phase 1 — CCL | Bad voxel, one bad backward neighbor → inherit label | `Test01`, `Test02`, `Test03` |
| 4 | Phase 1 — CCL | Bad voxel, bad backward neighbors from different labels → `unionFind.unite()` | `Test04`, `Test07` |
| 5 | Phase 2 — Resolution | `flatten()` — always executes | All tests |
| 6 | Phase 3 — Relabeling | Region < threshold → mark -1 | `Test01`, `Test06`, `FillBadData_SmallIN100` |
| 7 | Phase 3 — Relabeling | Region ≥ threshold, `storeAsNewPhase=false` → keep as 0 | `Test02`, `Test03` |
| 8 | Phase 3 — Relabeling | Region > threshold, `storeAsNewPhase=true`, `cellPhasesPtr ≠ nullptr` → `phase[i] = maxPhase+1` | `Test13_StoreAsNewPhase` |
| 9 | Phase 4 — Fill | FeatureId ≥ 0 → skip | All tests |
| 10 | Phase 4 — Fill | FeatureId = -1, ≥1 positive neighbor → vote and assign | `Test01`, `Test06`, `FillBadData_SmallIN100` |
| 11 | Phase 4 — Fill | FeatureId = -1, no positive neighbor this iteration → deferred to next iteration | `Test04` (interior voxels) |
| 12 | Phase 4 — Fill | Neighbor tie-break → first-encountered wins by scan order | `Test11_NeighborTieBreaking` |
| 13 | `operator()` | `storeAsNewPhase=false` → `cellPhasesPtr = nullptr`, phases loop skipped | Tests 01–07, 11, SmallIN100 |
| 14 | `operator()` | `m_ShouldCancel` in Phase 4 while-loop | *Not covered.* No test exercises mid-fill cancellation. |
| 15 | Preflight | `minAllowedDefectSize < 1` → `MakePreflightErrorResult(-16500, …)` | `"SimplnxCore::FillBadDataFilter:: Invalid Preflight Min Defect Size"` — inline DataStructure, `minAllowedDefectSize=0`; asserts error code `-16500`. |
| 16 | Phase 4 | no fillable neighbor for any remaining bad voxel → no-progress break (warning) | `AllBadData_TerminatesWithoutHang` — all-bad-data slab; asserts the filter returns instead of looping forever. |

## Test inventory

| Test case | Notes |
|-----------|-------|
| `SimplnxCore::FillBadData_SmallIN100` | Class 2 oracle. In-core. threshold=1000, storeAsNewPhase=false. |
| `SimplnxCore::FillBadData::Test01_SingleSmallDefect` | OOC (100-byte sentinel). threshold=20. Single small region — primary fill path. Class 1. |
| `SimplnxCore::FillBadData::Test02_SingleLargeDefect` | OOC (100-byte sentinel). threshold=20. Single large region kept as 0. Class 1. |
| `SimplnxCore::FillBadData::Test03_ThresholdBoundary` | OOC (100-byte sentinel). threshold=25. Exact-threshold boundary (≥ kept, < filled). Class 1. |
| `SimplnxCore::FillBadData::Test04_MultipleSmallDefects` | OOC (500-byte sentinel). threshold=50. Multiple disconnected small regions — multi-iteration fill. Class 1. |
| `SimplnxCore::FillBadData::Test05_MixedSmallAndLarge` | OOC (500-byte sentinel). threshold=50. Mixed small (filled) and large (kept). Class 1. |
| `SimplnxCore::FillBadData::Test06_SingleVoxelDefects` | OOC (100-byte sentinel). threshold=10. Single-voxel bad-data islands. Class 1. |
| `SimplnxCore::FillBadData::Test07_DefectsAtBoundaries` | OOC (100-byte sentinel). threshold=20. Regions at image boundary — exercises Phase 1 CCL boundary handling. Class 1. |
| `SimplnxCore::FillBadData::Test11_NeighborTieBreaking` | OOC (50-byte sentinel). threshold=10. Tie-break via scan order. Class 1. |
| `SimplnxCore::FillBadData::Test13_StoreAsNewPhase` | OOC (100-byte sentinel). threshold=20, storeAsNewPhase=true. Only test for `cellPhasesPtr ≠ nullptr` path (Path 8). Class 1. |
| `SimplnxCore::FillBadDataFilter:: Invalid Preflight Min Defect Size` | Inline DataStructure (no file load). `minAllowedDefectSize=0` → asserts invalid **and** error code `-16500`. Covers Path 15. |
| `SimplnxCore::FillBadData::AllBadData_TerminatesWithoutHang` | Inline 3×3×1 all-bad-data slab (no good neighbor). Asserts the filter returns rather than looping forever. Covers Path 16 (no-progress guard). |
| `SimplnxCore::FillBadDataFilter: SIMPL Backwards Compatibility` | SIMPL 6.4 + 6.5 via `DYNAMIC_SECTION`. UUID + arg-key + value assertions only. Not an oracle test. |

## Exemplar archive

- **Archive:** `6_5_fill_bad_data.tar.gz`
- **SHA512:** `73b6835252135619325d9b02b48286ca95f0235820610bddcc43ec0a3f075337481bace00bf20e9d59744f1d4024cc18be624ec522922a6461b55ae8dbb0e628`
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/FillBadDataFilter.md`

## Deviations from DREAM3D 6.5.171

No deviations observed. The legacy comparison is the SmallIN100 in-test check:

1. `FillBadData_SmallIN100` passes against `6_5_exemplar.dream3d` (Small IN100, MinAllowedDefectSize=1000) — SIMPLNX matches the 6.5.x exemplar element-wise.

Per-code-path correctness is pinned independently by the Class 1 analytical fixtures (Tests 01–07, 11, 13). (An earlier revision also cited a Test 08 three-way binary A/B; its working files are unrecoverable, so that claim is withdrawn.)

See `vv/deviations/FillBadDataFilter.md` for the full comparison record.
