# Deviations from DREAM.3D v6.5.171: WriteDREAM3DFilter

Part 1 lists file-format and data-representation differences from `DataContainerWriter` in DREAM.3D v6.5.171. Part 2 lists shared-I/O defects found and fixed during this filter's V&V. Both parts use the stable `WriteDREAM3DFilter-D<N>` sequence.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### WriteDREAM3DFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `WriteDREAM3DFilter-D1` |
| **Filter UUID** | `b3a95784-2ced-41ec-8d3d-0242ac130003` |
| **Status** | active |

**Symptom:** A SIMPLNX `.dream3d` file uses a different HDF5 layout from a DREAM.3D v6.5.171 file and cannot serve as a legacy-format replacement.

**Root cause:** Algorithmic choice. SIMPLNX writes file version 8.0 with a `DataStructure` group. DREAM.3D v6.5.171 writes version 7.0 with a `DataContainers` group. The new writer supports nested data objects, optional gzip compression and atomic replacement of the HDF5 destination. The legacy UUID mapping preserves the save-data role and converts the output-path and XDMF parameters; it does not promise the same file layout. Converted SIMPL pipelines disable compression, and the legacy time-series option has no corresponding writer parameter.

**Affected users:** Users who exchange `.dream3d` files with DREAM.3D v6.5.171 or read the HDF5 layout directly.

**Recommendation:** Trust SIMPLNX for the new file format. Use `ReadDREAM3DFilter` to import legacy files into DREAM3D-NX, and update external readers to the version 8 layout.

**Evidence:** `src/simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp` defines versions 7.0 and 8.0. `WriteDREAM3DFilter::FromSIMPLJson()` defines the parameter conversion. SIMPL source `0afaa6854` (the last revision before the DREAM.3D v6.5.171 tag date) defines version 7.0 and `DataContainers` in `Source/SIMPLib/Common/Constants.h`; `Source/SIMPLib/CoreFilters/DataContainerWriter.cpp` writes that layout. The round-trip tests compare constructed values, not bytes from a legacy file.

---

### WriteDREAM3DFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `WriteDREAM3DFilter-D2` |
| **Filter UUID** | `b3a95784-2ced-41ec-8d3d-0242ac130003` |
| **Status** | active |

**Symptom:** Imported legacy statistics use groups and arrays in SIMPLNX instead of native `StatsDataArray` and `StructArray` objects.

**Root cause:** Library. SIMPLNX has no `StatsDataArray` or `StructArray` DataObject type. Its legacy reader recognizes the `Statistics` object type, creates a `Statistics` DataGroup and recursively imports supported child groups, datasets and attributes. The writer has HDF5 factories for the resulting groups and arrays. The absence of the legacy C++ types does not mean that statistics must be absent from exported files.

**Affected users:** Users who migrate legacy statistics or use software that expects the legacy statistics object types and schema.

**Recommendation:** Trust SIMPLNX for saving its supported groups and arrays. Adapt consumers to the imported representation instead of expecting legacy object types or a legacy-format export.

**Evidence:** `src/simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.cpp` implements `readLegacyStatsDataArray()` and `readLegacyStatsDataArrayChild()`. `src/simplnx/DataStructure/IO/HDF5/DataIOManager.cpp` registers DataGroup, numeric-array and StringArray factories. SIMPL source `0afaa6854` contains `Source/SIMPLib/DataArrays/StatsDataArray.cpp` and `StructArray.hpp`. The writer test file has no statistics-specific import/export fixture.

---

## Part 2 — SIMPLNX defects and behavior changes

### WriteDREAM3DFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `WriteDREAM3DFilter-D3` |
| **Filter UUID** | `b3a95784-2ced-41ec-8d3d-0242ac130003` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** XDMF attribute references on node geometries could contain the geometry name where the HDF5 filename was required.

**Root cause:** Bug. `WriteXdmfNodeGeometry1D`, `WriteXdmfNodeGeometry2D` and `WriteXdmfNodeGeometry3D` exchanged two `std::string_view` arguments when they called the lower-dimensional helper. The compiler accepted the swapped arguments. The helpers now preserve the geometry-name and filename order. DREAM.3D v6.5.171 uses a different writer and has no such forwarding chain.

**Affected users:** Users who generate XDMF sidecars for edge, surface or volume node geometries with attributes handled by a lower-dimensional helper.

**Recommendation:** Trust SIMPLNX. Regenerate affected sidecars with DREAM3D-NX v7.4.2 or later.

**Evidence:** PR #1683 (`9bcf794e9`) fixes all three calls in `src/simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.cpp`. Direct source checks of every `upstream/release/v7.0.0` through `upstream/release/v7.4.1` show the exchanged arguments; `upstream/release/v7.4.2` has the corrections. `CheckXdmfFile()` in `src/Plugins/SimplnxCore/test/DREAM3DFileTest.cpp` checks filename prefixes in the pipeline/sidecar combination and file-I/O tests. It does not resolve every referenced HDF5 dataset. SIMPL `0afaa6854`, `Source/SIMPLib/DataContainers/DataContainer.cpp`, passes the container name and HDF5 filename separately to its geometry and attribute writers.

---

### WriteDREAM3DFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `WriteDREAM3DFilter-D4` |
| **Filter UUID** | `b3a95784-2ced-41ec-8d3d-0242ac130003` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** The shared file reader could access an invalid result instead of returning a DataStructure import error.

**Root cause:** Bug. After a successful pipeline import, the shared `ReadFile()` utility imported the DataStructure but checked `pipeline.invalid()` again. A failed DataStructure import then reached `dataStructure.value()`. The reader now tests `dataStructure.invalid()` and returns its errors. DREAM.3D v6.5.171 does not use this combined Result-based reader.

**Affected users:** Callers of the shared `ReadFile` utility, including writer round-trip checks, when pipeline import succeeds but DataStructure import fails.

**Recommendation:** Trust SIMPLNX. Use DREAM3D-NX v7.4.2 or later to receive the DataStructure import error.

**Evidence:** PR #1683 (`9bcf794e9`) changes the tested result in `src/simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.cpp`. Every release source from `upstream/release/v7.0.0` through `upstream/release/v7.4.1` repeats the pipeline check; `upstream/release/v7.4.2` checks the DataStructure result. SIMPL `0afaa6854`, `Source/SIMPLib/CoreFilters/DataContainerReader.cpp`, uses a separate DataContainerArray reader. The writer tests cover successful combined reads but do not inject this DataStructure import failure.
