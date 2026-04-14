# Compute Feature Reference Misorientations

## Group (Subgroup)

Statistics (Crystallography)

## Description

This **Filter** calculates the misorientation angle between each **Cell** within a **Feature** and a 
*reference orientation* for that **Feature**.  The user can choose the *reference orientation* to 
be used for the **Features** from a drop-down menu.  The options for the *reference orientation* are 
the average orientation of the **Feature** or the orientation of the **Cell** that is furthest from 
the *boundary* of the **Feature**.

Note: the average orientation of the **Feature** is a typical choice, but if the **Feature** has 
undergone plastic deformation and the amount of lattice rotation developed is of interest, then 
it may be more reasonable to use the orientation *near the center* of the **Feature** as it may 
not have rotated and thus serve as a better *reference orientation*.

### Reference Orientation

The *Reference Orientation* parameter provides the following choices:

- **Average Feature Orientation [0]**: Uses the average orientation of the **Feature** as the reference orientation for misorientation calculations.
- **Orientation Farthest from Feature Boundary [1]**: Uses the orientation of the **Cell** that is furthest from the boundary of the **Feature** (nearest to its Euclidean center) as the reference orientation.

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

## Algorithm

The algorithm calculates the crystallographic misorientation angle between each cell's quaternion and a reference orientation for its parent feature. The misorientation is computed using the LaueOps symmetry operators for the cell's crystal structure, and the result is stored in degrees.

When using the **Average Feature Orientation** reference, the pre-computed average quaternions are looked up per feature. When using the **Orientation Farthest from Feature Boundary** reference, a preliminary pass finds the cell with the maximum Euclidean distance from the grain boundary for each feature, and that cell's quaternion is used as the reference.

After computing per-cell misorientations, the filter also calculates the average misorientation across all cells belonging to each feature.

### In-Core Path

Input cell-level arrays (feature IDs, phases, quaternions) and the reference data (average quaternions or grain boundary distances) are accessed through the AbstractDataStore API. The per-cell misorientation output is written directly.

### Out-of-Core Path

All cell-level arrays are read in sequential 64K-tuple chunks via `copyIntoBuffer`. Crystal structures and average quaternions are cached locally at startup since they are ensemble-level and feature-level arrays respectively. For the boundary-distance reference mode, the center voxel identification pass also uses chunked reads of feature IDs and distance arrays.

Per-cell misorientation results are accumulated in a local buffer and bulk-written via `copyFromBuffer` one chunk at a time. Feature-level sums and counts are maintained in local vectors.

### Performance

The two-pass chunked design (center-finding pass for mode 1, then the main misorientation pass) ensures that cell-level data is always read sequentially. This avoids random page faults on HDF5-chunked DataStores and reduces I/O from millions of individual accesses to a few hundred bulk reads.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (05) SmallIN100 Crystallographic Statistics

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
