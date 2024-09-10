# Read CLI File

## Group (Subgroup)

IO (Input)

## Description

**Read CLI File** is a Python-based *simplnx* filter designed to read and process CLI (Common Layer Interface) files, typically used in additive manufacturing and 3D printing for representing slices of a 3D model.

This filter extracts geometric and attribute data from CLI files and organizes the data into the *simplnx* data structure by creating an edge geometry out of the imported data. It provides options to selectively mask the dataset based on X, Y, and Z dimensions to focus on specific parts of the model.

*Note*: If any edges in the dataset intersect the specified dimension range bound, one of the following behaviors will occur due to the specified Out-Of-Bounds Behavior option:

1. Interpolate Outside Vertex - Moves the outside vertex of a boundary-intersecting edge from its current position to the boundary edge.
2. Ignore Edge - Ignores any edge that intersects a bound.
3. Filter Error - Filter will throw an error when it encounters an edge that intersects a bound.

### Parameters

- `Input CLI File`: Filesystem path to the input CLI file.
- `Use X Dimension Range`: Enable this option to apply X bounds, masking out any dataset portions that fall outside the specified X dimension bounds.
- `X Min/Max`: Minimum and maximum coordinates for the X bounds.
- `Use Y Dimension Range`: Enable this option to apply Y bounds, masking out any dataset portions that fall outside the specified Y dimension bounds.
- `Y Min/Max`: Minimum and maximum coordinates for the Y bounds.
- `Use Z Dimension Range`: Enable this option to apply Z bounds, masking out any dataset portions that fall outside the specified Z dimension bounds.
- `Z Min/Max`: Minimum and maximum coordinates for the Z bounds.
- `Out-Of-Bounds Behavior`: The behavior to implement if an edge intersects a bound (one vertex is inside, one vertex is outside).
- `Read Extra Metadata`: Determines whether or not to read the extra metadata that may be included (per layer) in the file.
- `Create Edge Type Array`: Determines whether or not to create an unsigned 8-bit integer array that labels hatches edges with 0 and polyline edges with 1.
- `Output Edge Geometry`: Path where the edge geometry data will be stored in the data structure.
- `Output Vertex Attribute Matrix Name`: Name for the output vertex attribute matrix.
- `Output Edge Attribute Matrix Name`: Name for the output edge attribute matrix.
- `Output Feature Attribute Matrix Name`: Name for the output feature attribute matrix.
- `Shared Vertices Array Name`: Name of the shared vertices array created.
- `Shared Edges Array Name`: Name of the shared edges array created.

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report, or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GitHub site where the community of DREAM3D-NX users can help answer your questions.