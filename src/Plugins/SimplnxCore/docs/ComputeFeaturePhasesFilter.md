# Compute Feature Phases

## Group (Subgroup)

Generic (Misc)

## Description

This **Filter** determines the **Ensemble** (phase) of each **Feature** by iterating over all **Elements** and writing `featurePhases[featureId] = cellPhase` for each cell. When all cells of a **Feature** share the same phase the result is unambiguous. When they differ, the last cell encountered (by ascending index order) wins and a warning is emitted listing up to 15 affected **Feature** IDs.

**Background feature:** Cells with `featureId == 0` are skipped. `featurePhases[0]` is never written and will always be `0`; downstream filters should not rely on its value.

**Errors:** The filter halts with an error if any cell phase value is negative, or if the **Cell Phases** and **Feature Ids** arrays have different tuple counts.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (02) Small IN100 Full Reconstruction
+ INL Export


## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
