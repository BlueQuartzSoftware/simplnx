# IterativeClosestPoint #

## Group (Subgroup) ##

Reconstruction (Alignment)

## Description ##

This **Filter** estimates the rigid body transformation (i.e., rotation and translation) between two sets of points represted by **Vertex Geometries** using the *iterative closest point* (ICP) algorithm.  The two **Vertex Geometries** are not required to have the same number of points.  THe **Filter** first initializes temporary storage for each set of points and a global transformation.  Then, the alignment algorithm iterates through the following steps:

1. The closest point in the target geometry is determined for each point in the moving geoemetry; these are called the correspondence points.  "Closeness" is measured based on Euclidean distance.
2. The rigid body transformation between the moving and target correspondences is solved for analytically using least squares.
3. The above transformation is applied to the moving points.
4. The global transformation is updated with the transformation computed for the current iteration.

Iterations proceed for a fixed number of user-defined steps.  The final rigid body transformation is stored as a 4x4 transformation matrix in row-major order.  The user has the option to apply this transformation to the moving **Vertex Geometry**.  Note that this transformation is applied the the moving geometry *in place* if the option is selected.

ICP has a number of advantages, such as robustness to noise and no requirement that the two sets of points to be the same size.  However, peformance may suffer if the two sets of points are of siginficantly different size.

## Parameters ##

| Name | Type | Description |
|------|------|------|
| Number of Iterations | int | Number if iterations for the ICP algorithm |
| Apply Transform to Moving Geometry | bool | Whether to apply the computed transform to the moving **Vertex Geometry** |

## Required Geometry ##

Vertex

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|-------------|---------|-----|
| **Data Container** | None | N/A | N/A | **Data Container** holding the moving **Vertex Geometry** |
| **Data Container** | None | N/A | N/A | **Data Container** holding the target **Vertex Geometry** |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|-------------|---------|-----|
| **Attribute Matrix** | TransformAttributeMatrix | Generic | N/A | **Attribute Matrix** that stores the computed transformation |
| **Attribute Array** | Transform | float | (4, 4) | Computed transformation matrix |


## Example Pipelines ##

List the names of the example pipelines where this filter is used.

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users