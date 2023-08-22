# Read STL File 


## Group (Subgroup) ##

IO (Input)

## Description ##

This **Filter**  will read a binary STL File and create a **Triangle Geometry** object in memory. The STL reader is very strict to the STL specification. An explanation of the STL file format can be found on [Wikipedia](https://en.wikipedia.org/wiki/STL). The structure of the file is as follows:

	UINT8[80]     Header
	UINT32     Number of triangles

	foreach triangle
	REAL32[3]     Normal vector
	REAL32[3]     Vertex 1
	REAL32[3]     Vertex 2
	REAL32[3]     Vertex 3
	UINT16     Attribute byte count
	end

**It is very important that the "Attribute byte Count" is correct as DREAM.3D follows the specification strictly.** If you are writing an STL file be sure that the value for the "Attribute byte count" is _zero_ (0). If you chose to encode additional data into a section after each triangle then be sure that the "Attribute byte count" is set correctly. DREAM.3D will obey the value located in the "Attribute byte count".

## Parameters ##

| Name | Type | Description |
|------|------|------|
| STL File | File Path  | The input .stl file path |
| Scale Output | Bool | Should the output vertex values be scaled |
| Scale Factor | Float | Apply the scaling factor to each vertex |
| Vertex Attribute Matrix | String | Name of the created Vertex Attribute Matrix |
| Face Attribute Matrix | String | Name of the created Face Attribute Matrix |
| Shared Vertex Attribute Matrix | String | Name of the created Shared Vertex Attribute Matrix |
| Shared Face Attribute Matrix | String | Name of the created Shared Face Attribute Matrix |

## Required Geometry ##

Not Applicable

## Required Objects ##

None

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Data Container** | TriangleDataContainer  | N/A | N/A | Created **Data Container** name with a **Triangle Geometry** |
| **Attribute Matrix** | FaceData  | Face | N/A | Created **Face Attribute Matrix** name  |
| **Face Attribute Array** | FaceNormals  | double | (3) | Specifies the normal of each **Face** |


## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: StlFileReaderFilter
+ Displayed Name: Import STL File

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| face_matrix | Face Matrix Name | Name of the created Face Attribute Matrix | complex.StringParameter |
| geometry_data_path | Geometry Name [Data Group] | The complete path to the DataGroup containing the created Geometry data | complex.DataGroupCreationParameter |
| scale_factor | Scale Factor | The factor by which to scale the geometry | complex.Float32Parameter |
| scale_output | Scale Output Geometry | Scale the output Triangle Geometry by the Scaling Factor | complex.BoolParameter |
| shared_face_matrix | Shared Face Matrix Name | Name of the created Shared Face Attribute Matrix | complex.StringParameter |
| shared_vertex_matrix | Shared Vertex Matrix Name | Name of the created Shared Vertex Attribute Matrix | complex.StringParameter |
| stl_file_path | STL File | Input STL File | complex.FileSystemPathParameter |
| vertex_matrix | Vertex Matrix Name | Name of the created Vertex Attribute Matrix | complex.StringParameter |

