# Compute C-Axis Reference Misalignments

## Group (Subgroup)

Statistics (Crystallography)

## Description

Use this filter to separate **hard** and **soft** grains or cells in a hexagonal close-packed (HCP) material. A hard orientation has its crystal c-axis nearly parallel to the loading direction. A soft orientation has its c-axis nearly perpendicular to that direction. Set **Reference Direction** to the loading direction in the sample frame. The output gives the angle to that direction. This angle describes orientation relative to the load. It is not a full prediction of mechanical response.

The filter works on cell orientations, feature average orientations, or any other quaternion array. A **feature** usually represents a grain. Select a Phases array at the same level, with one phase index for each orientation. No geometry or Feature Ids array is required.

### Parameter Guidance

**Input Quaternions** must contain four-component float32 quaternions in EbsdLib order: x, y, z, w. Use cell *Quats* for cell results. Use feature *AvgQuats*, for example from [Compute Feature Average Orientations](ComputeAvgOrientationsFilter.md), for grain results. Convert Euler angles with [Convert Orientation Representation](ConvertOrientationsFilter.md) before using this filter.

A zero quaternion or a quaternion with a NaN or infinite component receives NaN. It does not define a valid orientation. Finite, nonzero quaternions need not be unit length because the computed c-axis is normalized.

**Reference Direction** has three dimensionless components in the sample frame. It need not be a unit vector. For example, *[3, 0, 3]* and *[1, 0, 1]* give the same result. All components must be finite. The vector length must be greater than 1e-12.

Only **Hexagonal_High** (6/mmm) and **Hexagonal_Low** (6/m) phases are accepted. Cubic, trigonal, unknown, and other non-hexagonal phases receive NaN. A warning gives the number of unmasked tuples that have a known non-hexagonal structure. **UnknownCrystalStructure** (999) tuples are not counted. They do not cause a warning. An unmasked phase index outside the Crystal Structures array is an error. Correct the Phases array or select the matching Crystal Structures array.

When **Use Mask Array** is enabled, tuples with a false or zero mask value receive NaN. Tuples with a true or nonzero mask value are included. The mask must be a one-component bool or uint8 array with the same tuple count as **Input Quaternions** and Phases.

The output is a one-component float32 array beside **Input Quaternions**. It keeps the input tuple shape. Valid angles are in **[0, 90] degrees**. An angle near 0 degrees means parallel. An angle near 90 degrees means perpendicular. The c-axis is antipodally symmetric: opposite c-axis directions represent the same axis. This folds angles greater than 90 degrees into the stated range.

### Related Filters

[Compute Feature Neighbor C-Axis Misalignments](ComputeFeatureNeighborCAxisMisalignmentsFilter.md) compares neighboring grains. [Compute Feature Reference C-Axis Misalignments](ComputeFeatureReferenceCAxisMisorientationsFilter.md) compares cell c-axes with their feature reference. This filter compares each selected quaternion with one user-supplied sample direction.

## Algorithm

Let g be the passive orientation matrix from the input quaternion. The sample-frame c-axis and normalized reference are

$$
\mathbf{c} = \frac{g^{T}(0,0,1)^{T}}{\left\|g^{T}(0,0,1)^{T}\right\|},
\qquad
\hat{\mathbf{r}} = \frac{\mathbf{r}}{\|\mathbf{r}\|}.
$$

The output angle is

$$
\theta = \frac{180}{\pi}\cos^{-1}\!\left(\operatorname{clamp}\left(|\mathbf{c}\cdot\hat{\mathbf{r}}|,0,1\right)\right).
$$

The reference is normalized once. One traversal processes matching chunks of quaternions, phases, and mask values. Completed output chunks are written in bulk. The same implementation supports in-memory and out-of-core arrays. Temporary storage is bounded by a chunk plus the small Crystal Structures array.

% Auto generated parameter table will be inserted here

## Example Pipelines

No example pipeline uses this filter yet.

## References

The following papers discuss HCP orientation effects and hard and soft grain combinations in titanium alloys:

1. Dunne, F. P. E., Rugg, D., and Walker, A. (2007). Lengthscale-dependent, elastically anisotropic, physically-based HCP crystal plasticity: application to cold-dwell fatigue in Ti alloys. [DOI: 10.1016/j.ijplas.2006.10.013](https://doi.org/10.1016/j.ijplas.2006.10.013).
2. Dunne, F. P. E., and Rugg, D. (2008). On the mechanisms of fatigue facet nucleation in titanium alloys. [DOI: 10.1111/j.1460-2695.2008.01284.x](https://doi.org/10.1111/j.1460-2695.2008.01284.x).
3. Dunne, F. P. E., Walker, A., and Rugg, D. (2007). A systematic study of hcp crystal orientation and morphology effects in polycrystal deformation and fatigue. [DOI: 10.1098/rspa.2007.1833](https://doi.org/10.1098/rspa.2007.1833).

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
