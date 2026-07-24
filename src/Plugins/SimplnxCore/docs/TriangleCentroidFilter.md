# Compute Triangle Centroids

## Group (Subgroup)

Surface Meshing (Misc)

## Description

This filter computes the centroid of each **Triangle** in a **Triangle Geometry** (a surface mesh built from triangles) by averaging the positions of the 3 **vertices** (nodes) that make up the triangle, and stores the result as a per-triangle **Face Data** array.

### When to Use This Filter

A per-triangle centroid gives a single representative point for each triangle. It is commonly used as a position reference when fitting a local neighborhood of triangles (for example, by [Compute Feature Face Curvature](FeatureFaceCurvatureFilter.md)) and when visualizing or sampling per-face quantities at a point rather than across the whole triangle.

Note: this filter produces one centroid **per triangle**. To instead compute one centroid **per Feature** (averaged over all the nodes that bound a feature), use [Compute Feature Centroids from Triangle Geometry](ComputeTriangleGeomCentroidsFilter.md).

### Units

Centroid coordinates are reported in the **geometry length units** of the **Triangle Geometry** vertices (for example, micrometers if the mesh coordinates are stored in micrometers).

### Required Input Sources

- **Triangle Geometry** -- a surface mesh, typically produced by a surface-meshing filter such as [Create Surface Mesh (Surface Nets)](SurfaceNetsFilter.md) or [Create Surface Mesh (QuickMesh)](QuickSurfaceMeshFilter.md).

% Auto generated parameter table will be inserted here

## Example Pipelines

+ Triangle_Face_Data_Demo.d3dpipeline

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
