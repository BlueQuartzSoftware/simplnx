# Iterative Closest Point

## Group (Subgroup)

Reconstruction (Alignment)

## Description

This **Filter** estimates the rigid body transformation (i.e., rotation and translation) between two sets of points represented
by **Vertex Geometries** using the *iterative closest point* (ICP) algorithm.  The two **Vertex Geometries** are not required
to have the same number of points.

The Iterative Closest Point (ICP) algorithm is a method used to minimize the difference between two clouds of points,
which we can describe in terms of "moving geometry" and "target geometry." The basic flow of the algorithm is:

1. Initial State: We start with two sets of points or geometries. The "moving geometry" is the one we aim to align with
the "target geometry." Initially, the moving geometry may be in any orientation or position relative to the target geometry.

2. Identify Correspondences: For each point in the moving geometry, we find the closest point in the target geometry. These
pairs of points are considered correspondences, based on the assumption that they represent the same point in space
after the moving geometry is properly aligned.

3. Estimate Transformation: With the correspondences identified, the algorithm calculates the optimal rigid body
transformation (which includes translation and rotation) that best aligns the moving geometry to the target
geometry. This step often involves minimizing a metric, such as the sum of squared distances between corresponding
points, to find the best fit.

4. Apply Transformation: The calculated transformation is applied to the moving geometry, aligning it closer to the
target geometry.

5. Iterate: Steps 2 through 4 are repeated iteratively. With each iteration, the moving geometry is brought into closer
alignment with the target geometry. The iterations continue until a stopping criterion is met, which could be a predefined
number of iterations, a minimum improvement threshold between iterations, or when the change in the error metric falls
below a certain threshold.

6. Final Alignment: Once the iterations stop, the algorithm concludes with the moving geometry optimally aligned to the target
geometry, based on the criteria set for minimizing the differences between them.

ICP has a number of advantages, such as robustness to noise and no requirement that the two sets of points to be the same
size.  However, performance may suffer if the two sets of points are of significantly different size.

## Saving the Output Transformation Matrix

The filter always reports the computed rigid body transformation as an output **DataArray**. By default the moving geometry itself is left unchanged. If the *Apply Transformation to Moving Geometry* parameter is enabled, the filter also applies the transformation to the moving geometry's vertices in place, so that it is aligned to the target geometry.

The output **DataArray** is a flattened array 16 elements in size that represents a 4x4 transformation matrix.
The elements are encoded in ROW MAJOR order, i.e.,

    1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16

represents the following 4x4 matrix

    1   2   3   4
    5   6   7   8
    9   10  11  12
    13  14  15  16

The upper-left 3x3 block holds the rotation, and the first three entries of the rightmost column (elements 4, 8, 12) hold the translation, expressed in the coordinate units of the input geometries.

The reported matrix can be supplied to [Apply Transformation to Geometry](ApplyTransformationToGeometryFilter.md) to transform this or another geometry as a separate step.

### Required Input Sources

- **Moving Vertex Geometry** -- the **Vertex Geometry** to be aligned to the target.
- **Target Vertex Geometry** -- the fixed reference **Vertex Geometry**. The two geometries need not have the same number of points.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
