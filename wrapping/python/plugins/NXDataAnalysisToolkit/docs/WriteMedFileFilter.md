# MeshIO: Write Med File

## Group (Subgroup)
Core (IO/Read)

## Description
The `WriteMedFile` filter uses the Python MeshIO library to write a simplnx geometry to a Med .med file.

## Parameters

### Input Parameters
- **`Input Geometry`** (nx.DataPath): Specifies the complete path to the input geometry.
- **`Cell Data Arrays To Write`** (List[nx.DataPath]): List of DataPaths to the cell data arrays to include in the output mesh file.
- **`Point Data Arrays To Write`** (List[nx.DataPath]): List of DataPaths to the point data arrays to include in the output mesh file.

### Output Parameters
- **`Output File`** (str): The path to the output .med file.

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.
