# Apply Transformation to Geometry

## Group (Subgroup) ##

Rotation & Transformation

## Description ##

This **Filter** applies a spatial transformation to an unstructured **Geometry**.  An "unstructured" **Geometry** is any geometry that requires explicit definition of **Vertex** positions.  Specifically, **Vertex**, **Edge**, **Triangle**, **Quadrilateral**, and **Tetrahedral** **Geometries** may be transformed by this **Filter**.  The transformation is applied in place, so the input **Geometry** will be modified.

The user may select from a variety of options for the type of transformation to apply:

| Enum Value |Transformation Type    | Representation                                                                       |
|------------|-----------------------|--------------------------------------------------------------------------------------|
| 0          | No Transformation                  | Identity transformation                                                              | 
| 1          | Pre-Computed Transformation Matrix | A 4x4 transformation matrix, supplied by an **Attribute Array** in _row major_ order |
| 2          | Manual Transformation Matrix       | Manually entered 4x4 transformation matrix                                           | 
| 3          | Rotation                           | Rotation about the supplied axis-angle <x,y,z> (Angle in Degrees).                   | 
| 4          | Translation                        | Translation by the supplied (x, y, z) values                                         |
| 5          | Scale                              | Scaling by the supplied (x, y, z) values                                             |

## Parameters ##

| Name                                        | Type        | Description                                                                                   |
|---------------------------------------------|-------------|-----------------------------------------------------------------------------------------------|
| Transformation Type                         | Enumeration | Type of transformation to be used. (0-5)                                                      |
| Transformation Matrix                       | float (4x4) | Entries of the 4x4 transformation matrix, if _Manual_ is chosen for the _Transformation Type_ |
| Rotation Axis-Angle (ijk)                   | float (4x)  | axis-angle <x,y,z> (Angle in Degrees)                                                         |
| Translation                                 | float (3x)  | (x, y, z) translation values, if _Translation_ is chosen for the _Transformation Type_        |
| Scale                                       | float (3x)  | (x, y, z) scale values, if _Scale_ is chosen for the _Transformation Type_                    |
| Precomputed Transformation Matrix Data Path | DataPath    |                                                                                               |
| Geometry to be transformed.                 | DataPath    |                                                                                               | 

## Required Geometry ###

Any unstructured **Geometry**

## Required Objects ##

| Kind                | Default Name | Type | Component Dimensions | Description |
|---------------------|--------------|------|----------------------|-------------|
| **Geometry**        | None | N/A | N/A                  | The **Data Container** holding the unstructured **Geometry** to transform |
| **DataArray** | TransformationMatrix | float | 4x4                  | The pre-computed transformation matrix to apply, if _Pre-Computed_ is chosen for the _Transformation Type_ |

## Created Objects ##

None

## Example Pipelines ##


## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
