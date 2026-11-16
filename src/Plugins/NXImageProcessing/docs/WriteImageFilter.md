# Write Image

## Group (Subgroup)

NXImageProcessing (IO)

## Description

Writes a 2D image or a series of 2D slices from a 3D **Image Geometry**.
The filter supports PNG, JPEG, BMP, TIFF, and MHA. MHA stores compressed pixels in the same file.

### Pixel Data Type Support By Format

With *Create Color Table* off, the file format must support the input array's type and number of components. The filter
checks both during preflight.

| Output format | Supported pixel types |
| --- | --- |
| PNG, JPG, JPEG, BMP | uint8 |
| TIF, TIFF | uint8, uint16, float32 |
| MHA | int8, uint8, int16, uint16, int32, uint32, int64, uint64, float32, float64 |

For example, write raw float32 data to TIFF or MHA. To write single-component float32 data as PNG, enable *Create Color
Table* to produce a uint8 RGB image.

MHA supports 1, 2, 3, 4, 10, 11, or 36 components per pixel. It stores the origin and spacing for the selected plane.
Other formats have their own component limits.

A saved pipeline can refer to the original ITK Write Image filter. The pipeline uses that filter when ITKImageProcessing
is available. Otherwise, NXImageProcessing uses this filter in its place.

Each slice is written to a temporary file before it replaces the destination. An export that stops partway through can
leave earlier slices in the output folder.

### Plane

For a 3D volume, *Plane* selects the direction of the slice series. **XY** steps along Z, **XZ** along Y, and **YZ**
along X.

The first slice uses *Index Offset*. For example, 117 XY slices with an offset of 0 are numbered 000 to 116. A single
slice uses the specified filename.

### Index Formatting

The *Total Number of Index Digits* and *Fill Character* parameters control the numeric suffix applied to each slice filename. For example, 3 total digits with a fill character of `0` produces `slice_000.tif`, `slice_001.tif`, etc. These parameters have no effect when only a single slice is written, since single-slice output does not receive an index suffix.

### Flip Output Image (Optional)

The *Flip Output Image* option optionally mirrors each image immediately before it is written to disk:

- **None**: No flip is applied (default).
- **Flip About X Axis**: Reverses the row order, mirroring the image top-to-bottom.
- **Flip About Y Axis**: Reverses the pixel order within each row, mirroring the image left-to-right.

The flip is applied uniformly to every image the filter writes: it works with both the raw-pixel-write path and the color-table path (see *Inline Color Table* below), and applies to slices generated from any of the three *Plane* selections (XY, XZ, YZ).

This option only affects the written image files. The input Image Geometry and its data arrays in the DataStructure are left unmodified.

### Inline Color Table (Optional)

*Create Color Table* is off by default. When enabled, the filter uses *Select Preset...* to map any single-component
numeric input to uint8 RGB. An input with more than one component causes a preflight error.

The filter uses all finite values for the color range, including finite values excluded by a mask. For a constant finite
range, each unmasked pixel uses the first color in the preset. Different input values can map to the same RGB values.
Extreme-range colors can differ from older exports.

#### Nonfinite Input Handling

This choice applies to color-table output. Raw floating-point TIFF and MHA exports keep the input values.

- **Error** (default) stops on an unmasked NaN or infinity before the filter writes or replaces any slice.
- **Warn and Continue** uses the preset's first color for unmasked NaN or infinity. The warning counts those pixels.
  They can look the same as valid pixels at the minimum of the finite range.

Both choices require at least one finite input value. Masking nonfinite pixels cannot supply a range. An entirely masked
array with at least one finite value can still be written in the mask color.

Saved pipelines without this choice use **Error** for color-table output. To export unmasked NaN or infinity, correct the
data, mask those pixels, or select **Warn and Continue**.

#### Mask Array

Enable *Use Mask Array* with *Create Color Table* to give excluded pixels the *Masked Color (RGB)*. Select a single-component
boolean or uint8 mask with the same tuple count as the image data. A false or zero value excludes a pixel. Masked NaN
and infinity receive the mask color and are excluded from the warning count.

### Add Physical Scale Bar (Optional)

When *Add Physical Scale Bar* is enabled, each written image is extended with a white band below the
image data containing a black scale bar and a length label (for example `100 µm`). The bar is
left-justified in the band with the label on the same line to its right. The image pixels are
never covered — the band is appended, so the written image is taller than the Image Geometry's slice
dimensions by the band height (8% of the image height, minimum 24 pixels). The preflight output reports
the padded size.

- **Bar length**: chosen automatically as the largest "nice" value (1, 2 or 5 times a power of ten)
  that spans no more than 25% of the physical image width, computed from the Image Geometry's spacing
  along the written image's horizontal axis.
- **Units**: taken from the Image Geometry's length unit and rescaled to the most readable SI prefix
  (a spacing expressed in meters that yields a 0.0001 m bar is labeled `100 µm`). Non-metric units
  (inch, foot, ...) are labeled as-is. If the geometry's unit is *Unspecified*, the label shows only
  the number.
- **Output format**: the written image is always 8-bit RGB when the scale bar is enabled. The input
  must be an 8-bit array with 1 (grayscale), 3 (RGB) or 4 (RGBA, alpha is dropped) components, or
  *Create Color Table* must be enabled (which already produces RGB). Other inputs fail preflight.
- **Interaction with *Flip Output Image***: the flip applies to the image data only; the band and its
  bar remain upright at the bottom of the written file.

### Required Input Sources

Select a **Cell Data** array on an **Image Geometry**. You can use an image reader such as [Read Image](ReadImageFilter.md)
or [Read Images [3D Stack]](ReadImageStackFilter.md), or an array from an upstream image-processing filter.

## Algorithm

The filter selects an algorithm from the storage type of the input **Data Array** and the optional mask. Both algorithms use the same color scan, nonfinite policy and normalization. They retain the existing slice extraction, flip, scale-bar and per-slice write paths.

### In-Core Path

`WriteImageDirect` runs when all applicable arrays use in-memory storage. The algorithm uses the original per-slice extraction path. This path preserves the lowest measured in-memory execution time.

The algorithm reads one selected XY, XZ, or YZ slice. It converts the slice to the required byte layout and writes the slice before it reads the next slice. Color-table output first scans scalar and optional mask pages with a combined target of 1 MiB. It releases these pages before it colorizes one source slice and its optional mask at a time.

### Out-of-Core Path

`WriteImageScanline` runs when the input array or optional mask uses out-of-core storage. The algorithm uses caller-owned buffers with the rectangular extent API. The storage backend does not create a returned vector for each extent.

XY slices match the default out-of-core Z-slice chunk shape. The algorithm reads one XY slice per extent to prevent unnecessary chunk scatter. For XZ and YZ output, the algorithm groups adjacent output slices into a source slab. The slab size uses an active working-memory reservation with a preferred limit of 64 MiB. If one slice exceeds this limit, the algorithm processes one slice at a time.

The algorithm keeps the grouped source slab and one output slice in memory. A color-table mask shares the same slab plan. The initial scalar/mask scan uses bounded pages before the slab allocation. The algorithm performs store I/O on one thread and checks cancellation between output slices.

The filter does not allocate a complete RGB volume. It still requires a complete selected 2D RGB slice. A scale bar can require a second padded slice and a separate band. The image codec can allocate more memory for that slice. The source-buffer reservation is not a limit on total process memory.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
