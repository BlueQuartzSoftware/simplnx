# Compute Feature Phases Binary

## Group (Subgroup)

Generic (Misc)

## Description

This **Filter** assigns phase 0 or 1 to each **Feature**. A Feature is a group of Cells with the same Feature Id. An **Ensemble** is a group of Features with the same phase. The output is a single-component int32 array in the selected *Cell Feature Attribute Matrix*. Its tuple count and shape match that matrix. Phase labels have no units.

The mask must have one component and use boolean or uint8 values. Zero means phase 0. Any nonzero value means phase 1. The location of the selected mask determines the mode.

### Cell Mask

If the mask's parent Attribute Matrix is different from the selected Feature Attribute Matrix, the filter uses Cell-mask mode. The mask and *Cell Feature Ids* must have the same tuple count.

Cells are processed in array order. Each Cell sets the phase of its Feature. If Cells in a Feature have different mask values, the last Cell decides the phase. Features with no Cells keep phase 0. Feature Id 0 is processed in the same way as other Feature Ids. Each Feature Id must be at least 0 and less than the Feature Attribute Matrix tuple count.

### Feature Mask

If the mask's parent Attribute Matrix is the selected Feature Attribute Matrix, the filter uses Feature-mask mode. Each mask value sets the phase of the corresponding Feature. This includes Feature 0.

*Cell Feature Ids* values are not read in this mode. The parameter still requires a valid single-component int32 array selection, but its tuple count and values do not affect the result. Alternative paths to the same Feature Attribute Matrix select the same mode.

### Cell Ensemble Attribute Matrix

*Create Cell Ensemble Attribute Matrix* is off by default. When enabled, the filter creates an empty Attribute Matrix with 2 tuples, one for each phase. It is a sibling of the selected Feature Attribute Matrix, under the same parent. The default name is *Cell Ensemble Data*. Choose a name that does not already exist at that location.

### Required Input Sources

- **Mask** -- a boolean or uint8 array, for example from [Multi-Threshold Objects](MultiThresholdObjectsFilter.md).
- **Cell Feature Ids** -- the Feature labels from segmentation, used only in Cell-mask mode.

## Algorithm

Both modes use bulk reads in chunks of at most 1,000,000 tuples. Cell-mask mode also keeps a Feature-sized phase cache and writes it after processing the Cells. Feature-mask mode converts and writes each chunk directly. Neither mode needs a full Cell-sized temporary array.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
