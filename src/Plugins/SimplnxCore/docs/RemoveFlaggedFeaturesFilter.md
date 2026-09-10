# Remove/Extract Flagged Features

## Group (Subgroup)

Processing (Cleanup)

## Description

This **Filter** removes or extracts **Features** that an earlier filter in the pipeline has flagged. The user supplies a boolean (or unsigned 8-bit) array at the **Feature** level. Features with a value of *true* are removed or extracted. This filter discards features by any criterion that a flag can express: biased features, surface features, minority phases, or user-selected features. For the specific case of removing features smaller than a size threshold, see [Remove Minimum Size Features](RequireMinimumSizeFeaturesFilter.md). For a rank-based selection ("remove the 10 smallest"), see [Keep/Remove Ranked Features](KeepRemoveRankedFeaturesFilter.md).

### Selected Operation

The *Selected Operation* parameter provides the following choices:

- **Remove [0]**: Remove the flagged **Features** from the geometry. Their **Cells** are set to 0 (background), or filled from neighboring features when *Fill-in Removed Features* is on. The feature **Attribute Matrix** is compacted and the surviving features are renumbered contiguously starting at 1.
- **Extract [1]**: Copy each flagged **Feature** into a new **Image Geometry** without modifying the original.
- **Extract then Remove [2]**: Extract first, from the unmodified data, and then remove.

### How Fill-in Removed Features Works

When *Fill-in Removed Features* is on, isotropic coarsening fills the vacated **Cells**. The neighboring features grow into the gap one layer per pass until no vacated cell remains.

1. Every **Cell** of a flagged **Feature** is marked as vacated.
2. Each vacated cell polls its six face neighbors in the order -Z, -Y, -X, +X, +Y, +Z. Neighbors outside the volume and neighbors that are themselves vacated are ignored.
3. The cell copies every **Cell** level array (except those listed in *Attribute Arrays to Ignore*) from the neighbor whose feature is the most common among the polled neighbors. A tie goes to the feature seen first in the order above. The *Cell Feature Ids* array is always copied, even if it is listed; the filter warns (*-45438*) and removes it from the list.
4. A vacated cell with no usable neighbor waits for the next pass, when its own neighbors have been filled.
5. Passes repeat until no vacated cell remains.

Background cells (FeatureId 0) are never vacated and are never filled, but they do count as neighbors: a vacated cell whose only usable neighbors are background becomes background. This matches DREAM3D 6.5.171 and the other coarsening filters.

A pass can fail to fill any remaining vacated cell. This happens only when every cell belonged to a flagged feature and the unflagged features own no cells. The filter then stops with error *-45436* instead of running forever.

### Extract Details

Each flagged **Feature** is cropped along its axis-aligned bounding box (in cells) into a new **Image Geometry** named `<Created Image Geometry Prefix>-<id>`. The id is zero-padded to the number of digits in the feature count so the geometries sort in order (for example *Extracted_Feature-03* when there are 12 features). The new geometry keeps the source spacing, has its origin at the bounding box corner, and carries every **Cell** array plus a copy of the feature **Attribute Matrix**. Features are not renumbered in the extracted geometry.

A flagged **Feature** that owns no **Cells** has nothing to extract. The filter emits warning *-53905* for it and creates no geometry.

### Input Validation

Every value in *Cell Feature Ids* must be in the range 0 through (number of feature tuples - 1), and the array must hold one value per **Cell** of the selected geometry. A value outside that range stops the filter with error *-45435*, and a tuple-count mismatch stops it with error *-45437*, before any data is modified. Flagging every **Feature** stops the filter with error *-45433*.

### WARNING: NeighborList Removal

When the operation is *Remove* or *Extract then Remove*, any *NeighborList* arrays in the feature **Attribute Matrix** are **removed** because the neighbor relationships have changed. Re-run [Compute Feature Neighbors](ComputeFeatureNeighborsFilter.md) afterward to rebuild them.

### Caveats

This filter will **only** run on an **Image Geometry**.

### Required Input Sources

- **Cell Feature Ids** -- produced by a segmentation filter such as [Segment Features (Misorientation)](../OrientationAnalysis/EBSDSegmentFeaturesFilter.md) or [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md).
- **Flagged Features** -- a boolean feature-level array. Typical producers: [Compute Biased Features (Bounding Box)](ComputeBiasedFeaturesFilter.md), [Compute Surface Features](ComputeSurfaceFeaturesFilter.md), or a custom flag built with threshold or boolean operations on any feature-level statistic.

% Auto generated parameter table will be inserted here

## Example Pipelines

None.

## Differences from DREAM3D 6.5.171

For valid input, the removal and fill outputs match DREAM3D 6.5.171 exactly, and extracted geometries have the same dimensions, origin and cell data. SIMPLNX differs in how it handles malformed input and in how it packages extracted geometries:

- [D3: Fill cannot make progress](../vv/deviations/RemoveFlaggedFeaturesFilter.md#removeflaggedfeaturesfilter-d3) -- SIMPLNX stops with error *-45436*; 6.5.171 runs forever.
- [D4: Feature ID outside the feature tuple range](../vv/deviations/RemoveFlaggedFeaturesFilter.md#removeflaggedfeaturesfilter-d4) -- SIMPLNX stops with error *-45435*; 6.5.171 reads out of bounds.
- [D6: Flagged feature with no cells on extract](../vv/deviations/RemoveFlaggedFeaturesFilter.md#removeflaggedfeaturesfilter-d6) -- SIMPLNX warns and skips it; 6.5.171 writes a spurious 1x1x1 geometry.
- [D7: Extracted geometry naming and contents](../vv/deviations/RemoveFlaggedFeaturesFilter.md#removeflaggedfeaturesfilter-d7) -- SIMPLNX uses `<prefix>-<zero-padded id>` and carries the feature **Attribute Matrix**; 6.5.171 uses `Feature_<id>` with cell data only.

DREAM3D-NX releases before the fixes described in [D1](../vv/deviations/RemoveFlaggedFeaturesFilter.md#removeflaggedfeaturesfilter-d1) (v7.4.1 and earlier) and [D2](../vv/deviations/RemoveFlaggedFeaturesFilter.md#removeflaggedfeaturesfilter-d2) (v7.4.2 and earlier) could run forever with *Fill-in Removed Features* on. Upgrade or disable fill on those versions.

## Related Filters

- [Remove Minimum Size Features](RequireMinimumSizeFeaturesFilter.md)
- [Require Minimum Number of Neighbors](RequireMinNumNeighborsFilter.md)
- [Keep/Remove Ranked Features](KeepRemoveRankedFeaturesFilter.md)
- [Compute Feature Neighbors](ComputeFeatureNeighborsFilter.md)

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
