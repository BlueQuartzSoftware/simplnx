# Color to GrayScale

## Group (Subgroup)

Processing (Image)

## Description

This **Filter** allows the user to select a *flattening* method for turning an array of RGB or RGBa values into grayscale values.

### Conversion Algorithm

The *Conversion Algorithm* parameter provides the following choices:

- **Luminosity [0]**: A weighted average of RGB channels that accounts for human perception (more sensitive to green). Uses the BT709 formula by default: Red: 0.2125, Green: 0.7154, Blue: 0.0721. The user can set custom weightings.
- **Average [1]**: Computes a simple arithmetic average of R, G, and B channel values: (R + G + B) / 3.
- **Lightness [2]**: Averages the maximum and minimum channel values: (max(R, G, B) + min(R, G, B)) / 2.
- **SingleChannel [3]**: The user selects a specific R, G, or B channel to use directly as the grayscale values.

The user can select 1 or more image data arrays which are assumed to be multi-component arrays of unsigned 8 bit values. The user can create a new AttributeMatrix if they want to store all the newly created arrays in a separate AttributeMatrix.

### Additional GrayScale Conversions

The following are some additional accepted grayscale conversions

+ RMY Greyscale: Red: 0.5 Green: 0.419 Blue: 0.081
+ (YIQ/NTSC): Red: 0.299 Green: 0.587 Blue: 0.114

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
