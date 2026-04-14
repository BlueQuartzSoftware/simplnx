# Create Surface Mesh (QuickMesh)

## Group (Subgroup)

Surface Meshing (Generation)

## Deprecation Notice

The "SurfaceNets" filter should be used instead. Search the filter list for "Surface Nets".

## Description

This **Filter** generates a **Triangle Geometry** from a grid **Geometry** (either an **Image Geometry** or a **RectGrid Geometry**) that represents a surface mesh of the present **Features**. The algorithm proceeds by creating a pair of **Triangles** for each face of the **Cell** where the neighboring **Cells** have a different **Feature** Id value. The meshing operation is extremely quick but can result in a surface mesh that is very "stair stepped". The user is encouraged to use a smoothing operation to reduce this "blockiness".

The user may choose any number of **Cell Attribute Arrays** to transfer to the created **Triangle Geometry**. The **Faces** will gain the values of the **Cells** from which they were created.  Currently, the **Filter** disallows the transferring of data that has a *multi-dimensional* component dimensions vector.  For example, scalar values and vector values are allowed to be transferred, but N x M matrices cannot currently be transferred.

This filter will ensure that the smaller of the 2 **FaceLabel** values will always be in the first component (component[0]). This will allow assumptions made in downstream filters to continue to work correctly.

This filter attempts to repair the windings for a mesh. This may not be possible due to the nature of how meshes are stored in the software. See Verify Traingle Winding documentation for detailed breakdown of nuance.

For more information on surface meshing, visit the tutorial.

---------------

![Example Quick Mesh Output](Images/QuickSurface_Output.png)

Quick Surface Mesh output **without** any extra smoothing applied

---------------

![Example Quick Mesh Output](Images/QuickSurface_Smooth_Output.png)

Quick Surface Mesh output **with** Laplacian Smoothing filter applied.

---------------

![NodeType = 2](Images/QuickMesh_NodeType_2.png)

NodeType = 2

---------------

![NodeType = 3](Images/QuickMesh_NodeType_3.png)

NodeType = 3

---------------

![NodeType = 4](Images/QuickMesh_NodeType_4.png)

NodeType = 4

---------------

### Node Types

One of the arrays to come out of the algorithm is the "Node Type" vertex array. This array uses a value to label each vertex as to what kind of node it was determined to be during the meshing process.

| Value | Description |
|-------|-------------|
| 2 | Node within the interior of the grain face.  |
| 3 | Node along a triple line  |
| 4 | Node that is a Quadruple point  |
| 12 | Node that is on the exterior of the mesh  |
| 13 | Node that is on the exterior of the mesh and is a triple line  |
| 14 | Node that is on the exterior of the mesh and is a quadruple point   |

### Exterior or Boundary Triangles

Each triangle that is created will have an 2 component attribute called `Face Labels` that represent the Feature ID on either
side of the triangle. If one of the triangles represents the border of the virtual box then one of the FaceLables will
have a value of -1.

## Algorithm

This filter uses a dispatch mechanism to select the optimal algorithm implementation based on the storage type of the input arrays.

### In-Core Algorithm (Direct)

When all input arrays are backed by in-memory storage, the **QuickSurfaceMeshDirect** algorithm is used. This is the original implementation that accesses the FeatureIds array via direct element indexing.

The algorithm proceeds in three phases:

1. **Problem Voxel Correction** (optional): Iteratively examines every 2x2x2 block of voxels to detect diagonal-conflict configurations that would produce non-manifold mesh geometry. Conflicting voxels are randomly reassigned to a neighbor's FeatureId using a seeded RNG for reproducibility. Up to 20 correction iterations are performed.

2. **Node and Triangle Counting**: A single pass over all voxels counts the number of unique mesh vertices (nodes) and boundary triangles. For each voxel, the algorithm checks whether the FeatureId differs from the +X, +Y, and +Z neighbors. Volume boundary faces also produce triangles. A mapping array of size (xP+1) x (yP+1) x (zP+1) assigns sequential vertex IDs to active dual-grid corners.

3. **Mesh Generation**: A second pass writes vertex coordinates, triangle connectivity, face labels, and node types. Face labels ensure the smaller FeatureId is always in component[0], with -1 used for exterior boundary faces. Each vertex is classified by how many features share it (2=interior face, 3=triple line, 4=quad point, +10 for boundary vertices).

### Out-of-Core Algorithm (Scanline)

When any input array uses chunked out-of-core (OOC) storage, the **QuickSurfaceMeshScanline** algorithm is selected automatically. This variant produces identical output but avoids random-access reads that would cause chunk thrashing on disk-backed data stores.

Key optimizations:

- **Z-slice bulk I/O**: FeatureIds are read one Z-slice at a time (xP x yP elements) via `copyIntoBuffer()` instead of per-element reads. At most two adjacent Z-slices are buffered simultaneously.

- **Rolling node-plane buffers**: Instead of the O(volume) node mapping array used by the Direct variant, two node-plane buffers of size O((xP+1) x (yP+1)) each are maintained and swapped after each Z-slice. This reduces memory from O(volume) to O(slice).

- **Buffered output writes**: Triangle connectivity and face labels are accumulated in per-slice buffers and flushed via `copyFromBuffer()`. Vertex coordinates are buffered for all nodes and flushed once at the end.

- **Dirty-flag write-back**: During problem voxel correction, modified Z-slices are tracked with dirty flags and only written back if they were actually changed.

### Performance

The in-core (Direct) variant is fastest for datasets that fit in memory. The out-of-core (Scanline) variant avoids the 100-1000x performance penalty that would occur from chunk thrashing on OOC datasets, at the cost of slightly more complex bookkeeping. Both variants produce bit-identical output.

## Notes

The Quick Mesh algorithm is very crude and naive in its implementation. This filter
along with the Laplacian Smoothing filter can give you reasonable results. The
newer filter that should replace both the Quick Mesh and the Laplacian Smoothing
filter is the "Surface Nets" surface meshing algorithm. This will create the surface
mesh and smooth in a single filter and give subjectively better results and perform
much faster at both.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (01) SmallIN100 Quick Mesh

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
