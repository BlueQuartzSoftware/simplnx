# Apply Transformation to Geometry

## Group (Subgroup)

Rotation, Scale & Transformation

## Description

This **Filter** applies a spatial transformation to either a node **Geometry** or an **Image Geometry**.

### Node Geometries

 A node **Geometry** is any geometry that requires explicit definition of **Vertex** positions. Specifically, **Vertex**, **Edge**, **Triangle**, **Quadrilateral**, and **Tetrahedral** **Geometries** may be transformed by this **Filter**. The transformation is applied in place, so the input **Geometry** will be modified.

- **NO** interpolation will take place as the only changes that take place are the actual coordinates of the vertices.

### Image Geometry

If the user selects an **Image Geometry** then there are 2 additional required filter parameters that need to be set:

- **Interpolation Method**: This will be used when transferring the data from the old geometry to the newly transformed geometry.
- **Cell Attribute Matrix**: This Attribute Matrix holds the data that is associated with each cell of the image geometry.

The linear/Bi-Linear/Tri-Linear Interpolation is adapted from the equations presented
in [https://www.cs.purdue.edu/homes/cs530/slides/04.DataStructure.pdf, page 36}](https://www.cs.purdue.edu/homes/cs530/slides/04.DataStructure.pdf)

## Example Image Geometry Transformations

| Description | Example Output Image |
|-------------|----------------------|
| Input Image |  ![Input Image](Images/ApplyTransformation_AsRead.png) |
| After Rotation of 45 Degrees around the <001> axis | ![Rotation of 45 Degrees around the <0,0,1> axis](Images/ApplyTransformation_Rotated.png) |
| Scaled by 2x in the X and Y axis  | ![Scaled by 2x in the X and Y axis.](Images/ApplyTransformation_Scaled.png) |

## Image Geometry Caveat

Using this filter several times in a row to apply several transforms in succession to the same image geometry is highly likely to result in visual artifacts related to the intermediate re-gridding of the image geometry between transformations.  For example, let's rotate an image geometry 90 degrees along the Z axis:

| Description | Image |
|-------------|----------------------|
| Input Image |  ![Input Image](Images/ApplyTransformation_ImageGeom.png) |
| After Rotation of 90 Degrees around the <001> axis | ![Rotation of 90 Degrees around the <0,0,1> axis](Images/ApplyTransformation_ImageGeom_Final.png) |

Instead of using a single rotation of 90 degrees, if the user has a need to instead apply several rotations that still add up to 90 degrees, for example a pair of 45 degree rotations, potential unwanted artifacts can occur due to the intermediate regridding for each rotation.

| Description | Image |
|-------------|----------------------|
| Input Image |  ![Input Image](Images/ApplyTransformation_ImageGeom.png) |
| After 1st Rotation of 45 Degrees around the <001> axis | ![1st Rotation of 45 Degrees around the <0,0,1> axis](Images/ApplyTransformation_ImageGeom_Intermediate.png) |
| After 2nd Rotation of 45 Degrees around the <001> axis | ![2nd Rotation of 45 Degrees around the <0,0,1> axis](Images/ApplyTransformation_ImageGeom_Final_Artifacts.png) |

Why does this happen?  Let's overlay the centers of each cell on top of the original image geometry.

| Description | Image |
|-------------|----------------------|
| Input Image |  ![Input Image](Images/ApplyTransformation_ImageGeom_WithVertices.png) |
| After 1st Rotation of 45 Degrees around the <001> axis | ![1st Rotation of 45 Degrees around the <0,0,1> axis](Images/ApplyTransformation_ImageGeom_Intermediate_WithVertices.png) |

The green vertices refer to the center of each image geometry cell.  As you can see, after the first 45 degree rotation, the image geometry is re-gridded and now the transformed green vertices are no longer in the center of each cell.

On the 2nd and final 45 degree rotation, the image geometry is going to double in size because the algorithm doesn't know the final image geometry's exact size and doubles its size to account for the worst case scenario.

Let's see how the transformed green vertices overlay on the intermediate image geometry when the field of vertices has doubled in size but the image geometry hasn't actually been transformed a 2nd time yet.

| Description | Image |
|-------------|----------------------|
| After 1st Rotation of 45 Degrees around the <001> axis | ![1st Rotation of 45 Degrees around the <0,0,1> axis](Images/ApplyTransformation_ImageGeom_Intermediate_WithVertices2.png) |

The cell circled in purple has two green vertices inside it.  This means that once the 2nd 45 degree transformation is completed, the final image geometry will have that orange color shifted outside where we would expect it to be.  And sure enough:

| Description | Image |
|-------------|----------------------|
| After 2nd Rotation of 45 Degrees around the <001> axis | ![2nd Rotation of 45 Degrees around the <0,0,1> axis](Images/ApplyTransformation_ImageGeom_Final_Artifacts2.png) |

To avoid this problem, it is considered best practice to use the *[Combine Transformation Matrices](CombineTransformationMatricesFilter.md)* filter to combine all transforms together into one transform before applying the transform to an image geometry.

| Description | Image |
|-------------|----------------------|
| After combining both 45 degree rotations and applying around the <001> axis | ![Rotation of 90 Degrees around the <0,0,1> axis](Images/ApplyTransformation_ImageGeom_Final.png) |

## NOTE: 

This caveat is ONLY for image geometries.  Multiple transformations can be applied in succession to any of the "Node" based geometries without any issues. Those are:

- Vertex
- Edge
- Triangle
- Quad
- Tetrahedral
- Hexahedral

## Transformation Information

### Transformation Type

The *Transformation Type* parameter provides the following choices:

- **No Transform [0]**: Applies an identity transformation; the geometry is unchanged.
- **Pre-Computed Transformation Matrix (4x4) [1]**: Uses a 4x4 transformation matrix supplied as an Attribute Array in row major order.
- **Manual Transformation Matrix [2]**: Uses a manually entered 4x4 transformation matrix.
- **Rotation [3]**: Rotates about a supplied axis-angle <x,y,z> with the angle specified in degrees.
- **Translation [4]**: Translates the geometry by the supplied (x, y, z) values.
- **Scale [5]**: Scales the geometry by the supplied (x, y, z) values.

The user may select from a variety of options for the type of transformation to apply:

| Enum Value | Transformation Type                | Representation                                                                       |
|------------|------------------------------------|--------------------------------------------------------------------------------------|
| 0          | No Transformation                  | Identity transformation                                                              |
| 1          | Pre-Computed Transformation Matrix | A 4x4 transformation matrix, supplied by an **Attribute Array** in *row major* order |
| 2          | Manual Transformation Matrix       | Manually entered 4x4 transformation matrix                                           |
| 3          | Rotation                           | Rotation about the supplied axis-angle <x,y,z> (Angle in Degrees).                   |
| 4          | Translation                        | Translation by the supplied (x, y, z) values                                         |
| 5          | Scale                              | Scaling by the supplied (x, y, z) values                                             |

The **Translate Geometry To Global Origin Before Transformation** option must be selected if the user wants to translate their volume to (0, 0, 0), apply the transform, and then translate the volume back to its original location.

### Resampling or Interpolation (Image Geometry Only)

When transforming an **Image Geometry**, the *Resampling or Interpolation* parameter controls how cell data values are assigned in the newly created grid:

- **Nearest Neighbor Resampling [0]**: Each output cell takes the value of the nearest input cell. This is fast and preserves sharp boundaries, but may produce a blocky appearance.
- **Linear Interpolation [1]**: Each output cell value is computed using trilinear interpolation from the surrounding input cells. This produces smoother results but may blur sharp features.
- **No Interpolation [2]**: The transformation is applied without any resampling of cell data. Use this option when the transformation does not change the grid topology (e.g., integer translations that align exactly with the existing grid).

## Saving the final transformation Matrix.

There is an option to save the final transformation matrix into its own array. The format of the output DataArray is a
flattened array 16 elements in size that represents a 4x4 matrix. The elements are encoded in a ROW MAJOR array, i.e., 

    1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16

represents the following 4x4 Matrix

    1   2   3   4
    5   6   7   8
    9   10  11  12
    13  14  15  16

## Algorithm

### The transform

Regardless of how the user specifies the transformation (rotation, scale, translation, or a raw matrix), the filter first converts it into a single homogeneous **4x4 matrix `M`** that maps points from the original coordinate space to the transformed one. Points are stored as `(x, y, z, 1)` column vectors; `M * p` gives the transformed point. Combining multiple transforms (e.g. translate-to-origin → rotate → translate-back) is a matrix multiplication.

From that point on, applying the transform is a two-step procedure: (1) figure out where each output element lives in space, (2) populate its data. How step 2 works depends on whether the target is a **node geometry** or an **image geometry**.

### Node geometries (Vertex, Edge, Triangle, Quad, Tetra, Hex)

Node geometries store **explicit vertex coordinates**. Applying the transform is straightforward: multiply every vertex by `M`. Cell-level arrays (triangle labels, vertex attributes, etc.) are unchanged — the topology and per-element data still apply to the same cell, just at a new position in space.

The filter processes vertices in **16K-vertex chunks** using bulk I/O:

1. `copyIntoBuffer()` reads a chunk of 16,384 vertices (48 KB = 3 floats × 16K × 4 B) from the vertex list into a RAM buffer.
2. The loop applies `M * p` to each vertex in-buffer.
3. `copyFromBuffer()` writes the transformed chunk back.

This replaces a per-element `at()` / `setValue()` loop that would thrash HDF5 chunk caches on out-of-core vertex lists. Memory is bounded at 48 KB per worker.

### Image geometries — the re-gridding problem

An **Image Geometry** stores data on a rigid regular grid; there are no explicit vertex coordinates to transform. Instead, the filter must build a **new** grid aligned with the transformed space, then decide — for every output voxel — what value the corresponding input location had. This is called **re-gridding** or **resampling**.

For each output voxel:
1. Compute the physical coordinate `p_out` of the voxel's center.
2. Apply the **inverse** transform: `p_in = M^(-1) * p_out`. This is the physical coordinate in the original image.
3. Look up (or interpolate) the input value at `p_in`.

The user picks one of two strategies for step 3:

- **Nearest Neighbor** — snap `p_in` to the nearest input voxel and copy its value. Fast and preserves sharp labels (e.g. FeatureIds), but has blocky artifacts.
- **Trilinear Interpolation** — find the 8 input voxels surrounding `p_in` and compute a weighted average based on the fractional position inside that 8-corner cube. Smoother but blurs sharp boundaries.

### Z-slice slab cache (both Image paths)

A naive implementation would read individual source voxels on demand as it walks the output volume. For a tilted rotation, each output slice may pull from dozens of different source Z-slices, blasting the HDF5 chunk cache. The filter avoids this with a **Z-slice slab cache**:

For each output Z-slice `k`:
1. **Analytically determine the source Z range** by computing the inverse transform of the four XY corners of the output slice. Take the min and max source Z coordinates those corners land in. For trilinear, pad by ±2 slices so all 8 corner neighbors for every interior voxel are guaranteed to be inside the cached range.
2. **Ensure the slab cache covers that range.** If the current cache is missing some of the needed slices, update it (see the next section).
3. **Process the full output slice** reading the source only from the RAM slab — no OOC access per voxel.
4. Write the computed output slice back with a single `copyFromBuffer()`.

The slab cache is a single contiguous buffer holding K consecutive source Z-slices at their logical positions. Reads from the slab are plain `buffer[z_offset * sliceSize + y * dimX + x]` indexing, no virtual dispatch.

### Sliding-window slab updates

When consecutive output slices need source Z ranges that overlap heavily (typical for small or moderate rotations), re-reading the entire slab for each output slice would waste most of the I/O. Instead, the helper `updateSlabCache<T>()` does **incremental updates**:

1. Compute the intersection of the current cached `[cachedZMin, cachedZMax]` and the newly needed `[newZMin, newZMax]` ranges.
2. If they overlap:
   - **Shift** the surviving slices inside the buffer via `std::memmove` to their new positions.
   - **Read** only the slices below the overlap (if the new range extends further back) and above the overlap (if the new range extends further forward). Typically this is 1–2 new slices per output slice for a mild rotation — orders of magnitude less I/O than re-reading the full slab.
3. If there's no overlap (first iteration, or large jump), fall back to a full slab re-read.

### Intra-slice parallelism

Inside the output slice loop, the inner `for y in [0, outDimY): for x in [0, outDimX):` work is farmed out to threads via `ParallelDataAlgorithm`:

- Each thread processes a contiguous range of Y rows.
- All threads **share** the slab buffer (read-only for the duration of the compute phase — no thread writes to it).
- Each thread writes to its own, disjoint Y-row range of a local output slice buffer.
- `ImageGeom::computeCellIndex()` and `getCoordsf()` are const and thread-safe; `FindOctant()` (used by trilinear) is a pure function.
- Trilinear's per-voxel `pValues` scratch (8 vertices × numComps) is declared inside the lambda body so each thread gets its own.

No `DataStore` is touched inside the parallel region — I/O is strictly serialized between phases. This sidesteps the well-documented thread-safety limitations of `AbstractDataStore`.

### Putting it all together

For the CT_align rotation case (1472 × 1139 × 1174 uint16 = 1.97 B voxels, tilted rotation), the combination of slab caching, sliding-window updates, and intra-slice parallelism turns an otherwise infeasible operation (>5 min, OOM risk) into a ~20 s operation with peak working memory bounded by the slab size and the output slice buffer (both a few tens of MB).

% Auto generated parameter table will be inserted here

## Example Pipelines

- Pipelines/SimplnxCore/Examples/apply_transformation_basic.d3dpipeline
- Pipelines/SimplnxCore/Examples/apply_transformation_image.d3dpipeline
- Pipelines/SimplnxCore/Examples/apply_transformation_node.d3dpipeline

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
