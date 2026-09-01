# Compute Triangle Areas

## Group (Subgroup)

Surface Meshing (Misc)

## Description

This filter computes the surface area of every **Triangle** in a **Triangle Geometry** (a surface mesh built from triangles) and stores the result as a per-triangle **Face Data** array.

### What This Measures and Why

Each triangle is defined by three corner **vertices** (nodes). The filter computes the area enclosed by those three vertices using

    Area = 1/2 * |AB| * |AC| * sin(O)

where *AB* and *AC* are two edge vectors of the triangle and *O* is the angle between them.

Per-triangle area is a basic mesh-quality and statistics input. It is used to weight other per-triangle quantities (for example, area-weighting triangle normals or curvature when computing feature-level averages) so that large triangles contribute proportionally more than small ones, and it can be summed to report the total surface area of a **Feature** or boundary.

### Units

The output area is reported in **squared geometry length units** (length^2). The value uses whatever length unit the **Triangle Geometry** vertices are stored in (for example, micrometers^2 if the mesh coordinates are in micrometers).

### Required Input Sources

- **Triangle Geometry** -- a surface mesh, typically produced by a surface-meshing filter such as [Create Surface Mesh (Surface Nets)](SurfaceNetsFilter.md) or [Create Surface Mesh (QuickMesh)](QuickSurfaceMeshFilter.md).

## Algorithm

### What the filter computes

Given a triangle with vertices `A`, `B`, `C` in 3D space, its area equals half the magnitude of the cross product of two of its edge vectors:

    area = 0.5 * |(A - B) × (A - C)|

This is a closed-form, per-triangle computation — there is no iteration, no dependence on other triangles, and no geometry-wide state. The only inputs are the three vertex coordinates for each triangle; the output is one `float64` area per triangle.

### Data access pattern

A **Triangle Geometry** stores two cell-level arrays relevant here:

- **Triangle connectivity**: per triangle, three uint64 vertex indices pointing into the vertex list.
- **Vertex coordinates**: per vertex, three float32s (x, y, z).

For each triangle, the naive implementation issues one triangle-connectivity read plus three random vertex reads — six OOC chunk-cache hits per triangle. At tens of millions of triangles on a CT-scale mesh, that is hundreds of millions of virtual dispatches through the DataStore layer, each with ~50–100 ns of overhead even when the backing chunk is cached. Real-world pipelines spent 20+ seconds inside this filter alone.

### Chunked bulk I/O with span-bounded vertex loads

Filter-generated meshes (QuickSurfaceMesh, SurfaceNets, ExtractInternalSurfaces) create triangles in spatial-locality order, so consecutive triangles tend to reference nearby vertex indices. The filter exploits this with a chunked pipeline:

**For each chunk of 65,536 triangles:**

1. **Bulk-read connectivity** — read all `3 × 65,536` vertex indices for this chunk in one `copyIntoBuffer()` call (~1.5 MB).
2. **Determine the vertex-index span** — scan the connectivity buffer to find `[minVertIdx, maxVertIdx]`. For spatially-coherent meshes, this span is typically in the tens of thousands, not the millions.
3. **Bulk-read the vertex-coordinate range** — if `maxVertIdx − minVertIdx + 1 ≤ 16M vertices` (~192 MB cap for float32 xyz), read that entire range of vertex coords into a local buffer in one `copyIntoBuffer()` call.
4. **Parallel compute** — dispatch the area formula across the chunk's triangles using `ParallelDataAlgorithm`. Threads read from the shared triangle-connectivity and vertex-coordinate RAM buffers (both plain `T[]` / `std::vector<T>`, not `DataStore`) and write to disjoint positions in a local area output buffer. No `DataStore` access inside the parallel region — sidesteps the thread-safety constraint on `AbstractDataStore`.
5. **Bulk-write areas** — flush this chunk's computed areas in one `copyFromBuffer()` call.

Total I/O cost per chunk: 1 triangle read + 1 vertex-range read + 1 area write = **3 bulk calls per 65K triangles**. A typical 10M-triangle mesh takes ~150 chunks, for ~450 HDF5 chunk operations instead of the naive ~60 M.

### Fallback for pathological meshes

If a chunk's vertex span exceeds 16 M vertices (a corner case — it means the mesh's vertex indexing is extremely scattered), the filter falls back to a **serial per-triangle vertex read** path within that chunk. Each triangle issues 3 small `copyIntoBuffer()` calls, each reading a single vertex's 3 floats. This is slower (roughly the original performance), but it runs serially because the `DataStore` isn't thread-safe for concurrent reads. In practice this fallback is never triggered on filter-produced meshes.

### Memory footprint

Peak working memory per filter invocation:

- Triangle connectivity scratch: 1.5 MB (`k_ChunkTriangles × 3 × sizeof(uint64)`)
- Area output scratch: 512 KB (`k_ChunkTriangles × sizeof(float64)`)
- Vertex coordinate scratch: up to ~192 MB (chunk span × 3 × sizeof(float32)), typically a few MB in practice

All bounded, independent of mesh size — the whole mesh is never materialized at once.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (07) Small IN100 Mesh Statistics
+ Triangle_Face_Data_Demo.d3dpipeline

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
