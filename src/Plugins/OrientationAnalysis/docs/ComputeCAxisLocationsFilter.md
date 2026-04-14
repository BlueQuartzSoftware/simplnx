# Compute C-Axis Locations

## Group (Subgroup)

Statistics (Crystallography)

## Description

This **Filter** determines the direction <u,v,w> of the C-axis for each **Element** by applying the quaternion of the **Element** to the <001> direction, which is the C-axis for *Hexagonal* materials.  This will tell where the C-axis of the **Element** sits in the *sample reference frame*.

*Note:* This **Filter** will only work properly for *Hexagonal* materials.  The **Filter** does not apply any symmetry operators because there is only one c-axis (<001>) in *Hexagonal* materials and thus all symmetry operators will leave the c-axis in the same position in the sample *reference frame*.  However, in *Cubic* materials, for example, the {100} family of directions are all equivalent and the <001> direction will change location in the *sample reference frame* when symmetry operators are applied.

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

EBSD_Hexagonal_Data_Analysis

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
