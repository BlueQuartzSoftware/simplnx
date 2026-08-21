# Color to GrayScale

## Group (Subgroup)

Processing (Image)

## Description

This **Filter** converts one or more color image arrays (3-component RGB or 4-component RGBA, uint8) into single-component grayscale image arrays using one of four standard flattening algorithms. Use this filter as a preprocessing step before any image-processing filter that expects single-channel intensity data.

### Conversion Algorithm

The *Conversion Algorithm* parameter selects how the three (or four) color channels are combined into a single value:

- **Luminosity [0]**: weighted average accounting for human perception (more sensitive to green). Default weights are the BT.709 standard: R=0.2125, G=0.7154, B=0.0721. The user may supply custom weights.
- **Average [1]**: simple arithmetic mean of the three channels: (R + G + B) / 3.
- **Lightness [2]**: average of the maximum and minimum channel values: (max(R, G, B) + min(R, G, B)) / 2.
- **SingleChannel [3]**: the user picks one channel (R, G, or B) and that channel's value is used directly.

### Additional Standard Weights

Other widely-used weighting schemes (not built-in, but easy to supply as custom Luminosity weights):

- **RMY Greyscale**: R=0.500, G=0.419, B=0.081
- **YIQ / NTSC**: R=0.299, G=0.587, B=0.114

### Input/Output Format

- **Input arrays** must be uint8 with 3 components (RGB) or 4 components (RGBA). With RGBA input, the alpha channel is ignored.
- **Output arrays** are single-component uint8.

Multiple input arrays can be processed in one filter pass; each produces its own output array. If *Output to New Attribute Matrix* is enabled, the new arrays are placed in a separate Attribute Matrix to keep them organized; otherwise they live alongside the originals.

### Required Input Sources

- **Input Image Arrays** -- 3- or 4-component uint8 arrays, typically produced by [ITK Import Image Stack](../ITKImageProcessing/ITKImportImageStackFilter.md) or another color-image reader.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
