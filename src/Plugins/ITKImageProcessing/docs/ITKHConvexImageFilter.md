# ITK H Convex Image Filter

Extracts bright peaks (local maxima) that rise more than a height *h* above their surroundings.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

The **H-Convex** transform isolates the prominent peaks in a grayscale image. It keeps only those local maxima whose **prominence — their height above the local background — is greater than *h***, and reports each peak's prominence (up to *h*) while flattening everything else toward zero. In other words, it extracts objects that are brighter than their surroundings by at least *h* intensity units.

Use this filter to detect or enhance bright blobs, spots, or particles on an uneven background. Because it measures prominence rather than absolute brightness, it finds peaks consistently even when the background level drifts across the image.

### Parameter Guidance

- **Height** — the height *h*, in the **input image's intensity units**. It sets the minimum peak prominence: only maxima that rise more than *h* above their local background are extracted; shallower bumps are suppressed. Larger values keep only the most prominent peaks. Default *2.0*.
- **Fully Connected** — controls pixel connectivity. When off (default), neighbors share a face; when on, neighbors also include edge- and corner-touching pixels. Turn it on for thin, one-pixel-wide features.

### Required Input Sources

Operates on any scalar image — typically from [Read Image](../SimplnxCore/ReadImageFilter.md), [Read Images [3D Stack]](../SimplnxCore/ReadImageStackFilter.md), or the output of a prior ITK image filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
