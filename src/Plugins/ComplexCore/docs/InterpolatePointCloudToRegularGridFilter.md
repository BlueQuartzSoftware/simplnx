Interpolate Point Cloud to Regular Grid
=============

## Group (Subgroup) ##

Sampling (Interpolation)

## Description ##

This **Filter** interpolates the values of arrays stored in a **Vertex Geometry** onto a user-selected **Image Geometry**.  The user defines the (x,y,z) radii of a kernel in *real space units*.  This kernel can be intialized to either *uniform* or *Gaussian*.  The interpolation algorithm proceeds as follows:

1. The kernel radii defined by the user in real space units are converted into voxel units, based on the spacing of the supplied **Image Geometry**.
2. The kernel is centered on each vertex in the **Vertex Geometry**.
3. The values of each **Attribute Array** on the centered vertex are associated to each voxel intersected by the kernel.  This stored value is multiplied by the value of the kernel.

The result of the above approach is a list of data at each voxel in the **Image Geometry** for each interpolated **Attribute Array**.  These lists may be of different lengths within each voxel, since the kernels from each point may overlap. This duplication may result in significant memory usage if the number of points is large; the user may select a subset of arrays to interpolate to alleviate this issue.  Note that all arrays selected for interpolation must be scalar.

A mask may be supplied to the filter.  Points that are not within the mask are ignored during interpolation.  Additionally, the distances between each voxel and the source point for the intersecting kernel may be stored; this significantly increases the required memory.  Arrays may be passed through to the image geometry without applying any interpolation.  This operation is equivalent to used a uniform kernel.

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| Use Mask | bool | Whether to use a mask when interpolating the vertex arrays |
| Store Kernel Distances | bool | Whether to store the kernel distances for each vertex |
| Interpolation Technique | Enumeration | The type of kernel to use, either *Uniform* or *Gaussian* |
| Kernel Size | float 3x | The size of the interpolation kernel, in real space units |

## Required Geometry ###

Vertex and Image

## Required Objects ##

| Kind                      | Default Name | Type     | Comp. Dims | Description                                 |
|---------------------------|--------------|----------|------------|---------------------------------------------|
| Data Container | None | N/A | N/A | Data Container holding the **Vertex Geometry** to interpolate |
| Data Container | None | N/A | N/A | Data Container holding the **Image Geometry** that serves as the target grid for interpolation |
| Vertex Attribute Array | VoxelIndices | size_t | (1) | The indices that indicate in which voxel in the supplied **Image Geometry** each vertex lies |
| Various **Attribute Array | None | Varies | (1) | The **Vertex Attribute Arrays** to interpolate |
| Various **Attribute Array | None | Varies | (1) | The **Vertex Attribute Arrays** to copy |

## Created Objects ##

| Kind                      | Default Name | Type     | Comp. Dims | Description                                 |
|---------------------------|--------------|----------|------------|---------------------------------------------|
|   Attribute Matrix   | InterpolatedAttributeMatrix | Cell | N/A | Attribute Matrix** that stores the interpolated **Attribute Arrays |

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.


