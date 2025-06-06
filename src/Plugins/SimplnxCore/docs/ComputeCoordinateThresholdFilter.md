# Compute Coordinate Threshold

## Group (Subgroup)

Geometry

## Description

This filter produces a mask that marks cells that fall inside or outside a given bounding shape within a supplied geometry. The filter outputs a mask to provide the greatest flexibilty, while leveraging exisitng algorithms. This filter doesn't modify the input geometry in any way, if you wish to modify the data within the bounds consider using one of the cleanup filters on the marked values. See _Remove Flagged Vertices/Edges/Triangles_ for an example of a potential followup filter. There are several caveats to be aware of with this filter, detailed thouroghly in the following sections.

### Input Geometry Types

This filter is meant to be as widely applicable as possible, so **cells will only be included in bounding box if all points fall within the bounds**.

Starting with the simple case, a `VertexGeom`, if a vertex/point (cell-level) falls inside the bounds it will be flagged as within the bounds in the mask. The same is true for `ImageGeom`. For `EdgeGeom`, the edges (cell-level) must have both points fall inside the bounds to be considered inside. For `TriangleGeom`, the faces (cell-level) must have all 3 points fall inside the bounds to be considered inside. For `QuadGeom`, the faces (cell-level) must have all 4 points fall inside the bounds to be considered inside.

### Sphere Bounding Type

The way a point is determined to be in the sphere uses the following calculation where `p` is the query point, `c` is the centroid of the sphere (provided from the first 3 values in "Sphere Info" parameter), and `r` is the radius of the sphere (provided from the 4th value in "Sphere Info" parameter):

`(p_x - c_x)^2 + (p_y - c_y)^2 + (p_z - c_z)^2 <= r^2`

### Inverting the Mask

This is primarily a convience option provided to the user. If toggled on the values will be true (`1`) by default and values withing the bounds will be marked false (`0`). This doesn't modify anything other than switching what value is set for bounds that fall inside or outside respectively.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
