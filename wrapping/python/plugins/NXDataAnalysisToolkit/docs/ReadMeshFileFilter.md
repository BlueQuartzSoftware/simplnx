# MeshIO: Read Mesh File

## Group (Subgroup)
Core (IO/Read)

## Description
The `ReadMeshFile` filter uses the Python MeshIO library to read a mesh file into a simplnx geometry.

The following mesh formats are supported by this filter:
+ Abaqus (.inp)
+ Ansys (.msh)
+ Gmsh (.msh)
+ Med (.med)
+ TetGen (.node/.ele)
+ VTK (.vtu)

*NOTE*: This filter makes the assumption (for cell and point data stored inside the mesh file) that the first element in the numpy shape list is the only tuple dimension, and every other shape list element is part of the component dimensions!

## Parameters

### Input Parameters
- **`Input File`** (str): The path to the input mesh file.

### Created Parameters
- **`Created Geometry`** (nx.DataPath): The path to the geometry that will be created.
- **`Vertex Attribute Matrix Name`** (str): The name of the vertex attribute matrix that will be created inside the geometry.
- **`Cell Attribute Matrix Name`** (str): The name of the cell attribute matrix that will be created inside the geometry.
- **`Vertex Array Name`** (str): The name of the vertex array that will be created inside the geometry.
- **`Cell Array Name`** (str): The name of the cell array that will be created inside the geometry.

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.
