# Approximate Signed Distance Map Image Filter

Compute an approximate signed distance map of a mask (negative inside, positive outside).

## Group (Subgroup)

ImageProcessing (Morphology)

## Description

Produces a **float32** image whose value at each voxel approximates the signed Euclidean distance to the boundary between the **Inside Value** and **Outside Value** regions of the input mask: negative inside the object, positive outside.

It is a two-step composite. First an **Iso Contour Distance** pass lays down an accurate narrow signed band around the boundary (the iso-level is the average of Inside Value and Outside Value). Then a **Fast Chamfer Distance** pass propagates that band outward, using an optimized chamfer metric, to fill the whole image. The result is accurate near the boundary and an approximation (chamfer metric) away from it; when an exact signed distance transform is required, prefer the Signed Maurer Distance Map Image Filter.

The input array must be single-component (scalar) and of an integer type. The output is a fixed **float32** image regardless of the input element type. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Approximate Signed Distance Map Image Filter, and matches it exactly.

### Inside Value

The input intensity value that marks the interior of objects in the mask. Default is 1.

### Outside Value

The input intensity value that marks non-objects (background) in the mask. Default is 0.

## Algorithm

The filter runs two sequential stages: Iso Contour Distance creates the narrow signed band, then Fast Chamfer Distance propagates it through the image. When **Inside Value** is greater than **Outside Value**, Fast Chamfer folds the required sign correction into its final writes.

### In-Core Path

Resident arrays use the parallel IsoContour gather. Fast Chamfer runs in place on the array as a parallel wavefront over full-width row groups, without plane copies. Optional sign inversion occurs after each row group is final in the backward sweep.

### Out-of-Core Path

For the certified `512 x 512 x 128` uint8 image, Iso Contour and Fast Chamfer may request complete resident working states of 160 MiB and 128 MiB respectively. They run one after another, so these reservations do not overlap. Each fast path requires a complete shared grant and performs bulk store transfers.

If a stage receives only a partial grant or allocation fails, the stage uses a bounded algorithm. IsoContour uses rolling planes. Fast Chamfer uses a temporary-record scan that processes 16-plane blocks with the same parallel row-group wavefront inside each block. Sign correction occurs after each backward row group is final. True 2-D uses the bounded row and tile implementations. The filter remains usable when the full dataset is much larger than memory. All requests remain subject to the shared aggregate 25% limit.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
