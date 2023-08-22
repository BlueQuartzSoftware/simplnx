# Extract Vertex Geometry

## Group (Subgroup) ##

Core Filters (Conversion)

## Description ##

This filter will extract all the voxel centers of an Image Geometry or a RectilinearGrid geometry
into a new VertexGeometry. The user is given the option to copy or move cell arrays over to the 
newly created VertexGeometry.

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| Array Handling | int | 0=Move Arrays, 1=Copy Arrays |
| Use Mask | Bool | Use a mask array to determine which vertices are extracted |
| Mask | DataPath | The path to the mask array |
| Input Geometry | DataPath | The path to the input geometry that will be extracted into a vertex geometry |
| Included Attribute Arrays | std::vector<DataPath> | List of DataPaths to either copy or move |
| Output Vertex Geometry | DataPath | The path to the vertex geometry that will be output by this filter |
| Output Vertex Attribute Matrix Name | string | The name of the vertex attribute matrix that will be created as a child of the output vertex geometry |
| Output Shared Vertex List Name | string | The name of the shared vertex list name that will be output with the vertex geometry |

## Required Geometry ##

Required Geometry Type: **ImageGeom** or **RectGridGeom**

## Example Pipelines ##

PrebuiltPipelines/Examples/Extract Vertex Geometry.json

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users


## Python Filter Arguments

+ module: complex
+ Class Name: ExtractVertexGeometryFilter
+ Displayed Name: Extract Vertex Geometry

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| array_handling | Array Handling | Move or Copy input data arrays | complex.ChoicesParameter |
| included_data_array_paths | Included Attribute Arrays | The arrays to copy/move to the vertex array | complex.MultiArraySelectionParameter |
| input_geometry_path | Input Geometry | The input Image/RectilinearGrid Geometry to convert | complex.DataGroupSelectionParameter |
| mask_array_path | Mask | DataPath to the boolean mask array. Values that are true will mark that cell/point as usable. | complex.ArraySelectionParameter |
| shared_vertex_list_name | Output Shared Vertex List Name | The name of the shared vertex list that will be created | complex.DataObjectNameParameter |
| use_mask | Use Mask | Specifies whether or not to use a mask array | complex.BoolParameter |
| vertex_attr_matrix_name | Output Vertex Attribute Matrix Name | The name of the vertex attribute matrix that will be created | complex.DataObjectNameParameter |
| vertex_geometry_path | Output Vertex Geometry | The complete path to the vertex geometry that will be created | complex.DataGroupCreationParameter |

