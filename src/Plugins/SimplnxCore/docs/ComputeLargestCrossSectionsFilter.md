# Compute Feature Largest Cross-Section Areas

## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** calculates the largest cross-sectional area on a user-defined plane for all **Features**.  The **Filter** simply iterates through all **Cells** (on each section) asking for **Feature** that owns them.  On each section, the count of **Cells** for each **Feature** is then converted to an area and stored as the *LargestCrossSection* if the area for the current section is larger than the existing *LargestCrossSection* for that **Feature**.

### Plane of Interest

The *Plane of Interest* parameter selects the plane along which cross-sections are computed. The filter iterates through all slices perpendicular to the remaining axis:

- **XY**: Cross-sections are taken on planes perpendicular to the Z axis. Each Z slice is one cross-section.
- **XZ**: Cross-sections are taken on planes perpendicular to the Y axis. Each Y slice is one cross-section.
- **YZ**: Cross-sections are taken on planes perpendicular to the X axis. Each X slice is one cross-section.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
