# Compute IPF Colors (Face)

## Group (Subgroup)

Processing (Crystallography)

## Description

This **Filter** generates a pair of colors for each **Triangle** in a **Triangle Geometry** based on the inverse pole figure (IPF) color scheme for the present crystal structure. Each **Triangle** has 2 colors since any **Face** sits at a boundary between 2 **Features** for a well-connected set of **Features** that represent _grains_. The reference direction used for the IPF color generation is the _normal_ of the **Triangle**.

Each side of a **Face** is colored using **that side's own** feature: the first color uses the first feature's orientation, its phase's crystal symmetry (Laue group), and the face normal; the second color uses the second feature's orientation, **its own** phase's crystal symmetry, and the negated (inward) face normal. This matters for multi-phase microstructures where the two features adjacent to a **Face** belong to phases with different Laue groups (for example a cubic phase against a hexagonal phase) — each side is colored with the correct symmetry for its phase. A side whose feature is invalid (a boundary/exterior face, feature id ≤ 0) is colored black.

> **Note on migration from DREAM3D 6.5.171:** earlier DREAM3D 6.5.171 colored the *second* side of every **Face** using the *first* feature's crystal symmetry, which produced incorrect colors on mixed-Laue-group faces and left the second color black on faces whose first side was the exterior. DREAM3D-NX (and DREAM3D 6.5.172) compute each side with its own phase's symmetry. See V&V deviation `ComputeFaceIPFColoringFilter-D1`.

------------

![Face IPF Coloring](Images/ComputeFaceIPFColoring.png)

------------

### Crystallographic Convention (Hexagonal & Trigonal)

The IPF color itself is **convention-independent** for hexagonal/trigonal phases (the X‖a (TSL/EDAX) and X‖a\* (MTEX/Oxford) bases differ only by a 30° rotation about the c-axis, which does not change the RGB). Only the *labeling* of the inverse-pole-figure triangle corners depends on the convention; the IPF legends shipped with DREAM3D-NX are drawn in the **X‖a (TSL/EDAX)** convention (apex `[0001]` = red, the a-axis `[2-1-10]` = green, the m-axis `[10-10]` = blue). See the **Compute IPF Colors** filter documentation for the full corner→color table.

------------

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (07) Small IN100 Mesh Statistics

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
