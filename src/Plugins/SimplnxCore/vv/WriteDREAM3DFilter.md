# V&V Report: WriteDREAM3DFilter

|                             |                                                                             |
|-----------------------------|-----------------------------------------------------------------------------|
| Plugin                      | SimplnxCore                                                                |
| SIMPLNX UUID                | `b3a95784-2ced-41ec-8d3d-0242ac130003`                                     |
| SIMPLNX Human Name          | Write DREAM3D-NX File                                                      |
| DREAM3D 6.5.171 equivalent  | `DataContainerWriter` — SIMPL UUID `3fcd4c43-9d75-5b86-aad4-4441bc914f37`  |
| Verified commit             | *<filled at SBIR deliverable assembly>*                                   |
| Status                      | COMPLETE — 2026-08-20                                                      |
| Sign-off                    | Matthew Marine (V&V author, PR #1683). Second engineer: Michael A. Jackson <mike.jackson@bluequartz.net>, 2026-08-20 (PR #1683 review). |

## At a glance

| Aspect                 | Current state |
|------------------------|---------------|
| Algorithm Relationship | **Rewrite** — same UUID/role as legacy `DataContainerWriter`, but the on-disk format is entirely new (v8 `DataStructure` HDF5 layout + `AtomicFile` atomic-write + optional gzip compression), not a translation of the legacy writer's code. |
| Oracle (confirmed)     | **Class 1 (Analytical)** — expected content is the hand-built in-memory `DataStructure`/`Pipeline` the test itself constructed; expected HDF5 physical layout (contiguous vs. chunked+deflate) is a closed-form function of array byte-size and the two compression parameters. 20 Write-related fixtures across `DREAM3DFileTest.cpp`, all pass. |
| Code paths enumerated  | **15 of 19** exercised; the 4 remaining gaps are defensive/unreachable-via-public-API guards (see table). |
| Tests today            | **20 Write-related `TEST_CASE`s of the 23 in `DREAM3DFileTest.cpp` / 30 ctest entries**, all passing (some with `GENERATE`/`DYNAMIC_SECTION` multiplying cases) — preflight validation, full round-trip content fidelity across every geometry/DataObject type, SIMPL args backward-compat, and a 6-test compression sub-suite (layout, bypass threshold, level monotonicity). |
| Exemplar archive       | **None.** Every test builds its `DataStructure` inline in C++ and round-trips it through `WriteFile`/`ReadFile` in the same run — no cached `.tar.gz` golden file is used or needed for a Class 1 oracle. |
| Legacy comparison      | **Not run — and not applicable.** The two writers target deliberately different on-disk contracts, so a byte/dataset-level A/B against 6.5.171 `DataContainerWriter` output would be 100% noise by design, not signal. `ReadDREAM3DFilter` is the only tool in either codebase that understands both formats; fidelity is instead verified independently via round-trip Class 1 tests. |
| Bug flags              | One, since resolved: `WriteXdmfNodeGeometry1D/2D/3D` (`Dream3dIO.cpp`) forwarded to the next-lower writer with `geomName` and `hdf5FilePath` transposed (both `std::string_view`, so it compiled silently), producing `.xdmf` node-attribute references that ParaView/VisIt could not resolve. Fixed alongside a content-level `.xdmf` oracle (`CheckXdmfFile`) that would have caught it. |
| V&V phase              | Discovery, algorithm relationship, oracle design, code-path enumeration, test inventory, deviations, and the bug fixes found along the way — **complete**. Second-engineer review of the oracle design, the 4 uncovered defensive paths, and the `DynamicListArray`/`GridMontage` serialization boundaries **signed off by Michael A. Jackson, 2026-08-20** (PR #1683). No legacy A/B is applicable (see Legacy comparison). Montage support remains an open design question, explicitly out of scope for this cycle and recorded as a capability boundary rather than a defect. **Nothing outstanding.** |

## Summary

`WriteDREAM3DFilter` serializes the current `DataStructure` (and, when run inside a pipeline, the preceding `Pipeline`) to an HDF5 `.dream3d` file, with an optional companion `.xdmf` sidecar and optional gzip compression of array datasets. It replaces legacy SIMPL's `DataContainerWriter` under the same conceptual role but with an intentionally new v8 file format, so verification is independent of 6.5.171: correctness is established by writing hand-built `DataStructure`s covering every geometry and `DataObject` type, then reading them back and asserting exact structural/content equality (Class 1 Analytical), plus closed-form assertions on the resulting HDF5 physical layout under each compression setting. All 20 test cases pass. One defect is resolved: the `.xdmf` node-geometry writers in the shared `Dream3dIO` utility transposed the geometry name and HDF5 file path when forwarding between levels, producing sidecar attribute references ParaView could not resolve (see Bug flags above). `StatsDataArray`/`StructArray` (SIMPL's per-ensemble statistics types) are out of scope for this cycle — those `DataObject` types do not yet exist in this branch of simplnx (see deviation D2). Separately, two `DataObject` types have no registered HDF5 IO factory and therefore cannot be written by this or any filter: a bare `DynamicListArray` (as opposed to its `NeighborList` specialization), and `GridMontage` — whose IO class exists but is never registered even though `CreateGridMontageAction` is public core API. Both are recorded under Code path coverage as observations about current behavior; neither is presented as a defect with a known fix, and montage support in particular is an unsettled design question this report takes no position on. The resulting write-failure contract is pinned by a dedicated test.

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

- **Content fidelity:** each test builds a `DataStructure` in C++ (`CreateTestDataStructure()`, or an inline array/geometry), writes it, reads it back, and asserts the read-back content equals what was built — by construction, not by comparison to a previously-captured file. `CheckTestDataStructure()` walks every `DataObject` kind the filter must support (nested `DataGroup`s, `AttributeMatrix`, `NeighborList`, `StringArray`, and all eight geometry types: Vertex/Edge/Triangle/Quad/Tetrahedral/Hexahedral/Image/RectGrid) and asserts exact values against the hand-known fill pattern from `FillDataStore()`.
- **Xdmf sidecar:** `CheckXdmfFile()` asserts the sidecar exists and that every heavy-data reference in it (`<file>:/DataStructure/<path>` DataItems) resolves to the `.dream3d` file that was written — a content-level check, not an existence-only check. The test fixture includes a vertex `AttributeMatrix` on a node geometry so node-centered attribute references are actually emitted and validated.
- **HDF5 physical layout:** the filter's documented compression policy (`docs/WriteDREAM3DFilter.md`) states arrays under 16 KiB always stay contiguous/uncompressed regardless of settings, and any larger array is chunked+deflated at the requested level when compression is enabled. This is a closed-form predicate on `(array byte size, UseCompression, CompressionLevel)` — tests assert it directly via `UnitTest::ProbeHdf5Dataset` rather than trusting the filter's own claim about what it wrote.
- **Pipeline embedding:** when run inside an actual `Pipeline`, the written file's embedded pipeline JSON must reproduce the exact filter sequence and count that was executed — asserted against the hand-built pipeline (`pipeline.size() == 3`, filter names checked by index).

**Encoded tests:** `src/Plugins/SimplnxCore/test/DREAM3DFileTest.cpp` (20 of the file's 23 `TEST_CASE`s touch Write, several parameterized via `GENERATE`/`DYNAMIC_SECTION`) — all pass. See Test inventory below for the full list and for the three `ReadDREAM3DFilter`-only cases that are deliberately out of scope.

*Second-engineer review:* **Signed off by Michael A. Jackson <mike.jackson@bluequartz.net>, 2026-08-20** (PR #1683 review, per sign-off convention). The V&V work was authored by Matthew Marine, so the review is independent of the author. Reviewed across three passes (2026-07-23, 2026-08-12, and this closing pass):

- **Oracle design.** Confirmed Class 1 is correct and non-circular: every fixture builds its own `DataStructure` in C++ and asserts the read-back against the hand-known fill pattern, so no previously-captured file is ever the source of truth. The HDF5 physical-layout assertions are a genuine closed-form predicate on `(byte size, UseCompression, CompressionLevel)` checked via `ProbeHdf5Dataset`, not a restatement of what the filter reported writing.
- **Class 4 companion.** The file-size monotonicity invariant in `Compression_LevelsRoundTrip` is a legitimate invariant check, not a substitute for the per-level content round-trip that runs alongside it.
- **Scope boundary (D2).** Confirmed by inspection that `StatsDataArray`/`StructArray` do not exist in this branch, so the exclusion is a real scope boundary rather than an untested path.
- **The 4 uncovered defensive paths.** Each was independently confirmed unreachable through the public API, and each appears as its own row in the code-path table rather than being omitted.
- **Bugs found during review.** Three transposed `std::string_view` arguments in `WriteXdmfNodeGeometry1D/2D/3D` and a wrong-`Result` test in `DREAM3D::ReadFile` are resolved; both are recorded under Bug flags with the tests that now pin them.

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
| 13 | Execute — write    | `DREAM3D::WriteFile(...)` returns invalid → skip commit, return the error                     | `"WriteDREAM3DFilter:Unwritable DataObject Type"` — writes a `DataStructure` holding a `GridMontage`, a type with no registered HDF5 IO factory. Asserts preflight still succeeds, execute fails with exactly one error of code `-5` ("Could not find IO factory for datatype: …"), and the destination file does **not** exist afterward (i.e. the `AtomicFile` was never committed). See the capability-boundary note at the end of this section. |
| 14 | Execute — write    | `DREAM3D::WriteFile(...)` returns valid → proceed to commit                                   | Every passing execute-path test |
| 15 | Execute — commit   | `atomicFile.commit()` fails (rename onto final destination fails)                             | *Not directly tested.* Would require the destination path to become invalid between `AtomicFile::Create` and `commit()` (e.g., concurrent deletion of the parent directory) — a race not exercised by the suite. |
| 16 | Execute — commit   | `atomicFile.commit()` succeeds                                                                | Every passing execute-path test (the output file is present and re-readable in every round-trip test) |
| 17 | Execute — xdmf     | `write_xdmf_file=true` → rename temp `.xdmf` into place, succeeds                              | `"DREAM3DFileTest:DREAM3D File IO Test"` (writeXdmf=true), `CreateExportPipeline()`/`CreateMultiExportFiles()` (`write_xdmf_file=true`) |
| 18 | Execute — xdmf     | `write_xdmf_file=true`, rename fails → `MakeErrorResult` with system error message             | *Not directly tested.* Would require the `.xdmf` destination to become unwritable between the HDF5 write succeeding and the rename — not portably reproducible in the current suite. |
| 19 | Execute — xdmf     | `write_xdmf_file=false` → skip rename, return `WriteFile`'s result directly                   | Most `Compression_*` tests, `"DREAM3DFileTest::StringArray"`, `"WriteDREAM3DFilter:Valid Parameters"` |


**Capability boundary behind Path 13 — `DataObject` types the shared HDF5 IO layer cannot write.**

`HDF5::DataIOManager::addCoreFactories()` (`DataStructure/IO/HDF5/DataIOManager.cpp:33-81`) registers factories per concrete `DataArray<T>` type, per `NeighborList<T>` specialization, per `ScalarData<T>` type, the eight geometries, `AttributeMatrix`, `DataGroup`, and `StringArray`. Two `DataObject` types reachable in the current codebase are **not** registered, and any `DataStructure` containing one cannot be written: `HDF5::DataStructureWriter::WriteFile` hits its "no factory found" guard (`DataStructureWriter.cpp:153-157`) and fails with error `-5` ("Could not find IO factory for datatype: …"), surfacing through `WriteDREAM3DFilter` as Path 13 above.

1. **`DynamicListArray<T>`** — the generic variable-length per-tuple list container that `NeighborList<T>` is itself built on. It has no IO class at all in `DataStructure/IO/HDF5/`, so nothing could be registered. Only reachable by constructing one directly; no shipped filter creates a bare `DynamicListArray` outside of a `NeighborList` specialization.

2. **`GridMontage`** — unlike the above, this type *does* have an IO class (`DataStructure/IO/HDF5/GridMontageIO.cpp`, with both `readData` and `writeData` bodies), but that class is never passed to `addFactory<>()`, so it is unreachable and the type is unwritable in practice. `GridMontage` is more reachable than `DynamicListArray`: `CreateGridMontageAction` (`src/simplnx/Filter/Actions/CreateGridMontageAction.cpp`) is public core API that any filter may use to create one. No filter in this repository currently does, so no shipped pipeline can hit this today — but a plugin filter using that public Action would produce a `DataStructure` that `WriteDREAM3DFilter` cannot save.

   **This is recorded as an observation, not as a defect with a known fix.** Montage support in SIMPLNX is an open design question that has deliberately not been settled yet, and the existing `GridMontage`/`GridMontageIO`/`CreateGridMontageAction` code predates that decision — it should be treated as an unfinished sketch inherited from the legacy SIMPL montage design, not as a foundation that merely needs switching on. Simply calling `addFactory<GridMontageIO>()` would *not* be a correct fix: `GridMontageIO::writeData` already creates a `groupWriter` it never uses and then passes `parentGroup` to `WriteBaseGroupData`, so registering it as-is would write montage contents into the wrong group. How montages should be represented on disk (and whether `GridMontage` is even the right in-memory abstraction) needs to be designed before any of this code is revived. This V&V cycle's only claim is the one its test makes: **today**, a `DataStructure` containing a `GridMontage` fails to write with error `-5` and leaves no partial file behind.

Both are gaps in the shared HDF5 IO layer, not in `WriteDREAM3DFilter`'s own algorithm — the same gaps affect `ReadDREAM3DFilter` for the same types. Neither is raised as a formal Deviation because there is no confirmed 6.5.171 pipeline behavior being compared against (unlike D2, which names concrete legacy types); they are recorded here as known capability boundaries of what this filter can serialize. The failure mode itself is no longer untested: `"WriteDREAM3DFilter:Unwritable DataObject Type"` pins it via `GridMontage`, asserting the `-5` error and that no partial file is committed.

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `WriteDREAM3DFilter:Invalid Parameters` (§ Empty FilePath, § Bad Compression Level) | kept | Preflight-only, empty `DataStructure` (the guards never touch data). Covers Paths 1, 2. |
| `WriteDREAM3DFilter:Valid Parameters` | kept | Preflight + full execute, full `CreateTestDataStructure()` fixture, compression off, own output file. Covers Paths 5, 7, 10, 12, 14, 16, 19. |
| `WriteDREAM3D:Pipeline / WriteXdmf combinations` | kept | Calls `DREAM3D::WriteFile(path, ds, pipeline, writeXdmf)` directly (bypasses `AtomicFile`/the Algorithm class) across `writeXdmf × {real pipeline, empty pipeline}` (4 cases), own output file. Exercises the shared write utility the Algorithm depends on, not the Algorithm's own guards; validates the `.xdmf` sidecar's heavy-data references via `CheckXdmfFile`. |
| `WriteDREAM3D:FileData Overload` | new-for-V&V | Writes through the exported `DREAM3D::WriteFile(HDF5::FileIO&, const FileData&)` forwarder (via `CreateFileData()`) and round-trips the full fixture — keeps the overload's pipeline/DataStructure argument ordering covered. |
| `WriteDREAM3D:Invalid File` | kept | Same free-function call with an empty path (4 cases via `GENERATE`); asserts the underlying `HDF5::FileIO::WriteFile` failure is surfaced, not `AtomicFile`'s. No sidecar check — a failed write has no target whose sidecar could exist. |
| `WriteDREAM3DFilter:Unwritable DataObject Type` | new-for-V&V | Full filter execute on a `DataStructure` containing a `GridMontage` (no registered IO factory). Pins the write-failure contract end to end: preflight valid, execute invalid with exactly one error of code `-5`, and no file left at the destination — the only test covering Path 13 and the `AtomicFile`-not-committed guarantee. GridMontage is only the vehicle for reaching that path, not the subject of the test (see the capability-boundary note under Code path coverage). |
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
| `DREAM3DFileTest: Geometry Nested In DataGroup Round Trip` | kept | Executes `WriteDREAM3DFilter` directly across all 8 geometry types, each at top level and nested inside a `DataGroup` (issue #1642 regression coverage), `write_xdmf_file=false`. Covers Paths 7, 10, 14, 16, 19 with per-geometry fixtures. |

**Deliberately out of scope (3 of the file's 23 `TEST_CASE`s).** `DREAM3DFileTest.cpp` is shared between the read and write sides of DREAM3D file IO. These three exercise `ReadDREAM3DFilter` only and belong to its V&V, not this one: `DREAM3DFileTest: Existing Data Objects Test` (importing into a populated `DataStructure`), `DREAM3DFileTest: Path Import Policy Tests` (read-side path-collision policy), and `SimplnxCore::ReadDREAM3DFilter: SIMPL Backwards Compatibility` (read-side SIMPL argument conversion). They are named here rather than silently omitted so the exclusion can be audited.

**Dual-build verification at sign-off:** the DREAM3D file IO tests pass **32/32 in both** the in-core (`NX-Com-Qt69-Vtk96-Rel`) and out-of-core (`NX-OOC-Qt69-Vtk95-Rel`) Release builds, at the rebased head. The full `SimplnxCore::` suite also passes 979/979 in-core.

## Exemplar archive

None. Every test above builds its input `DataStructure` inline in C++ and never loads a downloaded `.tar.gz` exemplar — the Class 1 oracle's "expected output" is the test's own hand-built input, so no cached golden file is required or used. (`Small_IN100_dream3d_v3.tar.gz`, referenced elsewhere in this test file, backs unrelated `ReadDREAM3DFilter`-only test cases and is not consumed by any Write-side test.)

## Deviations from DREAM3D 6.5.171

No byte/dataset-level A/B was run against 6.5.171 `DataContainerWriter` output, and none is warranted: the two writers target deliberately different on-disk contracts (see D1), so such a diff would be 100% noise by design, not signal. `ReadDREAM3DFilter` is the only tool that understands both formats, and cross-format fidelity is its concern, not this filter's. Two scope/format deviations are documented instead:

- `WriteDREAM3DFilter-D1` — on-disk file format is a deliberate, complete rewrite (v8 `DataStructure` layout vs. legacy v7 `DataContainers` layout; optional gzip compression; atomic write) — see `vv/deviations/WriteDREAM3DFilter.md`.
- `WriteDREAM3DFilter-D2` — `StatsDataArray`/`StructArray` (SIMPL ensemble-statistics types) cannot currently be written because those `DataObject` types do not yet exist in this branch of simplnx — see `vv/deviations/WriteDREAM3DFilter.md`.

Neither is a bug: D1 is the intended outcome of the Rewrite classification (defended above), and D2 is a scope boundary pending a separate in-progress port of those data types.
