# MeshIO: Write TetGen File

## Group (Subgroup)
Core (IO/Read)

## Description
The `WriteTetGenFile` filter uses the Python MeshIO library to write a simplnx tetrahedral geometry to TetGen .node and .ele files.

*Note*: Either .node or .ele can be chosen as the output file; the one that is not chosen will also be automatically created.

## Parameters

### Input Parameters
- **`Input Geometry`** (nx.DataPath): Specifies the complete path to the input geometry.
- **`Cell Data Arrays To Write`** (List[nx.DataPath]): List of DataPaths to the cell data arrays to include in the output mesh file.
- **`Point Data Arrays To Write`** (List[nx.DataPath]): List of DataPaths to the point data arrays to include in the output mesh file.

### Output Parameters
- **`Output File`** (str): The path to the output .node/.ele file.  Regardless of which extension is chosen as the output file, both .node and .ele files will be written to the chosen location with the chosen file name.

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.
