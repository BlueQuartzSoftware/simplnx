# Compute Feature Centroids from Triangle Geometry

## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** determines the centroids of each **Feature** in a **Triangle Geometry**. The centroids are determined
using the following algorithm:

1. Query each triangle within the **Triangle Geometry** to determine its two owners
2. Add the 3 nodes of that triangle to the set of nodes bounding those two owners (*Note that a set will only allow each
   node to be entered once for a given owner*)
3. For each **Feature**, find the average (x,y,z) coordinate from the set of nodes that bound it

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
