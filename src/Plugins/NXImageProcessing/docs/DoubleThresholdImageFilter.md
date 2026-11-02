# Double Threshold Image Filter

Binarize an input image using double thresholding.

## Group (Subgroup)

ImageProcessing (MathematicalMorphology)

## Description

Segments a scalar image using **four** thresholds, `Threshold1 <= Threshold2 <= Threshold3 <= Threshold4`. Two bands are formed from the input:

- a **narrow marker band** `[Threshold2, Threshold3]` that seeds the foreground, and
- a **wide mask band** `[Threshold1, Threshold4]` that bounds where the foreground may grow.

The marker is reconstructed (by geodesic dilation) under the mask until convergence. As a result, any connected component of the wide band that contains at least one narrow-band voxel is set to the **Inside Value**; every other voxel is set to the **Outside Value**. This double-threshold-with-reconstruction behaves like a hysteresis threshold: strong (narrow-band) responses seed the object and the weaker (wide-band) response is kept only where it is connected to a strong response.

The thresholds are compared in the input pixel type (the Float64 threshold values are cast to the input element type before comparison), so the band membership test is `lower <= value <= upper` inclusive on both bounds. The input array must be single-component (scalar); any scalar type is accepted. The output is a fixed **uint8** label image regardless of the input element type. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Double Threshold Image Filter, and matches it exactly (byte-identical output values).

### Threshold1 / Threshold2 / Threshold3 / Threshold4

Set the thresholds. Four thresholds should be specified with `Threshold1 <= Threshold2 <= Threshold3 <= Threshold4`. Threshold2/Threshold3 define the narrow marker band; Threshold1/Threshold4 define the wide mask band.

### Inside Value

Set the "inside" pixel value written where the reconstructed foreground lies.

### Outside Value

Set the "outside" pixel value written everywhere else.

### Fully Connected

Whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is off. For objects that are 1 pixel wide, use Fully Connected on.

## Algorithm

The filter thresholds the input into a narrow marker band and a wider mask band, then reconstructs the marker within the mask by dilation. Resident arrays use the Vincent hybrid reconstruction. Disk-backed reconstruction iterates a forward raster sweep followed by a reverse anti-raster sweep over consecutive Z-plane slabs until stable, skipping any slab whose planes are already proven unchanged since that direction's last visit. Within a plane, the raster/anti-raster sweep is tiled into independent blocks that run in parallel; under face connectivity every voxel still sees exactly the values the un-blocked sweep would produce, and under full connectivity the iterated forward/reverse passes still converge to the identical final result. Each work and mask slab targets 16 MiB, plus one boundary plane. At least one plane is processed, so a single plane wider than the target can make the buffers exceed 16 MiB. Update order and output are unchanged, preserving exact storage-path parity.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
