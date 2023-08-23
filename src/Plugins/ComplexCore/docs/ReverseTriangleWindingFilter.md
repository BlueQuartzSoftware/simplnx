# Reverse Triangle Winding

## Group (Subgroup)

Surface Meshing (Connectivity/Arrangement)

## Description

This **Filter** reverses the *winding* for each **Triangle** in a **Triangle Geometry**. This will *reverse* the direction of calculated **Triangle** normals. Some analysis routines require the normals to be pointing "away" from the center of a **Feature**. This **Filter** allows for manipulation of this construct.

## Parameters

None

## Required Geometry

Triangle

## Required Objects

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Data Container** | None | N/A | N/A | **Data Container** that holds the **Triangle Geometry** to reverse winding |

## Created Objects

None

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: ReverseTriangleWindingFilter
+ Displayed Name: Reverse Triangle Winding

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| triangle_geometry_path | Triangle Geometry | The DataPath to then input Triangle Geometry | complex.GeometrySelectionParameter |

