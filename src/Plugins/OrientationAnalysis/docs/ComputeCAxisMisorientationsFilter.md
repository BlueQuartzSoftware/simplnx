# Compute C-Axis Misorientations

## Group (Subgroup)

Statistics (Crystallography)

## Description

This **Filter** computes the angle between each **Feature**'s average C-axis direction and a user-supplied reference direction. The result is a scalar angle in degrees per **Feature**, indicating how closely the grain's C-axis is aligned with a direction of interest in the sample reference frame — such as a loading axis, rolling direction, or sample normal.

### Relationship to C-Axis Misalignment

This filter differs from [Compute Feature Neighbor C-Axis Misalignments](ComputeFeatureNeighborCAxisMisalignmentsFilter.md), which measures the angle *between neighboring grains*. This filter instead measures the angle *between each grain and a fixed reference direction*, making it useful for global texture analysis.

### Prerequisite

The **Average C-Axes** input must be produced by [Compute Average C-Axis Orientations](ComputeAvgCAxesFilter.md) before running this filter. The reference direction is specified in the sample reference frame.

### Hexagonal Materials Only

Because the C-axis is only well-defined in hexagonal crystal systems, [Compute Average C-Axis Orientations](ComputeAvgCAxesFilter.md) marks non-hexagonal **Features** with NaN. This filter propagates those NaN values directly to the output without additional phase checking.

### Result Range

The C-axis has antipodal symmetry — the [001] and [00-1] directions are crystallographically identical. The output is therefore always in **[0°, 90°]**, where 0° means the C-axis is parallel (or antiparallel) to the reference direction and 90° means it is perpendicular.

## Algorithm

For each **Feature**, the filter computes the absolute dot product of the (already-normalized) average C-axis vector with the normalized reference direction, then takes the arc-cosine and converts to degrees:

```
angle = arccos( |avgCAxis · refDir| ) × (180 / π)
```

**Features** whose average C-axis is NaN (non-hexagonal phases) produce NaN in the output.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
