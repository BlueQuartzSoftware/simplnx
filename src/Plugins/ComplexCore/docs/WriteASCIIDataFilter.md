# Write ASCII Data


## Group (Subgroup) ##

IO (Output) (Write) (Export) (Text) (CSV) (ASCII)

## Description ##

This **Filter** accepts DataArray(s) as input, extracts the data, creates the file(s), and writes it out according to parameter choices

## Parameters ##

| Name | Type | Decision |
|-------|-----|----------|
| Output Type | OutputSytle enum class | Whether data is printed to one file or multiple |
| Output Path | Filesystem::path | Directory to store printed array files |
| File Extension | string | The file extension used for generated files |
| Maximum Tuples Per Line | int32 | Number of tuples printed before newline character is printed |
| Delimiter | Delimiter enum class | ASCII character used to seperate values |

## Required Geometry ##

None

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description                                          |
|-----|-------|------|--------|------------------------------------------------------|
| **DataArray** | Attribute Arrays to Export | DataArray |  any | Specifies **DataArray** to have their values printed |

## Created Objects ##

None

## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: WriteASCIIDataFilter
+ Displayed Name: Export ASCII Data

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| delimiter | Delimiter | The delimiter separating the data | complex.ChoicesParameter |
| file_extension | File Extension | The file extension for the output file(s) | complex.StringParameter |
| includes | Header and Index Options | Default Include is Headers only | complex.ChoicesParameter |
| max_val_per_line | Maximum Elements Per Line | Number of tuples to print on each line | complex.Int32Parameter |
| output_dir | Output Directory | The output file path | complex.FileSystemPathParameter |
| output_path | Output Path | The output file path | complex.FileSystemPathParameter |
| output_style | Output Type | Whether to output a folder of files or a single file with all the data in column form | complex.ChoicesParameter |
| selected_data_array_paths | Attribute Arrays to Export | Output arrays to be written as ASCII representations | complex.MultiArraySelectionParameter |

