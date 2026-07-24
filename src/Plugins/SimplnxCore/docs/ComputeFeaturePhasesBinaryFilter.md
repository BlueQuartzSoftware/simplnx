# Compute Feature Phases Binary

## Group (Subgroup)

Generic (Misc)

## Description

This **Filter** assigns a binary **Ensemble** Id number to each **Cell** based on a boolean (true/false) mask. A **Cell** is a single voxel of an **Image Geometry**, and an **Ensemble** is a group of **Cells** that share a common phase or classification. The input is a single-component boolean mask array; the output is a single-component **Ensemble** array in which every *true* **Cell** is assigned **Ensemble** 1 and every *false* **Cell** is assigned **Ensemble** 0.

This **Filter** is generally useful any time data has already been reduced to a two-class (binary) distinction and you need an integer **Ensemble** label for downstream processing — for example separating "selected" from "unselected", "inside" from "outside", or any pass/fail criterion. A common materials-science example is an image segmented into precipitates and non-precipitates: the precipitates (mask *true*) are assigned to **Ensemble** 1 and the non-precipitates (mask *false*) to **Ensemble** 0.

### Required Input Sources

- **Mask** -- the per-**Cell** boolean mask array, typically produced by [Multi-Threshold Objects](MultiThresholdObjectsFilter.md).

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
