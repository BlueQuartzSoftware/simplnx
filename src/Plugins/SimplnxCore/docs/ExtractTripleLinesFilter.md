# Extract Triple Lines

## Group (Subgroup)

Surface Meshing (Generation)

## Description

This **Filter** extracts the *triple lines* of a multi-material surface mesh into a new **Edge Geometry**.

A mesh edge is a triple line segment when the triangles that share it border **three or more distinct
Feature Ids**. Where three grains meet, the segment borders three Feature Ids; where four meet, it
borders four and forms a *quadruple point line*.

Because the lines are extracted from whatever **Triangle Geometry** is supplied, they always lie exactly
on that surface. This is the reason the extraction is a separate **Filter** rather than an option on the
surface meshing filters: placing it *after* a smoothing step means the triple lines follow the smoothed
surface.

A typical pipeline is:

1. **Quick Surface Mesh** (or **SurfaceNets**, or **M3C Surface Meshing**)
2. **Laplacian Smoothing** on the Triangle Geometry
3. **Extract Triple Lines**

Running the extraction before smoothing instead simply yields the lines of the unsmoothed mesh.

### Include Exterior Triple Lines

The outside of the volume is represented in **Face Labels** by the value `-1`. *Include Exterior Triple
Lines* controls whether that counts as a distinct region.

When disabled (the default), only interior triple lines are produced. When enabled, a grain boundary
that reaches the free surface of the volume also registers as a triple line, because the outside counts
as a third region there. Enabling it therefore produces noticeably more segments.

### Created Outputs

The created **Edge Geometry** contains:

| Output | Description |
|--------|-------------|
| Shared Vertex List | Only the vertices lying on a triple line, copied from the source mesh |
| Shared Edge List | The triple line segments |
| Number of Features | Per segment: `3` for a triple line, `4` for a quadruple point line |
| Node Types | Per vertex, copied from the source mesh's **Node Types** |

The vertex list is a compacted copy rather than a reference, so the **Edge Geometry** is self-contained:
smoothing or deleting the source **Triangle Geometry** afterwards cannot corrupt it. The trade-off is
that the triple lines will *not* follow the mesh if it is modified after extraction — re-run this
**Filter** to bring them back into alignment.

**Node Types** is copied through so that the created **Edge Geometry** can be used directly by filters
that require a node type array, such as **Laplacian Smoothing**, which accepts Edge Geometries.

## Notes

The ordering of the created vertices and edges is deterministic for a given build but is not guaranteed
to be identical across platforms. Comparisons against a stored exemplar should sort the edges and
vertices first rather than relying on their order.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the
[DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where
the community of DREAM3D-NX users can help answer your questions.
