# Write Abaqus Hexahedron

## Group (Subgroup)

IO (Output)

## Description

This filter converts a voxel grid (an **Image Geometry**) into a hexahedral element mesh and writes it to a set of Abaqus input (`.inp`) files. **Abaqus** is a commercial *finite-element-analysis* (FEA) software package; FEA is a numerical technique that divides a body into many small, simple elements and solves for quantities such as stress and strain across them. By exporting the microstructure as an Abaqus mesh, the filter lets users run mechanical (stress/strain) simulations on a digital microstructure.

Each **Cell** (voxel) in the **Image Geometry** becomes one hexahedral element with 8 corner nodes. Adjacent elements share nodes. By default, the filter writes reduced-integration `C3D8R` elements. Disable *Use Reduced Integration Elements* to write standard `C3D8` elements instead.

The filter uses *Cell Feature Ids* and *Cell Phases* to create one phase-qualified element set and solid section for each grain. The filter does not write material definitions. Add definitions for the referenced `Grain<id>_Phase<phase>_mat` materials before Abaqus runs the deck.

![Fig. 1: Each voxel is exported as an 8-node C3D8R or C3D8 hexahedral element. Adjacent elements share nodes.](Images/WriteAbaqusHexahedron_VoxelToHex.png)

### Output Files

The filter writes five plain-text files. The first is a master file that pulls in the other four using Abaqus's `*Include` directive, so the simulation is launched from the master file alone:

- `<prefix>.inp` — the master file. It `*Include`s the four files below.
- `<prefix>_nodes.inp` — the node coordinates.
- `<prefix>_elems.inp` — the element connectivity (which 8 nodes make up each element).
- `<prefix>_sects.inp` — the section definitions that assign a material name to each grain set.
- `<prefix>_elset.inp` — one phase-qualified element set for each grain.

Node coordinates use the physical units of the **Image Geometry**. Each coordinate uses the geometry spacing and origin and has three fractional digits.

### Write Dummy Node

When the *Write Dummy Node* parameter is enabled, the filter appends one extra "dummy" node at the end of the `_nodes.inp` file. This node is not attached to any element; it exists only as a reference point that some Abaqus stress/strain workflows require (for example, to apply or read periodic boundary conditions). If the export is not being used for stress/strain curves, this parameter should be disabled, because the unused node can interfere with mesh connectivity tools that expect every node to belong to an element.

### Hourglass Stiffness

When *Use Reduced Integration Elements* is enabled, the `_sects.inp` file includes an `*Hourglass Stiffness` value for each grain section. "Hourglass" modes are spurious, zero-energy deformation patterns that reduced-integration brick elements can exhibit; a small artificial stiffness suppresses them. The *Hourglass Stiffness Value* parameter sets this number (default *250*, a dimensionless solver-control value). Standard `C3D8` elements do not use this value, so the filter omits the hourglass records when reduced integration is disabled.

### Required Input Sources

- **Image Geometry** -- the voxel grid to export.
- **Cell Feature Ids** -- a scalar `int32` array that assigns each positive cell ID to a grain. A segmentation filter such as [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md) can create this array.
- **Cell Phases** -- a scalar `int32` array that assigns a phase to each cell.

Both arrays must contain one tuple for each cell in the **Image Geometry**. For a grain that contains more than one cell, the last cell in linear index order supplies the phase ID.

Feature ID *0* and negative Feature IDs remain mesh elements but do not occur in grain element sets. The `_elems.inp` header assigns every element to `ALLELEMENTS`.

### Example Output

The filter is based on a Python script developed by Matthew W. Priddy at Georgia Tech in early 2015.

The master file:

```text
*Heading
Job 101
** Job name : Job 101
*Preprint, echo = NO, model = NO, history = NO, contact = NO
**
*Include, Input = 32x32x32_nodes.inp
*Include, Input = 32x32x32_elems.inp
*Include, Input = 32x32x32_sects.inp
*Include, Input = 32x32x32_elset.inp
**
```

The `_nodes.inp` file (node ID followed by x, y, z in geometry units):

```text
*NODE, NSET=ALLNODES
1, 0.000, 0.000, 0.000
2, 0.500, 0.000, 0.000
3, 1.000, 0.000, 0.000
   ..
```

The `_elems.inp` file (element ID followed by its 8 node IDs):

```text
*ELEMENT, TYPE=C3D8R, ELSET=ALLELEMENTS
1, 1, 2, 35, 34, 1090, 1091, 1124, 1123
2, 2, 3, 36, 35, 1091, 1092, 1125, 1124
   ..
```

The `_elset.inp` file contains one phase-qualified set for each grain. The full sets contain many element IDs and are abbreviated here:

```text
*Elset, elset=Grain1_Phase1_set
24126, 24127, 24128, 24159, 24160, 24191, 24192, 25085, 25086, ..
*Elset, elset=Grain2_Phase1_set
   ..
```

The `_sects.inp` file for reduced-integration elements (one section per grain, each with its hourglass stiffness):

```text
*Solid Section, elset=Grain1_Phase1_set, material=Grain1_Phase1_mat
*Hourglass Stiffness
250
*Solid Section, elset=Grain2_Phase1_set, material=Grain2_Phase1_mat
   ..
```

### Migration from DREAM.3D 6.5.171 and Earlier SIMPLNX Releases

Converted legacy pipelines do not contain a *Cell Phases* selection. Select this array before execution. Converted pipelines retain standard `C3D8` integration, while newly created pipelines default to `C3D8R`.

The common Abaqus mesh format changes headers, coordinate precision, local node ordering, names, and include order. See [WriteAbaqusHexahedronFilter deviations](../vv/deviations/WriteAbaqusHexahedronFilter.md) for migration details.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
