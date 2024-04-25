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

The user is required to set the origin and spacing (Length units per pixel) for the imported image. The default values are an origin
of (0,0,0) and a spacing of (1,1,1). Any values stored in the actual input file **will be overridden** by the values from the user interface

% Auto generated parameter table will be inserted here

## Example Pipelines

+ Image Histogram

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
