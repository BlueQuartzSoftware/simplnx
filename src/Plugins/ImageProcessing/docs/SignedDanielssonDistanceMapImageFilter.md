# Signed Danielsson Distance Map Image Filter

Compute the signed (approximate) Euclidean distance to the object boundary using Danielsson's 4SED algorithm.

## Group (Subgroup)

ImageProcessing (Morphology)

## Description

Produces a **float32** image in which the value at each voxel is the signed distance from that voxel to the boundary of the object defined by the input image (the object is the set of **nonzero** voxels). By default the distance is **negative inside** the object and **positive outside**; enabling **Inside Is Positive** flips that sign convention.

This reproduces ITK's composite exactly: it runs the Danielsson distance transform twice -- once on the input and once on the dilated inverted input -- and subtracts the two distance maps. Because it is built on Danielsson's four-point sequential Euclidean distance (4SED) vector propagation, the result is an **approximation** of the exact signed Euclidean distance (matching the legacy ITK filter's behavior exactly). When an exact signed distance transform is required, prefer the Signed Maurer Distance Map Image Filter.

The input array must be single-component (scalar) and of an integer type. The output is a fixed **float32** distance image regardless of the input element type. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Signed Danielsson Distance Map Image Filter, and matches it exactly.

### Inside Is Positive

Selects the sign convention. **Off** (default) writes negative distances inside the object and positive distances outside; **On** reverses that.

### Squared Distance

Selects whether the output stores the **squared** Euclidean distance (**On**) or the true Euclidean distance (**Off**, default).

### Use Image Spacing

Selects whether distances are measured in physical units using the Image Geometry's per-axis voxel spacing (**On**) or in voxel units (**Off**, default).

Danielsson's algorithm is described in Per-Erik Danielsson, "Euclidean Distance Mapping," *Computer Graphics and Image Processing*, 14(3):227-248, 1980.

## Algorithm

The filter inverts the binary object, dilates the inverted image by a radius-one ball, computes Danielsson distance maps for the original and dilated-inverted images, and subtracts the maps with the selected sign convention.

### In-Core Path

For resident arrays, the dilation uses `BinaryMorphDirect` and each distance transform uses `DanielssonDistanceInCore`.

### Out-of-Core Path

When either endpoint is disk-backed, the invert, dilate, and second distance-map scratch arrays remain in an OOC endpoint's format. The radius-one binary dilation and each Danielsson transform may use their faster resident algorithms only after receiving complete shared working-memory reservations.

For the certified `512 x 512 x 128` uint8 image, each Danielsson transform requests about 417.3 MiB. Each complete-state request includes the offset-vector map, feature mask, one input transfer plane, one output transfer plane, and reflective axis-visit lists. The two transforms run one after the other, so their reservations do not overlap. A partial grant or allocation failure uses `DanielssonDistanceSlab`, which keeps its internal offset-vector map in a raw, uncompressed fixed-record scratch store rather than the endpoint's format. Its fixed working set contains two vector planes, one input plane, and one output plane, plus compact-transfer staging when used. True 2-D also keeps the bounded row/tile route. The binary morphology stage independently follows its own complete-grant policy and bounded fallback. All requests share the application cache budget and remain subject to its aggregate 25% limit.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
