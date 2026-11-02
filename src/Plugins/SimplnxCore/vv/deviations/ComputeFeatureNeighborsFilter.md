# Deviations from DREAM.3D v6.5.171: ComputeFeatureNeighborsFilter

Part 1 lists the two documented differences from `FindNeighbors` in DREAM.3D v6.5.171. Both defects also affected DREAM3D-NX v7.0.0 through v7.4.1. There are no Part 2 entries.

The deviation IDs are stable. The Filter UUID is the permanent cross-reference anchor.

## Part 1 — Differences from DREAM.3D v6.5.171

### ComputeFeatureNeighborsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureNeighborsFilter-D1` |
| **Filter UUID** | `7177e88c-c3ab-4169-abe9-1fdaff20e598` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Shared surface areas can differ from DREAM.3D v6.5.171 when voxel spacing is anisotropic.

**Root cause:** Bug. `FindNeighbors.cpp` counts contacts and multiplies every count by X spacing times Y spacing. This is the Z-normal face area. An X-normal face needs Y spacing times Z spacing; a Y-normal face needs X spacing times Z spacing. Current SIMPLNX accumulates each contact with the area returned by `computeFaceSurfaceAreas()` in `src/simplnx/Utilities/NeighborUtilities.hpp`.

**Affected users:** Users with anisotropic spacing and contacts whose face areas differ from X spacing times Y spacing. Downstream calculations that use SharedSurfaceAreaList can change. Isotropic spacing hides the error.

**Recommendation:** Trust SIMPLNX. DREAM3D-NX v7.4.2 and later use the geometric area of each shared face. The analytical 2D and 3D fixtures check direction-specific areas independently of the archive.

**Evidence:** The DREAM.3D v6.5.171 source at `Source/Plugins/Statistics/StatisticsFilters/FindNeighbors.cpp` contains `float area = float(number) * xRes * yRes`. Each DREAM3D-NX release from v7.0.0 through v7.4.1 contains the same source formula. The v7.4.2 source contains per-face accumulation. PR #1569 (`9e61b41d4`) introduced the correction; the release-source table is in `../provenance/ComputeFeatureNeighborsFilter.md`. The 2026-10-22 comparison in `vv_work:ComputeFeatureNeighborsFilter/comparison/LegacyComparison.md` records 16 SharedSurfaceAreaList value differences between DREAM.3D v6.5.171 and DREAM3D-NX in `anisotropic_xyz`, with an exact match for a local proof version that applies the fixes documented in `vv_work:ComputeFeatureNeighborsFilter/notes/proof_commits.md`.

---

### ComputeFeatureNeighborsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureNeighborsFilter-D2` |
| **Filter UUID** | `7177e88c-c3ab-4169-abe9-1fdaff20e598` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** DREAM.3D v6.5.171 marks interior features as surface features on 1D images and on 2D images with a unit X or Y dimension.

**Root cause:** Bug. The legacy boundary condition omits Z when Z has length one, but always tests X and Y. With unit X or Y, every cell has column zero or row zero, so every positive feature is marked. Current Direct execution distinguishes corners, edges and faces by dimensionality. Current Scanline execution tests boundaries only on axes longer than one and treats a single voxel as a surface feature.

**Affected users:** Users who enable SurfaceFeatures on 1D images or unit-X or unit-Y 2D images. Full 3D images and 2D images with only a unit Z dimension do not expose this defect.

**Recommendation:** Trust SIMPLNX. DREAM3D-NX v7.4.2 and later mark only features that touch the boundary in an active dimension. The 1D and 2D analytical fixtures assert false flags for interior features.

**Evidence:** The tagged `FindNeighbors.cpp` source tests `column == 0` and `row == 0` in both boundary conditions. Each DREAM3D-NX release from v7.0.0 through v7.4.1 retains those source conditions. The v7.4.2 source uses dimensionality-specific marking. Current tests `Case 1.0.0`, `Case 1.1.0`, `Case 1.2.0`, `Case 2.1.0` and `Case 2.2.0` assert the corrected flags. The 2026-10-22 comparison in `vv_work:ComputeFeatureNeighborsFilter/comparison/LegacyComparison.md` records two SurfaceFeatures flags that change from 1 in DREAM.3D v6.5.171 to 0 in DREAM3D-NX in each of `unit_y_surface` and `line_z_surface`, with exact matches for a local proof version that applies the fixes documented in `vv_work:ComputeFeatureNeighborsFilter/notes/proof_commits.md`.
