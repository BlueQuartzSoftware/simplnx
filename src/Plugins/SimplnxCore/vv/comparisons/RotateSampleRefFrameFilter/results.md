# Legacy Comparison: RotateSampleRefFrame

Date: 2026-07-02

## Environment

- **DREAM3D 6.5.171:** `/Users/mjackson/Applications/DREAM3D.app/Contents/Bin/PipelineRunner` (official release)
- **DREAM3D-NX:** `.../DREAM3D-Build/NX-Com-Qt69-Vtk96-Rel/Bin/nxrunner`
- **Legacy filter:** `Source/Plugins/Sampling/SamplingFilters/RotateSampleRefFrame.cpp` (SIMPL UUID `{e25d9b4c-2b37-578c-b1de-cf7032b5ef19}`)

## Input Data

Shared, identical input minted with the `legacy_dream3d` h5py writer (`make_input.py`): a `4×3×2` **Image Geometry** (`ImageDataContainer`), origin (0,0,0), spacing (1,1,1), with an Int32 cell array `Data` filled `1..24` in ZYX order (distinct, nonzero → the exact voxel permutation is visible). FileVersion `7.0`, read correctly by both PipelineRunner and nxrunner. See `make_input.py`, `gen_simpl_pipeline.py`, `gen_nx_pipeline.py`.

Both versions rotate in place (`ImageDataContainer/CellData/Data`). Legacy has no slice-by-slice or representation parameter (always full 3D rotation); NX configured with the Axis-Angle representation, `remove_original_geometry=true`, `slice_by_slice=false`.

## Test Cases and Results

| Case | Axis-Angle | Output dims (nx,ny,nz) | Legacy vs SIMPLNX |
|---|---|---|---|
| 90Z | (0,0,1,90) | (3,4,2) | **bit-identical** |
| 180Z | (0,0,1,180) | (4,3,2) | **bit-identical** |
| 90X | (1,0,0,90) | (4,2,3) | **bit-identical** |
| 180Y | (0,1,0,180) | (4,3,2) | **bit-identical** |

All four cases: identical output dimensions **and** identical voxel values (element-wise equal, 0/24 differ). SIMPLNX also matches the independent Class 1 analytical-permutation oracle encoded in the unit test (e.g. 180Z reverses each Z slice: `[12..1, 24..13]`).

## Root-cause note

Legacy computes the source index by **truncation** (`colOld = (int64)(coord/xRes)`, old-origin assumed 0) while SIMPLNX inverse-maps each **cell center** through origin-aware `ImageGeom::computeCellIndex`. For an exact 90°-multiple rotation about a principal axis with integer coordinates and origin (0,0,0), every output cell center inverse-maps exactly onto a source cell center, so truncation and nearest-cell resolve to the same source voxel — the two implementations converge exactly. (Off-axis / non-90 rotations, where the two rules would diverge, are now rejected in SIMPLNX preflight; see deviation D1.)

## Fixes Applied

None — outputs matched on the entire supported (principal-90) domain.

## Deviation

One documented behavioral deviation, `RotateSampleRefFrameFilter-D1`: legacy silently accepts arbitrary (non-principal-90) rotations and produces a lossy nearest-neighbor resample; SIMPLNX rejects them in preflight (`-6850`). Intentional (root cause: algorithmic choice). See `../../deviations/RotateSampleRefFrameFilter.md`.
