# Write Abaqus Crystal Plasticity File

## Group (Subgroup)

IO (Output)

## Description

This filter writes an Abaqus input deck for a crystal-plasticity analysis that uses a user material, such as a UMAT. The filter converts an **Image Geometry** to a finite-element mesh. Each **Cell** becomes one eight-node, reduced-integration brick element of type `C3D8R`. Adjacent elements share nodes.

The filter writes five Abaqus input (`.inp`) files:

- `<prefix>_nodes.inp` contains the node coordinates.
- `<prefix>_elems.inp` contains the `C3D8R` element connectivity.
- `<prefix>_sects.inp` contains the solid-section definitions that assign materials to grain element sets.
- `<prefix>_elset.inp` contains one element set for each feature ID from *1* through the maximum positive feature ID.
- `<prefix>.inp` is the master file. It contains the material cards and relative `*Include` statements for the other four files.

Keep all five files in the same directory. The master file uses relative file names in its `*Include` statements. The include order is nodes, elements, sections, and element sets. Abaqus accepts the forward reference from each `*Solid Section` entry to the element set in the later element-set include file.

### Required Input Sources

- **Image Geometry** defines the voxel grid. The mesh equals this grid. Node coordinates use the geometry origin and spacing, and they use the geometry length units.
- **Cell Feature Ids** is an `int32` scalar array. Each positive value assigns one element to a grain.
- **Cell Euler Angles** is a `float32` array with three components. Input angles are in radians. The filter writes the angles in degrees.
- **Cell Phases** is an `int32` scalar array. Each value assigns the cell to a phase.

The three selected arrays must have one tuple for each cell in the **Image Geometry**.

### Material Card

The filter writes this legacy material-card structure for each grain:

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

`*Depvar` gives the number of solution-dependent state variables that the user material uses. `*User Material, constants = 5 + N` gives the total number of constants. The first five constants are the grain ID, phase ID, and three Euler angles in degrees. The filter then writes the *N* values from *Material Constants*. The filter writes a maximum of eight constants on each line. `*User Output Variables` gives the number of user output variables.

Each row in *Material Constants* must contain exactly one value. The table can contain zero or more rows.

### Behavior Notes

The filter reads cells in linear index order. If multiple cells have the same positive feature ID, the last cell supplies the phase and Euler angles for that grain.

Feature ID *0* and negative feature IDs are skipped. They do not occur in an element set and do not supply grain data.

The filter writes one grain for each ID in the range `[1, maximum positive feature ID]`. If an ID in this range has no cells, the filter writes an empty element set. The filter also writes a phase *0* material with a zero orientation and sends one warning. If the input has no positive feature IDs, the filter returns an error and does not write files.

The output directory must exist before execution. The legacy DREAM.3D 6.6 filter created the directory when it did not exist.

The DREAM.3D 6.6 writer used the `_set` suffix in the material-card name. Its section reference used the `_mat` suffix. This filter uses `_mat` in both locations, so the material reference is valid. Element-set names continue to use `_set`.

### Memory

The temporary element-bucket storage uses 8 bytes for each cell with a positive feature ID. The per-grain phase, orientation, count, and offset storage uses about 32 bytes for each grain.

% Auto generated parameter table will be inserted here

## Example Output

This example uses a `2 x 2 x 1` **Image Geometry** with origin `(0, 0, 0)` and spacing `(0.5, 0.5, 0.5)`. Feature IDs are `{1, 1, 2, 2}`. Phases are `{1, 1, 2, 2}`. The output prefix is `Abaqus_CP_Test`.

`Abaqus_CP_Test_nodes.inp`:

```text
*NODE, NSET=ALLNODES
1, 0.000, 0.000, 0.000
2, 0.500, 0.000, 0.000
3, 1.000, 0.000, 0.000
4, 0.000, 0.500, 0.000
5, 0.500, 0.500, 0.000
6, 1.000, 0.500, 0.000
7, 0.000, 1.000, 0.000
8, 0.500, 1.000, 0.000
9, 1.000, 1.000, 0.000
10, 0.000, 0.000, 0.500
11, 0.500, 0.000, 0.500
12, 1.000, 0.000, 0.500
13, 0.000, 0.500, 0.500
14, 0.500, 0.500, 0.500
15, 1.000, 0.500, 0.500
16, 0.000, 1.000, 0.500
17, 0.500, 1.000, 0.500
18, 1.000, 1.000, 0.500
```

`Abaqus_CP_Test_elems.inp`:

```text
*ELEMENT, TYPE=C3D8R, ELSET=ALLELEMENTS
1, 1, 2, 5, 4, 10, 11, 14, 13
2, 2, 3, 6, 5, 11, 12, 15, 14
3, 4, 5, 8, 7, 13, 14, 17, 16
4, 5, 6, 9, 8, 14, 15, 18, 17
```

`Abaqus_CP_Test_sects.inp`:

```text
*Solid Section, elset=Grain1_Phase1_set, material=Grain1_Phase1_mat
*Solid Section, elset=Grain2_Phase2_set, material=Grain2_Phase2_mat
```

`Abaqus_CP_Test_elset.inp`:

```text
*Elset, elset=Grain1_Phase1_set
1, 2
*Elset, elset=Grain2_Phase2_set
3, 4
```

`Abaqus_CP_Test.inp`:

```text
*Heading
UnitTest
** Job name : UnitTest
*Preprint, echo = NO, model = NO, history = NO, contact = NO
**
*Include, Input = Abaqus_CP_Test_nodes.inp
*Include, Input = Abaqus_CP_Test_elems.inp
*Include, Input = Abaqus_CP_Test_sects.inp
*Include, Input = Abaqus_CP_Test_elset.inp
**
*Material, name = Grain1_Phase1_mat
*Depvar
3
*User Material, constants = 7
1, 1, 0.000, 90.000, 180.000, 1.500, 2.250
*User Output Variables
2
*Material, name = Grain2_Phase2_mat
*Depvar
3
*User Material, constants = 7
2, 2, 45.000, 30.000, 60.000, 1.500, 2.250
*User Output Variables
2
```

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
