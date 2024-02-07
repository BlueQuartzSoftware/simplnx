# Fuse Regular Grids (Nearest Points)

## Group (Subgroup)

Sampling (Resolution)

## Description

This **Filter** fuses two **Image Geometry** data sets together. The grid of **Cells** in the *Reference* **Data Container** is overlaid on the grid of **Cells** in the *Sampling* **Data Container**.  Each **Cell** in the *Reference* **Data Container** is associated with the nearest point in the *Sampling* **Data Container** (i.e., no *interpolation* is performed).  All the attributes of the **Cell** in the *Sampling* **Data Container** are then assigned to the **Cell** in the *Reference* **Data Container**.  Additional to the **Cell** attributes being copied, all **Feature and Ensemble Attribute Matrices** from the *Sampling* **Data Container** are copied to the *Reference* **Data Container**.

*Note:* The *Sampling* **Data Container** remains identical after this **Filter**, but the *Reference* **Data Container**, while "geometrically identical", gains all the attribute arrays from the *Sampling* **Data Container**.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
