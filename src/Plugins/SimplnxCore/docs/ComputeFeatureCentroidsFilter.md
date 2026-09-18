# Compute Feature Centroids

## Group (Subgroup)

Generic (Morphological)

## Description

This **Filter** calculates the *centroid* of each **Feature** by determining the average X, Y, and Z position (in physical coordinates) of all the **Cells** belonging to the **Feature**. The per-cell coordinates are accumulated using Kahan compensated summation to limit floating-point round-off on features with large cell counts. An *Is Periodic* option is available: when enabled, a **Feature** that spans the full extent of an axis (touching both opposing faces of the **Image Geometry**) is treated as wrapping around to the opposite face, and its centroid on that axis is shifted by half the physical distance between the first and last cell centers. When *Is Periodic* is disabled, **Features** that intersect the outer surfaces of the sample will still have *centroids* calculated, but they will be *centroids* of the truncated part of the **Feature** that lies inside the sample.

A **Feature** with no **Cells** (an unused Feature Id) keeps a centroid of (0, 0, 0).

## Algorithm

The algorithm iterates over all voxels, accumulating each voxel's XYZ coordinate (computed from its flat index, the geometry origin, and spacing) into a per-feature Kahan sum. After all voxels are processed, each feature's centroid is the accumulated sum divided by the voxel count.

If *Is Periodic* is enabled, a post-processing step checks whether any feature's bounding box spans the full extent of a dimension (indicating it wraps around a periodic boundary) and adjusts the centroid accordingly.

### Performance

This filter is optimized for out-of-core (OOC) data storage. The Feature IDs array is read in fixed-size chunks (64K tuples) via `copyIntoBuffer()` rather than accessing each voxel individually. All accumulation is performed in plain `std::vector` buffers (Kahan sums, compensators, voxel counts, XYZ ranges) to avoid per-element virtual dispatch through the DataStore interface. The final centroids are written to the output array in a single `copyFromBuffer()` call.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (03) Small IN100 Morphological Statistics
+ InsertTransformationPhase
+ (06) SmallIN100 Synthetic

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
