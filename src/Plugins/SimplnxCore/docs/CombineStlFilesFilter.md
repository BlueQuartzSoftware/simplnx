# Combine STL Files

## Group (Subgroup)

Reader/Input (AMProcessMonitoring)

## Description

This **Filter** combines all of the STL files from a given directory into a single triangle geometry. This filter will make use of the **Import STL File Filter** to read in each stl file in the given directory and then will proceed to combine each of the imported files into a single triangle geometry.

There is an option to label the triangles and vertices with the "index" of the file that was read. This would be based on the lexographical index and starts from 1. This allows for the immediate "segmentation" of the resulting triangle geometry or just as a convenience to "color by" in the visualization widget.

% Auto generated parameter table will be inserted here

## Example Pipelines

CombineStlFiles

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
