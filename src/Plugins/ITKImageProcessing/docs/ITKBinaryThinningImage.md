# ITK Binary Thinning Image Filter

This filter computes one-pixel-wide edges of the input image.

## Group (Subgroup)

ITKBinaryMathematicalMorphology (BinaryMathematicalMorphology)

## Description

This class is parameterized over the type of the input image and the type of the output image.

The input is assumed to be a binary image. If the foreground pixels of the input image do not have a value of 1, they are rescaled to 1 internally to simplify the computation.

The filter will produce a skeleton of the object. The output background values are 0, and the foreground values are 1.

This filter is a sequential thinning algorithm and known to be computational time dependable on the image size. The algorithm corresponds with the 2D implementation described in:

Rafael C. Gonzales and Richard E. Woods. Digital Image Processing. Addison Wesley, 491-494, (1993).

To do: Make this filter ND.* MorphologyImageFilter

## Parameters

| Name | Type | Description |
|------------|------| --------------------------------- |

## Required Geometry

Image Geometry

## Required Objects

| Name |Type | Description |
|-----|------|-------------|
| Input Image Geometry | DataPath | DataPath to the Input Image Geometry |
| Input Image Data Array | DataPath | Path to input image with pixel type matching IntegerPixelIDTypeList |

## Created Objects

| Name |Type | Description |
|-----|------|-------------|
| Output Image Data Array | DataPath | Path to output image with pixel type matching IntegerPixelIDTypeList |

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.
