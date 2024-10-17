# Read GrainMapper3D File

## Group (Subgroup)

Readers

## Description

This filter will read Version 4 and Version 5 GrainMapper3D HDF5 files. 

- Euler data is read as radians
- The Image Geometry that is produced is in units of millimeters
- The user has the opportunity to create compatible Orientation Data and Phase data. See below.

## Parameter Discussion

GrainMapper3D orientation convention is the same as used by [MTEX](https://mtex-toolbox.github.io), and the inverse of that adapted by DREAM.3D.
This requires certain modifications to the orientation related data (Rodrigues and Quaternions) when being read from the
file. These modifications ensure that when DREAM3D computes orientation related data, the correct results
will be output.

Specifically, the Rodrigues vector will be converted into a 4 component and the conjugate computed. The quaternion
order will be changed from wxyz to xyzw and the conjugate will be computed.

PhaseId data will be converted to "int32" (as an option) to make that data immediately compatible
with DREAM3D's filters.

## Special Notes

The IPF colors (if any) that are read in from the file are *NOT* compatible with the IPF 
Color legends provided by DREAM3D-NX or EBSDLib. The user can use the "Compute IPF Colors"
if they need to specifically understand the crystallographic orientations or they
can obtain the IPF legends from XNovo.

% Auto generated parameter table will be inserted here

## References

[https://xnovotech.com/3d-crystallographic-imaging-software/](https://xnovotech.com/3d-crystallographic-imaging-software/)

## Example Pipelines


## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
