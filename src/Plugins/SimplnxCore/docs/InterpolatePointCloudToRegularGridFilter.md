# Interpolate Point Cloud to Regular Grid

## Group (Subgroup)

Sampling (Interpolation)

## Description

This **Filter** interpolates the values of arrays stored in a **Vertex Geometry** (point cloud) onto a user-selected **Image Geometry** (regular grid). For each point in the cloud, the filter applies a kernel centered on the point's voxel and accumulates weighted contributions into every voxel the kernel overlaps. The result is a single interpolated value per voxel per array, computed as a weighted average of all contributions.

### Interpolation Technique

The *Interpolation Technique* parameter provides the following choices:

- **Uniform [0]**: Every voxel within the kernel radius receives equal weight (1.0).
- **Gaussian [1]**: Voxel weights fall off with distance from the kernel center according to a Gaussian function, controlled by user-specified sigma values in each dimension.

### Kernel

The user defines the (x, y, z) radii of a kernel in *real space units*. The kernel radii are converted to voxel units based on the spacing of the **Image Geometry**. If the kernel radius in a given dimension is smaller than the voxel spacing, the kernel has a zero extent in that dimension (i.e., each point only affects its own voxel along that axis).

### Algorithm

The interpolation proceeds in two passes:

**Pass 1 – Accumulation (one iteration over all vertices):**

1. For each vertex in the **Vertex Geometry**, compute the destination voxel index from the vertex coordinates and the **Image Geometry**'s origin, spacing, and dimensions. Vertices that fall outside the grid are silently skipped.
2. If a mask is enabled and the vertex is masked out, skip it.
3. Center the kernel on that voxel and determine the kernel bounds, clipped to the grid extents.
4. For each voxel within the clipped kernel:
   - Look up the kernel weight using the 3D offset from the center voxel.
   - For each *interpolated* array: multiply the source value by the kernel weight and accumulate the weighted value, the weight itself and any enabled statistics into the destination voxel.
   - For each *copied* array: accumulate the source value with uniform weight (1.0) regardless of the selected kernel type.

**Pass 2 – Finalization (one iteration over all voxels):**

For each voxel that received at least one contribution:

- The interpolated output value is `sum(weight * value) / sum(weight)`.
- Any enabled statistics are written from the accumulated state.

### Arrays to Interpolate vs. Arrays to Copy

Both options transfer data from the **Vertex Geometry** onto the **Image Geometry**, but they differ in how the kernel is applied:

- **Arrays to Interpolate** use the selected kernel (Uniform or Gaussian). The kernel weight multiplies each vertex's value before being accumulated into surrounding voxels. The final output is a kernel-weighted average. Optional statistics (length, min, max, mean, standard deviation, summation) can be computed on the weighted contributions. The output array type is always *float64* because the weighted average produces floating-point values regardless of the input type.

- **Arrays to Copy** always use a uniform kernel (weight = 1.0), even when Gaussian interpolation is selected. The final output is a simple arithmetic average of all values that fell within the kernel radius. No statistics are computed for copied arrays. The output array type matches the source array type.

Use *Arrays to Interpolate* for data values that should be smoothed or blended by the kernel (e.g., measured scalar fields). Use *Arrays to Copy* for categorical or index data where Gaussian weighting would not be meaningful.

All arrays selected for interpolation or copying must be **scalar** (single-component) arrays.

### Inline Statistics

Rather than storing all per-voxel contributions and computing statistics in a separate filter, this filter can compute the following statistics *inline* during the accumulation pass. Each statistic is optional and controlled by a boolean parameter. When enabled, an additional output array is created for each interpolated array with the array name plus a configurable suffix (e.g., `FaceAreas_Length`).

| Statistic              | Output Type | Description                                                                                                  |
|------------------------|-------------|--------------------------------------------------------------------------------------------------------------|
| **Length**             | uint64      | Number of weighted contributions to the voxel                                                                |
| **Minimum**            | float32     | Smallest weighted value (`weight * source_value`)                                                            |
| **Maximum**            | float32     | Largest weighted value                                                                                       |
| **Mean**               | float32     | Arithmetic mean of weighted values (`sum / count`)                                                           |
| **Standard Deviation** | float32     | Population standard deviation of weighted values, computed via Welford's numerically stable online algorithm |
| **Summation**          | float32     | Sum of all weighted values                                                                                   |

Statistics are computed on the *weighted* values (`kernel_weight * source_value`), which are the same quantities that contribute to the interpolated average. Voxels with no contributions receive a value of zero for all statistics.

Median is not supported because it requires storing every vertex's contribution, which would negate the memory savings of the flat accumulation approach.

### Required Input Sources

- **Vertex Geometry** -- the point cloud whose arrays will be interpolated, produced by a filter that creates a **Vertex Geometry** such as [Extract Vertex Geometry](ExtractVertexGeometryFilter.md) or [Create Geometry](CreateGeometryFilter.md).

### Mask

An optional boolean or uint8 mask array may be provided. Vertices where the mask value is *false* or *0* are skipped entirely during interpolation – they do not contribute to any voxel's accumulated value or statistics.

### Memory Usage

This filter uses flat arrays rather than variable-length lists, so memory usage is proportional to the number of voxels in the **Image Geometry** (not the number of points times the kernel size). For each interpolated array, the filter temporarily allocates ~56 bytes per voxel for accumulation state. For each copied array, ~16 bytes per voxel is allocated. These temporary variables are freed after the finalization pass.

### Voxel Index Computation

The destination voxel for each vertex is computed on-the-fly from the vertex coordinates and the **Image Geometry**'s origin, spacing, and dimensions. There is no need to pre-compute a voxel indices array (e.g., via the [Map Point Cloud to Regular Grid](MapPointCloudToRegularGridFilter.md) filter). Vertices whose coordinates fall outside the **Image Geometry** bounds are silently skipped.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
