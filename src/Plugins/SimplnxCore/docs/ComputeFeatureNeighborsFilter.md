# Compute Feature Neighbors

## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** determines, for each **Feature**, the number of other **Features** that are in contact with it.  The algorithm for determining the number of "contiguous" neighbors of each **Feature** is as follows:

1. Identify the **Feature** to which a **Cell** belongs
2. Identify the **Features** to which each of the current **Cell**'s six (6) face-face neighboring **Cells** (front, back, left, right, up, down) belong
3. If a neighboring **Cell** belongs to a different **Feature** than the current **Cell**, then that **Feature** (owner of the neighboring **Cell**) is added to the list of contiguous neighbors of the **Feature** that owns the current **Cell**
4. Repeat 1-3 for all **Cells**

While performing the above steps, the number of neighboring **Cells** with a different **Feature** owner than a given **Cell** is stored, which identifies whether a **Cell** lies on the surface/edge/corner of a **Feature** (i.e. the **Feature** boundary). Additionally, the surface area shared between each set of contiguous **Features** is calculated by tracking the number of times two neighboring **Cells** correspond to a contiguous **Feature** pair. The **Filter** also notes which **Features** touch the outer surface of the sample (this is obtained for "free" while performing the above algorithm). The **Filter** gives the user the option whether or not they want to store this additional information.

## Algorithm

This filter has two algorithm implementations that are automatically selected at runtime based on how the input data is stored. The user does not need to choose between them.

### In-Core Algorithm (Direct)

When all input arrays reside in memory, the **Direct** algorithm is used. It employs compile-time dimension specialization (via C++ templates) to handle 0D, 1D, 2D, and 3D image geometries without runtime branching in the inner loops.

Processing is split into two stages:

1. **Boundary cells** (corners, edges, faces): Each voxel's face neighbors are checked with validity guards since boundary voxels do not have all 6 neighbors.
2. **Internal cells** (3D only): All 6 face neighbors are guaranteed to exist, so no validity checks are needed in the innermost loop.

Surface area accumulation uses per-face area values computed from the geometry spacing, correctly handling non-cubic voxels. This fixes a bug present in DREAM3D 6.5 where all faces were assumed to have the same area.

### Out-of-Core Algorithm (Scanline)

When any input array is backed by chunked on-disk storage (out-of-core), the **Scanline** algorithm is used. Out-of-core data lives in compressed chunks on disk; random per-element access would trigger repeated chunk load/decompress/evict cycles ("chunk thrashing"), making the algorithm catastrophically slow.

The Scanline algorithm avoids this by reading data one Z-slice at a time using bulk I/O, maintaining a rolling window of 3 Z-slices in memory:

- **Previous slice** (z-1): Used for -Z neighbor lookups
- **Current slice** (z): The slice being processed
- **Next slice** (z+1): Used for +Z neighbor lookups

Within each slice, +/-X and +/-Y neighbors are resolved by simple index arithmetic on the in-memory buffer. After processing a slice, the buffers rotate (prev gets cur, cur gets next, next loads z+2) and the BoundaryCells output is written back via bulk I/O.

### Performance

The in-core Direct algorithm accesses data through per-element getValue() calls, which are essentially pointer dereferences for in-memory data. The out-of-core Scanline algorithm uses sequential bulk I/O (copyIntoBuffer/copyFromBuffer), reading one Z-slice at a time. Memory usage is bounded to 3 Z-slices of FeatureIds plus 1 Z-slice of BoundaryCells, regardless of the total volume size.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (03) Small IN100 Morphological Statistics
+ (02) Small IN100 Full Reconstruction
+ (02) Single Hexagonal Phase Equiaxed
+ (03) Single Cubic Phase Rolled
+ (05) Composite

+ (01) Single Cubic Phase Equiaxed
+ (04) Two Phase Cubic Hexagonal Particles Equiaxed
+ (06) SmallIN100 Synthetic

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
