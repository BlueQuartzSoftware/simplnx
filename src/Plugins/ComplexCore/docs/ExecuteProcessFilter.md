# Execute Process


## Group (Subgroup) ##

Core (Misc)

## Description ##

This filter allows the user to execute any application, program, shell script or any other executable program on the computer system. Any output can be found in the user specified log file.

The Command Line Arguments parameter should contain the absolute path to the program you wish to execute as well as any arguments that program requires. The formatting of this should follow the same general rules that you would follow when executing the command from a terminal. The following examples show how various commands should be formatted in different scenarios;

Simple directory listing (for Mac/Linux users)

> /bin/ls

Simple command with arguments (for Windows users)

> C:/Applications/DREAM3D-6.6.332/nxrunner.exe C:/Applications/DREAM3D/DREAM3D-6.6.332/Data/MyPipeline.json

For a command with spaces in the path 

> "C:/Program Files/DREAM3D-6.6.332/nxrunner.exe" "C:/Program Files/DREAM3D-6.6.332/PrebuiltPipelines/Workshop/EBSD Reconstruction/(01) SmallIN100 Archive.json"

## Parameters ##

| Name             | Type | Description |
|------------------|------|-------------|
| Command Line | String| The complete command to execute. |
| Should Block | Bool | Whether to block the pipeline process while the command executes or not. |
| Timeout | Int32 | The amount of time (in ms) to wait for the command to start/finish when blocking is selected. |
| Output Log File | File Path | The log file where the output from the process will be stored. |


## Required Geometry ##

Not Applicable

## Required Objects ##

None

## Created Objects ##

None


## Example Pipelines ##

ExecuteProcess


## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM.3D Mailing Lists ##

If you need more help with a **Filter**, please consider asking your question on the [DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)



## Python Filter Arguments

+ module: complex
+ Class Name: ExecuteProcessFilter
+ Displayed Name: Execute Process

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| arguments | Command Line Arguments | The complete command to execute. | complex.StringParameter |
| blocking | Should Block | Whether to block the process while the command executes or not | complex.BoolParameter |
| output_log_file | Output Log File | The log file where the output from the process will be stored | complex.FileSystemPathParameter |
| timeout | Timeout (ms) | The amount of time to wait for the command to start/finish when blocking is selected | complex.Int32Parameter |

