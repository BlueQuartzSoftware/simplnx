# Scipy: Interpolate Grid Data

## Group (Subgroup)

Sampling (Interpolation)

## Description

**Interpolate Grid Data** is a Python-based *simplnx* filter designed to interpolate data points on a grid. It utilizes the `griddata` method from the `scipy` library to perform interpolation on a set of input points and their associated data values, projecting these values onto a specified set of target points. This filter offers flexible interpolation capabilities, including handling irregularly spaced data and supporting different interpolation methods.

The interpolation method can be chosen based on the data distribution and the desired smoothness of the interpolated surface:

- `Nearest`: This method will assign the value of the nearest input point to each target point. It is fast but can lead to a non-smooth, blocky surface.
- `Linear`: This method will triangulate the input points and use linear barycentric interpolation within the triangles. It creates a piecewise linear surface, suitable for smoothly varying data.
- `Cubic`: This method uses cubic splines to interpolate within each triangle of the triangulated input data. It creates a smooth surface but can be influenced significantly by outliers.

Additionally, the filter can optionally use a fill value for points outside of the convex hull of the input points and rescale points to a unit cube to improve interpolation accuracy.

### Parameters

- `Input Points Array Path`: Path in the data structure to the input points array.
- `Input Data Array Path`: Path in the data structure to the input data array.
- `Target Points Array Path`: Path in the data structure to the target points array where interpolation is performed.
- `Interpolation Method`: The method used to interpolate the input data. Options are 'Nearest', 'Linear', and 'Cubic'.
- `Use Fill Value`: Determines whether or not to use the Fill Value parameter.
- `Fill Value`: The value to use for points outside of the convex hull of the input points. This option has no effect for the "Nearest" interpolation method.
- `Rescale Points to Unit Cube`: Whether to rescale the input and target points to a unit cube, which can help in improving the accuracy of interpolation.
- `Interpolated Data Array Path`: Path in the data structure where the interpolated data will be stored.

## Example Pipelines

None

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GitHub site where the community of DREAM3D-NX users can help answer your questions.

