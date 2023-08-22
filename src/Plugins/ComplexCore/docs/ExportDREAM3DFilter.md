# ExportDREAM3DFilter

## Description ##

This **Filter** dumps the data structure to an hdf5 file with the .dream3d extension.

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| Export File Path | Filesystem Path | The file path the DataStructure should be written to as an HDF5 file. |
| Write Xdmf File | bool | Whether or not to write the data out an xdmf file |


## Required Geometry ###

None

## Required Objects ##

None

## Created Objects ##

None

## Example Pipelines ##

ALL

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: ExportDREAM3DFilter
+ Displayed Name: Write DREAM3D NX File

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| export_file_path | Export File Path | The file path the DataStructure should be written to as an HDF5 file. | complex.FileSystemPathParameter |
| write_xdmf_file | Write Xdmf File | Whether or not to write the data out an xdmf file | complex.BoolParameter |

