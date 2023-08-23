# Generate Triangle Areas


## Group (Subgroup) ##

Surface Meshing (Misc)

## Description ##

This **Filter** computes the area of each **Triangle** in a **Triangle Geometry** by calculating the following: 
    
    1/2*|AB||AC|sin(O)

where *O* is the angle between |AB| and |AC|.

## Parameters ##

None

## Required Geometry ##

Triangle

## Required Objects ##

None

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Face Attribute Array**  | FaceAreas | double | (1) | Specifies the area of each **Face** |


## Example Pipelines ##

+ (03) SmallIN100 Mesh Statistics
+ Triangle_Face_Data_Demo.d3dpipeline

## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: CalculateTriangleAreasFilter
+ Displayed Name: Calculate Triangle Areas

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| triangle_areas_array_path | Created Face Areas | The complete path to the array storing the calculated face areas | complex.DataObjectNameParameter |
| triangle_geometry_data_path | Triangle Geometry | The complete path to the Geometry for which to calculate the face areas | complex.GeometrySelectionParameter |

