# Deviations from DREAM3D 6.5.171: RemoveFlaggedFeaturesFilter

This file lists every documented behavioral difference between SIMPLNX `RemoveFlaggedFeaturesFilter` and its DREAM3D 6.5.171 equivalents, `RemoveFlaggedFeatures` (*Remove*) and `ExtractFlaggedFeatures` (*Extract*).

Entries are referenced by stable ID (`RemoveFlaggedFeaturesFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

On valid input (every FeatureId in range, at least one unflagged feature that owns cells, every flagged feature owning at least one cell) the removal and fill outputs of the two implementations are bit-identical on every fixture run, and the extracted geometry's dimensions, origin and cell arrays match. The deviations below concern inputs outside that envelope, two SIMPLNX-only defects that are fixed, and the packaging of extracted geometries.

---

## RemoveFlaggedFeaturesFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `RemoveFlaggedFeaturesFilter-D1` |
| **Filter UUID** | `6e8cc6ec-8b9b-402e-9deb-85bd1cdba743` |
| **Status** | retired 2026-08-13 (fixed in DREAM3D-NX v7.4.2 by PR #1700); regression test added 2026-09-02 |

**Symptom:** With *Fill-in Removed Features* enabled, DREAM3D-NX v7.0.0 through v7.4.1 never returned when a vacated cell's valid face neighbors all belonged to distinct features, or when it had a single valid neighbor. DREAM3D 6.5.171 completed and filled the cell. Reported as GitHub issue #1698.

**Root cause:** `bug` in SIMPLNX. The per-cell tally in `IdentifyNeighbors()` pushed a newly seen feature onto `discoveredFeatures` without incrementing its hit count, so a fill source was only recorded on the second sighting of the same feature. A cell with no repeated neighbor feature never received a source, `FindVoxelArrays()` copied nothing into it, it stayed at -1, and the `do { } while(shouldLoop)` loop repeated the same pass forever. The legacy `n[feature]++` tally counted the first sighting. On large 3D volumes most vacated cells have two face neighbors in the same feature, which hid the defect; it surfaces on thin volumes and on small or isolated removed regions.

**Affected users:** Anyone who enabled *Fill-in Removed Features* on DREAM3D-NX v7.0.0 through v7.4.1 with thin geometries (one slice or one voxel deep) or with small isolated removed features. The pipeline had to be killed; no output was written.

**Recommendation:** `trust SIMPLNX` v7.4.2 or later, whose output is identical to 6.5.171 on the issue's 5x2x1 case and on every other fixture in this V&V. Users on v7.4.1 or earlier must upgrade or disable fill.

---

## RemoveFlaggedFeaturesFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `RemoveFlaggedFeaturesFilter-D2` |
| **Filter UUID** | `6e8cc6ec-8b9b-402e-9deb-85bd1cdba743` |
| **Status** | retired 2026-09-02 (fixed on the V&V branch) |

**Symptom:** With *Fill-in Removed Features* enabled, DREAM3D-NX v7.0.0 through v7.4.2 never returned when the Feature Ids array contained any cell with FeatureId 0. DREAM3D 6.5.171 completed and left the background cells at 0.

**Root cause:** `bug` in SIMPLNX. `IdentifyNeighbors()` skipped cells with `featureName > 0`, so background cells (0) were treated as unresolved and set `shouldLoop`, but `FindVoxelArrays()` only copies into cells with `featureName < 0`. Background cells were therefore never overwritten and every pass ended with `shouldLoop == true`. The legacy loop tests `featurename < 0`. SIMPLNX now tests `featureName >= 0 -> skip`, matching 6.5.171: background is never a fill target but remains a legal fill source (see the note at the end of the V&V report).

**Affected users:** Anyone who enabled fill on data with unassigned (background) cells, which is the common case after a segmentation with a mask. The pipeline had to be killed.

**Recommendation:** `trust SIMPLNX` at or after this fix; its output is identical to 6.5.171 on the 4x4x1 background fixture. Users on v7.4.2 or earlier must disable fill on data with background cells.

---

## RemoveFlaggedFeaturesFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `RemoveFlaggedFeaturesFilter-D3` |
| **Filter UUID** | `6e8cc6ec-8b9b-402e-9deb-85bd1cdba743` |
| **Status** | active |

**Symptom:** With fill enabled, when every cell belongs to a flagged feature and the unflagged features own no cells, SIMPLNX returns error `-45436` and leaves the cells at -1; DREAM3D 6.5.171 loops forever. Reproduced on a 4x1x1 volume with `FeatureIds = [1, 1, 1, 1]`, three feature tuples, and only feature 1 flagged: 6.5.171 was still running when killed after 30 seconds.

**Root cause:** `bug` in DREAM3D 6.5.171 (and in SIMPLNX before this branch). The all-flagged guard only inspects the flag array, so an unflagged feature that owns no cells satisfies it. After marking, no cell is non-negative, so no vacated cell ever finds a fill source and neither implementation detected the lack of progress. SIMPLNX now tracks whether a pass recorded any fill source and stops with a deterministic error when vacated cells remain without one. This is the same class of defect as `RequireMinNumNeighborsFilter-D3`.

**Affected users:** Users whose feature Attribute Matrix contains features that own no cells, when the flag array removes every feature that does own cells. Valid inputs are unaffected.

**Recommendation:** `trust SIMPLNX`. The error names the array and explains the two ways to correct the input; the legacy behavior is an unbounded execution.

---

## RemoveFlaggedFeaturesFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `RemoveFlaggedFeaturesFilter-D4` |
| **Filter UUID** | `6e8cc6ec-8b9b-402e-9deb-85bd1cdba743` |
| **Status** | active |

**Symptom:** When a FeatureId is negative or not less than the feature tuple count, SIMPLNX returns error `-45435` before modifying anything; DREAM3D 6.5.171 indexes its flag vector out of bounds. In the A/B run (5x2x1, cell 6 set to 5 with five feature tuples) 6.5.171 completed and silently zeroed that cell, which depends on whatever memory followed the vector.

**Root cause:** `bug` in DREAM3D 6.5.171 (and in SIMPLNX before this branch). `remove_flaggedfeatures()` evaluates `activeObjects[gnum]` for every cell without validating `gnum`. SIMPLNX now validates every FeatureId against the flag count in a read-only pass before the marking pass. This is the same class of defect as `RequireMinNumNeighborsFilter-D2`.

**Affected users:** Users with a Feature Ids array that does not correspond to the selected feature Attribute Matrix, for example after selecting the wrong Attribute Matrix or after a partial renumbering. Valid inputs are unaffected.

**Recommendation:** `trust SIMPLNX`. The error names the cell, the value and the valid range, and no data is modified.

---

## RemoveFlaggedFeaturesFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `RemoveFlaggedFeaturesFilter-D5` |
| **Filter UUID** | `6e8cc6ec-8b9b-402e-9deb-85bd1cdba743` |
| **Status** | retired 2026-09-02 (fixed on the V&V branch) |

**Symptom:** In *Extract* and *Extract then Remove*, DREAM3D-NX v7.0.0 through v7.4.2 ignored a failure of the bounding-box or crop sub-filter's execute step and continued with unset bounds, and reported a sub-filter preflight failure by throwing `std::runtime_error` rather than returning a filter error. Not applicable to 6.5.171, whose extract filter calls `CropImageGeometry` directly and checks no result.

**Root cause:** `bug` in SIMPLNX. After each `filter.execute()` the code tested `preflightResult.outputActions.invalid()` instead of `executeResult.result.invalid()`. SIMPLNX now returns errors `-53901`/`-53902` (bounding box preflight/execute) and `-53903`/`-53904` (crop preflight/execute) carrying the sub-filter's own message.

**Affected users:** Users whose extract run hit a sub-filter failure, chiefly the empty-feature case in D6. Successful extractions were unaffected.

**Recommendation:** `trust SIMPLNX`. Failures are now reported as ordinary filter errors with the cause attached.

---

## RemoveFlaggedFeaturesFilter-D6

| Field | Value |
|---|---|
| **Deviation ID** | `RemoveFlaggedFeaturesFilter-D6` |
| **Filter UUID** | `6e8cc6ec-8b9b-402e-9deb-85bd1cdba743` |
| **Status** | active |

**Symptom:** When a flagged feature owns no cell, SIMPLNX emits warning `-53905` and creates no geometry for it; DREAM3D 6.5.171 creates a 1x1x1 geometry named `Feature_<id>` at origin (-1, -1, -1) that contains cell 0 of the source volume. Reproduced on the 4x4x1 fixture with an unused fifth feature flagged: 6.5.171 produced `Feature_4` with `FeatureIds = [0]` and `CellValue = [100]`. DREAM3D-NX v7.0.0 through v7.4.2 threw an uncaught exception on the same input (see D5).

**Root cause:** `bug` in DREAM3D 6.5.171 (bogus output) and in SIMPLNX before this branch (uncaught exception). Legacy `find_feature_bounds()` initializes every bound to -1 and only updates bounds for features that own cells, so an empty feature keeps (-1, -1, -1) to (-1, -1, -1), which `CropImageGeometry` accepts as a one-voxel crop. In SIMPLNX `ComputeFeatureRect` leaves minimum > maximum for an empty feature; the crop preflight rejected that and the algorithm threw. SIMPLNX now detects minimum > maximum, warns, and skips the feature.

**Affected users:** Users extracting from a feature Attribute Matrix with unused tuples, for example after a filter that removed features without compacting, or with a flag array that marks unused ids. The legacy output contains a geometry that does not correspond to any feature.

**Recommendation:** `trust SIMPLNX`. The warning names the feature and the array; no spurious geometry is written.

---

## RemoveFlaggedFeaturesFilter-D7

| Field | Value |
|---|---|
| **Deviation ID** | `RemoveFlaggedFeaturesFilter-D7` |
| **Filter UUID** | `6e8cc6ec-8b9b-402e-9deb-85bd1cdba743` |
| **Status** | active |

**Symptom:** An extracted geometry is named `<Created Image Geometry Prefix>-<id>` with the id zero-padded to the width of the feature count (for example `Extracted_Feature-03` when there are 12 feature tuples) and carries a copy of the source feature Attribute Matrix; DREAM3D 6.5.171 names it `Feature_<id>` with no padding and copies only the cell Attribute Matrix. The geometry's dimensions, origin, spacing and cell arrays are identical in both.

**Root cause:** `algorithmic choice`. SIMPLNX exposes the prefix as a parameter and pads so that geometries sort in id order; it reuses `CropImageGeometryFilter` with feature renumbering off, which copies the feature Attribute Matrix so feature-level attributes stay available in the extracted geometry.

**Affected users:** Pipelines or scripts that reference extracted geometries by the legacy `Feature_<id>` name, and users who expect the extracted geometry to contain only cell data.

**Recommendation:** `trust SIMPLNX`. Set the prefix to `Feature_` for a closer match; note that the id is still zero-padded and the feature Attribute Matrix is still carried.
