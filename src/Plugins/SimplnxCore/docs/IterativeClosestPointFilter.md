# IterativeClosestPoint

## Group (Subgroup)

Reconstruction (Alignment)

## Description

This **Filter** estimates the rigid body transformation (i.e., rotation and translation) between two sets of points represted by **Vertex Geometries** using the *iterative closest point* (ICP) algorithm.  The two **Vertex Geometries** are not required to have the same number of points.  The **Filter** first initializes temporary storage for each set of points and a global transformation.  Then, the alignment algorithm iterates through the following steps:

1. The closest point in the target geometry is determined for each point in the moving geoemetry; these are called the correspondence points.  "Closeness" is measured based on Euclidean distance.
2. The rigid body transformation between the moving and target correspondences is solved for analytically using least squares.
3. The above transformation is applied to the moving points.
4. The global transformation is updated with the transformation computed for the current iteration.

Iterations proceed for a fixed number of user-defined steps.  The final rigid body transformation is stored as a 4x4 transformation matrix in row-major order.  The user has the option to apply this transformation to the moving **Vertex Geometry**.  Note that this transformation is applied to the moving geometry *in place* if the option is selected.

ICP has a number of advantages, such as robustness to noise and no requirement that the two sets of points to be the same size.  However, peformance may suffer if the two sets of points are of siginficantly different size.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
