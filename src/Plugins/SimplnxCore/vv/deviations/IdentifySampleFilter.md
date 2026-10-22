# Deviations from DREAM.3D v6.5.171: IdentifySampleFilter

Part 1 records the difference from DREAM.3D v6.5.171. Part 2 records SIMPLNX defects and behavior changes. The existing `IdentifySample-D1` ID is unchanged; new entries continue the sequence as `IdentifySampleFilter-D2` through `IdentifySampleFilter-D6`. The Filter UUID is the permanent cross-reference anchor.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### IdentifySample-D1

| Field | Value |
|---|---|
| **Deviation ID** | `IdentifySample-D1` |
| **Filter UUID** | `94d47495-5a89-4c7f-a0ee-5ff20e6bd273` |
| **Status** | active |

**Symptom:** With `FillHoles=true` and whole-image processing, BFS fills an enclosed hole on a 2D image. DREAM.3D v6.5.171 leaves it false.

**Root cause:** Bug. DREAM.3D v6.5.171 checks the extrema of all three axes when it tests whether a false component reaches the boundary. A unit dimension makes that condition true at every cell. BFS checks neighbors only along non-flat axes. This corrects the enclosure test without changing face connectivity. DREAM3D-NX v7.0.0 through v7.4.1 used the same boundary test as DREAM.3D v6.5.171; DREAM3D-NX v7.4.2 contains the BFS correction. The separate CCL behavior is `IdentifySampleFilter-D6`.

**Affected users:** Users who fill enclosed holes on a single-slice image with whole-image BFS. DREAM.3D v6.5.171 was affected. The legacy boundary test also marks every cell on a one-dimensional image as boundary-connected.

**Recommendation:** Trust SIMPLNX BFS for 2D hole filling. The flat axis must not make an otherwise enclosed hole part of the exterior.

**Evidence:** DREAM.3D tag `v6.5.171`, `Source/Plugins/Processing/ProcessingFilters/IdentifySample.cpp`, uses the three-axis coordinate test. Current `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/IdentifySampleBFS.cpp` selects `EmptyZImage2D`, `EmptyYImage2D`, or `EmptyXImage2D` and tests valid neighbors. PR #1590 (`e76ad508e`) introduced this dimensional dispatch. Direct inspection of all `upstream/release/v7.0.0` through `upstream/release/v7.4.2` sources establishes the release range. The three non-square tests check connectivity only; they disable hole filling.

---

## Part 2 — SIMPLNX defects and behavior changes

### IdentifySampleFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `IdentifySampleFilter-D2` |
| **Filter UUID** | `94d47495-5a89-4c7f-a0ee-5ff20e6bd273` |
| **Status** | active |
| **Affected released versions** | None. DREAM3D-NX v7.0.0 through v7.4.2 were not affected. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Dimensional BFS could change the only false voxel in a 1×1×1 image to true when hole filling was enabled.

**Root cause:** Bug. `SingleVoxelImage` has zero neighbors. The empty neighbor loop left `touchesBoundary=false`. The corrected initialization, `touchesBoundary = k_NeighborCount == 0`, treats the voxel as boundary-connected. Earlier released implementations used coordinate checks and already left this voxel false.

**Affected users:** Users of the dimensional BFS before its single-voxel correction. No listed DREAM3D-NX release exposed the defect.

**Recommendation:** Trust SIMPLNX. A single false voxel stays false because it touches the image boundary.

**Evidence:** PR #1640 (`a332e7369`) adds the guard after PR #1590 introduced dimensional dispatch. Releases through v7.4.1 have coordinate boundary checks; v7.4.2 has the guard. DREAM.3D v6.5.171 also has coordinate checks. Current `IdentifySampleBFS.cpp` retains the guard; no test in `IdentifySampleTest.cpp` constructs a single-voxel image.

---

### IdentifySampleFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `IdentifySampleFilter-D3` |
| **Filter UUID** | `94d47495-5a89-4c7f-a0ee-5ff20e6bd273` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.2 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Slice BFS selected the first of two equally large components, while whole-image BFS selected the later component.

**Root cause:** Behavior change. Slice selection used `currentVList.size() > biggestBlock`. Whole-image selection used `>=`. Slice BFS now also uses `>=`, so a later equal-size component replaces the earlier component.

**Affected users:** Users of slice processing with two or more components tied for largest size. DREAM.3D v6.5.171 has no slice-processing mode.

**Recommendation:** Trust SIMPLNX. Do not depend on the older first-component tie rule when comparing saved masks.

**Evidence:** PR #1640 (`a332e7369`) changes `>` to `>=`. Release sources v7.0.0 and v7.0.1 have no slice functor; v7.0.2 through v7.4.1 use `>`; v7.4.2 uses `>=`. Current `IdentifySampleCommon.hpp::processSlice()` retains that comparison. No current fixture isolates an equal-size largest-component tie.

---

### IdentifySampleFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `IdentifySampleFilter-D4` |
| **Filter UUID** | `94d47495-5a89-4c7f-a0ee-5ff20e6bd273` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.2 through v7.4.0. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.1 |

**Symptom:** Hole filling in an XZ or YZ slice could write true values to the wrong image cells.

**Root cause:** Bug. The write used `fixedIdx * fixedStride + idx`, where `idx` was a local plane index. The correction decomposed that index into two plane coordinates and applied their strides. Current slice implementations transfer the selected plane to a buffer and write it back with the correct image layout.

**Affected users:** Users who enabled hole filling and selected XZ or YZ slice processing. DREAM.3D v6.5.171 has no slice-processing mode.

**Recommendation:** Trust SIMPLNX. Recalculate masks from affected releases when slice hole filling used XZ or YZ.

**Evidence:** PR #1562 (`42bc9ba6e`) corrects the index and registers `identify_sample_v2.tar.gz`. Release sources v7.0.2 through v7.4.0 contain the incorrect write; v7.4.1 and v7.4.2 contain the stride correction. `SimplnxCore::IdentifySampleFilter` compares all values in `sliced_xz_fill` and `sliced_yz_fill`. Current implementations are `IdentifySampleCommon.hpp` and `IdentifySampleCCL.cpp`.

---

### IdentifySampleFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `IdentifySampleFilter-D5` |
| **Filter UUID** | `94d47495-5a89-4c7f-a0ee-5ff20e6bd273` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.2 through v7.4.0. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.1 |

**Symptom:** Slice processing could leave false cells inside an enclosed hole where it had removed a smaller true island.

**Root cause:** Bug. Slice BFS reused the visited flags from largest-component selection. Hole filling skipped cells that the first pass had visited and then cleared. Slice BFS now resets the flags before hole filling. CCL labels false components in a separate pass. The legacy whole-image algorithm already reset its visited flags.

**Affected users:** Users who enabled slice hole filling on masks with disconnected true islands inside an enclosed hole. DREAM.3D v6.5.171 has no slice-processing mode.

**Recommendation:** Trust SIMPLNX. Recalculate affected slice masks to fill the removed internal islands.

**Evidence:** PR #1562 (`42bc9ba6e`) adds `checked.assign(planeDim1 * planeDim2, false)`. Release sources v7.0.2 through v7.4.0 omit the slice reset; v7.4.1 and v7.4.2 contain it. The archive's `generated_data/ReadMe.md` defines an eight-cell internal island for this regression. The `sliced_*_fill` sections compare the complete output masks. Current `IdentifySampleCommon.hpp::processSlice()` clears the visited flags.

---

### IdentifySampleFilter-D6

| Field | Value |
|---|---|
| **Deviation ID** | `IdentifySampleFilter-D6` |
| **Filter UUID** | `94d47495-5a89-4c7f-a0ee-5ff20e6bd273` |
| **Status** | active |
| **Affected released versions** | None. DREAM3D-NX v7.0.0 through v7.4.2 do not contain CCL. DREAM.3D v6.5.171 was affected by the same boundary rule. |
| **Fixed in** | Not fixed |

**Symptom:** With whole-image processing and hole filling enabled, CCL leaves enclosed holes false on a 2D image. BFS fills them.

**Root cause:** Bug. CCL marks a false component as exterior when any voxel lies at an extremum of any of the three axes. A flat axis therefore marks every false component as exterior. BFS excludes flat axes from its neighbor boundary check. This CCL regression restores the boundary rule documented in `IdentifySample-D1`.

**Affected users:** Users whose whole-image execution selects CCL for a flat image with an enclosed hole. Slice processing uses a separate boundary test.

**Recommendation:** Use BFS for whole-image 2D hole filling, or select slice processing in the non-flat plane. Do not assume whole-image BFS and CCL give the same filled mask on a flat image.

**Evidence:** Current `IdentifySampleCCL.cpp` marks boundary roots with `x == 0 || x == dimX - 1 || y == 0 || y == dimY - 1 || z == 0 || z == dimZ - 1`. The behavior follows directly for a 5×5×1 true mask with a false center: `z == 0` marks that center as exterior. The filter dispatcher does not exclude flat images from CCL. PR #1568 (`4ec21e93f`) adds CCL after v7.4.2. All listed release sources contain only the earlier algorithm. The non-square tests disable hole filling, and the hidden HDF5 test uses a 129×129×3 image, so neither detects this case.
