# Compute Coordinate Threshold

## Group (Subgroup)

Geometry

## Description

This filter produces a mask that marks cells that fall inside or outside a given bounding shape within a supplied geometry. The filter outputs a mask to provide the greatest flexibility, while leveraging existing algorithms. This filter doesn't modify the input geometry in any way if you wish to modify the data within the bounding box, consider using one of the cleanup filters on the marked values. See *Remove Flagged Vertices/Edges/Triangles* for an example of a potential followup filter.

This filter has a check at runtime (during the execute phase, before individual mask value determination begins) that will return a warning (not error) if the bounds don't intersect any cells in the geometry. The intention of this is to alert the user that the mask will contain the same value throughout to allow the user to adapt the pipeline/parameters accordingly. When this check causes early bailout and warning, the mask will still be filled with outside bounds flag at every value. `ImageGeom` is the exception to this as it has checks done at preflight to prevent this from occurring.

There are several issues to be aware of with this filter, detailed thoroughly in the following sections.

### Input Geometry Types

This filter is meant to be as widely applicable as possible, so **cells will only be included in the bounding box if all points fall within the bounds**.

Starting with the simple case, a `VertexGeom`, if a vertex/point (cell-level) falls inside the bounding box, it will be flagged as within the bounds in the mask. For `EdgeGeom`, the edges (cell-level) must have both points fall inside the bounds to be considered inside. For `TriangleGeom`, the faces (cell-level) must have all 3 points fall inside the bounds to be considered inside. For `QuadGeom`, the faces (cell-level) must have all 4 points fall inside the bounds to be considered inside.

`ImageGeom` is a nuanced case. For it to be considered inside, all eight of the vertices making up the cell/voxel must fall within the bounds. This differs from various other places in the code where the centroids of the voxel are used to make determinations.

### Sphere Bounding Type

The way a point is determined to be in the sphere uses the following calculation where `p` is the query point, `c` is the centroid of the sphere (provided from the first 3 values in "Sphere Info" parameter), and `r` is the radius of the sphere (provided from the 4th value in "Sphere Info" parameter):

`(p_x - c_x)^2 + (p_y - c_y)^2 + (p_z - c_z)^2 <= r^2`

### Inverting the Mask

This is primarily a convenience option provided to the user. If toggled on the values will be true (`1`) by default and values withing the bounds will be marked false (`0`). This doesn't modify anything other than switching what value is set for bounds that fall inside or outside respectively.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
