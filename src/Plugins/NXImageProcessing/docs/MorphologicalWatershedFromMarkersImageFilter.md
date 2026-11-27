# Morphological Watershed From Markers Image Filter

Applies a marker-controlled (seeded) morphological watershed segmentation to a grayscale image.

## Group (Subgroup)

NXImageProcessing (Watersheds / Segmentation)

## Description

Floods a grayscale image from integer seed labels called **markers**. Seed regions grow as the gray level rises.
The output labels identify the regions, or **basins**, formed when they meet.

- **Mark Watershed Line** is on by default. An initially unmarked pixel can remain zero where competing basins meet.
  If neighboring pixels already have different nonzero seed IDs after marker conversion, their basins touch directly.
  With this choice off, the filter assigns reached pixels to basins without a separating line.
- **Fully Connected** is off by default. The filter uses face neighbors (4 in 2D, 6 in 3D).
  Enable it to include diagonal neighbors (8 total in 2D, 26 in 3D).

### Input Guidance

Select a single-component grayscale image and a single-component integer marker image with the same tuple count. Marker
ID 0 means unmarked. Pixels that use the same nonzero ID are one seed region, even if they are in separate locations.
If no nonzero seed remains, the output stays unmarked. An in-bounds UInt32 marker of 4,294,967,295 is a valid seed.

The output has the marker array's element type and the grayscale image's tuple dimensions.

### Marker ID Conversion Handling

The filter converts marker IDs to `uint32` for flooding, then restores the marker type. All 8-, 16-, and 32-bit
integer values keep their original IDs, including signed negatives. For `int64` and `uint64`, the values that keep
their IDs range from 0 through 4,294,967,295, inclusive.

- **Error** (default) stops before flooding if an ID changes during conversion. The error identifies the marker array,
  flat index, original ID, and ID after conversion back to the marker type.
- **Warn and Continue (Legacy Behavior)** uses the old conversion and continues flooding. It reports how many marker
  values changed and identifies the first change.

| Original marker in Warn mode | ID after conversion | Effect |
| --- | --- | --- |
| `uint64` 4,294,967,296 | 0 | The seed disappears. |
| `uint64` 4,294,967,297 | 1 | The seed can merge with seed 1. |
| `int64` −1 | 4,294,967,295 | The seed gets a different label. |

Seed changes can alter basin boundaries, feature counts, and measurements. Saved pipelines without the choice use
**Error**. Select **Warn and Continue (Legacy Behavior)** only when you intend to keep the old conversion and can accept
those effects.

## Algorithm

Flooding processes lower gray levels first and preserves arrival order at the same gray level. The filter checks
marker IDs as it copies them in groups of up to 65,536 values.

The in-memory path holds the flood state in memory. The out-of-core path can also use memory when the state fits.
For larger data it keeps the flood state on disk and reads and writes data in bounded groups. Both paths use the same
flood order.

% Auto generated parameter table will be inserted here

## Example Pipelines

The **Powder Particle Watershed Segmentation** bookmark uses connected-component labels as markers on a gradient image to produce its final **Particle Labels**. [Example pipeline guide](https://github.com/BlueQuartzSoftware/simplnx/blob/develop/src/Plugins/NXImageProcessing/pipelines/08_Powder_Particle_Watershed_Segmentation/08_Powder_Particle_Watershed_Segmentation.md)

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
