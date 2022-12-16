# WriteASCIIData


## Group (Subgroup) ##

IO (Output) (Write) (Export) (Text) (CSV) (ASCII)

## Description ##

This **Filter** accepts DataArray(s) as input, extracts the data, creates the file(s), and writes it out according to parameter choices

## Parameters ##

| Name | Type | Decision |
|-------|---------------|------------|-----------------|----------------|----------|
| Output Type | OutputSytle enum class | Whether data is printed to one file or multiple |
| Output Path | Filesystem::path | Directory to store printed array files |
| File Name | string | The file name used for generated file in single file mode |
| File Extension | string | The file extension used for generated files |
| Maximum Tuples Per Line | int32 | Number of tuples printed before newline character is printed |
| Delimiter | Delimiter enum class | ASCII character used to seperate values |

## Required Geometry ##

None

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
**DataArray** | Attribute Arrays to Export | DataArray | any | any | Specifies **DataArray** to have their values printed |

## Created Objects ##

None

## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM.3D Mailing Lists ##

If you need more help with a **Filter**, please consider asking your question on the [DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)