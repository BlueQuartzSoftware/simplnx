# Point Sample Triangle Geometry

## Group (Subgroup)

Sampling (Geometry)

## Description

This **Filter** randomly samples point locations on the triangles of a **Triangle Geometry** (a node-based surface mesh of triangular faces) and uses those sampled locations to construct a new **Vertex Geometry** (a point cloud). The total number of sample points is supplied directly through the *Number of Sample Points* parameter.

To ensure an even sampling across the whole surface area of the **Triangle Geometry**, the average number of points placed on each triangle is made proportional to that triangle's area. Larger triangles therefore receive proportionally more samples than smaller ones, which produces a spatially uniform point density rather than an even count per triangle.

Within a given triangle, each point is chosen with the following formula:

![Equation: the sampled point P equals (1 minus the square root of r1) times A, plus the square root of r1 times (1 minus r2) times B, plus the square root of r1 times r2 times C.](Images/PointSampleTriangleGeometry_Eqn1.png)

where ![the sampled point coordinates P](Images/PSTG_2.png) are the coordinates of the sampled point; ![vertex A](Images/PSTG_3.png), ![vertex B](Images/PSTG_4.png), and ![vertex C](Images/PSTG_5.png) are the coordinates of the vertices belonging to the triangle; and ![random number r1](Images/PSTG_6.png) and ![random number r2](Images/PSTG_7.png) are random real numbers on the interval ![the interval from zero to one](Images/PSTG_8.png). This approach gives uniform sampling within the triangle area and works correctly regardless of the dimensionality of the embedding space (whether the triangle lies in a plane or is embedded in 3D).

### Random Sampling and Reproducibility

The point placement is **random**: both the distribution of samples across triangles and the location of each point within its triangle are drawn from a pseudo-random number generator. By default a new seed is used each run, so the output point cloud differs from run to run. To obtain repeatable results, enable *Use Seed for Random Generation* and supply a fixed *Seed Value*; the seed actually used is also stored in an output array so a run can be reproduced later.

### Masking and Transferred Data

The user may opt to use a **mask** (a boolean per-face flag) to prevent certain triangles from being sampled; where the mask is *false*, the triangle is not sampled. Additionally, the user may choose any number of **Face Attribute Arrays** to transfer to the created **Vertex Geometry**. Each vertex in the new geometry inherits the values of the face from which it was sampled.

### Required Input Sources

- **Triangle Geometry to Sample** -- a **Triangle Geometry** (surface mesh), typically produced by [Create Surface Mesh (QuickMesh)](QuickSurfaceMeshFilter.md).
- **Face Areas** -- a single-component per-face area array, produced by [Compute Triangle Areas](ComputeTriangleAreasFilter.md).
- **Mask** (optional) -- a boolean per-face array, typically produced by a thresholding filter such as [Multi-Threshold Objects](MultiThresholdObjectsFilter.md).

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
