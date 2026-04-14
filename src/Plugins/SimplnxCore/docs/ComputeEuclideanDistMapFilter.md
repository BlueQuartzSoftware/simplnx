# Compute Euclidean Distance Map

## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** calculates the distance of each **Cell** from the nearest **Feature** boundary, **triple line** and/or **quadruple point**.  The following algorithm explains the process:

1. Find the **Feature** that owns each **Cell** and its six face-face neighbors of each **Cell**
2. For all **Cells** that have *at least 2* different neighbors, set their *GBEuclideanDistance* to *0*.  For all **Cells** that have *at least 3* different neighbors, set their *TJEuclideanDistance* to *0*.  For all **Cells** that have *at least 4* different neighbors, set their *QPEuclideanDistance* to *0*
3. For each of the three *EuclideanDistace* maps, iteratively "grow" out from the **Cells** identified to have a distance of *0* by the following sub-steps:

- Determine the **Cells** that neighbor a **Cell** of distance *0* in the current map.
- Assign a distance of *1* to those **Cells** and track the *0* **Cell** neighbor internally as their *nearest neighbor*
- Repeat previous two sub-steps, increasing the distances by *1* each iteration, until no **Cells** remain without a distance assigned.

    *Note:* the distances calculated at this point are "city-block" distances and not "shortest distance" distances. The nearest neighbor information is used internally by the algorithm but is not saved as an output array.

4. If the option *Calculate Manhattan Distance* is *false*, then the "city-block" distances are overwritten with the *Euclidean Distance* from the **Cell** to its internally tracked *nearest neighbor* **Cell** and stored in a *float* array instead of an *integer* array.

### Performance

This filter is optimized for out-of-core (OOC) data storage. The entire Feature IDs array is bulk-read into a local buffer via `copyIntoBuffer()` at the start. Each distance map (grain boundary, triple junction, quadruple point) is also initialized and bulk-read into a local buffer. All boundary identification and iterative distance propagation operates on these local buffers with plain pointer access. Results are written back to the output DataStores via a single `copyFromBuffer()` call per map. This reduces OOC I/O operations from O(total_voxels * passes) to O(1) per map.

The three distance maps (when enabled) are computed in parallel using `ParallelTaskAlgorithm`.

% Auto generated parameter table will be inserted here

## Example Pipelines

- (01) SmallIN100 Morphological Statistics

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
