# Write Abaqus Crystal Plasticity File

## Group (Subgroup)

IO (Output)

## Description

This filter writes an **Image Geometry** and crystal-plasticity material data to five Abaqus input (`.inp`) files. The finite-element mesh equals the voxel grid: each **Cell** becomes one eight-node, reduced-integration brick element of type `C3D8R`. Adjacent elements share nodes.

Node coordinates use the origin and spacing of the **Image Geometry**. The *Cell Feature Ids* array assigns each element to a grain element set. The *Cell Phases* and *Cell Euler Angles* arrays supply the phase and orientation for each grain.

### Output Files

The filter writes these files:

- `<prefix>.inp` — the master input file. It includes the other four files and contains the material cards.
- `<prefix>_nodes.inp` — the node coordinates.
- `<prefix>_elems.inp` — the `C3D8R` element connectivity.
- `<prefix>_elset.inp` — one element set for each positive grain ID.
- `<prefix>_sects.inp` — the solid-section definitions that assign materials to grain element sets.

The master file includes the files in the legacy order: nodes, elements, sections, and element sets.

### Grain Data

The three selected arrays are **Cell** arrays. The filter reads the cells in linear index order. If multiple cells have the same positive feature ID, the last cell supplies that grain's phase and Euler angles. Cells with feature ID *0* do not supply grain data and do not occur in an element set.

The filter converts the three Euler angles from radians to degrees. It prepends five values to the user-supplied material constants: grain ID, phase ID, Euler 1, Euler 2, and Euler 3. The filter writes a maximum of eight constants on each line.

The material card has this format:

```text
*Material, name = GrainId#_PhaseID#_mat
*Depvar
<Number of Solution Dependent State Variables>
*User Material, constants = 5 + <Number of Material Constants>
grainID, phaseID, Euler1, Euler2, Euler3, materialConstant1, materialConstant2, materialConstant3
...
*User Output Variables
<Number of User Output Variables>
```

The DREAM.3D 6.6 writer used a `_set` suffix for the material-card name but used a `_mat` suffix in the section reference. This filter writes the `_mat` suffix in both locations so that the material reference is valid.

### Required Input Sources

- **Image Geometry** — the voxel grid to export.
- **Cell Feature Ids** — an `int32` scalar array that assigns each cell to a grain.
- **Cell Euler Angles** — a `float32` three-component array that gives each cell orientation in radians.
- **Cell Phases** — an `int32` scalar array that assigns each cell to a phase.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
