# Deviations from DREAM.3D v6.5.171: RotateSampleRefFrameFilter

Part 1 describes the supported-rotation difference from DREAM.3D v6.5.171. Part 2 records two fixed SIMPLNX defects. All entries use one stable ID sequence and the filter UUID as a permanent reference.

## Part 1 — Differences from DREAM.3D v6.5.171

### RotateSampleRefFrameFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `RotateSampleRefFrameFilter-D1` |
| **Filter UUID** | `d2451dc1-a5a1-4ac2-a64d-7991669dcffc` |
| **Status** | active |

**Symptom:** DREAM.3D v6.5.171 accepts arbitrary rotation angles. SIMPLNX returns `-6850` for rotations outside the proper signed axis permutations and `-6851` when slice-by-slice operation exchanges Z with another axis.

**Root cause:** Algorithmic choice. SIMPLNX checks the rotation block for one signed unit entry per row and column and a determinant of +1, within tolerance `1e-4`. This includes 120-degree rotations about (111), as well as principal-axis quarter turns. The slice guard also requires the Z axis to remain on Z. Arbitrary nearest-neighbor rotations can duplicate or omit source cells and introduce zero background. DREAM.3D v6.5.171 has neither guard. DREAM3D-NX v7.4.2 contains both guards.

**Affected users:** Users who request arbitrary rotations or combine a slice-axis exchange with slice-by-slice operation. `ReadH5Ebsd` calls this filter in slice-by-slice mode for recommended sample transformations and propagates its errors during import.

**Recommendation:** Trust SIMPLNX for the supported rotations. Use `Apply Transformation To Geometry` for arbitrary resampling. Disable `Use Recommended Transformations` in `ReadH5Ebsd` when the stored transform cannot use slice-by-slice operation, and apply the required transformation separately.

**Evidence:** `RotateSampleRefFrameFilter.cpp::IsLosslessGridRotation` and `preflightImpl()` implement the guards. The two rejection tests check `-6850` and `-6851`; the 120-degree fixture checks acceptance and value conservation. Tagged DREAM.3D v6.5.171 source has no equivalent validation. `vv_work:RotateSampleRefFrameFilter/results.md` records four matching principal-axis cases on 2026-07-02; subsequent algorithm changes are listed in the provenance sidecar.

---

## Part 2 — SIMPLNX defects and behavior changes

### RotateSampleRefFrameFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `RotateSampleRefFrameFilter-D2` |
| **Filter UUID** | `d2451dc1-a5a1-4ac2-a64d-7991669dcffc` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** A rotation with a nonzero input origin placed the output geometry at the wrong origin when the filter used the transform-derived origin.

**Root cause:** Bug. Preflight added the input origin to the minimum corner of the transformed bounding box. That corner already included the input origin. Preflight now assigns the corner directly, so the stored origin agrees with the geometry used for sampling. DREAM.3D v6.5.171 does not add an input origin to an absolute transformed corner and was not affected by this defect.

**Affected users:** Users with a nonzero input origin who use the transform-derived output origin. For a 3×2×1 input at (10,20,0), a 90-degree Z rotation previously stored (-12,30,0) instead of (-22,10,0).

**Recommendation:** Trust SIMPLNX in DREAM3D-NX v7.4.2 or later. The output origin now uses the transformed bounding box once.

**Evidence:** Direct inspection of every release source from `upstream/release/v7.0.0` through `upstream/release/v7.4.1` shows `origin +=` with origin-aware bounds; v7.4.2 assigns the bounds. PR #1651 (`790d9ca18`) contains the fix. `Class 1 - non-zero input origin` checks both origin modes and exact voxel values. Tagged legacy `Source/Plugins/Sampling/SamplingFilters/RotateSampleRefFrame.cpp` constructs coordinates without the input origin and calls `setOrigin(xMin, yMin, zMin)`.

---

### RotateSampleRefFrameFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `RotateSampleRefFrameFilter-D3` |
| **Filter UUID** | `d2451dc1-a5a1-4ac2-a64d-7991669dcffc` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** Copying extra geometry children could change their names or produce an invalid destination path when a child or parent name contained the source geometry name.

**Root cause:** Bug. Preflight replaced every occurrence of the geometry name in each complete path string. For example, copying `Geom/Geom Feature Data/Geom Values` to `Geom Out` also changed both child names. `DataPath::rebase()` now changes only the source geometry path prefix, including the declared recursive paths. DREAM.3D v6.5.171 rotates its selected container in place and has no corresponding child-copy path.

**Affected users:** Users whose geometry has additional children, or parent groups, with names that contain the geometry name. The copied object names or their destination paths could change unexpectedly.

**Recommendation:** Trust SIMPLNX with the fix in the DREAM3D-NX release after v7.4.2. On affected releases, use names that do not repeat the geometry name in parent or child components.

**Evidence:** Each release source from `upstream/release/v7.0.0` through `upstream/release/v7.4.2` uses `StringUtilities::replace()` for copied paths. PR #1765 (`1986127eb`) replaces both sites with `DataPath::rebase()`. `Child names containing the geometry name are kept` checks declared paths and copied objects. Tagged DREAM.3D v6.5.171 source has no corresponding copy operation.
