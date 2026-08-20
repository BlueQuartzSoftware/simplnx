# Voxelize Point Cloud

## Group (Subgroup)

Core (Geometry)

## Description

This **Filter** maps a node-based point cloud geometry onto a regular grid and produces a **UInt8** voxel mask array. Each voxel in the mask is set to *1* if one or more points from the point cloud fall within that voxel, and *0* otherwise.

The filter supports three operating modes selected by the **Use Existing Grid Geometry** toggle:

### Auto-sized Image Geometry (default)

When **Use Existing Grid Geometry** is *false*, the filter creates a new **Image Geometry** that tightly wraps the input point cloud:

1. The axis-aligned bounding box of the point cloud is computed.
2. A padding of **0.1%** of each side length is added to both the minimum and maximum extents, ensuring boundary points are not clipped.
3. Grid dimensions are computed as `ceil(padded_extent / spacing)` per axis, where spacing defaults to *1.0* in all axes.
4. The origin is set to the padded minimum point.

The half-open interval `[origin, origin + dims × spacing)` defines which points map into each cell. A point exactly on the maximum boundary is therefore excluded and falls outside the last cell. The 0.1% padding guarantees that no input point lands on this boundary after the bounding box is expanded.

A zero-extent dimension — which occurs when the point cloud is degenerate (e.g., a single point or all points collinear along an axis) — produces an error because zero-dimensional geometries are invalid. A minimum grid size of 1 is required in every axis.

### Existing Image Geometry

When **Use Existing Grid Geometry** is *true* and the destination is an **Image Geometry**, each point is mapped to a cell using the same half-open interval semantics as above. Points that fall outside the geometry's extent are silently skipped.

Cell index per axis: `floor((point - origin) / spacing)`. The mask is written to the **Cell Data Attribute Matrix** of the destination geometry.

### Existing Rectilinear Grid Geometry

When **Use Existing Grid Geometry** is *true* and the destination is a **RectGrid Geometry**, each point is placed using a binary search over the per-axis boundary arrays. The cell index along each axis is determined as the index of the first boundary value strictly greater than the point coordinate (`std::upper_bound`). A point exactly on an interior boundary is assigned to the **upper** cell. Points outside the grid extent in any axis are silently skipped.

% Auto generated parameter table will be inserted here

## Notes

- The voxel mask is written as a **UInt8 Data Array** named by the **Voxel Mask Name** parameter, stored inside the **Cell Data Attribute Matrix** of the output or destination geometry.
- Any node-based geometry type is accepted as the point cloud source: Vertex, Edge, Triangle, Quad, Tetrahedral, or Hexahedral. Only the vertex positions are used.
- When using an existing geometry, the mask **Data Array** is pre-allocated to match the existing cell dimensions during preflight. When auto-sizing, a placeholder 1×1×1 geometry is created at preflight and resized to the final dimensions during execution.
- To voxelize onto a grid with a **specific origin, spacing, or dimensions**, first create the desired **Image Geometry** using the **Create Geometry** filter, then run this filter with **Use Existing Grid Geometry** enabled and select that geometry as the destination.

## Error Codes

| Code | Condition |
|------|-----------|
| -45980 | The point cloud has no valid bounding box (empty point cloud). |
| -45981 | One or more computed grid dimensions are zero (degenerate point cloud with zero spatial extent along at least one axis). |

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
