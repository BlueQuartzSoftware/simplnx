# Write Image (ITK)

## Group (Subgroup)

ITKImageProcessing (ITKImageProcessing)

## Description

This **Filter** will save images based on scalar, vector, RGB, or RGBA values. Supported component counts are 1, 2, 3, 4, 10, 11, and 36. If the input array represents a 3D volume, the **Filter** will output a series of slices along one of the orthogonal axes. The options are to produce XY slices along the Z axis, XZ slices along the Y axis or YZ slices along the X axis.

The available output formats are determined by the installed ITK image I/O backends and the filename extension. TIFF, BMP, and PNG are common 2D choices; not every pixel type is supported by every format.

For a series, the *Output File* stem is followed by an underscore and the slice index. *Index Offset*, *Total Number of Index Digits*, and *Fill Character* control the first index and its formatting. For example, with 117 Z cells, offset *0*, width *3*, and fill character `0`, the XY output files are named `slice_000.tif` through `slice_116.tif`. A single-slice output keeps the exact name supplied in *Output File*.

An example of a **Filter** that produces color data that can be used as input to this **Filter**
is the {ref}`Generate IFP Colors <OrientationAnalysis/ComputeIPFColorsFilter:Description>` **Filter**, which will generate RGB values for each voxel in the volume.

### Plane

The *Plane* parameter controls which orthogonal plane is used when writing a 3D volume as a series of 2D image slices:

- **XY [0]**: Write image slices along the XY plane (normal to Z axis).
- **XZ [1]**: Write image slices along the XZ plane (normal to Y axis).
- **YZ [2]**: Write image slices along the YZ plane (normal to X axis).

% Auto generated parameter table will be inserted here

## Example Pipelines

+ INL Export
+ TxCopper_Exposed
+ TxCopper_Unexposed
+ Edax IPF Colors

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
