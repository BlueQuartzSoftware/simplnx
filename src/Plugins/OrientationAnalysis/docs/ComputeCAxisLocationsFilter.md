# Compute C-Axis Locations

## Group (Subgroup)

Statistics (Crystallography)

## Description

This **Filter** computes the C-axis direction for each individual **Cell** (voxel) in hexagonal materials. The result is a unit vector per **Cell** that indicates where that cell's C-axis points in the sample reference frame.

### What is the C-Axis?

In hexagonal crystal structures (such as titanium, magnesium, and zinc), the *C-axis* is the unique crystallographic direction that runs along the long axis of the hexagonal unit cell (the [001] direction). This axis is important because many mechanical and physical properties of hexagonal materials vary depending on whether they are measured along or perpendicular to the C-axis.

![Fig. 1: The C-axis in a hexagonal unit cell.](Images/ComputeAvgCAxes_HexagonalCAxis.png)

### How This Filter Works

Each **Cell** has a measured crystal orientation stored as a quaternion. This filter uses that orientation to rotate the crystal [001] direction into the physical sample coordinate system, producing a 3D unit vector that describes where the C-axis points.

The output vectors are normalized to unit length and constrained to the upper hemisphere (positive Z component) to provide a unique, unambiguous representation.

### Hexagonal Materials Only

This filter only produces valid results for hexagonal phases (6/mmm or 6/m symmetry). In hexagonal materials, the C-axis is unique -- there is only one [001] direction, so crystal symmetry does not create ambiguity.

In cubic materials, the [001], [010], and [100] directions are all crystallographically equivalent. Applying symmetry operators would move the [001] direction to different positions in the sample frame, making this computation ambiguous. For this reason, **non-hexagonal phases will have their output values set to NaN**.

### Comparison with Compute Average C-Axis Orientations

This filter computes the C-axis direction for each individual **Cell**. If you need the average C-axis direction for each **Feature** (grain) instead, use the [Compute Average C-Axis Orientations](ComputeAvgCAxesFilter.md) filter.

### Required Input Sources

- **Cell Quaternions** -- typically read from EBSD data via [Read H5EBSD](ReadH5EbsdFilter.md), [Read CTF Data](ReadCtfDataFilter.md), or [Read ANG Data](ReadAngDataFilter.md); can also be produced from Euler angles by [Convert Orientations](ConvertOrientationsFilter.md).
- **Cell Phases** -- typically read from EBSD data alongside the quaternions.
- **Crystal Structures** -- ensemble-level array read from EBSD data or created by [Create Ensemble Info](CreateEnsembleInfoFilter.md).

## Algorithm

For each element, the quaternion is converted to an orientation matrix. The matrix is transposed (converting the passive rotation to an active rotation) and multiplied by the crystallographic c-axis <001> to produce the c-axis direction in the sample reference frame. The result is normalized, and if the z-component is negative the vector is flipped to ensure consistent hemisphere placement. Non-hexagonal phases produce NaN output values.

### In-Core Path

Input quaternions, cell phases, and the output c-axis locations array are accessed through the AbstractDataStore API.

### Out-of-Core Path

Cell-level data is processed in sequential 64K-tuple chunks. Quaternions (4 components) and phases (1 component) are bulk-read via `copyIntoBuffer`, the c-axis is computed for each element in the chunk, and results (3 components) are bulk-written via `copyFromBuffer`. The ensemble-level crystal structures array is cached locally at startup.

### Performance

The simple per-element transform (quaternion to rotation matrix to c-axis vector) is compute-light, so I/O dominates the cost for OOC data. Chunked sequential access converts millions of virtual dispatch calls into a small number of contiguous reads and writes.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ `EBSD_Hexagonal_Data_Analysis`

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
