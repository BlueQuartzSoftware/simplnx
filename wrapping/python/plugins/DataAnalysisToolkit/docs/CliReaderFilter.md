# Read CLI File

## Group (Subgroup)

IO (Input)

## Description

**Read CLI File** is a Python-based *simplnx* filter designed to read and process CLI (Common Layer Interface) files, typically used in additive manufacturing and 3D printing for representing slices of a 3D model.

This filter extracts geometric and attribute data from CLI files and organizes the data into the *simplnx* data structure by creating an edge geometry out of the imported data.

### Parameters

- `Input CLI File`: Filesystem path to the input CLI file.
- `Output Edge Geometry`: Path where the edge geometry data will be stored in the data structure.
- `Output Vertex Attribute Matrix Name`: Name for the output vertex attribute matrix.
- `Output Edge Attribute Matrix Name`: Name for the output edge attribute matrix.
- `Output Feature Attribute Matrix Name`: Name for the output feature attribute matrix.

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.
