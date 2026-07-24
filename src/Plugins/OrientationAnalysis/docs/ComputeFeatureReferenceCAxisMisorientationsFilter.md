# Compute Feature Reference C-Axis Misalignments

## Group (Subgroup)

Statistics (Crystallography)

## Description

This **Filter** measures how much the C-axis orientation varies within each **Feature** (grain) in hexagonal materials. It does this by comparing the C-axis direction of each individual **Cell** (voxel) to the average C-axis direction for the grain it belongs to.

This metric is useful for characterizing intragranular orientation gradients -- regions within a grain where the crystal lattice has rotated relative to the grain's average orientation, which can indicate deformation, subgrain boundaries, or measurement noise.

### What This Filter Computes

For each **Cell**, the filter calculates the angle (in degrees) between that cell's C-axis and the average C-axis of its parent **Feature**. The filter then computes summary statistics per **Feature**:

- **Average misalignment** -- the mean C-axis deviation across all cells in the grain
- **Standard deviation** -- how much the deviation varies within the grain

A low average misalignment indicates a grain with a uniform C-axis orientation. A high value suggests significant internal orientation variation.

### Hexagonal Materials Only

This filter requires at least one hexagonal crystal structure phase (6/m or 6/mmm). Non-hexagonal phases are skipped. See the [Compute Average C-Axis Orientations](ComputeAvgCAxesFilter.md) documentation for an explanation of why C-axis calculations are restricted to hexagonal materials.

### Note

Results may differ from the DREAM3D 6.6 version by approximately 0.0001 degrees due to improved double-precision calculations for cross-platform accuracy.

### Required Input Sources

- **Cell Feature Ids** -- produced by a segmentation filter such as [Segment Features (Misorientation)](EBSDSegmentFeaturesFilter.md) or [Segment Features (C-Axis Misalignment)](CAxisSegmentFeaturesFilter.md).
- **Cell Quaternions** -- typically read from EBSD data via [Read H5EBSD](ReadH5EbsdFilter.md), [Read CTF Data](ReadCtfDataFilter.md), or [Read ANG Data](ReadAngDataFilter.md).
- **Cell Phases** -- typically read from EBSD data alongside the quaternions.
- **Average C-Axes** -- produced by [Compute Average C-Axis Orientations](ComputeAvgCAxesFilter.md).
- **Crystal Structures** -- ensemble-level array read from EBSD data or created by [Create Ensemble Info](CreateEnsembleInfoFilter.md).

% Auto generated parameter table will be inserted here

## Example Pipelines

+ `EBSD_Hexagonal_Data_Analysis`

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
