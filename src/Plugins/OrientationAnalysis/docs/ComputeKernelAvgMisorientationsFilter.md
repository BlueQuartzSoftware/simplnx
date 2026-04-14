# Compute Kernel Average Misorientations

## Group (Subgroup)

Statistics (Crystallography)

## Description

This **Filter** determines the Kernel Average Misorientation (KAM) for each **Cell**.  The user can select the size of the kernel to be used in the calculation.  The kernel size entered by the user is the *radius* of the kernel (i.e., entering values of *1*, *2*, *3* will result in a kernel that is *3*, *5*, and *7* **Cells** in size in the X, Y and Z directions, respectively).  The algorithm for determination of KAM is as follows:

1. Calculate the misorientation angle between each **Cell** in a kernel and the central **Cell** of the kernel
2. Average all of the misorientations for the kernel and store at the central **Cell**

The calculation will **not** consider cells that belong to different 'feature Ids', ie.e, different grains.

*Note:* All **Cells** in the kernel are weighted equally during the averaging, though they are not equidistant from the central **Cell**.

## Algorithm

For each cell in the ImageGeom, the algorithm examines all cells within a user-specified kernel radius in X, Y, and Z. Only neighbor cells that share the same feature ID as the center cell are included. The crystallographic misorientation angle between the center cell's quaternion and each qualifying neighbor's quaternion is computed using the appropriate LaueOps symmetry operators. The average of these misorientation angles is stored as the KAM value for the center cell.

### In-Core Path

All cell-level arrays (phases, feature IDs, quaternions) are accessed through the AbstractDataStore API. The output array is written directly.

### Out-of-Core Path

The algorithm processes data one Z-plane at a time. For each plane, a slab of input data spanning `[plane - kernelZ, plane + kernelZ]` is bulk-read via `copyIntoBuffer`. This slab contains all data needed for neighbor lookups of cells in the current plane. The crystal structures array is cached locally at startup. Output values for each plane are accumulated in a local buffer and bulk-written via `copyFromBuffer`.

### Performance

The slab-based approach is critical for KAM because each cell needs random access to its neighbors within the kernel radius. By reading the entire slab into memory, all neighbor lookups become local memory accesses rather than individual OOC page faults. The slab size is bounded by `(2 * kernelZ + 1) * sliceSize`, which is manageable even for large kernel radii. Sequential plane processing ensures the data is read in order through the volume.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ MassifPipeline
+ (05) SmallIN100 Crystallographic Statistics

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
