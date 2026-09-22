# Write Image

## Group (Subgroup)

ImageProcessing (IO)

## Description

Writes a 2D image or a series of 2D slices from a 3D **Image Geometry**. This **Filter** does not depend on ITK. It uses stb_image_write for PNG/JPEG/BMP files, libtiff for TIFF files, and zlib for compressed MHA files.

The following output file formats are supported:

- PNG (via stb)
- JPEG / JPG (via stb)
- BMP (via stb)
- TIFF / TIF (via libtiff)
- MHA / MetaImage (compressed, attached pixel data)

### Pixel Data Type Support By Format

When *Create Color Table* is **disabled**, the filter writes the **Data Array**'s own pixel values directly, and the chosen output file format constrains which data types can be written. This is validated during preflight, so an unsupported combination is reported as an error before the filter runs rather than failing partway through writing:

| Output Format             | Backend | Supported Data Types      |
|----------------------------|---------|----------------------------|
| PNG / JPG / JPEG / BMP     | stb     | uint8                       |
| TIFF / TIF                  | libtiff | uint8, uint16, float32      |
| MHA                         | zlib    | int8, uint8, int16, uint16, int32, uint32, int64, uint64, float32, float64 |

For example, writing a float32 array directly to a `.png` file fails preflight, since PNG supports only uint8 pixel data. The same array can be written to TIFF or MHA. If PNG is required, enable *Create Color Table* to write a uint8 RGB image.

MHA output supports the component counts accepted by the ITK writer: 1, 2, 3, 4, 10, 11, and 36. MHA files also store the origin and spacing for the selected output plane. Other formats use only the component counts supported by their backend.

If an existing pipeline contains the ITK Write Image filter UUID, the ImageProcessing plugin maps that UUID to this **Filter** when ITKImageProcessing is not available. When ITKImageProcessing is available, the original ITK **Filter** remains registered.

If the input array represents a 3D volume, the filter will output a series of slices along one of the orthogonal axes. The options are to produce XY slices along the Z axis, XZ slices along the Y axis, or YZ slices along the X axis. The output files will be numbered sequentially starting at the *Index Offset* and ending at *Index Offset + (dim - 1)* for the chosen axis. For example, if the Z axis has 117 dimensions and *Index Offset* is 0, 117 XY image files will be produced and numbered 000 through 116. If the volume produces only a single slice, no index is appended and the file is written with exactly the user-specified name.

Writes are performed through an `AtomicFile`, so partially-written slices from an aborted run will not corrupt existing output files.

### Plane

The *Plane* parameter controls which orthogonal plane is used when writing a 3D volume as a series of 2D image slices:

- **XY [0]**: Write image slices along the XY plane (normal to Z axis).
- **XZ [1]**: Write image slices along the XZ plane (normal to Y axis).
- **YZ [2]**: Write image slices along the YZ plane (normal to X axis).

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

Enabling *Create Color Table* converts a single-component **Data Array** to an RGB image using a selected color preset immediately before writing, so there is no need to run the Create Color Map **Filter** as a separate step beforehand. The results are identical to running Create Color Map followed by Write Image.

- *Select Preset...* chooses the color preset (for example grayscale, rainbow, or jet) that is applied to the array's values.
- When *Create Color Table* is enabled, the **Input Image Data Array** must be a single-component numeric array, but it may be **any** numeric type: int8, uint8, int16, uint16, int32, uint32, int64, uint64, float32, or float64. This is wider than the type restrictions that apply when writing an array's raw pixel values directly (see *Pixel Data Type Support By Format* above), since color-table mode always produces a uint8 RGB image regardless of the input array's type. If the selected array has more than one component, the filter will fail during preflight.
- Colorization uses bounded source groups and writes one RGB slice at a time. The filter never holds a full RGB volume in memory. The filter first computes the minimum and maximum input values while ignoring the mask. It uses this range to normalize each voxel against the preset's control points.
- If every voxel in the input array has the same value (a constant array), every voxel normalizes to the same value and is colored using the preset's first control color.

*Use Mask Array* is a separate, top-level toggle (it is not nested under *Create Color Table*) that assigns a fixed color to "bad" voxels instead of coloring them from the preset:

- *Mask Array* selects a boolean or uint8, single-component array that marks each **Voxel** as good (*true* / non-zero) or bad (*false* / zero).
- *Masked Color (RGB)* is the RGB triplet written for any voxel marked bad by the mask.
- The mask is only applied when *Create Color Table* is also enabled. If *Create Color Table* is disabled, *Use Mask Array* and *Masked Color (RGB)* have no effect, since the filter writes the raw pixel data unchanged.

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

- **Image Data** -- a `uint8` (or `uint16`/`float32` for TIFF) **Cell Data** array on an **Image Geometry**, produced by an image reader such as [Read Image](ReadImageFilter.md) / [Read Images [3D Stack]](ReadImageStackFilter.md), or by any upstream image-processing filter.

## Algorithm

The filter selects an algorithm from the storage type of the input **Data Array** and the optional mask. Both algorithms produce the same files and preserve the same cancellation, flip, color-table, scale-bar, and atomic-write behavior.

### In-Core Path

`WriteImageDirect` runs when all applicable arrays use in-memory storage. The algorithm uses the original per-slice extraction path. This path preserves the lowest measured in-memory execution time.

The algorithm reads one selected XY, XZ, or YZ slice. It converts the slice to the required byte layout and writes the slice before it reads the next slice. Color-table output first scans the scalar array in fixed pages to calculate the normalization range. It then colorizes one source slice and its optional mask at a time.

### Out-of-Core Path

`WriteImageScanline` runs when the input array or optional mask uses out-of-core storage. The algorithm uses caller-owned buffers with the rectangular extent API. The storage backend does not create a returned vector for each extent.

XY slices match the default out-of-core Z-slice chunk shape. The algorithm reads one XY slice per extent to prevent unnecessary chunk scatter. For XZ and YZ output, the algorithm groups adjacent output slices into a source slab. The slab size uses an active working-memory reservation with a preferred limit of 64 MiB. If one slice exceeds this limit, the algorithm processes one slice at a time.

The algorithm keeps the grouped source slab and one output slice in memory. A color-table mask shares the same slab plan. The algorithm performs store I/O on one thread and checks cancellation between output slices.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
