# Color to GrayScale

## Group (Subgroup)

Processing (Image)

## Description

This **Filter** allows the user to select a *flattening* method for turning an array of RGB or RGBa values into grayscale values.

+ Luminosity: The **luminosity** method is a more sophisticated version of the average method. It also averages the values, but it forms a weighted average to account for human perception. We   re more sensitive to green than other colors, so green is weighted most heavily. The default formula for luminosity is BT709 Greyscale: Red: 0.2125 Green: 0.7154 Blue: 0.0721. The user can set the weightings to what ever values they would like.
+ Average  (R + G + B) / 3
+ Lightness (max(R, G, B) + min(R, G, B)) / 2
+ Color Channel: The user selects a specific R|G|B channel to use as the GrayScale values.

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
