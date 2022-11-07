# Remove Flagged Vertices #

## Group (Subgroup) ##

DREAM3D Review (Geometry)

## Description ##

This **Filter** removes **Vertices** from the supplied **Vertex Geometry** that are flagged by a boolean mask array.  Specifically, **Vertices** flagged as _true_ are removed from the **Geometry**.  A new reduced **Vertex Geometry** is created that contains all the remaining **Vertices**.  It is unknown until run time how many **Vertices** will be removed from the **Geometry**.  Therefore, this **Filter** requires that a new **Data Container** be created to contain the reduced **Vertex Geometry**.  This new **Data Container** will contain copies of any **Feature** or **Ensemble** **Attribute Matrices** from the original **Data Container**.  Additionally, all **Vertex** data will be copied, with tuples _removed_ for any **Vertices** removed by the **Filter**.  The user must supply a name for the reduced **Data Container**, but all other copied objects (**Attribute Matrices** and **Attribute Arrays**) will retain the same names as the original source.

_Note:_ Since it cannot be known before run time how many **Vertices** will be removed, the new **Vertex Geometry** and all associated **Vertex** data to be copied will be initialized to have size 0.  Any **Feature** or **Ensemble** information will retain the same dimensions and size.     

## Parameters ##

None

## Required Geometry ###

Vertex

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Data Container** | VertexDataContainer | N/A | N/A | **Data Container** holding the **Vertex Geometry** to reduce |
| **Vertex Attribute Array** | Mask | bool | (1) | Mask array specifying which **Vertices* to remove |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| Reduced **Data Container** | ReducedVertexDataContainer | N/A | N/A | **Data Container** holding the reduced **Vertex Geometry** and any copied **Attribute Matrices** and **Attribute Arrays** |

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
