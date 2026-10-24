# Deviations from DREAM.3D v6.5.171: RemoveFlaggedFeaturesFilter

Part 1 lists current differences from DREAM.3D v6.5.171. Part 2 lists fixed SIMPLNX defects. All entries share one stable ID sequence.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### RemoveFlaggedFeaturesFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `RemoveFlaggedFeaturesFilter-D3` |
| **Filter UUID** | `6e8cc6ec-8b9b-402e-9deb-85bd1cdba743` |
| **Status** | active |

**Symptom:** SIMPLNX returns `-45436` when no removed cell can be filled; DREAM.3D loops forever. SIMPLNX also prevents FeatureIds from being ignored and emits warning `-45438`.

**Root cause:** bug. An unflagged feature with no cells passes the all-flagged check but cannot supply fill values. Neither earlier implementation detected a pass without progress. Ignoring FeatureIds also prevented progress. The current algorithm checks progress and always copies FeatureIds.

**Affected users:** Users who flag every occupied feature or include FeatureIds among the ignored arrays with fill enabled. DREAM3D-NX v7.0.0 through v7.4.2 and DREAM.3D v6.5.171 were affected.

**Recommendation:** Trust SIMPLNX. The fix will be in the DREAM3D-NX release after v7.4.2. Keep an unflagged feature that owns cells, or disable fill; after `-45436`, removed cells remain -1.

**Evidence:** The tagged legacy removal source and every release source through v7.4.2 omit the progress guard. `Execute Error - no fill progress (-45436)` and `Fill cannot ignore the Feature Ids array (-45438 warning)` cover the fixes. `vv_work:RemoveFlaggedFeaturesFilter/ReadMe.md` and `vv_work:RemoveFlaggedFeaturesFilter/log_6_5_171_C_hang.txt` record the legacy timeout.

### RemoveFlaggedFeaturesFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `RemoveFlaggedFeaturesFilter-D4` |
| **Filter UUID** | `6e8cc6ec-8b9b-402e-9deb-85bd1cdba743` |
| **Status** | active |

**Symptom:** SIMPLNX rejects invalid FeatureIds or tuple counts before removal; DREAM.3D can read beyond an array.

**Root cause:** bug. Earlier removal code indexed the active-feature vector without checking each ID. It also accepted a FeatureIds tuple count that differed from the geometry cell count. Remove and Extract then Remove now validate both before modification, with errors `-45435` and `-45437`.

**Affected users:** Users whose FeatureIds do not match the selected feature data or geometry. DREAM3D-NX v7.0.0 through v7.4.2 and DREAM.3D v6.5.171 were affected.

**Recommendation:** Trust SIMPLNX. The fix will be in the DREAM3D-NX release after v7.4.2. Correct the selected arrays or IDs before removal; validation errors leave the data unchanged.

**Evidence:** The tagged legacy source and every release source through v7.4.2 contain unchecked marking. The two execute-error tests assert the error codes and unchanged IDs. `vv_work:RemoveFlaggedFeaturesFilter/ReadMe.md` records the legacy out-of-range result on fixture D.

### RemoveFlaggedFeaturesFilter-D6

| Field | Value |
|---|---|
| **Deviation ID** | `RemoveFlaggedFeaturesFilter-D6` |
| **Filter UUID** | `6e8cc6ec-8b9b-402e-9deb-85bd1cdba743` |
| **Status** | active |

**Symptom:** SIMPLNX skips an empty flagged feature with warning `-53905`; DREAM.3D can create a false one-cell geometry at origin (-1,-1,-1). Earlier DREAM3D-NX releases threw an exception.

**Root cause:** bug. DREAM.3D initialized empty bounds to -1. Earlier SIMPLNX passed minimum bounds greater than maximum bounds to the crop. The current algorithm detects empty bounds, skips those features and lists them in one warning.

**Affected users:** Users who flag feature tuples that own no cells. DREAM3D-NX v7.0.0 through v7.4.2 and DREAM.3D v6.5.171 were affected.

**Recommendation:** Trust SIMPLNX. The fix will be in the DREAM3D-NX release after v7.4.2. No geometry should represent a feature that has no cells.

**Evidence:** `vv_work:RemoveFlaggedFeaturesFilter/ReadMe.md` records fixture F: DREAM.3D created `Feature_4` with ID 0 and CellValue 100. Tagged legacy extraction initializes bounds to -1. All DREAM3D-NX release sources through v7.4.2 omit the skip check. The empty-feature test asserts one warning and absent geometries.

### RemoveFlaggedFeaturesFilter-D7

| Field | Value |
|---|---|
| **Deviation ID** | `RemoveFlaggedFeaturesFilter-D7` |
| **Filter UUID** | `6e8cc6ec-8b9b-402e-9deb-85bd1cdba743` |
| **Status** | active |

**Symptom:** SIMPLNX names extracted geometries `<prefix>-<zero-padded id>` and copies the source Feature Attribute Matrix. DREAM.3D uses `Feature_<id>` and copies only cell data.

**Root cause:** algorithmic choice. SIMPLNX exposes a prefix and pads IDs to the width of the feature tuple count. It delegates extraction to CropImageGeometry with feature renumbering disabled, which copies feature attributes without recomputing them.

**Affected users:** Users whose scripts use legacy geometry names or expect feature attributes to describe only the crop.

**Recommendation:** Trust SIMPLNX. Update geometry paths for the mandatory hyphen and padded ID. Recompute feature attributes when they must describe the cropped cells.

**Evidence:** The Class 1 extraction tests assert names, crop bounds, cell values and unchanged copied feature arrays. The tagged legacy source builds `Feature_` names. `vv_work:RemoveFlaggedFeaturesFilter/ReadMe.md` records the packaging difference for fixture B.

---

## Part 2 — SIMPLNX defects and behavior changes

### RemoveFlaggedFeaturesFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `RemoveFlaggedFeaturesFilter-D1` |
| **Filter UUID** | `6e8cc6ec-8b9b-402e-9deb-85bd1cdba743` |
| **Status** | retired 2026-08-13 |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Fill could run forever when a removed cell had one valid face neighbor or when all valid neighbors had different feature IDs.

**Root cause:** bug. The tally recorded a source only after a feature appeared twice. The first sighting now counts as one hit and can supply the cell. DREAM.3D counted the first sighting.

**Affected users:** Users who enable fill on thin geometries or isolated removed regions.

**Recommendation:** Trust SIMPLNX in DREAM3D-NX v7.4.2 or later. Earlier releases require fill to be disabled for these inputs.

**Evidence:** Release sources through v7.4.1 omit the initial hit; v7.4.2 sets it in `FeatureRemovalUtilities.cpp`. The Class 1 distinct-neighbor fixture checks the exact IDs and copy sources. PR #1700 fixed the defect.

### RemoveFlaggedFeaturesFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `RemoveFlaggedFeaturesFilter-D2` |
| **Filter UUID** | `6e8cc6ec-8b9b-402e-9deb-85bd1cdba743` |
| **Status** | retired 2026-09-02 |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** Fill could run forever when input cells had FeatureId 0.

**Root cause:** bug. The neighbor search treated background cells as unresolved, but tuple copying only changed negative IDs. The search now skips all nonnegative IDs. Background remains a valid fill source.

**Affected users:** Users who enable fill on data with background cells.

**Recommendation:** Trust SIMPLNX with the fix. In DREAM3D-NX v7.4.2 or earlier, disable fill when background cells are present.

**Evidence:** Every release source through v7.4.2 uses `featureName > 0` to skip targets. The current Direct and Scanline sources use `>= 0`. The background Class 1 fixture and termination test cover this defect.

### RemoveFlaggedFeaturesFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `RemoveFlaggedFeaturesFilter-D5` |
| **Filter UUID** | `6e8cc6ec-8b9b-402e-9deb-85bd1cdba743` |
| **Status** | retired 2026-09-02 |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected by this result-handling defect. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** Extraction ignored delegated execution failures and threw an exception for delegated preflight failures.

**Root cause:** bug. After execute, SIMPLNX tested the earlier preflight result instead of the execution result. The filter now checks each result and returns errors `-53901` through `-53904` with the delegated message. DREAM.3D had no corresponding Result-based check.

**Affected users:** Users whose bounds computation or crop fails during extraction.

**Recommendation:** Trust SIMPLNX. Use the returned error message to correct the input.

**Evidence:** Every release source through v7.4.2 repeats `preflightResult.outputActions.invalid()` after execute and throws on preflight failure. The current Direct, Scanline and common crop sources return errors. These delegated failure paths have no direct fixture.

### RemoveFlaggedFeaturesFilter-D8

| Field | Value |
|---|---|
| **Deviation ID** | `RemoveFlaggedFeaturesFilter-D8` |
| **Filter UUID** | `6e8cc6ec-8b9b-402e-9deb-85bd1cdba743` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** Extraction left the internal `tempBounds` array in the source and copied it into extracted feature data.

**Root cause:** bug. The bounds sub-filter created an array in the Feature Attribute Matrix, and extraction left it there during cropping. The current algorithm copies bounds into a local buffer and removes the array before cropping. It also removes the array when bounds execution fails. DREAM.3D kept its bounds in a private array.

**Affected users:** Users who extract features and inspect, save or process the resulting feature arrays.

**Recommendation:** Trust SIMPLNX. Extracted feature data no longer contains the internal bounds array.

**Evidence:** All release versions through v7.4.2 read `TempBoundsPath` without removing it. PR #1736 adds the cleanup. `Class 1 Oracle - Extract crops the bounding box of each flagged feature` asserts that `tempBounds` is absent from source and output.

### RemoveFlaggedFeaturesFilter-D9

| Field | Value |
|---|---|
| **Deviation ID** | `RemoveFlaggedFeaturesFilter-D9` |
| **Filter UUID** | `6e8cc6ec-8b9b-402e-9deb-85bd1cdba743` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** Preflight reported the same NeighborList deletion through two warnings.

**Root cause:** bug. The filter emitted `-11505` and then invoked the shared helper, which emitted `-5558`. The current preflight uses only the helper for operations that remove features. DREAM.3D did not use this pair of warnings.

**Affected users:** Users who remove features from a Feature Attribute Matrix with NeighborLists.

**Recommendation:** Trust SIMPLNX. One warning identifies the NeighborLists that removal will delete; extraction alone keeps them.

**Evidence:** Every release filter source through v7.4.2 contains both the local warning and `NeighborListRemovalPreflightCode`. PR #1736 removes the local warning. `Preflight Warning - NeighborLists are removed (-5558)` checks one warning for both removal modes and none for extraction.
