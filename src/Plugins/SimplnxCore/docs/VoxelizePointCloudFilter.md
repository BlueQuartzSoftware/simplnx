# Voxelize Point Cloud

## Group (Subgroup)

Core (Geometry)

## Description

This **Filter** maps a node-based point cloud geometry onto a regular grid and produces a **UInt8** voxel mask array. Each voxel in the mask is set to *1* if one or more points fall within it, and *0* otherwise. The mask is binary: duplicate points and multiple points per voxel all collapse to a single *1*; no count is accumulated.

The filter operates in two modes selected by the **Use Existing Grid Geometry** toggle. When using an existing geometry, behavior differs by destination type:

### Auto-sized Image Geometry (default)

When **Use Existing Grid Geometry** is *false*, the filter creates a new **Image Geometry** that tightly wraps the input point cloud:

1. The axis-aligned bounding box of the point cloud is computed.
2. A padding of **0.1%** of each side length is added to both the minimum and maximum extents, ensuring boundary points are not clipped.
3. Grid dimensions are computed as `ceil(padded_extent / spacing)` per axis, where spacing defaults to *1.0* in all axes.
4. The origin is set to the padded minimum point.

The half-open interval `[origin, origin + dims × spacing)` defines which points map into each cell. A point exactly on the maximum boundary is therefore excluded and falls outside the last cell. The 0.1% padding is designed to keep input points away from this boundary, though at very large coordinate magnitudes (≈ 1e5 and above) float32 precision limits may reduce its effect.

A zero-extent dimension — which occurs when the point cloud is degenerate (e.g., a single point or all points coplanar along an axis) — is clamped to 1, producing a geometry with a single-voxel slice along that axis. All input points still map into the resulting grid.

### Existing Image Geometry

When **Use Existing Grid Geometry** is *true* and the destination is an **Image Geometry**, each point is mapped to a cell using the same half-open interval semantics as above. Points that fall outside the geometry's extent or have non-finite coordinates (NaN, ±Inf) are skipped and counted in the end-of-execution warning.

Cell index per axis: `floor((point - origin) / spacing)`. The mask is written to the **Cell Data Attribute Matrix** of the destination geometry.

### Existing Rectilinear Grid Geometry

When **Use Existing Grid Geometry** is *true* and the destination is a **RectGrid Geometry**, each point is placed using a binary search over the per-axis boundary arrays. The cell index along each axis is determined as the index of the first boundary value strictly greater than the point coordinate (`std::upper_bound`). A point exactly on an interior boundary is assigned to the **upper** cell. Points outside the grid extent in any axis or with non-finite coordinates (NaN, ±Inf) are skipped and counted in the end-of-execution warning.

### Related Filters

**[Map Point Cloud to Regular Grid](MapPointCloudToRegularGridFilter.md)** answers the inverse question: *for each point, which voxel does it fall in?* It writes a **uint64** voxel index per point into the vertex Attribute Matrix, leaving the grid itself unchanged. Use that filter when you need per-point grid coordinates; use this filter when you need a per-voxel occupancy mask.

### Notes

- The voxel mask is written as a **UInt8 Data Array** named by the **Voxel Mask Name** parameter, stored inside the **Cell Data Attribute Matrix** of the output or destination geometry.
- Any node-based geometry type is accepted as the point cloud source: Vertex, Edge, Triangle, Quad, Tetrahedral, or Hexahedral. Only the vertex positions are used.
- When using an existing geometry, the mask **Data Array** is pre-allocated to match the existing cell dimensions during preflight. When auto-sizing, a placeholder 1×1×1 geometry is created at preflight and resized to the final dimensions during execution.
- To voxelize onto a grid with a **specific origin, spacing, or dimensions**, first create the desired **Image Geometry** using the **Create Geometry** filter, then run this filter with **Use Existing Grid Geometry** enabled and select that geometry as the destination.

### Errors

| Code | Severity | Condition |
|------|----------|-----------|
| -45980 | Error | Point cloud bounding box is invalid. The point cloud is empty or contains only non-finite coordinates. Auto-size path only. |
| -45982 | Error | Grid allocation failed (`std::bad_alloc`). The point cloud extent relative to the current spacing produces a grid too large to fit in memory. Auto-size path only. |
| -45988 | Error | Destination geometry type is not Image Geometry or RectGrid Geometry. Defensive; unreachable under normal operation because the parameter gates these types. |

A **warning** (no error code) is emitted after execution if any points were skipped. Skipped points include those with non-finite coordinates (NaN, ±Inf) and those that fall outside the destination geometry. The message reports the count of skipped points out of the total.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
