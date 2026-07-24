# Fill Bad Data

## Group (Subgroup)

Processing (Cleanup)

## Description

This **Filter** distinguishes small *bad-data noise* from large *real defects* in a segmented dataset and fills only the noise. **Cells** with *Feature Id = 0* are considered *bad*; the filter groups them into connected components, classifies each component by its size, and fills components below the user threshold by copying the most common neighbor's data into them. Components at or above the threshold are left intact, since they likely represent real features (pores, cracks, voids).

| Small IN100 Before                   | Small IN100 After                   |
|--------------------------------------|-------------------------------------|
| ![](Images/fill_bad_data_before.png) | ![](Images/fill_bad_data_after.png) |

The above images show the before and after results of running this filter with a minimum defect size of 1000 voxels. The all-black overscan area around the sample exceeds the 1000-voxel threshold, so it is correctly preserved.

### How This Filter Works

1. **Connected components.** All bad cells (*Feature Id = 0*) are grouped into connected regions using face neighbors.
2. **Classify by size.**
   - Components with fewer than *Minimum Allowed Defect Size* cells are flagged as *small noise* and filled.
   - Components with at least *Minimum Allowed Defect Size* cells are kept as bad data, or optionally moved to a new phase if *Store Defects as New Phase* is enabled.
3. **Iterative fill.** Each cell flagged for filling examines its 6 face neighbors and copies all cell-level array data from whichever neighbor's *Feature Id* is most common. The pass repeats until every flagged cell has been filled.

### Minimum Defect Size Units

The *Minimum Allowed Defect Size* is in **cells** (integer voxel count), not physical units. To convert a physical-volume threshold into a cell count, divide by the cell volume (dx * dy * dz).

Choose the threshold based on what you want to keep:

- For removing single-cell or small-cluster EBSD scan noise, **5-50 cells** is typical.
- For preserving real pores while cleaning small noise (as in the example above), thresholds of **500-5000 cells** depend on pore size.

### Store Defects as New Phase

When this option is enabled, any connected component that meets or exceeds the size threshold has its cells reassigned to a new phase index rather than being left as Feature Id 0. This makes large defects available to downstream filters as a distinct phase (e.g., for separate statistics or visualization).

### Performance Note

The implementation is chunk-sequential and optimized for out-of-core data. For large datasets stored on disk, expect 10-100x speedups over random-access algorithms.

## WARNING: Feature Data Will Become Invalid

By modifying cell-level data, any feature-level data that was previously computed (sizes, centroids, average orientations, etc.) will most likely be invalid after this filter runs. Re-run any downstream feature-level computation filters to ensure accurate results.

### Required Input Sources

- **Cell Feature Ids** -- produced by a segmentation filter such as [Segment Features (Misorientation)](../OrientationAnalysis/EBSDSegmentFeaturesFilter.md) or [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md). Cells with Feature Id = 0 are treated as bad.
- **Cell Phases** -- typically read from EBSD data; only required when *Store Defects as New Phase* is enabled.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (02) SmallIN100 Full Reconstruction

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
