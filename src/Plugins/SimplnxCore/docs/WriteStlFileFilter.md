# Write STL Files from Triangle Geometry

## Group (Subgroup)

IO (Output)

## Description

This **Filter** can generate a single or multiple binary STL (StereoLithography) 
files for a given Triangle Geometry. This is controlled by the "File Grouping Type"
parameter which is a combo box parameter type with the following choices:

- Features: The user must supply a 2 component Int32 array where the values are the 2 features that the triangle belongs to.
- Phases and Features: The user must the same kind of array as in the "Features" but also
 must supply a single component array that represents another higher order of grouping, such
 as a phase (in the case of microstructure data) or a part number in the base of importing
 a large multipart geometry.
- Single File: The entire Triangle Geometry is saved as a single STL File
- Part Index: The user must supply a single component Int32 array at the triangle 
 level that represents which part the triangle belongs to. This is handy when the Triangle Geometry
 has distinct, non-overlapping "parts", such as an AM build with.


% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
