# ITK Double Threshold Image Filter

Binarizes an image using a hysteresis-style double threshold that keeps the objects of interest without picking up extraneous ones.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

Choosing a single threshold is often a compromise: a wide range catches the whole object but also lets in noise and extraneous pixels, while a narrow range rejects the noise but also clips the object. **Double threshold** resolves this with two nested ranges — a **narrow** range that reliably marks only true object pixels, and a **wide** range that captures the full extent of the objects (and some extras).

The filter keeps every object that contains at least one pixel passing the narrow range, and grows it out to its full extent within the wide range (a geodesic dilation of the narrow-range result, constrained to the wide-range mask). The result is that only genuine objects survive, but each appears at the full size it would have had under the wide threshold.

### Parameter Guidance

- **Threshold1, Threshold2, Threshold3, Threshold4** — four bounds, in the **input image's intensity units**, that must be ordered `Threshold1 ≤ Threshold2 ≤ Threshold3 ≤ Threshold4`. They define the **wide** range as `[Threshold1, Threshold4]` and the **narrow** range as `[Threshold2, Threshold3]` inside it.
- **Inside Value** — the output value for pixels that pass (a `uint8` label, default *1*).
- **Outside Value** — the output value for all other pixels (a `uint8` label, default *0*).
- **Fully Connected** — controls neighbor connectivity for the grow step (face-only versus face + edge + corner). Turn it on for thin, one-pixel-wide features.

### Required Input Sources

Operates on any scalar image — typically from [Read Image](../SimplnxCore/ReadImageFilter.md), [Read Images [3D Stack]](../SimplnxCore/ReadImageStackFilter.md), or the output of a prior ITK image filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
