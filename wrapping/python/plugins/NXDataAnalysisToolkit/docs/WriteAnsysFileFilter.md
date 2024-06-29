# MeshIO: Write Ansys File

## Group (Subgroup)
Core (IO/Read)

## Description
The `WriteAnsysFile` filter uses the Python MeshIO library to write a simplnx geometry to an Ansys .msh file.

*Note*: This filter does not support writing cell data or point data to the mesh file.

## Parameters

### Input Parameters
- **`Input Geometry`** (nx.DataPath): Specifies the complete path to the input geometry.

### Output Parameters
- **`Output File`** (str): The path to the output .msh file.

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.
