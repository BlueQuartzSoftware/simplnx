# Compute Feature Neighbor C-Axis Misalignments

## Group (Subgroup)

Statistics (Crystallography)

## Description

This **Filter** computes the C-axis misalignment angle between each **Feature** (grain) and its neighbors in hexagonal materials. The C-axis misalignment is the angular difference between the C-axis directions of two neighboring grains -- it measures how closely their unique crystallographic axes are aligned.

This is distinct from a full misorientation, which considers the complete rotational difference between two crystal orientations. C-axis misalignment only considers the alignment of the [001] direction, which is often the most mechanically significant axis in hexagonal materials.

### Flexible Neighbor Definition

The neighbor list is supplied by the user and can come from any filter that produces neighbor relationships. For example:
- [Compute Feature Neighbors](../SimplnxCore/ComputeFeatureNeighborsFilter.md) -- neighbors that physically share a boundary
- [Compute Feature Neighborhoods](../SimplnxCore/ComputeNeighborhoodsFilter.md) -- neighbors within a specified radius
- Any other custom filter that generates a neighbor list

The filter also optionally computes the average misalignment across all of a feature's neighbors.

### Hexagonal Materials Only

Only **Features** with identical phase values and a crystal structure of **Hexagonal_High** (6/mmm) or **Hexagonal_Low** (Hexagonal 6/m (C6h)) are compared. If two **Features** have different phases or a non-hexagonal crystal structure, a value of NaN is stored for the misalignment. See the [Compute Average C-Axis Orientations](ComputeAvgCAxesFilter.md) documentation for an explanation of why C-axis calculations are restricted to hexagonal materials.

### Note

Results from this filter may differ from the original DREAM.3D 6.5 version by approximately 0.0001 degrees due to the use of double precision and Eigen for matrix operations.

### Required Input Sources

- **Neighbor List** -- produced by [Compute Feature Neighbors](../SimplnxCore/ComputeFeatureNeighborsFilter.md) or [Compute Feature Neighborhoods](../SimplnxCore/ComputeNeighborhoodsFilter.md).
- **Average Quaternions** -- produced by [Compute Average Orientations](ComputeAvgOrientationsFilter.md).
- **Feature Phases** -- produced by [Compute Feature Phases](../SimplnxCore/ComputeFeaturePhasesFilter.md).
- **Crystal Structures** -- ensemble-level array read from EBSD data or created by [Create Ensemble Info](CreateEnsembleInfoFilter.md).

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

+ `EBSD_Hexagonal_Data_Analysis`

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
