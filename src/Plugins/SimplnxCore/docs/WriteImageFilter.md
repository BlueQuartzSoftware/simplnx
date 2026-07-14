# Write Image

## Group (Subgroup)

IO (Output)

## Description

Writes a 2D image or a series of 2D slices from a 3D Image Geometry. This filter does not depend on ITK. It uses stb_image_write for PNG/JPEG/BMP files and libtiff for TIFF files.

The following output file formats are supported:

- PNG (via stb)
- JPEG / JPG (via stb)
- BMP (via stb)
- TIFF / TIF (via libtiff)

### Pixel Data Type Support By Format

When *Create Color Table* is **disabled**, the filter writes the **Data Array**'s own pixel values directly, and the chosen output file format constrains which data types can be written. This is validated during preflight, so an unsupported combination is reported as an error before the filter runs rather than failing partway through writing:

| Output Format             | Backend | Supported Data Types      |
|----------------------------|---------|----------------------------|
| PNG / JPG / JPEG / BMP     | stb     | uint8                       |
| TIFF / TIF                  | libtiff | uint8, uint16, float32      |

For example, writing a float32 array directly to a `.png` file fails preflight, since PNG (via stb) only supports uint8 pixel data. The same float32 array can be written directly to a `.tif` file, or, if a PNG is required, *Create Color Table* can be enabled to normalize the array into a uint8 RGB image before writing (see below). Since color-table output is always uint8 RGB, it is compatible with every supported output format.

If the input array represents a 3D volume, the filter will output a series of slices along one of the orthogonal axes. The options are to produce XY slices along the Z axis, XZ slices along the Y axis, or YZ slices along the X axis. The output files will be numbered sequentially starting at the *Index Offset* and ending at *Index Offset + (dim - 1)* for the chosen axis. For example, if the Z axis has 117 dimensions and *Index Offset* is 0, 117 XY image files will be produced and numbered 000 through 116.

Writes are performed through an `AtomicFile`, so partially-written slices from an aborted run will not corrupt existing output files.

### Plane

The *Plane* parameter controls which orthogonal plane is used when writing a 3D volume as a series of 2D image slices:

- **XY [0]**: Write image slices along the XY plane (normal to Z axis).
- **XZ [1]**: Write image slices along the XZ plane (normal to Y axis).
- **YZ [2]**: Write image slices along the YZ plane (normal to X axis).

### Index Formatting

The *Total Number of Index Digits* and *Fill Character* parameters control the numeric suffix applied to each slice filename. For example, 3 total digits with a fill character of `0` produces `slice_000.tif`, `slice_001.tif`, etc.

### Inline Color Table (Optional)

Enabling *Create Color Table* converts a single-component **Data Array** to an RGB image using a selected color preset immediately before writing, so there is no need to run the Create Color Map **Filter** as a separate step beforehand. The results are identical to running Create Color Map followed by Write Image.

- *Select Preset...* chooses the color preset (for example grayscale, rainbow, or jet) that is applied to the array's values.
- When *Create Color Table* is enabled, the **Input Image Data Array** must be a single-component numeric array, but it may be **any** numeric type: int8, uint8, int16, uint16, int32, uint32, int64, uint64, float32, or float64. This is wider than the type restrictions that apply when writing an array's raw pixel values directly (see *Pixel Data Type Support By Format* above), since color-table mode always produces a uint8 RGB image regardless of the input array's type. If the selected array has more than one component, the filter will fail during preflight.
- Colorization is performed one slice at a time as each slice is streamed to disk; a full RGB copy of the volume is never held in memory. Only the minimum and maximum values across the entire input array (mask ignored) are computed up front and used to normalize each voxel's value against the preset's control points.
- If every voxel in the input array has the same value (a constant array), every voxel normalizes to the same value and is colored using the preset's first control color.

*Use Mask Array* is a separate, top-level toggle (it is not nested under *Create Color Table*) that assigns a fixed color to "bad" voxels instead of coloring them from the preset:

- *Mask Array* selects a boolean or uint8, single-component array that marks each **Voxel** as good (*true* / non-zero) or bad (*false* / zero).
- *Masked Color (RGB)* is the RGB triplet written for any voxel marked bad by the mask.
- The mask is only applied when *Create Color Table* is also enabled. If *Create Color Table* is disabled, *Use Mask Array* and *Masked Color (RGB)* have no effect, since the filter writes the raw pixel data unchanged.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
