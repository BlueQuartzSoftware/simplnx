# ITK H Maxima Image Filter (ITKHMaximaImage)

Suppress local maxima whose height above the baseline is less than h.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

HMaximaImageFilter suppresses local maxima that are less than h intensity units above the (local) background. This has the effect of smoothing over the "high" parts of the noise in the image without smoothing over large changes in intensity (region boundaries). See the HMinimaImageFilter to suppress the local minima whose depth is less than h intensity units below the (local) background.

If the output of HMaximaImageFilter is subtracted from the original image, the significant "peaks" in the image can be identified. This is what the HConvexImageFilter provides.

This filter uses the ReconstructionByDilationImageFilter . It provides its own input as the "mask" input to the geodesic dilation. The "marker" image for the geodesic dilation is the input image minus the height parameter h.

Geodesic morphology and the H-Maxima algorithm is described in Chapter 6 of Pierre Soille's book "Morphological Image Analysis:
Principles and Applications", Second Edition, Springer, 2003.

The height parameter is set using SetHeight.

\see ReconstructionByDilationImageFilter , HMinimaImageFilter , HConvexImageFilter 


\see MorphologyImageFilter , GrayscaleDilateImageFilter , GrayscaleFunctionDilateImageFilter , BinaryDilateImageFilter

## Parameters

| Name | Type | Description |
|------|------|-------------|
| Height | float64 | Set/Get the height that a local maximum must be above the local background (local contrast) in order to survive the processing. Local maxima below this value are replaced with an estimate of the local background. |

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

## Python Filter Arguments

+ module: ITKImageProcessing
+ Class Name: ITKHMaximaImage
+ Displayed Name: ITK H Maxima Image Filter

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| height | Height | Set/Get the height that a local maximum must be above the local background (local contrast) in order to survive the processing. Local maxima below this value are replaced with an estimate of the local background. | complex.Float64Parameter |
| input_image_data_path | Input Image Data Array | The image data that will be processed by this filter. | complex.ArraySelectionParameter |
| output_image_data_path | Output Image Data Array | The result of the processing will be stored in this Data Array. | complex.DataObjectNameParameter |
| selected_image_geom_path | Image Geometry | Select the Image Geometry Group from the DataStructure. | complex.GeometrySelectionParameter |

