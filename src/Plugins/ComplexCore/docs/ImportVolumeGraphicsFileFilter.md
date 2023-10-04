# Import Volume Graphics Files (.vgi, .vol)

## Group (Subgroup)

Import/Export

## Description

This **Filter** will import Volume Graphics data files in the form of .vgi/.vol pairs. Both files must exist and be in the same directory for the filter to work. The .vgi file is read to find out the dimensions, spacing and units of the data. The name of the .vol file is also contained in the .vgi file.

## Parameters

| Name | Type | Description |
|------------|------| --------------------------------- |
| VolumeGraphics .vgi File | std::filesystem::path | Path to the .vgi file on disk |
| Image Geometry | DataPath | Name of the created Image Geometry |
| Cell Attribute Matrix | std::string | Name of the created AttributeMatrix |
| Density | std::string | Name of the created Cell Data |

## Required Geometry #

Not Applicable

## Required Objects

Not Applicable

## Created Objects

| Kind                      | Default Name | Type     | Comp Dims | Description                                 |
|---------------------------|--------------|----------|--------|---------------------------------------------|
| Image Geometry | VolumeGraphics | Image Geometry | N/A |  |
|   Attribute Matrix   | CT Data | Cell Attribute Matrix | N/A |  |
| Element Attribute Array | Density | float | (1) | raw data |

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.
