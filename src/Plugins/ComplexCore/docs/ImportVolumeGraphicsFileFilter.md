# Import Volume Graphics Files (.vgi, .vol) #

## Group (Subgroup) ##

Import/Export

## Description ##

This **Filter** will import Volume Graphics data files in the form of .vgi/.vol pairs. Both files must exist and be in the same directory for the filter to work. The .vgi file is read to find out the dimensions, spacing and units of the data. The name of the .vol file is also contained in the .vgi file.

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| VolumeGraphics .vgi File | std::filesystem::path | Path to the .vgi file on disk |
| Image Geometry | DataPath | Name of the created Image Geometry |
| Cell Attribute Matrix | std::string | Name of the created AttributeMatrix |
| Density | std::string | Name of the created Cell Data |

## Required Geometry ###

Not Applicable

## Required Objects ##

Not Applicable

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Image Geometry** | VolumeGraphics | Image Geometry | N/A |  |
| **Attribute Matrix** | CT Data | Cell Attribute Matrix | N/A |  |
| **Element Attribute Array** | Density | float | (1) | raw data |

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the [DREAM3D Users mailing list](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)


## Python Filter Arguments

+ module: complex
+ Class Name: ImportVolumeGraphicsFileFilter
+ Displayed Name: Import Volume Graphics File (.vgi/.vol)

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| cell_attribute_matrix_name | Cell Attribute Matrix | The attribute matrix created as a child of the image geometry | complex.DataObjectNameParameter |
| density_array_name | Density | The data array created as a child of the attribute matrix | complex.DataObjectNameParameter |
| new_image_geometry | Image Geometry | Path to create the Image Geometry | complex.DataGroupCreationParameter |
| vg_header_file | VolumeGraphics .vgi File | The input VolumeGraphics file | complex.FileSystemPathParameter |

