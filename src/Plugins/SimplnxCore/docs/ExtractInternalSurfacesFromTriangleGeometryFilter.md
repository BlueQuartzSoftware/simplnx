# Extract Internal Surfaces From Triangle Geometry

## Group (Subgroup)

Geometry

## Description

This **Filter** extracts any **Triangles** from the supplied **Triangle Geometry** that contain any *internal nodes*, then uses these extracted **Triangles** to create a new **Data Container** with the reduced **Triangle Geometry**.  This operation is the same as removing all **Triangles** that only lie of the outer surface of the supplied **Triangle Geometry**.  The user must supply a "Node Type" **Vertex Attribute Array** that defines the type for each node of the **Triangle Geometry**.  Node types may take the following values:

### Quick Surface Mesh

| Id Value | Node Type |
|----------|-----------|
| 2 | Normal **Vertex |
| 3 | Triple Line |
| 4 | Quadruple Point |
| 12 | Normal **Vertex** on the outer surface |
| 13 | Triple Line on the outer surface |
| 14 | Quadruple Point on the outer surface |

### SurfaceNets Mesh

| Id Value | Node Type |
|----------|-----------|
| 0 | Normal **Vertex |
| 3 | Triple Line |
| 4-8 | Quadruple Point |
| 13 | Normal **Vertex** on the outer surface |
| 14 | Triple Line on the outer surface |
| 15-18 | Quadruple Point on the outer surface |

This **Filter** has the effect of removing any **Triangles** that only contain **Vertices** whose node Id values fall outside of the min and max that the user sets.  In general, this *node type* array is created when the original surface mesh is created.

It is unknown until runtime how the **Geometry** will be changed by removing certain **Vertices** and **Triangles**.

## Algorithm

### What the filter does (conceptually)

A surface mesh generated from a 3D segmented volume contains three kinds of triangles: those on the outer "box" of the volume (exterior faces), those separating two interior features (internal surfaces), and those mixed between the two. Only the internal surfaces carry physically meaningful information about feature-to-feature boundaries. This filter discards the exterior triangles and rebuilds a smaller **Triangle Geometry** containing only triangles whose three vertices all have node-type values inside the user-specified `[minType, maxType]` range.

The output geometry has:
- A **compact** vertex list (only vertices actually referenced by a kept triangle)
- A **compact** triangle list (only triangles with all three vertices inside the node-type range)
- New vertex/triangle indices numbered consecutively starting at 0
- Re-mapped triangle connectivity pointing at the new vertex indices
- All selected cell-level arrays (vertex-attached and triangle-attached) copied through the same index remapping

### Preserving ordering

Downstream filters rely on a subtle invariant: triangle *i*'s three fresh vertices (vertices seen for the first time when triangle *i* is encountered in traversal order) receive new vertex indices consecutively. So if triangles 0, 1, 2 each introduce three brand-new vertices, those vertices get new indices 0, 1, 2, 3, 4, 5, 6, 7, 8 respectively — in that exact order. Some downstream operations (e.g. winding-sensitive mesh cleanup) would misbehave if this ordering were not maintained. The filter therefore keeps a **dense per-vertex map** (`vertNewIndex`) of size `8 B × numVerts` to record each vertex's assigned new index. This is unavoidable: the ordering information cannot be reconstructed from a bitmap alone.

### Bitmap + prefix-sum triangle bookkeeping

Triangles, on the other hand, get their new indices in strict source-order (triangle traversal is monotone), so the filter can use a much more compact representation: a **1-bit-per-triangle "keep" bitmap** (`triMask`) plus a **sparse prefix-sum popcount table** (`triPrefixSum`) that records the number of set bits in the bitmap every 4096 triangles. Looking up triangle *i*'s new compact index reduces to `triPrefixSum[i / 4096]` + a popcount over the 4096 bits preceding *i* within the bitmap. This gives O(1) lookup with constant factors measured in tens of popcount instructions — cheap, and uses ~6.4× less memory than a dense 8-byte-per-triangle map.

### Six-pass streaming pipeline

The algorithm streams the full geometry six times, each pass using bounded chunk buffers (65,536 tuples by default) so peak memory stays O(bitmap) + O(vertNewIndex), independent of dataset size apart from those two data structures. The passes are:

1. **Pass 1a — Vertex node-type scan.** Stream the vertex NodeTypes array in 65K-tuple chunks. For each vertex, if its node type is in `[minType, maxType]`, set the corresponding bit in a vertex-acceptability bitmap (`vertOkMask`).

2. **Pass 1b — Triangle scan and vertex index assignment.** Stream the triangle connectivity (3 vertex indices per triangle) in 65K-triangle chunks. A triangle survives if and only if all three of its vertices pass the node-type test (check the bits in `vertOkMask`). For each surviving triangle:
   - Set the corresponding bit in `triMask`.
   - For each of the triangle's three vertices, if that vertex hasn't been seen before (sentinel value in `vertNewIndex`), assign the next available new vertex index and store it in `vertNewIndex`.
   - This is the pass that captures the "contiguous-per-triangle" ordering invariant.

3. **Pass 2 — Build the triangle prefix-sum popcount table.** Walk `triMask` once and compute the sparse `triPrefixSum` table (one entry per 4096 triangles). Total kept-triangle count is a free byproduct.

4. **Pass 3 — Copy kept vertex XYZ coordinates.** Stream the source vertex list in 65K-tuple chunks. For each vertex with a valid new index, write its XYZ coords into the compact output buffer. Because new vertex indices are NOT monotonically increasing in source order (a later source vertex can have an earlier new index), the writes are random on the destination side — but the reads are bulk and sequential.

5. **Pass 4 — Copy kept triangles with vertex indices remapped.** Stream the source triangle connectivity in 65K-triangle chunks. For each kept triangle, rewrite its three vertex indices via `vertNewIndex[vertId]` and append the remapped triple to the output buffer. Because new triangle indices ARE monotonic, both reads and writes are bulk-chunked.

6. **Pass 5 — Copy per-vertex attached arrays.** For each user-selected vertex attribute array, apply the same vertex-index remapping as pass 3.

7. **Pass 6 — Copy per-triangle attached arrays.** For each user-selected face attribute array, apply the same triangle-index remapping as pass 4. Bulk reads and bulk writes throughout.

### Memory footprint

Peak memory is bounded by:

- `vertOkMask`: 1 bit per vertex (1 MB for 8M vertices).
- `triMask`: 1 bit per triangle (2 MB for 16M triangles).
- `triPrefixSum`: 1 uint64 per 4096 triangles (tiny).
- `vertNewIndex`: 8 B per vertex (128 MB for 16M vertices) — the dense dominator.
- Per-pass chunk buffers: ~1 MB each, released between passes.

On a mesh with hundreds of millions of vertices, `vertNewIndex` dominates. This is the price paid for preserving the contiguous-per-triangle ordering invariant.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.
