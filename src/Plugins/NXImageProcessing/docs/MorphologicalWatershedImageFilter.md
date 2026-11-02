# Morphological Watershed Image Filter

Applies a morphological watershed segmentation to a grayscale image, seeded automatically from the image's regional minima (no marker image required).

## Group (Subgroup)

ImageProcessing (Watersheds / Segmentation)

## Description

Segments a grayscale image into catchment basins by flooding it from its regional minima. Each basin is assigned a unique label; when **Mark Watershed Line** is on, the ridges separating adjacent basins are left as a background (label 0) watershed line. Unlike the marker-controlled watershed, the seeds are found automatically (from the regional minima of the image), so no marker image is required; the number of basins is instead controlled by the **Level** parameter below.

Internally this is the ITK morphological-watershed composite:

1. **Level (h-minima suppression):** when **Level** is nonzero, shallow minima whose depth is less than **Level** are first suppressed with an *h-minima* transform, merging basins that would otherwise be spurious. A **Level** of 0 performs no suppression (every regional minimum seeds a basin, which tends to over-segment).
2. **Regional minima:** the regional minima of the (optionally h-minima-filtered) image are found.
3. **Connected components:** the minima are labeled with connected components to form the flood markers.
4. **Watershed flood:** a marker-controlled hierarchical-queue flood grows the labeled minima outward over ascending gray levels.

**Fully Connected** selects the neighborhood used throughout: face connectivity (6-connected in 3D, 4-connected in 2D) when off, or face+edge+vertex connectivity (26-connected in 3D, 8-connected in 2D) when on. For objects that are 1 pixel wide, use FullyConnectedOn.

### Required Inputs

- **Input Cell Data (grayscale):** a single-component scalar image of any numeric type. The flood ascends this image's gray levels.

### Created Outputs

- A fixed **uint32** label image regardless of the input element type. Basin labels are consecutive; watershed-line voxels (when Mark Watershed Line is on) are 0.

### Notes / Limitations

- The output reproduces the legacy ITK `MorphologicalWatershedImageFilter` exactly, including the label *values*.
- The number of basins is unbounded on a noisy image; increasing **Level** merges shallow basins. To reorder the output labels by object size, pass the result to a Relabel Component Image Filter.

This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Morphological Watershed Image Filter.

## Algorithm

When **Level** is nonzero, the filter first suppresses shallow minima with morphological reconstruction before it labels minima and floods the watershed. Resident reconstruction uses the Vincent hybrid. Disk-backed reconstruction repeats a forward raster sweep and a reverse anti-raster sweep until the result is stable. Its plane blocks and optional resident prefix come from an adaptive working-memory grant. A bounded streaming path remains available when a larger block does not fit.

The regional-minima stage uses a resident stack flood or a disk-backed iterate-to-stability sweep. The disk-backed path bulk-reads bounded slabs or true-2-D row/X tiles and preserves forward-raster/reverse-anti-raster update order. Connected components use their endpoint-selected label scratch.

The final resident watershed uses an in-memory hierarchical queue. An out-of-core endpoint preserves the same lowest-level-first and FIFO-within-level order with bounded, file-backed state. The engine sizes its page caches from the data and the live working-memory grant. A complete state can stay resident when it fits. Large partial 3-D state uses 32 x 32 x 8 voxel tiles so face neighbors usually share one page.

Eight- and sixteen-bit gray types use ordered buckets with file-backed FIFO blocks of 64-bit voxel indexes. Wider integers and floating-point types use a file-backed `(effective level, insertion sequence)` minimum heap. Marker values are streamed during initialization. All input and output transfers use bounded bulk I/O.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
