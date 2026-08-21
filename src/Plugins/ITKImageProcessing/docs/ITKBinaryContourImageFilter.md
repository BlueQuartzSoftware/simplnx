# ITK Binary Contour Image Filter

Extracts the one-pixel-wide outlines of the foreground objects in a binary image.

## Group (Subgroup)

ITKImageLabel (ImageLabel)

## Description

This filter keeps only the **border** pixels of the foreground objects in a binary image. A foreground pixel (one whose value equals the *Foreground Value*) is retained as a contour pixel if it is adjacent to a background pixel; all interior and background pixels are set to the *Background Value*. The result is the outline of each object.

### Parameter Guidance

- **Foreground Value** — the pixel value that marks objects in the input. Contour pixels keep this value in the output. Default *1*.
- **Background Value** — the value written for non-contour pixels (interior and background). Default *0*.
- **Fully Connected** — controls neighbor connectivity. With it off, only face-sharing neighbors count (thinner contours); with it on, edge- and corner-sharing neighbors also count, producing **thicker contours**.

### Required Input Sources

Operates on a binary/segmented image — typically the output of a thresholding filter such as [ITK Binary Threshold Image Filter](ITKBinaryThresholdImageFilter.md) or [Multi-Threshold Objects](../SimplnxCore/MultiThresholdObjectsFilter.md). For outlines of a multi-region label image, see [ITK Label Contour Image Filter](ITKLabelContourImageFilter.md).

## Reference

Insight Journal: <https://www.insight-journal.org/browse/publication/217>

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
