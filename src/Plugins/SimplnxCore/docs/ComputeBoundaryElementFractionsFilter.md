# Compute Feature Boundary Element Fractions

## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** calculates, for each **Feature**, the fraction of its **Elements** that lie on the boundary ("surface") of that **Feature**. A **Feature** is a contiguous region of like-segmented cells (for example a grain), and an **Element** is a single cell of the geometry. An **Element** is considered a *surface* (boundary) **Element** when at least one of its neighboring **Elements** belongs to a different **Feature**. This boundary flag is not computed here; it is supplied through the *Surface Elements* input array, where any value greater than zero (the count of differing neighbors) marks the **Element** as a boundary **Element**.

The **Filter** iterates through every **Element**, asking which **Feature** owns it and whether it is a boundary **Element**. Each **Feature** accumulates the total number of **Elements** it owns and the number of those that are boundary **Elements**. The output for each **Feature** is the ratio:

> boundary Elements / total Elements

This is a dimensionless fraction in the range [0, 1]. A value near 0 means the **Feature** is large and compact relative to its surface; a value near 1 means nearly all of the **Feature**'s **Elements** touch a neighbor of a different **Feature** (a thin or small **Feature**).

### Required Input Sources

- **Cell Feature Ids** -- the Feature that owns each Element, produced by a segmentation filter such as [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md).
- **Surface Elements** -- the per-Element count of neighbors belonging to a different Feature, produced by [Compute Boundary Cells (Image)](ComputeBoundaryCellsFilter.md).

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
