# ITK Exp Negative Image Filter

Computes the function exp(-K.x) for each input pixel.

## Group (Subgroup)

ITKImageIntensity (ImageIntensity)

## Description

Every output pixel is equal to std::exp(-K.x ). where x is the intensity of the homologous input pixel, and K is a user-provided constant.

## Parameters

| Name | Type | Description |
|------------|------| --------------------------------- |

## Required Geometry

Image Geometry

## Required Objects

| Name |Type | Description |
|-----|------|-------------|
| Input Image Geometry | DataPath | DataPath to the Input Image Geometry |
| Input Image Data Array | DataPath | Path to input image with pixel type matching BasicPixelIDTypeList |

## Created Objects

| Name |Type | Description |
|-----|------|-------------|
| Output Image Data Array | DataPath | Path to output image with pixel type matching BasicPixelIDTypeList |

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.
