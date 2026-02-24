# Create Feature Boundaries (2D)

## Group (Subgroup)

SimplnxCore (Geometry)

## Description

This **Filter** extracts 2D feature boundaries from an Image Geometry and creates an Edge Geometry representing the outlines of features defined by a Feature IDs array. The filter operates only in the XY plane and requires the input Image Geometry to have a Z dimension of 1.

**NOTE**: The "FeatureIds" array can be any integer type, not just `signed int32` as is the typical featureIds arrays. This means that it can work immediately on scalar images that have not been segmented already. The user could use a basic threshold to create a unsigned int 8 mask value, then use that mask value as the input into this filter. This could be used, for example, to outline just the sample border if there is overscan or just outline specific regions of a 2D Image Geometry.

The algorithm scans the Feature IDs array and identifies boundaries where adjacent cells have different feature IDs. For each boundary, an edge (line segment) is created at the cell interface. The resulting Edge Geometry contains:

- **Shared Vertex List**: 3D coordinates of all unique vertices at feature boundaries
- **Shared Edge List**: Connectivity pairs defining line segments between vertices

### Z Value Options

The filter provides three options for setting the Z coordinate of the generated vertices:

1. **Use min z value from Image geometry**: Uses the origin Z value of the input Image Geometry
2. **Use max z value from Image geometry**: Uses the origin Z plus the spacing times the Z dimension
3. **Use Custom z Offset**: Allows specifying a custom Z value for all vertices

### Algorithm Details

The algorithm uses a two-pass approach for efficiency:

1. **Count Pass**: Scan to count the total number of boundary edges (both vertical and horizontal)
2. **Populate Pass**: Creation of vertices and edge connectivity

After edge creation, duplicate vertices are eliminated to ensure a clean, connected edge network.

## Example Output

![Images/ExtractFeatureBoundaries2D_1.png](Images/ExtractFeatureBoundaries2D_1.png)

% Auto generated parameter table will be inserted here

## Example Pipelines

- ExtractFeatureBoundaries2D.d3dpipline

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
