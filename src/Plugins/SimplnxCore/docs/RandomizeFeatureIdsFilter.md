# Randomize Feature Ids

## Group (Subgroup)

Core (Filters)

## Description

***WARNING:** This filter can throw a pipeline terminating error (at runtime) if the number of tuples in the supplied Feature `Attribute Matrix` is less than the max value in the Feature Ids `DataArray`*

This filter will randomize a user selected **Feature Ids** array and update every container (`DataArray`, `NeighborList`, and `StringArray`) in the Feature **Attribute Matrix**. This does not generate random data but instead uses the existing values and swaps the positions of the values in the array. The intended use case is primarily for visualization, so feature data does not appear as a smooth gradient.

**Feature Ids** are per-cell integer labels that assign each element (voxel) of a geometry to a *Feature* (for example, a grain). The Feature **Attribute Matrix** is the container holding one tuple per Feature; its per-Feature **Attribute Arrays** (such as sizes, phases, or average orientations) and any `NeighborList` (a variable-length list of each Feature's neighbors) are re-ordered consistently with the new Feature Id assignment so the data stays internally coherent.

This filter does not expose a random-seed parameter, so the shuffle is **not reproducible** from run to run. If you need a deterministic visualization, apply the randomization once and persist the result rather than re-running it.

### Required Input Sources

- A **Feature Ids** array, typically produced by a feature-segmentation filter such as [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md), and the matching Feature **Attribute Matrix** that array indexes into.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
