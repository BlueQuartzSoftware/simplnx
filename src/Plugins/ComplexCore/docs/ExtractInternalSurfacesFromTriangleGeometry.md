# Extract Internal Surfaces from Triangle Geometry

## Group (Subgroup) ##

Geometry

## Description ##

This **Filter** extracts any **Triangles** from the supplied **Triangle Geometry** that contain any _internal nodes_, then uses these extracted **Triangles** to create a new **Data Container** with the reduced **Triangle Geometry**.  This operation is the same as removing all **Triangles** that only lie of the outer surface of the supplied **Triangle Geometry**.  The user must supply a **Vertex Attribute Array** that defines the type for each node of the **Triangle Geometry**.  Node types may take the following values:

| Id Value | Node Type |
|----------|-----------|
| 2 | Normal **Vertex** |
| 3 | Triple Line |
| 4 | Quadruple Point |
| 12 | Normal **Vertex** on the outer surface |
| 13 | Triple Line on the outer surface |
| 14 | Quadruple Point on the outer surface |

This **Filter** has the effect of removing any **Triangles** that only contain **Vertices** whose node Id values are 12, 13, or 14.  In general, this _node type_ array is created when the [original surface mesh is created](@ref quicksurfacemesh).   

It is unknown until runtime how the **Geometry** will be changed by removing certain **Vertices** and **Triangles**.

## Parameters ##

None

## Required Geometry ###

Triangle

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Vertex Data Array** | NodeTypes | int8 | (1) | Specifies the type of node in the **Geometry** |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| Geometry | None | Triangle | N/A | The new Triangle Geometry |
| Attribute Matrix | Vertex Data | N/A | N/A | Created vertex data AttributeMatrix name |
| Attribute Matrix | Face Data | N/A | N/A | Created face data AttributeMatrix name |

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this plugin.
