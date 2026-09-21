# Read Image

## Group (Subgroup)

IO (Input)

## Description

Reads an image file into a newly created **Image Geometry**. This filter is dimension-aware: a single-page raster file is read as a 2D image (`Z = 1`), a multi-page TIFF is read as a 3D Z-stack (one page per Z slice), and a NRRD file is read as its native 2D or 3D volume. This filter does not depend on ITK. It uses stb_image for PNG/JPEG/BMP files, libtiff for TIFF files, and an ITK-free (zlib-only) reader for NRRD files.

The following image types are supported:

- PNG (via stb)
- JPEG / JPG (via stb)
- BMP (via stb)
- TIFF / TIF, including multi-page TIFF read as a 3D Z-stack (via libtiff)
- NRRD (`.nrrd` attached, `.nhdr` detached) (ITK-free, zlib only)

### NRRD Volumes

For a [NRRD](https://teem.sourceforge.net/nrrd/format.html) input (attached single-file `.nrrd`, or detached `.nhdr` + external data file), the geometry is built directly from the NRRD header. Supported header features:

- `encoding: raw` and `encoding: gzip` (only the data segment is compressed).
- `endian: little` / `big` (multi-byte types are byte-swapped to host order).
- Scalar images and interleaved component images (a leading `vector`/`none` axis, e.g. RGB) — the component axis must be the fastest axis (axis 0).
- Axis-aligned `space directions` / `spacings` (spacing = per-axis vector magnitude) and `space origin`.
- 2D images are loaded with `Z = 1`.

Unsupported NRRD header features produce explicit errors or warnings:

- `encoding` other than raw/gzip (ascii, hex, bzip2) → error.
- `type: block` and unrecognized types → error.
- A component axis that is not the fastest axis → error.
- Non-axis-aligned `space directions` (rotation / permutation) → **warning**; the rotation is dropped (simplnx `ImageGeom` is axis-aligned), spacing/origin are still extracted, and voxels are loaded in their stored order.

The origin/spacing/data-type override and cropping options below apply to all backends; the origin/spacing/data-type overrides are ignored for the NRRD backend (its geometry comes from the header). `.mha`/`.mhd` files are handled by the separate Read Mha File filter (which is transform-aware).

### Origin & Spacing Caveats

The user can optionally override the origin and spacing for the imported image. *Spacing* is the physical size of each pixel and *Origin* is the coordinate of the lower-left corner of the geometry; both are expressed in the same physical length units (for example microns). The default values from the input file will be used unless the user explicitly enables the "Set Origin" and/or "Set Spacing" options.

When setting a custom origin, the user can choose whether to place the origin at the corner of the geometry (default) or at the center of the geometry by enabling the "Put Input Origin at the Center of Geometry" option.

### Origin & Spacing Processing

The *Origin & Spacing Processing* parameter provides the following choices:

- **Preprocessed [0]**: Origin and spacing overrides are applied before any cropping operations.
- **Postprocessed [1]**: Origin and spacing overrides are applied after cropping operations.

### Output Data Type

The *Output Data Type* parameter provides the following choices:

- **uint8 [0]**: Convert image data to 8-bit unsigned integer.
- **uint16 [1]**: Convert image data to 16-bit unsigned integer.
- **uint32 [2]**: Convert image data to 32-bit unsigned integer.

### Data Type Conversion

The user can optionally convert the image data to a different data type by enabling the "Set Image Data Type" option. Conversion is performed by normalizing the source value against the source type's maximum and scaling to the destination type's maximum.

### Cropping Caveats

The user can crop the incoming image using the Cropping Options section. The cropping type options are:

- **No Cropping**: Read the full image into an image geometry
- **Voxel Subvolume**: Crop the image using voxel (pixel) coordinates
- **Physical Subvolume**: Crop the image using physical coordinates

Both subvolume cropping types have checkboxes to turn on/off cropping in the X, Y, and Z dimensions. For example, if **Physical Subvolume** is selected and only **Crop Y Dimension** is enabled, the image will be cropped in the Y dimension only using physical coordinate bounds. Z cropping only applies to a genuine multi-page/volume input (`Z > 1`); for a 2D image the Z crop flag is ignored.

## Required Input Sources

None — this filter reads directly from a single image file on disk.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
