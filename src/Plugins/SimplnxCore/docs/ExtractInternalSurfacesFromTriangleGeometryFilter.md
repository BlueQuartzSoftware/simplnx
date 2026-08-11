# Extract Internal Surfaces From Triangle Geometry

## Group (Subgroup)

Geometry

## Description

This **Filter** extracts any **Triangles** from the supplied **Triangle Geometry** that contain any *internal nodes*, then uses these extracted **Triangles** to create a new **Data Container** with the reduced **Triangle Geometry**.  This operation is the same as removing all **Triangles** that only lie of the outer surface of the supplied **Triangle Geometry**.  Internal surfaces can be identified two ways, chosen with the **Internal Surface Criterion** parameter below.

### Node Type Range Criterion

With the **Node Type Range** criterion, the user must supply a "Node Type" **Vertex Attribute Array** that defines the type for each node of the **Triangle Geometry**. All three surface meshing **Filters** (Create Surface Mesh (QuickMesh), Create Surface Mesh (Surface Nets), and Create Surface Mesh (M3C)) emit Node Types in the same convention:

| Id Value | Node Type |
|----------|-----------|
| 2 | Normal **Vertex** |
| 3 | Triple Line |
| 4 | Quadruple Point (four or more Features meet) |
| 12 | Normal **Vertex** on the outer surface of the bounding box |
| 13 | Triple Line on the outer surface of the bounding box |
| 14 | Quadruple Point on the outer surface of the bounding box |

The value is the number of distinct Features meeting at the node, capped at 4, plus 10 when
the node lies on the bounding box wall.

This **Filter** removes any **Triangle** that contains only **Vertices** whose node Id values fall outside the minimum and maximum that the user sets.

It is unknown until runtime how the **Geometry** will be changed by removing certain **Vertices** and **Triangles**.

### Choosing a Criterion

**Node Type Range** keeps a triangle only when all three of its nodes have a Node Type
inside the given range. Because every node on the bounding box wall is promoted by 10, this
also discards internal boundary triangles that merely *touch* the wall — leaving a
one-triangle-wide gap wherever an internal surface meets the box. A Feature flush with the
box wall will come out open.

**Face Labels** instead discards only the faces whose Face Labels are `{-1, 0}`: the
bounding box wall where it borders the background. Faces where the wall caps a real Feature
are kept, so Features flush with the box stay closed, and no rim is eroded. This is the
same rule as the **Omit Bounding Box Skin** option on the surface meshing filters, and is
the better choice for meshes that already exist.

% Auto generated parameter table will be inserted here

- **Triangle Geometry** -- the surface mesh to process, typically produced by [Create Surface Mesh (Surface Nets)](SurfaceNetsFilter.md) or [Create Surface Mesh (QuickMesh)](QuickSurfaceMeshFilter.md).
- **Node Types** -- the per-vertex node classification array, produced by the same surface-meshing filter that created the geometry.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
