# Compute Feature Reference C-Axis Misalignments

## Group (Subgroup)

Statistics (Crystallography)

## Description

This **Filter** calculates the misorientation angle between the C-axis of each **Cell** within a **Feature** and the average C-axis for that **Feature** and stores that value for each **Cell**.  The average and standard deviation of those values for all **Cells** belonging to the same **Feature** is also stored for each **Feature**.

This filter requires at least one Hexagonal crystal structure phase (Hexagonal-Low 6/m or Hexagonal-High 6/mmm). Although it is not recommended, you can give input data with mixed phase types and all non hexagonal phases will be skipped in the calculations.

Results from this filter can differ from its original version in DREAM3D 6.6 by around 0.0001. This version uses double precision in part of its calculation to improve agreement and accuracy between platforms (notably ARM).

## Algorithm

For each cell, the quaternion is converted to an active rotation matrix (transpose of the passive orientation matrix) and applied to the <001> c-axis to find the c-axis direction in the sample reference frame. The angle between this cell-level c-axis and the feature's average c-axis is computed. Angles exceeding 90 degrees are folded to (180 - angle) to account for the antipodal symmetry of hexagonal c-axes. The per-cell misorientation values are then used to compute the average and population standard deviation per feature.

### In-Core Path

Cell-level arrays are iterated directly. Crystal structure validation ensures at least one hexagonal phase is present.

### Out-of-Core Path

Cell-level data (feature IDs, phases, quaternions) is processed one Z-slice at a time. For each slice, all input arrays are bulk-read via `copyIntoBuffer` into pre-allocated slice buffers, the misorientation is computed for every cell in the slice, and results are bulk-written via `copyFromBuffer`. The feature-level average c-axes array is cached locally at startup. Crystal structures are also cached from the ensemble-level array.

A second Z-slice pass re-reads the cell-level feature IDs and the just-written misorientation output to compute the population standard deviation per feature.

### Performance

The Z-slice processing pattern is well-suited for ImageGeom data where HDF5 chunks are often aligned by slice. Each slice is a single contiguous range in the linear cell index, so the bulk reads are sequential and cache-friendly. The two-pass approach (mean then standard deviation) avoids storing all cell values in memory simultaneously.

% Auto generated parameter table will be inserted here

## Example Pipelines

EBSD_Hexagonal_Data_Analysis

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
