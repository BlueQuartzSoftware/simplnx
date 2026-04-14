# Compute GBCD

## Group (Subgroup)

Statistics (Crystallography)

## Description

This **Filter** computes the 5D grain boundary character distribution (GBCD) for a **Triangle Geometry**, which is the relative area of grain boundary for a given misorientation and normal. The GBCD can be visualized by using either the **Write GBCD Pole Figure (GMT)** or the **Write GBCD Pole Figure (VTK)** **Filters**.

## Algorithm

The filter computes the 5D grain boundary character distribution by iterating over all triangle faces in chunks. For each triangle, the Euler angles of the two adjacent features are used with all pairs of crystal symmetry operators to compute the symmetric misorientation and the crystal normal direction. These are mapped to a 5D GBCD bin index. The triangle's area is accumulated into the corresponding bin. After all triangles are processed, the histogram is normalized by total face area per phase to produce a distribution in multiples of the random distribution (MRD).

### In-Core Path

Feature-level arrays (Euler angles, phases) and face-level arrays (labels, normals, areas) are accessed through the AbstractDataStore API. Triangle processing is parallelized using `ParallelDataAlgorithm` within each chunk.

### Out-of-Core Path

Feature-level Euler angles, phases, and ensemble-level crystal structures are bulk-read into local caches at startup via `copyIntoBuffer`. Triangle-level arrays (face labels, normals, areas) are read in chunks of 50,000 triangles via `copyIntoBuffer`. The parallel GBCD bin computation for each chunk operates on raw pointer offsets into these local buffers, with zero OOC virtual dispatch.

The full GBCD output array is accumulated in a local `std::vector<float64>` buffer (sized by bin resolution, not cell count) and bulk-written to the DataStore via `copyFromBuffer` after normalization.

### Performance

The dominant cost is the O(triangles * symmetry_ops^2) bin computation. By caching feature data locally and chunk-reading triangle data, the algorithm avoids OOC overhead in the triple-nested symmetry loop. The GBCD output buffer size depends on the bin resolution parameter, not the number of cells, so it remains manageable in memory.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (08) Small IN100 GBCD

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
