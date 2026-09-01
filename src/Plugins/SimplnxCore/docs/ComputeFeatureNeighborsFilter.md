# Compute Feature Neighbors

## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** determines, for each **Feature**, which other **Features** it is in direct contact with and how much boundary they share. The result is a set of feature-level arrays that many downstream filters depend on -- neighbor misorientation statistics, GBCD, twin merging, boundary strength analysis, and more.

### How This Filter Works

The filter walks every cell in the input **Image Geometry** and compares each cell's *Feature Id* to the *Feature Ids* of its six face-sharing neighbors (front, back, left, right, up, down):

1. Identify the **Feature** the current cell belongs to.
2. For each of the cell's six face-neighbors, check whether the neighbor cell belongs to a different feature.
3. If so, record that neighbor feature in the current feature's *contiguous neighbor* list and increment the count of shared cell-faces between the two features.
4. While iterating, the filter also tracks which cells lie on a feature boundary (any cell with at least one differently-labeled neighbor) and which features touch the outer geometry boundary (any cell whose neighbor is outside the volume).

### What This Filter Produces

The main feature-level outputs are:

- **Neighbor List** -- for each feature, the list of *Feature Ids* of the features it touches.
- **Number of Neighbors** -- for each feature, the integer count of its contiguous neighbors. Equivalent to the length of each *Neighbor List* entry.
- **Shared Surface Area List** -- for each pair of neighboring features, the number of cell-faces they share. This is in **cell-face units** (a dimensionless count), not physical area. To convert to physical area, multiply by the area of one cell face (which depends on the cell spacing and the orientation of the shared face).

Two optional outputs can also be stored:

- **Boundary Cells** (enable *Store Boundary Cells Array*) -- a cell-level array marking which cells sit on any feature boundary. Useful for visualization and for flagging the cells that contribute to grain-boundary statistics.
- **Surface Features** (enable *Store Surface Features Array*) -- a feature-level boolean marking which features touch the outer volume bounds. Downstream statistical filters often exclude surface features from distributions because they are biased by sample truncation (see [Compute Biased Features](ComputeBiasedFeaturesFilter.md)).

### Required Input Sources

- **Cell Feature Ids** -- produced by a segmentation filter such as [Segment Features (Misorientation)](../OrientationAnalysis/EBSDSegmentFeaturesFilter.md) or [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md).

This filter handles Image Geometries of all dimensions (0D/1D/2D/3D). Thus, it is up to the user to ensure spacing is set inline with intended behavior, specifically for Shared Surface Area List calculation. For more details see the Image Geometry section of the Geometry documentation (currently in the python docs).

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
