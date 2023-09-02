# Generate Color Table

## Description ##

This **Filter** generates a color table array for a given 1-component input array.  Each element of the input array
is normalized and converted to a color based on where the value falls in the spectrum of the selected color preset.

## Parameters ##

| Name            | Type | Description                                                       |
|-----------------|------|-------------------------------------------------------------------|
| Selected Preset | JSON | The selected color preset, used to generate the color table array |

## Required Geometry ###

None

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description                                                    |
|------|--------------|-----|----------------------|----------------------------------------------------------------|
| **Attribute Array** | N/A | Any | (1) | 1-component input array used to generate the color table array |

## Created Objects ##

| Kind | Default Name | Type  | Component Dimensions | Description                                             |
|------|--------------|-------|----------------------|---------------------------------------------------------|
| **Attribute Array** | N/A | uint8 | (3) | RGB output color table array |

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: GenerateColorTableFilter
+ Displayed Name: Generate Color Table

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| output_rgb_array_name | Output RGB Array | The rgb array created by normalizing each element of the input array and converting to a color based on the selected preset color scheme | complex.DataObjectNameParameter |
| selected_data_array_path | Data Array | The complete path to the data array from which to create the rgb array by applying the selected preset color scheme | complex.ArraySelectionParameter |
| selected_preset | Select Preset... | Select a preset color scheme to apply to the created array | complex.GenerateColorTableParameter |

