# Create Feature Array from Element Array

## Group (Subgroup)

Core (Memory/Management)

## Description

This **Filter** collapses a per-**Cell** (element) array down to a per-**Feature** array by storing one value per Feature. The value stored for each Feature is the **last cell value visited** during the scan -- not an average or majority vote.

## WARNING: Lossy for Within-Feature Variation

If the source cell array varies across cells *within* the same Feature, almost all of those values are discarded -- only the last cell scanned wins. **This filter is the correct choice only when every cell in a given Feature is guaranteed to hold the same value** (e.g., a cell-level phase array where every cell of a grain shares the same phase).

For situations where you want a meaningful per-Feature summary (mean, max, min, median, etc.) of a cell-level array, use:

- [Compute Array Statistics](ComputeArrayStatisticsFilter.md) -- computes mean, std dev, min, max, etc. per Feature.
- [Compute Average Orientations](../OrientationAnalysis/ComputeAvgOrientationsFilter.md) -- per-Feature average orientation specifically.

### When This Filter Is Appropriate

- Copying a uniform-within-feature cell-level array (cell phases, cell ensemble labels) to its Feature-level equivalent.
- Quick "any one example" extraction for diagnostic purposes.

### Required Input Sources

- **Cell Array to Copy** -- a cell-level array whose values are uniform within each Feature.
- **Cell Feature Ids** -- produced by a segmentation filter such as [Segment Features (Misorientation)](../OrientationAnalysis/EBSDSegmentFeaturesFilter.md).

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
