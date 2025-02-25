# Combine Node Based Geometries

## Group (Subgroup)

Core

## Description

This **Filter** will combine any node-based geometries together into one node-based geometry.  The algorithm is governed by several rules:

1. All input geometries must have the same geometry type.  For example, inputting all Triangle geometries will output a combined Triangle geometry, all Edge geometries will output a combined Edge geometry, etc.
2. All input geometries must contain vertex and node data that have the exact same names, types, and component dimensions.  For example, if one input geometry has an edges array with edges data arrays, all input geometries must have an edges array with edges data arrays that have the exact same names, types, and component dimensions.  If one input geometry has vertex data arrays, then all input geometries must have vertex data arrays that all have the exact same names, types, and component dimensions.
3. Higher order input geometries can include lower order node data.  For example, an input tetrahedral geometry can include edge data (as long as all input tetrahedral geometries contain edge data with the same names, types, and component dimensions).

*NOTE:* Any additional groups, attribute matrices, or arrays that are not one of the following:

1. Vertex arrays or vertex data
2. Edge arrays or edge data
3. Face arrays or face data
4. Polyhedra arrays or polyhedra data

WILL BE ignored and will not exist in the geometry outputted by this filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
