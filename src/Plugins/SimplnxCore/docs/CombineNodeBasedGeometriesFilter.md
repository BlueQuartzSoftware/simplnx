# Combine Node Based Geometries

## Group (Subgroup)

Core (Combining)

## Description

This **Filter** merges two or more **node-based geometries** into a single combined geometry. A **node-based geometry** is any geometry built from a shared list of vertices (nodes) plus a connectivity that defines its elements: a **Vertex Geometry** (points only), an **Edge Geometry** (line segments), a **Triangle** or **Quadrilateral Geometry** (surface faces), or a **Tetrahedral** or **Hexahedral Geometry** (solid cells).

When combining, the filter concatenates the vertex lists of all inputs into one list and then renumbers the connectivity. The element (cell) indices from each input geometry are shifted by the running total number of vertices contributed by all preceding geometries, so that every element continues to reference the correct vertices in the merged vertex list. **Duplicate or coincident vertices are *not* detected or merged** — if two inputs contain a vertex at the same physical location, both copies appear in the output.

The algorithm is governed by several rules:

1. All input geometries must have the same geometry type. For example, combining only **Triangle Geometries** produces a combined **Triangle Geometry**, and combining only **Edge Geometries** produces a combined **Edge Geometry**.
2. All input geometries must contain vertex and element data arrays with the exact same names, types, and component dimensions. For example, if one input has an edge data array, every input must have an edge data array with a matching name, type, and component dimension; the same requirement applies to vertex data arrays.
3. A **higher-order** geometry (one whose elements are built from more vertices, such as a **Tetrahedral Geometry**) may also carry the data of its **lower-order** elements. For example, a tetrahedral geometry may include edge data, as long as every input tetrahedral geometry includes matching edge data.

*NOTE:* Any additional groups, attribute matrices, or arrays that are not one of the following:

1. Vertex arrays or vertex data
2. Edge arrays or edge data
3. Face arrays or face data
4. Polyhedra arrays or polyhedra data

will be ignored and will not appear in the geometry produced by this filter.

### Required Input Sources

- **Input Geometries** -- two or more **node-based geometries** of the *same* type. Surface meshes are typically produced by [Create Surface Mesh (QuickMesh)](QuickSurfaceMeshFilter.md); edge geometries by [Slice Triangle Geometry](SliceTriangleGeometryFilter.md) or [Create AM Scan Paths](CreateAMScanPathsFilter.md).

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
