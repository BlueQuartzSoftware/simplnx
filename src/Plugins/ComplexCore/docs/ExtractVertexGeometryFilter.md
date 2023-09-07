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

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.


