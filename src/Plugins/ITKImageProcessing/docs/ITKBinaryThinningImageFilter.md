# ITK Binary Thinning Image Filter

Reduces the foreground objects of a binary image to a one-pixel-wide skeleton.

## Group (Subgroup)

ITKBinaryMathematicalMorphology (BinaryMathematicalMorphology)

## Description

This filter computes the **skeleton** (medial axis) of the foreground objects in a binary image: it repeatedly removes boundary pixels until each object is reduced to a connected, one-pixel-wide line that still captures the object's overall shape and connectivity. Skeletons are useful for analyzing the topology of elongated structures — measuring branch lengths, counting junctions, or tracing networks of fibers, cracks, or vessels.

The input is treated as a binary image; if the foreground pixels are not already valued 1 they are rescaled to 1 internally. In the output, background pixels are 0 and the skeleton pixels are 1.

**Caveat:** this is a sequential 2D thinning algorithm (the classic Gonzalez & Woods method). On a 3D volume it operates slice by slice rather than producing a true 3D skeleton, and its run time grows with image size.

### Required Input Sources

Operates on a binary image — typically the output of a thresholding filter such as [ITK Binary Threshold Image Filter](ITKBinaryThresholdImageFilter.md) or [Multi-Threshold Objects](../SimplnxCore/MultiThresholdObjectsFilter.md).

## Reference

Rafael C. Gonzalez and Richard E. Woods. *Digital Image Processing*. Addison Wesley, 491-494 (1993).

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
