# Create Surface Mesh (QuickMesh)

## Group (Subgroup)

Surface Meshing (Generation)

## ⚠ Deprecation Notice

**This filter is deprecated.** Use the [Surface Nets](SurfaceNetsFilter.md) filter instead. Surface Nets produces a smoother mesh (no stair-stepping), runs faster, and includes smoothing in a single pass. This filter is retained for compatibility with legacy pipelines.

## Description

This **Filter** generates a **Triangle Geometry** surface mesh from a grid **Geometry** (Image or Rectilinear Grid). For every cell face shared between two different **Features** (i.e., a Feature boundary), the filter emits two triangles, producing the *stair-stepped* surface that defines the boundary of each Feature.

The resulting mesh is "blocky" because each triangle aligns with a voxel face. To smooth it, apply [Laplacian Smoothing](LaplacianSmoothingFilter.md) afterward -- or, preferably, use [Surface Nets](SurfaceNetsFilter.md), which combines meshing and smoothing in one step.

![Example Quick Mesh Output](Images/QuickSurface_Output.png)

*Quick Surface Mesh output without smoothing.*

![Example Quick Mesh Output](Images/QuickSurface_Smooth_Output.png)

*Quick Surface Mesh output with Laplacian Smoothing applied.*

### Cell Data Transfer

The user may select any number of **Cell Attribute Arrays** to transfer onto the new **Triangle Geometry**. Each output **Face** inherits the values of the **Cell** it came from. Only scalar and vector arrays can be transferred; multi-dimensional component shapes (e.g., N×M matrices) are not supported.

### Face Labels Convention

Each triangle gets a 2-component *Face Labels* attribute storing the two Feature IDs on either side of the boundary. The filter guarantees the smaller Feature ID is in component 0, so downstream filters can rely on a consistent ordering.

If a triangle borders the outer volume rather than a real Feature (i.e., the cell on one side is outside the geometry), one of its Face Labels is set to **-1**.

### Node Types

A *Node Type* vertex array is produced classifying each vertex by its mesh role:

The value is the number of distinct **Features** meeting at the node, capped at 4, plus 10 when the
node lies on the outer surface of the bounding box. The region outside the volume counts as one of
those owners, which is why an ordinary vertex on the wall between the exterior and a single Feature
is `12` — two owners, one of them the exterior — rather than `11`.

| Value | Description |
|-------|-------------|
| 2 | Vertex on the interior of a grain face |
| 3 | Vertex on a triple line (3 grains meet) |
| 4 | Vertex on a quadruple point (4 grains meet) |
| 12 | Vertex on the exterior of the mesh, on a grain face |
| 13 | Vertex on the exterior of the mesh, on a triple line |
| 14 | Vertex on the exterior of the mesh, on a quadruple point |

When the **Bounding Box Skin** option below prunes wall faces, the Node Type of every surviving
vertex is unchanged from the value it had in the full (unpruned) mesh; this is asserted by a unit
test. Create Surface Mesh (M3C) offers no such guarantee.

### Exterior or Boundary Triangles

*Node Type 2 -- interior face vertex.*

![NodeType = 3](Images/QuickMesh_NodeType_3.png)

*Node Type 3 -- triple line vertex.*

![NodeType = 4](Images/QuickMesh_NodeType_4.png)

*Node Type 4 -- quadruple point vertex.*

### Winding

The filter attempts to repair mesh windings so that triangle normals point outward consistently. This may not always succeed -- see [Verify Triangle Winding](VerifyTriangleWindingFilter.md) for cases where the mesh storage scheme prevents fully consistent windings.

### Required Input Sources

- **Cell Feature Ids** -- produced by a segmentation filter such as [Segment Features (Misorientation)](../OrientationAnalysis/EBSDSegmentFeaturesFilter.md) or [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md).
- **Image Geometry** -- typically produced by [Create Image Geometry](CreateImageGeometryFilter.md), [ITK Import Image Stack](../ITKImageProcessing/ITKImportImageStackFilter.md), or an EBSD reader.

## Comparison of Surface Meshing Filters

DREAM3D-NX provides three **Filters** that convert a segmented grid into a multi-material triangle surface mesh. All three produce the same output data model (a **Triangle Geometry** with **Face Labels** and **Node Types**), so they are interchangeable inputs to downstream mesh **Filters**; they differ in how the triangles are generated and therefore in mesh smoothness, triangle count, and performance.

| Aspect | Create Surface Mesh (QuickMesh) | Create Surface Mesh (Surface Nets) | Create Surface Mesh (M3C) |
|---|---|---|---|
| Algorithm | Voxel-face ("staircase") | Dual (SurfaceNets) | Primal multi-material marching cubes |
| Vertex placement | Voxel corners | One relaxed vertex per boundary **Cell** | On **Cell** edges/faces |
| Surface quality | Blocky / stair-stepped | Smooth, sharp edges preserved | Faceted (marching-cubes) |
| Built-in smoothing | No (apply Laplacian Smoothing afterward) | Yes, optional and accuracy-controlled | No (apply Laplacian Smoothing afterward) |
| Relative triangle count | Highest | Lowest | Moderate to high (configuration dependent) |
| Multi-material junctions | Yes | Native | Yes (via case table) |
| Performance | Fastest | Fast (parallelized) | Moderate (multithreaded) |
| Status | Deprecated | Recommended default | Specialized / legacy-compatible |

**Guidance:** Surface Nets is the recommended default for most workflows — it yields the smoothest mesh with the fewest triangles and preserves sharp boundaries. Use M3C when a primal, marching-cubes case-table topology is required for a specific downstream modeling or simulation workflow. QuickMesh is retained for backward compatibility.

### Omitting the Bounding Box Skin

By default this filter generates triangles covering all six outer walls of the Image
Geometry's bounding box. These faces are artifacts of where the volume was cropped rather
than real interfaces, and they receive a Face Label of `-1` on the exterior side.

Enabling the **Bounding Box Skin** option's **Background-Backed Walls Only** mode suppresses a
wall face when the voxel behind it is background (Feature Id 0) — that is, when its Face Labels would be `{-1, 0}`. Wall faces
that cap a *real* Feature are still generated, because that cut plane is the only possible
closure for a Feature flush with the box. A cylinder sitting flush with the box floor
therefore comes out as a closed surface with no surrounding box.

On a fully-indexed volume with no Feature Id 0 voxels, nothing is dropped and the option
has no effect — every boundary Feature already needs its wall cap to stay closed. Rather than
silently doing nothing, the **Filter** reports this with a warning (code `-56342`): the option was
enabled but removed zero faces because the input has no background voxels for it to act on.

Because the test is per-face rather than per-vertex, no triangles are lost along the rim
where an internal boundary meets the box wall. With the option **off**, wherever an internal
Feature-Feature boundary meets the bounding box wall, three faces share that edge: the internal
boundary quad and the two wall quads on either side of it — a non-manifold T-junction that is
inherent to including the full box skin. With the option **on**, the background-backed wall quad
on that edge is dropped, leaving exactly two faces sharing it, which is manifold. The option
therefore does not merely remove unwanted geometry: in the configurations exercised by this
**Filter**'s cross-mesher conformance test (a cylinder **Feature** flush with the box wall, and a
**Feature** occupying a box corner), enabling it produces a watertight mesh where leaving it off
does not. This is not a universal guarantee of watertightness for arbitrary input.

If every voxel in the volume is background (Feature Id 0), every face is a background-backed
wall face and the option removes all of them. The **Filter** reports a warning (code `-56340`)
and creates the **Triangle Geometry** with zero vertices and zero faces; this is treated as
success, not an error, because the input is legal — it simply contains no internal interface
and no Feature to cap.

### Feature Id Validation

Independently of the **Bounding Box Skin** setting, this **Filter** always rejects a **Feature
Ids** array that contains a negative value, because negative values collide with an internal
ghost/exterior sentinel convention. A **Feature Id** must therefore be `>= 0` (this **Filter**,
unlike Create Surface Mesh (Surface Nets) and Create Surface Mesh (M3C), does not additionally
reject `INT32_MAX`). A rejected value produces an error (code `-56343`) naming the offending value,
its tuple index, and the array's **Data Path**. This is a mitigation for the underlying
sentinel-collision design (tracked as issue #1705), not a fix for it.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (01) SmallIN100 Quick Mesh

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
