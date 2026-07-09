# Deviations from DREAM3D 6.5.171: RegularizeZSpacingFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent (`RegularizeZSpacing`, Sampling plugin, UUID `bc4952fa-34ca-50bf-a1e9-2b9f7e5d47ce`).

Entries are referenced by stable ID (`RegularizeZSpacingFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

---

## No deviations

**None observed.** A direct A/B comparison was run against DREAM3D 6.5.171.

- **Fixture:** synthetic Image Geometry, dims 3×2×4, spacing (0.5, 0.75, 1.0), origin (1, 2, 3). Cell data: `Data` (`int32`, value == voxel index), `Mask` (`DataArray<bool>`), `Vec` (`float32`, 3-component). Authored directly in legacy v7 `.dream3d` format so DREAM3D 6.5.171 and DREAM3D-NX read byte-identical input.
- **Parameters:** Z boundary positions `{0, 1, 3, 6, 10}` (ZPoints + 1 = 5 values); `new_z_spacing = 2.0`; in-place.
- **Result:** legacy 6.5.171 output, SIMPLNX output, and the independent Class-1 analytical oracle are **bit-identical** on every cell array (`Data`, `Mask`, `Vec`) and on the output geometry (dimensions 3×2×5, spacing (0.5, 0.75, 2.0), origin unchanged). The upsampled plane-repeat pattern (destination planes 2 and 3 both drawing from source plane 2) is reproduced identically by both implementations.

The port preserves the legacy Z-plane mapping rule and dimension arithmetic exactly; the port-time deltas enumerated in the report (bulk copy, new-geometry output mode, added preflight validation, parallelization, cell-AM binding) are non-behavioral for valid input, as confirmed by this comparison.

---

## Scope note: cell-AM binding on converted pipelines (not an output deviation)

Legacy selected the target cell data by an explicit AttributeMatrix path; SIMPLNX selects the Image Geometry and operates on its *assigned* cell AttributeMatrix. When a legacy pipeline is converted (`FromSIMPLJson`), only the DataContainer name is kept from `CellAttributeMatrixPath` — the AM name is discarded.

- **Common case (one cell AM per geometry):** identical behavior; the discarded AM name and the geometry's cell AM are the same object.
- **Corner cases:** a legacy DataContainer holding more than one cell-level AM where the pipeline selected the non-canonical one will, after conversion, resample the geometry's assigned cell AM instead; a stale AM name that legacy rejected with an error converts to a pipeline that proceeds on the actual cell AM. Additionally, SIMPLNX errors on a geometry with no cell AM (`-5560`) and on non-DataArray cell-AM members (`-5561`) — states legacy either could not express or would have crashed on.

These are parameter-model/conversion semantics, not output-math differences; for identical valid inputs the outputs are bit-identical (comparison above).
