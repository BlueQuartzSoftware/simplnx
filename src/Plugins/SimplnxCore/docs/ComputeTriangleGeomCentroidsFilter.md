# Compute Feature Centroids from Triangle Geometry

## Group (Subgroup)

Statistics (Morphological)

## Description

This filter computes the centroid (the average position of the bounding nodes) of each **Feature** in a **Triangle Geometry** (a surface mesh built from triangles).

In a surface mesh, each triangle carries a **Face Labels** value: a pair of **Feature** Ids naming the two features that meet at that triangle. These two features are the triangle's "owners". A **Feature** is the volume of material enclosed by the triangles that reference its Id. The corner points of each triangle are its **nodes** (also called vertices).

The centroid of each feature is determined as follows:

1. For each triangle, read its two owners from the **Face Labels** array.
2. Add the 3 nodes of that triangle to the set of nodes bounding each of those two owners. A set stores each node only once per owner, so shared nodes are not double-counted.
3. For each **Feature**, average the (x, y, z) coordinates of the set of nodes that bound it to obtain the centroid.

### Boundary / Exterior Label

A **Face Labels** value of -1 indicates the exterior of the sample (the triangle faces open space rather than another feature). Feature Id -1 is the exterior and is not a real feature; its accumulated "centroid" entry is therefore not physically meaningful and should be ignored.

### Units

Centroid coordinates are reported in the **geometry length units** of the **Triangle Geometry** vertices (for example, micrometers if the mesh coordinates are stored in micrometers).

### Required Input Sources

- **Triangle Geometry** -- a surface mesh, typically produced by a surface-meshing filter such as [Create Surface Mesh (Surface Nets)](SurfaceNetsFilter.md) or [Create Surface Mesh (QuickMesh)](QuickSurfaceMeshFilter.md).
- **Face Labels** -- the per-triangle pair of **Feature** Ids, produced alongside the mesh by [Create Surface Mesh (QuickMesh)](QuickSurfaceMeshFilter.md) or [Create Surface Mesh (Surface Nets)](SurfaceNetsFilter.md).

## Is Periodic

When **Is Periodic** is enabled, features whose nodes wrap across opposing boundaries of the mesh are handled with a
minimum-image (periodic) centroid rather than the plain average. For any axis on which a feature touches both opposing
faces of the geometry's bounding box, the centroid component on that axis is computed by unwrapping the feature's nodes
across the boundary (using the largest empty gap in their distribution) before averaging, and mapping the result back
into the domain. This depends on where the feature's mass actually lies, so it is correct for asymmetrically-wrapped
features and never places the centroid outside the domain. Axes on which the feature does not wrap keep the ordinary
average, and a feature that fills the whole domain along an axis falls back to the ordinary average on that axis. When
**Is Periodic** is disabled (the default), the plain average from step 3 is reported.

> **Note:** Enabling **Is Periodic** assumes the periodic domain equals the geometry's bounding box. An informational
> message is emitted for each feature that is adjusted, since a manual review may be warranted.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
