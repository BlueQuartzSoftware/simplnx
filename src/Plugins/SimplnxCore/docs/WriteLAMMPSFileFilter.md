# Write LAMMPS File

## Group (Subgroup)

IO (Output)

## Description

This filter writes atom coordinates from a **Vertex Geometry** (a point cloud, where each point is one atom) into an input file for **LAMMPS**. LAMMPS (Large-scale Atomic/Molecular Massively Parallel Simulator) is an open-source molecular-dynamics code from Sandia National Laboratories that simulates the motion of atoms and molecules.

The typical workflow first builds an atomistic representation of a microstructure with the *Insert Atoms* filter. Given a microstructure, *Insert Atoms* follows the crystallographic orientation of each **Feature** (grain) to place atoms inside it and stores the resulting atom positions in a **Vertex Geometry**. This **Write LAMMPS File** filter then reads that **Vertex Geometry** and writes a LAMMPS data file that LAMMPS can use to initialize the atom coordinates of a simulation.

Atom coordinates are written in the physical units of the **Vertex Geometry** (the same units as the vertex positions, typically Angstroms or nanometers for atomistic data). The *Atom Feature Labels* array assigns each atom an atom type, so atoms belonging to different grains can be distinguished in the simulation.

### Example Output

A LAMMPS data file begins with header lines giving the atom and type counts and the simulation box bounds, followed by an `Atoms` section listing, for each atom, its ID, type, and x/y/z coordinates:

```text
Number of Atoms

8000 atoms
25 atom types

0.0 100.0 xlo xhi
0.0 100.0 ylo yhi
0.0 100.0 zlo zhi

Atoms

1 1 0.000000 0.000000 0.000000
2 1 1.500000 0.000000 0.000000
3 2 3.000000 0.000000 0.000000
   ..
```

### Required Input Sources

- **Vertex Geometry** -- the atom positions to export, typically produced by the *Insert Atoms* filter.
- **Atom Feature Labels** -- a per-atom integer array (one value per vertex) giving each atom's type, also produced by *Insert Atoms*.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
