# Compute Feature Centroids

## Group (Subgroup)

Generic (Morphological)

## Description

This **Filter** calculates the *centroid* of each **Feature** by averaging the physical X, Y, and Z positions of all its **Cells**. It uses Kahan compensated summation to limit floating-point round-off. With *Is Periodic* enabled, a feature touching both opposing faces of an axis uses the unique largest empty gap between occupied cell centers as its unwrapping seam. The filter then takes the arithmetic mean of all its unwrapped cell centers, including repeated centers from multiple cells, and maps the result back into the domain. If the largest gap is tied or every cell coordinate on that axis is occupied, the seam is ambiguous and the ordinary mean is retained. Axes with one cell and features that do not touch both faces also retain the ordinary mean. With *Is Periodic* disabled, features intersecting the sample boundary use the ordinary mean of their cells inside the sample.

A **Feature** with no **Cells** (an unused Feature Id) keeps a centroid of (0, 0, 0).

## Algorithm

The algorithm iterates over all voxels, accumulating each voxel's XYZ coordinate (computed from its flat index, the geometry origin, and spacing) into a per-feature Kahan sum. After all voxels are processed, each feature's centroid is the accumulated sum divided by the voxel count.

If *Is Periodic* is enabled, a post-processing step checks for features touching both faces of an axis. It records occupied coordinates in packed bitsets, scanning Feature IDs sequentially in batches or axis windows when the available working-memory grant is small. Occupancy determines the seam; a final sequential read uses the original cell counts to compute adjusted components. The filter requests up to 32 MiB of shared working allowance for occupancy and reduces batch or window size to the actual grant. This allowance covers the new occupancy payload only; the existing 256 KiB Feature-ID transfer buffer and feature-sized sums, ranges, output, and candidate metadata are additional memory. A periodic candidate requires at least one 8-byte occupancy word. If that minimum is unavailable, execution reports a resource error before publishing final centroids.

### Performance

The Feature IDs array is read in fixed-size chunks (64K tuples) via `copyIntoBuffer()` rather than accessing each voxel individually. Nonperiodic mode and periodic input with no boundary-spanning feature need only the original algorithm scan. If K occupancy batches or windows are needed, an adjusted result needs K+2 algorithm scans: the original sum, K occupancy scans, and one weighted correction scan. Smaller grants can increase K substantially. Feature-ID validation makes one separate scan. The final centroids are written in one `copyFromBuffer()` call.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (03) Small IN100 Morphological Statistics
+ InsertTransformationPhase
+ (06) SmallIN100 Synthetic

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
