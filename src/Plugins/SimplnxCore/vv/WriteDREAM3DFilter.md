# V&V Report: WriteDREAM3DFilter

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `b3a95784-2ced-41ec-8d3d-0242ac130003` |
| SIMPLNX Human Name | Write DREAM3D-NX File |
| DREAM.3D v6.5.171 equivalent | `DataContainerWriter` (SIMPL UUID `3fcd4c43-9d75-5b86-aad4-4441bc914f37`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Matthew Marine, 2026-07-23; second engineer: Michael Jackson, 2026-08-20 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Rewrite of DREAM.3D v6.5.171 `DataContainerWriter`. SIMPLNX writes the version 8 HDF5 layout, with atomic file replacement and optional compression. |
| Oracle (confirmed) | Class 1 constructed values, HDF5 layout checks and XDMF filename checks, with a Class 4 compression-size check. The ordinary tests in `test/DREAM3DFileTest.cpp` encode these checks. All pass in CI. |
| Code paths enumerated | 15 of 21 exercised. Failure injection, detached-pipeline, cancellation and hidden storage-boundary paths have no ordinary-test witness. |
| Tests today | 24 test cases in the shared file: 23 ordinary cases and 1 hidden storage-boundary case. They cover writing, reading, compression, geometry ownership and SIMPL conversion. |
| Exemplar archive | None for writer verification. The writer fixtures construct their input and expected values in C++. |
| Legacy comparison | Not run — comparison artifacts are not available. |
| Bug flags | None |
| V&V phase | COMPLETE. |

## Summary

`WriteDREAM3DFilter` saves a DataStructure and its preceding pipeline in a `.dream3d` file, with optional compression and an XDMF sidecar. Constructed values and direct HDF5 layout checks verify the writer; the ordinary tests all pass in CI. DREAM3D-NX v7.4.2 contains the two shared-I/O defect fixes found during V&V.

## Algorithm Relationship

*Classification:* Rewrite

*Evidence:* `SimplnxCoreLegacyUUIDMapping.hpp` maps `DataContainerWriter` to this filter; `Dream3dIO.hpp` defines version 8.0 with a `DataStructure` layout instead of the legacy version 7.0 `DataContainers` layout.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant) companions.

*Applied:* Constructed values define expected arrays and geometry links; HDF5 probes check compression, and XDMF text checks require the output filename in data references.

*Encoded:* `src/Plugins/SimplnxCore/test/DREAM3DFileTest.cpp::WriteDREAM3DFilter:Valid Parameters`, [File I/O test](../test/DREAM3DFileTest.cpp#L1318), and the five `WriteDREAM3DFilter: Compression_*` cases provide seven content, layout and validation fixtures. All pass in CI.

*Second-engineer review:* Michael Jackson, 2026-08-20.

## Bugs found and fixed

All defects in this table are fixed. The fixes will be in the DREAM3D-NX release after v7.4.1. DREAM3D-NX v7.4.2 contains the fix for `WriteDREAM3DFilter-D3`. DREAM3D-NX v7.4.2 contains the fix for `WriteDREAM3DFilter-D4`.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `WriteDREAM3DFilter-D3` | XDMF node-geometry helpers exchanged the geometry name and HDF5 filename, which broke attribute references. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Each helper forwards the geometry name and filename in the correct order. |
| `WriteDREAM3DFilter-D4` | The shared `ReadFile` utility checked the pipeline result twice and could access a failed DataStructure result. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | The reader returns DataStructure import errors before it accesses the value. |

## Code path coverage

15 of 21 paths exercised. The table distinguishes ordinary-test coverage from failure paths and the hidden storage-boundary fixture.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/WriteDREAM3D.cpp` (86 lines) and `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/WriteDREAM3DFilter.cpp` (142 lines). Shared I/O checks use `src/simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.cpp` (3302 lines).

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Preflight | Empty output path is rejected | `WriteDREAM3DFilter:Invalid Parameters` — `Empty FilePath` checks an invalid result, not a specific error code |
| 2 | Preflight | Compression enabled, level below 1 → error `-2` | `WriteDREAM3DFilter:Invalid Parameters`; `WriteDREAM3DFilter: Compression_Preflight_RejectsOutOfRangeLevel` |
| 3 | Preflight | Compression enabled, level above 9 → error `-2` | `WriteDREAM3DFilter: Compression_Preflight_RejectsOutOfRangeLevel` |
| 4 | Preflight | Compression disabled → ignore an out-of-range level | `WriteDREAM3DFilter: Compression_Preflight_RejectsOutOfRangeLevel` |
| 5 | Preflight | Valid parameters → success | `WriteDREAM3DFilter:Valid Parameters` |
| 6 | Setup | `AtomicFile::Create` fails → return its error | *Not directly tested. No fixture injects a destination or permission failure at creation.* |
| 7 | Setup | `AtomicFile::Create` succeeds | `WriteDREAM3DFilter:Valid Parameters` |
| 8 | Pipeline | A pipeline node has no preceding pipeline → error `-15` | *Not directly tested. No fixture supplies a detached pipeline node.* |
| 9 | Pipeline | A pipeline node supplies its preceding pipeline | [Single-file import/export test](../test/DREAM3DFileTest.cpp#L1402); [Multiple-file import/export test](../test/DREAM3DFileTest.cpp#L1513) |
| 10 | Pipeline | No pipeline node → write an empty pipeline | `WriteDREAM3DFilter:Valid Parameters` — combined reader accepts the saved pipeline and data |
| 11 | Compression | Enabled → use the selected compression level | `WriteDREAM3DFilter: Compression_On_IsChunkedAndDeflated`; `WriteDREAM3DFilter: Compression_SmallArray_Bypasses`; `WriteDREAM3DFilter: Compression_LevelsRoundTrip` |
| 12 | Compression | Disabled → set compression level to zero | `WriteDREAM3DFilter: Compression_Off_IsContiguous` |
| 13 | Write | `WriteFile` fails → return its error without file replacement | `WriteDREAM3DFilter:Unwritable DataObject Type` — error `-5`, no destination file |
| 14 | Write | `WriteFile` succeeds | `WriteDREAM3DFilter:Valid Parameters` — expected data values survive the round trip |
| 15 | File replacement | Atomic file replacement fails → return its error | *Not directly tested. No fixture injects a failure after the temporary file is written.* |
| 16 | File replacement | Atomic file replacement succeeds | `WriteDREAM3DFilter:Valid Parameters` — destination exists and can be read |
| 17 | XDMF | Enabled → rename the sidecar successfully | [Single-file import/export test](../test/DREAM3DFileTest.cpp#L1402) exercises the rename; [Pipeline and XDMF combinations test](../test/DREAM3DFileTest.cpp#L1210) checks references from the shared writer |
| 18 | XDMF | Sidecar rename fails → return the system error | *Not directly tested. No fixture injects a sidecar rename failure.* |
| 19 | XDMF | Disabled → skip sidecar rename | `WriteDREAM3DFilter:Valid Parameters`; `WriteDREAM3DFilter: Compression_Off_IsContiguous` |
| 20 | Storage boundary | Write a full source chunk and one extra tuple | *Not directly tested. The dedicated `SimplnxCore::WriteDREAM3DFilter: real HDF5 chunk-tail raw-read oracle` is hidden and requires the HDF5-OOC manager.* |
| 21 | Cancellation | Cancellation after a successful write → return `-1` before file replacement | *Not directly tested. No fixture sets the cancellation flag at this point.* |

## Test inventory

`src/Plugins/SimplnxCore/test/DREAM3DFileTest.cpp` contains all 24 cases below. The 23 ordinary cases run in the in-core configuration. All pass in CI. Linked test labels point to the exact declarations in this file.

| Test case | Status | Notes |
|-----------|--------|-------|
| `WriteDREAM3DFilter:Invalid Parameters` | kept | Rejects an empty output path and compression level 0 when compression is enabled. |
| `SimplnxCore::WriteDREAM3DFilter: real HDF5 chunk-tail raw-read oracle` | new-for-V&V | Hidden. Direct HDF5 reads compare 262,148 uint32 values across a 65,536-tuple boundary, with compression off and on; checks the deflate flag. |
| `WriteDREAM3DFilter:Valid Parameters` | kept | Executes the filter and checks constructed values, geometry links and tuple shapes after reading the result. |
| [Pipeline and XDMF combinations test](../test/DREAM3DFileTest.cpp#L1210) | kept | Calls the shared writer for four pipeline/sidecar combinations. Checks XDMF filename prefixes when enabled. |
| [FileData overload test](../test/DREAM3DFileTest.cpp#L1231) | new-for-V&V | Calls the FileData overload and checks pipeline size, constructed values and tuple shapes after reading. |
| [Invalid-file test](../test/DREAM3DFileTest.cpp#L1256) | kept | Checks failure for an empty path in all four pipeline/sidecar combinations. |
| `WriteDREAM3DFilter:Unwritable DataObject Type` | new-for-V&V | A GridMontage has no registered HDF5 factory. Checks error `-5` and no destination file. |
| [File I/O test](../test/DREAM3DFileTest.cpp#L1318) | kept | Checks arrays, scalar values, eight geometry types and links, pipeline size and filter names; validates XDMF filename prefixes. |
| `DREAM3DFileTest::StringArray` | kept | Writes and reads `foo`, `bar`, `baz` through the filters and compares every string. |
| [Single-file import/export test](../test/DREAM3DFileTest.cpp#L1402) | kept | Executes an export pipeline; checks imported objects and the difference between execution stores and metadata-only preflight stores. |
| `DREAM3DFileTest: Preflight imports geometry connectivity as metadata-only stores` | kept | Reader check on `geoms.dream3d`: ten connectivity/bounds paths in each of the `Preflight`, `Execute` and `DataStructureReader::ReadFile` dynamic-section groups. Checks store type and tuple count. |
| [Multiple-file import/export test](../test/DREAM3DFileTest.cpp#L1513) | kept | Imports two exported files; checks both top-level groups and tuple shapes. |
| `DREAM3DFileTest: Existing Data Objects Test` | kept | Reader check: imports Small IN100 into a populated DataStructure and checks tuple shapes. |
| `DREAM3DFileTest: Path Import Policy Tests` | kept | Reader check: imports all paths or includes/excludes selected leaves and parents from Small IN100. |
| `SimplnxCore::ReadDREAM3DFilter: SIMPL Backwards Compatibility` | kept | Reader conversion check in `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` dynamic sections; checks filter identity and pipeline size. |
| `SimplnxCore::WriteDREAM3DFilter: SIMPL Backwards Compatibility` | kept | The same two conversion sections check UUID, comments, output path, XDMF setting and disabled compression. |
| `DREAM3DFileTest: DataArray datasets are chunked+deflated when WriteOptions requests it` | kept | Shared writer: 500,000 float32 values, level 5 deflate, chunked layout and exact array round trip. |
| `WriteDREAM3DFilter: Compression_Off_IsContiguous` | kept | Checks contiguous layout and no deflate for 200,000 float32 values. |
| `WriteDREAM3DFilter: Compression_On_IsChunkedAndDeflated` | kept | Checks chunked layout, level 5 deflate and an exact round trip of 500,000 float32 values. |
| `WriteDREAM3DFilter: Compression_SmallArray_Bypasses` | kept | A 400-byte array stays contiguous; a 2 MB array uses chunking and deflate. |
| `WriteDREAM3DFilter: Compression_LevelsRoundTrip` | kept | Checks one million repeating values at levels 1, 5 and 9, then requires non-increasing file size for this fixture. |
| `DREAM3DFileTest: Geometry Nested In DataGroup Round Trip` | kept | Eight dynamic sections: VertexGeom, EdgeGeom, TriangleGeom, QuadGeom, TetrahedralGeom, HexahedralGeom, ImageGeom and RectGridGeom. Checks concrete types at top level and inside a group. |
| `WriteDREAM3DFilter: Compression_Preflight_RejectsOutOfRangeLevel` | kept | Rejects levels 0 and 10 with compression enabled; accepts level 0 with compression disabled. |
| `DREAM3DFileTest: PreflightCache avoids re-reading unchanged files` | kept | Reader cache check: repeated preflight adds no miss or file open; execution reads values, and file replacement invalidates the cache. |

## Exemplar archive

- **Archive:** None for writer verification. The two Small IN100 reader cases use `Small_IN100_dream3d_v3.tar.gz`; writer expectations are constructed in C++.
- **SHA512:** No writer archive. The shared reader archive matches `test/CMakeLists.txt`: `bfa9547e787b0f8e8122702da0eb4e519f48a48bf4f94aad020f72479d071d32dfc96a1425705874c68507a61ed391d28606d9c4f4acd559043ef0ace64fd33f`.
- **Provenance:** Not applicable to the constructed writer fixtures; their builders and expected values are in `src/Plugins/SimplnxCore/test/DREAM3DFileTest.cpp`.

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/WriteDREAM3DFilter.md` for the root cause, affected users and recommendation. These Part 1 differences follow from source inspection.

| Deviation | Observed difference |
|-----------|---------------------|
| `WriteDREAM3DFilter-D1` | SIMPLNX writes version 8 `DataStructure` files instead of version 7 `DataContainers` files. |
| `WriteDREAM3DFilter-D2` | SIMPLNX imports legacy statistics as groups and arrays; it does not write legacy `StatsDataArray` or `StructArray` objects. |
