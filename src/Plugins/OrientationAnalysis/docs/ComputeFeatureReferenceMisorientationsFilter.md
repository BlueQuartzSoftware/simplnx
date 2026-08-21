# Compute Feature Reference Misorientations

## Group (Subgroup)

Statistics (Crystallography)

## Description

This **Filter** measures how much crystal orientation varies within each **Feature** (grain) by computing the misorientation angle between each **Cell** (voxel) and a chosen *reference orientation* for that **Feature**.

This is useful for detecting intragranular orientation gradients caused by plastic deformation, subgrain formation, or other processes that cause the crystal lattice to rotate within a grain.

### Choosing a Reference Orientation

The user selects the reference orientation from two options. The choice depends on the physical question being asked:

- **Average Feature Orientation** -- Uses the average orientation of all cells in the grain. This is the typical choice for characterizing overall orientation spread within a grain.
- **Orientation Farthest from Feature Boundary** -- Uses the orientation of the cell nearest to the Euclidean center of the grain (farthest from any boundary). If the grain has undergone plastic deformation, boundary regions tend to rotate more than the interior. Using the center cell as the reference provides a more stable baseline that better reveals the pattern of lattice rotation from the interior outward.

### Reference Orientation

The *Reference Orientation* parameter provides the following choices:

- **Average Feature Orientation [0]**: Uses the average orientation of the **Feature** as the reference orientation for misorientation calculations.
- **Orientation Farthest from Feature Boundary [1]**: Uses the orientation of the **Cell** that is furthest from the boundary of the **Feature** (nearest to its Euclidean center) as the reference orientation. Requires a `Boundary Euclidean Distances` array as input; that array is typically produced by the [Compute Euclidean Distance Map](../SimplnxCore/ComputeEuclideanDistMapFilter.md) filter upstream of this one.

### Output Units

The misorientation values in both output arrays (`Cell Reference Misorientations` and `Feature Average Misorientations`) are expressed in **degrees**, not radians.

### Mode 1 — Raster-order tie-break for the "farthest from boundary" voxel

When two or more voxels within a single feature share the same maximum `Boundary Euclidean Distances` value, the algorithm selects the voxel with the **latest linear (raster) index** as the feature's reference. This matches the legacy DREAM3D 6.5.171 behavior. In practice, ties are rare on real EBSD data and the choice between tied voxels has no qualitative impact on the resulting misorientation field; for synthetic inputs that deliberately tie distances, the reader should be aware that re-ordering the voxel layout would change which voxel is selected as the reference.

## IPF Colors <001> Direction

This is the data set's IPF colors by the <001> direction which shows the reader the relative
orientation gradients within each feature (grain).

![ComputeFeatureReferenceMisorientations_3.png](Images/ComputeFeatureReferenceMisorientations_3.png)

## Example Using Feature's Average Orientation

Using the `T12-MAI-2010/fw-ar-IF1-aptr12-corr.ctf` data set we can generate the following
data using the **Feature Average Orientation** choice.

![ComputeFeatureReferenceMisorientations_0.png](Images/ComputeFeatureReferenceMisorientations_0.png)

### Example Using Feature's Euclidean Center Voxel's Orientation

Using the same dataset, the algorithm will find the voxel that is the furthest from the
feature boundary, and use that voxel's orientation as the **reference orientation**.

![ComputeFeatureReferenceMisorientations_1.png](Images/ComputeFeatureReferenceMisorientations_1.png)

### Required Input Sources

- **Cell Feature Ids** -- produced by a segmentation filter such as [Segment Features (Misorientation)](EBSDSegmentFeaturesFilter.md).
- **Cell Quaternions** -- typically read from EBSD data via [Read H5EBSD](ReadH5EbsdFilter.md), [Read CTF Data](ReadCtfDataFilter.md), or [Read ANG Data](ReadAngDataFilter.md).
- **Cell Phases** -- typically read from EBSD data alongside the quaternions.
- **Average Quaternions** (for Average Feature Orientation mode) -- produced by [Compute Average Orientations](ComputeAvgOrientationsFilter.md).
- **Boundary Euclidean Distances** (for Orientation Farthest from Feature Boundary mode) -- produced by [Compute Euclidean Distance Map](../SimplnxCore/ComputeEuclideanDistMapFilter.md).
- **Crystal Structures** -- ensemble-level array read from EBSD data or created by [Create Ensemble Info](CreateEnsembleInfoFilter.md).

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (04) Small IN100 Crystallographic Statistics

## Related Filters

- [Compute Feature Reference C-Axis Misorientations](ComputeFeatureReferenceCAxisMisorientationsFilter.md) — the C-axis variant of this filter, used for hexagonal-phase reconstructions.
- [Compute Kernel Average Misorientations](ComputeKernelAvgMisorientationsFilter.md) — computes a per-voxel kernel average misorientation; complementary to this filter for grain-boundary characterization.
- [Compute Feature Neighbor Misorientations](ComputeFeatureNeighborMisorientationsFilter.md) — computes pairwise feature-to-neighbor misorientations.
- [Compute Euclidean Distance Map](../SimplnxCore/ComputeEuclideanDistMapFilter.md) — typical upstream filter that produces the `Boundary Euclidean Distances` array required by Mode 1.

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
