# Execute Process

## Group (Subgroup)

Core (Misc)

## Description

This filter allows the user to execute any application, program, shell script or any other executable program on the computer system. Any output can be found in the user specified log file.

The Command Line Arguments parameter should contain the absolute path to the program you wish to execute as well as any arguments that program requires. The formatting of this should follow the same general rules that you would follow when executing the command from a terminal. The following examples show how various commands should be formatted in different scenarios;

Simple directory listing (for Mac/Linux users)

> /bin/ls

Simple command with arguments (for Windows users)

> C:/Applications/DREAM3D-6.6.332/nxrunner.exe C:/Applications/DREAM3D/DREAM3D-6.6.332/Data/MyPipeline.json

For a command with spaces in the path

> "C:/Program Files/DREAM3D-6.6.332/nxrunner.exe" "C:/Program Files/DREAM3D-6.6.332/PrebuiltPipelines/Workshop/EBSD Reconstruction/(01) SmallIN100 Archive.json"

% Auto generated parameter table will be inserted here

## Example Pipelines

ExecuteProcess

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
