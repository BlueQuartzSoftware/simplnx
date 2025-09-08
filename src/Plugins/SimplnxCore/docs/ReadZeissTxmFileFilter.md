# Read Zeiss TXM & TXRM File

## Group (Subgroup) ##

IO (Read)

## Description ##

This filter will read the entire volume or optionally a subvolume from a .txm or .txrm file. These files
are Zeiss xCT reconstruction files.

The minimum slice always starts at 1.
The last slice is inclusive.

## WARNING

Be aware of how large of data you are reading into DREAM3D-NX as these files can become quite large and will overwhelm the visualization system.
It is recommended that you view the data as a "2D Slice/Image View" or "Volume Render" view.

% Auto generated parameter table will be inserted here

## References

This code uses the MIT licensed OLESS software library.

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
