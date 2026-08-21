# Compute Feature Largest Cross-Section Areas

## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** calculates the largest cross-sectional area on a user-defined plane for all **Features**. A **Feature** is a contiguous group of **Cells** (voxels) that share the same Feature Id; a **Cell** is a single voxel. The **Filter** simply iterates through all **Cells** (on each section) asking for the **Feature** that owns them. On each section, the count of **Cells** for each **Feature** is then converted to an area and stored as the *LargestCrossSection* if the area for the current section is larger than the existing *LargestCrossSection* for that **Feature**.

The area is reported in **square physical units**, not a raw cell count. For each section the per-**Feature** **Cell** count is multiplied by the in-plane voxel area (the product of the two **Image Geometry** spacing values that lie in the chosen plane, e.g. `spacing-X * spacing-Y` for the XY plane). The output `LargestCrossSections` array is a single-component `float32` array created in the **Cell Feature Attribute Matrix**, holding the largest cross-sectional area found for each **Feature**.

### Plane of Interest

The *Plane of Interest* parameter selects the plane along which cross-sections are computed. The filter iterates through all slices perpendicular to the remaining axis:

- **XY [0]**: Cross-sections are taken on planes perpendicular to the Z axis. Each Z slice is one cross-section.
- **XZ [1]**: Cross-sections are taken on planes perpendicular to the Y axis. Each Y slice is one cross-section.
- **YZ [2]**: Cross-sections are taken on planes perpendicular to the X axis. Each X slice is one cross-section.

### Required Input Sources

This filter requires a **Cell Feature Ids** array, typically produced by a segmentation filter such as [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md) or one of the misorientation-based segmentation filters in the OrientationAnalysis plugin.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
