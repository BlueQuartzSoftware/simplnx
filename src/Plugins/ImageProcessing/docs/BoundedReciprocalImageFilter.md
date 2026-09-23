# Bounded Reciprocal Image Filter

Computes `1 / (1 + x)` for each pixel.

## Group (Subgroup)

ImageProcessing (Pointwise)

## Description

Computes the bounded reciprocal, `1 / (1 + x)`, for every pixel, where `x` is the intensity of the input pixel. The output image always has a `float64` element type, regardless of the input element type, since the result is bounded to the range `(0, 1]` for non-negative inputs. ITK-free, out-of-core-capable reimplementation of the legacy ITK Bounded Reciprocal Image Filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
