# Read Deform Key File (v12)

## Group (Subgroup)

SimulationIO (SimulationIO)

## Description

This filter reads a **DEFORM** version 12 "key file" and imports its mesh and field data into a newly created **Quad Geometry**. **DEFORM** is a commercial finite-element simulation package for metal-forming processes (such as forging, extrusion, and rolling). A DEFORM *key file* is the text results file the software exports; it contains the simulation mesh together with the values of physical variables computed at each node and element.

### What the Filter Creates

The filter builds a **Quad Geometry** — a surface mesh made of four-sided (quadrilateral) elements — from the key file. The **Quad Geometry** holds:

- **Node coordinates**: the positions of the mesh nodes (vertices).
- **Connectivity**: which four nodes form each quadrilateral element.

Alongside the geometry, the filter creates two attribute groups for the field variables stored in the key file:

- **Vertex (node) data**: variables defined at each node, such as `ndtmp` (nodal temperature).
- **Cell (element) data**: variables defined at each element, such as stress and strain.

The units of each imported variable are whatever DEFORM exported them in (for example temperature in degrees Celsius, or stress in MPa); the filter does not convert units.

### Example Input Structure

A DEFORM v12 key file is a keyword-driven text file. Each block begins with a keyword followed by the data for that block — for example, a node-coordinate block, an element-connectivity block, and one block per nodal or element variable:

```text
RZ
   1   0.000000   0.000000
   2   0.500000   0.000000
   ..
ELMCON
   1   1   2   5   4
   ..
NDTMP
   1   20.000000
   2   20.000000
   ..
```

### Required Input Sources

None — this filter reads directly from a DEFORM v12 key file on disk.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
