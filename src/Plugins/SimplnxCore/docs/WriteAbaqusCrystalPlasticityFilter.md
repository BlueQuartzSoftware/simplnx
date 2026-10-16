# Write Abaqus Crystal Plasticity File

## Group (Subgroup)

IO (Output)

## Description

This filter writes an Abaqus input deck for a crystal-plasticity analysis that uses a user material, such as a UMAT. The filter converts an **Image Geometry** to a finite-element mesh. By default, each **Cell** becomes one eight-node, standard-integration brick element of type `C3D8`. Enable *Use Reduced Integration Elements* to write reduced-integration `C3D8R` elements instead. Adjacent elements share nodes.

The filter writes five Abaqus input (`.inp`) files:

- `<prefix>_nodes.inp` contains the node coordinates.
- `<prefix>_elems.inp` contains the `C3D8R` or `C3D8` element connectivity.
- `<prefix>_elset.inp` contains the `cube` element set spanning all cells and one grain element set for each feature ID from *1* through the maximum positive feature ID.
- `<prefix>_sects.inp` contains the solid-section definitions that assign materials to grain element sets. For `C3D8R` elements, it also contains the hourglass stiffness value.
- `<prefix>.inp` is the master file. It contains the material cards and relative `*Include` statements for the other four files.

Keep all five files in the same directory. The master file uses relative file names in its `*Include` statements. The include order is nodes, elements, element sets, and sections. Element sets are included before sections so that each `*Solid Section` refers to an already defined set. The full-model element set is named `cube`; the node and element headers do not create `ALLNODES` or `ALLELEMENTS` sets.

The *Output File Prefix* must be a nonempty plain file name without spaces, other whitespace, or path separators (`/` or `\`). It cannot be `.` or `..`. Preflight also rejects any of the five target file paths that is an existing directory. Choose another output path or prefix, or remove the conflicting directory.

### Required Input Sources

- **Image Geometry** defines the voxel grid. The mesh equals this grid. Node coordinates use the geometry origin and spacing, and they use the geometry length units.
- **Cell Feature Ids** is an `int32` scalar array. Each positive value assigns one element to a grain.
- **Cell Euler Angles** is a `float32` array with three components. Input angles are in radians. The filter writes the angles in radians by default. Enable *Write Euler Angles in Degrees* to convert them to degrees for output; the input must still be in radians.
- **Cell Phases** is an `int32` scalar array. Each value assigns the cell to a phase.

Coordinates are computed in double precision (`float64`) and written with 9 significant digits, so very small spacings (for example `1e-7`) keep their value.

The three selected arrays must have one tuple for each cell in the **Image Geometry**. Feature IDs can be produced by a segmentation filter such as [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md). Cell Euler angles and phases can be read together by [Read EDAX EBSD Data (.ang)](../OrientationAnalysis/ReadAngDataFilter.md).

### Integration Type and Hourglass Stiffness

Enable *Use Reduced Integration Elements* to write `C3D8R` elements. Reduced integration can introduce spurious zero-energy deformation patterns called hourglass modes. The filter writes an `*Hourglass Stiffness` value for each grain section to control these modes. The *Hourglass Stiffness Value* parameter sets the stiffness in the model's stress units, such as Pa or MPa. Its default is 250.

The default standard `C3D8` elements use full integration and do not use hourglass stiffness. When *Use Reduced Integration Elements* is disabled, the filter writes `C3D8` and omits all `*Hourglass Stiffness` records.

### Material Card

The filter writes one material card for each grain. This card is copied from `Abaqus_CP_Test.inp`, produced by the *Synthetic Two Grain* test described below:

```text
*Material, name=Grain1_Phase1_mat
*Depvar
3
*User Material, constants=7, unsymm
1, 1, 0, 1.5707964, 3.1415927, 1.5, 2.25
*User Output Variables
2
```

`*Depvar` gives the number of solution-dependent state variables that the user material uses. `*User Output Variables` gives the number of user output variables. Both are nonnegative counts.

*Material Constants* requires at least one row and has exactly one column. Each row supplies one user constant, in table order. Labels such as "User Constant 1" identify table rows. The final position in the `*User Material` list depends on *Include Grain and Phase IDs as Constants* and *Euler Angles Start Index*. The units and meaning of these values depend on the user material. Material constants and Euler angles are written at full precision, with a maximum of eight values per line.

*Include Grain and Phase IDs as Constants* is enabled by default. It places the grain ID and phase ID at constant positions *1* and *2*. These IDs are dimensionless. Disable this option if the user material does not expect the IDs.

*Euler Angles Start Index* is the 1-based position of the first Euler angle in the complete constant list. The three Euler angles occupy this position and the next two, and the user constants retain their table order around them. Let `p` be *2* when grain and phase IDs are included, or *0* when they are excluded, and let `N` be the number of rows in *Material Constants*. The valid range is `p + 1 <= Euler Angles Start Index <= p + N + 1`. An index outside this range fails preflight with error `-12017`. The complete list contains `p + N + 3` constants.

The default start index of *3*, with IDs included, writes the grain ID, phase ID, three Euler angles, then the user constants. With two user constants, the valid start indices are *3* through *5*. To write Euler angles first without IDs, disable *Include Grain and Phase IDs as Constants* and set the start index to *1*; at least one user constant is still required.

*Use Unsymmetric Solver* is enabled by default. It appends `unsymm` to each `*User Material` line to request Abaqus's unsymmetric equation solver, which is recommended for crystal plasticity. Disable it to omit `unsymm`.

For a layout with Euler angles at positions *17–19*, disable *Include Grain and Phase IDs as Constants*, enter 17 user constants with values *10, 20, ..., 170*, and set *Euler Angles Start Index* to *17*. With `p = 0` and `N = 17`, the valid range is *1* through *18*, and the complete list has 20 constants. For Euler angles `(0.25, 0.5, 0.75)` radians, the following card is copied from `Layout_17.inp`, produced by the *Custom Constant Layout* test. The final user constant, *170*, follows the angles at position *20*:

```text
*Material, name=Grain1_Phase1_mat
*Depvar
3
*User Material, constants=20, unsymm
10, 20, 30, 40, 50, 60, 70, 80
90, 100, 110, 120, 130, 140, 150, 160
0.25, 0.5, 0.75, 170
*User Output Variables
2
```

### Behavior Notes

The filter reads cells in linear index order. If multiple cells have the same positive feature ID, the last cell supplies the phase and Euler angles for that grain.

Feature ID *0* and negative feature IDs are skipped when assigning grains. Their elements remain in the full-model `cube` set, but they do not occur in a grain element set and do not supply grain data.

The filter writes one grain for each ID in the range `[1, maximum positive feature ID]`. If an ID in this range has no cells, the filter writes an empty element set. The filter also writes a phase *0* material with a zero orientation and sends one warning. If the input has no positive feature IDs, the filter returns an error and does not write files.

If the output directory does not exist, preflight reports a warning and the filter creates the directory during execution.

The DREAM.3D 6.6 writer used the `_set` suffix in the material-card name. Its section reference used the `_mat` suffix. This filter uses `_mat` in both locations, so the material reference is valid. Element-set names continue to use `_set`.

When converting a legacy SIMPL pipeline, the filter enables reduced integration (`C3D8R`) and *Write Euler Angles in Degrees*, disables *Use Unsymmetric Solver*, and includes grain and phase IDs with *Euler Angles Start Index* set to *3*. Conversion preserves the legacy element type, angle units, solver flag, and constant layout. The legacy writer wrote `C3D8R` elements without any `*Hourglass Stiffness` record. The converted pipeline additionally writes `*Hourglass Stiffness` with the default value 250 after each `*Solid Section`. Users who want the exact legacy section file can disable *Use Reduced Integration Elements* only if they also accept `C3D8` elements. There is no option for `C3D8R` without hourglass stiffness.

### Memory

Per-grain tables for phase, orientation, and the has-cells flag use about 17 bytes per feature ID through the maximum ID. In-core element-set grouping keeps one element index (8 bytes) per cell with a positive feature ID, plus a small vector per grain.

On out-of-core stores, the writer reads all arrays in chunks of 65,536 cells. It sorts `(feature ID, element ID)` records with the registered bounded external-sort provider. This bounds the cell-processing memory so it does not grow with the cell count. The per-grain tables still depend on the maximum feature ID.

The maximum feature ID is limited to 100,000,000. Larger IDs cause an error during execution; renumber the features to be contiguous. The node and element label limit is 999,999,999. Mesh counts above this limit cause a preflight error.

% Auto generated parameter table will be inserted here

## Example Output

These files are copied from the *Synthetic Two Grain* test output. The example uses a `2 x 2 x 1` **Image Geometry** with origin `(0, 0, 0)` and spacing `(0.5, 0.5, 0.5)` in the geometry's length units. Feature IDs are `{1, 1, 2, 2}`. Phases are `{1, 1, 2, 2}`. The output prefix is `Abaqus_CP_Test`. The test explicitly enables reduced integration and sets *Hourglass Stiffness Value* to *417*. It uses radians, the unsymmetric solver, grain and phase IDs, and Euler angles starting at constant *3*. The two user constants are *1.5* and *2.25*, with 3 solution-dependent state variables and 2 user output variables.

`Abaqus_CP_Test_nodes.inp` uses coordinates computed in `float64` and written with 9 significant digits:

```text
** ----------------------------------------------------------------
**
*Node
1, 0, 0, 0
2, 0.5, 0, 0
3, 1, 0, 0
4, 0, 0.5, 0
5, 0.5, 0.5, 0
6, 1, 0.5, 0
7, 0, 1, 0
8, 0.5, 1, 0
9, 1, 1, 0
10, 0, 0, 0.5
11, 0.5, 0, 0.5
12, 1, 0, 0.5
13, 0, 0.5, 0.5
14, 0.5, 0.5, 0.5
15, 1, 0.5, 0.5
16, 0, 1, 0.5
17, 0.5, 1, 0.5
18, 1, 1, 0.5
**
** ----------------------------------------------------------------
**
```

`Abaqus_CP_Test_elems.inp`:

```text
** ----------------------------------------------------------------
**
*Element, type=C3D8R
1, 1, 2, 5, 4, 10, 11, 14, 13
2, 2, 3, 6, 5, 11, 12, 15, 14
3, 4, 5, 8, 7, 13, 14, 17, 16
4, 5, 6, 9, 8, 14, 15, 18, 17
**
** ----------------------------------------------------------------
**
```

`Abaqus_CP_Test_elset.inp`:

```text
** ----------------------------------------------------------------
**
** The element sets
*Elset, elset=cube, generate
1, 4, 1
**
** Each Grain is made up of multiple elements
**
*Elset, elset=Grain1_Phase1_set
1, 2
*Elset, elset=Grain2_Phase2_set
3, 4
**
** ----------------------------------------------------------------
**
```

`Abaqus_CP_Test_sects.inp`:

```text
** ----------------------------------------------------------------
**
** Each section is a separate grain
** Section: Grain1_Phase1
*Solid Section, elset=Grain1_Phase1_set, material=Grain1_Phase1_mat
*Hourglass Stiffness
417
** --------------------------------------
** Section: Grain2_Phase2
*Solid Section, elset=Grain2_Phase2_set, material=Grain2_Phase2_mat
*Hourglass Stiffness
417
** --------------------------------------
**
** ----------------------------------------------------------------
**
```

`Abaqus_CP_Test.inp`:

```text
*Heading
UnitTest
** Job name : UnitTest
*Preprint, echo = NO, model = NO, history = NO, contact = NO
**
** ----------------------------Geometry----------------------------
**
*Include, Input = Abaqus_CP_Test_nodes.inp
*Include, Input = Abaqus_CP_Test_elems.inp
*Include, Input = Abaqus_CP_Test_elset.inp
*Include, Input = Abaqus_CP_Test_sects.inp
**
** ----------------------------Materials---------------------------
**
*Material, name=Grain1_Phase1_mat
*Depvar
3
*User Material, constants=7, unsymm
1, 1, 0, 1.5707964, 3.1415927, 1.5, 2.25
*User Output Variables
2
*Material, name=Grain2_Phase2_mat
*Depvar
3
*User Material, constants=7, unsymm
2, 2, 0.7853982, 0.5235988, 1.0471976, 1.5, 2.25
*User Output Variables
2
**
** ----------------------------------------------------------------
**
```

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
