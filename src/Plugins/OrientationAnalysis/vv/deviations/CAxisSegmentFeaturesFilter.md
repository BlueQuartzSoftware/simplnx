# Deviations from DREAM.3D v6.5.171: CAxisSegmentFeaturesFilter

Part 1 lists current differences from DREAM.3D v6.5.171 (D2 and D3). Part 2 lists fixed SIMPLNX defects (D1 and D4 through D9). Both parts use the stable `CAxisSegmentFeaturesFilter-D<N>` sequence; existing IDs are unchanged.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### CAxisSegmentFeaturesFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `CAxisSegmentFeaturesFilter-D2` |
| **Filter UUID** | `9fe07e17-aef1-4bf1-834c-d3a73dafc27d` |
| **Status** | active |

**Symptom:** Raw FeatureIds can differ between DREAM.3D v6.5.171 and SIMPLNX even when the segmentation partitions agree.

**Root cause:** Algorithmic choice. DREAM.3D v6.5.171 sets `m_RandomizeFeatureIds` to true and seeds the generator from the clock; its pipeline reader exposes no randomization control. SIMPLNX defaults to no randomization. Its optional shuffle uses `std::mt19937_64::default_seed` in `src/simplnx/Utilities/ClusteringUtilities.cpp` and leaves background ID zero fixed.

**Affected users:** Users who compare numeric labels across applications or repeated legacy runs.

**Recommendation:** Trust SIMPLNX. Compare partitions when label numbers are not the quantity of interest.

**Evidence:** On 2026-09-17, repeated stock DREAM.3D v6.5.171 runs changed raw labels and preserved the partition. A local proof version with randomization disabled matched DREAM3D-NX v7.4.2 raw labels on the six valid shared Image cases. See `vv_work:CAxisSegmentFeaturesFilter/comparison/comparison_results.md`. The current `Class 4 Invariants (RandomizeFeatureIds)` test checks partition preservation and repeatability.

---

### CAxisSegmentFeaturesFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `CAxisSegmentFeaturesFilter-D3` |
| **Filter UUID** | `9fe07e17-aef1-4bf1-834c-d3a73dafc27d` |
| **Status** | active |

**Symptom:** SIMPLNX rejects participating non-hexagonal phases with `-8363` and positive phase indices outside Crystal Structures with `-8364`; DREAM.3D v6.5.171 segments these inputs.

**Root cause:** Algorithmic choice. The legacy grouping calculation does not read Crystal Structures. SIMPLNX checks each participating phase against Hexagonal_High or Hexagonal_Low and validates the ensemble index. Masked cells and cells with nonpositive phases do not participate. The supported scientific domain is hexagonal c-axis segmentation.

**Affected users:** Users with mixed phases or invalid phase indices.

**Recommendation:** Trust SIMPLNX. Mask non-hexagonal cells and correct invalid phase indices before segmentation.

**Evidence:** `Execute Error - Non-Hexagonal Crystal Structure (-8363)` and `Execute Error - Phase Out of Ensemble Bounds (-8364)` check the current errors. The release comparison records the corresponding legacy outputs as TC9 and TC7 in `vv_work:CAxisSegmentFeaturesFilter/comparison/comparison_results.md`.

---

## Part 2 — SIMPLNX defects and behavior changes

### CAxisSegmentFeaturesFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `CAxisSegmentFeaturesFilter-D1` |
| **Filter UUID** | `9fe07e17-aef1-4bf1-834c-d3a73dafc27d` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** A masked or unindexed first cell could create an empty feature or start growth from an ineligible cell.

**Root cause:** Bug. DREAM3D-NX v7.4.1 sets `seed = 0` before its flood-fill loop. It bypasses `getSeed()` validation and label assignment. DREAM3D-NX v7.4.2 validates the first seed with `getSeed()`. Current connected-component labeling checks every cell with `isValidVoxel()` before assigning a label. DREAM.3D v6.5.171 obtains every seed through `getSeed()`.

**Affected users:** Users of DREAM3D-NX v7.4.1 whose first cell cannot seed a feature, including all-masked input.

**Recommendation:** Trust SIMPLNX. Use DREAM3D-NX v7.4.2 or later and regenerate affected results.

**Evidence:** PR #1685 (`025333c60`) contains the correction. `Class 1 Analytical (Mask Excludes Voxel 0)` and `Execute Error - No Features Found (-87000)` check the correction. TC3 in `vv_work:CAxisSegmentFeaturesFilter/comparison/comparison_results.md` records v7.4.1 labels `[0,2,2,0,3]` and four Active tuples, versus v7.4.2 labels `[0,1,1,0,2]` and three tuples.

---

### CAxisSegmentFeaturesFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `CAxisSegmentFeaturesFilter-D4` |
| **Filter UUID** | `9fe07e17-aef1-4bf1-834c-d3a73dafc27d` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Excluded non-hexagonal phases could cause a false rejection; later validation also rejected phase-zero cells and read outside the ensemble array.

**Root cause:** Bug. DREAM3D-NX v7.0.0 through v7.2.2 validate every ensemble entry after zero, including unused or masked phases. DREAM3D-NX v7.4.0 and v7.4.1 instead validate every cell through `crystalStructures[currentPhaseIdx]`, without mask, nonpositive-phase or bounds guards. Thus the phase-zero rejection and unchecked phase-index read apply to v7.4.0 through v7.4.1, not the entire range. DREAM3D-NX v7.4.2 excludes masked and nonpositive-phase cells from validation. It checks phase bounds and caches validated phases. DREAM.3D v6.5.171 has no crystal-structure validation loop.

**Affected users:** Users with excluded non-hexagonal phases; users of v7.4.0 through v7.4.1 with unindexed cells or invalid phase indices.

**Recommendation:** Trust SIMPLNX. Use DREAM3D-NX v7.4.2 or later; valid excluded cells no longer cause rejection.

**Evidence:** PR #1682 (`5eb69f359`) adds the exemptions; PR #1685 (`025333c60`) adds bounds checking and cached validation. The phase-zero, masked non-hexagonal and phase-bounds tests cover these corrections. TC4, TC6 and TC7 in `vv_work:CAxisSegmentFeaturesFilter/comparison/comparison_results.md` demonstrate the v7.4.0/v7.4.1 failures and v7.4.2 results.

---

### CAxisSegmentFeaturesFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `CAxisSegmentFeaturesFilter-D5` |
| **Filter UUID** | `9fe07e17-aef1-4bf1-834c-d3a73dafc27d` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** RectGrid input passed the geometry selection and then crashed during execution.

**Root cause:** Bug. The parameter accepts Image and RectGrid, but the algorithm fetched `ImageGeom`. The shared driver dereferenced the resulting null pointer for RectGrid. DREAM3D-NX v7.4.2 uses `IGridGeometry` for both geometry types. The DREAM.3D v6.5.171 filter parameters require Image geometry; RectGrid is not a supported path.

**Affected users:** Users who select RectGrid input in DREAM3D-NX v7.0.0 through v7.4.1.

**Recommendation:** Trust SIMPLNX. Use DREAM3D-NX v7.4.2 or later for RectGrid segmentation.

**Evidence:** PR #1685 (`025333c60`) contains the correction. `Class 1 Analytical (RectGrid Geometry)` asserts `[1,1,2]`. TC8 in `vv_work:CAxisSegmentFeaturesFilter/comparison/comparison_results.md` records signal 11 for v7.4.0 and v7.4.1 and the expected labels for v7.4.2.

---

### CAxisSegmentFeaturesFilter-D6

| Field | Value |
|---|---|
| **Deviation ID** | `CAxisSegmentFeaturesFilter-D6` |
| **Filter UUID** | `9fe07e17-aef1-4bf1-834c-d3a73dafc27d` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.0. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.1 |

**Symptom:** The feature AttributeMatrix had one extra empty tuple; all-masked input could return success with no real feature.

**Root cause:** Bug. The shared driver initialized `gnum` to 1, incremented it after each feature and stored `m_FoundFeatures = gnum`. The filter then allocated `m_FoundFeatures + 1` tuples. DREAM3D-NX v7.4.1 stores `gnum - 1`; its separate first-seed defect is D1. Current labeling counts resolved real features. DREAM.3D v6.5.171 resizes its feature matrix only when it finds a valid seed and rejects a matrix containing background only.

**Affected users:** Users of DREAM3D-NX v7.0.0 through v7.4.0 who use feature tuple counts or depend on the no-feature error.

**Recommendation:** Trust SIMPLNX. Use DREAM3D-NX v7.4.2 or later to obtain both the counter correction and the D1 correction.

**Evidence:** PR #1466 (`3d1cc153a`) contains the counter correction. TC1 in `vv_work:CAxisSegmentFeaturesFilter/comparison/comparison_results.md` records four real features but six Active tuples in v7.4.0; five tuples are correct. Current analytical fixtures call `CheckActiveArray()` to enforce N + 1 tuples.

---

### CAxisSegmentFeaturesFilter-D7

| Field | Value |
|---|---|
| **Deviation ID** | `CAxisSegmentFeaturesFilter-D7` |
| **Filter UUID** | `9fe07e17-aef1-4bf1-834c-d3a73dafc27d` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Cell arrays with equal tuple counts but fewer tuples than geometry cells could pass preflight and cause an out-of-bounds traversal.

**Root cause:** Bug. Preflight compared the selected arrays with each other but not with the geometry. DREAM3D-NX v7.4.2 checks the geometry count (`-652`) and the CellData count (`-653`). It uses the CellData shape for FeatureIds. DREAM.3D v6.5.171 also compares only array tuple counts; its driver derives neighbor indices from geometry dimensions. SIMPL `DataContainerArray::validateNumberOfTuples()` does not check geometry size.

**Affected users:** Users with malformed cell arrays or an inconsistent cell AttributeMatrix.

**Recommendation:** Trust SIMPLNX. Supply exactly one tuple per geometry cell; current preflight rejects inconsistent data.

**Evidence:** PR #1685 (`025333c60`) contains the correction. `Preflight Error - Cell Arrays Smaller Than Geometry (-652)` uses nine tuples for ten cells. `Preflight Error - Cell AttributeMatrix Smaller Than Geometry (-653)` checks a nine-tuple output parent with ten input tuples. Both guards are absent through v7.4.1 and present in v7.4.2.

---

### CAxisSegmentFeaturesFilter-D8

| Field | Value |
|---|---|
| **Deviation ID** | `CAxisSegmentFeaturesFilter-D8` |
| **Filter UUID** | `9fe07e17-aef1-4bf1-834c-d3a73dafc27d` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Quats in a sibling AttributeMatrix caused execution to dereference a missing FeatureIds array.

**Root cause:** Bug. Preflight created FeatureIds in the geometry cell matrix, but execution used `QuatsArrayPath.replaceName(...)`. DREAM3D-NX v7.4.2 uses `gridGeom.getCellDataPath()` in both preflight and execution. DREAM.3D v6.5.171 creates and uses FeatureIds beside Quats, so its paths agree.

**Affected users:** Users whose selected Quats array is outside geometry CellData.

**Recommendation:** Trust SIMPLNX. Current execution writes FeatureIds to the same path declared by preflight.

**Evidence:** PR #1685 (`025333c60`) contains the correction. `Class 1 Analytical (Quats Outside Cell AttributeMatrix)` asserts `[1,1,2]` in CellData with Quats and phases in OrientationData. The mismatched path is present in every release through v7.4.1 and corrected in v7.4.2.

---

### CAxisSegmentFeaturesFilter-D9

| Field | Value |
|---|---|
| **Deviation ID** | `CAxisSegmentFeaturesFilter-D9` |
| **Filter UUID** | `9fe07e17-aef1-4bf1-834c-d3a73dafc27d` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Cancellation during a valid segmentation could produce error `-87000` instead of a clean return.

**Root cause:** Bug. The released driver returned on cancellation before assigning `m_FoundFeatures`, which starts at zero. The caller then applied the no-feature check. DREAM3D-NX v7.4.2 returns on cancellation before that check. DREAM.3D v6.5.171 sizes Active when it finds each valid seed and checks that array size after stopping; it does not use this deferred feature counter.

**Affected users:** Users who cancel segmentation after an eligible seed exists.

**Recommendation:** Trust SIMPLNX. DREAM3D-NX v7.4.2 and current code return before reporting a no-feature error when canceled.

**Evidence:** PR #1685 (`025333c60`) contains the correction. The cancellation return is absent in the algorithm on release branches through v7.4.1 and present in v7.4.2. Current code keeps the guard after `executeCCL()`. No test in `CAxisSegmentFeaturesTest.cpp` injects cancellation; report path 11 records that gap.
