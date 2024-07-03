# MeshIO: Write Vtu File

## Group (Subgroup)
Core (IO/Read)

## Description
The `WriteVtuFile` filter uses the Python MeshIO library to write a simplnx geometry to a VTK .vtu file.

The output file can also be compressed if the user chooses LZMA or ZLIB as the Output Compression Type.

## Parameters

### Input Parameters
- **`Input Geometry`** (nx.DataPath): Specifies the complete path to the input geometry.
- **`Cell Data Arrays To Write`** (List[nx.DataPath]): List of DataPaths to the cell data arrays to include in the output mesh file.
- **`Point Data Arrays To Write`** (List[nx.DataPath]): List of DataPaths to the point data arrays to include in the output mesh file.

### Output Parameters
- **`Output File`** (str): The path to the output .vtu file.
- **`Output Compression Type`** (int): The compression type to use when writing the output file.  The choices are Uncompressed, LZMA, and ZLIB.

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.
