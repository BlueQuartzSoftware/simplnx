# Compute Feature Neighbor C-Axis Misalignments

## Group (Subgroup)

Statistics (Crystallography)

## Description

For each feature, the C-Axis misalignments are determined for each neighbor of the feature. The neighbor list is a variable that is passed in by the user. This "NeighborList" could have been generated from any other appropriate filter. This means that a neighbor list could represent all neighbors that are physically connected to the current feature (Find Feature Neighbors), within a certain radius of the feature (Compute Feature Neighborhoods) or any other custom filter.

There are 2 outputs from this filter:
- The list of misalignments
- Optionally the average of all misalignments.

**The misalignment values are stored as Degrees.**

### Notes

**NOTE:** Only features with identical phase values and a crystal structure of **Hexagonal_High** will be calculated. If two features have different phase values or a crystal structure that is *not* Hexagonal_High then a value of NaN is set for the misorientation.

Results from this filter can differ from its original version in DREAM.3D 6.5.171 by around 0.0001. This version uses double precision and Eigen for matrix operations which account for the differences in output.

## Algorithm

For each feature, the algorithm converts the feature's average quaternion to an active rotation matrix and applies it to the <001> c-axis to find the c-axis direction in the sample reference frame. It then iterates over the feature's neighbor list, performing the same c-axis computation for each neighbor. The misalignment angle between the two c-axes is computed via the arc-cosine of their dot product, folded to [0, 90] degrees. Only neighbor pairs where both features are hexagonal and share the same phase contribute valid values; mismatched pairs are assigned NaN. If requested, the average misalignment across all valid neighbors is also computed per feature.

### In-Core Path

Feature-level arrays (phases, average quaternions) and ensemble-level arrays (crystal structures) are accessed through the AbstractDataStore API. NeighborList data structures provide per-feature neighbor information.

### Out-of-Core Path

All feature-level arrays (phases, average quaternions) and the ensemble-level crystal structures are bulk-read into local `std::vector` caches at startup via `copyIntoBuffer`. The per-feature loop then operates entirely on these local caches with zero OOC virtual dispatch overhead. The optional average misalignment output is accumulated in a local buffer and bulk-written via `copyFromBuffer` at the end.

### Performance

Because this filter operates on feature-level data (thousands of entries, not millions of cells), the entire working set fits comfortably in memory. The local caching eliminates any OOC overhead from the hot nested loop over features and their neighbors.

% Auto generated parameter table will be inserted here

## Example Pipelines

EBSD_Hexagonal_Data_Analysis

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
