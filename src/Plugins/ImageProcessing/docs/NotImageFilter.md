# Not Image Filter

Computes the logical NOT of each pixel.

## Group (Subgroup)

ImageProcessing (Pointwise)

## Description

Computes the pixel-wise logical NOT for every pixel: a pixel value of `0` becomes `1`, and any non-zero pixel value becomes `0`. Only integer-typed input arrays are supported. ITK-free, out-of-core-capable reimplementation of the legacy ITK Not Image Filter.

## Algorithm

The filter applies the logical NOT independently to every value. When both input and output are resident in memory, it operates directly on their contiguous storage. When either **Data Array** is out of core, it reads and writes bounded batches through the generic bulk-I/O interface and performs the transformation only on local buffers.

The buffered path targets 64 MiB of combined input and output scratch memory. Each batch is normally rounded to a bounded common alignment of complete trailing-dimensional slabs (complete XY planes for a conventional three-dimensional **Image Geometry**) that is compatible with every out-of-core endpoint. If mismatched input and output layouts would make that common alignment larger than both the target and either individual slab, the filter instead aligns batches to the output slab. This keeps output writes rectangular and memory bounded; the storage backend handles any partial input regions in that pathological mixed-layout case. If the selected alignment unit is larger than the target, one complete unit is used as the minimum working set. Memory therefore scales with the target or one endpoint slab, not with the complete volume.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
