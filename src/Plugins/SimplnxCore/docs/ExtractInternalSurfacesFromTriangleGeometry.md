# Extract Internal Surfaces from Triangle Geometry

## Group (Subgroup)

Geometry

## Description

This **Filter** extracts any **Triangles** from the supplied **Triangle Geometry** that contain any *internal nodes*, then uses these extracted **Triangles** to create a new **Data Container** with the reduced **Triangle Geometry**.  This operation is the same as removing all **Triangles** that only lie of the outer surface of the supplied **Triangle Geometry**.  The user must supply a "Node Type" **Vertex Attribute Array** that defines the type for each node of the **Triangle Geometry**.  Node types may take the following values:

### Quick Surface Mesh

| Id Value | Node Type |
|----------|-----------|
| 2 | Normal **Vertex |
| 3 | Triple Line |
| 4 | Quadruple Point |
| 12 | Normal **Vertex** on the outer surface |
| 13 | Triple Line on the outer surface |
| 14 | Quadruple Point on the outer surface |

### SurfaceNets Mesh

| Id Value | Node Type |
|----------|-----------|
| 0 | Normal **Vertex |
| 3 | Triple Line |
| 4-8 | Quadruple Point |
| 13 | Normal **Vertex** on the outer surface |
| 14 | Triple Line on the outer surface |
| 15-18 | Quadruple Point on the outer surface |

This **Filter** has the effect of removing any **Triangles** that only contain **Vertices** whose node Id values fall outside of the min and max that the user sets.  In general, this *node type* array is created when the original surface mesh is created.

It is unknown until runtime how the **Geometry** will be changed by removing certain **Vertices** and **Triangles**.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.
