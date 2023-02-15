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

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
