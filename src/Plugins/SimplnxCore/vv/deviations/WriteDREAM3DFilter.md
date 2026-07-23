# Deviations from DREAM3D 6.5.171: WriteDREAM3DFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent (`DataContainerWriter`).

Entries are referenced by stable ID (`WriteDREAM3DFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

---

## WriteDREAM3DFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `WriteDREAM3DFilter-D1` |
| **Filter UUID** | `b3a95784-2ced-41ec-8d3d-0242ac130003` |
| **Status** | active |

**Symptom:** A `.dream3d` file written by SIMPLNX is not byte-compatible with, and cannot be opened by, a DREAM3D 6.5.171 install expecting the legacy layout — and the reverse is only possible through SIMPLNX's dedicated legacy-import code path (`ReadDREAM3DFilter`'s legacy `DataContainers` reader), not by treating the file as interchangeable.

**Root cause:** Algorithmic choice. SIMPLNX writes a clean-sheet v8 HDF5 layout: a top-level `DataStructure` group (vs. legacy's `DataContainers` group), a `FileVersion` attribute of `"8.0"` (vs. legacy `"7.0"`), an embedded JSON pipeline representation (vs. legacy's own pipeline serialization), atomic-rename write semantics (`AtomicFile`, so a crash mid-write cannot leave a corrupt file at the destination path), and an optional gzip/deflate compression scheme for `DataArray`/`NeighborList` datasets that has no legacy equivalent. This has been true since the filter's introduction — it was never a translation of legacy `DataContainerWriter`'s C++ (see Algorithm Relationship in the V&V report).

**Affected users:** Anyone attempting to open a SIMPLNX-written `.dream3d` file directly in a DREAM3D 6.5.171 install, or vice versa, outside of DREAM3D-NX's own dual-format `ReadDREAM3DFilter`. Users staying entirely within DREAM3D-NX (write with `WriteDREAM3DFilter`, read with `ReadDREAM3DFilter`) never observe this — round-trip fidelity within the new format is verified by the Class 1 tests in the V&V report.

**Recommendation:** Trust SIMPLNX. The new format is a deliberate design improvement (atomicity, compression, JSON pipeline embedding) required for capabilities legacy DREAM3D never had (out-of-core datasets, versioned pipeline metadata). It is not "wrong" relative to 6.5.171; it is a different, and newer, on-disk contract. DREAM3D-NX remains able to *read* legacy 6.5.171 files via `ReadDREAM3DFilter`'s legacy import path, so migration is one-directional by design (import old, export new) rather than bidirectional.

---

## WriteDREAM3DFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `WriteDREAM3DFilter-D2` |
| **Filter UUID** | `b3a95784-2ced-41ec-8d3d-0242ac130003` |
| **Status** | active |

**Symptom:** A pipeline that would have produced `StatsDataArray` or `StructArray` objects in legacy DREAM3D (e.g., ensemble statistics from a "Generate Ensemble Statistics"-style filter) cannot have those objects written to a `.dream3d` file by `WriteDREAM3DFilter` in the current develop branch — there is nothing in the `DataStructure` for the writer to serialize, because the types themselves do not exist yet.

**Root cause:** Library (incomplete port, out of scope for this V&V cycle). `StatsDataArray` and `StructArray` are DREAM3D 6.5.171 `DataObject` types whose simplnx equivalents are being implemented on a separate, not-yet-merged branch. `WriteDREAM3DFilter`'s own write path (`Algorithms/WriteDREAM3D.cpp`) has no special-case logic to reject or special-case these types — the gap is entirely upstream, in `DataStructure`/`HDF5::DataStructureWriter` not yet having a type to construct. This V&V pass covers everything `WriteDREAM3DFilter` can currently write; it makes no claim about statistics data because that data cannot currently exist in a simplnx `DataStructure`.

**Affected users:** Anyone migrating a legacy pipeline that computes per-ensemble statistics. Their exported SIMPLNX `.dream3d` file will simply lack the statistics group entirely (there being no such object to write) until the separate `StatsDataArray`/`StructArray` port lands and this filter is re-verified against it.

**Recommendation:** Trust 6.5.171 for statistics data until the pending port lands. This is a temporary feature gap, not a correctness defect in `WriteDREAM3DFilter` itself — re-run this V&V cycle's oracle tests once `StatsDataArray`/`StructArray` exist in this branch to confirm the writer handles them correctly.
