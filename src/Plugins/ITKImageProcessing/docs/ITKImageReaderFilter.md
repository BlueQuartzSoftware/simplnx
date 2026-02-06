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

### Origin & Spacing Options

The user can optionally override the origin and spacing (length units per pixel) for the imported image. The default values from the input file will be used unless the user explicitly enables the "Set Origin" and/or "Set Spacing" options.

When setting a custom origin, the user can choose whether to place the origin at the corner of the geometry (default) or at the center of the geometry by enabling the "Put Input Origin at the Center of Geometry" option.

The **Origin & Spacing Processing Order** parameter controls when origin and spacing overrides are applied:
- **Preprocessed**: Origin and spacing are applied before any cropping operations
- **Postprocessed**: Origin and spacing are applied after cropping operations

### Data Type Conversion

The user can optionally convert the image data to a different data type by enabling the "Set Image Data Type" option. Supported output data types include:
- uint8
- uint16
- uint32

### Cropping Options

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
