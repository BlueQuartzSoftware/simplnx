# Voxelize Point Cloud

## Group (Subgroup)

Core (Geometry)

## Description

This **Filter** maps a node-based point cloud geometry onto a regular grid and produces a **UInt8** voxel mask array. Each voxel in the mask is set to *1* if one or more points fall within it, and *0* otherwise. The mask is binary: duplicate points and multiple points per voxel all collapse to a single *1*; no count is accumulated.

The **Select the partitioning mode** parameter controls how the output grid's dimensions, origin, and spacing are determined. The four modes are described below.

### Basic Mode (default)

When **Select the partitioning mode** is *Basic (0)*, the filter creates a new **Image Geometry** sized automatically to the input point cloud. **Number Of Cells Per Axis** sets the exact cell count in each dimension; the filter derives spacing to fit those cells over the padded bounding box.

The sizing procedure at execute time:

1. The axis-aligned bounding box of the point cloud is computed.
2. A padding of **0.1%** of each side length is added to both the minimum and maximum extents. If the proportional padding is too small to affect the boundary float32 values (which can occur at large coordinate magnitudes), the filter expands by at least one float32 ULP per boundary face instead. This guarantees all input points fall strictly inside the generated bounds.
3. The output grid has exactly **N cells per axis**, where N is the value of **Number Of Cells Per Axis**. Spacing is derived as `padded_extent / N` per axis so that the N cells exactly cover the padded region.
4. The origin is set to the padded minimum point.

Preflight creates a placeholder geometry with the correct cell count. The exact origin and spacing are finalized at execute time once the point cloud's bounding box is known.

The half-open interval `[origin, origin + dims × spacing)` defines which points map into each cell; a point exactly on the maximum boundary falls outside the last cell. The padding prevents input points from reaching this boundary.

A zero-extent dimension — which occurs when the point cloud is degenerate (e.g., a single point or all points coplanar along an axis) — receives a spacing derived from the nextafter-expanded extent, producing a single-voxel slice along that axis. All input points still map into the resulting grid.

### Advanced Mode

When **Select the partitioning mode** is *Advanced (1)*, the filter creates a new **Image Geometry** with a fully user-specified geometry. The following parameters are used:

- **Number Of Cells Per Axis** — exact cell count along each axis.
- **Partition Grid Origin** — the origin (minimum corner) of the output grid.
- **Cell Length (Physical Units)** — the spacing in physical units along each axis.

The output grid's dimensions, origin, and spacing are set exactly from these parameters at preflight time. No bounding-box computation is performed.

### Bounding Box Mode

When **Select the partitioning mode** is *Bounding Box (2)*, the filter creates a new **Image Geometry** bounded by user-supplied coordinates. The following parameters are used:

- **Number Of Cells Per Axis** — exact cell count along each axis.
- **Minimum Grid Coordinate** — the lower-left corner of the bounding box.
- **Maximum Grid Coordinate** — the upper-right corner of the bounding box.

Spacing along each axis is derived as `(max - min) / N`. If `max == min` along an axis (degenerate flat dimension), the filter expands the extent by ±1 float32 ULP so that points on that plane receive a valid finite cell index rather than NaN. The origin is set to the minimum coordinate.

### Existing Partition Grid Mode

When **Select the partitioning mode** is *Existing Partition Grid (3)*, the filter maps the point cloud onto a pre-existing grid geometry selected by **Existing Partition Grid Path**. No new geometry is created; the mask is written into the **Cell Data Attribute Matrix** of the chosen geometry. Behavior differs by destination type:

#### Existing Image Geometry

Each point is mapped to a cell using the same half-open interval semantics described under Basic Mode. Cell index per axis: `floor((point - origin) / spacing)`. Points that fall outside the geometry's extent or have non-finite coordinates (NaN, ±Inf) are skipped and counted in the end-of-execution warning.

#### Existing Rectilinear Grid Geometry

Each point is placed using a binary search over the per-axis boundary arrays. The cell index along each axis is determined as the index of the first boundary value strictly greater than the point coordinate (`std::upper_bound`). A point exactly on an interior boundary is assigned to the **upper** cell. Points outside the grid extent in any axis or with non-finite coordinates (NaN, ±Inf) are skipped and counted in the end-of-execution warning.

### Related Filters

**[Map Point Cloud to Regular Grid](MapPointCloudToRegularGridFilter.md)** answers the inverse question: *for each point, which voxel does it fall in?* It writes a **uint64** voxel index per point into the vertex Attribute Matrix, leaving the grid itself unchanged. Use that filter when you need per-point grid coordinates; use this filter when you need a per-voxel occupancy mask.

### Notes

- The voxel mask is written as a **UInt8 Data Array** named by the **Voxel Mask Name** parameter, stored inside the **Cell Data Attribute Matrix** of the output or destination geometry.
- Any node-based geometry type is accepted as the point cloud source: Vertex, Edge, Triangle, Quad, Tetrahedral, or Hexahedral. Only the vertex positions are used.
- In Existing Partition Grid mode, the mask **Data Array** is pre-allocated to match the existing cell dimensions during preflight. In Basic, Advanced, and Bounding Box modes, preflight creates a new geometry with the user-requested cell count; origin and spacing are refined at execute time for Basic mode, or set exactly from user parameters for Advanced and Bounding Box modes.
- To voxelize onto a grid with a **specific origin, spacing, or dimensions**, use Advanced mode to specify them directly, or create the desired **Image Geometry** with the **Create Geometry** filter and then use Existing Partition Grid mode.

### Errors

| Code | Phase | Condition |
|------|-------|-----------|
| -3003 | Preflight | Advanced mode: Cell Length X value is not greater than zero. |
| -3004 | Preflight | Advanced mode: Cell Length Y value is not greater than zero. |
| -3005 | Preflight | Advanced mode: Cell Length Z value is not greater than zero. |
| -3006 | Preflight | Bounding Box mode: Minimum Grid Coordinate X is greater than Maximum Grid Coordinate X. |
| -3007 | Preflight | Bounding Box mode: Minimum Grid Coordinate Y is greater than Maximum Grid Coordinate Y. |
| -3008 | Preflight | Bounding Box mode: Minimum Grid Coordinate Z is greater than Maximum Grid Coordinate Z. |
| -3012 | Preflight | Number Of Cells Per Axis X is not greater than zero (zero or negative). Applies to Basic, Advanced, and Bounding Box modes. |
| -3013 | Preflight | Number Of Cells Per Axis Y is not greater than zero (zero or negative). Applies to Basic, Advanced, and Bounding Box modes. |
| -3014 | Preflight | Number Of Cells Per Axis Z is not greater than zero (zero or negative). Applies to Basic, Advanced, and Bounding Box modes. |
| -45980 | Execute | Point cloud bounding box is invalid. The point cloud is empty or contains only non-finite coordinates. Basic mode only. |
| -45982 | Execute | Grid allocation failed (`std::bad_alloc`). The point cloud extent relative to the current spacing produces a grid too large to fit in memory. Basic mode only. |
| -45983 | Preflight | The selected Image Geometry has invalid spacing. All three spacing values must be greater than zero. Existing Partition Grid mode with Image Geometry only. |
| -45984 | Preflight | The selected destination geometry has no Cell Attribute Matrix assigned. Existing Partition Grid mode only. |
| -45985 | Preflight | The selected point cloud geometry has no vertex list assigned. |
| -45986 | Preflight | The selected RectGrid Geometry is missing one or more bounds arrays (X, Y, or Z). Existing Partition Grid mode with RectGrid Geometry only. |
| -45987 | Preflight | The selected RectGrid Geometry bounds arrays are inconsistent with its declared dimensions; expected bounds array length = dims + 1 per axis. Existing Partition Grid mode with RectGrid Geometry only. |
| -45988 | Execute | Destination geometry type is not Image Geometry or RectGrid Geometry. Defensive; unreachable under normal operation because the parameter gates these types. |
| -45989 | Preflight | The point cloud vertex list does not have exactly 3 components per vertex (X, Y, Z). |

A **warning** (no error code) is emitted after execution if any points were skipped. Skipped points include those with non-finite coordinates (NaN, ±Inf) and those that fall outside the destination geometry. The message reports the count of skipped points out of the total.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
