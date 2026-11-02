# Signed Maurer Distance Map Image Filter

Compute the signed Euclidean distance transform of an object using Maurer's exact algorithm.

## Group (Subgroup)

ImageProcessing (Morphology)

## Description

Produces a **float32** image in which the value at each voxel is the signed distance from that voxel to the boundary of the object defined by the input image. A voxel is considered **inside** the object when its value is not equal to the **Background Value**; all voxels equal to the Background Value form the background.

By default the distance is **negative inside** the object and **positive outside** it. Enabling **Inside Is Positive** flips that sign convention (positive inside, negative outside). The transform is computed with Maurer's exact O(n) linear-time algorithm, so the result is the true (not an approximate) Euclidean distance.

The input array must be single-component (scalar) and of an integer type. The output is a fixed **float32** distance image regardless of the input element type. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Signed Maurer Distance Map Image Filter, and matches it exactly.

### Inside Is Positive

Selects the sign convention. **Off** (default) writes negative distances inside the object and positive distances outside; **On** reverses that.

### Squared Distance

Selects whether the output stores the **squared** Euclidean distance (**On**, default) or the true Euclidean distance (**Off**). Squared distances avoid the per-voxel square root and are sufficient when only relative distances are needed.

### Use Image Spacing

Selects whether distances are measured in physical units using the Image Geometry's per-axis voxel spacing (**On**) or in voxel units (**Off**, default).

### Background Value

The input value that marks the background. Every voxel whose value is not equal to this is treated as being inside the object.

Maurer's algorithm is described in Calvin R. Maurer Jr., Rensheng Qi, and Vijay Raghavan, "A Linear Time Algorithm for Computing Exact Euclidean Distance Transforms of Binary Images in Arbitrary Dimensions," *IEEE Transactions on Pattern Analysis and Machine Intelligence*, 25(2):265-270, 2003.

## Algorithm

### In-Core Path

A contiguous in-memory `DataStore` input is borrowed. Any other in-memory store is copied once. The filter owns one float32 work volume. Each work value encodes the inside or outside state in its sign. The boundary scan uses a bounds-free interior fast path and checks image borders separately. The filter processes independent one-dimensional lines in parallel and waits between axis passes. When requested, the filter applies the square root to the work volume in place.

### Out-of-Core Path

On the complete-grant route, resident memory holds an input copy and the float32 work volume. A `512 x 512 x 128` uint8 image needs about 160 MiB.

On the bounded route, the X and Y passes stream one Z plane at a time into raw scratch storage. One I/O thread prefetches the next input plane. The Z pass uses two bounded staging buffers. The I/O thread writes the previous Y batch and gathers the next one while workers transform the current batch. Store calls remain serial, and workers access only resident buffers. The Z-pass resident peak is worker line scratch plus two staged XZ bands. All requests share the application cache budget and its 25% limit.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
