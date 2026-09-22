# Morphological Watershed From Markers Image Filter

Applies a marker-controlled (seeded) morphological watershed segmentation to a grayscale image.

## Group (Subgroup)

ImageProcessing (Watersheds / Segmentation)

## Description

Floods a grayscale image from a set of user-supplied **marker** (seed) labels, growing each marked region outward over ascending gray levels until neighboring regions meet. The result is a label image in which every voxel is assigned the label of the marker whose basin claimed it. This is the marker-controlled watershed: unlike the parameter-free morphological watershed, the number and identity of the output regions are determined entirely by the markers, so it does not over-segment.

Two flooding variants are provided via **Mark Watershed Line**:

- **On (Meyer's algorithm, the default):** voxels equidistant between two different marker labels are left as a *watershed line* (label 0), producing a thin ridge of background separating the basins.
- **Off (Beucher's algorithm):** every voxel is assigned to a basin (no separating line); this is also slightly faster.

**Fully Connected** selects the neighborhood used for flooding: face connectivity (6-connected in 3D, 4-connected in 2D) when off, or face+edge+vertex connectivity (26-connected in 3D, 8-connected in 2D) when on.

### Required Inputs

- **Input Cell Data (grayscale):** a single-component scalar image of any numeric type. The flood ascends this image's gray levels.
- **Marker Cell Data:** a single-component **integer** scalar image with the same number of tuples as the grayscale input. Nonzero voxels are markers/seeds; a value of 0 means "unmarked". Voxels sharing the same nonzero label belong to the same seed region, so two disjoint blobs with the same label flood into one basin.

### Created Outputs

- A label image with the **same element type as the marker image**. Basin labels are copied from the markers; watershed-line voxels (when Mark Watershed Line is on) are 0.

### Notes / Limitations

- The output reproduces the legacy ITK `MorphologicalWatershedFromMarkersImageFilter` exactly, including the label *values*.
- Internally the flood runs in `uint32`. A marker label of `0xFFFFFFFF` is valid and is handled as a normal in-bounds marker. Labels stored in a wider integer type are narrowed to 32 bits during the flood.

This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Morphological Watershed From Markers Image Filter.

## Algorithm

Resident input uses an in-memory hierarchical queue. The filter processes the lowest gray level first. It uses FIFO order for voxels at the same level.

An out-of-core endpoint keeps the same processing order with bounded, file-backed state. The engine gets an adaptive working-memory grant from the shared cache budget. It uses only the useful part of that grant and keeps a disk-backed fallback. If a complete state fits, the engine can keep it resident. Otherwise, it keeps fixed record pages in memory. Large partial 3-D state uses 32 x 32 x 8 voxel tiles so face neighbors usually share one page.

Eight- and sixteen-bit gray types use ordered gray-level buckets. Each bucket stores 64-bit voxel indexes in file-backed FIFO blocks. Wider integers and floating-point types use a file-backed minimum heap keyed by flood level and insertion order. Marker values are streamed during initialization and are not kept in each voxel record. Input and output transfers use bounded bulk I/O.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
