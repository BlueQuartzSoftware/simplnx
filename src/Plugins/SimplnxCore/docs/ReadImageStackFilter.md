# Read Images [3D Stack]

## Group (Subgroup)

IO (Input)

## Description

Reads a numbered sequence of 2D image files and stacks them into a 3D volume. This filter does not depend on ITK; it uses stb_image for PNG/JPEG/BMP files and libtiff for TIFF files. The per-slice read is delegated to the [Read Image](ReadImageFilter.md) filter, so any option available there (data type conversion, origin/spacing overrides, 2D cropping) is also available per-slice here.

Supported image types:

- PNG (via stb)
- JPEG / JPG (via stb)
- BMP (via stb)
- TIFF / TIF (via libtiff)

### File List and Slice Ordering

The stack is defined by a file list rather than a single file. The user supplies an input directory plus a numeric naming pattern (a file prefix, suffix, file extension, and the start/end index with a padding digit count). The filter expands that pattern into an ordered list of files, where each file becomes one Z-slice of the resulting 3D volume. The first file in the list becomes Z = 0, the next file Z = 1, and so on, so the numeric ordering of the file names directly determines the slice order along the Z axis. The total number of files becomes the Z dimension of the created **Image Geometry**.

### Processing Order

Image operations are applied in the following order:

1. Read image
2. Crop image (X / Y in 2D; Z applied across the stack)
3. Resample image
4. Convert to grayscale
5. Flip image in X or Y

### Origin & Spacing Caveats

The filter will create a new **Image Geometry**. The user can optionally override the origin and spacing for the created geometry. *Spacing* is the physical size of each voxel and *Origin* is the coordinate of the lower-left-back corner of the volume; both are expressed in the same physical length units (for example microns). The default values from the input files will be used unless the user explicitly enables the "Set Origin" and/or "Set Spacing" options.

### Origin & Spacing Processing

The *Origin & Spacing Processing* parameter provides the following choices:

- **Preprocessed [0]**: Origin and spacing overrides are applied before the image cropping step.
- **Postprocessed [1]**: Origin and spacing overrides are applied after the image flipping step.

When **Preprocessed** is selected, the processing order becomes:

1. Read image
2. Set origin and spacing values
3. Crop image
4. Resample image
5. Convert to grayscale
6. Flip image in X or Y

When **Postprocessed** is selected, the processing order becomes:

1. Read image
2. Crop image
3. Resample image
4. Convert to grayscale
5. Flip image in X or Y
6. Set origin and spacing values

### Resampling Caveats

The user can optionally resample each image as it is read in. The *Resample Images* parameter provides the following choices:

- **Do Not Resample (0)**: Images are used at their native pixel dimensions.
- **Scaling (1)**: A percentage value scales the X and Y pixel counts. A value of 100 means no scaling, 50 means half the pixels, and 200 means double the pixels. The value must be greater than or equal to 1.0.
- **Exact X/Y Dimensions (2)**: The X and Y dimensions of the resampled output are set to the user-supplied values. The Z dimension is left equal to the number of images in the stack.

### Grayscale Conversion

When *Convert To GrayScale* is enabled, RGB image data is converted to a scalar grayscale array using the luminosity algorithm. The luminosity algorithm computes each gray value as a weighted sum of the red, green, and blue channels (gray = wR·R + wG·G + wB·B), where the weights come from the supplied *Color Weighting* values (dimensionless). This produces a perceptually weighted brightness rather than a simple channel average. Only uint8 input data is supported for grayscale conversion.

### Output Data Type

The *Output Data Type* parameter provides the following choices:

- **uint8 [0]**: Convert image data to 8-bit unsigned integer.
- **uint16 [1]**: Convert image data to 16-bit unsigned integer.
- **uint32 [2]**: Convert image data to 32-bit unsigned integer.

### Cropping Caveats

The user can crop the incoming image geometry using the Cropping Options section. The cropping type options are:

- **No Cropping**: Read the full volume into an image geometry
- **Voxel Subvolume**: Read a subvolume into an image geometry using voxel coordinates
- **Physical Subvolume**: Read a subvolume into an image geometry using physical coordinates

Both subvolume cropping types have checkboxes to turn on/off cropping in each of the X, Y, and Z dimensions.

### Image Operations

The user can select to flip the images about the X or Y axis during import.

## Required Input Sources

None — this filter reads from a file list of image files on disk.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
