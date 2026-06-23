# Deviations from DREAM3D 6.5.171: ComputeFeatureNeighborsFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent (`FindNeighbors`, SIMPL UUID `97cf66f8-7a9b-5ec2-83eb-f8c4c8a17bac`, `Source/Plugins/Statistics/StatisticsFilters/FindNeighbors.cpp`).

Legacy source reviewed at: `DREAM3D/Source/Plugins/Statistics/StatisticsFilters/FindNeighbors.cpp`.

Comparison run on the `6_6_stats_test_v2.tar.gz` SmallIn100 fixture (SHA512 `e84999dec914d81efce4fc4237c49c9bf32e48381b1e79f58aa4df934f0d7606cd7a948f9a5e7b17a126a7944cc531b531cfdc70756ca3e2207b20734e089723`).

---

## ComputeFeatureNeighborsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureNeighborsFilter-D1` |
| **Filter UUID** | `7177e88c-c3ab-4169-abe9-1fdaff20e598` |
| **Status** | active |

**Symptom:** `SharedSurfaceAreaList` values produced by SIMPLNX differ from DREAM3D 6.5.171 for any dataset with anisotropic spacing. The test at `test/ComputeFeatureNeighborsTest.cpp:910` records this explicitly: *"The exemplar Shared Surface Area is not valid after a bug fix, and the input file is used in other test cases."*

**Root cause:** Bug in 6.5.171. The legacy finalization loop at `FindNeighbors.cpp:431`:

```cpp
float area = float(number) * xRes * yRes;
```

computes the shared surface area between a feature pair as `count × xSpacing × ySpacing`, where `count` is the number of shared face-boundary voxel-contacts. This formula is only correct for faces whose normal points along Z (those faces have area = xSpacing × ySpacing). For faces with X-normal (area = ySpacing × zSpacing) or Y-normal (area = xSpacing × zSpacing), the formula gives the wrong result whenever spacing is anisotropic.

SIMPLNX corrects this via `computeFaceSurfaceAreas<ImageDimensionStateT>()` (`NeighborUtilities.hpp:322`), which returns a per-face-direction area array: `{sx·sy, sx·sz, sy·sz, sy·sz, sx·sz, sx·sy}` for faces `{−Z, −Y, −X, +X, +Y, +Z}`. The accumulation `neighborSurfaceAreas[feature][neighborFeatureId] += precomputedFaceAreas[faceIndex]` therefore applies the geometrically correct area for each individual face contact.

**Affected users:** All workflows using `SharedSurfaceAreaList` on datasets with anisotropic voxel spacing (xSpacing ≠ ySpacing ≠ zSpacing). For isotropic spacing (all three equal), the legacy formula happens to give the correct result for all face directions and no deviation is observable. The Small IN100 dataset has anisotropic spacing; downstream filters that consume `SharedSurfaceAreaList` (e.g., `ComputeSlipTransmissionMetrics`) will produce different results.

**Recommendation:** Trust SIMPLNX. The 6.5.171 formula is geometrically incorrect for any non-Z-normal face when spacing is anisotropic. The SIMPLNX values are verified against the geometric definition of face area by 37 independent Class 1 analytical test cases.

---

## ComputeFeatureNeighborsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureNeighborsFilter-D2` |
| **Filter UUID** | `7177e88c-c3ab-4169-abe9-1fdaff20e598` |
| **Status** | active |

**Symptom:** `SurfaceFeatures` in DREAM3D 6.5.171 marks every feature as a surface feature for any image whose X or Y dimension is 1 (i.e., 1D images and 2D EmptyY/EmptyX images). SIMPLNX correctly identifies only features that touch the actual image boundary in the active dimensions.

**Root cause:** Bug in 6.5.171. The legacy surface-feature check at `FindNeighbors.cpp:330–343`:

```cpp
// Branch 1 — fires when ZPoints != 1
if((column == 0 || column == XPoints-1 || row == 0 || row == YPoints-1 ||
    plane == 0 || plane == ZPoints-1) && ZPoints != 1)
{
  m_SurfaceFeatures[feature] = true;
}
// Branch 2 — fires when ZPoints == 1 (EmptyZ 2D)
if((column == 0 || column == XPoints-1 || row == 0 || row == YPoints-1) && ZPoints == 1)
{
  m_SurfaceFeatures[feature] = true;
}
```

Branch 2 correctly handles the EmptyZ case (ZPoints==1) by restricting the check to X and Y boundaries. However, Branch 1 is used for all other cases including 1D images and 2D EmptyY/EmptyX images:

- **1D Z image** (XPoints=1, YPoints=1, ZPoints=N): `column == 0` is always true (only one X column), so every voxel triggers Branch 1 → every feature is marked surface.
- **1D Y image** (XPoints=1, YPoints=N, ZPoints=1): `column == 0` is always true → Branch 2 fires → every feature is marked surface.
- **1D X image** (XPoints=N, YPoints=1, ZPoints=1): `row == 0` is always true (only one Y row) → Branch 2 fires → every feature is marked surface.
- **2D EmptyY image** (XPoints=M, YPoints=1, ZPoints=N, ZPoints≠1): `row == 0` is always true → Branch 1 fires → every feature is marked surface.
- **2D EmptyX image** (XPoints=1, YPoints=M, ZPoints=N, ZPoints≠1): `column == 0` is always true → Branch 1 fires → every feature is marked surface.

SIMPLNX's correction: explicit dimensionality dispatch to `ImageDimensionStateT` specializations. The `ProcessSurfaceFeaturesV` template flag is applied at three levels: corner processing (marks all dimensionalities unconditionally for corners, which are always on the boundary), edge processing (`!Is1DImageDimsState()` guard — edges are only on the boundary for 2D and 3D), and face processing (`std::is_same_v<ImageDimensionStateT, Image3D>` guard — boundary-plane interior voxels only exist in 3D). This correctly restricts surface-feature marking to voxels that genuinely touch the image boundary in each active dimension.

**Affected users:** Any pipeline that runs `ComputeFeatureNeighbors` / `FindNeighbors` on a 2D or 1D image geometry (`XPoints==1` or `YPoints==1`) and uses the `SurfaceFeatures` output. For full 3D datasets (all three dimensions > 1), the two implementations produce identical `SurfaceFeatures` output. This deviation is **not observable** on the SmallIn100 dataset (all three dimensions ≫ 1), which is why the SmallIn100 legacy comparison test passes for SurfaceFeatures.

**Recommendation:** Trust SIMPLNX. The 6.5.171 result is geometrically incorrect for degenerate image dimensions — it flags internal features as surface features when a dimension-1 axis makes `column==0` or `row==0` trivially true for every voxel.

---

## Non-deviations (documented for future-engineer awareness)

- **NumNeighbors** — bit-identical on SmallIn100. Both implementations deduplicate face contacts via a map-like structure (`QMap<int32_t, int32_t>` in legacy; `std::map<usize, float64>` in SIMPLNX) and write the size as `NumNeighbors`.
- **NeighborList** — bit-identical on SmallIn100. Both produce entries in ascending neighbor feature-ID order (legacy via `QMap` iteration; SIMPLNX via `std::map` iteration). This ordering is an algorithm characteristic, not a guaranteed API contract.
- **BoundaryCells** — not present in the `6_6_stats_test_v2.tar.gz` archive; comparison not run on SmallIn100. By inspection, both implementations count `onsurf` / `numDiffNeighbors` identically: increment once per valid face contact where the neighbor has a different non-zero feature ID. The face-validity logic (boundary guard per face index) is equivalent in both for all dimensionalities — the legacy `neighpoints` + per-index plane/row/column guards correctly exclude invalid faces for all geometry shapes, matching the SIMPLNX `computeValidFaceNeighbors` output. Correctness of SIMPLNX BoundaryCells established by the Class 1 oracle fixtures.
- **Neighbor ordering** — same `QMap` / `std::map` ascending-key iteration order in both; algorithm characteristic, not a specified API guarantee.
