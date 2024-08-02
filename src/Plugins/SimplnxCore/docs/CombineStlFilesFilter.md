# Combine STL Files

## Group (Subgroup)

Reader/Input

## Description

This **Filter** combines all of the STL files from a given directory into a single triangle geometry. This filter will make use of the **Import STL File Filter** to read in each stl file in the given directory and then will proceed to combine each of the imported files into a single triangle geometry.

There is an option to label the faces and vertices with a "Part Number" that represents the index into the list of files that was used as the input. This would be based on the lexographical index and starts from 1. This allows for the immediate "segmentation" of the resulting triangle geometry or just as a convenience to "color by" in the visualization widget. This can
also be used in the "Write STL Files from Triangle Geometry" Filter if the selection for
the "File Grouping Type" is set to "Part Index" in the UI. Then use the "Part Number" array
that is created in this filter for the "Part Index". 

% Auto generated parameter table will be inserted here

## Example Pipelines

CombineStlFiles

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
