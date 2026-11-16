# Connected Component Image Filter

Label the objects in a binary image.

## Group (Subgroup)

NXImageProcessing (Segmentation)

## Description

Labels connected foreground objects. Nonzero input voxels are foreground. Input voxels with value 0 are background.
The input must have one component and can use any signed or unsigned integer type.

The output uses uint32 labels (4 bytes per voxel). Below the run ID limit, background has label 0 and each object has
a unique label. The filter assigns labels in scan order, starting at 1 and stepping along X first, then Y, then Z.
These labels match the legacy ITK filter below the run ID limit.

### Fully Connected

**Fully Connected** is off by default. The filter joins voxels that share a face. Enable it to also join voxels that
touch at an edge or vertex.

### Provisional Label Limit Handling

The filter assigns one provisional ID to each uninterrupted foreground run along X. Multiple runs can join to form one
object. For example, a bridge can connect several stripes. The limit counts runs.

- **Error** (default) accepts provisional ID 4,294,967,295. The filter stops if another run needs an ID.
- **Warn and Continue (Legacy Behavior)** reports one warning at the first overflow and continues. The first overflowing
  ID becomes background 0. Later IDs reuse existing labels, starting at 1.

In Warn mode, a wrapped 0 in the previous row can cause scanline processing to label an adjacent object as background.
Tiled processing skips that previous-row 0, so the two routes can give different labels. In either route, foreground can
disappear and distinct objects can merge. These changes can affect object counts, sizes, and measurements.

Saved pipelines without this choice use **Error**. Select **Warn and Continue (Legacy Behavior)** only if you need the
old overflow behavior and can accept its effects. A memory or file error can stop either choice.

## Algorithm

The filter finds runs along X and joins neighboring runs with an equivalence table. A final pass assigns consecutive
labels.

For out-of-core 3D data, the filter keeps as much working data in memory as the available budget allows. If it can
keep the complete state in memory, it reads the input once. With less memory it can keep provisional labels in memory.
If the labels also do not fit, it reads the input twice and writes the output once in bounded groups.

For 2D data, wide rows can be processed in tiles. Temporary files can hold the previous row's labels. The final pass
reads input tiles and writes output tiles in bounded groups.

All routes keep equivalence tables in memory. Memory use grows with the number of provisional runs. Three uint32 tables
at the ID limit need at least 48 GiB, before other working data. A memory allocation can fail before the ID limit is reached.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
