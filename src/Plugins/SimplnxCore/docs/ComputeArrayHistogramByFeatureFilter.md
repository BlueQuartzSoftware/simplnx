# Compute Attribute Array Frequency Histogram (Feature)

## Description

This **Filter** accepts one or more numeric **DataArray** inputs and computes per-**Feature** histograms. A **Feature** is a connected region of **Cells** that share the same **Feature Id** (the integer label assigned by a segmentation filter). For each selected array, the per-Cell values are grouped according to their owning **Feature Id**, then divided into equal-width bins—using either automatically derived or user-defined minimum/maximum bounds—and counted per bin.

Optionally, a mask **DataArray** can be provided to include only specific elements in the histogram computation.

Outputs include:
- **Counts DataArray**: One N-component tuple per feature where N is the number of bins and each component holds the histogram count for that feature.
  - *Example*: With 5 bins, feature 8's counts appear as the five components at tuple index 8.
- **Bin Ranges DataArray**: One (N*2)-component tuple per feature defining each bin's inclusive lower bound and exclusive upper bound.
  - *Example*: For 4 bins spanning [0,8), a feature's tuple is [0,2), [2,4), [4,6), [6,8) represented as `[0,2,2,4,4,6,6,8]`.
- **Most Populated Bin DataArray**: One tuple per feature with one component indicating the bin index containing the most values.
  - *Example*: If feature 3's third bin contains the most values, then tuple 3 has the value `2`.
- **Modal Bin Ranges INeighborList** (optional): One NeighborList that contains one list per feature specifying inclusive lower bound and exclusive upper bound containing the feature's mode(s). Because there can be multiple modes, each list may include more than 2 entries.
  - *Example*: list 5 contains `[0.5, 10.2, 41.3, 54.1]` if feature 5 has two mode values and they fall between 0.5 and 10.2 and between 41.3 and 54.1.

### Required Input Sources

- **Cell Feature Ids** -- the per-Cell **Feature** label array, typically produced by [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md) or another segmentation filter.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.