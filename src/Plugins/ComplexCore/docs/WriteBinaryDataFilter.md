# WriteBinaryData


## Group (Subgroup) ##

IO (Output) (Write) (Export) (Binary)

## Description ##

This **Filter** accepts DataArray(s) as input, extracts the data, creates the file(s), and writes it out to a single file in binary

## Parameters ##

| Name | Type | Decision |
|-------|------|----------|
| Endianess | Endianess enum class | Determines underlying ordering of binary, if unsure, use Little (most universal) |
| Output Path | Filesystem::path | Directory to store printed array files |
| File Extension | string | The file extension used for generated files |

## Required Geometry ##

None

## Required Objects ##

| Kind          | Default Name | Type | Component Dimensions | Description |
|---------------|-------|------|------|-----------------------------------------------------|
| **DataArray** | Attribute Arrays to Export | DataArray | any | Specifies **DataArray** to have their values printed |

## Created Objects ##

None

## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: WriteBinaryDataFilter
+ Displayed Name: Export Binary Data

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| endianess | Endianess | Default is little endian | complex.ChoicesParameter |
| file_extension | File Extension | The file extension for the output file | complex.StringParameter |
| output_path | Output Path | The output file path | complex.FileSystemPathParameter |
| selected_data_array_paths | Attribute Arrays to Export | The arrays to be exported to a binary file | complex.MultiArraySelectionParameter |

