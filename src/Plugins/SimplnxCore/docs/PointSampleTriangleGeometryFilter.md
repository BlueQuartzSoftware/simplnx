# Point Sample Triangle Geometry

## Group (Subgroup)

DREAM3D Review (Geometry)

## Description

This **Filter** randomly samples point locations on **Triangles** in a **Triangle Geometry**.  The sampled point locations are then used to construct a **Vertex Geometry**.  The number of point samples may either be specified manually or by inferring from another **Geometry**:

| Geometry | Source for Number of Samples |
|----------|-----------|
| Image | Number of cells |
| Rectilinear Grid | Number of cells |
| Vertex | Number of vertices |
| Edge | Number of vertices |
| Triangle | Number of vertices |
| Quadrilateral | Number of vertices |
| Tetrahedral | Number of vertices |

In order to ensure an even sampling of the total surface are of the **Triangle Geometry**, the average number of points sampled per triangle is made proportional to the area of the triangle.  Within a given **Triangle**, a point is chosen using the following approach:

![Equation 1](Images/PointSampleTriangleGeometry_Eqn1.png)

where ![](Images/PSTG_2.png) are the coordinates of the sampled point; ![](Images/PSTG_3.png), ![](Images/PSTG_4.png), and ![](Images/PSTG_5.png) are the coordinates of the vertices beloning to the **Triangle**; and ![](Images/PSTG_6.png) and ![](Images/PSTG_7.png) are random real numbers on the interval ![](Images/PSTG_8.png).  This approach has the benefit of uniform sampling within the **Triangle** area, and functions correctly regardless of the dimensionality of the space embedding (i.e., whether the **Triangle** is in the plane or embedded in 3D).

The user may opt to use a mask to prevent certain **Triangles** from being sampled; where the mask is _false_, the **Triangle** will not be sampled.  Additionally, the user may choose any number of **Face Attribute Arrays** to transfer to the created **Vertex Geometry**. The vertices in the new **Vertex Geometry** will gain the values of the **Faces** from which they were sampled.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
