# V&V Report: WriteDREAM3DFilter

|                             |                                                                             |
|-----------------------------|-----------------------------------------------------------------------------|
| Plugin                      | SimplnxCore                                                                |
| SIMPLNX UUID                | `b3a95784-2ced-41ec-8d3d-0242ac130003`                                     |
| SIMPLNX Human Name          | Write DREAM3D-NX File                                                      |
| DREAM3D 6.5.171 equivalent  | `DataContainerWriter` — SIMPL UUID `3fcd4c43-9d75-5b86-aad4-4441bc914f37`  |
| Verified commit             | *<filled at SBIR deliverable assembly>*                                   |
| Status                      | **READY FOR REVIEW** (second-engineer review outstanding — see V&V phase) |
| Sign-off                    | *pending second-engineer review*                                           |

## At a glance

| Aspect                 | Current state |
|------------------------|---------------|
| Algorithm Relationship | **Rewrite** — same UUID/role as legacy `DataContainerWriter`, but the on-disk format is entirely new (v8 `DataStructure` HDF5 layout + `AtomicFile` atomic-write + optional gzip compression), not a translation of the legacy writer's code. |
| Oracle (confirmed)     | **Class 1 (Analytical)** — expected content is the hand-built in-memory `DataStructure`/`Pipeline` the test itself constructed; expected HDF5 physical layout (contiguous vs. chunked+deflate) is a closed-form function of array byte-size and the two compression parameters. 17 fixtures across `DREAM3DFileTest.cpp`, all pass. |
| Code paths enumerated  | **14 of 19** exercised; 5 gaps are defensive/unreachable-via-public-API guards (see table). |
| Tests today            | **17 TEST_CASEs** (some with `GENERATE`/`DYNAMIC_SECTION` multiplying cases) — preflight validation, full round-trip content fidelity across every geometry/DataObject type, SIMPL args backward-compat, and a 6-test compression sub-suite (layout, bypass threshold, level monotonicity). |
| Exemplar archive       | **None.** Every test builds its `DataStructure` inline in C++ and round-trips it through `WriteFile`/`ReadFile` in the same run — no cached `.tar.gz` golden file is used or needed for a Class 1 oracle. |
| Legacy comparison      | **Not run — and not applicable.** The two writers target deliberately different on-disk contracts, so a byte/dataset-level A/B against 6.5.171 `DataContainerWriter` output would be 100% noise by design, not signal. `ReadDREAM3DFilter` is the only tool in either codebase that understands both formats; fidelity is instead verified independently via round-trip Class 1 tests. |
| Bug flags              | None. |
| V&V phase              | Oracle chosen, code paths enumerated, test inventory reviewed, deviations documented. Outstanding: second-engineer review of the oracle design, of the 5 uncovered defensive paths, and of the `DynamicListArray` IO gap (Known limitations). |

## Summary

`WriteDREAM3DFilter` serializes the current `DataStructure` (and, when run inside a pipeline, the preceding `Pipeline`) to an HDF5 `.dream3d` file, with an optional companion `.xdmf` sidecar and optional gzip compression of array datasets. It replaces legacy SIMPL's `DataContainerWriter` under the same conceptual role but with an intentionally new v8 file format, so verification is independent of 6.5.171: correctness is established by writing hand-built `DataStructure`s covering every geometry and `DataObject` type, then reading them back and asserting exact structural/content equality (Class 1 Analytical), plus closed-form assertions on the resulting HDF5 physical layout under each compression setting. All 17 test cases pass; no bugs were found. `StatsDataArray`/`StructArray` (SIMPL's per-ensemble statistics types) are out of scope for this cycle — those `DataObject` types do not yet exist in this branch of simplnx (see deviation D2). Separately, a bare `DynamicListArray` (as opposed to its `NeighborList` specialization) has no HDF5 IO factory at all in the current codebase and cannot be written by this or any filter (see Known limitations).

## Algorithm Relationship

*Classification:* **Rewrite** ~~| Port | Minor changes | New filter~~

`WriteDREAM3DFilter` keeps the SIMPL UUID mapping (`3fcd4c43-9d75-5b86-aad4-4441bc914f37` → `WriteDREAM3DFilter`, `SimplnxCoreLegacyUUIDMapping.hpp:170`) and the legacy `DataContainerWriter` role, but the algorithm (`Algorithms/WriteDREAM3D.cpp`, 82 lines) was designed from the start for the current v8 `DataStructure` HDF5 layout, `AtomicFile`-based atomic writes, and (as of PR #1606) optional gzip compression — none of which exist in the legacy 6.5.171 writer. This has never been a line-by-line port of the legacy C++; the file format itself is a clean-sheet design (`k_CurrentFileVersion = "8.0"` vs. legacy's `"7.0"`/`DataContainers` group tag, see `Dream3dIO.hpp:31`).

*Evidence:* `parametersVersion()` is at 2 (compression parameters added after the filter's initial release); `git log --follow` on the algorithm/filter files shows the write path has been restructured multiple times since inception (out-of-core support #1253, atomic-file rework #900, algorithm-class extraction #1544) without ever tracking legacy DataContainerWriter's implementation.

*Port-time deltas (SIMPL → SIMPLNX argument mapping, `WriteDREAM3DFilter::FromSIMPLJson`):*

1. `OutputFile` → `export_file_path`, `WriteXdmfFile` → `write_xdmf_file`: direct 1:1 mapping, no behavior change.
2. SIMPL's "Write Time Series" parameter has no SIMPLNX equivalent (dropped) — legacy time-series writing is not part of this filter's scope in NX; not a regression since no NX pipeline concept maps to it.
3. `use_compression` is force-overridden to `false` for any pipeline converted from SIMPL JSON (`WriteDREAM3DFilter.cpp:130`), even though SIMPLNX's own default is `true`. This is deliberate: SIMPL v6 pipelines never wrote compressed files, so a converted pipeline preserves the exact on-disk encoding it shipped with rather than silently changing file size/behavior on re-run.

*Material PRs since baseline:* #1606 (added HDF5 compression parameters/behavior), #1544 (moved `executeImpl` logic into the `WriteDREAM3D` algorithm class, no behavior change), #1253 (out-of-core support).

## Oracle

*Class:* **1 (Analytical)**, primary. `Compression_LevelsRoundTrip` also carries a **Class 4 (Invariant)** companion check (file size must be non-increasing as gzip level rises) alongside its per-level content round-trip.

*Applied:* Every test constructs its expected answer directly, without ever running the filter to "produce" the expected value:

- **Content fidelity:** each test builds a `DataStructure` in C++ (`CreateTestDataStructure()`, or an inline array/geometry), writes it, reads it back, and asserts the read-back content equals what was built — by construction, not by comparison to a previously-captured file. `CheckTestDataStructure()` walks every `DataObject` kind the filter must support (nested `DataGroup`s, `AttributeMatrix`, `NeighborList`, `StringArray`, and all seven geometry types: Vertex/Edge/Triangle/Quad/Tetrahedral/Hexahedral/Image) and asserts exact values against the hand-known fill pattern from `FillDataStore()`.
- **HDF5 physical layout:** the filter's documented compression policy (`docs/WriteDREAM3DFilter.md`) states arrays under 16 KiB always stay contiguous/uncompressed regardless of settings, and any larger array is chunked+deflated at the requested level when compression is enabled. This is a closed-form predicate on `(array byte size, UseCompression, CompressionLevel)` — tests assert it directly via `UnitTest::ProbeHdf5Dataset` rather than trusting the filter's own claim about what it wrote.
- **Pipeline embedding:** when run inside an actual `Pipeline`, the written file's embedded pipeline JSON must reproduce the exact filter sequence and count that was executed — asserted against the hand-built pipeline (`pipeline.size() == 3`, filter names checked by index).

**Encoded tests:** `src/Plugins/SimplnxCore/test/DREAM3DFileTest.cpp` (17 `TEST_CASE`s touching Write, several parameterized via `GENERATE`/`DYNAMIC_SECTION`) — all pass. See Test inventory below for the full list.

*Second-engineer review:* Outstanding. Recommended focus: the Class 1 boundary-of-scope claim in deviation D2 (StatsDataArray/StructArray) and the 5 uncovered defensive paths (Code path coverage below).

## Code path coverage

**14 of 19** paths exercised. The 5 gaps are all defensive guards that require conditions unreachable through the public filter/pipeline API (invalid destination mid-write after preflight already validated it, or a detached `PipelineFilter`) rather than genuine untested behavior.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/WriteDREAM3D.cpp` (82 lines). Preflight guards below live in the sibling `Filters/WriteDREAM3DFilter.cpp` (`preflightImpl`), which the policy still treats as in-scope algorithm surface (parameter validation gates that the Algorithm class depends on).

| #  | Phase              | Path                                                                                        | Test case |
|----|--------------------|----------------------------------------------------------------------------------------------|-----------|
| 1  | Preflight          | `export_file_path` empty → error `-1`                                                        | `"WriteDREAM3DFilter:Invalid Parameters"` § `Empty FilePath` |
| 2  | Preflight          | `use_compression=true`, `compression_level < 1` → error `-2`                                  | `"WriteDREAM3DFilter:Invalid Parameters"` § `Bad Compression Level`; `"WriteDREAM3DFilter: Compression_Preflight_RejectsOutOfRangeLevel"` (level=0) |
| 3  | Preflight          | `use_compression=true`, `compression_level > 9` → error `-2`                                  | `"WriteDREAM3DFilter: Compression_Preflight_RejectsOutOfRangeLevel"` (level=10) |
| 4  | Preflight          | `use_compression=false` → `compression_level` ignored even if out of `[1,9]`                  | `"WriteDREAM3DFilter: Compression_Preflight_RejectsOutOfRangeLevel"` (level=0, compression off) |
| 5  | Preflight          | Valid parameters → success                                                                   | `"WriteDREAM3DFilter:Valid Parameters"`; implicitly, every passing execute-path test below |
| 6  | Execute — setup    | `AtomicFile::Create` fails (unwritable/invalid destination directory)                        | *Not directly tested.* Preflight only rejects an empty path; a directory-permission failure at execute time would require a filesystem fixture (e.g., a read-only directory) not set up by the current suite. |
| 7  | Execute — setup    | `AtomicFile::Create` succeeds                                                                 | Every passing execute-path test |
| 8  | Execute — pipeline | `PipelineNode != nullptr` and `getPrecedingPipeline()` returns `nullptr` → error `-15`         | *Not directly tested.* Only reachable if a `PipelineFilter` is detached from its parent `Pipeline`, which normal `filter.execute()`/`pipeline.execute()` usage never produces. |
| 9  | Execute — pipeline | `PipelineNode != nullptr`, preceding pipeline retrieved successfully → embedded in file        | `"DREAM3DFileTest:Import/Export DREAM3D Filter Test"` (`exportPipeline.execute()`); `"DREAM3DFileTest:Import/Export Multi-DREAM3D Filter Test"` (`CreateMultiExportFiles()`) |
| 10 | Execute — pipeline | `PipelineNode == nullptr` → empty pipeline written                                            | `"WriteDREAM3DFilter:Valid Parameters"`, `"DREAM3DFileTest::StringArray"`, all `Compression_*` tests (all call `filter.execute(ds, args)` directly) |
| 11 | Execute — options  | `use_compression=true` → `writeOptions.compressionLevel = CompressionLevel`                   | `"...Compression_On_IsChunkedAndDeflated"`, `"...Compression_SmallArray_Bypasses"`, `"...Compression_LevelsRoundTrip"` |
| 12 | Execute — options  | `use_compression=false` → `writeOptions.compressionLevel = 0`                                 | `"...Compression_Off_IsContiguous"`; `"WriteDREAM3DFilter:Valid Parameters"` |
| 13 | Execute — write    | `DREAM3D::WriteFile(...)` returns invalid → skip commit, return the error                     | *Not directly tested* through the full filter/`AtomicFile` path, but concretely reachable (not merely defensive): `HDF5::DataStructureWriter::WriteFile` returns error `-5` ("Could not find IO factory for datatype: …") for any `DataObject` type with no registered HDF5 IO factory — see Known limitations below (a bare `DynamicListArray`, not wrapped as `NeighborList`). No test currently puts such an object in the `DataStructure` before writing. |
| 14 | Execute — write    | `DREAM3D::WriteFile(...)` returns valid → proceed to commit                                   | Every passing execute-path test |
| 15 | Execute — commit   | `atomicFile.commit()` fails (rename onto final destination fails)                             | *Not directly tested.* Would require the destination path to become invalid between `AtomicFile::Create` and `commit()` (e.g., concurrent deletion of the parent directory) — a race not exercised by the suite. |
| 16 | Execute — commit   | `atomicFile.commit()` succeeds                                                                | Every passing execute-path test (the output file is present and re-readable in every round-trip test) |
| 17 | Execute — xdmf     | `write_xdmf_file=true` → rename temp `.xdmf` into place, succeeds                              | `"DREAM3DFileTest:DREAM3D File IO Test"` (writeXdmf=true), `CreateExportPipeline()`/`CreateMultiExportFiles()` (`write_xdmf_file=true`) |
| 18 | Execute — xdmf     | `write_xdmf_file=true`, rename fails → `MakeErrorResult` with system error message             | *Not directly tested.* Would require the `.xdmf` destination to become unwritable between the HDF5 write succeeding and the rename — not portably reproducible in the current suite. |
| 19 | Execute — xdmf     | `write_xdmf_file=false` → skip rename, return `WriteFile`'s result directly                   | Most `Compression_*` tests, `"DREAM3DFileTest::StringArray"`, `"WriteDREAM3DFilter:Valid Parameters"` |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `WriteDREAM3DFilter:Invalid Parameters` (§ Empty FilePath, § Bad Compression Level) | kept | Preflight-only, inline `DataStructure`. Covers Paths 1, 2. |
| `WriteDREAM3DFilter:Valid Parameters` | kept | Preflight + full execute, inline `DataStructure`, compression off. Covers Paths 5, 7, 10, 12, 14, 16, 19. |
| `WriteDREAM3D:Pipeline / WriteXdmf combinations` | kept | Calls `DREAM3D::WriteFile(path, ds, pipeline, writeXdmf)` directly (bypasses `AtomicFile`/the Algorithm class) across `writeXdmf × {real pipeline, empty pipeline}` (4 cases). Exercises the shared write utility the Algorithm depends on, not the Algorithm's own guards. |
| `WriteDREAM3D:Invalid File` | kept | Same free-function call with an empty path (4 cases via `GENERATE`); asserts the underlying `HDF5::FileIO::WriteFile` failure is surfaced, not `AtomicFile`'s. |
| `DREAM3DFileTest:DREAM3D File IO Test` | kept | The primary Class 1 content-fidelity test. Builds every geometry/`DataObject` type (`CreateTestDataStructure`), writes + reads back (`writeXdmf ∈ {true,false}`), and asserts full structural/content equality (`CheckTestDataStructure`) plus pipeline round-trip (`pipeline.size()==3`, filter names by index). |
| `DREAM3DFileTest::StringArray` | kept | Round-trips a `StringArray` through the actual `WriteDREAM3DFilter`/`ReadDREAM3DFilter` classes (not the free function) — covers Path 10 with real filter execution. |
| `DREAM3DFileTest:Import/Export DREAM3D Filter Test` | kept | Executes `WriteDREAM3DFilter` inside a real `Pipeline` (`exportPipeline.execute()`) — the only test exercising Path 9 (non-null `PipelineNode` with a real preceding pipeline). Also checks preflight-imported vs. executed array store types on the read side. |
| `DREAM3DFileTest:Import/Export Multi-DREAM3D Filter Test` | kept | Two independent export pipelines (`CreateMultiExportFiles`) each executing `WriteDREAM3DFilter` in-pipeline, then a single import pipeline consuming both files. Second confirmation of Path 9. |
| `DREAM3DFileTest: Preflight imports geometry connectivity as metadata-only stores` | kept | Read-side only (consumes a pre-existing `geoms.dream3d` asset); does not exercise `WriteDREAM3DFilter`. Listed for completeness since it shares the tag set. |
| `SimplnxCore::WriteDREAM3DFilter: SIMPL Backwards Compatibility` | kept | `DYNAMIC_SECTION` over SIMPL 6.5 (UUID-keyed) and 6.4 (name-keyed) `DataContainerWriter` fixtures. Asserts UUID resolution, empty comments, and `export_file_path`/`write_xdmf_file` value conversion. Not an oracle test — argument-mapping check only. |
| `DREAM3DFileTest: DataArray datasets are chunked+deflated when WriteOptions requests it` | kept | Calls the `WriteOptions`-aware free function directly with a 2 MB array; asserts chunked+deflate layout at level 5 via `ProbeHdf5Dataset`, then round-trips content. Covers the compression-options contract the Algorithm class relies on. |
| `WriteDREAM3DFilter: Compression_Off_IsContiguous` | kept | Full filter execute, `use_compression=false`. Asserts contiguous/no-deflate layout. Covers Path 12. |
| `WriteDREAM3DFilter: Compression_On_IsChunkedAndDeflated` | kept | Full filter execute, `use_compression=true`, level 5, 500K-element array. Asserts chunked+deflate layout and round-trips content. Covers Path 11. |
| `WriteDREAM3DFilter: Compression_SmallArray_Bypasses` | kept | Full filter execute with one <16 KiB and one >16 KiB array in the same `DataStructure`, `use_compression=true`. Asserts the small array stays contiguous/uncompressed while the large one is chunked+deflated — the Class 1 closed-form threshold check. |
| `WriteDREAM3DFilter: Compression_LevelsRoundTrip` | kept | Full filter execute at levels {1,5,9} on the same 1M-element pattern. Round-trips content at each level and asserts non-increasing file size as level rises (Class 4 companion invariant). |
| `WriteDREAM3DFilter: Compression_Preflight_RejectsOutOfRangeLevel` | kept | Three sequential preflight-only checks: level=0 (invalid), level=10 (invalid), level=0 with compression off (valid — ignored). Covers Paths 2, 3, 4. |
| `DREAM3DFileTest: PreflightCache avoids re-reading unchanged files` | kept | Uses `DREAM3D::WriteFile` only to create read-side fixture files; does not exercise `WriteDREAM3DFilter`'s own behavior. Listed for completeness since it shares source-file/tag space. |

## Exemplar archive

None. Every test above builds its input `DataStructure` inline in C++ and never loads a downloaded `.tar.gz` exemplar — the Class 1 oracle's "expected output" is the test's own hand-built input, so no cached golden file is required or used. (`Small_IN100_dream3d_v3.tar.gz`, referenced elsewhere in this test file, backs unrelated `ReadDREAM3DFilter`-only test cases and is not consumed by any Write-side test.)

## Known limitations (current simplnx HDF5 IO layer)

`DynamicListArray` (the generic variable-length per-tuple list container that `NeighborList<T>` is itself built on) has **no registered HDF5 IO factory**. `HDF5::DataIOManager::addCoreFactories()` (`DataStructure/IO/HDF5/DataIOManager.cpp:33-81`) registers factories per concrete `DataArray<T>` type, per `NeighborList<T>` specialization, geometries, `AttributeMatrix`, `DataGroup`, `StringArray`, and scalar attributes — but nothing for a bare `DynamicListArray<T>`. If one is ever placed directly in a `DataStructure` (outside of a `NeighborList` specialization) and written, `HDF5::DataStructureWriter::WriteFile` hits its "no factory found" guard (`DataStructureWriter.cpp:153-157`) and fails with error `-5` ("Could not find IO factory for datatype: …"), surfacing through `WriteDREAM3DFilter` as Path 13 above.

This is a gap in the shared HDF5 IO layer, not something specific to `WriteDREAM3DFilter`'s own algorithm — the same gap would affect `ReadDREAM3DFilter` for the same object type. It is not raised as a formal Deviation because there is no confirmed 6.5.171 pipeline behavior being compared against (unlike D2, which names concrete legacy types); it is recorded here as a known, currently-untested capability boundary of what this filter can serialize. No current `WriteDREAM3DFilter` test constructs a bare `DynamicListArray`, so Path 13 remains untested rather than confirmed-safe.

## Deviations from DREAM3D 6.5.171

No byte/dataset-level A/B was run against 6.5.171 `DataContainerWriter` output, and none is warranted: the two writers target deliberately different on-disk contracts (see D1), so such a diff would be 100% noise by design, not signal. `ReadDREAM3DFilter` is the only tool that understands both formats, and cross-format fidelity is its concern, not this filter's. Two scope/format deviations are documented instead:

- `WriteDREAM3DFilter-D1` — on-disk file format is a deliberate, complete rewrite (v8 `DataStructure` layout vs. legacy v7 `DataContainers` layout; optional gzip compression; atomic write) — see `vv/deviations/WriteDREAM3DFilter.md`.
- `WriteDREAM3DFilter-D2` — `StatsDataArray`/`StructArray` (SIMPL ensemble-statistics types) cannot currently be written because those `DataObject` types do not yet exist in this branch of simplnx — see `vv/deviations/WriteDREAM3DFilter.md`.

Neither is a bug: D1 is the intended outcome of the Rewrite classification (defended above), and D2 is a scope boundary pending a separate in-progress port of those data types.
