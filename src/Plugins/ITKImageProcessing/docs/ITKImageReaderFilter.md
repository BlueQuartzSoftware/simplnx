# Read Image (ITK)

This filter directly wraps an ITK filter of the same name.

## Group (Subgroup)

ITKImageProcessing (ITKImageProcessing)

## Description

Reads images through the ITK software library [https://www.itk.org](https://www.itk.org)

The following image types are supported:

- PNG
- TIFF
- BMP
- JPG
- NRRD
- MHA

### Origin & Spacing Caveats

The user can optionally override the origin and spacing (length units per pixel) for the imported image. The default values from the input file will be used unless the user explicitly enables the "Set Origin" and/or "Set Spacing" options.

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

The user can optionally convert the image data to a different data type by enabling the "Set Image Data Type" option.

### Cropping Caveats

The user can crop the incoming 2D image using the Cropping Options section. The cropping type options are:
- **No Cropping**: Read the full image into an image geometry
- **Voxel Subvolume**: Crop the image using voxel (pixel) coordinates
- **Physical Subvolume**: Crop the image using physical coordinates

Both subvolume cropping types have checkboxes to turn on/off cropping in the X and Y dimensions. For example, if **Physical Subvolume** is selected and only **Crop Y Dimension** is enabled, the image will be cropped in the Y dimension only using physical coordinate bounds

% Auto generated parameter table will be inserted here

## Example Pipelines

+ Image Histogram

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
