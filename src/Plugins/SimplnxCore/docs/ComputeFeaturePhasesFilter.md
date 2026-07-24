# Compute Feature Phases

## Group (Subgroup)

Generic (Misc)

## Description

This **Filter** determines the **Ensemble** (phase) of each **Feature** by querying the **Ensemble** of the **Elements** that belong to the **Feature**. A **Feature** is a connected region of **Elements** sharing the same **Feature Id**; an **Element** is a single member of a geometry (for an **Image Geometry** this is a **Cell**, i.e. a voxel); and an **Ensemble** is a group of **Features** that share a common phase. Note that it is assumed that all **Elements** belonging to a **Feature** are of the same phase, and thus any **Element** can be used to determine the **Ensemble** of the **Feature** that owns that **Element**.

The output is a single-component, per-**Feature** phase array: tuple *i* holds the **Ensemble** (phase) Id of **Feature** *i*.

### Required Input Sources

- **Cell Feature Ids** -- the per-**Element** **Feature** label array, typically produced by [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md) or another segmentation filter.
- **Cell Phases** -- the per-**Element** phase array, typically read from EBSD data via [Read H5EBSD](../OrientationAnalysis/ReadH5EbsdFilter.md), [Read CTF Data](../OrientationAnalysis/ReadCtfDataFilter.md), or [Read ANG Data](../OrientationAnalysis/ReadAngDataFilter.md).

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (02) Small IN100 Full Reconstruction
+ INL Export


## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
