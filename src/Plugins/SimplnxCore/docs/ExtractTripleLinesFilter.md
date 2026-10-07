# Extract Triple Lines

## Group (Subgroup)

Surface Meshing (Generation)

## Description

This **Filter** finds the *triple lines* in a surface mesh and saves them as a new **Edge Geometry**.

### What is a Triple Line?

A **Feature** is one region of the microstructure, for example one grain. Each Feature has a number, its
**Feature Id**. A surface mesh separates the Features with triangles. The **Face Labels** array stores,
for each triangle, the two Feature Ids on either side of that triangle.

A **triple line** is a line where three or more Features meet. The **Filter** checks each edge of the
mesh. It collects the Feature Ids of all triangles that use that edge. If the edge has three or more
different Feature Ids, the edge is a triple line segment.

- Three different Feature Ids: the edge is part of a *triple line*.
- Four or more different Feature Ids: the edge is part of a *quadruple point line*.

An edge where only two Features meet is part of a normal grain boundary. It is not a triple line, even
when many triangles use that edge.

<!-- FIGURE PENDING: ExtractTripleLines_Definition.png
     The 2 x 2 x 1 four-grain block (one grain per cell). Show the surface mesh transparent, colored by
     Feature, with the single quadruple point line through the center drawn as a thick dark line.
![Fig. 1: Four grains meet at the center of the block. The thick line is the quadruple point line that this Filter extracts.](Images/ExtractTripleLines_Definition.png)
-->

### Where to Use This Filter in a Pipeline

The **Filter** uses the **Triangle Geometry** that you give it. The triple lines are always exactly on
that mesh. If you smooth the mesh first, the triple lines follow the smoothed surface.

A typical pipeline is:

1. Create the surface mesh with [Create Surface Mesh (QuickMesh)](QuickSurfaceMeshFilter.md),
   [Create Surface Mesh (Surface Nets)](SurfaceNetsFilter.md), or
   [Create Surface Mesh (M3C Multi-Material Marching Cubes)](M3CSurfaceMeshingFilter.md).
2. Smooth the mesh with [Laplacian Smoothing](LaplacianSmoothingFilter.md).
3. Extract the triple lines with this **Filter**.

If you run this **Filter** before the smoothing step, you get the triple lines of the mesh before
smoothing.

<!-- FIGURE PENDING: ExtractTripleLines_BeforeAfterSmoothing_1.png / _2.png
     Small IN100, the same view twice. Left: triple lines extracted from the unsmoothed mesh.
     Right: triple lines extracted after Laplacian Smoothing. Show the triple lines over a
     semi-transparent surface so the reader can see that the lines follow each surface.
| Before smoothing | After smoothing |
|------------------|-----------------|
| ![](Images/ExtractTripleLines_BeforeAfterSmoothing_1.png) | ![](Images/ExtractTripleLines_BeforeAfterSmoothing_2.png) |
-->

### Parameter Guidance

#### Include Exterior Triple Lines

Any negative **Face Label** means the space outside the volume. All negative values count as
one outside region. All three surface meshing filters listed above write `-1` for this region.

- **Off (default):** the **Filter** ignores all negative labels. You get only the triple lines inside the volume.
- **On:** the **Filter** treats the outside as one more region. A grain boundary that touches the
  outside surface of the volume then becomes a triple line. You get many more segments.

**Feature Id 0** is treated as a normal Feature. Only negative values mean "outside".
Feature 0 counts toward the three regions needed for a triple line, even when this option is off.
This also applies if your data uses Feature 0 for background or bad data.

<!-- FIGURE PENDING: ExtractTripleLines_ExteriorOff.png / ExtractTripleLines_ExteriorOn.png
     Small IN100, the same view twice. Left: option off (interior lines only). Right: option on
     (the extra lines on the outside surface of the volume are visible).
| Include Exterior Triple Lines: Off | Include Exterior Triple Lines: On |
|------------------------------------|-----------------------------------|
| ![](Images/ExtractTripleLines_ExteriorOff.png) | ![](Images/ExtractTripleLines_ExteriorOn.png) |
-->

#### Copy Node Types

When this option is on (the default), the **Filter** copies the **Node Types** value of each vertex from
the surface mesh onto the new vertices. Some filters need a Node Types array, for example
[Laplacian Smoothing](LaplacianSmoothingFilter.md), which can also smooth an **Edge Geometry**. Turn the
option off when your **Triangle Geometry** has no Node Types array. Node Types is never used to decide
which edges are triple lines.

### Required Input Sources

- **Triangle Geometry** and **Face Labels** -- produced by
  [Create Surface Mesh (QuickMesh)](QuickSurfaceMeshFilter.md),
  [Create Surface Mesh (Surface Nets)](SurfaceNetsFilter.md), or
  [Create Surface Mesh (M3C Multi-Material Marching Cubes)](M3CSurfaceMeshingFilter.md).
  **Face Labels** must have one tuple for each triangle.
- **Node Types** (only when *Copy Node Types* is on) -- produced by the same surface meshing filters.
  It must have one tuple for each vertex of the **Triangle Geometry**.

### Created Outputs

The new **Edge Geometry** contains:

- A vertex list with only the vertices that are on a triple line. The coordinates are copied from the
  surface mesh.
- An edge list with the triple line segments.
- **Number of Features** (one value for each edge): `3` when three Features meet at the edge. `4` when
  four **or more** Features meet. The value is never larger than 4.
- **Node Types** (one value for each vertex, only when *Copy Node Types* is on).

The new **Edge Geometry** has its own copy of the vertices. If you change or delete the
**Triangle Geometry** later, the triple lines do not change. If you need the triple lines to match a
changed mesh, run this **Filter** again.

The order of the output is the same on every computer. The vertices are in the same order as in the
surface mesh. The edges are sorted by their vertex numbers.

<!-- OPTIONAL FIGURE: ExtractTripleLines_NumFeatures.png
     Small IN100 triple lines colored by Number of Features (3 and 4), to show where quadruple
     point lines occur.
-->

## Algorithm

The **Filter** first finds candidate vertices. A candidate is a vertex that touches at least three
regions. It processes the vertices in batches. Each batch contains at most `k_DefaultVertexBatchSize`
vertices by default: 8,388,608 vertices (8 million).

1. For one batch, read all input triangles and their Face Labels in order. Record up to three different
   region labels for each vertex in that batch. Set a candidate bit for each vertex with three labels.
2. Repeat for the next batch. The input triangles and labels are read once per batch.
3. Read the triangles once more. Collect up to four different region labels for each edge whose two
   endpoints are candidates. Keep the edge if it borders at least three regions.
4. Sort the selected edges. Copy only their vertices into the output geometry. Copy Node Types when
   requested. Store the region count for each edge, with counts above four stored as `4`.

Every triangle that uses an edge also uses both endpoints. The regions bordering an edge therefore
also touch both endpoints. The candidate check cannot remove a triple line.

The working memory contains:

- 1 bit per input mesh vertex for the candidate flags.
- About 12 bytes per vertex in one batch. The default maximum is about 96 MiB (101 MB).
  The batch label sets are released before the next batch is allocated.
- One hash-map entry for each candidate edge, roughly 50 bytes per entry. The exact size depends on
  the platform and the hash-map allocation overhead.
- 16 bytes per selected output edge for its source indices and region count, plus vector capacity
  overhead. Vertex compaction reserves a further 16 bytes per output edge for source vertex indices.

These amounts exclude the input and output data arrays. Only the candidate bits scale with all input
vertices. The label sets are limited to one batch. The edge tables scale with candidate or selected
edges, which can still be numerous in a complex mesh. Smaller batches use less memory but read the
input more times.

If no triple lines are found, the **Filter** creates an empty Edge Geometry and returns a warning.
This can mean that no edge borders three or more Features with the selected exterior option. The
Triangle Geometry must also share vertices between triangles. Two triangles share an edge only when
they use the same vertex indices at both ends. A mesh with duplicated vertices, such as an imported
STL mesh, does not share edges. Merge coincident vertices before using this **Filter**.

% Auto generated parameter table will be inserted here

## Example Pipelines

- (10) Small IN100 Triple Lines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the
[DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where
the community of DREAM3D-NX users can help answer your questions.
