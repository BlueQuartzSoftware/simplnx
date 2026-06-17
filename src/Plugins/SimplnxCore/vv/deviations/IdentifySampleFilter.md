# Deviations from DREAM3D 6.5.171: IdentifySample

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

Entries are referenced by stable ID (`IdentifySample-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

---

## 3D mode — no deviations

Source-inspection comparison of the genuinely-3D path (`IdentifySampleFunctor<Image3D>`) against DREAM3D 6.5.171 `IdentifySample::execute()` confirmed identical algorithmic structure:

- **Phase 1 (largest-component isolation):** BFS flood-fill over all good voxels; tracks the largest connected component; all good voxels outside that component are set to false/0.
- **Phase 2 (hole fill, optional):** BFS flood-fill over all bad voxels; connected components that do not reach the volume boundary are flipped to good.

The algorithm is purely boolean/integer — no floating-point arithmetic — so precision drift cannot occur between two correct implementations of the same 3D logic.

**Confirmed parity — `checked` reset between phases:** `vv/provenance/IdentifySampleFilter.md` documents an "Internal cluster" fixture in `identify_sample_v2.tar.gz`, purpose-built to catch a bug where the `checked` BFS-visited tracker is not reset between Phase 1 and Phase 2 — voxels visited (and marked `checked=true`) while being absorbed into a non-largest component during Phase 1 would then be skipped entirely during Phase 2's hole-fill scan, since `checked` would already be `true` for them, leaving them permanently un-filled even when they qualify as an enclosed hole.

Direct source comparison confirms **both versions reset correctly and this is not a deviation**:

- **Legacy** (`DREAM3D/Source/Plugins/Processing/ProcessingFilters/IdentifySample.cpp:254`, checked out at tag `v6.5.171`): `checked.assign(totalPoints, false);` immediately before the `m_FillHoles` block.
- **SIMPLNX 3D mode** (`Algorithms/IdentifySample.cpp:112`): `checked.assign(totalPoints, false);` immediately before the `if(fillHoles)` block.
- **SIMPLNX SliceBySlice mode** (`Algorithms/IdentifySample.cpp:325`): `checked.assign(planeDim1 * planeDim2, false);` immediately before its own `if(fillHoles)` block, scoped per-slice.

The bug referenced in the test data's `generated_data/ReadMe.md` was a SIMPLNX-only regression at some prior point in development (already fixed in current source, as shown above) — it never existed in DREAM3D 6.5.171. The "Internal cluster" fixture remains in the archive as a guard against reintroducing it, not as evidence of any current or legacy deviation.

---

## IdentifySample-D1

| Field | Value |
|---|---|
| **Deviation ID** | `IdentifySample-D1` |
| **Filter UUID** | `94d47495-5a89-4c7f-a0ee-5ff20e6bd273` |
| **Status** | Active — SIMPLNX defines correct behavior; legacy behavior for this input class is unverified and suspected incorrect |

**Symptom:** For `ImageGeom` inputs with one or more dimensions equal to 1 (2D, 1D, or single-voxel geometries), `IdentifySampleFilter` output may differ from DREAM3D 6.5.171 `IdentifySample`.

**Root cause:** Legacy `IdentifySample::execute()` always computes face-neighbor offsets from a single fixed 3D-stride formula (`±1`, `±dims[0]`, `±dims[0]·dims[1]`) sized for a true 3D volume. SIMPLNX's `ProcessVoxels<>` dispatch (`Algorithms/IdentifySample.cpp:393-440`) detects degenerate dimensions via `emptyDimCount` and routes to dimensionality-specific neighbor templates (`EmptyZ/Y/XImage2D`, `Z/Y/XImage1D`, `SingleVoxelImage`), each with neighbor-offset and boundary-validity logic sized for the actual number of non-flat axes.

**Affected users:** Anyone running `IdentifySampleFilter` on a 2D, 1D, or single-voxel `ImageGeom` (e.g., a single-slice EBSD scan, a line scan, a single-point dataset) who expects numeric parity with a DREAM3D 6.5.171 pipeline run on the same degenerate input.

**Recommendation:** Trust SIMPLNX. The dimensionality-aware dispatch is purpose-built and Class 1 tested (3 non-square 2D fixtures in `IdentifySampleTest.cpp`, one per `EmptyZ/Y/XImage2D` path) to produce correct face connectivity regardless of geometry shape.

---

## New features (no legacy code path to compare against — not deviations)

These differ from `IdentifySample-D1` in that no input could have triggered the corresponding legacy behavior at all — there is nothing to deviate from.

- **`SliceBySlice` mode** (`IdentifySampleSliceBySliceFunctor`): gated behind a parameter (`SliceBySlice`) that does not exist in DREAM3D 6.5.171 and is not mapped by `FromSIMPLJson`. No legacy pipeline could have exercised this path.
- **Cancel checks** (`m_ShouldCancel`): UX-only; no algorithmic effect on a run that completes.

*If a future run against DREAM3D 6.5.171 confirms (or refutes) the suspected incorrectness in `IdentifySample-D1`, update its Status field accordingly.*
