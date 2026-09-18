# Connected Component Image Filter

Label the objects in a binary image.

## Group (Subgroup)

ImageProcessing (Segmentation)

## Description

Labels the objects in the input image: non-zero input voxels are considered foreground, zero-valued voxels are considered background. Each distinct foreground object is assigned a unique label. Objects that are reached earlier by a raster-order (X fastest, then Y, then Z) scan receive a lower label, so labels start at 1 and are consecutive; background voxels are always labeled 0.

The input array must be single-component (scalar) and may be **any integer type** (signed or unsigned). The output is a fixed **uint32** label image regardless of the input element type. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Connected Component Image Filter, and matches it exactly (byte-identical label values).

### Fully Connected

Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. For objects that are 1 pixel wide, use FullyConnectedOn.

## Algorithm

The filter scans the image in X-fastest order. It encodes each foreground run along X. A union-find table joins runs that have the selected connectivity.

After the first pass, the filter maps each provisional label to a consecutive label. The first output label is one. Each table entry represents a run, not a voxel.

For OOC 3D data, the filter first requests memory for the complete working state. A complete grant uses one input read and one output write.

If this request is not complete, the filter requests memory for the uint32 provisional-label volume. A complete second grant keeps that volume in memory.

If the second grant is not complete, the filter uses bounded replay. Pass 1 reads each input plane once. It encodes independent rows in parallel and keeps only adjacent-plane run data.

After union resolution, pass 2 reads adjacent plane groups. It counts and maps independent rows in parallel. The group size uses at most one-twentieth of the complete replay working set, with a one-plane minimum. The shared cache budget can reduce this size.

The bounded 3D route uses two full input reads and one full output write. Grouping changes the number of transfers, not the logical I/O. The route does not store a complete provisional-label volume.

For true 2D data (`Z=1`), a checked scheduler uses at most 64 MiB for blocks. An extremely wide row uses X tiles.

The tiled first pass can use temporary records for previous-row labels. The final pass rereads bounded input tiles and writes bounded output tiles.

All routes create labels at the same raster run starts. They preserve the same union order and consecutive-label order.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
