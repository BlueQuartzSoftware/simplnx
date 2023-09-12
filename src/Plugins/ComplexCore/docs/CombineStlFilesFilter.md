# Combine STL Files 

## Group (Subgroup) 

AMProcessMonitoring (AMProcessMonitoring)

## Description 

This **Filter** combines all of the STL files from a given directory into a single triangle geometry. This filter will make use of the **Import STL File Filter** to read in each stl file in the given directory and then will proceed to combine each of the imported files into a single triangle geometry.

## Parameters 
| Name | Type | Description |
|------|------|------|
| Path to STL Files | Directory Path | The path to the folder containing all the STL files to be combined |

## Required Geometry 
Not Applicable

## Created Objects 
| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|-------------|---------|-----|
| **Geometry** | TriangleGeometry  | N/A | N/A | The path to the triangle geometry to be created from the combined STL files |
| **Face Attribute Matrix** | FaceData  | Face | N/A | Created **Face Attribute Matrix** name for the combined geometry  |
| **Face Attribute Array** | FaceNormals  | double | (3) | Specifies the normal of each **Face** in the combined geometry |
| **Vertex Attribute Matrix** | VertexData  | Vertex | N/A | Created **Vertex Attribute Matrix** name for the combined geometry |

## Example Pipelines

CombineStlFiles

## License & Copyright 

Please see the description file distributed with this plugin.

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.


