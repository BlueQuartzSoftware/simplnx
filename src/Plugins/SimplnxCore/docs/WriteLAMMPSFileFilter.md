# Write LAMMPS File

## Group (Subgroup)

IO (Output)

## Description

This **Filter** is used to create an atomistic representation of microstructure from image data. The data file produced by this **filter** can be used for initializing the atomic coordinates in LAMMPS package.

This **filter** should be used in conjunction with another **filter** named "Insert Atoms". Given a microstructure, the "Insert Atoms" filter follows the orientation of different features to insert atoms in them and saves the configuration of atoms in a **Vertex Data Container**. The "Export LAMMPS Filter" uses this **Vertex Data Container** to create an input file for LAMMPS.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
