# Danielsson Distance Map Image Filter

Compute the (approximate) Euclidean distance from every background voxel to the nearest foreground voxel using Danielsson's 4SED algorithm.

## Group (Subgroup)

ImageProcessing (Morphology)

## Description

Produces a **float32** image in which the value at each voxel is the distance from that voxel to the nearest foreground voxel. Following ITK's Danielsson convention, **nonzero** input voxels are the foreground "features" (distance 0) and **zero** input voxels are solved: each stores the distance to its nearest foreground voxel.

The transform uses Danielsson's four-point sequential Euclidean distance (4SED) vector-propagation algorithm. 4SED is an **approximation** of the exact Euclidean distance transform (it can slightly over-estimate the true distance for sparse, isolated features), matching the legacy ITK filter's behavior exactly. When an exact distance transform is required, prefer the Signed Maurer Distance Map Image Filter.

The input array must be single-component (scalar) and of an integer type. The output is a fixed **float32** distance image regardless of the input element type. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Danielsson Distance Map Image Filter, and matches it exactly.

### Input Is Binary

Provided for parity with the legacy filter. In ITK it only affects the Voronoi/label map (which this filter does not emit), so it has **no effect on the distance output**.

### Squared Distance

Selects whether the output stores the **squared** Euclidean distance (**On**) or the true Euclidean distance (**Off**, default). Squared distances avoid the per-voxel square root and are sufficient when only relative distances are needed.

### Use Image Spacing

Selects whether distances are measured in physical units using the Image Geometry's per-axis voxel spacing (**On**) or in voxel units (**Off**, default).

Danielsson's algorithm is described in Per-Erik Danielsson, "Euclidean Distance Mapping," *Computer Graphics and Image Processing*, 14(3):227-248, 1980.

## Algorithm

The filter propagates three-component integer offset vectors with Danielsson's reflective 4SED scan and converts the final vectors to **float32** distances.

### In-Core Path

For resident input and output arrays, `DanielssonDistanceInCore` builds the complete offset-vector map and feature mask in parallel from the resident input span. It propagates the vectors with a serial reflective sweep. It then converts the final vectors in parallel directly into the resident float32 output span.

### Out-of-Core Path

For a three-dimensional disk-backed image, the filter first asks the shared cache-memory budget for the complete offset-vector map, feature mask, one input transfer plane, one output transfer plane, and reflective axis-visit lists. The fast resident path runs only when the complete request is granted. For a `512 x 512 x 128` uint8 image, the request is about 417.3 MiB.

If the complete request is unavailable, allocation fails, or the image is truly two-dimensional, `DanielssonDistanceSlab` keeps the offset-vector map in a raw, uncompressed fixed-record scratch store when either endpoint is disk-backed (keeping that traffic off the deflate codec), or in the resolved in-core format when neither endpoint is out-of-core, and processes one Z plane at a time. The seed and distance phases run in parallel on resident plane buffers. The reflective propagation remains serial. Two vector planes, one input plane, and one output plane form the fixed working set. Compact vector scratch also uses a compact-transfer staging plane when applicable. Consecutive forward and backward visits reuse the previously written vector plane as the next adjacent neighbor. All temporary-memory requests share the application cache budget and remain subject to its aggregate 25% limit.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
