# Append Image Geometry

## Group (Subgroup)

Sampling (Memory/Management)

## Description

This filter allows the user to append one or multiple image geometries to a given image geometry, in any direction (X,Y,Z). The input and
destination **ImageGeometry** objects must have the same dimensions in the directions that are NOT chosen.  If the X direction is chosen, the geometries must match in Y & Z.  If the Y direction is chosen, the geometries must match in X & Z.  If the Z direction is chosen, the geometries must match in X & Y.  Optional checks for equal **Resolution** values can also be performed.

This filter also has an option to mirror the resulting geometry in the chosen direction.  If the X direction is chosen, it will mirror the positions of the YZ planes.  If the Y direction is chosen, it will mirror the positions of the XZ planes.  If the Z direction is chosen, it will mirror the positions of the XY planes.

### Direction

The *Direction* parameter selects the axis along which the input geometries are appended onto the destination geometry:

- **X**: Appends geometries along the X axis. The input and destination geometries must have matching Y and Z dimensions.
- **Y**: Appends geometries along the Y axis. The input and destination geometries must have matching X and Z dimensions.
- **Z**: Appends geometries along the Z axis. The input and destination geometries must have matching X and Y dimensions.

### X Direction Examples

#### Example 1 (X)
If the user has an already existing **Image Geometry** that is 100 voxels in the *Y* direction by 300 pixels in the
*Z* direction and composed of 10 *X* slices, then if the user appends another three geometries in the X direction that are the same dimensions in Y & Z but contain 20 *X* slices each, the resulting **Image Geometry** will have a total of 70 *X* slices.

The filter inputs for this example are as follows:

+ **Input Image Geometries**: Geometry A (20x100x300), Geometry B (20x100x300), Geometry C (20x100x300)
+ **Destination Image Geometry**: Image Geometry (10x100x300)
+ **Direction**: X
+ **Mirror Geometry In Direction**: OFF
+ **Check Spacing**: OFF
+ **Default Value**: 0
+ **Save As New Geometry**: OFF

#### Example 2 (Visual Example) (X)
Here's the SmallIN100 dataset example sliced into three pieces in the X direction:

![](Images/AppendImageGeometry/x_direction_pieces.png)

And here is what the geometry looks like after appending the three pieces together in the X direction.  On the left is the regular result, on the right is the mirrored result:
| ![](Images/AppendImageGeometry/x_complete.png) | ![](Images/AppendImageGeometry/x_mirrored.png) |
|:----------------------:|:----------------------:|

### Y Direction Examples

#### Example 1 (Y)
If the user has an already existing **Image Geometry** that is 400 voxels in the *X* direction by 200 pixels in the
*Z* direction and composed of 50 *Y* slices, then if the user appends another two data sets in the Y direction that are the same dimensions in X & Z but contain 40 *Y* slices each, the resulting **Image Geometry** will have a total of 130 *Y* slices.

The filter inputs for this example are as follows:

+ **Input Image Geometries**: Geometry A (400x40x200), Geometry B (400x40x200)
+ **Destination Image Geometry**: Image Geometry (400x50x200)
+ **Direction**: Y
+ **Mirror Geometry In Direction**: OFF
+ **Check Spacing**: OFF
+ **Default Value**: 0
+ **Save As New Geometry**: OFF

#### Example 2 (Visual Example) (Y)
Here's the SmallIN100 dataset example sliced into three pieces in the Y direction:

![](Images/AppendImageGeometry/y_direction_pieces.png)

And here is what the geometry looks like after appending the three pieces together in the Y direction.  On the left is the regular result, on the right is the mirrored result:
| ![](Images/AppendImageGeometry/y_complete.png) | ![](Images/AppendImageGeometry/y_mirrored.png) |
|:----------------------:|:----------------------:|

### Z Direction Examples

#### Example 1 (Z)
If the user has an already existing **Image Geometry** that is 100 voxels in the *X* direction by 200 pixels in the
*Y* direction and composed of 5 *Z* slices, then if the user appends one other data set in the Z direction that is the same dimensions in X & Y but contains 10 *Z* slices, the resulting **Image Geometry** will have a total of 15 *Z* slices.

The filter inputs for this example are as follows:

+ **Input Image Geometries**: Geometry A (100x200x10)
+ **Destination Image Geometry**: Image Geometry (100x200x5)
+ **Direction**: Z
+ **Mirror Geometry In Direction**: OFF
+ **Check Spacing**: OFF
+ **Default Value**: 0
+ **Save As New Geometry**: OFF

#### Example 2 (Visual Example) (Z)
Here's the SmallIN100 dataset example sliced into three pieces in the Z direction:

![](Images/AppendImageGeometry/z_direction_pieces.png)

And here is what the geometry looks like after appending the three pieces together in the Z direction.  On the left is the regular result, on the right is the mirrored result:
| ![](Images/AppendImageGeometry/z_complete.png) | ![](Images/AppendImageGeometry/z_mirrored.png) |
|:----------------------:|:----------------------:|

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
