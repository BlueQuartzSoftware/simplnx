# Read Zeiss TXM & TXRM File

## Group (Subgroup) ##

IO (Read)

## Description ##

This filter will read the entire volume or optionally a cropped subvolume from a .txm or .txrm file. These files are Zeiss xCT reconstruction files.

The cropping type options in the Cropping Options section are `No Cropping` to read the full volume into an image geometry, `Voxel Subvolume` to read a subvolume into an image geometry using voxel coordinates, and `Physical Subvolume` to read a subvolume into an image geometry using physical coordinates.  Both subvolume cropping types have checkboxes to turn on/off cropping in each of the X, Y, and Z dimensions.  So for example, if the cropping type `Physical Subvolume` is selected, `Crop Y Dimension` is turned on, and `Crop X Dimension` and `Crop Z Dimension` are turned off, then the incoming volume will be cropped in the Y dimension only and the cropping bounds will be in physical units.

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
