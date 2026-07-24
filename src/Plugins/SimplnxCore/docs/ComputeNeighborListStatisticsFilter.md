# Compute Neighbor List Statistics

## Group (Subgroup)

Statistics (Misc Filters)

## Description

This **Filter** reduces a **NeighborList** -- a per-feature list of values, one entry per neighbor -- into per-feature scalar summary statistics. Each chosen statistic becomes its own output **Attribute Array** at the feature level.

A typical use is summarizing per-feature neighbor misorientations: [Compute Feature Neighbor Misorientations](../OrientationAnalysis/ComputeFeatureNeighborMisorientationsFilter.md) produces a NeighborList containing the misorientation between a feature and each of its neighbors; this filter reduces that list to "average misorientation per feature" or "max neighbor misorientation per feature" so the result can be plotted or thresholded.

### Available Statistics

The user can independently enable any combination of:

- **Length** -- the number of entries in the list (i.e., the number of neighbors).
- **Minimum** -- smallest value in the list.
- **Maximum** -- largest value in the list.
- **Mean** -- arithmetic mean of the list.
- **Median** -- median of the list.
- **Standard Deviation** -- sample standard deviation of the list.
- **Summation** -- sum of all values in the list.

Output statistics inherit the units of the input list (e.g., degrees for a misorientation list, dimensionless count for a neighbor count list).

### Required Input Sources

- **Input NeighborList** -- a feature-level list-of-lists. Common producers include [Compute Feature Neighbors](ComputeFeatureNeighborsFilter.md) (shared surface area), [Compute Feature Neighbor Misorientations](../OrientationAnalysis/ComputeFeatureNeighborMisorientationsFilter.md) (misorientation per neighbor), [Compute Feature Neighbor C-Axis Misalignments](../OrientationAnalysis/ComputeFeatureNeighborCAxisMisalignmentsFilter.md), or [Compute Slip Transmission Metrics](../OrientationAnalysis/ComputeSlipTransmissionMetricsFilter.md).

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
