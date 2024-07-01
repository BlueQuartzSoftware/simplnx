# Read CLI File

## Group (Subgroup)

IO (Input)

## Description

**Read CLI File** is a Python-based *simplnx* filter designed to read and process CLI (Common Layer Interface) files, typically used in additive manufacturing and 3D printing for representing slices of a 3D model.

This filter extracts geometric and attribute data from CLI files and organizes the data into the *simplnx* data structure by creating an edge geometry out of the imported data. It provides options to selectively mask the dataset based on X, Y, and Z dimensions to focus on specific parts of the model.

*Note*: If any edges in the dataset straddle the specified mask bounds, this filter will return an error.

### Parameters

- `Input CLI File`: Filesystem path to the input CLI file.
- `Mask X Dimension`: Enable this option to apply X bounds, masking out any dataset portions that fall outside the specified X dimension bounds.
- `X Min/Max`: Minimum and maximum coordinates for the X bounds.
- `Mask Y Dimension`: Enable this option to apply Y bounds, masking out any dataset portions that fall outside the specified Y dimension bounds.
- `Y Min/Max`: Minimum and maximum coordinates for the Y bounds.
- `Mask Z Dimension`: Enable this option to apply Z bounds, masking out any dataset portions that fall outside the specified Z dimension bounds.
- `Z Min/Max`: Minimum and maximum coordinates for the Z bounds.
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