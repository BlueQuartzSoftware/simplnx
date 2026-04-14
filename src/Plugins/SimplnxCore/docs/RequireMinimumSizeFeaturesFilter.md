# Remove Minimum Size Features

## Group (Subgroup)

Processing (Cleanup)

## Description

This **Filter** removes **Features** that have a total number of **Cells** below the minimum threshold defined by the user. Entering a number larger than the largest **Feature** generates an *error* (since all **Features** would be removed). Hence, a choice of threshold should be carefully chosen if it is not known how many **Cells** are in the largest **Features**. After removing all the small **Features**, the remaining **Features** are isotropically coarsened to fill the gaps left by the small **Features**.

The **Filter** can be run in a mode where the minimum number of neighbors is applied to a single **Ensemble**.  The user can select to apply the minimum to one specific **Ensemble**.

## Algorithm

The algorithm operates in two phases:

1. **Feature Removal**: Features with fewer voxels than the threshold have their voxels' Feature IDs set to -1 (invalid). This is done using chunked bulk I/O (64K tuples per chunk).
2. **Gap Filling**: The resulting "holes" (voxels with Feature ID = -1) are iteratively filled by majority voting among each hole voxel's 6 face-neighbors. Each iteration assigns the most-voted valid neighbor's Feature ID. This repeats until no holes remain.

### Performance

This filter is optimized for out-of-core (OOC) data storage in both phases:

- **Feature Removal**: Feature IDs are read and written in 64K-tuple chunks via `copyIntoBuffer()` / `copyFromBuffer()`. Only chunks that contain modifications are written back.
- **Gap Filling**: A rolling 3-slice buffer holds the previous, current, and next Z-slices of Feature IDs in memory. All 6 face-neighbor reads come from these local buffers rather than per-element OOC DataStore access. Changed voxels are tracked in a compact list, and only those voxels have their data arrays updated, rather than scanning all voxels for each data array.

## WARNING: Feature Data Will Become Invalid

By modifying the cell level data, any feature data that was previously computed will most likely be invalid at this point. Filters that compute feature level data should be rerun to ensure accurate final results from your pipeline.

## WARNING: NeighborList Removal

If the Cell Feature AttributeMatrix contains any *NeighborList* data arrays, those arrays will be **REMOVED** because those lists are now invalid. Re-run the *Find Neighbors* filter to re-create the lists.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (02) Small IN100 Full Reconstruction


## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
